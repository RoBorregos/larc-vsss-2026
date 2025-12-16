#ifndef DETECTION_METHOD_H
#define DETECTION_METHOD_H

#include <string>
#include <opencv4/opencv2/opencv.hpp>
#include <kinematics.h>
#include <optional>

enum class TeamColor { NONE, BLUE, YELLOW };
enum class PatchColor { RED, GREEN, CYAN, MAGENTA, UNKNOWN };

enum class Method {
	NONE,
	BALL,
	ROBOT
};

class DetectionMethod {
private:
	Method method{Method::NONE};
	std::vector<PatchColor> target_pattern{PatchColor::UNKNOWN, PatchColor::UNKNOWN};
	TeamColor target_team{TeamColor::NONE};

	State detect_ball(const cv::Mat& frame);
	State detect_robot(const cv::Mat& frame);

public:
	DetectionMethod() = default;

	void setup_as_ball();
	void setup_as_robot(TeamColor team, const std::vector<PatchColor>& patch_target);

	std::optional<State> detect(const cv::Mat& frame);
};

#endif //DETECTION_METHOD_H
