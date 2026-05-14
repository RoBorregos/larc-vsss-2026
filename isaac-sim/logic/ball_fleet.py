import torch

from isaacsim.core.prims import RigidPrim
from isaacsim.core.utils.prims import get_prim_at_path
from pxr import UsdGeom, UsdPhysics, PhysxSchema, Sdf, Gf

from logic.constants import config


class BallFleet:
    """
    Una pelota por env, controlada como UN solo RigidPrim view vectorizado.

    Mismo patrón que RobotFleet:
      - `spawn()` crea N esferas (una por env) en el stage.
      - `initialize()` (tras world.reset()) arma la view.
      - Las queries devuelven tensores (N, ...).
    """

    def __init__(
        self,
        prim_paths: list[str],
        prim_paths_expr: str,
        spawn_positions: list[tuple[float, float, float]],
        name: str = "ball_fleet",
        radius: float = 0.021,
        mass: float = 0.045,
        color: tuple[float, float, float] = (1.0, 0.5, 0.0),
        device: str = config["robot"]["device"],
    ):
        assert len(prim_paths) == len(spawn_positions)
        self.prim_paths = list(prim_paths)
        self.prim_paths_expr = prim_paths_expr
        self.spawn_positions = list(spawn_positions)
        self.name = name
        self.radius = radius
        self.mass = mass
        self.color = color
        self.device = device

        self.N = len(prim_paths)
        self.view: RigidPrim | None = None

    def spawn(self):
        import omni.usd
        import time
        stage = omni.usd.get_context().get_stage()

        total_balls = len(self.prim_paths)
        start_time = time.time()

        for idx, (prim_path, pos) in enumerate(zip(self.prim_paths, self.spawn_positions)):
            sphere = UsdGeom.Sphere.Define(stage, Sdf.Path(prim_path))
            sphere.CreateRadiusAttr(self.radius)
            sphere.CreateDisplayColorAttr().Set([Gf.Vec3f(*self.color)])

            xform = UsdGeom.Xformable(sphere.GetPrim())
            xform.ClearXformOpOrder()
            xform.AddTranslateOp().Set(Gf.Vec3d(*pos))

            prim = get_prim_at_path(prim_path)
            UsdPhysics.RigidBodyAPI.Apply(prim)
            UsdPhysics.CollisionAPI.Apply(prim)
            mass_api = UsdPhysics.MassAPI.Apply(prim)
            mass_api.CreateMassAttr(self.mass)
            PhysxSchema.PhysxRigidBodyAPI.Apply(prim)

            # --- Progress and ETA ---
            current = idx + 1
            elapsed_time = time.time() - start_time
            porcentaje = (current / total_balls) * 100.0
            time_per_ball = elapsed_time / current
            eta_seconds = time_per_ball * (total_balls - current)
            eta_mins, eta_secs = divmod(int(eta_seconds), 60)

            print(
                f"\r[INFO] {self.name} Spawning: {current}/{total_balls} [{porcentaje:.1f}%] - ETA: {eta_mins:02d}:{eta_secs:02d} - {prim_path}" + " " * 15,
                end='', flush=True)

        total_time = time.time() - start_time
        tot_mins, tot_secs = divmod(int(total_time), 60)
        print(f"\n[INFO] {self.name} Fleet spawned completado en {tot_mins:02d}:{tot_secs:02d}.")

        self.view = RigidPrim(prim_paths_expr=self.prim_paths_expr, name=self.name)

    def initialize(self):
        self.view.initialize()
        assert self.view.count == self.N, (
            f"BallFleet view tiene {self.view.count} prims, esperaba {self.N}."
        )

    # --- queries vectorizadas (N, ...) ---
    def get_positions(self) -> torch.Tensor:
        pos, _ = self.view.get_world_poses()
        return torch.as_tensor(pos, device=self.device).view(self.N, 3)

    def get_pose(self) -> tuple[torch.Tensor, torch.Tensor]:
        pos, quat = self.view.get_world_poses()
        return (
            torch.as_tensor(pos, device=self.device).view(self.N, 3),
            torch.as_tensor(quat, device=self.device).view(self.N, 4),
        )

    def get_velocities(self) -> torch.Tensor:
        """Linear + angular velocities. Shape: (N, 6)."""
        lin = self.view.get_linear_velocities()
        ang = self.view.get_angular_velocities()
        lin_t = torch.as_tensor(lin, device=self.device).view(self.N, 3)
        ang_t = torch.as_tensor(ang, device=self.device).view(self.N, 3)
        return torch.cat([lin_t, ang_t], dim=1)