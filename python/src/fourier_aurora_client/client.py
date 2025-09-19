# -*- coding: utf-8 -*-
import time
from typing import Dict
from dataclasses import dataclass
from .qos_profile import PublisherQosProfile, SubscriberQosProfile
from .dds_interface import DDSInterface
import fourier_msgs.msg.AuroraCmd as AuroraCmd
import fourier_msgs.msg.MotionControlCmd as MotionControlCmd
import fourier_msgs.msg.MotorCfgCmd as MotorCfgCmd
import fourier_msgs.msg.MotionControlState as MotionControlState
import fourier_msgs.msg.BaseData as BaseData
import fourier_msgs.msg.TrueData as TrueData
import fourier_msgs.msg.ContactData as ContactData
import fourier_msgs.msg.AuroraState as AuroraState
from fourier_aurora_client.aurora_state_map import WholeBodyFsmState, UpperBodyFsmState, VelocityCommandSource


class AuroraClient:
    _instance = None

    def __new__(cls, domain_id, participant_qos, robot_name, serial_number):
        if not cls._instance:
            cls._instance = super(AuroraClient, cls).__new__(cls)
            cls._instance._initialize(domain_id, participant_qos, robot_name, serial_number)
            
        return cls._instance
    
    def __del__(self):
        self.close()
    
    def _initialize(self, domain_id, participant_qos, robot_name, serial_number):
        print("Client not initialized, Initializing Aurora Client...")
        self._dds_interface = DDSInterface(domain_id, participant_qos)
        self._serial_number = serial_number
        self._robot_name = robot_name
        self.topic_prefix = ""
        if self._serial_number:
            self.topic_prefix = self._robot_name + "/" + self._serial_number + "/"
            print("Topic prefix set to: " + self.topic_prefix)
        self._init_subscribers()
        self._init_publishers()
        print("Aurora Client initialized successfully with domain id: " + str(domain_id))

    def _init_subscribers(self):
        print("Creating subscribers...")
        self_subscriber_list = []

        # subscriber for aurora state
        self._aurora_state = {}
        def aurora_state_callback(self, data):
            self._aurora_state['whole_body_fsm_state'] = WholeBodyFsmState(data.whole_body_fsm_state()) 
            self._aurora_state['upper_body_fsm_state'] = UpperBodyFsmState(data.upper_body_fsm_state())
            self._aurora_state['velocity_command_source'] = VelocityCommandSource(data.velocity_command_source())
            self._aurora_state['allow_upper_body_override'] = data.allow_upper_body_override()

        self._aurora_state_subscriber = self._dds_interface.create_subscriber(
                self.topic_prefix + "aurora_state", 
                AuroraState.AuroraStatePubSubType(), 
                AuroraState.AuroraState(), 
                lambda data: aurora_state_callback(self, data), 
                SubscriberQosProfile.BEST_EFFORT)
        
        # subscriber for aurora state
        self._stand_pose_state = [0]*4
        def stand_pose_callback(self, data):
            self._stand_pose_state[0] = float(data.delta_z())
            self._stand_pose_state[1] = float(data.delta_pitch())
            self._stand_pose_state[2] = float(data.delta_yaw())
            self._stand_pose_state[3] = int(data.stable_level())

        self._stand_pose_subscriber = self._dds_interface.create_subscriber(
                self.topic_prefix + "robot_stand_pose_state", 
                AuroraState.RobotStandPoseStatePubSubType(), 
                AuroraState.RobotStandPoseState(), 
                lambda data: stand_pose_callback(self, data), 
                SubscriberQosProfile.BEST_EFFORT)

        # subscriber for robot control group state
        self._robot_control_group_state = {}
        def robot_control_group_state_callback(self, data):
            for control_group_state in data.group_state():
                tmp_group_state = {}
                tmp_group_state['group_name'] = control_group_state.group_name()
                tmp_group_state['position'] = list(control_group_state.joint_position())
                tmp_group_state['velocity'] = list(control_group_state.joint_velocity())
                tmp_group_state['effort'] = list(control_group_state.joint_effort())
                self._robot_control_group_state[tmp_group_state['group_name']] = tmp_group_state

        self._robot_control_group_state_subscriber = self._dds_interface.create_subscriber(
                self.topic_prefix + "robot_control_group_state", 
                MotionControlState.RobotControlGroupStatePubSubType(), 
                MotionControlState.RobotControlGroupState(), 
                lambda data: robot_control_group_state_callback(self, data), 
                SubscriberQosProfile.BEST_EFFORT)   
        self_subscriber_list.append(self._robot_control_group_state_subscriber)

        # subscriber for robot cartesian state
        self._robot_cartesian_state = {}
        def robot_cartesian_state_callback(self, data):
            for cartesian_state in data.cartesian_state():
                tmp_cartesian_state = {}
                tmp_cartesian_state['group_name'] = cartesian_state.group_name()
                tmp_cartesian_state['pose'] = list(cartesian_state.pose())
                tmp_cartesian_state['twist'] = list(cartesian_state.twist())
                tmp_cartesian_state['wrench'] = list(cartesian_state.wrench())
                self._robot_cartesian_state[tmp_cartesian_state['group_name']] = tmp_cartesian_state

        self._robot_cartesian_state_subscriber = self._dds_interface.create_subscriber(
                self.topic_prefix + "robot_cartesian_state", 
                MotionControlState.RobotCartesianStatePubSubType(), 
                MotionControlState.RobotCartesianState(), 
                lambda data: robot_cartesian_state_callback(self, data), 
                SubscriberQosProfile.BEST_EFFORT)   
        self_subscriber_list.append(self._robot_control_group_state_subscriber)

        # subscriber for base data
        self._base_data = {}
        def base_data_callback(self, data):
            self._base_data['quat_xyzw'] = list(data.quat_xyzw())
            self._base_data['quat_wxyz'] = list(data.quat_wxyz())
            self._base_data['rpy'] = list(data.rpy())
            self._base_data['omega_W'] = list(data.omega_W())
            self._base_data['acc_W'] = list(data.acc_W())
            self._base_data['omega_B'] = list(data.omega_B())
            self._base_data['acc_B'] = list(data.acc_B())
            self._base_data['vel_W'] = list(data.vel_W())
            self._base_data['pos_W'] = list(data.pos_W())
            self._base_data['vel_B'] = list(data.vel_B())

        self._base_data_subscriber = self._dds_interface.create_subscriber(
                self.topic_prefix + "base_data_pub", 
                BaseData.BaseDataPubSubType(), 
                BaseData.BaseData(), 
                lambda data: base_data_callback(self, data), 
                SubscriberQosProfile.BEST_EFFORT)   
        self_subscriber_list.append(self._base_data_subscriber)

        # subscriber for true data
        self._true_data = {}
        def true_data_callback(self, data):
            self._true_data['vel_B'] = list(data.vel_B())
            self._true_data['vel_W'] = list(data.vel_W())
            self._true_data['pos_W'] = list(data.pos_W())
            self._true_data['contact_fz'] = list(data.contact_fz())
            self._true_data['contact_prob'] = list(data.contact_prob())

        self._true_data_subscriber = self._dds_interface.create_subscriber(
                self.topic_prefix + "true_data_pub", 
                TrueData.TrueDataPubSubType(), 
                TrueData.TrueData(), 
                lambda data: true_data_callback(self, data), 
                SubscriberQosProfile.BEST_EFFORT)   
        self_subscriber_list.append(self._true_data_subscriber)

        # subscriber for contact data
        self._contact_data = {}
        def contact_data_callback(self, data):
            self._contact_data['contact_force'] = list(data.contact_force())
            self._contact_data['contact_prob'] = list(data.contact_prob())
        
        self._contact_data_subscriber = self._dds_interface.create_subscriber(
                self.topic_prefix + "contact_data_pub", 
                ContactData.ContactDataPubSubType(), 
                ContactData.ContactData(), 
                lambda data: contact_data_callback(self, data), 
                SubscriberQosProfile.BEST_EFFORT)
        

        cnt = 0
        while not all(subscriber.is_matched for subscriber in self_subscriber_list):
            time.sleep(0.5)
            print("Waiting for all subscribers to be matched...")
            cnt += 1
            if cnt > 5:
                raise TimeoutError("Timeout waiting for subscribers to be matched, please check if AuroraCore is running!")
        print("All subscribers are matched")

    def _init_publishers(self):
        print("Creating publishers...")
        self._publisher_list = []
        self._state_publisher = self._dds_interface.create_publisher(self.topic_prefix + "whole_body_fsm_state_change_cmd", \
                                AuroraCmd.WholeBodyFsmStateChangeCmdPubSubType(), PublisherQosProfile.BEST_EFFORT)
        self._publisher_list.append(self._state_publisher)

        self._upper_state_publisher = self._dds_interface.create_publisher(self.topic_prefix + "upper_body_fsm_state_change_cmd", \
                                AuroraCmd.UpperBodyFsmStateChangeCmdPubSubType(), PublisherQosProfile.BEST_EFFORT)
        self._publisher_list.append(self._upper_state_publisher)

        self._velocity_publisher = self._dds_interface.create_publisher(self.topic_prefix + "velocity_cmd", \
                                AuroraCmd.VelocityCmdPubSubType(), PublisherQosProfile.BEST_EFFORT)
        self._publisher_list.append(self._velocity_publisher)

        self._stand_pose_publisher = self._dds_interface.create_publisher(self.topic_prefix + "robot_stand_pose_cmd", \
                                AuroraCmd.RobotStandPoseCmdPubSubType(), PublisherQosProfile.BEST_EFFORT)
        self._publisher_list.append(self._stand_pose_publisher)

        self._velocity_source_publisher = self._dds_interface.create_publisher(self.topic_prefix + "velocity_source_cmd", \
                                AuroraCmd.VelocitySourceCmdPubSubType(), PublisherQosProfile.BEST_EFFORT)
        self._publisher_list.append(self._velocity_source_publisher)

        self._robot_control_group_cmd_publisher = self._dds_interface.create_publisher(self.topic_prefix + "robot_control_group_cmd", \
                                MotionControlCmd.RobotControlGroupCmdPubSubType(), PublisherQosProfile.BEST_EFFORT)
        self._publisher_list.append(self._robot_control_group_cmd_publisher)

        self._robot_motor_cfg_group_cmd_publisher = self._dds_interface.create_publisher(self.topic_prefix + "robot_motor_cfg_group_cmd", \
                                MotorCfgCmd.RobotMotorCfgGroupCmdPubSubType(), PublisherQosProfile.BEST_EFFORT)
        self._publisher_list.append(self._robot_motor_cfg_group_cmd_publisher)
    
        cnt = 0
        while not all(publisher.is_matched for publisher in self._publisher_list):
            time.sleep(0.5)
            print("Waiting for all publishers to be matched...")
            cnt += 1
            if cnt > 5:
                raise TimeoutError("Timeout waiting for publishers to be matched, please check if AuroraCore is running!")
        print("All publishers are matched")

#----------------------------------------------User Interface----------------------------------------------------------------
    def close(self):
        del self._dds_interface

    @classmethod
    # if not "null", add prefix "RobotName/SerialNumber/"to the topic names
    def get_instance(cls, domain_id: int =123, participant_qos = 0, robot_name: str = "gr2t2v2", serial_number = None) -> "AuroraClient":
        """Get the AuroraClient instance.
        Args:
            domain_id (int): Domain ID of the DDS participant.
            participant_qos (int): QoS of the DDS participant.
            robot_name (str): Name of the robot.
            serial_number (str): Serial number of the robot. If not "null", add prefix "RobotName/SerialNumber/"to the topic names
        Returns:
            AuroraClient: The AuroraClient instance.
        """
        if cls._instance is None:
            cls._instance = cls(domain_id, participant_qos, robot_name, serial_number)
        return cls._instance

#----------------------------------------------Setter Functions----------------------------------------------------------------
    def set_fsm_state(self, state: int):
        """Set the robot's whole body fsm state.
        
        Args:
            state (int): Desired state of the robot. Valid values are:
                0 - Default state
                1 - Joint stand
                2 - PD stand 
                3~8 - User controller A~F
                9 - Security protection
                10 - User command
                11 - Upper body user command

        """
        whole_body_fsm_state_change_cmd = AuroraCmd.WholeBodyFsmStateChangeCmd()
        whole_body_fsm_state_change_cmd.desired_state(state)
        self._state_publisher.publish(whole_body_fsm_state_change_cmd)
    
    def set_upper_fsm_state(self, state: int):
        """Set the robot's upper body fsm state.
        
        Args:
            state (int): Desired upper state of the robot. Valid values are:
                0 - Default state
                1 - Act state (Arm swing)
                2 - Remote state

        """
        upper_body_fsm_state_change_cmd = AuroraCmd.UpperBodyFsmStateChangeCmd()
        upper_body_fsm_state_change_cmd.desired_state(state)
        self._upper_state_publisher.publish(upper_body_fsm_state_change_cmd)

    def set_velocity_source(self, source: int):
        """Set the source of velocity commands.
        
        Args:
            source (int): Desired velocity command source. Valid values are:
                0 - Joystick
                1 - Handheld device
                2 - Navigation 
        
        Note:
            For client velocity control, set source to 2 (Navigation).
        """
        velocity_source_cmd = AuroraCmd.VelocitySourceCmd()
        velocity_source_cmd.desired_source(source)
        self._velocity_source_publisher.publish(velocity_source_cmd)

    def set_velocity(self, vx: float, vy: float, yaw: float):
        """Set the robot's velocity command.
        
        Args:
            vx (float): Linear velocity in x direction (forward/backward) in m/s
            vy (float): Linear velocity in y direction (left/right) in m/s
            yaw (float): Angular velocity around z axis (rotation) in rad/s
        
        Note:
            The actual velocity achieved may be limited by the robot's physical capabilities.
        """
        velocity_cmd = AuroraCmd.VelocityCmd()
        velocity_cmd.vx(vx) 
        velocity_cmd.vy(vy) 
        velocity_cmd.yaw(yaw)
        self._velocity_publisher.publish(velocity_cmd)
        
    def set_stand_pose(self, delta_z: float, delta_pitch: float, delta_yaw: float):
        """Set the robot's stand pose command.
        
        Args:
            delta_z (float): ...
            delta_pitch (float): ...
            delta_yaw (float): ...
        
        Note:
            Stand pose adjustment is only avaliable in PdStand state (2). The actual stand pose may be limited by the robot's physical capabilities.
        """
        stand_pose_cmd = AuroraCmd.RobotStandPoseCmd()
        stand_pose_cmd.delta_z(delta_z) 
        stand_pose_cmd.delta_pitch(delta_pitch) 
        stand_pose_cmd.delta_yaw(delta_yaw)
        self._stand_pose_publisher.publish(stand_pose_cmd)

    def set_joint_positions(self, position_dict: Dict[str, list[float]], is_upper: bool = True):
        """Set the robot's joint positions.
        
        Args:
            position_dict (Dict[str, list[float]]): Dictionary with joint names as keys and joint positions as values.
        """

        if not isinstance(position_dict, Dict):
            raise TypeError("position_dict should be a dictionary with joint names as keys and joint positions as values")
        group_cmd_list = []
        for group_name, position in position_dict.items():
            
            group_cmd = MotionControlCmd.ControlGroupCmd()
            group_cmd.group_name(group_name)
            group_cmd.position(position)
            group_cmd_list.append(group_cmd)

        robot_control_group_cmd = MotionControlCmd.RobotControlGroupCmd()
        try:
            robot_control_group_cmd.group_cmd(group_cmd_list)
        except Exception as e:
            print(f"Error: {e}")

        self._robot_control_group_cmd_publisher.publish(robot_control_group_cmd)

    def set_motor_cfg(self, kp_config: Dict[str, list[float]], kd_config:Dict[str, list[float]]):
        """Set the robot's motor configuration.

        Args:
            kp_config (Dict[str, list[float]]): Dictionary with group names as keys and proportional gain (kp) values as lists.
            kd_config (Dict[str, list[float]]): Dictionary with group names as keys and derivative gain (kd) values as lists.
        
        Note:
            The keys in kp_config and kd_config should match the group names in the robot's motor configuration.
            The values in kp_config and kd_config should be lists of floats representing the gains for each joint in the group.
        """

        group_cfg_list = []
        for group_name in kp_config.keys():
            
            group_cfg = MotorCfgCmd.MotorCfgGroupCmd()
            group_cfg.group_name(group_name)
            group_cfg.pd_kp(kp_config[group_name])
            group_cfg.pd_kd(kd_config[group_name])
            group_cfg_list.append(group_cfg)

        robot_motor_cfg_group_cmd = MotorCfgCmd.RobotMotorCfgGroupCmd()
        try:
            robot_motor_cfg_group_cmd.group_cmd(group_cfg_list)
        except Exception as e:
            print(f"Error: {e}")
        self._robot_motor_cfg_group_cmd_publisher.publish(robot_motor_cfg_group_cmd)
    
#----------------------------------------------Getter Functions----------------------------------------------------------------

    def get_fsm_state(self) -> str:
        """Get the current state of the robot.
        
        Returns:
            str: The name of the current whole body fsm state.
        """

        if 'whole_body_fsm_state' not in self._aurora_state:
            raise KeyError("Whole body fsm state not found in aurora state. Please check if the subscriber is initialized correctly.")
        return self._aurora_state['whole_body_fsm_state'].name

    def get_upper_fsm_state(self) -> str:
        """Get the current upper state of the robot.
        
        Returns:
            str: The name of the current upper body fsm state.
        """

        if 'upper_body_fsm_state' not in self._aurora_state:
            raise KeyError("Upper body fsm state not found in aurora state. Please check if the subscriber is initialized correctly.")
        return self._aurora_state['upper_body_fsm_state'].name

    def get_velocity_source(self) -> str:
        """Get the current velocity command source of the robot.
        
        Returns:
            str: The name of the current velocity command source.
        """

        if 'velocity_command_source' not in self._aurora_state:
            raise KeyError("Velocity command source not found in aurora state. Please check if the subscriber is initialized correctly.")
        return self._aurora_state['velocity_command_source'].name

    def get_stand_pose(self) -> list: 
        """Get the stand pose of the robot.

        Returns:
            list[float]: stand pose data in a list, formatted as [delta_z, delta_pitch, delta_yaw, stable_level]
        """
        return self._stand_pose_state

    
    def get_group_state(self, group_name: str, key: str = 'position') -> list[float]:
        """Get the state of a specific robot control group.

        Args:
            group_name (str): The name of the robot control group.
            key (str): The key to retrieve from the group state. Valid values are:
                'position' - Joint positions
                'velocity' - Joint velocities
                'effort' - Joint efforts
        Returns:
            list[float]: The requested state data for the specified group.
        """

        if group_name not in self._robot_control_group_state:
            raise KeyError(f"Joint name {group_name} not found in robot control group state. Available joints: {list(self._robot_control_group_state.keys())}")
        elif key not in self._robot_control_group_state[group_name]:
            raise KeyError(f"Key {key} not found in group state for {group_name}. Available keys: {list(self._robot_control_group_state[group_name].keys())}")
        else:
            return self._robot_control_group_state[group_name][key]
        
    def get_cartesian_state(self, group_name: str, key: str = 'pose') -> list[float]:
        """Get the cartesian state of a specific robot group.

        Args:
            group_name (str): The name of the robot group.
            key (str): The key to retrieve from the cartesian state. Valid values are:
                'pose' - Group pose
                'twist' - Group twist
                'wrench' - Group wrench
        Returns:
            list[float]: The requested state cartesian for the specified group.
        """

        if group_name not in self._robot_cartesian_state:
            raise KeyError(f"Group name {group_name} not found in robot control group state. Available joints: {list(self._robot_cartesian_state.keys())}")
        elif key not in self._robot_cartesian_state[group_name]:
            raise KeyError(f"Key {key} not found in group state for {group_name}. Available keys: {list(self._robot_cartesian_state[group_name].keys())}")
        else:
            return self._robot_cartesian_state[group_name][key]
    
    def get_base_data(self, key: str) -> list[float]:
        """Get the base data of the robot.

        Args:
            key (str): The key to retrieve from the base data.
        Returns:
            list[float]: The requested base data.
        """

        if key not in self._base_data:
            raise KeyError(f"Key {key} not found in base data. Available keys: {list(self._base_data.keys())}")
        else:
            return self._base_data[key]
        
    def get_true_data(self, key: str) -> list[float]:
        """Get the true data of the robot.
        Args:
            key (str): The key to retrieve from the true data.
        Returns:
            list[float]: The requested true data.
        """

        if key not in self._true_data:
            raise KeyError(f"Key {key} not found in true data. Available keys: {list(self._true_data.keys())}")
        else:
            return self._true_data[key]
    
    def get_contact_data(self, key: str) -> list[float]:
        """Get the contact data of the robot.
        Args:
            key (str): The key to retrieve from the contact data.
        Returns:
            list[float]: The requested contact data.
        """
        
        if key not in self._contact_data:
            raise KeyError(f"Key {key} not found in contact data. Available keys: {list(self._contact_data.keys())}")
        else:
            return self._contact_data[key]
    
