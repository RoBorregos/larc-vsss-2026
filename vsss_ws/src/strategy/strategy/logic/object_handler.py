import time
import math
from ..field.robot import Robot
from ..field.ball import Ball
from .. import constants
from .physics import resolve_physics

class ObjectHandler:
    def __init__(self, infield_objects, ros_handler, simulation = False):
        self.infield_objects = infield_objects
        self.ros_handler = ros_handler
        self.simulation = simulation

        self.entities = {}

        self.BLUE_RANGE = list(range(10, 20))  # IDs 10-19
        self.YELLOW_RANGE = list(range(0, 10))  # IDs 0-9

        self.last_update_time = time.time()

        self._init_objects()

    def update(self):
        current_time = time.time()
        dt = current_time - self.last_update_time

        if dt <= 0:
            return

        for entity_id, entity in self.entities.items():
            frame_name = "ball" if entity_id == 20 else f"robot_{entity_id}"
            new_pose = self.ros_handler.get_pose(frame_name)

            if new_pose:
                entity.update_from_vision(new_pose, dt)
                entity.request_prediction(seconds_in_future=0.5)

            entity.apply_physics(dt)

        ball = self.get_ball()
        robots = self.get_all_robots()
        resolve_physics([ball, *robots])

        self.last_update_time = current_time

    def draw(self, canvas, draw_context):
        width = canvas.winfo_width()
        height = canvas.winfo_height()

        for entity in self.entities.values():
            entity.draw(canvas, draw_context)

    def _init_objects(self):
        for infield_object in self.infield_objects:
            if infield_object["id"] == 20:
                self.spawn_ball(infield_object["x"], infield_object["y"], infield_object["theta"])
            elif 0 <= infield_object["id"] < 20:
                self.spawn_robot(infield_object["id"], infield_object["x"], infield_object["y"], infield_object["theta"])

    def spawn_robot(self, robot_id, start_x, start_y, start_theta):
        robot = Robot(self.ros_handler, robot_id, start_x, start_y, start_theta, self.simulation)
        self.entities[robot_id] = robot

    def spawn_ball(self, start_x, start_y, start_theta):
        ball = Ball(self.ros_handler, start_x, start_y, start_theta)
        self.entities[20] = ball

    def get_blue_robots(self):
        return [e for e in self.entities.values()
                if isinstance(e, Robot) and e.id in self.BLUE_RANGE]

    def get_yellow_robots(self):
        return [e for e in self.entities.values()
                if isinstance(e, Robot) and e.id in self.YELLOW_RANGE]

    def get_all_robots(self):
        return [e for e in self.entities.values() if isinstance(e, Robot)]

    def get_robot(self, robot_id):
        entity = self.entities.get(robot_id)
        return entity if isinstance(entity, Robot) else None

    def get_ball(self):
        return self.entities.get(20)