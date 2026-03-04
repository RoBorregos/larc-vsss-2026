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

class PIDController
{
    public:
    PIDController(double kp, double ki, double kd)
    : kp_(kp), ki_(ki), kd_(kd), prev_error_(0.0), integral_(0.0) {}

    double compute(double error, double dt)
    {
        integral_ += error * dt;
        double derivative = (error - prev_error_) / dt;
        prev_error_ = error;
        return kp_ * error + ki_ * integral_ + kd_ * derivative;
    }

    private:
    double kp_;
    double ki_;
    double kd_;
    double prev_error_;
    double integral_;
};

class ControlNode : public rclcpp::Node
{
    public:
    ControlNode()
    : Node("control_node")
    {
        this->declare_parameter("frame_prefix", "robot1");

        frame_prefix_ = this->get_parameter("frame_prefix").as_string();

        RCLCPP_INFO(this->get_logger(), "Using frame prefix: %s", frame_prefix_.c_str());
        
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
                frame_prefix_ + "/local"
            );
        } catch (tf2::TransformException & ex) {
            RCLCPP_WARN(this->get_logger(), "Could not transform setpoint to robot frame: %s", ex.what());
            return;
        }

        geometry_msgs::msg::Twist cmd;

        tf2::getYaw(t.pose.orientation);

        double e_theta = tf2::getYaw(t.pose.orientation);
        double e_x = t.pose.position.x;
        double e_y = t.pose.position.y;
        
        cmd.angular.z = pid_theta_.compute(e_theta, sampling_time_);
        cmd.linear.x = pid_x_.compute(e_x, sampling_time_);
        cmd.linear.y = pid_y_.compute(e_y, sampling_time_);
        
        RCLCPP_INFO(this->get_logger(), "Publishing cmd: linear_x=%.2f, linear_y=%.2f, angular_z=%.2f", cmd.linear.x, cmd.linear.y, cmd.angular.z);
        cmdPub_->publish(cmd);
    }
    std::string frame_prefix_;
    geometry_msgs::msg::PoseStamped setpoint_;
    rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr strategySub;
    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmdPub_;
    std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
    std::shared_ptr<tf2_ros::TransformListener> tf_listener_;
    rclcpp::TimerBase::SharedPtr timer_;

    PIDController pid_x_{1.0, 0.0, 0.1};
    PIDController pid_y_{1.0, 0.0, 0.1};
    PIDController pid_theta_{1.0, 0.0, 0.1};

    float sampling_time_ = 0.1; // 100ms
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<ControlNode>());
    rclcpp::shutdown();
    return 0;
}