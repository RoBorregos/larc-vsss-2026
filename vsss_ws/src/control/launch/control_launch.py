from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='control',
            executable='control_node',
            namespace='robot1',
            name='control_node_robot1',
            parameters=[{"frame_prefix": "robot1"}]
        ),
        Node(
            package='control',
            executable='control_node',
            namespace='robot2',
            name='control_node_robot2',
            parameters=[{"frame_prefix": "robot2"}]
        ),
        Node(
            package='control',
            executable='control_node',
            namespace='robot3',
            name='control_node_robot3',
            parameters=[{"frame_prefix": "robot3"}]
        )
    ])