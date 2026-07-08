from __future__ import annotations

import warnings
from importlib import import_module

_sdk = import_module("._fourier_aurora_sdk", __package__)


def _state_info_to_dict(info):
    return {
        "id": info.id,
        "version": info.version,
        "name": info.name,
        "description": info.description,
        "active": info.active,
    }


def _entry_field(entry, field):
    if field == "position":
        return list(entry.joint_position)
    if field == "velocity":
        return list(entry.joint_velocity)
    if field == "effort":
        return list(entry.joint_effort)
    raise ValueError(f"unsupported field: {field}")


class AuroraClient:
    _instance = None

    def __init__(self, native):
        self._native = native

    @classmethod
    def get_instance(cls, *args, **kwargs):
        user_id = kwargs.pop("user_id", None)
        domain_id = kwargs.pop("domain_id", None)
        kwargs.pop("participant_qos", None)
        kwargs.pop("robot_name", None)
        namespace = kwargs.pop("namespace", None)
        is_ros_compatible = kwargs.pop("is_ros_compatible", None)
        use_namespace = kwargs.pop("use_namespace", True)
        use_discovery_server = kwargs.pop("use_discovery_server", None)
        dds_mode = kwargs.pop("dds_mode", 0)
        service_timeout_ms = kwargs.pop("service_timeout_ms", 1000)

        if args:
            if len(args) > 1:
                raise TypeError("get_instance accepts at most one positional compatibility domain_id")
            if domain_id is not None:
                raise TypeError("domain_id cannot be passed both positionally and by keyword")
            domain_id = args[0]
            if user_id is None:
                warnings.warn(
                    "Positional get_instance(domain_id, ...) is deprecated; pass user_id=... explicitly.",
                    DeprecationWarning,
                    stacklevel=2,
                )
                user_id = "python_compat"

        if user_id is None:
            raise TypeError("user_id is required")
        if domain_id is None:
            domain_id = 123
        if kwargs:
            unknown = ", ".join(sorted(kwargs))
            raise TypeError(f"unsupported get_instance options: {unknown}")

        options = _sdk.ConnectionOptions()
        options.dds_mode = int(dds_mode)
        options.use_namespace = bool(use_namespace)
        options.service_timeout_ms = int(service_timeout_ms)
        if namespace is not None:
            options.namespace_name = str(namespace)
        if is_ros_compatible is not None:
            options.is_ros_compatible = bool(is_ros_compatible)
        if use_discovery_server is not None:
            options.use_discovery_server = bool(use_discovery_server)

        native = _sdk.AuroraClient.get_instance(user_id, int(domain_id), options)
        cls._instance = cls(native)
        return cls._instance

    @classmethod
    def close_instance(cls):
        if cls._instance is not None:
            cls._instance.close()

    def __getattr__(self, name):
        return getattr(self._native, name)

    def close(self):
        self._native.close()
        if AuroraClient._instance is self:
            AuroraClient._instance = None

    def set_fsm_state(self, state_id):
        return self._native.requestFsmState(int(state_id)).success()

    def set_motion_cmd(
        self,
        position=None,
        lin_velocity=None,
        attitude=None,
        ang_velocity=None,
        delta_height=0.0,
        gait_type=0,
        skills=None,
        duration=1.0,
    ):
        cmd = _sdk.MotionCommand()
        if position is not None:
            cmd.position = list(position)
        if lin_velocity is not None:
            cmd.lin_velocity = list(lin_velocity)
        if attitude is not None:
            cmd.attitude = list(attitude)
        if ang_velocity is not None:
            cmd.ang_velocity = list(ang_velocity)
        cmd.delta_height = float(delta_height)
        cmd.gait_type = int(gait_type)
        if skills is not None:
            cmd.skills = list(skills)
        cmd.duration = float(duration)
        self._native.publishMotionCommand(cmd)

    def set_velocity(self, vx=0.0, vy=0.0, yaw=0.0, duration=1.0, gait_type=0):
        self._native.setVelocity(float(vx), float(vy), float(yaw), float(duration), int(gait_type))

    def set_stand_pose(self, delta_z=0.0, delta_pitch=0.0, delta_yaw=0.0, duration=1.0):
        self._native.setStandPose(float(delta_z), float(delta_pitch), float(delta_yaw), float(duration))

    def set_sub_controller_cmd(self, reset=False, controller_params=None):
        cmd = _sdk.SubControllerCommand()
        cmd.reset = bool(reset)
        if controller_params:
            cmd.controller_params = controller_params
        self._native.publishSubControllerCommand(cmd)

    def reset_unicontroller(self):
        self._native.resetUnicontroller()

    def set_group_cmd(self, position_cmd=None, velocity_cmd=None, effort_cmd=None, cartesian_cmd=None):
        command = _sdk.ControlGroupCommand()
        groups = set()
        for mapping in (position_cmd, velocity_cmd, effort_cmd):
            if mapping:
                groups.update(mapping.keys())
        joint_cmd = []
        for group in sorted(groups):
            entry = _sdk.JointCommand()
            entry.group_name = group
            if position_cmd and group in position_cmd:
                entry.position = list(position_cmd[group])
            if velocity_cmd and group in velocity_cmd:
                entry.velocity = list(velocity_cmd[group])
            if effort_cmd and group in effort_cmd:
                entry.effort = list(effort_cmd[group])
            joint_cmd.append(entry)
        command.joint_cmd = joint_cmd
        if cartesian_cmd:
            command.cartesian_cmd = list(cartesian_cmd)
        self._native.publishControlGroupCommand(command)

    def get_fsm_state(self):
        return self._native.getAuroraState().current_state.id

    def get_fsm_name(self):
        return self._native.getAuroraState().current_state.name

    def get_current_state_info(self):
        return _state_info_to_dict(self._native.getAuroraState().current_state)

    def get_available_state_info(self):
        return [_state_info_to_dict(info) for info in self._native.getAuroraState().available_states]

    def get_stand_pose(self):
        return list(self._native.getStandPose())

    def get_stable_level(self):
        return self._native.getStableLevel()

    def get_group_state(self, group_name, field="position"):
        return _entry_field(
            self._native.getControlGroupState().joint_state[
                [entry.group_name for entry in self._native.getControlGroupState().joint_state].index(group_name)
            ],
            field,
        )

    def get_group_cmd(self, group_name, field="position"):
        return _entry_field(
            self._native.getControlGroupState().joint_cmd[
                [entry.group_name for entry in self._native.getControlGroupState().joint_cmd].index(group_name)
            ],
            field,
        )

    def get_cartesian_state(self, link_name):
        entry = self._native.getCartesianState(link_name)
        return {"pose": list(entry.pose), "twist": list(entry.twist), "wrench": list(entry.wrench)}

    def get_cartesian_cmd(self, link_name):
        entry = self._native.getCartesianCommand(link_name)
        return {"pose": list(entry.pose), "twist": list(entry.twist), "wrench": list(entry.wrench)}

    def get_base_data(self):
        base = self._native.getBaseData()
        return {
            "quat_xyzw": list(base.quat_xyzw),
            "quat_wxyz": list(base.quat_wxyz),
            "rpy": list(base.rpy),
            "omega_W": list(base.omega_W),
            "acc_W": list(base.acc_W),
            "omega_B": list(base.omega_B),
            "acc_B": list(base.acc_B),
            "vel_W": list(base.vel_W),
            "pos_W": list(base.pos_W),
            "vel_B": list(base.vel_B),
            "stable_level": base.stable_level,
        }

    def get_contact_data(self):
        contact = self._native.getContactData()
        return {
            "contact_force": list(contact.contact_force),
            "contact_prob": list(contact.contact_prob),
        }

    def get_motion_state(self):
        raise NotImplementedError("get_motion_state() compatibility dict was removed; use getMotionState().")


StateNotReadyError = _sdk.StateNotReadyError
AuroraSdkError = _sdk.AuroraSdkError
