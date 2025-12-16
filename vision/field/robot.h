#ifndef ROBOT_H
#define ROBOT_H

#include "field_object.h"
#include <vector>

class Robot : public FieldObject {
private:
	TeamColor team;
	std::vector<PatchColor> color_pattern;

public:
	Robot(GUI* gui, AppData* app_data, Detector* detector);
	void initialize(int robot_id, TeamColor team_color, const std::vector<PatchColor>& pattern);

	void update() override;

	[[nodiscard]] TeamColor get_team() const;
	[[nodiscard]] bool is_ally(TeamColor my_team) const;
};

#endif // ROBOT_H