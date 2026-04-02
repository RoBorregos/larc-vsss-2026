import math
import rclpy
import random
from rclpy.node import Node
from geometry_msgs.msg import Twist
from std_msgs.msg import Bool, Float32
from .entity import Entity
from .. import constants
import numpy as np
from PIL import Image, ImageDraw

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

        colors = constants.ROBOT_DATABASE.get(robot_id)
        self.main_color = colors[0]
        self.detail_colors = [colors[2], colors[1]]  # I don't know why, it just does + is not critical to find why
        self.role = None

        self.sub_cmd_vel = None
        self.sub_kicker = None
        self.sub_stop = None

        self.subscribe_to_topics(simulation)

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

    def draw(self, canvas, draw_context, simulation):
        screen_x, screen_y = self.pixel_to_world(self.real_x, self.real_y)
        size = constants.ROBOT_RADIUS * 2 * constants.DISPLAY_SCALE

        half_size = size / 2
        points = [
            (-half_size, -half_size),
            (half_size, -half_size),
            (half_size, half_size),
            (-half_size, half_size)
        ]

        rotated_points_tk = []
        rotated_points_pil = []
        for px, py in points:
            rx = px * math.cos(self.real_theta) - py * math.sin(self.real_theta) + screen_x
            ry = px * math.sin(self.real_theta) + py * math.cos(self.real_theta) + screen_y
            rotated_points_tk.extend([rx, ry])
            rotated_points_pil.append((rx, ry))

        if self.is_moving:
            self._draw_movement_arrow(canvas, screen_x, screen_y)

        canvas.create_polygon(rotated_points_tk, fill=self.main_color, outline="black")

        if (simulation): draw_context.polygon(rotated_points_pil, fill=self.main_color, outline="black")
        self._draw_identification_marks(canvas, draw_context, screen_x, screen_y, size)

        text_offset = size / 2 + constants.TEXT_OFFSET
        text_y = screen_y - text_offset

        if self.role:
            canvas.create_text(
                screen_x, text_y,
                text=self.role,
                fill="white",
                font=("Arial", 10, "bold"),
                anchor="s"
            )

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

    def _draw_identification_marks(self, canvas, draw_context, cx, cy, size):
        mark_size = size * 0.5
        half_size = size / 2

        tex_w, tex_h = int(mark_size), int(mark_size)

        marks_offsets = [
            (-half_size, -half_size),
            (half_size - mark_size, -half_size)
        ]

        for i, (ox, oy) in enumerate(marks_offsets):
            if i < len(self.detail_colors) and self.detail_colors[i]:
                base_color = self.detail_colors[i]

                m_points = [
                    (ox, oy),
                    (ox + mark_size, oy),
                    (ox + mark_size, oy + mark_size),
                    (ox, oy + mark_size)
                ]

                rotated_m_tk = []
                rotated_m_pil = []
                for px, py in m_points:
                    rx = px * math.cos(self.real_theta) - py * math.sin(self.real_theta) + cx
                    ry = px * math.sin(self.real_theta) + py * math.cos(self.real_theta) + cy
                    rotated_m_tk.extend([rx, ry])
                    rotated_m_pil.append((rx, ry))

                color_with_illumination = self._change_color_illumination(base_color,
                                                                          intensity=constants.ILLUMINATION_INTENSITY)
                m = canvas.create_polygon(rotated_m_tk, fill=color_with_illumination, outline="black")

                texture = self._generate_noisy_texture(tex_w, tex_h, color_with_illumination,
                                                       intensity=constants.ILLUMINATION_INTENSITY)

                if texture:
                    temporal_image = Image.new('RGBA', texture.size, (0, 0, 0, 0))
                    temporal_drawing = ImageDraw.Draw(temporal_image)

                    patch_points = [(0, 0), (tex_w, 0), (tex_w, tex_h), (0, tex_h)]
                    temporal_drawing.polygon(patch_points, fill=color_with_illumination)

                    codes_x = [p[0] for p in rotated_m_pil]
                    codes_y = [p[1] for p in rotated_m_pil]

                    draw_context.polygon(rotated_m_pil, fill=color_with_illumination, outline="black")

                    bbox = [min(codes_x), min(codes_y), max(codes_x), max(codes_y)]
                    for _ in range(int(mark_size * mark_size * constants.NOISE_RATE)):
                        rx = random.randint(int(bbox[0]), int(bbox[2]))
                        ry = random.randint(int(bbox[1]), int(bbox[3]))

                        dot_color = random.choice([(0, 0, 0), (255, 255, 255)])
                        draw_context.point((rx, ry), fill=dot_color)

    def _generate_noisy_texture(self, w, h, base_hex_color, intensity=30):
        if w <= 0 or h <= 0: return None

        hex_color = base_hex_color.lstrip('#')
        r, g, b = tuple(int(hex_color[i:i + 2], 16) for i in (0, 2, 4))

        img_array = np.full((int(h), int(w), 3), [r, g, b], dtype=np.uint8)

        # Generate random noice and join both color arrays (normal + noise)
        noise = np.random.normal(0, intensity, (int(h), int(w), 3))
        noisy_array = np.clip(img_array + noise, 0, 255).astype(np.uint8)

        return Image.fromarray(noisy_array, 'RGB')

    def _change_color_illumination(self, hex_color, intensity=20):
        if hex_color is None:
            return None

        hex_color = hex_color.lstrip('#')
        r, g, b = tuple(int(hex_color[i:i + 2], 16) for i in (0, 2, 4))

        r = max(0, min(255, r + random.randint(-intensity, intensity)))
        g = max(0, min(255, g + random.randint(-intensity, intensity)))
        b = max(0, min(255, b + random.randint(-intensity, intensity)))

        return f'#{r:02x}{g:02x}{b:02x}'