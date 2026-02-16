#pragma once

#include <fstream>
#include <opencv2/opencv.hpp>
#include <vector>
#include <gui.h>
#include <cstring>
#include <cerrno>
#include <sysexits.h>
#include "app_settings.h"
#include "json.hpp"
using json = nlohmann::json;

class ColorCalibrator {
private:
	std::vector<cv::Point> roi_points;
	GUI* gui;
	AppData* app_data;

	void handle_click(int x, int y);
	static std::vector<std::vector<int>> points_to_json(const std::vector<cv::Point>& pts);
	static std::vector<cv::Point> json_to_points(const std::vector<std::vector<int>>& v);
public:
	ColorCalibrator(GUI* gui, AppData* app_data);
	static void on_mouse(int event, int x, int y, int flags, void* userdata);

	void calibrate_roi();
	void remove_last_point();
	void reset_points();
	[[nodiscard]] std::vector<cv::Point> get_roi() const;

	void save_calibration(const std::string& filename);
	void load_calibration(const std::string& filename);
};

