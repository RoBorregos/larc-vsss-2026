#ifndef CALIBRATOR_H
#define CALIBRATOR_H

#include <fstream>
#include <opencv2/opencv.hpp>
#include <vector>
#include <optional>
#include <gui.h>
#include "app_settings.h"
#include "json.hpp"
using json = nlohmann::json;

class BlobCalibrator {
private:
	std::vector<cv::Point> points;
	GUI* drawer;
	AppData* app_data;

	static std::vector<double> scalar_to_json(const cv::Scalar& s);
	static cv::Scalar json_to_scalar(const std::vector<double>& v);
	static std::vector<std::vector<int>> points_to_json(const std::vector<cv::Point>& pts);
	static std::vector<cv::Point> json_to_points(const std::vector<std::vector<int>>& v);

public:
	BlobCalibrator(GUI* drawer, AppData* app_data);

	void handle_click(int x, int y);
	static void on_mouse(int event, int x, int y, int flags, void* userdata);

	void reset_points();
	static void print_calibrations(const MatchCalibration& calibration);
	[[nodiscard]] std::optional<CalibrationResult> calibrate_individual();
	[[nodiscard]] std::vector<cv::Point> get_points() const { return points; }

	void save_calibration(const std::string& filename);
	void load_calibration(const std::string& filename);
};

#endif //CALIBRATOR_H