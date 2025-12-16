#ifndef FIELD_OBJECT_H
#define FIELD_OBJECT_H

#include <optional>
#include <detector.h>
#include <kinematics.h>

class FieldObject {
protected:
	int id;
	std::optional<Kinematics> kinematics;
	Detector* detector;
	AppData* app_data;
	GUI* gui;

public:
	FieldObject(GUI* gui, AppData* app_data, Detector* detector);
	virtual ~FieldObject() = default;

	virtual void update() = 0;

	[[nodiscard]] int get_id() const;
	[[nodiscard]] std::optional<State> get_position() const;
};



#endif //FIELD_OBJECT_H
