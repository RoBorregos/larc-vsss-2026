#include <chrono>
#include <functional>
#include <memory>

#include <tf2/exceptions.h>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <rclcpp/rclcpp.hpp>
#include <tf2/exceptions.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/buffer.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>
#include <tf2/utils.h>

using namespace std::chrono_literals;

class ControlNode : public rclcpp::Node
{
    public:
    ControlNode()
    : Node("control_node")
    {
        strategySub = this->create_subscription<geometry_msgs::msg::PoseStamped>(
            "strategy",
            10,
            std::bind(&ControlNode::strategyCallback, this, std::placeholders::_1)
        );

        tf_buffer_ = std::make_unique<tf2_ros::Buffer>(this->get_clock());
        tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

        cmdPub_ = this->create_publisher<geometry_msgs::msg::Twist>("cmd_vel", 10);

        timer_ = this->create_wall_timer(
            100ms, std::bind(&ControlNode::controlLoop, this)
        );

        RCLCPP_INFO(this->get_logger(), "Control Node has been started.");
    }
    private:
    void strategyCallback(const geometry_msgs::msg::PoseStamped::SharedPtr msg)
    {   
        RCLCPP_INFO(this->get_logger(), "Received new setpoint: x=%.2f, y=%.2f", msg->pose.position.x, msg->pose.position.y);
        setpoint_ = *msg;
    }

    void controlLoop()
    {
        if (setpoint_.header.frame_id.empty()) {
            return;
        }

        geometry_msgs::msg::PoseStamped t;

        try {
            tf_buffer_->transform(
                setpoint_,
                t,
                "robot"
            );
        } catch (tf2::TransformException & ex) {
            RCLCPP_WARN(this->get_logger(), "Could not transform setpoint to robot frame: %s", ex.what());
            return;
        }

        geometry_msgs::msg::Twist cmd;

        tf2::getYaw(t.pose.orientation);

        
        cmd.angular.z = tf2::getYaw(t.pose.orientation);
        cmd.linear.x = t.pose.position.x;
        cmd.linear.y = t.pose.position.y;
        
        RCLCPP_INFO(this->get_logger(), "Publishing cmd: linear_x=%.2f, linear_y=%.2f, angular_z=%.2f", cmd.linear.x, cmd.linear.y, cmd.angular.z);
        cmdPub_->publish(cmd);
    }
    geometry_msgs::msg::PoseStamped setpoint_;
    rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr strategySub;
    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmdPub_;
    std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
    std::shared_ptr<tf2_ros::TransformListener> tf_listener_;
    rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<ControlNode>());
    rclcpp::shutdown();
    return 0;
}