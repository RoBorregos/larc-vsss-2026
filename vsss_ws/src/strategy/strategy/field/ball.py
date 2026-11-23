from .entity import Entity
from ..constants import PhysicalProperties, VisualProperties

_BALL_PHYS = PhysicalProperties.Ball
_BALL_VIS = VisualProperties.Ball
_DISPLAY = VisualProperties.Display


class Ball(Entity):
    def __init__(self, ros_handler, start_x: float, start_y: float, start_theta: float = 0.0):
        super().__init__(ros_handler, _BALL_PHYS.ID)

        # Propiedades físicas
        self.mass_g: float = _BALL_PHYS.MASS_G
        self.radius_mm: float = _BALL_PHYS.RADIUS_MM
        self.restitution: float = _BALL_PHYS.RESTITUTION
        self.friction: float = _BALL_PHYS.FRICTION

        # Estado dinámico
        self.real_x: float = start_x
        self.real_y: float = start_y
        self.real_theta: float = start_theta

        # Propiedades visuales
        self.screen_radius: float = self.radius_mm * _DISPLAY.SCALE_PX_PER_MM

    def destroy(self, canvas) -> None:
        canvas.delete(_BALL_VIS.TAG)

    def draw(self, canvas, draw_context, simulation: bool) -> None:
        screen_x, screen_y = self.world_to_pixel(self.real_x, self.real_y)

        bbox = [
            screen_x - self.screen_radius,
            screen_y - self.screen_radius,
            screen_x + self.screen_radius,
            screen_y + self.screen_radius,
        ]

        canvas.create_oval(
            *bbox,
            fill=_BALL_VIS.COLOR,
            outline=_BALL_VIS.OUTLINE_COLOR,
            width=_BALL_VIS.OUTLINE_WIDTH_PX,
            tags=_BALL_VIS.TAG,
        )

        if simulation:
            draw_context.ellipse(
                bbox,
                fill=_BALL_VIS.COLOR,
                outline=_BALL_VIS.OUTLINE_COLOR,
                width=_BALL_VIS.OUTLINE_WIDTH_PX,
            )