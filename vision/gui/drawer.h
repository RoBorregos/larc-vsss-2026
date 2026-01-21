#ifndef DRAWER_H
#define DRAWER_H

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

	void plot(const cv::Point& p, const cv::Scalar& color);
	void line(const cv::Point& p1, const cv::Point& p2, int thickness, const cv::Scalar& color);
	void circle(const cv::Point& center, int radius, int thickness, const cv::Scalar& color);
	void polyline(const std::vector<cv::Point>& points, bool is_closed, int thickness, const cv::Scalar& color);
	void inverse_polyline(const std::vector<cv::Point>& points, const cv::Scalar& color, const cv::Rect& restriction_roi);
	void text(const std::string& content, const cv::Rect& roi, float font_scale, int y_offset, bool erase_bg);

	void render(cv::Mat& target);

	void clear();
private:
	std::vector<std::unique_ptr<DrawCommand>> command_queue;
};



#endif //DRAWER_H
