from typing import Final

class PhysicalProperties:
    class Ball:
        ID: Final[int] = 20
        MASS_G: Final[float] = 48.0
        RADIUS_MM: Final[float] = 21.35
        RESTITUTION: Final[float] = 0.7
        FRICTION: Final[float] = 0.2

    class Robot:
        MASS_G: Final[float] = 250.0
        APOTHEM_MM: Final[float] = 37.5
        RESTITUTION: Final[float] = 0.2
        FRICTION: Final[float] = 0.4
        KICK_IMPULSE: Final[float] = 0.3
        KICK_ANGLE_TOLERANCE_RAD: Final[float] = 0.2

        BLUE_TEAM_MIN_ID: Final[int] = 10
        ARUCO_DATABASE: Final[dict[int, int]] = {
            0: 955,
            1: 771,
            2: 939,

            10: 256,
            11: 272,
            12: 273,

            20: -1,
        }

    class Field:
        WIDTH_MM: Final[float] = 1500.0
        HEIGHT_MM: Final[float] = 1300.0
        CORNER_LENGTH_MM: Final[float] = 70.0
        GOAL_WIDTH_MM: Final[float] = 400.0
        GOAL_DEPTH_MM: Final[float] = 100.0




class VisualProperties:
    class Display:
        SCALE_PX_PER_MM: Final[float] = 0.5
        LEFT_PADDING_PX: Final[int] = 50
        UPPER_PADDING_PX: Final[int] = 50

    class Ball:
        COLOR: Final[str] = "orange"
        OUTLINE_COLOR: Final[str] = "black"
        OUTLINE_WIDTH_PX: Final[int] = 1
        TAG: Final[str] = "ball"

    class Robot:
        TEAM_BLUE_COLOR: Final[str] = "#0000FF"
        TEAM_YELLOW_COLOR: Final[str] = "#FFFF00"
        BODY_COLOR: Final[str] = "white"
        OUTLINE_COLOR: Final[str] = "black"

        ARUCO_SIZE_RATIO: Final[float] = 0.875
        ARUCO_MARKER_PX: Final[int] = 200

        ARROW_LENGTH_PX: Final[int] = 50
        ARROW_COLOR: Final[str] = "#00FF00"
        ARROW_WIDTH_PX: Final[int] = 3

        ROLE_TEXT_OFFSET_PX: Final[int] = 10
        ROLE_TEXT_COLOR: Final[str] = "white"
        ROLE_FONT: Final[tuple[str, int, str]] = ("Arial", 10, "bold")


    class Field:
        BG_COLOR: Final[str] = "#000000"
        LINE_COLOR: Final[str] = "white"
        LINE_WIDTH_PX: Final[int] = 2

        CENTER_CIRCLE_RADIUS_MM: Final[float] = 200.0
        AREA_DEPTH_MM: Final[float] = 150.0
        AREA_HEIGHT_MM: Final[float] = 700.0
        MARK_CIRCLE_RADIUS_MM: Final[float] = 10.0
        MARK_CROSS_SIZE_MM: Final[float] = 50.0

        FREE_BALL_CIRCLES_MM: Final[tuple[tuple[float, float], ...]] = (
            (160, 1050), (160, 250), (560, 1050), (560, 250),
            (1340, 250), (1340, 1050), (940, 1050), (940, 250),
        )
        FREE_BALL_CROSSES_MM: Final[tuple[tuple[float, float], ...]] = (
            (380, 1050), (380, 650), (380, 250),
            (1120, 1050), (1120, 650), (1120, 250),
        )

class RosProperties:
    NODE_NAME: Final[str] = "vsss_strategy_node"
    FIELD_FRAME: Final[str] = "field"
    BALL_FRAME: Final[str] = "ball"
    M_TO_MM: Final[float] = 1000.0