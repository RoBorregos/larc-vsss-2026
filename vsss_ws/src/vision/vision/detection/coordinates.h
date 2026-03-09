#pragma once
#include "app_settings.h"

namespace FieldConstants {
	const float FIELD_WIDTH_M = 1.50f;
	const float FIELD_HEIGHT_M = 1.30f;
}

class Coordinates {
private:
	AppData* app_data;
	cv::Mat homography_matrix;
	bool is_calibrated = false;

	static std::vector<cv::Point2f> get_sorted_corners(const std::vector<cv::Point>& points);
public:
	explicit Coordinates(AppData* app_data);
	void update_matrix();

	cv::Point2f pixel_to_meter (cv::Point2f pixel) const;
	cv::Point meter_to_minimap(const cv::Point2f& meter, int width, int height) const;
	double meter_to_pixel_scalar(double meter);
	cv::Mat get_warped_image(const cv::Mat& input_image) const;
	cv::Point2f warped_to_pixel(const cv::Point2f& warped_point, int width, int height) const;
};