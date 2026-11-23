import math
import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Vector3
from std_msgs.msg import Bool
from sensor_msgs.msg import Image
from tf2_ros import TransformException, Buffer, TransformListener
from cv_bridge import CvBridge
from vsss_vision.srv import Prediction

from .constants import PhysicalProperties, RosProperties
from . import settings

_BALL_ID = PhysicalProperties.Ball.ID
_ROS = RosProperties


class RosHandler(Node):
    def __init__(self, infield_objects):
        super().__init__(_ROS.NODE_NAME)

        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, self)
        self.infield_objects = infield_objects

        self.current_poses: dict[str, dict[str, float]] = {}
        self.publishers_map: dict = {}
        self.bridge = CvBridge()

        self.image_pub = self.create_publisher(Image, "/vision/sim_image", 10)
        self.prediction_client = self.create_client(Prediction, "/vision/prediction")
        self.create_timer(1.0 / settings.vision.pose_update_hz, self._update_poses_callback)

        while not self.prediction_client.wait_for_service(timeout_sec=2.0):
            self.get_logger().info("Waiting for prediction server")

    def _update_poses_callback(self) -> None:
        for obj in self.infield_objects:
            frame_id = _ROS.BALL_FRAME if obj["id"] == _BALL_ID else f"robot_{obj['id']}"
            pose = self._fetch_transform(frame_id)
            if pose:
                self.current_poses[frame_id] = pose
            else:
                self.current_poses.pop(frame_id, None)

    def _fetch_transform(self, frame_name: str) -> dict[str, float] | None:
        try:
            trans = self.tf_buffer.lookup_transform(
                _ROS.FIELD_FRAME,
                frame_name,
                rclpy.time.Time(),
                timeout=rclpy.duration.Duration(seconds=0.0),
            )
            return {
                "x": trans.transform.translation.x * _ROS.M_TO_MM,
                "y": trans.transform.translation.y * _ROS.M_TO_MM,
                "theta": self._quaternion_to_yaw(trans.transform.rotation),
            }
        except TransformException:
            return None

    def get_pose(self, frame_name: str) -> dict[str, float] | None:
        return self.current_poses.get(frame_name)

    @staticmethod
    def _quaternion_to_yaw(q) -> float:
        siny_cosp = 2.0 * (q.w * q.z + q.x * q.y)
        cosy_cosp = 1.0 - 2.0 * (q.y * q.y + q.z * q.z)
        return math.atan2(siny_cosp, cosy_cosp)

    def _get_or_create_pub(self, topic_name: str, msg_type):
        if topic_name not in self.publishers_map:
            self.publishers_map[topic_name] = self.create_publisher(msg_type, topic_name, 10)
        return self.publishers_map[topic_name]

    def publish_image(self, cv_image) -> None:
        msg = self.bridge.cv2_to_imgmsg(cv_image, encoding="bgr8")
        self.image_pub.publish(msg)

    def publish_omni_move(self, robot_id: int, speed: float, angle: float, facing_error: float = 0.0) -> None:
        pub = self._get_or_create_pub(f"/strategy/robot_{robot_id}/motion_control", Vector3)
        msg = Vector3()
        msg.x = float(speed)
        msg.y = float(angle)
        msg.z = float(facing_error)
        pub.publish(msg)

    def publish_kicker(self, robot_id: int, active: bool) -> None:
        pub = self._get_or_create_pub(f"/strategy/robot_{robot_id}/kicker", Bool)
        msg = Bool()
        msg.data = bool(active)
        pub.publish(msg)

    def publish_stop(self, robot_id: int, value: bool) -> None:
        pub = self._get_or_create_pub(f"/strategy/robot_{robot_id}/stop", Bool)
        msg = Bool()
        msg.data = bool(value)
        pub.publish(msg)

    def get_prediction(self, target_id: int, seconds_future: float):
        request = Prediction.Request()
        request.id = int(target_id)
        request.seconds_in_future = float(seconds_future)

        future = self.prediction_client.call_async(request)
        future.add_done_callback(self._prediction_callback)
        return future

    def _prediction_callback(self, future) -> None:
        try:
            future.result()
        except Exception as e:
            self.get_logger().error(f"Failed to call service: {e}")