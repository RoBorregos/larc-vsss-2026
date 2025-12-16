#include "field_object.h"

FieldObject::FieldObject(GUI* gui, AppData* app_data, Detector* detector) : id(-1), app_data(app_data), gui(gui), detector(detector) {
	kinematics = std::nullopt;
}

std::optional<State> FieldObject::get_position() const {
	if (kinematics.has_value()) {
		return kinematics->get_current_position();
	}
	return std::nullopt;
}

int FieldObject::get_id() const {
	return id;
}
