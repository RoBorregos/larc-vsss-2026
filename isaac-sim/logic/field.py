import numpy as np
from isaacsim.core.api.objects import FixedCuboid


class Field:
    def __init__(self, parent_path: str, name: str = "VSSS_Field"):
        self.parent_path = parent_path
        self.name = name
        self.field_path = f"{self.parent_path}/{self.name}"

        # Dimensiones físicas totales (incluyendo porterías)
        self.length = 1.70  # 150cm campo + 20cm porterías
        self.width = 1.30
        self.wall_height = 0.10  # 10 cm de alto
        self.wall_thickness = 0.02

        self.build_field()

    def build_field(self):
        """Construye el piso y las paredes."""
        self._spawn_ground_plane()
        self._spawn_invisible_colliders()

    def _spawn_ground_plane(self):
        # Piso sólido negro
        floor_thickness = 0.01
        z_center = 0.005  # Elevado para evitar parpadeo con el suelo base

        self.floor = FixedCuboid(
            prim_path=f"{self.field_path}/Floor",
            name="Field_Floor",
            scale=np.array([self.length, self.width, floor_thickness]),
            translation=np.array([0, 0, z_center]),
            visible=True,
            color=np.array([0.05, 0.05, 0.05])  # Negro mate
        )

    def _spawn_invisible_colliders(self):
        """Muros visibles de 10cm de altura."""
        z_center = self.wall_height / 2.0
        q_45 = np.array([0.9238795, 0.0, 0.0, 0.3826834])
        q_minus45 = np.array([0.9238795, 0.0, 0.0, -0.3826834])
        corner_length = 0.10

        walls = {
            "wall_top": {
                "scale": np.array([1.54, self.wall_thickness, self.wall_height]),
                "translation": np.array([0, 0.66, z_center])
            },
            "wall_bottom": {
                "scale": np.array([1.54, self.wall_thickness, self.wall_height]),
                "translation": np.array([0, -0.66, z_center])
            },
            "wall_left_top": {
                "scale": np.array([self.wall_thickness, 0.45, self.wall_height]),
                "translation": np.array([-0.76, 0.425, z_center])
            },
            "wall_left_bottom": {
                "scale": np.array([self.wall_thickness, 0.45, self.wall_height]),
                "translation": np.array([-0.76, -0.425, z_center])
            },
            "wall_right_top": {
                "scale": np.array([self.wall_thickness, 0.45, self.wall_height]),
                "translation": np.array([0.76, 0.425, z_center])
            },
            "wall_right_bottom": {
                "scale": np.array([self.wall_thickness, 0.45, self.wall_height]),
                "translation": np.array([0.76, -0.425, z_center])
            },
            "goal_left_back": {
                "scale": np.array([self.wall_thickness, 0.44, self.wall_height]),
                "translation": np.array([-0.86, 0, z_center])
            },
            "goal_right_back": {
                "scale": np.array([self.wall_thickness, 0.44, self.wall_height]),
                "translation": np.array([0.86, 0, z_center])
            },
            "goal_left_top_side": {"scale": np.array([0.10, self.wall_thickness, self.wall_height]),
                                   "translation": np.array([-0.81, 0.21, z_center])},
            "goal_left_bottom_side": {"scale": np.array([0.10, self.wall_thickness, self.wall_height]),
                                      "translation": np.array([-0.81, -0.21, z_center])},
            "goal_right_top_side": {"scale": np.array([0.10, self.wall_thickness, self.wall_height]),
                                    "translation": np.array([0.81, 0.21, z_center])},
            "goal_right_bottom_side": {"scale": np.array([0.10, self.wall_thickness, self.wall_height]),
                                       "translation": np.array([0.81, -0.21, z_center])},

            "corner_tl": {"scale": np.array([corner_length, self.wall_thickness, self.wall_height]),
                          "translation": np.array([-0.715, 0.615, z_center]), "orientation": q_45},
            "corner_tr": {"scale": np.array([corner_length, self.wall_thickness, self.wall_height]),
                          "translation": np.array([0.715, 0.615, z_center]), "orientation": q_minus45},
            "corner_bl": {"scale": np.array([corner_length, self.wall_thickness, self.wall_height]),
                          "translation": np.array([-0.715, -0.615, z_center]), "orientation": q_minus45},
            "corner_br": {"scale": np.array([corner_length, self.wall_thickness, self.wall_height]),
                          "translation": np.array([0.715, -0.615, z_center]), "orientation": q_45},
        }

        for name, params in walls.items():
            FixedCuboid(
                prim_path=f"{self.field_path}/Colliders/{name}",
                name=name,
                scale=params["scale"],
                translation=params["translation"],
                orientation=params.get("orientation", np.array([1.0, 0.0, 0.0, 0.0])),
                visible=True,  # Paredes visibles
                color=np.array([0.6, 0.1, 0.1])  # Rojo oscuro
            )