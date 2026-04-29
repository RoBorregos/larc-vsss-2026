import torch
import isaaclab.sim as sim_utils
from isaaclab.assets import RigidObject, RigidObjectCfg


class Robot:
    def __init__(self, prim_path, name, color=(0.0, 0.0, 1.0)):
        self.prim_path = prim_path
        self.name = name
        self.max_thrust = 10.0

        self.cfg = RigidObjectCfg(
            prim_path=self.prim_path,
            spawn=sim_utils.CuboidCfg(
                size=(0.075, 0.075, 0.075),
                rigid_props=sim_utils.RigidBodyPropertiesCfg(),
                mass_props=sim_utils.MassPropertiesCfg(mass=0.5),
                collision_props=sim_utils.CollisionPropertiesCfg(),
                visual_material=sim_utils.PreviewSurfaceCfg(diffuse_color=color)
            )
        )

        self.robot_asset = RigidObject(cfg=self.cfg)

    def apply_forward_thrust(self, desired_force_x):
        num_envs = self.robot_asset.num_instances

        force_tensor = torch.as_tensor(desired_force_x, dtype=torch.float32, device="cuda")

        forces = torch.zeros((num_envs, 1, 3), device="cuda")
        torques = torch.zeros((num_envs, 1, 3), device="cuda")

        forces[:, 0, 0] = torch.clamp(force_tensor, min=-self.max_thrust, max=self.max_thrust)

        self.robot_asset.permanent_wrench_composer.set_forces_and_torques(
            forces=forces,
            torques=torques
        )

        self.robot_asset.write_data_to_sim()