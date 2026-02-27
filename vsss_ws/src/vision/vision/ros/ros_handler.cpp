#include "ros_handler.h"

RosHandler::RosHandler(AppData* app_data, Tracker* tracker) : Node("vision_publisher_node") {
    this->app_data = app_data;
    this->tracker = tracker;
    tf_broadcaster = std::make_unique<tf2_ros::TransformBroadcaster>(this);

    auto qos_profile = rclcpp::SensorDataQoS();
    qos_profile.keep_last(10);

    publisher = this->create_publisher<vsss_vision::msg::EntityState>(
        "/vision/detection",
        qos_profile
    );

	image_subscription = this->create_subscription<sensor_msgs::msg::Image>(
			"/vision/sim_image",
			10,
			std::bind(&RosHandler::image_callback, this, std::placeholders::_1)
		);

    marker_pub = this->create_publisher<visualization_msgs::msg::MarkerArray>("/vision/markers", 10);
}

long long RosHandler::millis() {
	static const auto start_time = std::chrono::steady_clock::now();
	auto current_time = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time);
	return duration.count();
}

void RosHandler::image_callback(sensor_msgs::msg::Image::SharedPtr msg) {
	std::lock_guard<std::mutex> lock(frame_mutex_);
	latest_frame_ = cv_bridge::toCvCopy(msg, "bgr8")->image;
}

cv::Mat RosHandler::get_latest_frame() {
	std::lock_guard<std::mutex> lock(frame_mutex_);
	return latest_frame_.clone();
}

void RosHandler::publish_objects() {
    const rclcpp::Time now = this->get_clock()->now();

	if (millis() - previous_millis >= (1000 / publish_hz)) {
		// generate_random_data();

		publish_tfs(now);

		publish_markers(now);

		publish_field_markers(now);

		publish_legacy_messages();
	}
}

void RosHandler::publish_tfs(const rclcpp::Time & now) {
    geometry_msgs::msg::TransformStamped t_ball;
    t_ball.header.stamp = now;
    t_ball.header.frame_id = "field";
    t_ball.child_frame_id = "ball";
    t_ball.transform.translation.x = tracker->ball.pos.x;
    t_ball.transform.translation.y = tracker->ball.pos.y;
    t_ball.transform.translation.z = 0.0;
    t_ball.transform.rotation.w = 1.0;
    tf_broadcaster->sendTransform(t_ball);
	std::cout << now.seconds() << ": sending ball with x: " << tracker->ball.pos.x << " - y: " << tracker->ball.pos.y << std::endl;

    for (const auto& [id, robot] : tracker->robots) {
       geometry_msgs::msg::TransformStamped t_robot;
       t_robot.header.stamp = now;
       t_robot.header.frame_id = "field";
       t_robot.child_frame_id = "robot_" + std::to_string(id);
       t_robot.transform.translation.x = robot.pos.x;
       t_robot.transform.translation.y = robot.pos.y;
       t_robot.transform.translation.z = 0.0;
       t_robot.transform.rotation.z = sin(robot.facing / 2.0);
       t_robot.transform.rotation.w = cos(robot.facing / 2.0);
       tf_broadcaster->sendTransform(t_robot);
    }
}

void RosHandler::publish_markers(const rclcpp::Time & now) {
    visualization_msgs::msg::MarkerArray marker_array;

    visualization_msgs::msg::Marker ball_marker;
    ball_marker.header.frame_id = "field";
    ball_marker.header.stamp = now;
    ball_marker.ns = "ball";
    ball_marker.id = 20;
    ball_marker.type = visualization_msgs::msg::Marker::SPHERE;
    ball_marker.action = visualization_msgs::msg::Marker::ADD;
    ball_marker.pose.position.x = tracker->ball.pos.x;
    ball_marker.pose.position.y = tracker->ball.pos.y;
    ball_marker.pose.position.z = 0.02135;
    ball_marker.scale.x = 0.0427; ball_marker.scale.y = 0.0427; ball_marker.scale.z = 0.0427;
    ball_marker.color.r = 1.0;
	ball_marker.color.g = tracker->ball.visible ? 0.5 : 0;
	ball_marker.color.b = 0.0;
	ball_marker.color.a = 1.0;
    marker_array.markers.push_back(ball_marker);

    for (const auto& [id, robot] : tracker->robots) {
        visualization_msgs::msg::Marker m;
        m.header.frame_id = "field";
        m.header.stamp = now;
        m.ns = "robots";
        m.id = id;
        m.type = visualization_msgs::msg::Marker::CUBE;
        m.action = visualization_msgs::msg::Marker::ADD;
        m.pose.position.x = robot.pos.x;
        m.pose.position.y = robot.pos.y;
        m.pose.position.z = 0.035;
        m.pose.orientation.z = sin(robot.facing / 2.0);
        m.pose.orientation.w = cos(robot.facing / 2.0);
        m.scale.x = 0.075; m.scale.y = 0.075; m.scale.z = 0.070;

    	if (robot.team() == ObjectType::Yellow) {
	    	m.color.r = robot.visible ? 0.8 : 0.6;
    		m.color.g = 0.8;
    		m.color.b = 0.0;
    		m.color.a = 1.0;
    	} else {
	    	m.color.r = 0.0;
    		m.color.g = robot.visible ? 0.2 : 0.4;
    		m.color.b = 0.8;
    		m.color.a = 1.0;
    	}

        marker_array.markers.push_back(m);
    }
    marker_pub->publish(marker_array);
}

void RosHandler::publish_field_markers(const rclcpp::Time & now) {
    visualization_msgs::msg::MarkerArray field_array;
    visualization_msgs::msg::Marker lines;

    lines.header.frame_id = "field";
    lines.header.stamp = now;
    lines.ns = "field_layout";
    lines.id = 0;
    lines.type = visualization_msgs::msg::Marker::LINE_LIST;
    lines.action = visualization_msgs::msg::Marker::ADD;
	lines.scale.x = FieldSizes::LINE_THICKNESS;
	lines.color.r = 1.0;
	lines.color.g = 1.0;
	lines.color.b = 1.0;
	lines.color.a = 1.0;

    auto add_line = [&](double x1, double y1, double x2, double y2) {
        geometry_msgs::msg::Point p1, p2;
        p1.x = x1; p1.y = y1; p1.z = 0;
        p2.x = x2; p2.y = y2; p2.z = 0;
        lines.points.push_back(p1);
        lines.points.push_back(p2);
    };

	add_line(-FieldSizes::HALF_LENGTH,  FieldSizes::HALF_WIDTH,  FieldSizes::HALF_LENGTH,  FieldSizes::HALF_WIDTH);
	add_line(-FieldSizes::HALF_LENGTH, -FieldSizes::HALF_WIDTH,  FieldSizes::HALF_LENGTH, -FieldSizes::HALF_WIDTH);
	add_line(-FieldSizes::HALF_LENGTH, -FieldSizes::HALF_WIDTH, -FieldSizes::HALF_LENGTH,  FieldSizes::HALF_WIDTH);
	add_line( FieldSizes::HALF_LENGTH, -FieldSizes::HALF_WIDTH,  FieldSizes::HALF_LENGTH,  FieldSizes::HALF_WIDTH);

	add_line(0.0, 0.65, 0.0, -0.65);
	add_line(0.0, FieldSizes::HALF_WIDTH, 0.0, -FieldSizes::HALF_WIDTH);
	
	add_line(FieldSizes::GOAL_AREA_X,  FieldSizes::GOAL_AREA_HALF_WIDTH, FieldSizes::GOAL_AREA_X, -FieldSizes::GOAL_AREA_HALF_WIDTH);
	add_line(FieldSizes::GOAL_AREA_X,  FieldSizes::GOAL_AREA_HALF_WIDTH, FieldSizes::HALF_LENGTH,  FieldSizes::GOAL_AREA_HALF_WIDTH);
	add_line(FieldSizes::GOAL_AREA_X, -FieldSizes::GOAL_AREA_HALF_WIDTH, FieldSizes::HALF_LENGTH, -FieldSizes::GOAL_AREA_HALF_WIDTH);

	add_line(-FieldSizes::GOAL_AREA_X,  FieldSizes::GOAL_AREA_HALF_WIDTH, -FieldSizes::GOAL_AREA_X, -FieldSizes::GOAL_AREA_HALF_WIDTH);
	add_line(-FieldSizes::GOAL_AREA_X,  FieldSizes::GOAL_AREA_HALF_WIDTH, -FieldSizes::HALF_LENGTH,  FieldSizes::GOAL_AREA_HALF_WIDTH);
	add_line(-FieldSizes::GOAL_AREA_X, -FieldSizes::GOAL_AREA_HALF_WIDTH, -FieldSizes::HALF_LENGTH, -FieldSizes::GOAL_AREA_HALF_WIDTH);

	auto add_cross = [&](double x, double y) {
		add_line(x - FieldSizes::CROSS_SIZE, y, x + FieldSizes::CROSS_SIZE, y);
		add_line(x, y - FieldSizes::CROSS_SIZE, x, y + FieldSizes::CROSS_SIZE);
	};

	add_cross(FieldSizes::CENTER_CROSS_X, 0.0);
	add_cross(FieldSizes::CROSS_X_POS,  FieldSizes::CROSS_Y_POS);
	add_cross(FieldSizes::CROSS_X_POS, -FieldSizes::CROSS_Y_POS);

	add_cross(-FieldSizes::CENTER_CROSS_X, 0.0);
	add_cross(-FieldSizes::CROSS_X_POS,  FieldSizes::CROSS_Y_POS);
	add_cross(-FieldSizes::CROSS_X_POS, -FieldSizes::CROSS_Y_POS);
    field_array.markers.push_back(lines);

    visualization_msgs::msg::Marker circle;
    circle.header = lines.header;
    circle.type = visualization_msgs::msg::Marker::LINE_STRIP;
    circle.ns = "field_circle";
    circle.id = 1;
    circle.scale.x = 0.01;
    circle.color = lines.color;

    for (int i = 0; i <= 64; ++i) {
        double angle = i * (2.0 * M_PI / 64.0);
        geometry_msgs::msg::Point p;
        p.x = FieldSizes::CENTER_CIRCLE_RADIUS * cos(angle);
        p.y = FieldSizes::CENTER_CIRCLE_RADIUS * sin(angle);
        circle.points.push_back(p);
    }
    field_array.markers.push_back(circle);

    marker_pub->publish(field_array);
}

void RosHandler::publish_legacy_messages() const {
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

	for (const auto& robot : tracker->robots) {
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

/* simulate realistic circular and linear movements for the ball and robots
 *
 * Useful if strategy or control needs random values in order to test a feature
 */
void RosHandler::generate_random_data() {
	static double angle = 0.0;
	angle += 0.05;

	tracker->ball.pos.x = 0.8 * cos(angle);
	tracker->ball.pos.y = 0.4 * sin(angle);
	tracker->ball.vel.x = -0.8 * sin(angle);
	tracker->ball.vel.y = 0.4 * cos(angle);

	if (tracker->robots.empty()) {
		std::set<int> test_ids = {0, 1, 2};
		for (int id : test_ids) {
			auto& robot = tracker->robots[id];
			robot.pos.x = -0.5 + (id * 0.5);
			robot.pos.y = 0.2 * cos(angle + id);
			robot.facing = angle * (id + 1) * 0.1;
		}
	} else {
		for (auto& [id, robot] : tracker->robots) {
			robot.pos.x += 0.001 * cos(angle);
			robot.facing += 0.02;
		}
	}
}