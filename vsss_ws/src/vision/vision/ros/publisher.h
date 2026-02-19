#include "rclcpp/rclcpp.hpp"
#include "vsss_vision/msg/entity_state.hpp"
#include "app_settings.h"
#include "tracker.h"

#pragma once

class Publisher : public rclcpp::Node {
private:
	rclcpp::Publisher<vsss_vision::msg::EntityState>::SharedPtr publisher;

	AppData* app_data;
	Tracker* tracker;
public:
	Publisher(AppData * app_data, Tracker* tracker);

	void publish_objects();

	// void send_detection(float id) {
	// 	auto message = vsss_vision::msg::EntityState();
	//
	// 	message.id = id;
	// 	message.position.x = ;
	//
	// 	publisher_->publish(message);
	// }

};