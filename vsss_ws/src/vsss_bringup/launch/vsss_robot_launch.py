from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument 
from launch.substitutions import PathJoinSubstitution, LaunchConfiguration
from launch_ros.substitutions import FindPackageShare

def generate_launch_description():
    communication_dir = PathJoinSubstitution([FindPackageShare('communication'), 'launch'])
    
    return LaunchDescription([
        DeclareLaunchArgument('robot_id', default_value='0', description='ID of the robot'),
        IncludeLaunchDescription(
            PathJoinSubstitution([communication_dir, 'communication_launch.py']),
            launch_arguments={
                'robot_id': LaunchConfiguration('robot_id'),
            }.items()
        )
    ])