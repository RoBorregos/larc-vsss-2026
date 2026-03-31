from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='vsss_vision',
            executable='vision_node',
            name='vision_node',
            output='screen',
            emulate_tty=True,
            parameters=[{
                'use_camera': True,
                'simulator': False,
                'record_video': False,
                'debug_mode': True,
                'camera_id': '/dev/vsss_cam',
                'file_path': '/ros2_ws/vsss/vsss_ws/src/vision/media/log/raw_2026-02-24_04-18-37.avi'
            }]
        )
    ])