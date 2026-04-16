#!/usr/bin/env python3

# ROS2 node for sending RPMs to robots via UDP, subscribing to cmd_vel for each robot
import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist
from std_msgs.msg import Float32MultiArray, Bool, Float32
import socket
import struct
import math
import time

from communication.constants import OMEGA_TO_RPM, BETA, STATE_PACKET_SIZE, INIT_PACKET_SIZE, DEFAULT_PORT, WATCHDOG_TIME

class RobotUDPClient:
    def __init__(self, robot_ip, robot_port, local_port):
        self.robot_ip = robot_ip
        self.robot_port = robot_port
        self.client_socket = None
        self.local_port = local_port
        self.connect()
        self.communication_init()


    def connect(self):
        print("Setting up UDP socket...")
        try:
            self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

            print(f"UDP socket ready for {self.robot_ip}:{self.robot_port}")

            self.client_socket.bind(('', self.local_port))

        except socket.error as e:
            print(f"Socket error: {e}")
            self.client_socket = None


    def communication_init(self, buffer_size=2048):

        ping_number = 50056
        pong_number = 50057
        self.client_socket.settimeout(0.2) # Un poco más de tiempo para la red
        packed_ping = struct.pack('<i', ping_number)

        print("Handshake", end="")
        while True:
            try:
                self.client_socket.sendto(packed_ping, (self.robot_ip, self.robot_port))
                print(".", end="", flush=True)

                data, addr = self.client_socket.recvfrom(buffer_size)
                if len(data) == INIT_PACKET_SIZE:
                    val = struct.unpack('<i', data)[0]
                    if val == pong_number:
                        break
            except socket.timeout:
                
                time.sleep(0.05) 
                continue
        self.client_socket.setblocking(False)
            
            
    def receive_telemetry(self, buffer_size=2048):
        
        state = None
        
        if not self.client_socket:
            print("UDP socket not available.")
            return None
        
        while True:
            try:
                data, _ = self.client_socket.recvfrom(buffer_size)
                
                if len(data) == STATE_PACKET_SIZE: # Expecting 16 bytes for 4 floats
                    state = struct.unpack('<ffff', data)
                
            except BlockingIOError:
                break  # No more data to read
            except socket.error as e:
                print(f"Socket error during receive: {e}")
        
        return state


    def send_udp_command(self, rpm_left, rpm_right , rpm_back, kicker=False, stop=False):
        if not self.client_socket:
            print("UDP socket not available.")
            return False
        try:
            # Pack both floats into a single UDP packet
            packed_data = struct.pack('<fffbb', rpm_left, rpm_right, rpm_back, kicker, stop)
            self.client_socket.sendto(packed_data, (self.robot_ip, self.robot_port))
            
        except socket.error as e:
            print(f"Socket error during send: {e}")
            return False
        return True


    def close(self):
        if self.client_socket:
            self.client_socket.close()
            self.client_socket = None

class SingleRobotUDPNode(Node):
    def __init__(self):
        super().__init__('single_robot_udp_node')
        # Parameters should be loaded from external YAML config via ROS2 launch or CLI
        self.get_logger().info(f"Waiting to Start")
        self.declare_parameter('robot_name', 'vsss_robot')
        self.declare_parameter('robot_ip', '0.0.0.0')
        self.declare_parameter('robot_port', DEFAULT_PORT)
        self.declare_parameter('local_port', DEFAULT_PORT)
        self.latest_setpoints = [0.0, 0.0, 0.0]
        self.robot_yaw = 0.0
        self.last_twist_received = Twist()
        self.last_twist = Twist()
        self.last_twist.linear.x = 0.0
        self.last_twist.linear.y = 0.0
        self.last_twist.angular.z = 0.0
        self.dt = 0.02  # Control loop time step (50 Hz) 

        name = self.get_parameter('robot_name').value
        ip = self.get_parameter('robot_ip').value
        robot_port = self.get_parameter('robot_port').value
        local_port = self.get_parameter('local_port').value

        print("Parameters {}, {}, {}, {}".format(name, ip, robot_port, local_port))

        node_name = self.get_fully_qualified_name()
        cmd_vel_topic = self.resolve_topic_name('cmd_vel')
        rpms_topic = self.resolve_topic_name('encoders')
        
        self.get_logger().info(f"{node_name} Started with values name: {name}, connection->({ip}:{robot_port})")
        self.client = RobotUDPClient(ip, robot_port, local_port)
        self.get_logger().info(f"Communication established")

        self.get_logger().info(f"Waiting to Start")
        self.create_subscription(Twist, 'cmd_vel', self.cmd_vel_callback, 10)
        self.get_logger().info(f"Subscribed to {cmd_vel_topic} for {name} ({ip}:{robot_port})")
        
        self.rpms_pub = self.create_publisher(Float32MultiArray, 'encoders', 10)
        self.get_logger().info(f"Publishing RPMs to {rpms_topic} for {name} ({ip}:{robot_port})")

        self.create_subscription(Bool, 'stop', self.stop_callback, 10)
        self.get_logger().info(f"Subscribed to stop commands on {self.resolve_topic_name('stop')} for {name} ({ip}:{robot_port})")

        self.create_subscription(Bool, 'kicker', self.kicker_callback, 10)
        self.get_logger().info(f"Subscribed to kicker commands on {self.resolve_topic_name('kicker')} for {name} ({ip}:{robot_port})")

        self.create_timer(0.02, self.receive_telemetry_timer_callback)  # 50 Hz for telemetry

        self.create_timer(self.dt, self.control_loop_timer_callback)  # 50 Hz for control loop
        
        self.last_packet_time = self.get_clock().now().nanoseconds


    def receive_telemetry_timer_callback(self):
        # Attempt to receive telemetry data
        state = self.client.receive_telemetry()
        
        if state:
            rpm_msg = Float32MultiArray()
            
            rpm_msg.data = [state[1], state[2], state[3], self.latest_setpoints[0], self.latest_setpoints[1], self.latest_setpoints[2]]
            
            self.rpms_pub.publish(rpm_msg)

            self.last_packet_time = self.get_clock().now().nanoseconds
        else:
            if self.get_clock().now().nanoseconds - self.last_packet_time >= WATCHDOG_TIME:
                self.client.communication_init()
                self.last_packet_time = self.get_clock().now().nanoseconds 

    
    def stop_callback(self, msg):
        if msg.data:
            self.get_logger().info("Stop command received, sending zero RPMs")
            self.latest_setpoints = [0.0, 0.0, 0.0]
            self.last_twist_received.linear.x = 0.0
            self.last_twist_received.linear.y = 0.0
            self.last_twist_received.angular.z = 0.0
            self.client.send_udp_command(self.latest_setpoints[0], self.latest_setpoints[1], self.latest_setpoints[2], False, True)

        
    def kicker_callback(self, msg):
        if msg.data:
            self.get_logger().info("Kicker command received, sending kicker signal")
            self.client.send_udp_command(self.latest_setpoints[0], self.latest_setpoints[1], self.latest_setpoints[2], True)
        else: 
            self.get_logger().info("Kicker release command received, sending normal setpoints")
            self.client.send_udp_command(self.latest_setpoints[0], self.latest_setpoints[1], self.latest_setpoints[2], False)
            
        
    def destroy_node(self):
        # Ensure the UDP client is closed when the node is destroyed
        if self.client:
            self.client.close()
        super().destroy_node()


    def cmd_vel_callback(self, msg):
        self.get_logger().info(f"Received cmd_vel: linear=({msg.linear.x:.2f}, {msg.linear.y:.2f}), angular=({msg.angular.z:.2f})")

        self.last_twist_received = msg

        
    def control_loop_timer_callback(self):
       twist = self.limit_accel(self.last_twist_received) 
       
       rpm_left, rpm_right, rpm_back = self.twist_to_rpm(twist)
       self.get_logger().info(f"Rpm sent: L={rpm_left}, R={rpm_right}, B={rpm_back}")
       self.latest_setpoints = [rpm_left, rpm_right, rpm_back]
       sent = self.client.send_udp_command(rpm_left, rpm_right, rpm_back)


    def limit_accel(self, twist, max_linear=0.8, max_angular=2.5, ):
        # Limit the acceleration of the robot to prevent abrupt changes
        max_linear_change = max_linear * self.dt
        max_angular_change = max_angular * self.dt

        linear_x_diff = twist.linear.x - self.last_twist.linear.x
        linear_y_diff = twist.linear.y - self.last_twist.linear.y
        angular_z_diff = twist.angular.z - self.last_twist.angular.z

        limited_linear_x = self.last_twist.linear.x + max(-max_linear_change, min(max_linear_change, linear_x_diff))
        limited_linear_y = self.last_twist.linear.y + max(-max_linear_change, min(max_linear_change, linear_y_diff))
        limited_angular_z = self.last_twist.angular.z + max(-max_angular_change, min(max_angular_change, angular_z_diff))
        
        self.last_twist.linear.x = limited_linear_x
        self.last_twist.linear.y = limited_linear_y
        self.last_twist.angular.z = limited_angular_z
        
        return self.last_twist


    def twist_to_rpm(self, twist, wheel_radius=0.01431, wheel_distance=0.0311):
        # Convert Twist message to RPM for three-wheeled robot
        vx = twist.linear.x  # Forward velocity
        vy = twist.linear.y  # Sideways velocity 
        wz = twist.angular.z  # Angular velocity

        # Transform the twist to the frame of the robot
        vx_t = vx * math.cos(self.robot_yaw) + vy * math.sin(self.robot_yaw)
        vy_t = vx * -math.sin(self.robot_yaw) + vy * math.cos(self.robot_yaw)
        
        rpm_left = (-wheel_distance * wz  + math.cos(BETA) * vx_t + math.sin(BETA) * vy_t) * OMEGA_TO_RPM / wheel_radius
        rpm_right = (-wheel_distance * wz + math.cos(BETA) * vx_t - math.sin(BETA) * vy_t) * OMEGA_TO_RPM / wheel_radius
        rpm_back = (-wheel_distance * wz - vx_t) * OMEGA_TO_RPM / wheel_radius
        
        return rpm_left, rpm_right, rpm_back


def main(args=None):
    rclpy.init(args=args)
    node = SingleRobotUDPNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
