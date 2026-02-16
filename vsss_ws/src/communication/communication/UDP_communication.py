#!/usr/bin/env python3

# ROS2 node for sending RPMs to robots via UDP, subscribing to cmd_vel for each robot
import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist, Quaternion
import socket
import struct
import math

beta = 120 # degrees
gama = 0 # degrees
OMEGA_TO_RPM = 60 / (2 * math.pi)  # Conversion factor from rad/s to RPM
SIN_120 = math.sin(math.radians(beta))


class RobotUDPClient:
    def __init__(self, robot_ip, robot_port):
        self.robot_ip = robot_ip
        self.robot_port = robot_port
        self.client_socket = None
        self.connect()

    def connect(self):
        print("Setting up UDP socket...")
        try:
            self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.client_socket.setblocking(False)
            print(f"UDP socket ready for {self.robot_ip}:{self.robot_port}")
        except socket.error as e:
            print(f"Socket error: {e}")
            self.client_socket = None
            
    def receive_floats(self, buffer_size=1024):
        if not self.client_socket:
            print("UDP socket not available.")
            return None
        try:
            data, _ = self.client_socket.recvfrom(buffer_size)
            
            if len(data) < 16:
                print("Received data is too short.")
                return None
            unpacked_data = struct.unpack('<ffff', data)
            
            msg = Quaternion()
            msg.x = unpacked_data[0]
            msg.y = unpacked_data[1]
            msg.z = unpacked_data[2]
            msg.w = unpacked_data[3]
            return msg
        except BlockingIOError:
            pass
        except socket.error as e:
            print(f"Socket error during receive: {e}")
        

    def send_floats(self, float1, float2, float3):
        if not self.client_socket:
            print("UDP socket not available.")
            return False
        try:
            # Pack both floats into a single UDP packet
            packed_data = struct.pack('<fff', float1, float2, float3)
            self.client_socket.sendto(packed_data, (self.robot_ip, self.robot_port))
        except socket.error as e:
            print(f"Socket error during send: {e}")
            return False
        return True

    def close(self):
        if self.client_socket:
            self.client_socket.close()
            self.client_socket = None

def twist_to_rpm(twist, wheel_radius=0.03, wheel_distance=0.020):
    # Convert Twist message to RPM for three-wheeled robot
    vx = twist.linear.x  # Forward velocity
    vy = twist.linear.y  # Sideways velocity 
    wz = twist.angular.z  # Angular velocity
    
    rpm_front = (-wheel_distance * wz + vx) * OMEGA_TO_RPM / wheel_radius
    rpm_right = (-wheel_distance * wz - 0.5 * vx - SIN_120 * vy) * OMEGA_TO_RPM / wheel_radius
    rpm_left = (-wheel_distance * wz - 0.5 * vx + SIN_120 * vy) * OMEGA_TO_RPM / wheel_radius
    
    return rpm_front, rpm_right, rpm_left

class SingleRobotUDPNode(Node):
    def __init__(self):
        super().__init__('single_robot_udp_node')
        # Parameters should be loaded from external YAML config via ROS2 launch or CLI
        
        self.get_logger().info(f"Waiting to Start")
        self.declare_parameter('robot_name', 'robot1')
        self.declare_parameter('robot_ip', '192.168.0.188')
        self.declare_parameter('robot_port', 8080)    

        name = self.get_parameter('robot_name').value
        ip = self.get_parameter('robot_ip').value
        port = self.get_parameter('robot_port').value

        
        self.get_logger().info(f"Node Started with values name {name}, connection->({ip}:{port})")
        self.client = RobotUDPClient(ip, port)

        self.get_logger().info(f"Waiting to Start")
        topic = f'/{name}/cmd_vel'
        self.create_subscription(Twist, topic, self.cmd_vel_callback, 10)
        self.get_logger().info(f"Subscribed to {topic} for {name} ({ip}:{port})")
        
        self.telemetry_pub = self.create_publisher(Quaternion, f'/{name}/telemetry', 10)
        self.get_logger().info(f"Publishing telemetry to /{name}/telemetry")
        
        self.create_timer(0.02, self.receive_timer_callback)  # 50 Hz receive timer

    def receive_timer_callback(self):
        # Attempt to receive telemetry data
        data = self.client.receive_floats()
        
        if data:
            # Publish received telemetry
            self.telemetry_pub.publish(data)
            self.get_logger().info(f"Received telemetry: {data.data}")
        
    def destroy_node(self):
        # Ensure the UDP client is closed when the node is destroyed
        if self.client:
            self.client.close()
        super().destroy_node()

    def cmd_vel_callback(self, msg):
        rpm_front, rpm_right, rpm_left = twist_to_rpm(msg)
        sent = self.client.send_floats(rpm_front, rpm_right, rpm_left)

        self.get_logger().info(f"Sent: F={rpm_front:.2f} R={rpm_right:.2f} L={rpm_left:.2f}") 

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