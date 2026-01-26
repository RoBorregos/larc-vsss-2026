#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#include <string>
#include <opencv2/opencv.hpp>

namespace Color_ID {
	constexpr unsigned char NONE    = 0;
	constexpr unsigned char BLUE    = 1;
	constexpr unsigned char YELLOW  = 2;
	constexpr unsigned char CYAN    = 3;
	constexpr unsigned char GREEN   = 4;
	constexpr unsigned char MAGENTA = 5;
	constexpr unsigned char RED     = 6;
	constexpr unsigned char ORANGE  = 7;
}

struct ColorCalibration {
	int id{};
	cv::Scalar hsv_avg{};
	cv::Scalar hsv_stddev{};
};

enum class AppState {
	MAIN_MENU,
	MASK_CALIBRATING,
	BLOB_CALIBRATION_MENU,
	BLOB_CALIBRATING,
	CAMERA_CALIBRATING,
	COLOR_CALIBRATING,
	ROI_CALIBRATING,
	DETECTION
};

struct VisionParams {
	float saturation = 1.0f;
	float gamma_correction_s = 1.0f;
	float gamma_correction_v = 1.0f;
	float clahe_clip_limit = 1.0f;
	float bilateral_sigma = 0.0f;
	float green_boost = 1.0f;
	float blue_boost = 1.0f;
	float red_boost = 1.0f;
};

struct CameraParams {
	float brightness = 100.0f;
	float contrast = 100.0f;

	bool auto_exposure = false;
	float exposure = 150.0f;

	bool auto_focus = false;
	float focus = 0.0f;
};

struct MaskParams {
	float h_min = 0;
	float s_min = 0;
	float v_min = 0;
	float h_max = 180;
	float s_max = 255;
	float v_max = 255;
};

struct CalibrationResult {
	std::vector<cv::Point> points;
	unsigned long frame_id;
	unsigned long pixel_count;
	cv::Scalar min_hsv;
	cv::Scalar max_hsv;
	cv::Scalar avg_hsv;
	cv::Scalar std_dev_hsv;
	bool valid = false;
};

struct MatchCalibration {
	std::vector<CalibrationResult> yellow;
	std::vector<CalibrationResult> blue;
	std::vector<CalibrationResult> magenta;
	std::vector<CalibrationResult> red;
	std::vector<CalibrationResult> cyan;
	std::vector<CalibrationResult> green;
	std::vector<CalibrationResult> orange;
};

struct AppData {
	AppState current_state = AppState::MAIN_MENU;
	std::string current_color = "None";
	std::string calibration_filename = "/media/ikercsv/Files/Projects/larc-vsss-2026/vision/vsss_calibration.json";
	VisionParams color_params;
	CameraParams camera_params;
	MaskParams mask_params;
	MatchCalibration match_calibration;
	std::vector<cv::Point> roi_points;
	bool paused = false;
};

#endif // APP_SETTINGS_H