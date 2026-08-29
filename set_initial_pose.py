import rclpy
from rclpy.node import Node
from rclpy.parameter import Parameter
from geometry_msgs.msg import PoseWithCovarianceStamped
import math

class InitialPoseSetter(Node):
    def __init__(self):
        super().__init__('initial_pose_setter')
        self.set_parameters([Parameter('use_sim_time', Parameter.Type.BOOL, True)])
        self.pub = self.create_publisher(PoseWithCovarianceStamped, '/initialpose', 10)
        self.timer = self.create_timer(2.0, self.publish_pose)
        self.count = 0

    def publish_pose(self):
        if self.count >= 5:
            rclpy.shutdown()
            return
        yaw = 1.07  # radianos
        msg = PoseWithCovarianceStamped()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.header.frame_id = 'map'
        msg.pose.pose.position.x = 14.781
        msg.pose.pose.position.y = 5.713
        msg.pose.pose.position.z = 0.0
        msg.pose.pose.orientation.x = 0.0
        msg.pose.pose.orientation.y = 0.0
        msg.pose.pose.orientation.z = math.sin(yaw / 2.0)
        msg.pose.pose.orientation.w = math.cos(yaw / 2.0)
        msg.pose.covariance[0]  = 0.25
        msg.pose.covariance[7]  = 0.25
        msg.pose.covariance[35] = 0.068
        self.pub.publish(msg)
        self.get_logger().info(f'stamp={msg.header.stamp.sec} z={msg.pose.pose.orientation.z:.4f} w={msg.pose.pose.orientation.w:.4f}')
        self.count += 1

def main():
    rclpy.init()
    node = InitialPoseSetter()
    rclpy.spin(node)

if __name__ == '__main__':
    main()
