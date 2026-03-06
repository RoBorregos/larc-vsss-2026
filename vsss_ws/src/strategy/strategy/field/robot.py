import math
import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist
from std_msgs.msg import Bool
from .entity import Entity
from .. import constants

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

        self.canvas_id = None
        self.mark_ids = []

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
        self.real_vx = msg.linear.x
        self.real_vy = msg.linear.y
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

    def _draw_identification_marks(self, canvas, draw_context, cx, cy, size):
        for m_id in self.mark_ids:
            canvas.delete(m_id)
        self.mark_ids = []

        mark_size = size * 0.5
        half_size = size / 2

        marks_offsets = [
            (-half_size, -half_size),
            (half_size - mark_size, -half_size)
        ]

        for i, (ox, oy) in enumerate(marks_offsets):
            if i < len(self.detail_colors) and self.detail_colors[i]:
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

                m = canvas.create_polygon(rotated_m_tk, fill=self.detail_colors[i], outline="black")
                self.mark_ids.append(m)

                draw_context.polygon(rotated_m_pil, fill=self.detail_colors[i], outline="black")