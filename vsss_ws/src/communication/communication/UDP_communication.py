#!/usr/bin/env python3

# ROS2 node for sending RPMs to robots via UDP, subscribing to cmd_vel for each robot
import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist
from std_msgs.msg import Float32MultiArray, Bool, Float32
import socket
import struct
import math

BETA = math.radians(30)  # degrees
GAMA = 0 # degrees
OMEGA_TO_RPM = 60 / (2 * math.pi)  # Conversion factor from rad/s to RPM
COS_30 = math.cos(BETA) 


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
                
                if len(data) == 4:  # Expecting at least 4 bytes for 1 float
                    latest_orientation = struct.unpack('<f', data)
                elif len(data) == 12:  # Expecting at least 12 bytes for 3 floats
                    latest_rpms = struct.unpack('<fff', data)
                
            except BlockingIOError:
                break  # No more data to read
            except socket.error as e:
                print(f"Socket error during receive: {e}")
        
        return latest_orientation, latest_rpms

    def send_floats(self, float1, float2, float3, float4=0.0):
        if not self.client_socket:
            print("UDP socket not available.")
            return False
        try:
            # Pack both floats into a single UDP packet
            packed_data = struct.pack('<ffff', float1, float2, float3, float4)
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
        self.declare_parameter('communication_port', 8081)    
        self.latest_setpoints = [0, 0, 0]
        self.robot_yaw = 0.0

        name = self.get_parameter('robot_name').value
        ip = self.get_parameter('robot_ip').value
        port = self.get_parameter('communication_port').value
        
        node_name = self.get_fully_qualified_name()
        cmd_vel_topic = self.resolve_topic_name('cmd_vel')
        rpms_topic = self.resolve_topic_name('rpms')
        telemetry_topic = self.resolve_topic_name('telemetry')
        
        self.get_logger().info(f"{node_name} Started with values name: {name}, connection->({ip}:{port})")
        self.client = RobotUDPClient(ip, port)

        self.get_logger().info(f"Waiting to Start")
        self.create_subscription(Twist, 'cmd_vel', self.cmd_vel_callback, 10)
        self.get_logger().info(f"Subscribed to {cmd_vel_topic} for {name} ({ip}:{port})")
        
        self.telemetry_pub = self.create_publisher(Float32, 'telemetry', 10)
        self.get_logger().info(f"Publishing telemetry to {telemetry_topic} for {name} ({ip}:{port})")
        
        self.rpms_pub = self.create_publisher(Float32MultiArray, 'rpms', 10)
        self.get_logger().info(f"Publishing RPMs to {rpms_topic} for {name} ({ip}:{port})")
        
        self.create_subscription(Bool, 'stop', self.stop_callback, 10)
        self.get_logger().info(f"Subscribed to stop commands on {self.resolve_topic_name('stop')} for {name} ({ip}:{port})")
        
        self.create_subscription(Bool, 'kicker', self.kicker_callback, 10)
        self.get_logger().info(f"Subscribed to kicker commands on {self.resolve_topic_name('kicker')} for {name} ({ip}:{port})")
        
        self.create_timer(0.02, self.receive_telemetry_timer_callback)  # 50 Hz for telemetry
        

    def receive_telemetry_timer_callback(self):
        # Attempt to receive telemetry data
        orientation_data, rpms_data = self.client.receive_telemetry()
        
        if orientation_data:
            # Publish received telemetry
            
            orientation_msg = Float32()
            self.robot_yaw = math.radians(orientation_data[0])  # Update global yaw variable
            orientation_msg.data = self.robot_yaw
            self.telemetry_pub.publish(orientation_msg)
            self.get_logger().info(f"Received telemetry: {orientation_msg.data:.2f}")
        
        if rpms_data:
            rpms_msg = Float32MultiArray()
            rpms_msg.data = [rpms_data[0], rpms_data[1], rpms_data[2], self.latest_setpoints[0], self.latest_setpoints[1], self.latest_setpoints[2]]
            self.rpms_pub.publish(rpms_msg)
            self.get_logger().info(f"Received RPMs: F={rpms_msg.data[0]:.2f}, R={rpms_msg.data[1]:.2f}, L={rpms_msg.data[2]:.2f}")
    
    def stop_callback(self, msg):
        if msg.data:
            self.get_logger().info("Stop command received, sending zero RPMs")
            self.client.send_floats(0.0, 0.0, 0.0)
        
    def kicker_callback(self, msg):
        if msg.data:
            self.get_logger().info("Kicker command received, sending kicker signal")
            self.client.send_floats(self.latest_setpoints[0], self.latest_setpoints[1], self.latest_setpoints[2], 1.0)
        else: 
            self.get_logger().info("Kicker release command received, sending normal setpoints")
            self.client.send_floats(self.latest_setpoints[0], self.latest_setpoints[1], self.latest_setpoints[2], 0.0)
            
        
    def destroy_node(self):
        # Ensure the UDP client is closed when the node is destroyed
        if self.client:
            self.client.close()
        super().destroy_node()

    def cmd_vel_callback(self, msg):
        rpm_left, rpm_right, rpm_back = self.twist_to_rpm(msg)
        self.latest_setpoints = [rpm_left, rpm_right, rpm_back]
        sent = self.client.send_floats(rpm_left, rpm_right, rpm_back)

        self.get_logger().info(f"Sent: F={rpm_left:.2f} R={rpm_right:.2f} L={rpm_back:.2f}") 
        
    def twist_to_rpm(self, twist, wheel_radius=0.01431, wheel_distance=0.0311):
        # Convert Twist message to RPM for three-wheeled robot
        vx = twist.linear.x  # Forward velocity
        vy = twist.linear.y  # Sideways velocity 
        wz = twist.angular.z  # Angular velocity

        # Transform the twist to the frame of the robot
        vx_t = vx * math.cos(self.robot_yaw) + vy * math.sin(self.robot_yaw)
        vy_t = vx * -math.sin(self.robot_yaw) + vy * math.cos(self.robot_yaw)
        
        rpm_left = (-wheel_distance * wz  + COS_30 * vx_t + 0.5 * vy_t) * OMEGA_TO_RPM / wheel_radius
        rpm_right = (-wheel_distance * wz - COS_30 * vx_t + 0.5 * vy_t) * OMEGA_TO_RPM / wheel_radius
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