import math
from ..constants import PhysicalProperties, VisualProperties
from .. import settings

_FIELD = PhysicalProperties.Field
_DISPLAY = VisualProperties.Display


class Entity:
    """Clase base para entidades físicas (Ball, Robot).
    Unidades internas: posición en mm, velocidad en mm/s, masa en g, ángulos en rad.
    """

    def __init__(self, ros_handler, entity_id: int):
        self.id: int = entity_id
        self.ros_handler = ros_handler

        # --- PROPIEDADES FÍSICAS (sobrescritas por Ball / Robot) ---
        self.mass_g: float = 0.0
        self.restitution: float = 0.0
        self.friction: float = 1.0

        # --- GROUND TRUTH (Simulador / realidad) ---
        self.real_x: float = 0.0
        self.real_y: float = 0.0
        self.real_theta: float = 0.0
        self.real_vx: float = 0.0
        self.real_vy: float = 0.0
        self.real_angular_vel: float = 0.0

        # --- ESTADO ESTIMADO (Visión / detección) ---
        self.x: float = 0.0
        self.y: float = 0.0
        self.theta: float = 0.0
        self.vx: float = 0.0
        self.vy: float = 0.0

        # --- PREDICCIÓN ---
        self.pred_x: float = 0.0
        self.pred_y: float = 0.0
        self.is_predicting: bool = False

    @staticmethod
    def _normalize_angle(angle_rad: float) -> float:
        return math.atan2(math.sin(angle_rad), math.cos(angle_rad))

    def world_to_pixel(self, x_mm: float, y_mm: float) -> tuple[float, float]:
        """Convierte coordenadas del mundo (mm, origen al centro) a píxeles de pantalla."""
        screen_x = _DISPLAY.LEFT_PADDING_PX + (x_mm + _FIELD.WIDTH_MM / 2) * _DISPLAY.SCALE_PX_PER_MM
        screen_y = _DISPLAY.UPPER_PADDING_PX + (_FIELD.HEIGHT_MM / 2 - y_mm) * _DISPLAY.SCALE_PX_PER_MM
        return screen_x, screen_y

    def pixel_to_world(self, screen_x: float, screen_y: float) -> tuple[float, float]:
        """Convierte píxeles de pantalla a coordenadas del mundo (mm, origen al centro)."""
        x_mm = (screen_x - _DISPLAY.LEFT_PADDING_PX) / _DISPLAY.SCALE_PX_PER_MM - _FIELD.WIDTH_MM / 2
        y_mm = _FIELD.HEIGHT_MM / 2 - (screen_y - _DISPLAY.UPPER_PADDING_PX) / _DISPLAY.SCALE_PX_PER_MM
        return x_mm, y_mm

    def update_from_vision(self, vision_pose: dict | None, dt: float) -> None:
        if not vision_pose or dt <= 0:
            return

        alpha = settings.vision.velocity_smoothing_alpha
        raw_vx = (vision_pose["x"] - self.x) / dt
        raw_vy = (vision_pose["y"] - self.y) / dt

        self.vx = self.vx * (1.0 - alpha) + raw_vx * alpha
        self.vy = self.vy * (1.0 - alpha) + raw_vy * alpha

        self.x = vision_pose["x"]
        self.y = vision_pose["y"]
        self.theta = vision_pose["theta"]

    def set_display_pose(self, vision_pose: dict | None) -> None:
        if not vision_pose:
            return

        self.real_x = vision_pose["x"]
        self.real_y = vision_pose["y"]
        self.real_theta = self._normalize_angle(-vision_pose["theta"] + math.pi / 2)

    def apply_physics(self, dt: float) -> None:
        self.real_x += self.real_vx * dt
        self.real_y += self.real_vy * dt
        self.real_theta = self._normalize_angle(self.real_theta + self.real_angular_vel * dt)

        self.real_vx *= self.friction
        self.real_vy *= self.friction
        self.real_angular_vel *= self.friction

    def request_prediction(self, seconds_in_future: float) -> None:
        if self.is_predicting:
            return

        self.is_predicting = True
        future = self.ros_handler.get_prediction(self.id, seconds_in_future)
        future.add_done_callback(self._on_prediction_received)

    def _on_prediction_received(self, future) -> None:
        try:
            response = future.result()
            self.pred_x = response.predicted_position.position.x * 1000.0
            self.pred_y = response.predicted_position.position.y * 1000.0
        except Exception as e:
            print(f"Error prediciendo entidad {self.id}: {e}")
        finally:
            self.is_predicting = False