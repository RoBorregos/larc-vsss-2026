#include <functional>
#include <memory>
#include <string>

#include <geometry_msgs/msg/transform_stamped.hpp>
#include <rclcpp/rclcpp.hpp>
#include <tf2/LinearMath/Quaternion.h>
#include <tf2_ros/transform_broadcaster.h>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/quaternion.hpp>

class LocalizationNode : public rclcpp::Node
{
public:
    LocalizationNode() : Node("localization_node")
    {
        this->declare_parameter("robot_name", "vsss_robot");
        frame_prefix_ = this->get_parameter("robot_name").as_string();

        std::string node_name_ = this->get_fully_qualified_name();

        RCLCPP_INFO(this->get_logger(), "%s Started for robot: %s", node_name_.c_str(), frame_prefix_.c_str());

        tf_broadcaster_ = std::make_unique<tf2_ros::TransformBroadcaster>(*this);

        orientation_.w = 1.0;
        
        visionSub_ = this->create_subscription<geometry_msgs::msg::PoseStamped>(
            "vision",
            10,
            std::bind(&LocalizationNode::visionCallback, this, std::placeholders::_1)
        );

        telemetrySub_ = this->create_subscription<geometry_msgs::msg::Quaternion>(
            "telemetry",
            10,
            std::bind(&LocalizationNode::telemetryCallback, this, std::placeholders::_1)
        );

        RCLCPP_INFO(this->get_logger(), "Localization Node has been started.");
    }
private:
    void broadcastPose() 
    {
        geometry_msgs::msg::TransformStamped t;

        t.header.stamp = this->get_clock()->now();
        t.header.frame_id = "field";
        t.child_frame_id = frame_prefix_ + "/local";
        
        t.transform.translation.x = x_;
        t.transform.translation.y = y_;
        t.transform.translation.z = 0;

        t.transform.rotation = orientation_;
        
        RCLCPP_INFO(this->get_logger(), "Broadcasting transform: x=%.2f, y=%.2f, orientation_w=%.2f", x_, y_, orientation_.w);
        
        tf_broadcaster_->sendTransform(t);
    }
    void visionCallback(const geometry_msgs::msg::PoseStamped::SharedPtr msg)
    {
        x_ = msg->pose.position.x;
        y_ = msg->pose.position.y;

        RCLCPP_INFO(this->get_logger(), "Updated position from vision: x=%.2f, y=%.2f", x_, y_);
        broadcastPose();        
    }
    void telemetryCallback(const geometry_msgs::msg::Quaternion::SharedPtr msg)
    {
        orientation_ = *msg;
        RCLCPP_INFO(this->get_logger(), "Updated orientation from telemetry: w=%.2f, x=%.2f, y=%.2f, z=%.2f", orientation_.w, orientation_.x, orientation_.y, orientation_.z);
        broadcastPose();
    }
    std::string frame_prefix_;

    double x_;
    double y_;
    geometry_msgs::msg::Quaternion orientation_;

    rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr visionSub_;
    rclcpp::Subscription<geometry_msgs::msg::Quaternion>::SharedPtr telemetrySub_;
    std::unique_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_;

};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<LocalizationNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}