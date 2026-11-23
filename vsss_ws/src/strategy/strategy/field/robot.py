import math
import random
import cv2
import numpy as np
from PIL import Image, ImageTk
from geometry_msgs.msg import Vector3
from std_msgs.msg import Bool, Float32

from .entity import Entity
from ..constants import (PhysicalProperties, VisualProperties)
from .. import settings

_ROBOT_PHYS = PhysicalProperties.Robot
_ROBOT_VIS = VisualProperties.Robot
_DISPLAY = VisualProperties.Display

ARUCO_DICT = cv2.aruco.getPredefinedDictionary(cv2.aruco.DICT_ARUCO_ORIGINAL)


class Robot(Entity):
    def __init__(
        self,
        ros_handler,
        robot_id: int,
        start_x: float,
        start_y: float,
        start_theta: float = 0.0,
        simulation: bool = False,
    ):
        super().__init__(ros_handler, robot_id)
        self.simulation: bool = simulation

        # Propiedades físicas
        self.mass_g: float = _ROBOT_PHYS.MASS_G
        self.apothem_mm: float = _ROBOT_PHYS.APOTHEM_MM
        self.restitution: float = _ROBOT_PHYS.RESTITUTION
        self.friction: float = _ROBOT_PHYS.FRICTION

        # Estado dinámico (mm y rad)
        self.real_x: float = start_x
        self.real_y: float = start_y
        self.real_theta: float = start_theta

        # Control y estado de juego
        self.target_speed: float = 0.0
        self.target_angle_rad: float = 0.0
        self.is_moving: bool = False
        self.kick_request: bool = False
        self.role: str | None = None

        # Propiedades visuales
        self.screen_half_size: float = self.apothem_mm * _DISPLAY.SCALE_PX_PER_MM
        self.aruco_id: int | None = _ROBOT_PHYS.ARUCO_DATABASE.get(robot_id)
        self.main_color: str = (
            _ROBOT_VIS.TEAM_BLUE_COLOR
            if robot_id >= _ROBOT_PHYS.BLUE_TEAM_MIN_ID
            else _ROBOT_VIS.TEAM_YELLOW_COLOR
        )
        self._aruco_cache: Image.Image | None = None
        self.tk_aruco_img: ImageTk.PhotoImage | None = None

        # Suscripciones ROS
        self.sub_cmd_vel = None
        self.sub_kicker = None
        self.sub_stop = None
        self.sub_bno = None
        self._subscribe_to_topics()

    @staticmethod
    def _hex_to_rgb(hex_color: str) -> tuple[int, int, int]:
        hex_color = hex_color.lstrip("#")
        return tuple(int(hex_color[i : i + 2], 16) for i in (0, 2, 4))

    def _subscribe_to_topics(self) -> None:
        if self.simulation:
            self.sub_cmd_vel = self.ros_handler.create_subscription(
                Vector3, f"/strategy/robot_{self.id}/motion_control", self._simulation_move, 10
            )
            self.sub_kicker = self.ros_handler.create_subscription(
                Bool, f"/strategy/robot_{self.id}/kicker", self._simulation_kick, 10
            )
            self.sub_stop = self.ros_handler.create_subscription(
                Bool, f"/strategy/robot_{self.id}/stop", self._simulation_stop, 10
            )
        else:
            self.sub_bno = self.ros_handler.create_subscription(
                Float32, f"/communication/robot_{self.id}/bno", self._communication_bno, 10
            )

    def destroy(self, canvas) -> None:
        for sub in (self.sub_cmd_vel, self.sub_stop, self.sub_kicker, self.sub_bno):
            if sub:
                self.ros_handler.destroy_subscription(sub)

    # --- CALLBACKS ROS ---

    def _simulation_move(self, msg: Vector3) -> None:
        sim = settings.simulation

        self.target_speed = msg.x
        self.target_angle_rad = msg.y
        self.is_moving = self.target_speed > (sim.min_speed_mm_s * 2)

        new_speed = self.target_speed + random.gauss(0, abs(self.target_speed) * sim.movement_noise)
        new_angle = self.target_angle_rad + random.gauss(0, sim.movement_noise)

        if abs(new_speed) <= sim.min_speed_mm_s:
            new_speed = 0.0

        self.real_vx = new_speed * math.cos(new_angle)
        self.real_vy = new_speed * math.sin(new_angle)

        angular_vel = msg.z + random.gauss(0, sim.movement_noise)
        self.real_angular_vel = 0.0 if abs(angular_vel) <= sim.min_speed_mm_s else angular_vel


    def _communication_bno(self, msg: Float32) -> None:
        if msg.data:
            self.real_theta = msg.data

    def _simulation_kick(self, msg: Bool) -> None:
        if msg.data:
            self.kick_request = True

    def _simulation_stop(self, msg: Bool) -> None:
        if msg.data:
            self.real_vx = 0.0
            self.real_vy = 0.0
            self.is_moving = False
            self.target_speed = 0.0

    # --- COMANDOS DE ESTRATEGIA ---

    def stop(self) -> None:
        self.is_moving = False
        self.target_speed = 0.0
        self.ros_handler.publish_stop(self.id, True)

    def move(self, speed_mm_s: float, angle_rad: float, facing_target_deg: float) -> None:
        self.target_speed = speed_mm_s
        self.target_angle_rad = angle_rad
        self.is_moving = speed_mm_s > (settings.simulation.min_speed_mm_s * 2)

        self.ros_handler.publish_stop(self.id, False)

        facing_error = self._normalize_angle(self.theta - math.radians(facing_target_deg))
        self.ros_handler.publish_omni_move(self.id, speed_mm_s, angle_rad, facing_error)

    def kicker(self, active: bool) -> None:
        self.ros_handler.publish_kicker(self.id, active)

    # --- RENDERIZADO ---

    def _get_aruco_image(self) -> Image.Image:
        if self._aruco_cache is not None:
            return self._aruco_cache

        img_aruco = cv2.aruco.generateImageMarker(
            ARUCO_DICT, self.aruco_id, _ROBOT_VIS.ARUCO_MARKER_PX
        )
        data = np.array(Image.fromarray(img_aruco).convert("RGBA"))
        rgb_team = self._hex_to_rgb(self.main_color)

        if self.main_color == _ROBOT_VIS.TEAM_BLUE_COLOR:
            mask = (data[:, :, 0:3] == [0, 0, 0]).all(axis=2)
        else:
            mask = (data[:, :, 0:3] == [255, 255, 255]).all(axis=2)

        data[mask, 0:3] = rgb_team
        self._aruco_cache = Image.fromarray(data)
        return self._aruco_cache

    def draw(self, canvas, draw_context, simulation: bool) -> None:
        screen_x, screen_y = self.world_to_pixel(self.real_x, self.real_y)
        half = self.screen_half_size
        size = half * 2

        cos_t = math.cos(self.real_theta)
        sin_t = math.sin(self.real_theta)
        corners = ((-half, -half), (half, -half), (half, half), (-half, half))

        rotated_tk: list[float] = []
        rotated_pil: list[tuple[float, float]] = []
        for px, py in corners:
            rx = px * cos_t - py * sin_t + screen_x
            ry = px * sin_t + py * cos_t + screen_y
            rotated_tk.extend((rx, ry))
            rotated_pil.append((rx, ry))

        canvas.create_polygon(rotated_tk, fill=_ROBOT_VIS.BODY_COLOR, outline=_ROBOT_VIS.OUTLINE_COLOR)
        if simulation and draw_context:
            draw_context.polygon(rotated_pil, fill=_ROBOT_VIS.BODY_COLOR, outline=_ROBOT_VIS.OUTLINE_COLOR)

        self._draw_aruco_on_contexts(canvas, draw_context, screen_x, screen_y, size)

        if self.is_moving:
            self._draw_movement_arrow(canvas, screen_x, screen_y)

        if self.role:
            text_y = screen_y - (half + _ROBOT_VIS.ROLE_TEXT_OFFSET_PX)
            canvas.create_text(
                screen_x,
                text_y,
                text=self.role,
                fill=_ROBOT_VIS.ROLE_TEXT_COLOR,
                font=_ROBOT_VIS.ROLE_FONT,
                anchor="s",
            )

    def _draw_aruco_on_contexts(self, canvas, draw_context, cx: float, cy: float, size: float) -> None:
        aruco_size = int(size * _ROBOT_VIS.ARUCO_SIZE_RATIO)
        aruco_base = self._get_aruco_image()

        angle_deg = -math.degrees(self.real_theta)
        aruco_rotated = aruco_base.resize((aruco_size, aruco_size), Image.NEAREST).rotate(
            angle_deg, expand=True, resample=Image.BICUBIC
        )

        off_x = int(cx - aruco_rotated.width / 2)
        off_y = int(cy - aruco_rotated.height / 2)

        if draw_context is not None:
            if hasattr(draw_context, "_image"):
                draw_context._image.paste(aruco_rotated, (off_x, off_y), aruco_rotated)
            elif hasattr(draw_context, "paste"):
                draw_context.paste(aruco_rotated, (off_x, off_y), aruco_rotated)

        self.tk_aruco_img = ImageTk.PhotoImage(aruco_rotated)
        canvas.create_image(cx, cy, image=self.tk_aruco_img)

    def _draw_movement_arrow(self, canvas, cx: float, cy: float) -> None:
        end_x = cx + _ROBOT_VIS.ARROW_LENGTH_PX * math.cos(self.target_angle_rad)
        end_y = cy - _ROBOT_VIS.ARROW_LENGTH_PX * math.sin(self.target_angle_rad)

        head_size = _ROBOT_VIS.ARROW_WIDTH_PX * 3
        head_angle = math.pi / 6

        b1 = self.target_angle_rad + math.pi - head_angle
        b2 = self.target_angle_rad + math.pi + head_angle

        h1_x = end_x + head_size * math.cos(b1)
        h1_y = end_y - head_size * math.sin(b1)
        h2_x = end_x + head_size * math.cos(b2)
        h2_y = end_y - head_size * math.sin(b2)

        canvas.create_polygon(
            [end_x, end_y, h1_x, h1_y, h2_x, h2_y],
            fill=_ROBOT_VIS.ARROW_COLOR,
            outline=_ROBOT_VIS.ARROW_COLOR,
        )