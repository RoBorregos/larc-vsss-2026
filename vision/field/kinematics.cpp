#include "kinematics.h"

Kinematics::Kinematics() = default;

Kinematics::Kinematics(const double field_width, const double field_height) {
	set_field_dimensions(field_width, field_height);
}

Kinematics::Kinematics(
	const double field_width, const double field_height,
	const double start_x, const double start_y, const double start_facing
	) {
	set_field_dimensions(field_width, field_height);

	const double safe_x = std::clamp(start_x, 0.0, this->field_width);
	const double safe_y = std::clamp(start_y, 0.0, this->field_height);
	const double safe_facing = normalize_angle(start_facing);
	this->current_state = {safe_x, safe_y, safe_facing};
}

double Kinematics::normalize_angle(double angle) {
	while (angle <= -M_PI) angle += 2 * M_PI;
	while (angle > M_PI) angle -= 2 * M_PI;

	return angle;
}

void Kinematics::set_field_dimensions(const double field_width, const double field_height) {
	this->field_width = field_width < 0 ? 0 : field_width;
	this->field_height = field_height < 0 ? 0 : field_height;
}

void Kinematics::reset() {
	history.clear();
	vel_x = 0.0;
	vel_y = 0.0;
	vel_ang = 0.0;
}

void Kinematics::update(const double x, const double y, const double facing, const double timestamp) {
	const double safe_x = std::clamp(x, 0.0, this->field_width);
	const double safe_y = std::clamp(y, 0.0, this->field_height);
	const double safe_facing = normalize_angle(facing);

	if (!history.empty()) {
		const Snapshot& last_snapshot = history.back();
		const double time_diff = timestamp - last_snapshot.timestamp + EPSILON;

		vel_x = (safe_x - last_snapshot.state.x) / time_diff;
		vel_y = (safe_y - last_snapshot.state.y) / time_diff;

		double angle_diff = normalize_angle(safe_facing - last_snapshot.state.facing);
		vel_ang = angle_diff / time_diff;
	}

	current_state = {safe_x, safe_y, safe_facing};
	history.push_back({timestamp, current_state});

	if (history.size() > history_size) {
		history.pop_front();
	}
}

State Kinematics::predict(double time_delta) {
	// Do nothing right now
	return current_state;
}

