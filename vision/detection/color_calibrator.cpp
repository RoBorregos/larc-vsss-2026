#include "color_calibrator.h"

ColorCalibrator::ColorCalibrator(GUI *drawer, AppData* app_data) {
	this->app_data = app_data;
	this->drawer = drawer;
}

void ColorCalibrator::on_mouse(const int event, const int x, const int y, int flags, void* userdata) {
	auto* self = static_cast<ColorCalibrator*>(userdata);
	if (x < 0 || y < 0 || x >= self->drawer->get_image().cols || y >= self->drawer->get_image().rows)
		return;

	if (event == cv::EVENT_LBUTTONDOWN) {
		const cv::Point real_coords = self->drawer->screen_to_world(cv::Point(x, y));
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
			drawer->plot(point, {0, 255, 0});
		} else if (i == roi_points.size() - 1) {
			drawer->plot(point, {255, 0, 0});
		} else {
			drawer->plot(point);
		}
	}

	if (roi_points.size() > 1) {
		drawer->closed_polyline(roi_points);
	}
}


void ColorCalibrator::remove_last_point() {
	roi_points.pop_back();
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

	root["color_calibration"] = {
		{"saturation",          app_data->color_params.saturation},
		{"gamma_correction",    app_data->color_params.gamma_correction},
		{"clahe_clip_limit",    app_data->color_params.clahe_clip_limit},
		{"bilateral_sigma",     app_data->color_params.bilateral_sigma},
		{"green_boost",         app_data->color_params.green_boost},
		{"blue_boost",          app_data->color_params.blue_boost},
		{"red_boost",           app_data->color_params.red_boost}
	};

	root["roi_points"] = points_to_json(roi_points);
	std::ofstream file(filename);
	if (file.is_open()) {
		file << root.dump(4);
		file.close();
		std::cout << "[INFO] Calibración completa guardada en: " << filename << std::endl;
	} else {
		std::cerr << "[ERROR] No se pudo crear el archivo: " << filename << std::endl;
	}
}

void ColorCalibrator::load_calibration(const std::string &filename) {
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

	if (root.contains("color_calibration")) {
		auto j_color = root["color_calibration"];
		app_data->color_params.saturation        = j_color.value("saturation",        app_data->color_params.saturation);
		app_data->color_params.gamma_correction  = j_color.value("gamma_correction",  app_data->color_params.gamma_correction);
		app_data->color_params.clahe_clip_limit  = j_color.value("clahe_clip_limit",  app_data->color_params.clahe_clip_limit);
		app_data->color_params.bilateral_sigma   = j_color.value("bilateral_sigma",   app_data->color_params.bilateral_sigma);
		app_data->color_params.green_boost       = j_color.value("green_boost",       app_data->color_params.green_boost);
		app_data->color_params.blue_boost        = j_color.value("blue_boost",        app_data->color_params.blue_boost);
		app_data->color_params.red_boost         = j_color.value("red_boost",         app_data->color_params.red_boost);
		std::cout << "[INFO] Parámetros de color cargados desde el archivo." << std::endl;
	} else {
		std::cerr << "[WARN] No se encontraron parámetros de calibración de color en el archivo." << std::endl;
	}

	if (root.contains("roi_points")) {
		roi_points = json_to_points(root["roi_points"].get<std::vector<std::vector<int>>>());
		app_data->roi_points = roi_points;
		std::cout << "[INFO] Puntos ROI cargados desde el archivo." << std::endl;
	} else {
		std::cerr << "[WARN] No se encontraron puntos ROI en el archivo." << std::endl;
	}
}
