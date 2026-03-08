from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument 
from launch.substitutions import PathJoinSubstitution, LaunchConfiguration
from launch_ros.substitutions import FindPackageShare

def generate_launch_description():
    
    communication_dir = PathJoinSubstitution([FindPackageShare('communication'), 'launch'])
    control_dir = PathJoinSubstitution([FindPackageShare('control'), 'launch'])
    localization_dir = PathJoinSubstitution([FindPackageShare('localization'), 'launch'])
    
    return LaunchDescription([
        DeclareLaunchArgument('robot_name', default_value='vsss_robot', description='Name of the robot'),
        IncludeLaunchDescription(
            PathJoinSubstitution([communication_dir, 'communication_launch.py']),
            launch_arguments={
                'robot_name': LaunchConfiguration('robot_name')
            }.items()
        ),
        IncludeLaunchDescription(
            PathJoinSubstitution([control_dir, 'control_launch.py']),
            launch_arguments={
                'robot_name': LaunchConfiguration('robot_name')
            }.items()
        ),
        IncludeLaunchDescription(
            PathJoinSubstitution([localization_dir, 'localization_launch.py']),
            launch_arguments={
                'robot_name': LaunchConfiguration('robot_name')
            }.items()
        )
    ])