import torch
import math
import time
from collections import deque
from typing import Callable

from isaacsim.core.utils.stage import add_reference_to_stage
from isaacsim.core.prims import Articulation
from isaacsim.core.utils.prims import get_prim_at_path
from pxr import Usd, UsdGeom, Gf, UsdShade, Sdf

from logic.constants import config


class RobotFleet:
    """
    Vectorización de N robots idénticos bajo una sola Articulation view.

    - `prim_paths`: lista de rutas USD donde se referencía el robot (uno por instancia).
    - `prim_paths_expr`: regex que matchea los N prims; lo usa Articulation para construir
      un view vectorizado y operar sobre los N robots en una sola llamada GPU.
    - `spawn_positions`: lista de (x, y, z) en mismo orden que `prim_paths`.

    Todos los tensores internos tienen shape (N, ...).
    """

    def __init__(
        self,
        usd_path: str,
        prim_paths: list[str],
        prim_paths_expr: str,
        team_color: str = "blue",
        name: str = "robot_fleet",
        device: str = config["robot"]["device"],
        spawn_orientation_euler_deg: tuple[float, float, float] = config["robot"]["spawn_orientation_euler_deg"],
        spawn_positions: list[tuple[float, float, float]] | None = None,
        headless: bool = False,
    ):
        assert len(prim_paths) > 0, "RobotFleet needs at least 1 prim_path"
        if spawn_positions is None:
            spawn_positions = [(0.0, 0.0, 0.0)] * len(prim_paths)
        assert len(spawn_positions) == len(prim_paths), "spawn_positions debe tener mismo largo que prim_paths"

        self.usd_path = usd_path
        self.prim_paths = list(prim_paths)
        self.prim_paths_expr = prim_paths_expr
        self.spawn_positions = list(spawn_positions)
        self.name = name
        self.device = device
        self.spawn_orientation_euler_deg = spawn_orientation_euler_deg

        self.N = len(prim_paths)  # número de robots en la flota
        self.rgb_color = config["colors"]["yellow"] if team_color.lower() == "yellow" else config["colors"]["blue"]

        # --- Estado de física (se llenan en initialize()) ---
        self.articulation: Articulation | None = None
        self.num_dof = 0
        self.wheel_indexes: torch.Tensor | None = None  # índices de DOF de ruedas

        # --- Parámetros de motores/sensores ---
        self.max_velocity = config["motors"]["max_velocity"]
        self.max_acceleration_step = config["motors"]["max_acceleration_step"]
        self.friction_coast_step = config["motors"]["friction_coast_step"]
        self.latency_steps = config["sensors"]["default_latency_steps"]
        self.encoder_noise_std = config["sensors"]["default_encoder_noise_std"]
        self._samplers: dict[str, Callable] = {}

        # --- Buffers de comandos: TODOS (N, 4) ---
        self.command_buffer: deque[torch.Tensor] | None = None
        self.last_velocity_target = torch.zeros(self.N, 4, device=self.device)
        self.current_throttle = torch.zeros(self.N, 4, device=self.device)

        # --- Material del equipo (se crea en spawn()) ---
        self._team_material: UsdShade.Material | None = None
        self.headless = headless

        self._debug_last_print = 0.0

    def spawn(self):
        if not self.headless:
            self._team_material = self._create_team_material()

        total_robots = len(self.prim_paths)
        start_time = time.time()

        for idx, (prim_path, spawn_pos) in enumerate(zip(self.prim_paths, self.spawn_positions)):
            add_reference_to_stage(usd_path=self.usd_path, prim_path=prim_path)
            prim = get_prim_at_path(prim_path)
            prim.Load(Usd.LoadWithDescendants)

            if not self.headless:
                self._uninstance_visuals(prim)

            xform = UsdGeom.Xformable(prim)
            xform.ClearXformOpOrder()
            xform.AddTranslateOp().Set(Gf.Vec3d(*spawn_pos))
            xform.AddRotateXYZOp().Set(Gf.Vec3f(*self.spawn_orientation_euler_deg))

            if not self.headless:
                self._bind_team_material_recursive(prim, self._team_material)

            # --- Progress and ETA ---
            current = idx + 1
            elapsed_time = time.time() - start_time
            porcentaje = (current / total_robots) * 100.0
            time_per_robot = elapsed_time / current
            eta_seconds = time_per_robot * (total_robots - current)
            eta_mins, eta_secs = divmod(int(eta_seconds), 60)

            print(
                f"\r[INFO] {self.name} Spawning: {current}/{total_robots} [{porcentaje:.1f}%] - ETA: {eta_mins:02d}:{eta_secs:02d} - {prim_path}" + " " * 15,
                end='', flush=True)

        total_time = time.time() - start_time
        tot_mins, tot_secs = divmod(int(total_time), 60)
        print(f"\n[INFO] {self.name} Fleet spawned completado en {tot_mins:02d}:{tot_secs:02d}.")

        self.articulation = Articulation(prim_paths_expr=self.prim_paths_expr, name=self.name)

    def _uninstance_visuals(self, root_prim):
        """Marca como NO instanceable cualquier Xform 'visuals' (y descendientes
        instanceable). Esto sustituye el prototype compartido por una copia local
        por robot, permitiendo bindear materiales distintos por equipo.

        Costo: cada robot ahora tiene su propia geometría visual en RAM/USD.
        Para 36 robots con meshes de un VSSS pequeño esto es despreciable.
        """
        for child in Usd.PrimRange(root_prim, Usd.PrimAllPrimsPredicate):
            if child.IsInstanceable():
                child.SetInstanceable(False)

    def paint(self):
        """Re-aplica el material de equipo a todos los meshes de la flota.

        Llámalo DESPUÉS de world.reset() para asegurarte que la composición USD
        terminó y todos los meshes existen. Es idempotente: bindear dos veces
        el mismo material no hace daño.
        """
        if self.headless:
            return

        if self._team_material is None:
            self._team_material = self._create_team_material()
        for prim_path in self.prim_paths:
            prim = get_prim_at_path(prim_path)
            if prim and prim.IsValid():
                self._bind_team_material_recursive(prim, self._team_material)

    def _create_team_material(self) -> UsdShade.Material:
        """Crea (o reusa) un material OmniPBR con el color del equipo en /World/Looks.

        Usamos OmniPBR (MDL) porque tu URDF llega con materiales MDL bindeados,
        y la única forma garantizada de sobrescribirlos es con otro MDL.
        Si el material ya existe (segundo spawn, reuse), simplemente lo retorna.
        """
        import omni.usd
        stage = omni.usd.get_context().get_stage()

        # Asegurar el Scope /World/Looks
        looks_scope_path = "/World/Looks"
        if not stage.GetPrimAtPath(looks_scope_path):
            UsdGeom.Scope.Define(stage, looks_scope_path)

        mat_path = f"{looks_scope_path}/TeamMaterial_{self.name}"
        existing = stage.GetPrimAtPath(mat_path)
        if existing and existing.IsValid():
            return UsdShade.Material(existing)

        # Crear material + shader OmniPBR (MDL)
        material = UsdShade.Material.Define(stage, mat_path)
        shader = UsdShade.Shader.Define(stage, f"{mat_path}/Shader")
        shader.CreateImplementationSourceAttr(UsdShade.Tokens.sourceAsset)
        shader.SetSourceAsset(Sdf.AssetPath("OmniPBR.mdl"), "mdl")
        shader.SetSourceAssetSubIdentifier("OmniPBR", "mdl")

        # Inputs: color difuso + un poco menos metálico/rugoso para que se vea sólido
        r, g, b = self.rgb_color
        shader.CreateInput("diffuse_color_constant", Sdf.ValueTypeNames.Color3f).Set(Gf.Vec3f(r, g, b))
        shader.CreateInput("metallic_constant", Sdf.ValueTypeNames.Float).Set(0.0)
        shader.CreateInput("reflection_roughness_constant", Sdf.ValueTypeNames.Float).Set(0.6)

        # Conectar shader -> material output (MDL surface)
        material.CreateSurfaceOutput("mdl").ConnectToSource(shader.ConnectableAPI(), "out")
        material.CreateDisplacementOutput("mdl").ConnectToSource(shader.ConnectableAPI(), "out")
        material.CreateVolumeOutput("mdl").ConnectToSource(shader.ConnectableAPI(), "out")

        return material

    def _bind_team_material_recursive(self, root_prim, material: UsdShade.Material):
        """Para cada Mesh descendiente: desbindar lo que trae el URDF y bindear
        el material del equipo con `strongerThanDescendants` para que gane.
        """
        for child in Usd.PrimRange(root_prim):
            name = child.GetName()

            # Saltar colliders (invisibles, no necesitan material visual)
            if name.startswith("collisions"):
                continue

            if not child.IsA(UsdGeom.Gprim):
                continue

            binding_api = UsdShade.MaterialBindingAPI.Apply(child)
            binding_api.UnbindAllBindings()
            binding_api.Bind(
                material,
                bindingStrength=UsdShade.Tokens.strongerThanDescendants,
            )

    # ============================================================== #
    # INITIALIZE: hay que llamarlo DESPUÉS de world.reset()
    # ============================================================== #
    def initialize(self):
        self.articulation.initialize()
        self.num_dof = self.articulation.num_dof

        # Sanity check: el view debe contener exactamente N robots
        assert self.articulation.count == self.N, (
            f"Articulation view tiene {self.articulation.count} prims, esperaba {self.N}. "
            f"Revisa que prim_paths_expr='{self.prim_paths_expr}' matchee todos los prim_paths."
        )

        dof_names = list(self.articulation.dof_names)
        self.wheel_indexes = torch.tensor(
            [i for i, n in enumerate(dof_names) if n.startswith("wheel_")],
            dtype=torch.long,
            device=self.device,
        )

    # ============================================================== #
    # CONFIG DE DRIVES Y MOTORES
    # ============================================================== #
    def configure_drive_modes(self, wheel_damping: float = 0.1):
        # set_gains acepta (count, num_dof) — un set de gains por robot.
        kps = torch.zeros(self.N, self.num_dof, device=self.device)
        kds = torch.zeros(self.N, self.num_dof, device=self.device)
        kds[:, self.wheel_indexes] = wheel_damping
        self.articulation.set_gains(kps=kps.contiguous(), kds=kds.contiguous())

    def configure_motors(self, **samplers: Callable):
        self._samplers = samplers
        self.resample_motor_params()

    def resample_motor_params(self):
        self.max_velocity = self._samplers.get("max_velocity", lambda: 57.6)()
        self.max_acceleration_step = self._samplers.get("max_acceleration", lambda: 1.5)()
        self.latency_steps = self._samplers.get("latency_steps", lambda: 0)()
        self.encoder_noise_std = self._samplers.get("encoder_noise_std", lambda: 0.0)()

        # Buffer de latencia: cada slot es un tensor (N, 4)
        self.command_buffer = deque(
            [torch.zeros(self.N, 4, device=self.device) for _ in range(self.latency_steps + 1)],
            maxlen=self.latency_steps + 1,
        )

    # ============================================================== #
    # OBSERVACIONES (TODAS shape (N, ...))
    # ============================================================== #
    def get_observed_wheel_velocities(self) -> torch.Tensor:
        """Velocidades de las 4 ruedas por robot. Shape: (N, 4)."""
        vel = torch.as_tensor(self.articulation.get_joint_velocities(), device=self.device).view(self.N, -1)
        vel = vel[:, self.wheel_indexes]
        if self.encoder_noise_std > 0:
            vel = vel + torch.randn_like(vel) * self.encoder_noise_std
        return vel

    def get_world_pose(self) -> tuple[torch.Tensor, torch.Tensor]:
        """Posiciones (N, 3) y quaterniones wxyz (N, 4) de los N robots."""
        pos, quat = self.articulation.get_world_poses()
        return (
            torch.as_tensor(pos, device=self.device).view(self.N, 3),
            torch.as_tensor(quat, device=self.device).view(self.N, 4),
        )

    def get_heading_rad(self) -> torch.Tensor:
        """Yaw (rotación alrededor de Z) por robot. Shape: (N,)."""
        _, quat = self.get_world_pose()
        w, x, y, z = quat[:, 0], quat[:, 1], quat[:, 2], quat[:, 3]
        return torch.atan2(2 * (w * z + x * y), 1 - 2 * (y * y + z * z))

    def get_heading_deg(self) -> torch.Tensor:
        return self.get_heading_rad() * (180.0 / math.pi)

    # ============================================================== #
    # CONTROL — acepta escalar (mismo comando a todos) o tensor (N,)
    # ============================================================== #
    def move_omnidirectional(
        self,
        speed: float | torch.Tensor,
        direction_deg: float | torch.Tensor,
    ):
        """
        Calcula throttle para las 4 ruedas omnidireccionales.

        Cada parámetro puede ser:
          - escalar -> mismo comando para los N robots
          - tensor shape (N,) -> comando por robot
        """
        # Normalizar a tensores (N, 1) para broadcast con la "base" por rueda
        speed_t = self._as_per_robot(speed)              # (N, 1)
        rad = self._as_per_robot(direction_deg) * (math.pi / 180.0)  # (N, 1)

        # Base por rueda (1, 4) — los 4 desfases fijos del omni
        offsets = torch.tensor(
            [0.0, math.pi / 2, math.pi, 3 * math.pi / 2],
            device=self.device,
        ).view(1, 4)
        signs = torch.tensor([-1.0, -1.0, 1.0, -1.0], device=self.device).view(1, 4)

        # raw_throttle: (N, 4)
        raw_throttle = signs * torch.sin(rad + offsets) * speed_t

        # Normalización por robot: si el máximo abs > 1, escalamos a speed
        max_val = raw_throttle.abs().amax(dim=1, keepdim=True).clamp(min=1e-8)  # (N, 1)
        raw_throttle = raw_throttle * (speed_t / max_val)

        self.current_throttle = raw_throttle  # (N, 4)

    def _as_per_robot(self, v) -> torch.Tensor:
        """Convierte escalar o tensor (N,) en un tensor (N, 1)."""
        if isinstance(v, torch.Tensor):
            return v.to(self.device).view(self.N, 1)
        return torch.full((self.N, 1), float(v), device=self.device)

    def commit_motor_commands(self, debug: bool = False):
        target_vel = self.current_throttle.clamp(-1.0, 1.0) * self.max_velocity  # (N, 4)
        delta_vel = target_vel - self.last_velocity_target

        is_decel = (target_vel.abs() < self.last_velocity_target.abs()) | (
            (target_vel * self.last_velocity_target) < 0
        )
        limits = torch.where(
            is_decel,
            torch.tensor(self.friction_coast_step, device=self.device),
            torch.tensor(self.max_acceleration_step, device=self.device),
        )

        self.last_velocity_target = self.last_velocity_target + delta_vel.clamp(-limits, limits)
        self.command_buffer.append(self.last_velocity_target.clone())

        # Tensor completo de DOFs (N, num_dof), las ruedas reciben el comando, el resto 0
        full_targets = torch.zeros(self.N, self.num_dof, device=self.device)
        full_targets[:, self.wheel_indexes] = self.command_buffer[0]
        self.articulation.set_joint_velocity_targets(full_targets.contiguous())

        if debug:
            self._print_debug(target_vel, limits)

    # ============================================================== #
    # DEBUG
    # ============================================================== #
    def _print_debug(self, target_vel: torch.Tensor, limits: torch.Tensor):
        now = time.time()
        if now - self._debug_last_print < config["debug"]["print_interval_sec"]:
            return
        print(f"\n[DEBUG {self.name}] (mostrando robot 0 de {self.N})")
        print(f"Target Vel  : {target_vel[0].cpu().numpy().round(2)} rad/s")
        print(f"Limits      : {limits[0].cpu().numpy().round(1)}")
        print(f"Command Vel : {self.command_buffer[0][0].cpu().numpy().round(2)} rad/s")
        print(f"Actual Vel  : {self.get_observed_wheel_velocities()[0].cpu().numpy().round(2)} rad/s")
        print(f"Yaw         : {float(self.get_heading_deg()[0]):.2f}°")
        self._debug_last_print = now