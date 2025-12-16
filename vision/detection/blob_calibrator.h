#ifndef CALIBRATOR_H
#define CALIBRATOR_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <optional>
#include <gui.h>
#include "app_settings.h"

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

class BlobCalibrator {
private:
	std::vector<cv::Point> points;
	GUI* drawer;
	AppData* app_data;

	void handle_click(int x, int y);
public:
	BlobCalibrator(GUI* drawer, AppData* app_data);

	static void on_mouse(int event, int x, int y, int flags, void* userdata);

	void reset_points();
	static void print_calibrations(const MatchCalibration& calibration);
	[[nodiscard]] std::optional<CalibrationResult> calibrate_individual();
};

#endif //CALIBRATOR_H