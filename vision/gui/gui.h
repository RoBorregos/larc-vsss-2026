#ifndef DRAWER_H
#define DRAWER_H
#include <string>
#include <opencv4/opencv2/opencv.hpp>
#include "app_settings.h"

class GUI {
private:
	AppData* app_data;

	std::string window_name{};
	cv::Mat frame;
	cv::Mat image;
	cv::Rect viewport;

	int brightness = 0;
	float contrast = 1;
	float saturation = 1;
	float gamma_correction = 1;
	float clahe_clip_limit = 2.0f;
	int bilateral_sigma = 0;
	float green_boost = 1.0f;
	float blue_boost_factor = 1.0f;
	float red_boost = 1.0f;

	int image_width{};
	int total_width{};
	int total_height{};

	[[nodiscard]] bool valid_coordinate(const cv::Point& point) const;
	[[nodiscard]] bool valid_coordinate(const std::vector<cv::Point>& points) const;

	static inline cv::Scalar default_color{255, 255, 255};
public:
	explicit GUI(AppData* app_data);
	GUI(AppData* app_data, const std::string& window_name);

	void set_window_name(const std::string& window_name);
	void free_rowspace(int rowspace);

	[[nodiscard]] std::string get_window_name() const;
	[[nodiscard]] cv::Mat& get_frame();
	[[nodiscard]] cv::Rect rowspace_roi(int rowspace) const;
	[[nodiscard]] cv::Mat get_image(int conversion_code = -1, bool apply_preprocessing = true) const;

	void set_display_brightness(int brightness);
	void set_display_contrast(float contrast);
	void set_display_saturation(float saturation);
	void set_display_gamma_correction(float gamma_correction);
	void set_clahe_clip_limit(float clip_limit);
	void set_bilateral_sigma(int sigma);
	void set_green_boost(float green_boost_factor);
	void set_blue_boost(float blue_boost_factor);
	void set_red_boost(float red_boost_factor);

	void upload_frame(cv::Mat& input_frame);
	void display_frame() const;
	// void $display_processing_frame() const;
	void zoom(int mouse_x, int mouse_y, int scroll_amount);
	void reset_zoom();

	[[nodiscard]] cv::Point screen_to_world(const cv::Point& screen_point) const;

	void text(const std::string& text, int rowspace, float font_scale = 0.7, bool erase_rowspace = true, int y_offset = 0);

	void plot(const cv::Point& point, const cv::Scalar& color = default_color);
	void solid_circle(const cv::Point& center, int radius, const cv::Scalar& color = default_color);
	void hollow_circle(const cv::Point& center, int radius, int thickness = 1, const cv::Scalar& color = default_color);

	void line(const cv::Point& point1, const cv::Point& point2, int thickness = 1, const cv::Scalar& color = default_color);
	void polyline(const std::vector<cv::Point>& points, int thickness = 1, const cv::Scalar& color = default_color);
	void closed_polyline(const std::vector<cv::Point>& points, int thickness = 1, const cv::Scalar& color = default_color);
	void inverse_closed_polyline(const std::vector<cv::Point>& points, const cv::Scalar& color = default_color);
	void inverse_closed_polyline(const std::vector<cv::Point>& points, cv::Mat& input_mat, const cv::Scalar& color = default_color) const;
};

#endif //DRAWER_H
