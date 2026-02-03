import rclpy
from rclpy.node import Node

from sensor_msgs.msg import JointState
from nav_msgs.msg import Odometry
from geometry_msgs.msg import TransformStamped

from tf2_ros import TransformBroadcaster

import numpy as np
from scipy.spatial.transform import Rotation as R


# =======================
# ROBOT PARAMETERS
# =======================

WHEEL_RADIUS = 0.01     # [m]
L = 0.033               # distance center → wheel [m]
DT = 0.01               # timer period


class OdometryPublisher(Node):

    def __init__(self):
        super().__init__('odometry_publisher')

        # Robot state [x, y, theta]
        self.q = np.zeros((3, 1))

        # Encoder data
        self.joint_states = JointState()
        self.joint_states.position = [0.0, 0.0, 0.0]
        self.prev_pos_motors = np.zeros(3)

        # ROS interfaces
        self.create_subscription(
            JointState,
            'wheels_encoders',
            self.joint_states_callback,
            1
        )

        self.odom_pub = self.create_publisher(Odometry, 'odom', 1)
        self.tf_broadcaster = TransformBroadcaster(self)

        self.create_timer(DT, self.odometry_callback)

        self.get_logger().info("3-wheel odometry node started")

    # =======================
    # CALLBACKS
    # =======================

    def joint_states_callback(self, msg):
        self.joint_states = msg

    # =======================
    # ODOMETRY CORE
    # =======================

    def get_robot_speed(self):

        # Wheel angular velocities
        pos = np.array(self.joint_states.position)
        vel = (pos - self.prev_pos_motors) / DT
        self.prev_pos_motors = pos

        # Wheel angles (rad)
        th1 = 0.0
        th2 = 2.0 * np.pi / 3.0
        th3 = 4.0 * np.pi / 3.0

        # Jacobian (direct kinematics)
        J = (WHEEL_RADIUS / 3.0) * np.array([
            [-np.sin(th1), -np.sin(th2), -np.sin(th3)],
            [ np.cos(th1),  np.cos(th2),  np.cos(th3)],
            [1.0 / L,      1.0 / L,      1.0 / L]
        ])

        q_dot = J @ vel.reshape((3, 1))
        return q_dot

    def odometry_callback(self):

        q_dot = self.get_robot_speed()
        dq = q_dot * DT

        # Rotate to world frame
        theta = self.q[2, 0]
        rot = np.array([
            [np.cos(theta), -np.sin(theta), 0],
            [np.sin(theta),  np.cos(theta), 0],
            [0,              0,             1]
        ])

        self.q += rot @ dq

        # =======================
        # TF
        # =======================

        tf = TransformStamped()
        tf.header.stamp = self.get_clock().now().to_msg()
        tf.header.frame_id = 'odom'
        tf.child_frame_id = 'base_footprint'

        tf.transform.translation.x = self.q[0, 0]
        tf.transform.translation.y = self.q[1, 0]
        tf.transform.translation.z = 0.0

        quat = R.from_euler('z', self.q[2, 0]).as_quat()
        tf.transform.rotation.x = quat[0]
        tf.transform.rotation.y = quat[1]
        tf.transform.rotation.z = quat[2]
        tf.transform.rotation.w = quat[3]

        self.tf_broadcaster.sendTransform(tf)

        # =======================
        # ODOM MESSAGE
        # =======================

        odom = Odometry()
        odom.header.stamp = tf.header.stamp
        odom.header.frame_id = 'odom'
        odom.child_frame_id = 'base_footprint'

        odom.pose.pose.position.x = self.q[0, 0]
        odom.pose.pose.position.y = self.q[1, 0]
        odom.pose.pose.orientation.x = quat[0]
        odom.pose.pose.orientation.y = quat[1]
        odom.pose.pose.orientation.z = quat[2]
        odom.pose.pose.orientation.w = quat[3]

        odom.twist.twist.linear.x = q_dot[0, 0]
        odom.twist.twist.linear.y = q_dot[1, 0]
        odom.twist.twist.angular.z = q_dot[2, 0]

        self.odom_pub.publish(odom)


def main(args=None):
    rclpy.init(args=args)
    node = OdometryPublisher()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
