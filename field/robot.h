#ifndef ROBOT_H
#define ROBOT_H

#include "field_object.h"
#include <vector>

class Robot : public FieldObject {
private:
	TeamColor team;
	std::vector<PatchColor> color_pattern;

public:
	Robot(int robot_id, TeamColor team_color, const std::vector<PatchColor>& pattern) {
		this->id = robot_id;
		this->team = team_color;
		this->color_pattern = pattern;

		this->detection_method.setup_as_robot(team, pattern);
	}

	TeamColor getTeam() const { return team; }

	bool isAlly(TeamColor my_team) const {
		return team == my_team;
	}
};

#endif // ROBOT_H