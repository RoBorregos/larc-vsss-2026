#include "detection_method.h"

void DetectionMethod::setup_as_ball() {
	this->method = Method::BALL;
}

void DetectionMethod::setup_as_robot(const TeamColor team, const std::vector<PatchColor>& patch_target) {
	this->method = Method::ROBOT;
	this->target_team = team;
	this->target_pattern = patch_target;
}

State DetectionMethod::detect_ball(const cv::Mat &frame) {
	return {0, 0, 0};
}

State DetectionMethod::detect_robot(const cv::Mat &frame) {
	return {0, 0, 0};
}

std::optional<State> DetectionMethod::detect(const cv::Mat &frame) {
	switch(this->method) {
		case Method::BALL:
			return detect_ball(frame);
		case Method::ROBOT:
			return detect_robot(frame);
		case Method::NONE:
			return std::nullopt;
	}

	return std::nullopt;
}





