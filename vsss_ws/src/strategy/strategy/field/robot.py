import math
import rclpy
import random
from rclpy.node import Node
from geometry_msgs.msg import Twist
from std_msgs.msg import Bool
from .entity import Entity
from .. import constants
import numpy as np
from PIL import Image, ImageDraw

class Robot(Entity):
    def __init__(self, ros_handler, robot_id, start_x, start_y, start_theta, simulation = False):
        super().__init__()
        self.id = robot_id
        self.ros_handler = ros_handler
        self.simulation = simulation
        self.weight = 200
        self.screen_radius = constants.ROBOT_RADIUS * constants.DISPLAY_SCALE
        self.real_radius = constants.ROBOT_RADIUS / 100

        self.real_x = start_x
        self.real_y = start_y
        self.real_theta = start_theta

        colors = constants.ROBOT_DATABASE.get(robot_id)
        self.main_color = colors[0]
        self.detail_colors = [colors[2], colors[1]] # I don't know why, it just does + is not critical to find why
        self.role = None

        self.canvas_id = None
        self.mark_ids = []
        self.text_id = None

        self.sub_cmd_vel = None
        self.sub_stop = None

        if simulation:
            self.subscribe_to_topics()

    def subscribe_to_topics(self):
        self.sub_cmd_vel = self.ros_handler.create_subscription(
            Twist,
            f'/strategy/robot_{self.id}/cmd_vel',
            self._simulation_move,
            10
        )
        self.sub_stop = self.ros_handler.create_subscription(
            Bool,
            f'/strategy/robot_{self.id}/stop',
            self._simulation_stop,
            10
        )

    def _simulation_move(self, msg):
        new_vx = msg.linear.x + random.gauss(0, abs(msg.linear.x) * constants.MOVEMENT_NOISE)
        new_vy = msg.linear.y + random.gauss(0, abs(msg.linear.y) * constants.MOVEMENT_NOISE)

        if math.fabs(new_vx) <= constants.MIN_SPEED: new_vx = 0
        if math.fabs(new_vy) <= constants.MIN_SPEED: new_vy = 0

        self.real_vx = new_vx
        self.real_vy = new_vy
        self.real_theta_vel = msg.angular.z

    def _simulation_stop(self, msg):
        if msg.data:
            print("Stop robot with id {}".format(self.id))
            self.real_vx = 0
            self.real_vy = 0

    def stop(self):
        self.ros_handler.publish_stop(self.id, True)

    def move(self, speed, angle):
        self.ros_handler.publish_stop(self.id, False)
        self.ros_handler.publish_omni_move(self.id, speed, angle)

    def draw(self, canvas, draw_context):
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

        if self.canvas_id:
            canvas.delete(self.canvas_id)
        self.canvas_id = canvas.create_polygon(rotated_points_tk, fill=self.main_color, outline="black")

        draw_context.polygon(rotated_points_pil, fill=self.main_color, outline="black")
        self._draw_identification_marks(canvas, draw_context, screen_x, screen_y, size)

        self._draw_identification_marks(canvas, draw_context, screen_x, screen_y, size)

        text_offset = size / 2 + constants.TEXT_OFFSET
        text_y = screen_y - text_offset

        if self.text_id:
            canvas.delete(self.text_id)

        if self.role:
            self.text_id = canvas.create_text(
                screen_x, text_y,
                text=self.role,
                fill="white",
                font=("Arial", 10, "bold"),
                anchor="s"
            )

    def _draw_identification_marks(self, canvas, draw_context, cx, cy, size):
        for m_id in self.mark_ids:
            canvas.delete(m_id)
        self.mark_ids = []

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

                color_with_illumination = self._change_color_illumination(base_color, intensity=constants.ILLUMINATION_INTENSITY)
                m = canvas.create_polygon(rotated_m_tk, fill=color_with_illumination, outline="black")
                self.mark_ids.append(m)

                texture = self._generate_noisy_texture(tex_w, tex_h, color_with_illumination, intensity=constants.ILLUMINATION_INTENSITY)

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