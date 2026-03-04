#!/usr/bin/env python3

# ROS2 node for sending RPMs to robots via UDP, subscribing to cmd_vel for each robot
import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist, Quaternion
from std_msgs.msg import Float32MultiArray
import socket
import struct
import math

beta = 30  # degrees
gama = 0 # degrees
OMEGA_TO_RPM = 60 / (2 * math.pi)  # Conversion factor from rad/s to RPM
COS_30 = math.cos(math.radians(beta))


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
            
            self.client_socket.bind(('', 8081))
        except socket.error as e:
            print(f"Socket error: {e}")
            self.client_socket = None
            
    def receive_telemetry(self, buffer_size=2048):
        
        latest_orientation = None
        latest_rpms = None
        
        if not self.client_socket:
            print("UDP socket not available.")
            return None, None
        
        while True:
            try:
                data, _ = self.client_socket.recvfrom(buffer_size)
                
                if len(data) == 16:  # Expecting at least 16 bytes for 4 floats
                    latest_orientation = struct.unpack('<ffff', data)
                elif len(data) == 12:  # Expecting at least 12 bytes for 3 floats
                    latest_rpms = struct.unpack('<fff', data)
                
            except BlockingIOError:
                break  # No more data to read
            except socket.error as e:
                print(f"Socket error during receive: {e}")
        
        return latest_orientation, latest_rpms

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

def twist_to_rpm(twist, wheel_radius=0.01431, wheel_distance=0.0311):
    # Convert Twist message to RPM for three-wheeled robot
    vx = twist.linear.x  # Forward velocity
    vy = twist.linear.y  # Sideways velocity 
    wz = twist.angular.z  # Angular velocity
    
    rpm_left = (-wheel_distance * wz  COS_30 * vx + 0.5 * vy) * OMEGA_TO_RPM / wheel_radius
    rpm_right = (-wheel_distance * wz - 0.5 * vx + 0.5 * vy) * OMEGA_TO_RPM / wheel_radius
    rpm_back = (-wheel_distance * wz - vx) * OMEGA_TO_RPM / wheel_radius
    
    return rpm_left, rpm_right, rpm_back

class SingleRobotUDPNode(Node):
    def __init__(self):
        super().__init__('single_robot_udp_node')
        # Parameters should be loaded from external YAML config via ROS2 launch or CLI
        
        self.get_logger().info(f"Waiting to Start")
        self.declare_parameter('robot_name', 'robot1')
        self.declare_parameter('robot_ip', '192.168.0.220')
        self.declare_parameter('robot_port', 8081)    
        self.latest_setpoints = [0, 0, 0]

        name = self.get_parameter('robot_name').value
        ip = self.get_parameter('robot_ip').value
        port = self.get_parameter('robot_port').value

        
        self.get_logger().info(f"Node Started with values name {name}, connection->({ip}:{port})")
        self.client = RobotUDPClient(ip, port)

        self.get_logger().info(f"Waiting to Start")
        topic = 'cmd_vel'
        self.create_subscription(Twist, topic, self.cmd_vel_callback, 10)
        self.get_logger().info(f"Subscribed to {topic} for {name} ({ip}:{port})")
        
        self.telemetry_pub = self.create_publisher(Quaternion, 'telemetry', 10)
        self.get_logger().info(f"Publishing telemetry to /{name}/telemetry")
        
        self.rpms_pub = self.create_publisher(Float32MultiArray, 'rpms', 10)
        self.get_logger().info(f"Publishing RPMs to /{name}/rpms")
        
        self.create_timer(0.02, self.receive_telemetry_timer_callback)  # 50 Hz for telemetry
        
        

    def receive_telemetry_timer_callback(self):
        # Attempt to receive telemetry data
        orientation_data, rpms_data = self.client.receive_telemetry()
        
        if orientation_data:
            # Publish received telemetry
            
            orientation_msg = Quaternion()
            orientation_msg.x = orientation_data[0]
            orientation_msg.y = orientation_data[1]
            orientation_msg.z = orientation_data[2]
            orientation_msg.w = orientation_data[3]
            self.telemetry_pub.publish(orientation_msg)
            self.get_logger().info(f"Received telemetry: {orientation_msg.x:.2f}, {orientation_msg.y:.2f}, {orientation_msg.z:.2f}, {orientation_msg.w:.2f}")
        
        if rpms_data:
            rpms_msg = Float32MultiArray()
            rpms_msg.data = [rpms_data[0], rpms_data[1], rpms_data[2], self.latest_setpoints[0], self.latest_setpoints[1], self.latest_setpoints[2]]
            self.rpms_pub.publish(rpms_msg)
            self.get_logger().info(f"Received RPMs: F={rpms_msg.data[0]:.2f}, R={rpms_msg.data[1]:.2f}, L={rpms_msg.data[2]:.2f}")
            
        
    def destroy_node(self):
        # Ensure the UDP client is closed when the node is destroyed
        if self.client:
            self.client.close()
        super().destroy_node()

    def cmd_vel_callback(self, msg):
        rpm_left, rpm_right, rpm_back = twist_to_rpm(msg)
        self.latest_setpoints = [rpm_left, rpm_right, rpm_back]
        sent = self.client.send_floats(rpm_left, rpm_right, rpm_back)

        self.get_logger().info(f"Sent: F={rpm_left:.2f} R={rpm_right:.2f} L={rpm_back:.2f}") 

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