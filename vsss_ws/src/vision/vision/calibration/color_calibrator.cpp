#include "color_calibrator.h"

ColorCalibrator::ColorCalibrator(GUI *gui, AppData* app_data) {
	this->app_data = app_data;
	this->gui = gui;
}

void ColorCalibrator::on_mouse(const int event, const int x, const int y, int flags, void* userdata) {
	auto* self = static_cast<ColorCalibrator*>(userdata);
	if (x < 0 || y < 0 || x >= self->gui->get_image().cols || y >= self->gui->get_image().rows)
		return;

	if (event == cv::EVENT_LBUTTONDOWN) {
		const cv::Point real_coords = self->gui->screen_to_world(cv::Point(x, y));
		self->handle_click(real_coords.x, real_coords.y);
	}
}

void ColorCalibrator::handle_click(int x, int y) {
	roi_points.emplace_back(x, y);
}

void ColorCalibrator::reset_points() {
	roi_points.clear();
}

void ColorCalibrator::calibrate_roi() {
	for (int i = 0; i < roi_points.size(); ++i) {
		const cv::Point& point = roi_points[i];

		if (i == 0) {
			gui->plot(point, Drawer::Layer::MARKINGS, {0, 255, 0});
		} else if (i == roi_points.size() - 1) {
			gui->plot(point, Drawer::Layer::MARKINGS, {255, 0, 0});
		} else {
			gui->plot(point, Drawer::Layer::MARKINGS);
		}
	}

	if (roi_points.size() > 1) {
		gui->closed_polyline(roi_points, Drawer::Layer::MARKINGS);
	}
}


void ColorCalibrator::remove_last_point() {
	if (!roi_points.empty()) {
		roi_points.pop_back();
	}
}

std::vector<cv::Point> ColorCalibrator::get_roi() const {
	return roi_points;
}

std::vector<cv::Point> ColorCalibrator::json_to_points(const std::vector<std::vector<int> > &v) {
	std::vector<cv::Point> out;
	out.reserve(v.size());
	for (const auto& p : v) {
		if (p.size() >= 2) out.emplace_back(p[0], p[1]);
	}
	return out;
}

std::vector<std::vector<int> > ColorCalibrator::points_to_json(const std::vector<cv::Point> &pts) {
	std::vector<std::vector<int>> out;
	out.reserve(pts.size());
	for (const auto& p : pts) {
		out.push_back({p.x, p.y});
	}
	return out;
}

void ColorCalibrator::save_calibration(const std::string &filename) {
	json root;

	std::ifstream infile(filename);
	if (infile.is_open() && infile.peek() != std::ifstream::traits_type::eof()) {
		try {
			infile >> root;
		} catch (json::parse_error& e) {
			std::cerr << "[ERROR]: JSON corrupto en " << filename << ": " << e.what() << std::endl;

			infile.close();

			std::string backupName = filename + ".corrupt";

			std::cerr << "[WARN]: Moving corrupt file to '" << backupName
					  << "' and generating new file." << std::endl;

			try {
				std::filesystem::rename(filename, backupName);
			} catch (std::filesystem::filesystem_error& fs_e) {
				std::cerr << "[ERROR] No se pudo crear el backup: " << fs_e.what() << std::endl;
				std::cerr << "[FATAL] Cannot safe-keep corrupt config data. Fix permissions or disk space." << std::endl;
				exit(EX_CANTCREAT);
			}

			root = json::object();
		}
	}
	infile.close();

	root["color_calibration"] = {
		{"saturation",          app_data->color_params.saturation},
		{"gamma_correction_s",    app_data->color_params.gamma_correction_s},
		{"gamma_correction_v",    app_data->color_params.gamma_correction_v},
		{"clahe_clip_limit",    app_data->color_params.clahe_clip_limit},
		{"bilateral_sigma",     app_data->color_params.bilateral_sigma},
		{"green_boost",         app_data->color_params.green_boost},
		{"blue_boost",          app_data->color_params.blue_boost},
		{"red_boost",           app_data->color_params.red_boost}
	};

	root["roi_points"] = points_to_json(roi_points);

	root["mask_params"] = {
		{"h_min", app_data->mask_params.h_min},
		{"s_min", app_data->mask_params.s_min},
		{"v_min", app_data->mask_params.v_min},
		{"h_max", app_data->mask_params.h_max},
		{"s_max", app_data->mask_params.s_max},
		{"v_max", app_data->mask_params.v_max}
	};

	std::ofstream outfile(filename);
	if (outfile.is_open()) {
		outfile << root.dump(4);
		outfile.close();
		std::cout << "[INFO] ColorCalibrator saved in: " << filename << std::endl;
	} else {
		std::cerr << "[ERROR]: Failed to write in: " << filename << std::endl;
		std::cerr << "[SYSTEM] " << std::strerror(errno) << std::endl;
		std::cerr << "[FATAL] Cannot create output file. Check directory permissions or disk space." << std::endl;

		exit(EX_CANTCREAT);
	}
}

void ColorCalibrator::load_calibration(const std::string &filename) {
	std::cout << "[DEBUG] Opening file: "
		  << filename << std::endl;

	std::ifstream file(filename);

	if (!file.is_open()) {
		std::cerr << "[Warn] Could not open file" << std::endl;
		return;
	}

	if (file.peek() == std::ifstream::traits_type::eof()) {
		std::cerr << "[ERROR] File exists but its empty: " << filename << std::endl;
		std::cerr << "[FATAL] Configuration file '" << filename << "' is empty/corrupt. Delete it to regenerate defaults or fix manually." << std::endl;
		exit(EX_CONFIG);
	}

	json root;

	try {
		file >> root;
	} catch (json::parse_error& e) {
		std::cerr << "[ERROR] Corrupt JSON: " << e.what() << std::endl;
		std::cerr << "[FATAL] Configuration file '" << filename
				  << "' contains invalid syntax. Verify structure or delete the file to reset." << std::endl;
		exit(EX_CONFIG);
	}

	if (root.contains("color_calibration")) {
		auto j_color = root["color_calibration"];
		app_data->color_params.saturation        = j_color.value("saturation",        app_data->color_params.saturation);
		app_data->color_params.gamma_correction_s= j_color.value("gamma_correction_s",app_data->color_params.gamma_correction_s);
		app_data->color_params.gamma_correction_v= j_color.value("gamma_correction_v",app_data->color_params.gamma_correction_v);
		app_data->color_params.clahe_clip_limit  = j_color.value("clahe_clip_limit",  app_data->color_params.clahe_clip_limit);
		app_data->color_params.bilateral_sigma   = j_color.value("bilateral_sigma",   app_data->color_params.bilateral_sigma);
		app_data->color_params.green_boost       = j_color.value("green_boost",       app_data->color_params.green_boost);
		app_data->color_params.blue_boost        = j_color.value("blue_boost",        app_data->color_params.blue_boost);
		app_data->color_params.red_boost         = j_color.value("red_boost",         app_data->color_params.red_boost);
		std::cout << "[INFO] Color parameters loaded from file." << std::endl;
	} else {
		std::cerr << "[WARN] Color calibration parameters not found in file." << std::endl;
		std::cout << "[INFO] Using default color parameters." << std::endl;
	}

	if (root.contains("mask_params")) {
		auto j_mask = root["mask_params"];
		app_data->mask_params.h_min = j_mask.value("h_min", app_data->mask_params.h_min);
		app_data->mask_params.s_min = j_mask.value("s_min", app_data->mask_params.s_min);
		app_data->mask_params.v_min = j_mask.value("v_min", app_data->mask_params.v_min);
		app_data->mask_params.h_max = j_mask.value("h_max", app_data->mask_params.h_max);
		app_data->mask_params.s_max = j_mask.value("s_max", app_data->mask_params.s_max);
		app_data->mask_params.v_max = j_mask.value("v_max", app_data->mask_params.v_max);
		std::cout << "[INFO] Mask parameters loaded from file." << std::endl;
	} else {
		std::cerr << "[WARN] Mask calibration parameters not found in file." << std::endl;
		std::cout << "[INFO] Using default mask parameters." << std::endl;
	}

	if (root.contains("roi_points")) {
		roi_points = json_to_points(root["roi_points"].get<std::vector<std::vector<int>>>());
		app_data->roi_points = roi_points;
		std::cout << "[INFO] ROI points loaded from file." << std::endl;
	} else {
		std::cerr << "[WARN] ROI points not found in file." << std::endl;
		std::cout << "[INFO] Defaulting to full image." << std::endl;
	}
}
