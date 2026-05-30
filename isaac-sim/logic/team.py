from typing import Literal
import torch

from robot_fleet import RobotFleet
from constants import config


class Team:
    """
    Logic view of the 3 robots in the same env from the same teams

    This class associates the robots to the view, this will help at env level when
    rewards, scoring, roles and assignations are implemented.

    The layout is given by this formula

    GlobalIndex = env_idx * ROBOTS_PER_TEAM + robot_idx
    where ROBOTS_PER_TEAM = 3.
    """

    ROBOTS_PER_TEAM = 3

    def __init__(
        self,
        team_color: Literal["blue", "yellow"] = "blue",
        attacking: Literal["left", "right"] = "left",
        env_path: str = "",
        env_idx: int = 0,
        fleet: RobotFleet | None = None,
    ):
        self.team_color = team_color
        self.attacking = attacking
        self.env_path = env_path
        self.env_idx = env_idx
        self.fleet = fleet

        start = env_idx * self.ROBOTS_PER_TEAM
        self.robot_indices = torch.arange(
            start, start + self.ROBOTS_PER_TEAM,
            dtype=torch.long,
            device=config["robot"]["device"],
        )

    # ============================================================== #
    # Static helpers to build the fleet
    # ============================================================== #
    @classmethod
    def build_prim_paths(cls, env_path: str, team_color: str) -> list[str]:
        """Store the paths to the 3 robots."""
        return [f"{env_path}/team/{team_color}/robot_{i}" for i in range(cls.ROBOTS_PER_TEAM)]

    @classmethod
    def build_spawn_positions(cls, team_color: str) -> list[tuple[float, float, float]]:
        """Spawn positions relative to the environment."""
        return [
            config["spawn_positions"][team_color][f"robot_{i}"]
            for i in range(cls.ROBOTS_PER_TEAM)
        ]

    # ============================================================== #
    # VIEWS: Access to the values
    # ============================================================== #
    def attach_fleet(self, fleet: RobotFleet):
        """Link the team to a fleet."""
        self.fleet = fleet

    def get_positions(self) -> torch.Tensor:
        """Get the positions (3, 3) of the robots in the team."""
        pos, _ = self.fleet.get_world_pose()
        return pos[self.robot_indices]

    def get_headings_deg(self) -> torch.Tensor:
        """Yaw (3,) of the robots in the team."""
        return self.fleet.get_heading_deg()[self.robot_indices]

    def get_wheel_velocities(self) -> torch.Tensor:
        """Speed of each wheel (3, 4) of the robots in the team."""
        return self.fleet.get_observed_wheel_velocities()[self.robot_indices]