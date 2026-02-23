#pragma once

#include <optional>
#include <iostream>
#include <algorithm>
#include <ranges>
#include <vector>
#include <unordered_map>
#include <opencv4/opencv2/opencv.hpp>
#include <cuda_runtime.h>
#include <opencv2/core/cuda.hpp>
#include <opencv2/cudaarithm.hpp>
#include <opencv2/cudafilters.hpp>
#include <opencv2/cudaimgproc.hpp>

#include "gui.h"
#include "blob_calibrator.h"
#include "app_settings.h"
#include "robot_identities.h"
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

struct RobotRelationship {
	int parent_id;
	int child_1_id;
	std::optional<int> child_2_id;
};

class Detector {
private:
	GUI* gui;
	AppData* app_data;
	BlobCalibrator* blob_calibrator;

	std::set<std::string> window_names;

	static void on_debug_mouse(int event, int x, int y, int flags, void* userdata);

	cv::Mat get_label_map();
	std::vector<std::shared_ptr<Patch>> get_patches(const cv::Mat& label_map);
	std::vector<RobotPatch> get_robot_patches(std::vector<std::shared_ptr<Patch>>& patches);
	std::optional<BallPatch> get_ball_patch(const std::vector<std::shared_ptr<Patch>>& patches);
	std::vector<RobotPatch> get_isolated_robot_patches(std::vector<std::shared_ptr<Patch>>& patches);
	std::vector<RobotPatch> get_clustered_robot_patches(const std::vector<std::shared_ptr<Patch>>& clustered_patches);
	void get_robot_data(std::vector<RobotPatch>& robot_patches);

	[[nodiscard]] cv::cuda::GpuMat get_blob_mask(const cv::cuda::GpuMat& hsv_image);

	cv::Mat visualize_labels(const cv::Mat& label_map);

	void display_debug(cv::Mat& label_map, const std::vector<std::shared_ptr<Patch>>& patches, const std::vector<RobotPatch>& robot_patches, std
	                   ::optional<BallPatch>& ball_patch);

	static constexpr int MINIMUM_DISTANCE = 20;
	static constexpr int MAX_ITERATIONS = 1000;
public:
	Detector(GUI* gui, AppData* app_data, BlobCalibrator* blob_calibrator);

	[[nodiscard]] std::pair<std::vector<RobotPatch>, std::optional<BallPatch>> update();
	void upload_calibrations(const std::vector<ColorCalibration>& color_calibrations);
};
