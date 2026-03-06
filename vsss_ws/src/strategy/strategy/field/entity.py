import math
from .. import constants

class Entity:
    def __init__(self, ros_handler, id):
        # --- GROUND TRUTH (Simulator/real-life) ---
        self.real_x = 0.0
        self.real_y = 0.0
        self.real_theta = 0.0
        self.real_vx = 0.0
        self.real_vy = 0.0
        self.weight = 0.0

        # --- ESTIMATED STATE (vision/detection) ---
        self.x = 0.0
        self.y = 0.0
        self.theta = 0.0
        self.vx = 0.0
        self.vy = 0.0

        self.id = id
        self.ros_handler = ros_handler
        self.pred_x = 0.0
        self.pred_y = 0.0

        self.is_predicting = False

    def map(self, x, in_min, in_max, out_min, out_max):
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min

    def pixel_to_world(self, x, y):
        screen_x = constants.LEFT_PADDING + self.map(
            x,
            -(constants.DISPLAY_FIELD_WIDTH / (2 * 100)),
            constants.DISPLAY_FIELD_WIDTH / (2 * 100),
            0,
            constants.DISPLAY_FIELD_WIDTH * constants.DISPLAY_SCALE
        )
        screen_y = constants.UPPER_PADDING + self.map(
            y * -1, # Don't know why but this fixes it
            -(constants.DISPLAY_FIELD_HEIGHT / (2 * 100)),
            constants.DISPLAY_FIELD_HEIGHT / (2 * 100),
            0,
            constants.DISPLAY_FIELD_HEIGHT * constants.DISPLAY_SCALE
        )
        return screen_x, screen_y

    def world_to_pixel(self, screen_x, screen_y):
        real_x = self.map(
            screen_x - constants.LEFT_PADDING,
            0,
            constants.DISPLAY_FIELD_WIDTH * constants.DISPLAY_SCALE,
            -(constants.DISPLAY_FIELD_WIDTH / (2 * 100)),
            constants.DISPLAY_FIELD_WIDTH / (2 * 100)
        )

        real_y_mapped = self.map(
            screen_y - constants.UPPER_PADDING,
            0,
            constants.DISPLAY_FIELD_HEIGHT * constants.DISPLAY_SCALE,
            -(constants.DISPLAY_FIELD_HEIGHT / (2 * 100)),
            constants.DISPLAY_FIELD_HEIGHT / (2 * 100)
        )

        real_y = real_y_mapped * -1

        return real_x, real_y

    def update_from_vision(self, vision_pose, dt):
        if not vision_pose or dt <= 0:
            return

        raw_vx = (vision_pose['x'] - self.x) / dt
        raw_vy = (vision_pose['y'] - self.y) / dt

        self.vx = (self.vx * 0.7) + (raw_vx * 0.3)
        self.vy = (self.vy * 0.7) + (raw_vy * 0.3)

        self.x = vision_pose['x']
        self.y = vision_pose['y']
        self.theta = vision_pose['theta']

    def apply_physics(self, dt):
        self.real_x += self.real_vx * dt
        self.real_y += self.real_vy * dt

        self.real_vx *= constants.FRICTION
        self.real_vy *= constants.FRICTION

    def request_prediction(self, seconds_in_future):
        if self.is_predicting:
            return

        self.is_predicting = True
        future = self.ros_handler.get_prediction(self.id, seconds_in_future)
        future.add_done_callback(self._on_prediction_received)

    def _on_prediction_received(self, future):
        try:
            response = future.result()
            self.pred_x = response.predicted_position.position.x
            self.pred_y = response.predicted_position.position.y
        except Exception as e:
            print(f"Error prediciendo entidad {self.id}: {e}")
        finally:
            self.is_predicting = False