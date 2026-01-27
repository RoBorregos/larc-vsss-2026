#ifndef GUI_H
#define GUI_H
#include <string>
#include <opencv4/opencv2/opencv.hpp>
#include "drawer.h"
#include "app_settings.h"

class GUI {
private:
	AppData* app_data;
	Drawer drawer;

	std::string window_name{};
	cv::Mat image_with_gui;
	cv::cuda::GpuMat image_bgr;
	cv::cuda::GpuMat image_hsv;
	cv::Rect viewport;

	int image_width{};
	int total_width{};
	int total_height{};

	[[nodiscard]] bool valid_coordinate(const cv::Point& point) const;
	[[nodiscard]] bool valid_coordinate(const std::vector<cv::Point>& points) const;

	static inline cv::Scalar default_color{255, 255, 255};

	double current_fps = 0.0;
	int frame_counter = 0;
public:
	explicit GUI(AppData* app_data);
	GUI(AppData* app_data, const std::string& window_name);

	void set_window_name(const std::string& window_name);
	void free_rowspace(int rowspace);

	[[nodiscard]] std::string get_window_name() const;
	[[nodiscard]] cv::Mat& get_frame();
	[[nodiscard]] cv::Rect rowspace_roi(int rowspace) const;
	[[nodiscard]] cv::cuda::GpuMat get_image(int conversion_code = -1) const;

	void upload_frame(cv::Mat& input_frame);
	void display_frame();
	void zoom(int mouse_x, int mouse_y, int scroll_amount);
	void reset_zoom();

	[[nodiscard]] cv::Point screen_to_world(const cv::Point& screen_point) const;

	void text(const std::string& text, int rowspace, Drawer::Layer layer, float font_scale = 0.7, bool erase_rowspace = true, int y_offset = 0);

	void plot(const cv::Point& point, Drawer::Layer layer, const cv::Scalar& color = default_color);
	void solid_circle(const cv::Point& center, int radius, Drawer::Layer layer, const cv::Scalar& color = default_color);
	void hollow_circle(const cv::Point& center, int radius, Drawer::Layer layer, int thickness = 1, const cv::Scalar& color = default_color);

	void line(const cv::Point& point1, const cv::Point& point2, Drawer::Layer layer, int thickness = 1, const cv::Scalar& color = default_color);
	void polyline(const std::vector<cv::Point>& points, Drawer::Layer layer, int thickness = 1, const cv::Scalar& color = default_color);
	void closed_polyline(const std::vector<cv::Point>& points, Drawer::Layer layer, int thickness = 1, const cv::Scalar& color = default_color);
	void inverse_closed_polyline(const std::vector<cv::Point>& points, Drawer::Layer layer, const cv::Scalar& color = default_color);
	cv::TickMeter fps_timer;
};

#endif //GUI_H
