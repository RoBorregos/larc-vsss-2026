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
#include "ros_handler.h"

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
	std::shared_ptr<RosHandler> ros_handler;
	std::set<std::string> window_names;

	std::optional<BallPatch> ball_patch = std::nullopt;
	std::vector<RobotPatch> robot_patches = {};
	std::vector<std::shared_ptr<Patch>> patches = {};

	static void on_debug_mouse(int event, int x, int y, int flags, void* userdata);

	cv::Mat get_label_map();
	void get_patches(const cv::Mat& label_map);
	void get_robot_patches();
	void get_ball_patch();

	void get_isolated_robot_patches();
	void get_clustered_robot_patches(std::vector<std::shared_ptr<Patch>> clustered_patches);

	void get_single_robot_with_ball(const std::shared_ptr<Patch>& parent, std::vector<std::shared_ptr<Patch>>& children);

	void get_robot_data();

	[[nodiscard]] cv::cuda::GpuMat get_blob_mask(const cv::cuda::GpuMat& hsv_image, bool debug_mode);

	cv::Mat visualize_labels(const cv::Mat& label_map);

	void display_debug(cv::Mat& label_map, const std::vector<std::shared_ptr<Patch>>& patches, const std::vector<RobotPatch>& robot_patches, std
	                   ::optional<BallPatch>& ball_patch);

	static constexpr int MINIMUM_DISTANCE = 20;
	static constexpr int MAX_ITERATIONS = 1000;
    static constexpr int PATCH_MIN_AREA = 10;
public:
	Detector(GUI* gui, AppData* app_data, BlobCalibrator* blob_calibrator, std::shared_ptr<RosHandler> ros_handler);

	[[nodiscard]] std::pair<std::vector<RobotPatch>, std::optional<BallPatch>> update();
	void upload_calibrations(const std::vector<ColorCalibration>& color_calibrations);
};
