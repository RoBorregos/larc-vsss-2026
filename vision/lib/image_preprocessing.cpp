
#include "image_preprocessing.h"

[[maybe_unused]] void preprocessing::apply_preprogrammed_filters(cv::Mat &image, const VisionParams& params) {
	hsv_red_boost(image, params.red_boost);
	hsv_green_boost(image, params.green_boost);
	hsv_blue_boost(image, params.blue_boost);
	hsv_saturation(image, params.saturation);
	hsv_clahe(image, params.clahe_clip_limit);
	hsv_bilateral(image, params.bilateral_sigma);
	hsv_gamma_correction(image, params.gamma_correction);
}

[[maybe_unused]] void preprocessing::apply_preprogrammed_filters(cv::Mat &image, const ObjectVisionParams& params) {
	hsv_red_boost(image, params.red_boost);
	hsv_green_boost(image, params.green_boost);
	hsv_blue_boost(image, params.blue_boost);
	hsv_saturation(image, params.saturation);
	hsv_clahe(image, params.clahe_clip_limit);
	hsv_bilateral(image, params.bilateral_sigma);
	hsv_gamma_correction(image, params.gamma_correction);
}

[[maybe_unused]] void preprocessing::saturation(cv::Mat &image, float alpha) {
    cv::Mat hsv;
    cv::cvtColor(image, hsv, cv::COLOR_BGR2HSV);

    std::vector<cv::Mat> channels;
    cv::split(hsv, channels);

    channels[1] = channels[1] * alpha;

    cv::merge(channels, hsv);

    cv::cvtColor(hsv, image, cv::COLOR_HSV2BGR);
}

[[maybe_unused]] void preprocessing::brightness(cv::Mat &image, int beta) {
    image.convertTo(image, -1, 1, beta);
}

[[maybe_unused]] void preprocessing::contrast(cv::Mat &image, float alpha, int beta) {
    image.convertTo(image, -1, alpha, beta);
}

[[maybe_unused]] void preprocessing::gamma_correction(cv::Mat &image, float gamma) {
	cv::Mat lut(1, 256, CV_8U);
	for (int i = 0; i < 256; i++) {
		lut.at<uchar>(i) = cv::saturate_cast<uchar>(pow(i / 255.0, gamma) * 255.0);
	}

	cv::LUT(image, lut, image);
}

void preprocessing::hsv_blue_boost(cv::Mat &image, float alpha) {
	cv::Mat bgr_mat;
	cv::cvtColor(image, bgr_mat, cv::COLOR_HSV2BGR);

	std::vector<cv::Mat> channels;
	cv::split(bgr_mat, channels);

	channels[0].convertTo(channels[0], -1, alpha, 0);

	cv::merge(channels, bgr_mat);
	cv::cvtColor(bgr_mat, image, cv::COLOR_BGR2HSV);
}

void preprocessing::hsv_red_boost(cv::Mat &image, float alpha) {
	cv::Mat bgr_mat;
	cv::cvtColor(image, bgr_mat, cv::COLOR_HSV2BGR);

	std::vector<cv::Mat> channels;
	cv::split(bgr_mat, channels);

	channels[2].convertTo(channels[2], -1, alpha, 0);

	cv::merge(channels, bgr_mat);
	cv::cvtColor(bgr_mat, image, cv::COLOR_BGR2HSV);
}


[[maybe_unused]] void preprocessing::hsv_green_boost(cv::Mat &image, float alpha) {
	cv::Mat bgr_mat;
	cv::cvtColor(image, bgr_mat, cv::COLOR_HSV2BGR);

	std::vector<cv::Mat> channels;
	cv::split(bgr_mat, channels);

	channels[1].convertTo(channels[1], -1, alpha, 0);

	cv::merge(channels, bgr_mat);
	cv::cvtColor(bgr_mat, image, cv::COLOR_BGR2HSV);
}

[[maybe_unused]] void preprocessing::hsv_saturation(cv::Mat &image, float alpha) {
	std::vector<cv::Mat> channels;
	cv::split(image, channels);
	channels[1].convertTo(channels[1], -1, alpha, 0);
	cv::merge(channels, image);
}

[[maybe_unused]] void preprocessing::hsv_contrast(cv::Mat &image, float alpha, int beta) {
	std::vector<cv::Mat> channels;
	cv::split(image, channels);
	channels[2].convertTo(channels[2], -1, alpha, beta);
	cv::merge(channels, image);
}

[[maybe_unused]] void preprocessing::hsv_brightness(cv::Mat &image, int beta) {
	std::vector<cv::Mat> channels;
	cv::split(image, channels);
	channels[2].convertTo(channels[2], -1, 1, beta);
	cv::merge(channels, image);
}

cv::Mat get_gamma_lut(float gamma) {
	cv::Mat lut(1, 256, CV_8U);
	uchar* p = lut.ptr();
	for (int i = 0; i < 256; ++i) {
		p[i] = cv::saturate_cast<uchar>(pow(i / 255.0, 1.0 / gamma) * 255.0);
	}
	return lut;
}
[[maybe_unused]] void preprocessing::hsv_gamma_correction(cv::Mat &image, const float gamma_v, const float gamma_s) {
	std::vector<cv::Mat> channels;
	cv::split(image, channels);

	if (gamma_v != 1.0f) {
		cv::Mat lut_v = get_gamma_lut(gamma_v);
		cv::LUT(channels[2], lut_v, channels[2]);
	}

	if (gamma_s != 1.0f) {
		cv::Mat lut_s = get_gamma_lut(gamma_s);
		cv::LUT(channels[1], lut_s, channels[1]);
	}

	cv::merge(channels, image);
}

[[maybe_unused]] void preprocessing::hsv_clahe(cv::Mat &image, const double clip_limit) {
	cv::Mat lab;
	cv::cvtColor(image, lab, cv::COLOR_HSV2RGB);
	cv::cvtColor(lab, lab, cv::COLOR_RGB2Lab);

	std::vector<cv::Mat> lab_planes;
	cv::split(lab, lab_planes);

	cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
	clahe->setClipLimit(clip_limit);
	clahe->setTilesGridSize(cv::Size(8, 8));

	clahe->apply(lab_planes[0], lab_planes[0]);

	cv::merge(lab_planes, lab);
	cv::cvtColor(lab, lab, cv::COLOR_Lab2BGR);
	cv::cvtColor(lab, image, cv::COLOR_BGR2HSV);
}

[[maybe_unused]] void preprocessing::hsv_bilateral(cv::Mat &image, float sigma_color) {
	int sigma_space = 3;

	cv::Mat temp;

	cv::cvtColor(image, temp, cv::COLOR_HSV2BGR);
	cv::cvtColor(temp, temp, cv::COLOR_BGR2Lab);

	cv::Mat lab_filtered;
	cv::bilateralFilter(temp, lab_filtered, -1, sigma_color, sigma_space);

	cv::cvtColor(lab_filtered, temp, cv::COLOR_Lab2BGR);
	cv::cvtColor(temp, image, cv::COLOR_BGR2HSV);
}
[[maybe_unused]] void preprocessing::white_balance(cv::Mat &image, float temperature, float tint) {
    image.convertTo(image, CV_32F);

    float blue_scale = tint < 0 ? (1 + tint / 100) : (1 - tint / 100);
    float red_scale = tint > 0 ? (1 + tint / 100) : (1 - tint / 100);

    float kelvin_scale = temperature / 6500.0f;
    float red_channel = 1.0f, green_channel = 1.0f, blue_channel = 1.0f;
    if (kelvin_scale > 1.0) {
        red_channel = kelvin_scale;
    } else {
        blue_channel = static_cast<float>(1.0 / kelvin_scale);
    }

    image.forEach<cv::Vec3f>([&](cv::Vec3f &pixel, const int *pos) {
        pixel[0] *= blue_channel * blue_scale; // Blue channel
        pixel[1] *= green_channel;             // Green channel (unchanged)
        pixel[2] *= red_channel * red_scale;   // Red channel
    });

    image.convertTo(image, CV_8U);
}

[[maybe_unused]] void preprocessing::sharpen(cv::Mat &image) {
    cv::Mat kernel = (cv::Mat_<float>(3, 3) << 0, -1, 0, -1, 5, -1, 0, -1, 0);
    cv::filter2D(image, image, -1, kernel);
}

[[maybe_unused]] void preprocessing::edge_detection(cv::Mat &image) {
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    cv::Canny(gray, image, 100, 200);
}

[[maybe_unused]] void preprocessing::color_space_conversion(cv::Mat &image, int code) {
    cv::cvtColor(image, image, code);
}

[[maybe_unused]] void preprocessing::noise_reduction(cv::Mat &image) {
    cv::fastNlMeansDenoisingColored(image, image, 10, 10, 7, 21);
}

[[maybe_unused]] void preprocessing::threshold(cv::Mat &image, int threshold_value, int max_value, int threshold_type) {
    cv::cvtColor(image, image, cv::COLOR_BGR2GRAY);
    cv::threshold(image, image, threshold_value, max_value, threshold_type);
}

[[maybe_unused]] void preprocessing::blur(cv::Mat &image, int kernel_size) {
    cv::blur(image, image, cv::Size(kernel_size, kernel_size));
}

[[maybe_unused]] void preprocessing::histogram_equalization(cv::Mat &image) {
    cv::cvtColor(image, image, cv::COLOR_BGR2GRAY);
    cv::equalizeHist(image, image);
}

[[maybe_unused]] void preprocessing::mirror(cv::Mat &image) {
    cv::flip(image, image, 1);
}

[[maybe_unused]] void preprocessing::rotate(cv::Mat &image, int angle) {
    cv::Point2f center(static_cast<float>(image.cols / 2.0), static_cast<float>(image.rows / 2.0));
    cv::Mat rotation_matrix = cv::getRotationMatrix2D(center, angle, 1.0);
    cv::warpAffine(image, image, rotation_matrix, image.size());
}

[[maybe_unused]] void preprocessing::resize(cv::Mat &image, int width, int height) {
    cv::resize(image, image, cv::Size(width, height));
}