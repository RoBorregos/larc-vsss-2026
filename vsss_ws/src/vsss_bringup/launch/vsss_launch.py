from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.substitutions import PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare

def generate_launch_description():
    communication_dir = PathJoinSubstitution([FindPackageShare('communication'), 'launch'])
    control_dir = PathJoinSubstitution([FindPackageShare('control'), 'launch'])
    return LaunchDescription([
        IncludeLaunchDescription(
            PathJoinSubstitution([communication_dir, 'communication_launch.py'])
        ),
        IncludeLaunchDescription(
            PathJoinSubstitution([control_dir, 'control_launch.py'])
        ),
        IncludeLaunchDescription(
            PathJoinSubstitution([FindPackageShare('localization'), 'launch', 'localization_launch.py'])
        )
    ])