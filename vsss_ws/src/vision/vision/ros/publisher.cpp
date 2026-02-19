#include "publisher.h"

Publisher::Publisher(AppData* app_data, Tracker* tracker) : Node("vision_publisher_node") {
	this->app_data = app_data;
	this->tracker = tracker;
	publisher = this->create_publisher<vsss_vision::msg::EntityState>("/vision/detection", 10);
}

void Publisher::publish_objects() {
	auto ball_message = vsss_vision::msg::EntityState();
	ball_message.id = 20;
	ball_message.position.position.x = tracker->ball.pos.x;
	ball_message.position.position.y = tracker->ball.pos.y;
	ball_message.position.position.z = 0;

	ball_message.position.orientation.z = 0.0;
	ball_message.position.orientation.w = 1.0;

	ball_message.velocity.linear.x = tracker->ball.vel.x;
	ball_message.velocity.linear.y = tracker->ball.vel.y;
	publisher->publish(ball_message);

	for (const auto robot : tracker->robots) {
		auto robot_message = vsss_vision::msg::EntityState();
		robot_message.id = robot.first;

		robot_message.position.position.x = robot.second.pos.x;
		robot_message.position.position.y = robot.second.pos.y;
		robot_message.position.position.z = 0;

		robot_message.position.orientation.z = sin(robot.second.facing / 2.0);
		robot_message.position.orientation.w = cos(robot.second.facing / 2.0);

		robot_message.velocity.linear.x = robot.second.vel.x;
		robot_message.velocity.linear.y = robot.second.vel.y;

		publisher->publish(robot_message);
	}
}


