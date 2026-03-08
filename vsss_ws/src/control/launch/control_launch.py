from launch import LaunchDescription
from launch_ros.actions import Node
from launch.substitutions import LaunchConfiguration, PythonExpression
from launch.actions import DeclareLaunchArgument

def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument('robot_name', default_value='vsss_robot', description='Name of the robot'),
        Node(
            package='control',
            executable='control_node',
            namespace=LaunchConfiguration('robot_name'),
            name=PythonExpression(["'control_node_' + '", LaunchConfiguration('robot_name'), "'"]),
            parameters=[{'robot_name': LaunchConfiguration('robot_name')}]
        )
    ])