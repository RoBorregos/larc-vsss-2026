#include "blob_calibrator.h"

#include "image_preprocessing.h"

BlobCalibrator::BlobCalibrator(GUI* gui, AppData* app_data) : app_data(app_data) {
	this->gui = gui;
}

void BlobCalibrator::on_mouse(const int event, const int x, const int y, int flags, void* userdata) {
	auto* self = static_cast<BlobCalibrator*>(userdata);
	if (x < 0 || y < 0 || x >= self->gui->get_image().cols || y >= self->gui->get_image().rows)
		return;

	if (event == cv::EVENT_LBUTTONDOWN) {
		const cv::Point real_coords = self->gui->screen_to_world(cv::Point(x, y));
		self->handle_click(real_coords.x, real_coords.y);
	}
}

void BlobCalibrator::handle_click(int x, int y) {
	if (app_data->current_state != AppState::BLOB_CALIBRATING) return;
    if (points.size() >= 4) return;
    points.emplace_back(x, y);
}

void BlobCalibrator::reset_points() {
	points.clear();
}

void BlobCalibrator::print_calibrations(const MatchCalibration &calibration) {
	std::cout << std::fixed << std::setprecision(2);
    auto print_channel = [](const std::string& name, const std::vector<CalibrationResult>& calibration_vector) {
	    if (calibration_vector.empty()) {
	    	std::cout << ">>> " << name << ": [NO CALIBRADO]" << std::endl;
	    	return;
	    }

    	for (const auto & res : calibration_vector) {
    		std::cout << ">>> " << name << ":" << std::endl;

    		std::cout << "    Avg HSV: ["
					<< res.avg_hsv[0] << ", "
					<< res.avg_hsv[1] << ", "
					<< res.avg_hsv[2] << "]" << std::endl;

    		std::cout << "    Std Dev: ["
					<< res.std_dev_hsv[0] << ", "
					<< res.std_dev_hsv[1] << ", "
					<< res.std_dev_hsv[2] << "]" << std::endl;

    		std::cout << "    Min HSV: ["
					<< res.min_hsv[0] << ", "
					<< res.min_hsv[1] << ", "
					<< res.min_hsv[2] << "]" << std::endl;

    		std::cout << "    Max HSV: ["
					<< res.max_hsv[0] << ", "
					<< res.max_hsv[1] << ", "
					<< res.max_hsv[2] << "]" << std::endl;

    		std::cout << "------------------------------------------------" << std::endl;
    	};
    };

    std::cout << "\n========== TABLA DE CALIBRACION ACTUAL ==========" << std::endl;
    print_channel("BLUE (Azul)", calibration.blue);
    print_channel("YELLOW (Amarillo)", calibration.yellow);
    print_channel("ORANGE (Naranja)", calibration.orange);
    print_channel("GREEN (Verde)", calibration.green);
    print_channel("MAGENTA", calibration.magenta);
    print_channel("CYAN", calibration.cyan);
    print_channel("RED (Rojo)", calibration.red);
    std::cout << "=================================================\n" << std::endl;
	std::cout.unsetf(std::ios_base::floatfield);
}



std::optional<CalibrationResult> BlobCalibrator::calibrate_individual() {
    for (const auto point : points) gui->plot(point, Drawer::Layer::MARKINGS);
    if (points.size() == 4)    gui->closed_polyline(points, Drawer::Layer::MARKINGS);
    else if (points.size() > 1)    gui->polyline(points, Drawer::Layer::MARKINGS);

    if (points.size() != 4) return std::nullopt;

	cv::cuda::GpuMat d_hsv_image = gui->get_image(cv::COLOR_BGR2HSV);

	cv::Rect roi_rect = cv::boundingRect(points);
	roi_rect = roi_rect & cv::Rect(0, 0, d_hsv_image.cols, d_hsv_image.rows);
	if (roi_rect.empty()) return std::nullopt;

	cv::cuda::GpuMat d_cropped_hsv = d_hsv_image(roi_rect);

	cv::Mat mask = cv::Mat::zeros(roi_rect.size(), CV_8UC1);
	std::vector<cv::Point> offset_points;
	for (const auto& p : points) {
		offset_points.push_back(p - roi_rect.tl());
	}
	cv::fillConvexPoly(mask, offset_points, cv::Scalar(255));


	cv::Mat computation_mask;
	int margin_px = 5;
	int kernel_size = 2 * margin_px + 1;
	cv::Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(kernel_size, kernel_size));
	cv::erode(mask, computation_mask, element);

    if (cv::countNonZero(computation_mask) == 0) {
       computation_mask = mask;
    }

	cv::cuda::GpuMat d_mask;
	d_mask.upload(computation_mask);

	BlobStats* d_stats = nullptr;
	cudaMalloc(&d_stats, sizeof(BlobStats));

	launchBlobKernel(d_cropped_hsv, d_mask, d_stats);

	BlobStats h_stats;
	cudaMemcpy(&h_stats, d_stats, sizeof(BlobStats), cudaMemcpyDeviceToHost);
	cudaFree(d_stats);

	if (h_stats.count == 0) return std::nullopt;
	bool use_wrap = (h_stats.count_low_red > 0 && h_stats.count_high_red > 0);

	unsigned long long sum_h = use_wrap ? h_stats.sum_h_wrap : h_stats.sum_h;
	unsigned long long sq_sum_h = use_wrap ? h_stats.sq_sum_h_wrap : h_stats.sq_sum_h;
	int min_h = use_wrap ? h_stats.min_h_wrap : h_stats.min_h;
	int max_h = use_wrap ? h_stats.max_h_wrap : h_stats.max_h;

	double count = static_cast<double>(h_stats.count);

	auto calc_stat = [&](unsigned long long sum, unsigned long long sq_sum) {
		double avg = sum / count;
		double var = (sq_sum / count) - (avg * avg);
		double std_dev = std::sqrt(std::max(0.0, var));
		return std::make_pair(avg, std_dev);
	};

	auto [avg_h, std_h] = calc_stat(sum_h, sq_sum_h);
	auto [avg_s, std_s] = calc_stat(h_stats.sum_s, h_stats.sq_sum_s);
	auto [avg_v, std_v] = calc_stat(h_stats.sum_v, h_stats.sq_sum_v);

	if (avg_h >= 180.0) avg_h -= 180.0;

	CalibrationResult result;
	result.valid = true;
	result.pixel_count = h_stats.count;
	result.points = points;
	result.frame_id = 0;
	result.min_hsv = cv::Scalar(min_h, h_stats.min_s, h_stats.min_v);
	result.max_hsv = cv::Scalar(max_h, h_stats.max_s, h_stats.max_v);
	result.avg_hsv = cv::Scalar(avg_h, avg_s, avg_v);
	result.std_dev_hsv = cv::Scalar(std_h, std_s, std_v);
	return result;
}

std::vector<double> BlobCalibrator::scalar_to_json(const cv::Scalar &s) {
	return {s[0], s[1], s[2]};
}


cv::Scalar BlobCalibrator::json_to_scalar(const std::vector<double> &v) {
	if (v.size() < 3) return {0,0,0};
	return {v[0], v[1], v[2]};
}

std::vector<cv::Point> BlobCalibrator::json_to_points(const std::vector<std::vector<int> > &v) {
	std::vector<cv::Point> out;
	out.reserve(v.size());
	for (const auto& p : v) {
		if (p.size() >= 2) out.emplace_back(p[0], p[1]);
	}
	return out;
}

std::vector<std::vector<int> > BlobCalibrator::points_to_json(const std::vector<cv::Point> &pts) {
	std::vector<std::vector<int>> out;
	out.reserve(pts.size());
	for (const auto& p : pts) {
		out.push_back({p.x, p.y});
	}
	return out;
}

void BlobCalibrator::save_calibration(const std::string& filename) {
    json root;

    std::ifstream infile(filename);
    if (infile.is_open() && infile.peek() != std::ifstream::traits_type::eof()) {
        try {
            infile >> root;
        } catch (json::parse_error& e) {
            std::cerr << "[WARN] JSON corrupto al guardar, se sobrescribirá: " << e.what() << std::endl;
            root = json::object();
        }
    }
    infile.close();

    auto serialize_list = [&](const std::vector<CalibrationResult>& list) {
       json j_array = json::array();
       for (const auto& item : list) {
          json j_item;
          j_item["valid"]       = item.valid;
          j_item["pixel_count"] = item.pixel_count;
          j_item["min_hsv"]     = scalar_to_json(item.min_hsv);
          j_item["max_hsv"]     = scalar_to_json(item.max_hsv);
          j_item["avg_hsv"]     = scalar_to_json(item.avg_hsv);
          j_item["std_dev_hsv"] = scalar_to_json(item.std_dev_hsv);
          j_array.push_back(j_item);
       }
       return j_array;
    };

    root["yellow"]  = serialize_list(app_data->match_calibration.yellow);
    root["blue"]    = serialize_list(app_data->match_calibration.blue);
    root["magenta"] = serialize_list(app_data->match_calibration.magenta);
    root["red"]     = serialize_list(app_data->match_calibration.red);
    root["cyan"]    = serialize_list(app_data->match_calibration.cyan);
    root["green"]   = serialize_list(app_data->match_calibration.green);
    root["orange"]  = serialize_list(app_data->match_calibration.orange);

    std::ofstream outfile(filename);
    if (outfile.is_open()) {
       outfile << root.dump(4);
       outfile.close();
       std::cout << "[INFO] BlobCalibrator guardado en: " << filename << std::endl;
    } else {
       std::cerr << "[ERROR] No se pudo crear el archivo: " << filename << std::endl;
    }
}
void BlobCalibrator::load_calibration(const std::string &filename) {
	std::cout << "[DEBUG] Abriendo archivo: "
			  << filename << std::endl;

	std::ifstream file(filename);

	if (!file.is_open()) {
		std::cerr << "[WARN] No se pudo abrir el archivo (No existe o permisos)." << std::endl;
		return;
	}

	if (file.peek() == std::ifstream::traits_type::eof()) {
		std::cerr << "[ERROR] El archivo existe pero ESTÁ VACÍO (0 bytes)." << std::endl;
		return;
	}

	json root;

	try {
		file >> root;
	} catch (json::parse_error& e) {
		std::cerr << "[ERROR] JSON corrupto: " << e.what() << std::endl;
		return;
	}

	auto deserialize_list = [&](const std::string& key, std::vector<CalibrationResult>& list) {
		list.clear();
		if (!root.contains(key)) return;

		for (const auto& j_item : root[key]) {
			CalibrationResult item;

			item.valid       = j_item.value("valid", false);
			item.frame_id    = 0;
			item.pixel_count = j_item.value("pixel_count", 0ul);

			if(j_item.contains("min_hsv")) item.min_hsv = json_to_scalar(j_item["min_hsv"]);
			if(j_item.contains("max_hsv")) item.max_hsv = json_to_scalar(j_item["max_hsv"]);
			if(j_item.contains("avg_hsv")) item.avg_hsv = json_to_scalar(j_item["avg_hsv"]);
			if(j_item.contains("std_dev_hsv")) item.std_dev_hsv = json_to_scalar(j_item["std_dev_hsv"]);

			item.points = {};

			list.push_back(item);
		}
	};

	deserialize_list("yellow",  app_data->match_calibration.yellow);
	deserialize_list("blue",    app_data->match_calibration.blue);
	deserialize_list("magenta", app_data->match_calibration.magenta);
	deserialize_list("red",     app_data->match_calibration.red);
	deserialize_list("cyan",    app_data->match_calibration.cyan);
	deserialize_list("green",   app_data->match_calibration.green);
	deserialize_list("orange",  app_data->match_calibration.orange);

	std::cout << "[INFO] Calibración cargada exitosamente." << std::endl;
}
