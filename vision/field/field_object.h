#ifndef FIELD_OBJECT_H
#define FIELD_OBJECT_H

#include <optional>
#include <detection_method.h>
#include <kinematics.h>

class FieldObject {
protected:
	int id;
	std::optional<Kinematics> kinematics;
	DetectionMethod detection_method;

public:
	FieldObject();
	virtual ~FieldObject() = default;

	void update(const cv::Mat& frame, double timestamp);

	[[nodiscard]] int get_id() const;
	[[nodiscard]] std::optional<State> get_position() const;
};



#endif //FIELD_OBJECT_H
