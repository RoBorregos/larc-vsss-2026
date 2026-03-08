from launch import LaunchDescription
from launch_ros.actions import Node
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, PythonExpression
from launch_ros.substitutions import FindPackageShare
from launch.actions import DeclareLaunchArgument

def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument('robot_name', default_value='vsss_robot', description='Name of the robot'),
        Node(
            package='localization',
            executable='localization_node',
            namespace=LaunchConfiguration('robot_name'),
            name=PythonExpression(["'localization_node_' + '", LaunchConfiguration('robot_name'), "'"]),
            parameters=[{'robot_name': LaunchConfiguration('robot_name')}]
        )
    ])