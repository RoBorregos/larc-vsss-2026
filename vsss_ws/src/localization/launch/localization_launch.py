from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='localization',
            executable='localization_node',
            namespace='robot1',
            name='localization_node_robot1',
            parameters=[
                {"frame_prefix": "robot1"}
            ]
        ),
        Node(
            package='localization',
            executable='localization_node',
            namespace='robot2',
            name='localization_node_robot2',
            parameters=[
                {"frame_prefix": "robot2"}
            ]
        ),
        Node(
            package='localization',
            executable='localization_node',
            namespace='robot3',
            name='localization_node_robot3',
            parameters=[
                {"frame_prefix": "robot3"}
            ]
        )
    ])