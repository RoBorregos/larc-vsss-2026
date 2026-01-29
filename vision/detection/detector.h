#ifndef DETECTION_METHOD_H
#define DETECTION_METHOD_H

#include <optional>
#include <iostream>
#include <algorithm>
#include <vector>
#include <opencv4/opencv2/opencv.hpp>
#include <opencv2/cudaarithm.hpp>
#include <opencv2/cudafilters.hpp>
#include <opencv2/cudaimgproc.hpp>

#include "gui.h"
#include "blob_calibrator.h"
#include "app_settings.h"
#include "color_segmentation.cuh"

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

struct Patch {
	int id;
	int color;
	cv::Point2d centroid;
	bool parent;
};

struct RobotPatch {
	Patch parent_patch;
	std::vector<Patch> child_patches;
	std::vector<Patch> patches;
	cv::Point2d center{};
	double facing{};
};

class Detector {
private:
	GUI* gui;
	AppData* app_data;
	BlobCalibrator* blob_calibrator;

	std::set<std::string> window_names;

	static void on_debug_mouse(int event, int x, int y, int flags, void* userdata);

	cv::Mat get_label_map();
	std::vector<Patch> get_patches(const cv::Mat& label_map);
	std::vector<RobotPatch> get_robot_patches(const std::vector<Patch>& patches);
	void get_robot_data(std::vector<RobotPatch>& robot_patches);

	[[nodiscard]] cv::cuda::GpuMat get_blob_mask(const cv::cuda::GpuMat& hsv_image);

	cv::Mat visualize_labels(const cv::Mat& label_map);
	static double distance(const cv::Point2d& point_1, const cv::Point2d& point_2);
	static cv::Point2d find_geometric_median(const RobotPatch& robot_patch, int iterations = 5);

	void display_debug(cv::Mat& label_map, const std::vector<Patch>& patches, const std::vector<RobotPatch>& robot_patches);
public:
	Detector(GUI* gui, AppData* app_data, BlobCalibrator* blob_calibrator);

	void update();
	void upload_calibrations(const std::vector<ColorCalibration>& color_calibrations);
};

#endif //DETECTION_METHOD_H
