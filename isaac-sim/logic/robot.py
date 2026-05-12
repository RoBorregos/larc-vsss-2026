import torch
from isaacsim.core.api import World
from isaacsim.core.utils.stage import add_reference_to_stage
from isaacsim.core.prims import Articulation
from isaacsim.core.utils.prims import get_prim_at_path
from pxr import UsdGeom, Gf
from collections import deque
from typing import Callable


class Robot:
    """Wrapper for the Omni robot.

    Single-robot debug version. For parallel-env training (thousands of
    robots), port this to Isaac Lab's torch-native Articulation class.
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

        # Motor model state (configured later by configure_motors).
        self._stall_torque_sampler: Callable[[], torch.Tensor] | None = None
        self._no_load_speed_sampler: Callable[[], torch.Tensor] | None = None
        self._latency_steps_sampler: Callable[[], int] | None = None
        self._bearing_friction_sampler: Callable[[], torch.Tensor] | None = None
        self._encoder_noise_std_sampler: Callable[[], torch.Tensor] | None = None

        self.stall_torque: float = float("inf")
        self.no_load_speed: float = float("inf")
        self.latency_steps: int = 0
        self.bearing_friction: float = 0.0
        self.encoder_noise_std: float = 0.0
        self.command_buffer: deque[torch.Tensor] | None = None

    # ------------------------------------------------------------------ #
    # Lifecycle: spawn -> world.reset() -> initialize -> configure_motors
    # ------------------------------------------------------------------ #

    def spawn(self) -> None:
        """Add the robot USD at prim_path to the stage."""
        add_reference_to_stage(
            usd_path=self.usd_path,
            prim_path=self.prim_path,
        )

        prim = get_prim_at_path(self.prim_path)
        xform = UsdGeom.Xformable(prim)
        xform.ClearXformOpOrder()

        translate_op = xform.AddTranslateOp()
        translate_op.Set(Gf.Vec3d(*self.spawn_position))

        rotate_op = xform.AddRotateXYZOp()
        rotate_op.Set(Gf.Vec3f(*self.spawn_orientation_euler_deg))

        self.articulation = Articulation(
            prim_paths_expr=self.prim_path,
            name=self.name,
        )

    def initialize(self) -> None:
        """Finalize articulation handle and cache joint indexes.

        Must be called AFTER world.reset() so the physics scene exists.
        """
        assert self.articulation is not None, "Call spawn() before initialize()."

        self.articulation.initialize()

        self.num_dof = self.articulation.num_dof
        self.dof_names = list(self.articulation.dof_names)

        wheel_idx = [i for i, n in enumerate(self.dof_names) if n.startswith("wheel_")]
        roller_idx = [i for i, n in enumerate(self.dof_names) if n.startswith("roller_")]

        self.wheel_indexes = torch.tensor(wheel_idx, dtype=torch.long, device=self.device)
        self.roller_indexes = torch.tensor(roller_idx, dtype=torch.long, device=self.device)

        print(f"[{self.name}] Initialized: {self.num_dof} DOFs "
              f"({len(wheel_idx)} wheels, {len(roller_idx)} rollers)")


    # ------------------------------------------------------------------ #
    # Internal helpers for the numpy-backed articulation API
    # ------------------------------------------------------------------ #

    def _as_torch(self, x) -> torch.Tensor:
        """Coerce a value returned from the articulation API into a torch
        tensor on self.device. Handles numpy arrays and CPU torch tensors."""
        if isinstance(x, torch.Tensor):
            return x.to(self.device)
        return torch.as_tensor(x, device=self.device)

    def _to_articulation(self, x: torch.Tensor):
        """Convert a torch tensor into whatever the articulation API expects.

        Current Isaac Sim version is numpy-backed for this articulation, so we
        copy to CPU and convert to numpy. If a future version supports torch
        natively, this becomes a no-op.
        """
        return x.detach().cpu().numpy()

    def _get_wheel_velocities_raw(self) -> torch.Tensor:
        """Read true wheel velocities as a (4,) torch tensor on self.device."""
        all_vel = self._as_torch(self.articulation.get_joint_velocities())
        # API may return (num_dof,) or (1, num_dof) -- normalize to flat.
        if all_vel.dim() == 2:
            all_vel = all_vel[0]
        return all_vel[self.wheel_indexes]

    # ------------------------------------------------------------------ #
    # Low-level: raw effort on the 4 wheel joints
    # ------------------------------------------------------------------ #

    def apply_wheel_efforts(self, efforts: torch.Tensor) -> None:
        """Apply torques (Nm) to the 4 wheel drive joints.

        Args:
            efforts: shape (4,), one torque per wheel.
        """
        assert self.articulation is not None, "Call spawn() before apply_wheel_efforts()."
        assert efforts.shape == (4,), f"Expected shape (4,), got {efforts.shape}"

        full_efforts = torch.zeros(self.num_dof, device=self.device)
        full_efforts[self.wheel_indexes] = efforts.to(self.device)

        self.articulation.set_joint_efforts(
            self._to_articulation(full_efforts.unsqueeze(0))
        )

    # ------------------------------------------------------------------ #
    # Motor model: throttle in [-1, +1] -> torque, with optional realism
    # ------------------------------------------------------------------ #

    def configure_motors(
            self,
            stall_torque_sampler: Callable[[], torch.Tensor] | None = None,
            no_load_speed_sampler: Callable[[], torch.Tensor] | None = None,
            latency_steps_sampler: Callable[[], int] | None = None,
            bearing_friction_sampler: Callable[[], torch.Tensor] | None = None,
            encoder_noise_std_sampler: Callable[[], torch.Tensor] | None = None,
    ) -> None:
        """Configure the motor model.

        Each parameter is a callable that returns a sample. Pass None for any
        parameter to use the "perfect physics" default for that effect.
        Call resample_motor_params() to redraw all parameters.
        """
        self._stall_torque_sampler = stall_torque_sampler
        self._no_load_speed_sampler = no_load_speed_sampler
        self._latency_steps_sampler = latency_steps_sampler
        self._bearing_friction_sampler = bearing_friction_sampler
        self._encoder_noise_std_sampler = encoder_noise_std_sampler

        self.resample_motor_params()

    def configure_drive_modes(self, wheel_damping: float = 0.0) -> None:
        """Zero out stiffness. Keep rollers strictly passive, add slight damping to wheels.

        - Roller joints: passive (kps=0, kds=0) to spin freely.
        - Wheel joints: kps=0 (effort control), small kds to absorb instantaneous
          reaction torque and prevent chassis lurch without locking the wheels.
        """
        assert self.articulation is not None, "Call initialize() first."

        # Build zero-gain arrays for all DOFs.
        kps = torch.zeros(self.num_dof, device=self.device)
        kds = torch.zeros(self.num_dof, device=self.device)

        # Apply slight damping ONLY to the drive wheels
        kds[self.wheel_indexes] = wheel_damping

        # Apply to all joints.
        self.articulation.set_gains(
            kps=self._to_articulation(kps.unsqueeze(0)),
            kds=self._to_articulation(kds.unsqueeze(0)),
        )
        print(f"[{self.name}] Drive modes configured. Wheel damping: {wheel_damping}")

    def resample_motor_params(self) -> None:
        """Draw fresh values from each sampler. Call to randomize the motor
        model (e.g., at the start of each RL episode for domain randomization).
        """
        INF = float("inf")

        self.stall_torque = (
            self._stall_torque_sampler() if self._stall_torque_sampler else INF
        )
        self.no_load_speed = (
            self._no_load_speed_sampler() if self._no_load_speed_sampler else INF
        )
        self.latency_steps = (
            self._latency_steps_sampler() if self._latency_steps_sampler else 0
        )
        self.bearing_friction = (
            self._bearing_friction_sampler() if self._bearing_friction_sampler else 0.0
        )
        self.encoder_noise_std = (
            self._encoder_noise_std_sampler() if self._encoder_noise_std_sampler else 0.0
        )

        zero_cmd = torch.zeros(4, device=self.device)
        self.command_buffer = deque(
            [zero_cmd.clone() for _ in range(self.latency_steps + 1)],
            maxlen=self.latency_steps + 1,
        )

    def get_observed_wheel_velocities(self) -> torch.Tensor:
        """Wheel velocities as the robot's control logic would see them
        (with simulated encoder noise applied)."""
        assert self.articulation is not None
        wheel_vel = self._get_wheel_velocities_raw()

        if self.encoder_noise_std > 0:
            noise = torch.randn_like(wheel_vel) * self.encoder_noise_std
            wheel_vel = wheel_vel + noise

        return wheel_vel

    def apply_motor_command(self, throttle: torch.Tensor) -> None:
        """Drive the 4 wheels using the motor model.

        Args:
            throttle: shape (4,), values in [-1, +1]. With perfect-physics
                defaults, throttle acts as a direct torque scaling (1 Nm per
                unit throttle).
        """
        assert self.articulation is not None
        assert self.command_buffer is not None, (
            "Call configure_motors() before apply_motor_command()."
        )
        throttle = throttle.to(self.device).clamp(-1.0, 1.0)

        # Apply control latency: motor sees a command from N steps ago.
        self.command_buffer.append(throttle.clone())
        delayed_throttle = self.command_buffer[0]

        # True wheel velocities for the motor's internal physics
        # (encoder noise belongs to get_observed_wheel_velocities, not here).
        wheel_vel = self._get_wheel_velocities_raw()

        if self.stall_torque == float("inf") or self.no_load_speed == float("inf"):
            max_torque_nm = 0.05
            motor_torque = delayed_throttle * max_torque_nm
        else:
            # Linear torque-speed curve, scaled by throttle.
            motor_torque = (
                    delayed_throttle * self.stall_torque
                    - (self.stall_torque / self.no_load_speed) * wheel_vel
            )
            motor_torque = motor_torque.clamp(-self.stall_torque, self.stall_torque)

        if self.bearing_friction > 0:
            friction = -torch.sign(wheel_vel) * self.bearing_friction
            motor_torque = motor_torque + friction

        self.apply_wheel_efforts(motor_torque)