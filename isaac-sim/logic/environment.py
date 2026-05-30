# import torch
# import math
# import gymnasium
#
# from isaaclab.envs import DirectRLEnv, DirectRLEnvCfg
# from isaaclab.sim import SimulationCfg
# from isaaclab.scene import InteractiveSceneCfg
# from isaaclab.utils import configclass
#
#
# ######################################
# #    CONFIG - Read sizes/timing      #
# ######################################
#
# @configclass
# class EnvironmentCfg(DirectRLEnvCfg):
#     decimation = 4
#     episode_length_s = 10.0
#     sim: SimulationCfg = SimulationCfg()
#
#     observation_space = 0
#     action_space = 0
#     state_space = 0
#
#     scene: InteractiveSceneCfg = InteractiveSceneCfg(
#         num_envs=4096, env_spacing=3.0, replicate_physics=True,
#     )
#
#
# ######################################
# #    ENV - Rewards system here       #
# ######################################
# class Environment(DirectRLEnv):
#     cfg: EnvironmentCfg
#
#     def __init__(
#             self,
#             cfg: DirectRLEnvCfg,
#             render_mode: str | None = None,
#     ):
#         super().__init__(cfg, render_mode)
#
#
#     def _setup_scene(self):
#         pass
#
#     def _pre_physics_step(self, actions: torch.Tensor):
#         pass
#
#     def _apply_action(self):
#         pass
#
#     def _get_observations(self) -> dict:
#         pass
#
#     def _get_rewards(self) -> torch.Tensor:
#         pass
#
#     def _get_dones(self) -> tuple[torch.Tensor, torch.Tensor]:
#         pass
#
#     def _reset_idx(self, env_ids: torch.Tensor | None):
#         super()._reset_idx(env_ids)
