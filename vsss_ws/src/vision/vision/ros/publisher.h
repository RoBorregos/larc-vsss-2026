#include "app_settings.h"
#include "tracker.h"
#include "field_sizes.h"
#include "rclcpp/rclcpp.hpp"
#include "vsss_vision/msg/entity_state.hpp"
#include <tf2_ros/transform_broadcaster.h>
#include <visualization_msgs/msg/marker.hpp>
#include <visualization_msgs/msg/marker_array.hpp>
#include <geometry_msgs/msg/transform_stamped.hpp>

#pragma once

class Publisher : public rclcpp::Node {
private:
	rclcpp::Publisher<vsss_vision::msg::EntityState>::SharedPtr publisher;
	rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr marker_pub;
	std::unique_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster;

	AppData* app_data;
	Tracker* tracker;

	void publish_legacy_messages() const;
	void publish_tfs(const rclcpp::Time & now);
	void publish_markers(const rclcpp::Time & now);
	void publish_field_markers(const rclcpp::Time & now);
public:
	Publisher(AppData * app_data, Tracker* tracker);
	void generate_random_data();
	void publish_objects();
};