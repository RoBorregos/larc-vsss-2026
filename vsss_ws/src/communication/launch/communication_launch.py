from launch import LaunchDescription
from launch_ros.actions import Node
from launch.substitutions import PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PythonExpression
import os

def generate_launch_description():
    
    pkg_name = 'communication'
    pkg_share = FindPackageShare(pkg_name).find(pkg_name)
    params = PathJoinSubstitution([pkg_share, 'config', 'communication_config.yaml'])

    return LaunchDescription([
        DeclareLaunchArgument('robot_name', default_value='vsss_robot', description='Name of the robot'),
        Node(
            package='communication',
            executable='communication_node',
            namespace=LaunchConfiguration('robot_name'),
            name=PythonExpression(["'communication_node_' + '", LaunchConfiguration('robot_name'), "'"]),
            parameters=[params]
        )
    ])