import math

from PIL.JpegImagePlugin import zigzag_index

from .entity import Entity
from .. import constants

ball_id = 20

class Ball(Entity):
    def __init__(self, ros_handler, start_x, start_y, start_theta):
        super().__init__(ros_handler, ball_id)
        self.id = 20
        self.weight = 48
        self.screen_radius = constants.BALL_RADIUS * constants.DISPLAY_SCALE
        self.real_radius = constants.BALL_RADIUS / 100
        self.real_x = start_x
        self.real_y = start_y
        self.real_theta = start_theta

        print(f"started real values with: r({start_x}, {start_y}) - v({self.real_x}, {self.real_y})")

        self.canvas_id = None

    def draw(self, canvas, draw_context):
        screen_x, screen_y = self.pixel_to_world(self.real_x, self.real_y)

        bbox = [
            screen_x - self.screen_radius,
            screen_y - self.screen_radius,
            screen_x + self.screen_radius,
            screen_y + self.screen_radius
        ]

        if self.canvas_id:
            canvas.coords(self.canvas_id, *bbox)
        else:
            self.canvas_id = canvas.create_oval(
                *bbox,
                fill="orange",
                outline="black",
                width=10,
                tags="ball",
            )
        canvas.tag_raise(self.canvas_id)
        draw_context.ellipse(
            bbox,
            fill="orange",
            outline="black",
            width=1
        )
