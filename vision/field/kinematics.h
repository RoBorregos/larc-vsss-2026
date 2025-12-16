#ifndef KINEMATICS_H
#define KINEMATICS_H

#include <deque>
#include <detector.h>

const double EPSILON = 1e-9;


class Kinematics {
private:
	size_t history_size{5};
	double field_width{0.0};
	double field_height{0.0};

	double vel_x{0.0};
	double vel_y{0.0};
	double vel_ang{0.0};

	std::deque<Snapshot> history;

	State current_state{0.0, 0.0, 0.0};

	static double normalize_angle(double angle);
public:
	Kinematics();
	Kinematics(double field_width, double field_height);
	Kinematics(double field_width, double field_height, double start_x, double start_y, double start_facing);

	void set_field_dimensions(double field_width, double field_height);

	void reset();
	void update(double x, double y, double facing, double timestamp);
	State predict(double time_delta);
	[[nodiscard]] State get_current_position() const { return current_state; };
	[[nodiscard]] double get_x() const { return current_state.x; };
	[[nodiscard]] double get_y() const { return current_state.y; };
	[[nodiscard]] double get_facing() const { return current_state.facing; };
};



#endif //KINEMATICS_H
