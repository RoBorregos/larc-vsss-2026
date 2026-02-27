#include "app_settings.h"
#include "tracker.h"
#include "field_sizes.h"
#include "rclcpp/rclcpp.hpp"
#include "vsss_vision/msg/entity_state.hpp"
#include <tf2_ros/transform_broadcaster.h>
#include <visualization_msgs/msg/marker.hpp>
#include <visualization_msgs/msg/marker_array.hpp>
#include <geometry_msgs/msg/transform_stamped.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <cv_bridge/cv_bridge.h>
#include <chrono>

#pragma once

class RosHandler : public rclcpp::Node {
private:
	rclcpp::Publisher<vsss_vision::msg::EntityState>::SharedPtr publisher;
	rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr marker_pub;
	rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_subscription;
	std::unique_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster;

	AppData* app_data;
	Tracker* tracker;

	cv::Mat latest_frame_;
	std::mutex frame_mutex_;

	void publish_legacy_messages() const;
	void publish_tfs(const rclcpp::Time & now);
	void publish_markers(const rclcpp::Time & now);
	void publish_field_markers(const rclcpp::Time & now);
	void image_callback(sensor_msgs::msg::Image::SharedPtr msg);

	long long millis();
	long long previous_millis = millis();
	float publish_hz = 15;
public:
	RosHandler(AppData * app_data, Tracker* tracker);
	void generate_random_data();
	void publish_objects();
	cv::Mat get_latest_frame();
};