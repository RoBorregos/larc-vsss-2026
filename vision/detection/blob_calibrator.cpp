#include "blob_calibrator.h"

#include "image_preprocessing.h"

BlobCalibrator::BlobCalibrator(GUI* drawer, AppData* app_data) : app_data(app_data) {
	this->drawer = drawer;
}

void BlobCalibrator::on_mouse(const int event, const int x, const int y, int flags, void* userdata) {
	auto* self = static_cast<BlobCalibrator*>(userdata);
	if (x < 0 || y < 0 || x >= self->drawer->get_image().cols || y >= self->drawer->get_image().rows)
		return;

	if (event == cv::EVENT_LBUTTONDOWN) {
		const cv::Point real_coords = self->drawer->screen_to_world(cv::Point(x, y));
		self->handle_click(real_coords.x, real_coords.y);
	}
}

void BlobCalibrator::handle_click(int x, int y) {
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
	for (const auto point : points) drawer->plot(point);
	if (points.size() == 4)	drawer->closed_polyline(points);
	else if (points.size() > 1)	drawer->polyline(points);

	if (points.size() != 4) return std::nullopt;

	cv::Mat hsv_image = drawer->get_image(cv::COLOR_BGR2HSV);

	cv::Rect roi_rect = cv::boundingRect(points);
	roi_rect = roi_rect & cv::Rect(0, 0, hsv_image.cols, hsv_image.rows); // Prevent roi from image-overflow

	cv::Mat mask = cv::Mat::zeros(roi_rect.size(), CV_8UC1);
	std::vector<cv::Point> offset_points;
	for (const auto& p : points) {
		offset_points.push_back(p - roi_rect.tl());
	}

	cv::fillConvexPoly(mask, offset_points, cv::Scalar(255));

	cv::Mat computation_mask;
	int margin_px = 10;
	int kernel_size = 2 * margin_px + 1;
	cv::Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(kernel_size, kernel_size));
	cv::erode(mask, computation_mask, element);
	if (cv::countNonZero(computation_mask) == 0) {
		computation_mask = mask;
	}

	cv::Mat cropped_hsv = hsv_image(roi_rect);

	cv::Scalar avg_hsv, std_dev_hsv;
	cv::meanStdDev(cropped_hsv, avg_hsv, std_dev_hsv, computation_mask);

	std::vector<cv::Mat> channels;
	cv::split(cropped_hsv, channels);

	double min_h, max_h;
	double min_s, max_s;
	double min_v, max_v;

	cv::minMaxLoc(channels[0], &min_h, &max_h, nullptr, nullptr, computation_mask);
	cv::minMaxLoc(channels[1], &min_s, &max_s, nullptr, nullptr, computation_mask);
	cv::minMaxLoc(channels[2], &min_v, &max_v, nullptr, nullptr, computation_mask);

	CalibrationResult result;
	result.valid = true;
	result.min_hsv = cv::Scalar(min_h, min_s, min_v);
	result.max_hsv = cv::Scalar(max_h, max_s, max_v);
	result.avg_hsv = avg_hsv;
	result.std_dev_hsv = std_dev_hsv;

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

	std::ofstream file(filename);
	if (file.is_open()) {
		file << root.dump(4);
		file.close();
		std::cout << "[INFO] Calibración completa guardada en: " << filename << std::endl;
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
