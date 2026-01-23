#include "gui.h"
#include <image_preprocessing.h>

GUI::GUI(AppData* app_data): app_data(app_data) {
	this->window_name = "default";
}

GUI::GUI(AppData* app_data, const std::string& window_name): app_data(app_data) {
	this->window_name = window_name;
}

void GUI::set_window_name(const std::string& window_name) {
	this->window_name = window_name;
}

std::string GUI::get_window_name() const {
	return this->window_name;
}

bool GUI::valid_coordinate(const cv::Point& point) const {
	if (point.x >= total_width) return false;
	if (point.x < 0) return false;
	if (point.y < 0) return false;
	if (point.y >= total_height) return false;
	return true;
}

bool GUI::valid_coordinate(const std::vector<cv::Point>& points) const {
	return std::all_of(points.begin(), points.end(),
			[this](const cv::Point& p) {
				return this->valid_coordinate(p);
			});
}

void GUI::upload_frame(cv::Mat &input_frame) {
	fps_timer.start();

	preprocessing::resize(input_frame, 1024 * 0.7, 768 * 0.7);

	if (app_data->roi_points.size() > 1) {
		inverse_closed_polyline(app_data->roi_points, Drawer::Layer::PREPROCESSING, {0, 0, 0});
	}
	drawer.render(input_frame, Drawer::Layer::PREPROCESSING);

	image_bgr.upload(input_frame);
	preprocessing::filters(image_bgr, image_bgr, image_hsv, app_data->color_params);

	const int new_total_width = std::ceil(image_bgr.cols * 1.5);
	const int new_total_height = image_bgr.rows;

	const bool dimensions_modified = this->image_with_gui.cols != new_total_width
		|| this->image_with_gui.rows != new_total_height
		|| this->image_with_gui.type() != image_bgr.type();

	if (dimensions_modified) {
		this->image_with_gui = cv::Mat::zeros(new_total_height, new_total_width, image_bgr.type());

		image_width = image_bgr.cols;
		total_width = new_total_width;
		total_height = new_total_height;
		this->viewport = cv::Rect(0, 0, image_width, total_height);
	} else {
		this->image_with_gui.setTo(cv::Scalar(0, 0, 0));
	}
}

cv::Point GUI::screen_to_world(const cv::Point& screen_point) const {
	if (screen_point.x >= image_width) {
		return screen_point;
	}

	const double ratio_x = static_cast<double>(screen_point.x) / static_cast<double>(image_width);
	const double ratio_y = static_cast<double>(screen_point.y) / static_cast<double>(total_height);

	const int world_x = viewport.x + static_cast<int>(viewport.width * ratio_x);
	const int world_y = viewport.y + static_cast<int>(viewport.height * ratio_y);

	return {world_x, world_y};
}

void GUI::reset_zoom() {
	viewport = cv::Rect(0, 0, image_width, total_height);
}

void GUI::zoom(int mouse_x, int mouse_y, int scroll_amount) {
	if (mouse_x >= image_width) return;

	constexpr double zoom_factor = 0.1;
	const double scale = (scroll_amount > 0) ? (1.0 - zoom_factor) : (1.0 + zoom_factor);

	int new_w = static_cast<int>(viewport.width * scale);
	int new_h = static_cast<int>(viewport.height * scale);

	if (new_w > image_width) new_w = image_width;
	if (new_h > total_height) new_h = total_height;

	if (new_w < 10) new_w = 10;
	if (new_h < 10) new_h = 10;

	double ratio_x = static_cast<double>(mouse_x) / image_width;
	double ratio_y = static_cast<double>(mouse_y) / total_height;

	int new_x = static_cast<int>((viewport.x + (viewport.width * ratio_x)) - (new_w * ratio_x));
	int new_y = static_cast<int>((viewport.y + (viewport.height * ratio_y)) - (new_h * ratio_y));

	if (new_x < 0) new_x = 0;
	if (new_y < 0) new_y = 0;
	if (new_x + new_w > image_width) new_x = image_width - new_w;
	if (new_y + new_h > total_height) new_y = total_height - new_h;

	viewport = cv::Rect(new_x, new_y, new_w, new_h);
}

cv::Mat& GUI::get_frame() {
	return image_with_gui;
}

cv::cuda::GpuMat GUI::get_image(const int conversion_code) const {
	if (conversion_code == -1) {
		return image_bgr;
	}

	if (conversion_code == cv::COLOR_BGR2HSV) {
		return image_hsv;
	}

	cv::cuda::GpuMat output;
	cv::cvtColor(image_bgr, output, conversion_code);

	return output;
}

cv::Rect GUI::rowspace_roi(int rowspace) const {
	constexpr int rowspaces = 10;
	constexpr int margin = 10;
	if (rowspace < 0) rowspace = 0;
	if (rowspace >= rowspaces) rowspace = rowspaces - 1;

	const int x = image_width + margin;
	const int y = (total_height / rowspaces) * rowspace + margin;
	const int width = (total_width - image_width) - 2 * margin;
	const int height = (total_height / rowspaces) - 2 * margin;

	return { x, y, width, height };
}

void GUI::free_rowspace(const int rowspace) {
	if (image_with_gui.empty()) return;
	cv::Rect roi = rowspace_roi(rowspace);
	roi = roi & cv::Rect(0, 0, image_with_gui.cols, image_with_gui.rows);

	cv::rectangle(image_with_gui, roi, cv::Scalar(0, 0, 0), cv::FILLED);
}

void GUI::text(
	const std::string &text,
	const int rowspace,
	const Drawer::Layer layer,
	const float font_scale,
	const bool erase_rowspace,
	const int y_offset
	) {

	const cv::Rect roi = rowspace_roi(rowspace);
	drawer.text(text, roi, font_scale, y_offset, erase_rowspace, layer);
}

void GUI::display_frame() {
	if (image_with_gui.empty()) return;
	cv::Mat display_buffer = cv::Mat::zeros(total_height, total_width, image_with_gui.type());

	cv::Mat cpu_image_bgr;
	image_bgr.download(cpu_image_bgr);

	drawer.render(cpu_image_bgr, Drawer::Layer::MARKINGS);

	cv::Mat image_view = cpu_image_bgr(viewport);
	cv::resize(image_view, image_view, cv::Size(image_width, total_height), 0, 0, cv::INTER_LINEAR);

	image_view.copyTo(display_buffer(cv::Rect(0, 0, image_width, total_height)));

	cv::Rect sidebar_rect(image_width, 0, total_width - image_width, total_height);
	cv::Mat sidebar = image_with_gui(sidebar_rect);

	sidebar.copyTo(display_buffer(sidebar_rect));

	drawer.render(display_buffer, Drawer::Layer::INTERFACE);

	fps_timer.stop();
	frame_counter++;
	if (frame_counter % 10 == 0) {
		double time_sec = fps_timer.getTimeSec();
		if (time_sec > 0) {
			current_fps = 10.0 / time_sec;
		}
		fps_timer.reset();
	}
	cv::rectangle(display_buffer, cv::Rect(5, 5, 150, 40), cv::Scalar(0, 0, 0, 0.3), -1);
	std::string fps_text = "FPS: " + std::to_string(int(current_fps));
	cv::putText(display_buffer, fps_text, cv::Point(15, 35),
				cv::FONT_HERSHEY_DUPLEX, 1.0, cv::Scalar(0, 255, 0), 2);

	cv::imshow(window_name, display_buffer);
}

void GUI::plot(const cv::Point &point, const Drawer::Layer layer, const cv::Scalar &color) {
	if (!valid_coordinate(point)) return;
	drawer.plot(point, color, layer);
}


void GUI::solid_circle(const cv::Point& center, const int radius, const Drawer::Layer layer, const cv::Scalar& color) {
	if (!valid_coordinate(center)) return;
	drawer.circle(center, radius, cv::FILLED, color, layer);
}

void GUI::hollow_circle(const cv::Point& center, const int radius, const Drawer::Layer layer, const int thickness, const cv::Scalar& color) {
	if (!valid_coordinate(center)) return;
	drawer.circle(center, radius, thickness, color, layer);
}

void GUI::line(const cv::Point& point1, const cv::Point& point2, const Drawer::Layer layer, const int thickness, const cv::Scalar& color) {
	if (!valid_coordinate(point1)) return;
	if (!valid_coordinate(point2)) return;

	drawer.line(point1, point2, thickness, color, layer);
}

void GUI::polyline(
	const std::vector<cv::Point>& points,
	const Drawer::Layer layer,
	const int thickness,
	const cv::Scalar& color) {

	if (points.size() <= 1) return;
	if (!valid_coordinate(points)) return;

	drawer.polyline(points, false, thickness, color, layer);
}

void GUI::closed_polyline(
	const std::vector<cv::Point>& points,
	const Drawer::Layer layer,
	const int thickness,
	const cv::Scalar &color
	) {

	if (points.size() <= 1) return;
	if (!valid_coordinate(points)) return;

	drawer.polyline(points, true, thickness, color, layer);
}

void GUI::inverse_closed_polyline(
	const std::vector<cv::Point> &points,
	const Drawer::Layer layer,
	const cv::Scalar &color
	) {

	if (points.size() <= 1) return;
	if (!valid_coordinate(points)) return;

	cv::Rect image_area(0, 0, image_width, total_height);
	drawer.inverse_polyline(points, color, image_area, layer);
}


