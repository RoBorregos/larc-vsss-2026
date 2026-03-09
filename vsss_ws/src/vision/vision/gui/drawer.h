#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <memory>
#include <string>

class DrawCommand {
public:
	virtual ~DrawCommand() = default;
	virtual void draw(cv::Mat& target) = 0;
};

class Drawer {
public:
	Drawer() = default;

	enum class Layer {
		PREPROCESSING,
		MARKINGS,
		INTERFACE
	};

	void plot(const cv::Point& p, const cv::Scalar& color, Layer layer);
	void line(const cv::Point& p1, const cv::Point& p2, int thickness, const cv::Scalar& color, Layer layer);

	void circle(const cv::Point& center, int radius, int thickness, const cv::Scalar& color, Layer layer);
	void polyline(const std::vector<cv::Point>& points, bool is_closed, int thickness, const cv::Scalar& color, Layer layer);
	void inverse_polyline(const std::vector<cv::Point>& points, const cv::Scalar& color, const cv::Rect& restriction_roi, Layer layer);
	void text(const std::string& content, const cv::Rect& roi, float font_scale, int y_offset, bool erase_bg, Layer layer);
	void text(const std::string& content, cv::Point org, float font_scale, Layer layer);

	void render(cv::Mat& target, Layer layer);

	void clear(Layer layer);
	void clear_all();
private:
	std::map<Layer, std::vector<std::unique_ptr<DrawCommand>>> queues;
};
