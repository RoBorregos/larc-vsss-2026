import math
import rclpy
import random
from rclpy.node import Node
from geometry_msgs.msg import Twist
from std_msgs.msg import Bool, Float32
from .entity import Entity
from .. import constants
import numpy as np
from PIL import Image, ImageDraw, ImageTk
import cv2

ARUCO_DICT = cv2.aruco.getPredefinedDictionary(cv2.aruco.DICT_ARUCO_ORIGINAL)

class Robot(Entity):
    def __init__(self, ros_handler, robot_id, start_x, start_y, start_theta, simulation=False):
        super().__init__(ros_handler, robot_id)
        self.simulation = simulation
        self.weight = 200
        self.screen_radius = constants.ROBOT_RADIUS * constants.DISPLAY_SCALE
        self.real_radius = constants.ROBOT_RADIUS / 100

        self.real_x = start_x
        self.real_y = start_y
        self.real_theta = start_theta

        self.target_speed = 0.0
        self.target_angle_rad = 0.0
        self.is_moving = False

        self.kick_request = False

        self.aruco_id = constants.ROBOT_DATABASE.get(robot_id)
        self.main_color = "#0000FF" if robot_id >= 10 else "#FFFF00"
        self._aruco_cache = None

        self.role = None
        self.sub_cmd_vel = None
        self.sub_kicker = None
        self.sub_stop = None

        self.subscribe_to_topics(simulation)

    def _hex_to_rgb(self, hex_color):
        hex_color = hex_color.lstrip('#')
        return tuple(int(hex_color[i:i + 2], 16) for i in (0, 2, 4))

    def subscribe_to_topics(self, simulation):
        if simulation:
            self.sub_cmd_vel = self.ros_handler.create_subscription(
                Twist,
                f'/strategy/robot_{self.id}/cmd_vel',
                self._simulation_move,
                10
            )
            self.sub_kicker = self.ros_handler.create_subscription(
                Bool,
                f'/strategy/robot_{self.id}/kicker',
                self._simulation_kick,
                10
            )
            self.sub_stop = self.ros_handler.create_subscription(
                Bool,
                f'/strategy/robot_{self.id}/stop',
                self._simulation_stop,
                10
            )
        else:
            self.sub_ball = self.ros_handler.create_subscription(
                Float32,
                f'/communication/robot_{self.id}/bno',
                self._communication_bno,
                10
            )


    def destroy(self, canvas):
        if self.sub_cmd_vel:
            self.ros_handler.destroy_subscription(self.sub_cmd_vel)
        if self.sub_stop:
            self.ros_handler.destroy_subscription(self.sub_stop)
        if self.sub_kicker:
            self.ros_handler.destroy_subscription(self.sub_kicker)

    def _simulation_move(self, msg):
        self.target_vx = msg.linear.x
        self.target_vy = msg.linear.y

        self.target_speed = math.sqrt(self.target_vx ** 2 + self.target_vy ** 2)
        self.target_angle_rad = math.atan2(self.target_vy, self.target_vx)
        self.is_moving = self.target_speed > (constants.MIN_SPEED * 2)

        new_vx = msg.linear.x + random.gauss(0, abs(msg.linear.x) * constants.MOVEMENT_NOISE)
        new_vy = msg.linear.y + random.gauss(0, abs(msg.linear.y) * constants.MOVEMENT_NOISE)
        new_angular_vel = msg.angular.z + random.gauss(0, constants.MOVEMENT_NOISE)

        if math.fabs(new_vx) <= constants.MIN_SPEED: new_vx = 0
        if math.fabs(new_vy) <= constants.MIN_SPEED: new_vy = 0
        if math.fabs(new_angular_vel) <= constants.MIN_SPEED: new_angular_vel = 0

        self.real_vx = new_vx
        self.real_vy = new_vy
        self.real_angular_vel = msg.angular.z

    def _communication_bno(self, msg):
        if msg.data:
            self.real_theta = msg.data

    def _simulation_kick(self, msg):
        if msg.data:
            self.kick_request = True

    def _simulation_stop(self, msg):
        if msg.data:
            print("Stop robot with id {}".format(self.id))
            self.real_vx = 0
            self.real_vy = 0
            self.is_moving = False
            self.target_speed = 0.0

    def stop(self):
        self.is_moving = False
        self.target_speed = 0.0
        self.ros_handler.publish_stop(self.id, True)

    def move(self, speed, angle, facing_target_deg):
        self.target_speed = speed
        self.target_angle_rad = angle
        self.is_moving = speed > (constants.MIN_SPEED * 2)

        self.ros_handler.publish_stop(self.id, False)

        target_rad = math.radians(facing_target_deg)
        diff = self.theta - target_rad
        facing_error = (diff + math.pi) % (2 * math.pi) - math.pi

        self.ros_handler.publish_omni_move(self.id, speed, angle, facing_error)

    def kicker(self, active):
        self.ros_handler.publish_kicker(self.id, active)

    def _get_aruco_image(self, size):
        if self._aruco_cache is not None:
            return self._aruco_cache

        marker_px = 200
        img_aruco = cv2.aruco.generateImageMarker(ARUCO_DICT, self.aruco_id, marker_px)

        aruco_pil = Image.fromarray(img_aruco).convert("RGBA")
        data = np.array(aruco_pil)

        rgb_team = self._hex_to_rgb(self.main_color)

        black_areas = (data[:, :, 0:3] == [0, 0, 0]).all(axis=2)
        white_areas = (data[:, :, 0:3] == [255, 255, 255]).all(axis=2)

        if self.main_color == "#0000FF":  # BLUE
            data[black_areas, 0:3] = rgb_team
        else:
            data[white_areas, 0:3] = rgb_team

        self._aruco_cache = Image.fromarray(data)
        return self._aruco_cache

    def draw(self, canvas, draw_context, simulation):
        screen_x, screen_y = self.pixel_to_world(self.real_x, self.real_y)
        size = constants.ROBOT_RADIUS * 2 * constants.DISPLAY_SCALE

        half_size = size / 2
        rect_points = [
            (-half_size, -half_size), (half_size, -half_size),
            (half_size, half_size), (-half_size, half_size)
        ]

        rotated_tk = []
        rotated_pil = []
        for px, py in rect_points:
            rx = px * math.cos(self.real_theta) - py * math.sin(self.real_theta) + screen_x
            ry = px * math.sin(self.real_theta) + py * math.cos(self.real_theta) + screen_y
            rotated_tk.extend([rx, ry])
            rotated_pil.append((rx, ry))

        canvas.create_polygon(rotated_tk, fill=self.main_color, outline="black")

        canvas.create_polygon(rotated_tk, fill="white", outline="black")
        if simulation and draw_context:
            draw_context.polygon(rotated_pil, fill="white", outline="black")

        self._draw_aruco_on_contexts(canvas, draw_context, screen_x, screen_y, size)

        if self.is_moving:
            self._draw_movement_arrow(canvas, screen_x, screen_y)

        if self.role:
            text_y = screen_y - (half_size + constants.TEXT_OFFSET)
            canvas.create_text(screen_x, text_y, text=self.role, fill="white",
                               font=("Arial", 10, "bold"), anchor="s")

    def _draw_aruco_on_contexts(self, canvas, draw_context, cx, cy, size):
        aruco_size = int(size * 0.875)
        aruco_base = self._get_aruco_image(aruco_size)

        angle_deg = -math.degrees(self.real_theta)
        aruco_rotated = aruco_base.resize((aruco_size, aruco_size), Image.NEAREST).rotate(angle_deg, expand=True,
                                                                                          resample=Image.BICUBIC)

        off_x = int(cx - aruco_rotated.width / 2)
        off_y = int(cy - aruco_rotated.height / 2)

        if hasattr(draw_context, 'im'):
            pass

        self.tk_aruco_img = ImageTk.PhotoImage(aruco_rotated)
        canvas.create_image(cx, cy, image=self.tk_aruco_img)

    def _draw_movement_arrow(self, canvas, cx, cy):
        arrow_len = 50

        end_x = cx + arrow_len * math.cos(self.target_angle_rad)
        end_y = cy - arrow_len * math.sin(self.target_angle_rad)

        arrow_color = "#00FF00"
        arrow_width = 3

        head_size = arrow_width * 3
        head_angle = math.pi / 6

        b1_angle = self.target_angle_rad + math.pi - head_angle
        b2_angle = self.target_angle_rad + math.pi + head_angle

        # Arrow tip
        h1_x = end_x + head_size * math.cos(b1_angle)
        h1_y = end_y - head_size * math.sin(b1_angle)

        h2_x = end_x + head_size * math.cos(b2_angle)
        h2_y = end_y - head_size * math.sin(b2_angle)

        canvas.create_polygon([end_x, end_y, h1_x, h1_y, h2_x, h2_y], fill=arrow_color, outline=arrow_color)

    def _draw_aruco_on_contexts(self, canvas, draw_context, cx, cy, size):
        aruco_size = int(size * 0.875)
        aruco_base = self._get_aruco_image(aruco_size)

        angle_deg = -math.degrees(self.real_theta)
        aruco_rotated = aruco_base.resize((aruco_size, aruco_size), Image.NEAREST).rotate(angle_deg, expand=True,
                                                                                          resample=Image.BICUBIC)
        off_x = int(cx - aruco_rotated.width / 2)
        off_y = int(cy - aruco_rotated.height / 2)

        if draw_context is not None:
            try:
                draw_context._image.paste(aruco_rotated, (off_x, off_y), aruco_rotated)
            except AttributeError:
                if hasattr(draw_context, 'paste'):
                    draw_context.paste(aruco_rotated, (off_x, off_y), aruco_rotated)

        self.tk_aruco_img = ImageTk.PhotoImage(aruco_rotated)
        canvas.create_image(cx, cy, image=self.tk_aruco_img)