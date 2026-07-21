from __future__ import annotations

import threading
from importlib import import_module

_sdk = import_module("._fourier_aurora_sdk", __package__)
_CONSTRUCTION_TOKEN = object()


class AuroraClient:
    _instance = None
    _instance_lock = threading.Lock()

    def __init__(self, native, token=None):
        if token is not _CONSTRUCTION_TOKEN:
            raise TypeError("use AuroraClient.get_instance()")
        self._native = native

    @classmethod
    def get_instance(cls):
        with cls._instance_lock:
            if cls._instance is None:
                native = _sdk.AuroraClient.get_instance()
                cls._instance = cls(native, _CONSTRUCTION_TOKEN)
            return cls._instance

    def config(self):
        return self._native.config()

    def configure(self, options):
        return self._native.configure(options)

    def set_domain_id(self, domain_id):
        return self._native.set_domain_id(domain_id)

    def set_user_id(self, user_id):
        return self._native.set_user_id(user_id)

    def domain_id(self):
        return self._native.domain_id()

    def user_id(self):
        return self._native.user_id()

    def state(self):
        return self._native.state()

    def start(self):
        return self._native.start()

    def stop(self):
        return self._native.stop()

    def endpoint_match_status(self):
        return self._native.endpoint_match_status()

    def is_fsm_service_matched(self):
        return self._native.is_fsm_service_matched()

    def wait_for_endpoints(self, timeout):
        return self._native.wait_for_endpoints(timeout)

    def wait_for_fsm_state(self, state_id, timeout):
        return self._native.wait_for_fsm_state(state_id, timeout)

    def get_aurora_state(self):
        return self._native.get_aurora_state()

    def get_motion_state(self):
        return self._native.get_motion_state()

    def get_control_group_state(self):
        return self._native.get_control_group_state()

    def get_estimator_data(self):
        return self._native.get_estimator_data()

    def get_error_codes(self):
        return self._native.get_error_codes()

    def get_base_data(self):
        return self._native.get_base_data()

    def get_contact_data(self):
        return self._native.get_contact_data()

    def get_stable_level(self):
        return self._native.get_stable_level()

    def get_stand_pose(self):
        return self._native.get_stand_pose()

    def get_joint_state(self, group_name, field):
        return self._native.get_joint_state(group_name, field)

    def get_joint_command(self, group_name, field):
        return self._native.get_joint_command(group_name, field)

    def get_cartesian_state(self, link_name):
        return self._native.get_cartesian_state(link_name)

    def get_cartesian_command(self, link_name):
        return self._native.get_cartesian_command(link_name)

    def get_last_motion_command(self):
        return self._native.get_last_motion_command()

    def get_last_control_group_command(self):
        return self._native.get_last_control_group_command()

    def get_last_sub_controller_command(self):
        return self._native.get_last_sub_controller_command()

    def publish_motion_command(self, command):
        return self._native.publish_motion_command(command)

    def publish_control_group_command(self, command):
        return self._native.publish_control_group_command(command)

    def publish_sub_controller_command(self, command):
        return self._native.publish_sub_controller_command(command)

    def request_fsm_state(self, state_id):
        return self._native.request_fsm_state(state_id)

    def set_velocity(
        self,
        vx,
        vy,
        yaw,
        duration,
        gait_type=0,
    ):
        return self._native.set_velocity(
            vx,
            vy,
            yaw,
            duration,
            gait_type,
        )

    def set_stand_pose(
        self,
        delta_height,
        delta_pitch,
        delta_yaw,
        duration=1.0,
    ):
        return self._native.set_stand_pose(
            delta_height,
            delta_pitch,
            delta_yaw,
            duration,
        )

    def reset_unicontroller(self):
        return self._native.reset_unicontroller()
