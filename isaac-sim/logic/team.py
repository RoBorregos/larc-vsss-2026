from typing import Literal
import torch

from robot_fleet import RobotFleet
from constants import config


class Team:
    """
    Vista lógica de los 3 robots de un color dentro de UN env.

    Ya no posee robots. Apunta a un slice de la RobotFleet global de su color
    (la que abarca todos los envs). Sirve para razonar a nivel de cancha:
    rewards, scoring, asignaciones de roles, etc.

    El layout de la fleet sigue esta convención (definida en main.py):
        índice global = env_idx * ROBOTS_PER_TEAM + robot_idx
    donde ROBOTS_PER_TEAM = 3.
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
        self.fleet = fleet  # se asigna desde main después de crear las fleets

        # Índices (en la fleet global) de los 3 robots de este equipo en este env
        start = env_idx * self.ROBOTS_PER_TEAM
        self.robot_indices = torch.arange(
            start, start + self.ROBOTS_PER_TEAM,
            dtype=torch.long,
            device=config["robot"]["device"],
        )

    # ============================================================== #
    # HELPERS ESTÁTICOS para que main.py arme la fleet
    # ============================================================== #
    @classmethod
    def build_prim_paths(cls, env_path: str, team_color: str) -> list[str]:
        """Rutas USD de los 3 robots de este equipo en este env."""
        return [f"{env_path}/team/{team_color}/robot_{i}" for i in range(cls.ROBOTS_PER_TEAM)]

    @classmethod
    def build_spawn_positions(cls, team_color: str) -> list[tuple[float, float, float]]:
        """Posiciones de spawn locales (relativas al env)."""
        return [
            config["spawn_positions"][team_color][f"robot_{i}"]
            for i in range(cls.ROBOTS_PER_TEAM)
        ]

    # ============================================================== #
    # VISTA: acceso a los datos de la fleet filtrados por este equipo/env
    # ============================================================== #
    def attach_fleet(self, fleet: RobotFleet):
        """Vincula este Team a la fleet de su color (creada en main)."""
        self.fleet = fleet

    def get_positions(self) -> torch.Tensor:
        """Posiciones (3, 3) de los robots de este equipo en este env."""
        pos, _ = self.fleet.get_world_pose()
        return pos[self.robot_indices]

    def get_headings_deg(self) -> torch.Tensor:
        """Yaw (3,) de los robots de este equipo en este env."""
        return self.fleet.get_heading_deg()[self.robot_indices]

    def get_wheel_velocities(self) -> torch.Tensor:
        """Velocidades de rueda (3, 4) de este equipo en este env."""
        return self.fleet.get_observed_wheel_velocities()[self.robot_indices]