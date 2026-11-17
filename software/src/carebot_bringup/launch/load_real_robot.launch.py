from ament_index_python.packages import get_package_share_path, get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, RegisterEventHandler, TimerAction, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.substitutions import Command, LaunchConfiguration, PathJoinSubstitution
from launch.event_handlers import OnProcessStart
from launch_ros.actions import Node
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.parameter_descriptions import ParameterValue
import os


def generate_launch_description():

    declare_namespace_cmd = DeclareLaunchArgument(
        'namespace',
        default_value='',
        description='Top-level namespace')

    urdf_path = get_package_share_path('carebot_description')
    rviz_path = get_package_share_path('carebot_simulation')

    default_model_path = urdf_path / 'urdf/robot_diff.xacro'
    default_rviz_config_path = rviz_path / 'config/rviz/robot.rviz'

    gui_arg = DeclareLaunchArgument(name='gui', default_value='false', choices=['true', 'false'],
                                    description='Flag to enable joint_state_publisher_gui')
    model_arg = DeclareLaunchArgument(name='model', default_value=str(default_model_path),
                                      description='Absolute path to robot urdf file')
    rviz_arg = DeclareLaunchArgument(name='rvizconfig', default_value=str(default_rviz_config_path),
                                     description='Absolute path to rviz config file')
    use_sim_time_arg = DeclareLaunchArgument(name='use_sim_time', default_value='false',
                                             description='Flag to enable use_sim_time')
    teleop_joy_arg = DeclareLaunchArgument(name='teleop', default_value='false',
                                           description='Flag to enable the teleoperation')
    log_level = DeclareLaunchArgument(
        name='log_level', 
        default_value='INFO', 
        choices=['DEBUG', 'INFO', 'WARN', 'ERROR', 'FATAL'],
        description='Flag to set log level'
    )

    robot_description = ParameterValue(
        Command(['xacro ', LaunchConfiguration('model'), ' sim_mode:=', LaunchConfiguration('use_sim_time')]),
        value_type=str
    )

    robot_state_publisher_node = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        output='both',
        parameters=[{
            'use_sim_time': LaunchConfiguration('use_sim_time'), 
            'robot_description': robot_description
        }],
        arguments=['--ros-args', '--log-level', LaunchConfiguration('log_level')]
    )

    controller_params_file = os.path.join(
        get_package_share_directory('omnidrive_stm32'),
        'config',
        'diff_drive_controller.yaml'
    )

    controller_manager = Node(
        package="controller_manager",
        executable="ros2_control_node",
        parameters=[{'robot_description': robot_description},
                    controller_params_file],
        output="both",
    )

    start_controller_after_rsp = RegisterEventHandler(
        OnProcessStart(
            target_action=robot_state_publisher_node,
            on_start=[TimerAction(period=2.0, actions=[controller_manager])]
        )
    )

    diff_drive_spawner = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["diff_drive_controller"],
    )

    diff_drive_event_handler = RegisterEventHandler(
        event_handler=OnProcessStart(
            target_action=controller_manager,
            on_start=[diff_drive_spawner]
        )
    )

    joint_state_broadcaster_spawner = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["joint_state_broadcaster"],
    )

    joint_state_broadcaster_event_handler = RegisterEventHandler(
        event_handler=OnProcessStart(
            target_action=controller_manager,
            on_start=[joint_state_broadcaster_spawner]
        )
    )

    twist_relay = Node(
        package='carebot_bringup',
        executable='twist_to_stamped',
        name='twist_to_stamped',
        output='screen'
    )

    robot_localization_node = Node(
        package='robot_localization',
        executable='ekf_node',
        name='ekf_node',
        output='screen',
        parameters=[
            os.path.join(get_package_share_path('carebot_description'), 'config/ros2_control/ekf.yaml'),
            {'use_sim_time': LaunchConfiguration('use_sim_time')}
        ]
    )

    rplidar_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(
                get_package_share_directory('rplidar_ros'),
                'launch',
                'rplidar_c1_launch.py'
            )
        )
    )

    return LaunchDescription([
        declare_namespace_cmd,
        log_level,
        gui_arg,
        model_arg,
        rviz_arg,
        use_sim_time_arg,
        teleop_joy_arg,
        robot_state_publisher_node,
        start_controller_after_rsp,
        diff_drive_event_handler,
        joint_state_broadcaster_event_handler,
        twist_relay,
        robot_localization_node,
        # rplidar_launch, # Descomente quando o LiDAR estiver conectado na USB
    ])