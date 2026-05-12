import torch
import math
import time
from collections import deque
from typing import Callable

from isaacsim.core.api import World
from isaacsim.core.utils.stage import add_reference_to_stage
from isaacsim.core.prims import Articulation
from isaacsim.core.utils.prims import get_prim_at_path
from pxr import UsdGeom, Gf


class Robot:
    """Wrapper para el robot Omnidireccional.

    Arquitectura basada en Control de Velocidad (Velocity Drive) con Slew Rate Asimétrico.
    Simula de manera estable y realista la inercia, la fricción de la caja reductora
    y el coasting de los motores, ideal para transferencias Sim-to-Real.
    """

    def __init__(
            self,
            usd_path: str,
            prim_path: str,
            name: str = 'omnirobot',
            device: str = 'cuda:0',
            spawn_orientation_euler_deg: tuple[float, float, float] = (90.0, 0.0, 0.0),
            spawn_position: tuple[float, float, float] = (0.0, 0.0, 0.0),
    ):
        # 1. Propiedades de Identidad y Escenario
        self.usd_path = usd_path
        self.prim_path = prim_path
        self.name = name
        self.device = device
        self.spawn_orientation_euler_deg = spawn_orientation_euler_deg
        self.spawn_position = spawn_position

        self.articulation: Articulation | None = None
        self.num_dof: int = 0
        self.dof_names: list[str] = []
        self.wheel_indexes: torch.Tensor | None = None
        self.roller_indexes: torch.Tensor | None = None

        # 2. Mapeo Físico y Cinemático (Configuración Diamante / Cruz)
        self.ID_FRONT = 0
        self.ID_LEFT = 1
        self.ID_BACK = 2
        self.ID_RIGHT = 3

        # Inversores de giro (Polaridad)
        self.dir_FRONT = -1.0
        self.dir_LEFT = -1.0
        self.dir_BACK = 1.0
        self.dir_RIGHT = -1.0

        # 3. Parámetros Dinámicos y Cinemáticos (N20)
        self.max_velocity: float = 57.6  # rad/s (~550 RPM)
        self.max_acceleration_step: float = 1.5  # Aceleración con peso
        self.friction_coast_step: float = 3.0  # Fricción natural (Coasting)
        self.active_brake_step: float = 15.0  # Freno electromagnético por corto
        self.latency_steps: int = 0
        self.encoder_noise_std: float = 0.0

        # 4. Samplers para Randomización de Dominio
        self._max_velocity_sampler: Callable[[], float] | None = None
        self._max_acceleration_sampler: Callable[[], float] | None = None
        self._latency_steps_sampler: Callable[[], int] | None = None
        self._encoder_noise_std_sampler: Callable[[], torch.Tensor] | None = None

        # 5. Buffers de Estado y Control
        self.command_buffer: deque[torch.Tensor] | None = None
        self.last_velocity_target = torch.zeros(4, device=self.device)
        self.current_throttle = torch.zeros(4, device=self.device)
        self._debug_last_print = 0.0

    # ================================================================== #
    # 1. CICLO DE VIDA E INICIALIZACIÓN
    # ================================================================== #

    def spawn(self) -> None:
        """Añade el USD del robot al escenario."""
        add_reference_to_stage(usd_path=self.usd_path, prim_path=self.prim_path)

        prim = get_prim_at_path(self.prim_path)
        xform = UsdGeom.Xformable(prim)
        xform.ClearXformOpOrder()

        xform.AddTranslateOp().Set(Gf.Vec3d(*self.spawn_position))
        xform.AddRotateXYZOp().Set(Gf.Vec3f(*self.spawn_orientation_euler_deg))

        self.articulation = Articulation(prim_paths_expr=self.prim_path, name=self.name)

    def initialize(self) -> None:
        """Finaliza el handle de la articulación y mapea las juntas. (Llamar tras world.reset())"""
        assert self.articulation is not None, "Llama a spawn() antes de initialize()."
        self.articulation.initialize()

        self.num_dof = self.articulation.num_dof
        self.dof_names = list(self.articulation.dof_names)

        wheel_idx = [i for i, n in enumerate(self.dof_names) if n.startswith("wheel_")]
        roller_idx = [i for i, n in enumerate(self.dof_names) if n.startswith("roller_")]

        self.wheel_indexes = torch.tensor(wheel_idx, dtype=torch.long, device=self.device)
        self.roller_indexes = torch.tensor(roller_idx, dtype=torch.long, device=self.device)

        print(f"[{self.name}] Inicializado: {self.num_dof} DOFs ({len(wheel_idx)} llantas)")

    def configure_drive_modes(self, wheel_damping: float = 0.1) -> None:
        """Configura las juntas para el control implícito de velocidad."""
        assert self.articulation is not None, "Llama a initialize() primero."
        kps = torch.zeros(self.num_dof, device=self.device)
        kds = torch.zeros(self.num_dof, device=self.device)
        kds[self.wheel_indexes] = wheel_damping

        self.articulation.set_gains(
            kps=self._to_articulation(kps.unsqueeze(0)),
            kds=self._to_articulation(kds.unsqueeze(0)),
        )

    def configure_motors(
            self,
            max_velocity_sampler: Callable[[], float] | None = None,
            max_acceleration_sampler: Callable[[], float] | None = None,
            latency_steps_sampler: Callable[[], int] | None = None,
            encoder_noise_std_sampler: Callable[[], torch.Tensor] | None = None,
    ) -> None:
        """Configura y prepara los samplers cinemáticos."""
        self._max_velocity_sampler = max_velocity_sampler
        self._max_acceleration_sampler = max_acceleration_sampler
        self._latency_steps_sampler = latency_steps_sampler
        self._encoder_noise_std_sampler = encoder_noise_std_sampler
        self.resample_motor_params()

    def resample_motor_params(self) -> None:
        """Extrae nuevos valores (Útil para Domain Randomization al inicio del episodio)."""
        self.max_velocity = self._max_velocity_sampler() if self._max_velocity_sampler else 57.6
        self.max_acceleration_step = self._max_acceleration_sampler() if self._max_acceleration_sampler else 1.5
        self.latency_steps = self._latency_steps_sampler() if self._latency_steps_sampler else 0
        self.encoder_noise_std = self._encoder_noise_std_sampler() if self._encoder_noise_std_sampler else 0.0

        zero_cmd = torch.zeros(4, device=self.device)
        self.command_buffer = deque([zero_cmd.clone() for _ in range(self.latency_steps + 1)],
                                    maxlen=self.latency_steps + 1)

    # ================================================================== #
    # 2. SENSORES Y ODOMETRÍA (NUEVO)
    # ================================================================== #

    def get_observed_wheel_velocities(self) -> torch.Tensor:
        """Lectura de encoders simulados con ruido opcional."""
        wheel_vel = self._get_wheel_velocities_raw()
        if self.encoder_noise_std > 0:
            noise = torch.randn_like(wheel_vel) * self.encoder_noise_std
            wheel_vel = wheel_vel + noise
        return wheel_vel

    def get_world_pose(self) -> tuple[torch.Tensor, torch.Tensor]:
        """Devuelve la posición (x,y,z) y el cuaternión (w,x,y,z) del chasis en el mundo."""
        assert self.articulation is not None
        positions, orientations = self.articulation.get_world_poses()
        return self._as_torch(positions[0]), self._as_torch(orientations[0])

    def get_heading_rad(self) -> torch.Tensor:
        """Calcula el ángulo de guiñada (Yaw) del robot en radianes."""
        _, quat = self.get_world_pose()
        # Isaac Sim retorna los cuaterniones en formato [w, x, y, z]
        w, x, y, z = quat[0], quat[1], quat[2], quat[3]

        # Conversión de Cuaternión a Ángulo de Euler (Yaw en el eje Z)
        siny_cosp = 2 * (w * z + x * y)
        cosy_cosp = 1 - 2 * (y * y + z * z)
        yaw = torch.atan2(siny_cosp, cosy_cosp)
        return yaw

    def get_heading_deg(self) -> torch.Tensor:
        """Calcula el ángulo de guiñada (Yaw) del robot en grados."""
        yaw_rad = self.get_heading_rad()
        return yaw_rad * (180.0 / math.pi)

    # ================================================================== #
    # 3. CONTROL DE ALTO NIVEL (CINEMÁTICA)
    # ================================================================== #

    def set_throttle_front(self, speed: float) -> None:
        self.current_throttle[self.ID_FRONT] = speed * self.dir_FRONT

    def set_throttle_back(self, speed: float) -> None:
        self.current_throttle[self.ID_BACK] = speed * self.dir_BACK

    def set_throttle_left(self, speed: float) -> None:
        self.current_throttle[self.ID_LEFT] = speed * self.dir_LEFT

    def set_throttle_right(self, speed: float) -> None:
        self.current_throttle[self.ID_RIGHT] = speed * self.dir_RIGHT

    def move_omnidirectional(self, speed: float, direction_deg: float) -> None:
        """Traduce un vector de velocidad (magnitud y ángulo) a comandos de motor."""
        dir_rad = math.radians(direction_deg)

        front_speed = speed * math.sin(dir_rad + math.radians(0))
        left_speed = speed * math.sin(dir_rad + math.radians(90))
        back_speed = speed * math.sin(dir_rad + math.radians(180))
        right_speed = speed * math.sin(dir_rad + math.radians(270))

        max_speed = max(abs(front_speed), abs(left_speed), abs(back_speed), abs(right_speed))

        if max_speed != 0:
            ratio = speed / max_speed
            front_speed *= ratio
            left_speed *= ratio
            back_speed *= ratio
            right_speed *= ratio

        self.set_throttle_front(front_speed)
        self.set_throttle_left(left_speed)
        self.set_throttle_back(back_speed)
        self.set_throttle_right(right_speed)

    def commit_motor_commands(self, debug: bool = False) -> None:
        """Envía todas las señales acumuladas al simulador de un solo golpe."""
        self._apply_motor_command(self.current_throttle, debug=debug)

    # ================================================================== #
    # 4. CONTROL DE BAJO NIVEL Y FÍSICAS (PRIVADO)
    # ================================================================== #

    def _apply_motor_command(self, throttle: torch.Tensor, debug: bool = False) -> None:
        """Lógica interna de rampa de velocidad y envío a PhysX."""
        assert self.articulation is not None
        assert self.command_buffer is not None, "Llama a configure_motors() antes."

        target_throttle = throttle.to(self.device).clamp(-1.0, 1.0)
        final_target_velocity = target_throttle * self.max_velocity
        raw_delta_vel = final_target_velocity - self.last_velocity_target

        is_decelerating = (torch.abs(final_target_velocity) < torch.abs(self.last_velocity_target)) | \
                          ((final_target_velocity * self.last_velocity_target) < 0)

        step_limits = torch.full_like(final_target_velocity, self.max_acceleration_step)
        step_limits[is_decelerating] = self.friction_coast_step

        delta_vel = torch.max(torch.min(raw_delta_vel, step_limits), -step_limits)

        current_velocity_command = self.last_velocity_target + delta_vel
        self.last_velocity_target = current_velocity_command

        self.command_buffer.append(current_velocity_command.clone())
        delayed_velocity = self.command_buffer[0]

        full_vel_targets = torch.zeros(self.num_dof, device=self.device)
        full_vel_targets[self.wheel_indexes] = delayed_velocity

        self.articulation.set_joint_velocity_targets(
            self._to_articulation(full_vel_targets.unsqueeze(0))
        )

        if debug:
            self._print_debug(target_throttle, delayed_velocity, step_limits)

    # ================================================================== #
    # 5. HELPERS INTERNOS
    # ================================================================== #

    def _as_torch(self, x) -> torch.Tensor:
        if isinstance(x, torch.Tensor):
            return x.to(self.device)
        return torch.as_tensor(x, device=self.device)

    def _to_articulation(self, x: torch.Tensor):
        return x.detach().cpu().numpy()

    def _get_wheel_velocities_raw(self) -> torch.Tensor:
        all_vel = self._as_torch(self.articulation.get_joint_velocities())
        if all_vel.dim() == 2:
            all_vel = all_vel[0]
        return all_vel[self.wheel_indexes]

    def _print_debug(self, target_throttle: torch.Tensor, delayed_velocity: torch.Tensor, limits: torch.Tensor) -> None:
        now = time.time()
        if now - self._debug_last_print >= 0.1:
            np_throttle = target_throttle.cpu().numpy().round(3)
            np_target_vel = delayed_velocity.cpu().numpy().round(2)
            np_limits = limits.cpu().numpy().round(1)
            np_actual_vel = self._get_wheel_velocities_raw().detach().cpu().numpy().round(2)

            # También imprimimos el Yaw para validar la nueva función
            yaw = float(self.get_heading_deg().cpu())

            print(f"\n[DEBUG] --- Análisis Dinámico y Odometría ---")
            print(f"1. Target Throttle     : {np_throttle}")
            print(f"2. Fricción Aplicada   : {np_limits} (rad/s por frame)")
            print(f"3. Command Velocity    : {np_target_vel} rad/s")
            print(f"4. Actual Simulated Vel: {np_actual_vel} rad/s")
            print(f"5. Robot Heading (Yaw) : {yaw:.2f}°")
            print("----------------------------------------------------------")
            self._debug_last_print = now