# from launch import LaunchDescription
# from launch.actions import DeclareLaunchArgument, ExecuteProcess, SetEnvironmentVariable
# from launch.substitutions import LaunchConfiguration
# from ament_index_python.packages import get_package_share_directory
# from launch.actions import IncludeLaunchDescription
# from launch.launch_description_sources import PythonLaunchDescriptionSource

# def generate_launch_description():

#     gz_model_path = SetEnvironmentVariable(
#         name='GAZEBO_MODEL_PATH',
#         value=[get_package_share_directory('carebot_simulation'),'/simulation/models']
#     )

#     gz_model_uri = SetEnvironmentVariable(
#         name='GAZEBO_MODEL_DATABASE_URI', 
#         value=''
#     )

#     world_path = DeclareLaunchArgument(
#         name='world_path', 
#         default_value=[get_package_share_directory('carebot_simulation'), '/simulation/worlds/FEI.world'], 
#         description=''
#     )

#     gzserver = IncludeLaunchDescription(
#             PythonLaunchDescriptionSource([get_package_share_directory('ros_gz_sim'), '/launch/gzserver.launch.py']),
#             launch_arguments = {
#                 'verbose': 'false',
#                 'world': LaunchConfiguration('world_path')
#             }.items()
#         )

#     gzclient = IncludeLaunchDescription(
#             PythonLaunchDescriptionSource([get_package_share_directory('ros_gz_sim'), '/launch/gzclient.launch.py']),
#             # condition=IfCondition(LaunchConfiguration('simulation_gui'))
#         )

#     ld = LaunchDescription()
#     ld.add_action(gz_model_path)
#     # ld.add_action(gz_model_uri)
#     ld.add_action(world_path)
#     ld.add_action(gzserver)
#     ld.add_action(gzclient)

#     return ld

import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, SetEnvironmentVariable
from launch.substitutions import LaunchConfiguration
from ament_index_python.packages import get_package_share_directory
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node

def generate_launch_description():

    gz_model_path = SetEnvironmentVariable(
        name='GZ_SIM_RESOURCE_PATH',
        value=[get_package_share_directory('carebot_simulation'), '/simulation/models']
    )

    world_path = DeclareLaunchArgument(
        name='world_path',
        default_value=[get_package_share_directory('carebot_simulation'), '/simulation/worlds/FEI.world'],
        description=''
    )

    gz_sim = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            get_package_share_directory('ros_gz_sim'), '/launch/gz_sim.launch.py'
        ]),
        launch_arguments={
            'gz_args': [LaunchConfiguration('world_path'), ' -r'],
            'on_exit_shutdown': 'true'
        }.items()
    )

    # Bridge do clock do Gazebo para o ROS2
    clock_bridge = Node(
        package='ros_gz_bridge',
        executable='parameter_bridge',
        name='clock_bridge',
        arguments=['/clock@rosgraph_msgs/msg/Clock[gz.msgs.Clock'],
        output='screen'
    )

    # Bridge do LiDAR Gazebo -> ROS2
    lidar_bridge = Node(
        package='ros_gz_bridge',
        executable='parameter_bridge',
        name='lidar_bridge',
        arguments=['/scan@sensor_msgs/msg/LaserScan[gz.msgs.LaserScan'],
        parameters=[{
            'lazy': False,
            'subscription_qos': 'sensor_data',
        }],
        output='screen'
    )  
    
    # Nó para corrigir o frame_id do scan
    scan_frame_fix = Node(
        package='tf2_ros',
        executable='static_transform_publisher',
        name='scan_frame_fix',
        arguments=['0', '0', '0', '0', '0', '0', 'lidar_link', 'robot_diff/base_footprint/head_hokuyo_sensor'],
        output='screen'
    )

    gz_resource_path = SetEnvironmentVariable(
        name='GZ_SIM_RESOURCE_PATH',
        value=[
            get_package_share_directory('carebot_simulation') + '/simulation/models',
            ':',
            get_package_share_directory('carebot_description')
        ]
    )  

    ld = LaunchDescription()
    ld.add_action(gz_model_path)
    ld.add_action(world_path)
    ld.add_action(gz_sim)
    ld.add_action(clock_bridge)
    ld.add_action(lidar_bridge)
    ld.add_action(scan_frame_fix)
    ld.add_action(gz_resource_path)
    return ld