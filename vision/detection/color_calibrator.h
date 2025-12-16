#ifndef COLOR_CALIBRATOR_H
#define COLOR_CALIBRATOR_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <gui.h>
#include "app_settings.h"

class ColorCalibrator {
private:
	std::vector<cv::Point> roi_points;
	GUI* drawer;
	AppData* app_data;

	void handle_click(int x, int y);
public:
	ColorCalibrator(GUI* drawer, AppData* app_data);
	static void on_mouse(int event, int x, int y, int flags, void* userdata);

	void calibrate_roi();
	void remove_last_point();
	void reset_points();
	[[nodiscard]] std::vector<cv::Point> get_roi() const;
};

#endif // COLOR_CALIBRATOR_H