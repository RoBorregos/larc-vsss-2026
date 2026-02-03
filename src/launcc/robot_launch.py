import os
import launch
from launch import LaunchDescription
from ament_index_python.packages import get_package_share_directory
from webots_ros2_driver.webots_launcher import WebotsLauncher
from webots_ros2_driver.webots_controller import WebotsController
from webots_ros2_driver.wait_for_controller_connection import WaitForControllerConnection
from launch_ros.actions import Node
from launch.substitutions import LaunchConfiguration


def generate_launch_description():

    use_rviz = LaunchConfiguration('rviz', default=False)

    package_dir = get_package_share_directory('omni_drive_sim')
    robot_description_path = os.path.join(package_dir, 'resource', 'omni_robot.urdf')

    with open(robot_description_path, 'r') as f:
        robot_description = f.read()

    # ---------- WEBOTS ----------
    webots = WebotsLauncher(
        world=os.path.join(package_dir, 'worlds', 'Simulacion2.wbt')
    )

    # ---------- ROBOT DRIVER ----------
    robot_driver = WebotsController(
        robot_name='robot', 
        parameters=[
            {'robot_description': robot_description},
        ]
    )

    # ---------- RVIZ ----------
    rviz = Node(
        package='rviz2',
        executable='rviz2',
        output='screen',
        arguments=[
            '-d', os.path.join(package_dir, 'resource', 'rviz_config.rviz')
        ],
        condition=launch.conditions.IfCondition(use_rviz)
    )

    # ---------- TF ----------
    footprint_publisher = Node(
        package='tf2_ros',
        executable='static_transform_publisher',
        arguments=['0', '0', '0.075', '0', '0', '0', 'base_footprint', 'base_link'],
    )

    robot_state_publisher = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        parameters=[{'robot_description': robot_description}],
    )

    # ---------- ODOMETRÍA ----------
    odometry_node = Node(
        package='omni_drive_sim',
        executable='odometry_publisher',
    )

    waiting_nodes = WaitForControllerConnection(
        target_driver=robot_driver,
        nodes_to_start=[
            rviz,
            robot_state_publisher,
            odometry_node,
        ]
    )

    return LaunchDescription([
        webots,
        robot_driver,
        footprint_publisher,
        waiting_nodes
    ])
