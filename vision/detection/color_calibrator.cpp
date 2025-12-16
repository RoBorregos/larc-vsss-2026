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
