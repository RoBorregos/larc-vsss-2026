import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist, Vector3
from std_msgs.msg import Bool
import math
from tf2_ros import TransformException, Buffer, TransformListener
from sensor_msgs.msg import Image
from cv_bridge import CvBridge
from vsss_vision.srv import Prediction
from . import constants


class RosHandler(Node):
    def __init__(self, infield_objects):
        super().__init__('vsss_strategy_node')

        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, self)
        self.infield_objects = infield_objects

        self.current_poses = {}
        self.publishers_map = {}
        self.bridge = CvBridge()
        self.image_pub = self.create_publisher(Image, '/vision/sim_image', 10)
        self.prediction_client = self.create_client(Prediction, '/vision/prediction')
        self.create_timer(1.0 / 30.0, self._update_poses_callback)

        while not self.prediction_client.wait_for_service(timeout_sec=2.0):
            self.get_logger().info('Waiting for prediction server')

    def _update_poses_callback(self):
        for obj in self.infield_objects:
            frame_id = f"robot_{obj['id']}" if obj['id'] != 20 else "ball"
            pose = self._fetch_transform(frame_id)
            if pose:
                self.current_poses[frame_id] = pose
            else:
                self.current_poses.pop(frame_id, None)

    def _fetch_transform(self, frame_name):
        try:
            trans = self.tf_buffer.lookup_transform(
                'field',
                frame_name,
                rclpy.time.Time(),
                timeout=rclpy.duration.Duration(seconds=0.0)
            )

            return {
                'x': trans.transform.translation.x,
                'y': trans.transform.translation.y,
                'theta': self._quaternion_to_yaw(trans.transform.rotation)
            }
        except TransformException as e:
            # print("err", e)
            return None

    def get_pose(self, frame_name):
        return self.current_poses.get(frame_name)

    def _quaternion_to_yaw(self, q):
        siny_cosp = 2 * (q.w * q.z + q.x * q.y)
        cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z)
        return math.atan2(siny_cosp, cosy_cosp)

    def publish_image(self, cv_image):
        msg = self.bridge.cv2_to_imgmsg(cv_image, encoding="bgr8")
        self.image_pub.publish(msg)

    def publish_omni_move(self, robot_id, speed, angle, facing_error):
        topic_name = f'/strategy/robot_{robot_id}/motion_control'

        if topic_name not in self.publishers_map:
            self.publishers_map[topic_name] = self.create_publisher(Vector3, topic_name, 10)

        msg = Vector3()
        msg.x = float(speed)
        msg.y = float(angle)
        msg.z = float(0)  # ¡Incluso puedes meter el error de orientación!

        self.publishers_map[topic_name].publish(msg)

    def publish_kicker(self, robot_id, active):
        topic_name = f'/strategy/robot_{robot_id}/kicker'
        if topic_name not in self.publishers_map:
            self.publishers_map[topic_name] = self.create_publisher(Bool, topic_name, 10)

        msg = Bool()
        msg.data = active
        self.publishers_map[topic_name].publish(msg)

    def publish_stop(self, robot_id, value):
        topic_name = f'/strategy/robot_{robot_id}/stop'
        if topic_name not in self.publishers_map:
            self.publishers_map[topic_name] = self.create_publisher(Bool, topic_name, 10)

        msg = Bool()
        msg.data = value
        self.publishers_map[topic_name].publish(msg)

    def get_prediction(self, target_id, seconds_future):
        request = Prediction.Request()
        request.id = int(target_id)
        request.seconds_in_future = float(seconds_future)

        future = self.prediction_client.call_async(request)

        future.add_done_callback(self._prediction_callback)
        return future

    def _prediction_callback(self, future):
        try:
            response = future.result()
        except Exception as e:
            self.get_logger().error(f"Failed to call service: {e}")