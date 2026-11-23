import time
from ..field.robot import Robot
from ..field.ball import Ball
from ..constants import PhysicalProperties, RosProperties
from .. import settings
from ..simulator.physics import resolve_physics

_BALL_ID = PhysicalProperties.Ball.ID
_BLUE_MIN_ID = PhysicalProperties.Robot.BLUE_TEAM_MIN_ID


class ObjectHandler:
    def __init__(self, infield_objects, ros_handler, simulation: bool = False):
        self.infield_objects = infield_objects
        self.ros_handler = ros_handler
        self.simulation = simulation

        self.entities: dict[int, Robot | Ball] = {}
        self.last_update_time: float = time.time()
        self.in_goal_callback: bool = False

        self._init_objects()

    def update(self, canvas, simulation: bool) -> None:
        current_time = time.time()
        dt = current_time - self.last_update_time
        if dt <= 0:
            return

        for entity_id, entity in self.entities.items():
            frame_name = RosProperties.BALL_FRAME if entity_id == _BALL_ID else f"robot_{entity_id}"
            new_pose = self.ros_handler.get_pose(frame_name)

            if new_pose:
                entity.update_from_vision(new_pose, dt)
                entity.request_prediction(seconds_in_future=settings.strategy.prediction_horizon_s)
                if not simulation:
                    entity.set_display_pose(new_pose)

            if simulation:
                entity.apply_physics(dt)

        if simulation:
            resolve_physics(list(self.entities.values()), self.goal_callback, canvas)

        self.last_update_time = current_time

    def goal_callback(self, canvas) -> None:
        if self.in_goal_callback:
            return
        self.in_goal_callback = True

        width = canvas.winfo_width()
        height = canvas.winfo_height()

        goal_text = canvas.create_text(
            width / 2,
            height / 2,
            text="GOAL!",
            fill="yellow",
            font=("Arial", 80, "bold"),
        )

        def finish_and_reset(count: int) -> None:
            if count <= 0:
                canvas.delete(goal_text)
                for entity in self.entities.values():
                    entity.destroy(canvas)

                self.entities = {}
                self._init_objects()
                self.in_goal_callback = False
                return

            new_color = "red" if canvas.itemcget(goal_text, "fill") == "yellow" else "yellow"
            canvas.itemconfig(goal_text, fill=new_color)
            canvas.after(200, lambda: finish_and_reset(count - 1))

        finish_and_reset(10)

    def draw(self, canvas, draw_context, simulation: bool) -> None:
        if self.in_goal_callback:
            return
        for entity in self.entities.values():
            entity.draw(canvas, draw_context, simulation)

    def stop_robots(self) -> None:
        for robot in self.get_all_robots():
            robot.stop()

    def _init_objects(self) -> None:
        for obj in self.infield_objects:
            if obj["id"] == _BALL_ID:
                self.spawn_ball(obj["x"], obj["y"], obj["theta"])
            elif 0 <= obj["id"] < _BALL_ID:
                self.spawn_robot(obj["id"], obj["x"], obj["y"], obj["theta"])

    def spawn_robot(self, robot_id: int, start_x: float, start_y: float, start_theta: float) -> None:
        self.entities[robot_id] = Robot(
            self.ros_handler, robot_id, start_x, start_y, start_theta, self.simulation
        )

    def spawn_ball(self, start_x: float, start_y: float, start_theta: float) -> None:
        self.entities[_BALL_ID] = Ball(self.ros_handler, start_x, start_y, start_theta)

    def get_blue_robots(self) -> list[Robot]:
        return [e for e in self.entities.values() if isinstance(e, Robot) and e.id >= _BLUE_MIN_ID]

    def get_yellow_robots(self) -> list[Robot]:
        return [e for e in self.entities.values() if isinstance(e, Robot) and e.id < _BLUE_MIN_ID]

    def get_all_robots(self) -> list[Robot]:
        return [e for e in self.entities.values() if isinstance(e, Robot)]

    def get_robot(self, robot_id: int) -> Robot | None:
        entity = self.entities.get(robot_id)
        return entity if isinstance(entity, Robot) else None

    def get_ball(self) -> Ball | None:
        entity = self.entities.get(_BALL_ID)
        return entity if isinstance(entity, Ball) else None