#include "coordinates.h"

Coordinates::Coordinates(AppData* app_data) {
	this->app_data = app_data;

	update_matrix();
}

void Coordinates::update_matrix() {
	if (app_data->roi_points.size() < 4) {
		is_calibrated = false;
		return;
	}

	const std::vector<cv::Point2f> points = get_sorted_corners(app_data->roi_points);

	float half_w = FieldConstants::FIELD_WIDTH_M / 2.0f;
	float half_h = FieldConstants::FIELD_HEIGHT_M / 2.0f;

	const std::vector<cv::Point2f> dst_points = {
		{ -half_w,  half_h },
		{  half_w,  half_h },
		{  half_w, -half_h },
		{ -half_w, -half_h }
	};

	homography_matrix = cv::findHomography(points, dst_points);
	is_calibrated = !homography_matrix.empty();
}

cv::Point2f Coordinates::pixel_to_meter(const cv::Point2f pixel) const {
	if (!is_calibrated) return {0.0f, 0.0f};

	std::vector<cv::Point2f> src = { pixel };
	std::vector<cv::Point2f> dst;

	cv::perspectiveTransform(src, dst, homography_matrix);

	return dst[0];
}

std::vector<cv::Point2f> Coordinates::get_sorted_corners(const std::vector<cv::Point>& points) {
	std::vector<cv::Point2f> corners(4);

	// TL: x+y minimum | BR: x+y maximum
	// TR: x-y maximum | BL: x-y minimum
	float min_sum = 1e9, max_sum = -1e9;
	float min_diff = 1e9, max_diff = -1e9;

	for (const auto& p : points) {
		float sum = (float)(p.x + p.y);
		float diff = (float)(p.x - p.y);

		if (sum < min_sum) { min_sum = sum; corners[0] = p; } // TL
		if (diff > max_diff) { max_diff = diff; corners[1] = p; } // TR
		if (sum > max_sum) { max_sum = sum; corners[2] = p; } // BR
		if (diff < min_diff) { min_diff = diff; corners[3] = p; } // BL
	}
	return corners;
}

cv::Point Coordinates::meter_to_minimap(const cv::Point2f& meter, int width, int height) const {
	const float W = FieldConstants::FIELD_WIDTH_M;
	const float H = FieldConstants::FIELD_HEIGHT_M;

	float x_norm = (meter.x + (W / 2.0f)) / W;

	float y_norm = (meter.y + (H / 2.0f)) / H;
	y_norm = 1.0f - y_norm;

	int px = static_cast<int>(x_norm * width);
	int py = static_cast<int>(y_norm * height);

	return cv::Point(px, py);
}

double Coordinates::meter_to_pixel_scalar(double meter) {
	if (!is_calibrated || homography_matrix.empty()) return 0.0;

	cv::Mat inv_H = homography_matrix.inv();

	std::vector<cv::Point2f> src_meters = {
		{0.0f, 0.0f},
		{static_cast<float>(meter), 0.0f}
	};
	std::vector<cv::Point2f> dst_pixels;

	cv::perspectiveTransform(src_meters, dst_pixels, inv_H);

	return cv::norm(dst_pixels[0] - dst_pixels[1]);
}

cv::Mat Coordinates::get_warped_image(const cv::Mat& input_image) const {
	if (!is_calibrated || homography_matrix.empty()) {
		return input_image.clone();
	}

	int width = input_image.cols;
	int height = input_image.rows;
	cv::Size output_size(width, height);

	cv::Mat warped_image;

	cv::Mat shift = cv::Mat::eye(3, 3, CV_64F);
	shift.at<double>(0, 2) = FieldConstants::FIELD_WIDTH_M / 2.0;
	shift.at<double>(1, 2) = FieldConstants::FIELD_HEIGHT_M / 2.0;

	cv::Mat scale = cv::Mat::eye(3, 3, CV_64F);
	scale.at<double>(0, 0) = width / FieldConstants::FIELD_WIDTH_M;
	scale.at<double>(1, 1) = height / FieldConstants::FIELD_HEIGHT_M;

	cv::Mat flip_y = cv::Mat::eye(3, 3, CV_64F);
	flip_y.at<double>(1, 1) = -1.0;
	flip_y.at<double>(1, 2) = height;

	cv::Mat final_h = flip_y * scale * shift * homography_matrix;

	cv::warpPerspective(input_image, warped_image, final_h, output_size);

	return warped_image;
}

cv::Point2f Coordinates::warped_to_pixel(const cv::Point2f& warped_point, int width, int height) const {
	if (!is_calibrated || homography_matrix.empty()) return {0.0f, 0.0f};

	cv::Mat shift = cv::Mat::eye(3, 3, CV_64F);
	shift.at<double>(0, 2) = FieldConstants::FIELD_WIDTH_M / 2.0;
	shift.at<double>(1, 2) = FieldConstants::FIELD_HEIGHT_M / 2.0;

	cv::Mat scale = cv::Mat::eye(3, 3, CV_64F);
	scale.at<double>(0, 0) = (double)width / FieldConstants::FIELD_WIDTH_M;
	scale.at<double>(1, 1) = (double)height / FieldConstants::FIELD_HEIGHT_M;

	cv::Mat flip_y = cv::Mat::eye(3, 3, CV_64F);
	flip_y.at<double>(1, 1) = -1.0;
	flip_y.at<double>(1, 2) = (double)height;

	cv::Mat final_homography = flip_y * scale * shift * homography_matrix;

	cv::Mat inverse_final_homography = final_homography.inv();

	std::vector<cv::Point2f> src = { warped_point };
	std::vector<cv::Point2f> dst;
	cv::perspectiveTransform(src, dst, inverse_final_homography);

	return dst[0];
}