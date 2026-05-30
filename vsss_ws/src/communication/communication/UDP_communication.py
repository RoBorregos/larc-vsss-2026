#!/usr/bin/env python3

# ROS2 node for sending RPMs to robots via UDP, subscribing to cmd_vel for each robot
import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist, Vector3
from std_msgs.msg import Float32MultiArray, Bool, Float32
from tf2_ros import TransformException, Buffer, TransformListener
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

        self.client_socket.setblocking(False)


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
        self.declare_parameter('robot_frame', '')
        self.declare_parameter('global_frame', 'field')
        
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, self)
        
        self.latest_setpoints = [0.0, 0.0, 0.0]
        self.robot_yaw = 0 # Default to facing "up" in the field

        self.target_speed = 0.0
        self.target_angle = 0.0
        self.target_wz = 0.0  # Por si decides usar el Z del vector después

        self.last_vx = 0.0
        self.last_vy = 0.0
        self.last_wz = 0.0
        self.dt = 0.02  # Control loop time step (50 Hz)

        self.local_frame = self.get_parameter('robot_frame').value
        self.global_frame = self.get_parameter('global_frame').value

        name = self.get_parameter('robot_name').value
        ip = self.get_parameter('robot_ip').value
        robot_port = self.get_parameter('robot_port').value
        local_port = self.get_parameter('local_port').value
        

        print("Parameters {}, {}, {}, {}".format(name, ip, robot_port, local_port))

        node_name = self.get_fully_qualified_name()
        cmd_vel_topic = self.resolve_topic_name('motion_control')
        rpms_topic = self.resolve_topic_name('encoders')
        
        self.get_logger().info(f"{node_name} Started with values name: {name}, connection->({ip}:{robot_port})")

        self.get_logger().info(f"Initializing UDP client for {name} at {ip}:{robot_port} with local port {local_port}")
        self.client = RobotUDPClient(ip, robot_port, local_port)
        self.get_logger().info(f"Communication established")

        self.get_logger().info(f"Waiting to Start")
        self.create_subscription(Vector3, 'motion_control', self.cmd_vel_callback, 10)
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

    
    def yaw_from_quaternion(self, x, y, z, w):
        t3 = +2.0 * (w * z + x * y)
        t4 = +1.0 - 2.0 * (y * y + z * z)
        yaw_z = math.atan2(t3, t4)
        return yaw_z  # Adjust for robot's forward direction being along the y-axis

    
    def _update_robot_yaw(self):
        try:
            transform = self.tf_buffer.lookup_transform(self.local_frame, self.global_frame, rclpy.time.Time(), timeout=rclpy.duration.Duration(seconds=0.01))
            q = transform.transform.rotation
            yaw = self.yaw_from_quaternion(q.x, q.y, q.z, q.w)

            # self.get_logger().info(f"yaw received: {yaw}")
            self.robot_yaw = yaw
        except TransformException as e:
            pass

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
                self.get_logger().warn("No telemetry received for a while, reinitializing communication...")
                self.client.communication_init()
                self.last_packet_time = self.get_clock().now().nanoseconds

    def stop_callback(self, msg):
        if msg.data:
            self.target_speed = 0.0
            self.last_vx = 0.0
            self.last_vy = 0.0
            self.last_wz = 0.0
            self.client.send_udp_command(0.0, 0.0, 0.0, False, True)
        
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
        self.target_speed = msg.x
        self.target_angle = msg.y
        self.target_wz = msg.z  # Aunque dijiste que es 0, lo guardamos por si acaso

        self.get_logger().info(f"Received Move: Speed={msg.x:.2f}, Angle={msg.y:.2f}")

    def control_loop_timer_callback(self):
        # 1. Actualizamos orientación
        self._update_robot_yaw()

        # 2. Calculamos el error hacia 0 rad
        raw_error = -self.robot_yaw

        # --- NORMALIZACIÓN DEL ÁNGULO (Camino más corto) ---
        # Esto obliga al error a estar siempre entre -pi y pi
        angle_error = (raw_error + math.pi) % (2 * math.pi) - math.pi
        angle_error = -angle_error
        # --------------------------------------------------

        ANGLE_THRESHOLD = math.radians(25)

        # 4. Ganancia de rotación
        kp_rotation = 1.0
        wz_correction = angle_error * kp_rotation

        # 5. Lógica de decisión
        if abs(angle_error) > ANGLE_THRESHOLD:
            vx_target = 0.0
            vy_target = 0.0
        else:
            # Movimiento combinado
            speed_factor = math.cos(angle_error)
            current_target_speed = self.target_speed * max(0.5, speed_factor)

            vy_target = current_target_speed * math.cos(self.target_angle)
            vx_target = -(current_target_speed * math.sin(self.target_angle))

        # 6. Aplicamos límites y enviamos
        vx, vy, wz = self.limit_accel(vx_target, vy_target, wz_correction)
        rpm_left, rpm_right, rpm_back = self.calculate_omni_rpms(vx, vy, wz)

        self.latest_setpoints = [rpm_left, rpm_right, rpm_back]
        self.client.send_udp_command(rpm_left, rpm_right, rpm_back)

    def limit_accel(self, vx_target, vy_target, wz_target, max_linear=1.8, max_angular=4.5):
        max_l_change = max_linear * self.dt
        max_a_change = max_angular * self.dt

        self.last_vx += max(-max_l_change, min(max_l_change, vx_target - self.last_vx))
        self.last_vy += max(-max_l_change, min(max_l_change, vy_target - self.last_vy))
        self.last_wz += max(-max_a_change, min(max_a_change, wz_target - self.last_wz))

        return self.last_vx, self.last_vy, self.last_wz

    def calculate_omni_rpms(self, vx, vy, wz, wheel_radius=0.01431, wheel_distance=0.0311):
        rpm_left = (-wheel_distance * wz + math.cos(BETA) * vx + math.sin(BETA) * vy) * OMEGA_TO_RPM / wheel_radius
        rpm_right = (-wheel_distance * wz + math.cos(BETA) * vx - math.sin(BETA) * vy) * OMEGA_TO_RPM / wheel_radius
        rpm_back = (-wheel_distance * wz - vx) * OMEGA_TO_RPM / wheel_radius

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