#ifndef DETECTION_METHOD_H
#define DETECTION_METHOD_H

#include <optional>
#include <iostream>
#include <vector>
#include <opencv4/opencv2/opencv.hpp>

#include "gui.h"
#include "blob_calibrator.h"
#include "app_settings.h"
#include "image_preprocessing.h"

enum class TeamColor { NONE, BLUE, YELLOW };
enum class PatchColor { RED, GREEN, CYAN, MAGENTA, UNKNOWN };

struct DebugWindowData {
	BlobCalibrator* calibrator;
	cv::Rect roi_offset;
	double zoom_factor;
};

struct State {
	double x;
	double y;
	double facing;
};

struct Snapshot {
	double timestamp;
	State state;
};

struct DetectedPatch {
	std::vector<cv::Point> contour;
	unsigned long frame_id;
	unsigned long pixel_count;
	cv::Scalar min_hsv;
	cv::Scalar max_hsv;
	cv::Scalar avg_hsv;
	cv::Scalar std_dev_hsv;
};

class Detector {
private:
	GUI* gui;
	AppData* app_data;
	BlobCalibrator* blob_calibrator;

	std::vector<DebugWindowData> debug_window_contexts;

	cv::Mat hsv_mat;

	std::vector<std::vector<DetectedPatch>> square_patches;
	std::vector<std::vector<DetectedPatch>> categorized_objects;
	std::vector<std::vector<cv::Point>> found_objects;

	std::set<std::string> window_names;

	static void on_debug_mouse(int event, int x, int y, int flags, void* userdata);

	bool valid_square(const std::vector<cv::Point>& contour, std::vector<cv::Point>& dst);
	static double angle_cosine(cv::Point pt1, cv::Point pt2, cv::Point pt0);

	void filter_patches();
	void categorize_objects();
	void find_objects();
	static void show_debug_rois(const cv::Mat& src_img, const std::vector<std::vector<cv::Point>>& contours);
public:
	Detector(GUI* gui, AppData* app_data, BlobCalibrator* blob_calibrator);

	void update();
	void display_debug_info();
	void destroy_debug_windows();

	std::optional<State> ball();
	std::optional<State> robot(TeamColor team, const std::vector<PatchColor>& color_pattern);
};

#endif //DETECTION_METHOD_H
