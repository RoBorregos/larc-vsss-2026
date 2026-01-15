//
// Created by kali on 3/24/24.
//

#ifndef OPENCV_IMAGE_PREPROCESSING_H
#define OPENCV_IMAGE_PREPROCESSING_H

#include <opencv4/opencv2/opencv.hpp>
#include "app_settings.h"

namespace preprocessing {
	[[maybe_unused]] void apply_preprogrammed_filters(cv::Mat& image, const VisionParams& params);

    [[maybe_unused]] void saturation(cv::Mat &image, float alpha);
    [[maybe_unused]] void brightness(cv::Mat &image, int beta);
    [[maybe_unused]] void contrast(cv::Mat &image, float alpha, int beta = 0);
	[[maybe_unused]] void gamma_correction(cv::Mat &image, float gamma);

	[[maybe_unused]] void hsv_red_boost(cv::Mat &image, float alpha);
	[[maybe_unused]] void hsv_blue_boost(cv::Mat &image, float alpha);
	[[maybe_unused]] void hsv_green_boost(cv::Mat &image, float alpha);
	[[maybe_unused]] void hsv_saturation(cv::Mat &image, float alpha);
	[[maybe_unused]] void hsv_brightness(cv::Mat &image, int beta);
	[[maybe_unused]] void hsv_contrast(cv::Mat &image, float alpha, int beta = 0);
	[[maybe_unused]] void hsv_gamma_correction(cv::Mat &image, float gamma_v, float gamma_s = 1.0f);
	[[maybe_unused]] void hsv_clahe(cv::Mat &image, double clip_limit);
	[[maybe_unused]] void hsv_bilateral(cv::Mat &image, float sigma);

    [[maybe_unused]] void white_balance(cv::Mat &image, float temperature, float tint);

    [[maybe_unused]] void sharpen(cv::Mat &image);

    [[maybe_unused]] void edge_detection(cv::Mat &image);

    [[maybe_unused]] void color_space_conversion(cv::Mat &image, int code);

    [[maybe_unused]] void noise_reduction(cv::Mat &image);

    [[maybe_unused]] void threshold(cv::Mat &image, int threshold_value, int max_value, int threshold_type);

    [[maybe_unused]] void blur(cv::Mat &image, int kernel_size);

    [[maybe_unused]] void histogram_equalization(cv::Mat &image);

    [[maybe_unused]] void mirror(cv::Mat &image);

    [[maybe_unused]] void rotate(cv::Mat &image, int angle);

    [[maybe_unused]] void resize(cv::Mat &image, int width, int height);
}

#endif //OPENCV_IMAGE_PREPROCESSING_H
