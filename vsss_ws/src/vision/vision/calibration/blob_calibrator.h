#pragma once

#include <fstream>
#include <cuda_runtime.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <optional>
#include <gui.h>
#include "blob_kernels.h"
#include "app_settings.h"
#include "robot_identities.h"
#include "json.hpp"
#include "cuda_utils.h"

using json = nlohmann::json;

class BlobCalibrator {
private:
	std::vector<cv::Point> points;
	GUI* gui;
	AppData* app_data;

	static std::vector<double> scalar_to_json(const cv::Scalar& s);
	static cv::Scalar json_to_scalar(const std::vector<double>& v);
	static std::vector<std::vector<int>> points_to_json(const std::vector<cv::Point>& pts);
	static std::vector<cv::Point> json_to_points(const std::vector<std::vector<int>>& v);

public:
	BlobCalibrator(GUI* gui, AppData* app_data);

	void handle_click(int x, int y);
	static void on_mouse(int event, int x, int y, int flags, void* userdata);

	void reset_points();
	static void print_calibrations(const MatchCalibration& calibration);
	[[nodiscard]] std::optional<CalibrationResult> calibrate_individual();
	[[nodiscard]] std::vector<cv::Point> get_points() const { return points; }

	std::vector<ColorCalibration> save_calibration(const std::string& filename);
	std::vector<ColorCalibration> load_calibration(const std::string& filename);
};

