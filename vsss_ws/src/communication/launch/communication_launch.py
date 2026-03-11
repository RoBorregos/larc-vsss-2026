from launch import LaunchDescription
from launch_ros.actions import Node
from launch.substitutions import PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration

def generate_launch_description():
    
    pkg_name = 'communication'
    pkg_share = FindPackageShare(pkg_name).find(pkg_name)
    params = PathJoinSubstitution([pkg_share, 'config', 'communication_config.yaml'])

    return LaunchDescription([
        DeclareLaunchArgument('robot_id', default_value='1', description='ID of the robot'),
        Node(
            package='communication',
            executable='communication_node',
            namespace=['robot_', LaunchConfiguration('robot_id')],
            name=['communication_node_', LaunchConfiguration('robot_id')],
            remappings=[
                ('cmd_vel', ['/strategy/robot_', LaunchConfiguration('robot_id'), '/cmd_vel']),
                ('stop', ['/strategy/robot_', LaunchConfiguration('robot_id'), '/stop']),
                ('kicker', ['/strategy/robot_', LaunchConfiguration('robot_id'), '/kicker']),
            ],
            parameters=[params]
        )
    ])