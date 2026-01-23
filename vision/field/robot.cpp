#include "robot.h"

Robot::Robot(GUI* gui, AppData* app_data, Detector* detector): FieldObject(gui, app_data, detector) {
	team = TeamColor::NONE;
	color_pattern = {PatchColor::UNKNOWN, PatchColor::UNKNOWN};
};

void Robot::initialize(const int robot_id, const TeamColor team_color, const std::vector<PatchColor>& pattern) {
	id = robot_id;
	team = team_color;
	color_pattern = pattern;
}

void Robot::update() {
}


bool Robot::is_ally(const TeamColor my_team) const {
	return team == my_team;
}

TeamColor Robot::get_team() const {
	return team;
}

