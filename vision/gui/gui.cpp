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
	preprocessing::resize(input_frame, 1024 * 0.7, 768 * 0.7);
	image = input_frame.clone();

	const int new_total_width = std::ceil(input_frame.cols * 1.5);
	const int new_total_height = input_frame.rows;

	const bool dimensions_modified = this->frame.cols != new_total_width
		|| this->frame.rows != new_total_height
		|| this->frame.type() != input_frame.type();

	if (dimensions_modified) {
		this->frame = cv::Mat::zeros(new_total_height, new_total_width, input_frame.type());

		image_width = input_frame.cols;
		total_width = new_total_width;
		total_height = new_total_height;
		this->viewport = cv::Rect(0, 0, image_width, total_height);
	} else {
		this->frame.setTo(cv::Scalar(0, 0, 0));
	}

	const auto image_roi = cv::Rect(0, 0, input_frame.cols, input_frame.rows);
	input_frame.copyTo(this->frame(image_roi));
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
	return frame;
}

cv::Mat GUI::get_image(const int conversion_code, bool apply_preprocessing) const {
	if (conversion_code == -1 && !apply_preprocessing) {
		return image;
	}
	cv::Mat output = image.clone();

	if (apply_preprocessing) {
		inverse_closed_polyline(app_data->roi_points, output, {0, 0, 0});
		cv::cvtColor(output, output, cv::COLOR_BGR2HSV);

		if (conversion_code == cv::COLOR_BGR2HSV) {
			preprocessing::apply_preprogrammed_filters(output, *app_data);
			return output;
		}

		cv::cvtColor(output, output, cv::COLOR_HSV2BGR);
	}

	if (conversion_code != -1) {
		cv::cvtColor(image, output, conversion_code);
	}

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
	if (frame.empty()) return;
	cv::Rect roi = rowspace_roi(rowspace);
	roi = roi & cv::Rect(0, 0, frame.cols, frame.rows);

	cv::rectangle(frame, roi, cv::Scalar(0, 0, 0), cv::FILLED);
}

void GUI::set_display_brightness(int brightness) {
	this->brightness = brightness;
}

void GUI::set_display_contrast(float contrast) {
	this->contrast = contrast;
}

void GUI::set_display_saturation(float saturation) {
	this->saturation = saturation;
}

void GUI::set_display_gamma_correction(float gamma_correction) {
	this->gamma_correction = gamma_correction;
}

void GUI::set_clahe_clip_limit(float clip_limit) {
	this->clahe_clip_limit = clip_limit;
}

void GUI::set_bilateral_sigma(int sigma) {
	this->bilateral_sigma = sigma;
}

void GUI::set_green_boost(float green_boost_factor) {
	this->green_boost = green_boost_factor;
}

void GUI::set_blue_boost(float blue_boost_factor) {
	this->blue_boost_factor = blue_boost_factor;
}

void GUI::set_red_boost(float red_boost_factor) {
	this->red_boost = red_boost_factor;
}


void GUI::text(const std::string &text, const int rowspace, float font_scale, bool erase_rowspace, int y_offset) {
	if (frame.empty()) return;
	if (erase_rowspace) free_rowspace(rowspace);

	cv::Rect roi = rowspace_roi(rowspace);

	constexpr int font_face = cv::FONT_HERSHEY_SIMPLEX;
	constexpr int thickness = 1;
	int baseline = 0;

	cv::Size text_size = cv::getTextSize(text, font_face, font_scale, thickness, &baseline);

	while (text_size.width > roi.width && font_scale > 0.3) {
		font_scale -= 0.1;
		text_size = cv::getTextSize(text, font_face, font_scale, thickness, &baseline);
	}

	cv::Point text_org;
	text_org.x = roi.x + (roi.width - text_size.width) / 2;
	text_org.y = roi.y + (roi.height + text_size.height) / 2 + y_offset;

	cv::putText(frame, text, text_org, font_face, font_scale, cv::Scalar(255, 255, 255), thickness);
}

void GUI::display_frame() const {
	if (frame.empty()) return;

	cv::Mat display_buffer = cv::Mat::zeros(total_height, total_width, frame.type());

	cv::Mat image_view = frame(viewport);
	cv::resize(image_view, image_view, cv::Size(image_width, total_height), 0, 0, cv::INTER_LINEAR);

	cv::cvtColor(image_view, image_view, cv::COLOR_BGR2HSV);
	preprocessing::apply_preprogrammed_filters(image_view, *app_data);
	cv::cvtColor(image_view, image_view, cv::COLOR_HSV2BGR);

	image_view.copyTo(display_buffer(cv::Rect(0, 0, image_width, total_height)));

	cv::Rect sidebar_rect(image_width, 0, total_width - image_width, total_height);
	cv::Mat sidebar = frame(sidebar_rect);

	sidebar.copyTo(display_buffer(sidebar_rect));

	if (app_data->roi_points.size() > 1) {
		inverse_closed_polyline(app_data->roi_points, display_buffer, {0, 0, 0});
	}

	cv::imshow(window_name, display_buffer);
}
void GUI::plot(const cv::Point &point, const cv::Scalar &color) {
	if (!valid_coordinate(point)) return;
	cv::circle(frame, point, 1, color, -1);
}


void GUI::solid_circle(const cv::Point& center, const int radius, const cv::Scalar& color) {
	if (!valid_coordinate(center)) return;
	cv::circle(frame, center, radius, color, -1);
}

void GUI::hollow_circle(const cv::Point& center, const int radius, const int thickness, const cv::Scalar& color) {
	if (!valid_coordinate(center)) return;
	cv::circle(frame, center, radius, color, thickness);
}

void GUI::line(const cv::Point& point1, const cv::Point& point2, const int thickness, const cv::Scalar& color) {
	if (!valid_coordinate(point1)) return;
	if (!valid_coordinate(point2)) return;

	cv::line(frame, point1, point2, color, thickness);
}

void GUI::polyline(const std::vector<cv::Point>& points, const int thickness, const cv::Scalar&
color) {
	if (points.size() <= 1) return;
	if (!valid_coordinate(points)) return;

	const std::vector<std::vector<cv::Point>> contours = { points };
	cv::polylines(frame, contours, false, color, thickness);
}

void GUI::closed_polyline(const std::vector<cv::Point>& points, const int thickness, const cv::Scalar
&color) {
	if (points.size() <= 1) return;
	if (!valid_coordinate(points)) return;

	std::vector<std::vector<cv::Point>> contours = { points };

	cv::polylines(frame, contours, true, color, thickness);
}

void GUI::inverse_closed_polyline(const std::vector<cv::Point> &points, const cv::Scalar &color) {
	if (points.size() <= 1) return;
	if (!valid_coordinate(points)) return;

	std::vector<std::vector<cv::Point>> contours = { points };
	cv::Mat mask(frame.size(), CV_8UC1, cv::Scalar(255));
	cv::fillPoly(mask, contours, cv::Scalar(0));
	frame.setTo(color, mask);
}

void GUI::inverse_closed_polyline(const std::vector<cv::Point> &points, cv::Mat& input_mat, const cv::Scalar &color) const {
	if (points.size() <= 1) return;
	if (input_mat.empty()) return;

	if (input_mat.cols < image.cols) return;
	if (input_mat.rows < image.rows) return;

	cv::Rect safe_viewport = cv::Rect(0, 0, image.cols, image.rows);
	if (safe_viewport.empty()) return;

	std::vector<std::vector<cv::Point>> contours = { points };
	cv::Mat mask = cv::Mat::zeros(input_mat.size(), CV_8UC1);
	cv::rectangle(mask, safe_viewport, cv::Scalar(255), -1);

	cv::fillPoly(mask, contours, cv::Scalar(0));
	input_mat.setTo(color, mask);
}




