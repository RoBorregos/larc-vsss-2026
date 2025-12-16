#include "field_object.h"

FieldObject::FieldObject() : id(-1) {
	kinematics = Kinematics();
}

void FieldObject::update(const cv::Mat& frame, double timestamp) {
	std::optional<State> detected_state = detection_method.detect(frame);

	if (detected_state.has_value() && kinematics.has_value()) {
		kinematics->update(detected_state->x, detected_state->y, detected_state->facing, timestamp);
	}
}

std::optional<State> FieldObject::get_position() const {
	if (kinematics.has_value()) {
		return kinematics->get_current_position();
	}
	return std::nullopt;
}
