import rclpy
from geometry_msgs.msg import Twist
from sensor_msgs.msg import JointState
from std_msgs.msg import Header

import numpy as np

# === Parámetros geométricos ===
L = 0.033              # distancia rueda-centro (m)
WHEEL_RADIUS = 0.01    # radio de la rueda (m)

class RobotDriver:

    def init(self, webots_node, properties):

        # === Robot Webots ===
        self.robot = webots_node.robot

        # === Motores ===
        self.motor1 = self.robot.getDevice('motor1')
        self.motor2 = self.robot.getDevice('motor2')
        self.motor3 = self.robot.getDevice('motor3')

        for motor in [self.motor1, self.motor2, self.motor3]:
            motor.setPosition(float('inf'))
            motor.setVelocity(0.0)

        # === Encoders ===
        self.encoder1 = self.robot.getDevice('encoder1')
        self.encoder2 = self.robot.getDevice('encoder2')
        self.encoder3 = self.robot.getDevice('encoder3')

        for enc in [self.encoder1, self.encoder2, self.encoder3]:
            enc.enable(32)

        # === ROS 2 ===
        rclpy.init(args=None)
        self.node = rclpy.create_node('omni_robot_driver')

        self.cmd_vel = Twist()
        self.node.create_subscription(
            Twist,
            'cmd_vel',
            self.cmd_vel_callback,
            10
        )

        self.joint_pub = self.node.create_publisher(
            JointState,
            'joint_states',
            10
        )

        # Ángulos de las ruedas (rad)
        self.wheel_angles = [
            0.0,
            2*np.pi/3,
            4*np.pi/3
        ]

    def cmd_vel_callback(self, msg):
        self.cmd_vel = msg

    def step(self):
        rclpy.spin_once(self.node, timeout_sec=0)

        Vx = self.cmd_vel.linear.x
        Vy = self.cmd_vel.linear.y
        Wz = self.cmd_vel.angular.z

        # === Cinemática inversa ===
        w1 = (-np.sin(self.wheel_angles[0])*Vx + np.cos(self.wheel_angles[0])*Vy + L*Wz) / WHEEL_RADIUS
        w2 = (-np.sin(self.wheel_angles[1])*Vx + np.cos(self.wheel_angles[1])*Vy + L*Wz) / WHEEL_RADIUS
        w3 = (-np.sin(self.wheel_angles[2])*Vx + np.cos(self.wheel_angles[2])*Vy + L*Wz) / WHEEL_RADIUS

        self.motor1.setVelocity(w1)
        self.motor2.setVelocity(w2)
        self.motor3.setVelocity(w3)

        # === Publicar joint states ===
        msg = JointState()
        msg.header = Header()
        msg.header.stamp = self.node.get_clock().now().to_msg()
        msg.name = ['motor1', 'motor2', 'motor3']
        msg.position = [
            self.encoder1.getValue(),
            self.encoder2.getValue(),
            self.encoder3.getValue()
        ]

        self.joint_pub.publish(msg)
