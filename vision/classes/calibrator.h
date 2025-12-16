#ifndef CALIBRATOR_H
#define CALIBRATOR_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <gui.h>
#include <optional>
#include <iostream>
#include <iomanip>
#include "app_settings.h"

struct CalibrationResult {
	std::vector<cv::Point> points;
	unsigned long frame_id;
	unsigned long pixel_count;
	cv::Scalar min_hsv;
	cv::Scalar max_hsv;
	cv::Scalar avg_hsv;
	cv::Scalar std_dev_hsv;
	VisionParams params;
	bool valid = false;
};

// struct CalibrationGroup {
// 	int size = 0;
// 	std::vector<CalibrationResult> results;
// };

struct MatchCalibration {
	CalibrationResult yellow;
	CalibrationResult blue;
	CalibrationResult magenta;
	CalibrationResult red;
	CalibrationResult cyan;
	CalibrationResult green;
	CalibrationResult orange;
};

class Calibrator {
private:
	std::vector<cv::Point> points;
	GUI* drawer;

	void handle_click(int x, int y);
public:
	explicit Calibrator(GUI* drawer);

	static void on_mouse(int event, int x, int y, int flags, void* userdata);

	void reset_points();
	static void print_calibrations(const MatchCalibration& calibration);
	[[nodiscard]] std::optional<CalibrationResult> calibrate_individual(
		const VisionParams& params
	);
};

#endif //CALIBRATOR_H