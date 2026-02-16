// ======================= LIBRARIES =======================

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/pose_array.hpp>
#include <geometry_msgs/msg/pose2_d.hpp>
#include <std_msgs/msg/bool.hpp>
#include <vector>
#include <cmath>
#include <algorithm>

// ======================= CONSTANTS =======================

const float PI = 3.14159265359f;

// Field dimensions (cm)
const float FIELD_WIDTH  = 130.0f;
const float FIELD_HEIGHT = 150.0f;

// Goals
const float GOAL_X = 65.0f;
const float GOAL_Y_OWN   = 0.0f;
const float GOAL_Y_ENEMY = 150.0f;

// Robot limits
const float MAX_LIN_SPEED = 150.0f; // cm/s
const float MAX_ROT_SPEED = 10.0f;  // rad/s

// Strategy weights
const float WEIGHT_TIME  = 1.0f;
const float WEIGHT_ALIGN = 0.5f;
const float PENALTY_NO_KICKER = 1.5f;

// Prediction
const float INERTIA_LOOKAHEAD = 0.2f;

// Strategy parameters
const float ATTACK_OFFSET = 5.0f;       // cm behind ball
const float HYSTERESIS_BONUS = 0.15f;    // stability bonus

// Goalkeeper limits
const float GK_Y = 8.0f;
const float GK_X_RANGE = 20.0f;

// ======================= STRUCTS =======================

struct Vec2 {
    float x, y;

    float dist(const Vec2& o) const {
        return std::hypot(x - o.x, y - o.y);
    }

    Vec2 operator-(const Vec2& o) const {
        return {x - o.x, y - o.y};
    }

    Vec2 normalized() const {
        float n = std::hypot(x, y);
        if (n < 0.001f) return {0.0f, 0.0f};
        return {x / n, y / n};
    }

    float dot(const Vec2& o) const {
        return x * o.x + y * o.y;
    }
};

struct RobotData {
    int id;
    Vec2 pos;
    Vec2 vel;
    float angle;
    bool kickerReady;
};

// ======================= COST FUNCTION =======================

float calculateTimeToAction(const RobotData& robot, const Vec2& ballPos) {

    float distance = robot.pos.dist(ballPos);

    Vec2 dirToBall = (ballPos - robot.pos).normalized();
    float velTowardsBall = robot.vel.dot(dirToBall);

    float effectiveDist = distance - velTowardsBall * INERTIA_LOOKAHEAD;
    if (effectiveDist < 0.0f) effectiveDist = 0.0f;

    float timeToReach = effectiveDist / MAX_LIN_SPEED;

    float angleToBall = std::atan2(dirToBall.y, dirToBall.x);
    float angleError = angleToBall - robot.angle;

    while (angleError > PI)  angleError -= 2.0f * PI;
    while (angleError < -PI) angleError += 2.0f * PI;

    float timeToAlign = std::abs(angleError) / MAX_ROT_SPEED;

    float kickerPenalty = robot.kickerReady ? 0.0f : PENALTY_NO_KICKER;

    return (WEIGHT_TIME * timeToReach) +
           (WEIGHT_ALIGN * timeToAlign) +
           kickerPenalty;
}

// ======================= NODE =======================

class StrategyNode : public rclcpp::Node {
public:
    StrategyNode() : Node("strategy_node") {

        auto qos = rclcpp::QoS(1).best_effort();

        posesSub_ = this->create_subscription<geometry_msgs::msg::PoseArray>(
            "/vision/poses", qos,
            std::bind(&StrategyNode::posesCallback, this, std::placeholders::_1)
        );

        kickerSub_ = this->create_subscription<std_msgs::msg::Bool>(
            "/robot/kicker_ready", qos,
            std::bind(&StrategyNode::kickerCallback, this, std::placeholders::_1)
        );

        for (int i = 0; i < 3; i++) {
            goalPubs_.push_back(
                this->create_publisher<geometry_msgs::msg::Pose2D>(
                    "/strategy/robot_" + std::to_string(i) + "/goal", 10
                )
            );
        }

        lastTime_ = this->now();
        RCLCPP_INFO(this->get_logger(), "Strategy node running");
    }

private:
    rclcpp::Subscription<geometry_msgs::msg::PoseArray>::SharedPtr posesSub_;
    rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr kickerSub_;
    std::vector<rclcpp::Publisher<geometry_msgs::msg::Pose2D>::SharedPtr> goalPubs_;

    bool kickerReady_ = false;
    int currentAttacker_ = -1;

    std::vector<Vec2> prevPoses_;
    rclcpp::Time lastTime_;

    void posesCallback(const geometry_msgs::msg::PoseArray::SharedPtr msg) {

        if (msg->poses.size() < 4) return;

        // TIME STEP
        rclcpp::Time now = this->now();
        double dt = (now - lastTime_).seconds();
        if (dt <= 0.0) dt = 0.001;

        // BALL
        Vec2 ball {
            (float)msg->poses[0].position.x,
            (float)msg->poses[0].position.y
        };

        // ROBOTS
        std::vector<RobotData> robots(3);

        for (int i = 0; i < 3; i++) {
            robots[i].id = i;
            robots[i].pos = {
                (float)msg->poses[i + 1].position.x,
                (float)msg->poses[i + 1].position.y
            };
            robots[i].kickerReady = kickerReady_;

            double qx = msg->poses[i + 1].orientation.x;
            double qy = msg->poses[i + 1].orientation.y;
            double qz = msg->poses[i + 1].orientation.z;
            double qw = msg->poses[i + 1].orientation.w;

            robots[i].angle = std::atan2(
                2.0 * (qw * qz + qx * qy),
                1.0 - 2.0 * (qy * qy + qz * qz)
            );

            robots[i].vel = {0.0f, 0.0f};
            if (prevPoses_.size() > (size_t)i + 1) {
                robots[i].vel.x = (robots[i].pos.x - prevPoses_[i + 1].x) / dt;
                robots[i].vel.y = (robots[i].pos.y - prevPoses_[i + 1].y) / dt;
            }
        }

        // SELECT ROLES
        float bestCost = 1e9;
        int attacker = currentAttacker_;

        for (auto& r : robots) {
            float cost = calculateTimeToAction(r, ball);
            if (r.id == currentAttacker_) cost -= HYSTERESIS_BONUS;

            if (cost < bestCost) {
                bestCost = cost;
                attacker = r.id;
            }
        }

        currentAttacker_ = attacker;
        int defender = (attacker + 1) % 3;
        int goalie   = (attacker + 2) % 3;

        // ATTACKER GOAL
        Vec2 enemyGoal{GOAL_X, GOAL_Y_ENEMY};
        Vec2 attackDir = (enemyGoal - ball).normalized();

        Vec2 attackPoint{
            ball.x - attackDir.x * ATTACK_OFFSET,
            ball.y - attackDir.y * ATTACK_OFFSET
        };

        geometry_msgs::msg::Pose2D atk;
        atk.x = attackPoint.x;
        atk.y = attackPoint.y;
        atk.theta = std::atan2(attackDir.y, attackDir.x);

        // DEFENDER GOAL
        geometry_msgs::msg::Pose2D def;
        def.x = (ball.x + GOAL_X) * 0.5f;
        def.y = (ball.y + GOAL_Y_OWN) * 0.5f;
        def.theta = std::atan2(ball.y - def.y, ball.x - def.x);

        // GOALKEEPER GOAL
        geometry_msgs::msg::Pose2D gk;
        gk.y = GK_Y;
        gk.x = std::clamp(ball.x, GOAL_X - GK_X_RANGE, GOAL_X + GK_X_RANGE);
        gk.theta = std::atan2(ball.y - gk.y, ball.x - gk.x);

        // PUBLISH
        goalPubs_[attacker]->publish(atk);
        goalPubs_[defender]->publish(def);
        goalPubs_[goalie]->publish(gk);

        // HISTORY UPDATE
        prevPoses_.clear();
        for (const auto& p : msg->poses) {
            prevPoses_.push_back({
                (float)p.position.x,
                (float)p.position.y
            });
        }

        lastTime_ = now;
    }

    void kickerCallback(const std_msgs::msg::Bool::SharedPtr msg) {
        kickerReady_ = msg->data;
    }
};

// ======================= MAIN =======================

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<StrategyNode>());
    rclcpp::shutdown();
    return 0;
}
