import math
from enum import Enum

import rclpy
from rclpy.node import Node

from vsss_vision.msg import EntityState
from geometry_msgs.msg import Pose2D
from std_msgs.msg import Bool

# ======================= CONSTANTS =======================

PI = math.pi

GOAL_X = 65.0
GOAL_Y_OWN = 0.0
GOAL_Y_ENEMY = 150.0

MAX_LIN_SPEED = 150.0
MAX_ROT_SPEED = 10.0

WEIGHT_TIME = 1.0
WEIGHT_ALIGN = 0.5
PENALTY_NO_KICKER = 1.5

INERTIA_LOOKAHEAD = 0.2
ATTACK_OFFSET = 5.0
HYSTERESIS_BONUS = 0.15

GK_Y = 8.0
GK_X_RANGE = 20.0


# ======================= DATA STRUCTURES =======================

class Vec2:
    def __init__(self, x=0.0, y=0.0):
        self.x = x
        self.y = y

    def dist(self, o):
        return math.hypot(self.x - o.x, self.y - o.y)

    def __sub__(self, o):
        return Vec2(self.x - o.x, self.y - o.y)

    def __add__(self, o):
        return Vec2(self.x + o.x, self.y + o.y)

    def __mul__(self, s):
        return Vec2(self.x * s, self.y * s)

    def normalized(self):
        n = math.hypot(self.x, self.y)
        if n < 0.001:
            return Vec2()
        return Vec2(self.x / n, self.y / n)

    def dot(self, o):
        return self.x * o.x + self.y * o.y


class RobotData:
    def __init__(self):
        self.id = -1
        self.pos = Vec2()
        self.vel = Vec2()
        self.angle = 0.0
        self.kickerReady = False


class AttackState(Enum):
    INTERCEPT = 0
    ALIGN = 1
    PUSH = 2


# ======================= COST FUNCTION =======================

def calculate_time_to_action(robot: RobotData, ball_pos: Vec2):

    distance = robot.pos.dist(ball_pos)

    dir_to_ball = (ball_pos - robot.pos).normalized()
    vel_towards_ball = robot.vel.dot(dir_to_ball)

    effective_dist = distance - vel_towards_ball * INERTIA_LOOKAHEAD
    if effective_dist < 0.0:
        effective_dist = 0.0

    time_to_reach = effective_dist / MAX_LIN_SPEED

    angle_to_ball = math.atan2(dir_to_ball.y, dir_to_ball.x)
    angle_error = angle_to_ball - robot.angle

    while angle_error > PI:
        angle_error -= 2.0 * PI
    while angle_error < -PI:
        angle_error += 2.0 * PI

    time_to_align = abs(angle_error) / MAX_ROT_SPEED

    kicker_penalty = 0.0 if robot.kickerReady else PENALTY_NO_KICKER

    return (WEIGHT_TIME * time_to_reach +
            WEIGHT_ALIGN * time_to_align +
            kicker_penalty)


# ======================= NODE =======================

class StrategyNode(Node):

    def __init__(self):
        super().__init__('strategy_node')

        qos = 1

        self.vision_sub = self.create_subscription(
            EntityState,
            '/vision/detection',
            self.vision_callback,
            qos
        )

        self.kicker_sub = self.create_subscription(
            Bool,
            '/robot/kicker_ready',
            self.kicker_callback,
            qos
        )

        self.goal_pubs = []
        for i in range(3):
            self.goal_pubs.append(
                self.create_publisher(
                    Pose2D,
                    f'/strategy/robot_{i}/goal',
                    10
                )
            )

        # State variables
        self.ball_pos = Vec2()
        self.ball_vel = Vec2()
        self.ball_received = False

        # --- Team configuration ---
        self.team_color = "yellow"      # intercambiable
        self.enemy_color = "blue"

        self.ally_map = {}
        self.enemy_map = {}

        self.kicker_ready = False
        self.current_attacker = -1
        self.attack_state = AttackState.INTERCEPT

        self.get_logger().info("Strategy Node with FSM running")

    # ======================= CALLBACKS =======================

    def vision_callback(self, msg: EntityState):

        entity_id = msg.id

        # BALL (id 20)
        if entity_id == 20:
            self.ball_pos.x = msg.position.position.x
            self.ball_pos.y = msg.position.position.y
            self.ball_vel.x = msg.velocity.linear.x
            self.ball_vel.y = msg.velocity.linear.y
            self.ball_received = True
            return

        # ================= ALLIES =================
if self.is_ally(entity_id):

    team_id = self.convert_to_team_index(entity_id)

    r = RobotData()
    r.id = team_id

    r.pos.x = msg.position.position.x
    r.pos.y = msg.position.position.y

    r.vel.x = msg.velocity.linear.x
    r.vel.y = msg.velocity.linear.y

    qz = msg.position.orientation.z
    qw = msg.position.orientation.w

    r.angle = math.atan2(
        2.0 * qw * qz,
        1.0 - 2.0 * qz * qz
    )

    r.kickerReady = self.kicker_ready

    self.ally_map[team_id] = r

    self.run_strategy()
    return


# ================= ENEMIES =================
if self.is_enemy(entity_id):

    enemy_id = self.convert_to_team_index(entity_id)

    r = RobotData()
    r.id = enemy_id

    r.pos.x = msg.position.position.x
    r.pos.y = msg.position.position.y

    r.vel.x = msg.velocity.linear.x
    r.vel.y = msg.velocity.linear.y

    qz = msg.position.orientation.z
    qw = msg.position.orientation.w

    r.angle = math.atan2(
        2.0 * qw * qz,
        1.0 - 2.0 * qz * qz
    )

    self.enemy_map[enemy_id] = r

    def kicker_callback(self, msg: Bool):
        self.kicker_ready = msg.data
    
    def set_team_color(self, color: str):
        if color not in ["yellow","blue"]:
            self.get_logger().error("Invalid team color")
            return 
        self.team_color=color 
        self.enemy_color="blue" if color == "yellow" else "yellow"

        self.get_logger().info("Team color set to{self.team_color}")
    def is_ally(self,entity_id:int) -> bool:
        if self.team_color == "yellow":
            return 0 <= entity_id <= 9
        else:
            return 10<= entity_id <= 19 

    def is_enemy(self,entity_id:int)-> bool:
        if self.enemy_color == "yellow":
            return 0<= entity_id <= 9 
        else:
            return 10<=  entity_id <=19 

    def convert_to_team_index(self, entity_id: int) -> int:
    if entity_id >= 10:
        return entity_id - 10
    return entity_id



    # ======================= STRATEGY =======================

    def run_strategy(self):

        if not self.ball_received:
            return

        for i in range(3):
            if i not in self.ally_map:
                return

        robots = [self.ally_map[i] for i in range(3)]

        best_cost = 1e9
        attacker = self.current_attacker

        for r in robots:
            cost = calculate_time_to_action(r, self.ball_pos)
            if r.id == self.current_attacker:
                cost -= HYSTERESIS_BONUS

            if cost < best_cost:
                best_cost = cost
                attacker = r.id

        self.current_attacker = attacker
        defender = (attacker + 1) % 3
        goalie = (attacker + 2) % 3

        attacker_robot = self.ally_map[attacker]

        enemy_goal = Vec2(GOAL_X, GOAL_Y_ENEMY)
        attack_dir = (enemy_goal - self.ball_pos).normalized()
        dist_to_ball = attacker_robot.pos.dist(self.ball_pos)

        atk = Pose2D()

        # FSM
        if self.attack_state == AttackState.INTERCEPT:

            if dist_to_ball < 10.0:
                self.attack_state = AttackState.ALIGN

            intercept = self.ball_pos - attack_dir * ATTACK_OFFSET
            atk.x = intercept.x
            atk.y = intercept.y
            atk.theta = math.atan2(attack_dir.y, attack_dir.x)

        elif self.attack_state == AttackState.ALIGN:

            if dist_to_ball < 5.0:
                self.attack_state = AttackState.PUSH

            atk.x = attacker_robot.pos.x
            atk.y = attacker_robot.pos.y
            atk.theta = math.atan2(attack_dir.y, attack_dir.x)

        elif self.attack_state == AttackState.PUSH:

            if dist_to_ball > 15.0:
                self.attack_state = AttackState.INTERCEPT

            push = self.ball_pos + attack_dir * 3.0
            atk.x = push.x
            atk.y = push.y
            atk.theta = math.atan2(attack_dir.y, attack_dir.x)

        # Defender
        def_goal = Pose2D()
        def_goal.x = (self.ball_pos.x + GOAL_X) * 0.5
        def_goal.y = (self.ball_pos.y + GOAL_Y_OWN) * 0.5
        def_goal.theta = math.atan2(
            self.ball_pos.y - def_goal.y,
            self.ball_pos.x - def_goal.x
        )

        # Goalie
        gk = Pose2D()
        gk.y = GK_Y
        gk.x = max(GOAL_X - GK_X_RANGE,
                   min(self.ball_pos.x, GOAL_X + GK_X_RANGE))
        gk.theta = math.atan2(
            self.ball_pos.y - gk.y,
            self.ball_pos.x - gk.x
        )

        self.goal_pubs[attacker].publish(atk)
        self.goal_pubs[defender].publish(def_goal)
        self.goal_pubs[goalie].publish(gk)


# ======================= MAIN =======================

def main(args=None):
    rclpy.init(args=args)
    node = StrategyNode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()