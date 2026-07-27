from __future__ import annotations

import threading
from importlib import import_module

_sdk = import_module("._fourier_aurora_sdk", __package__)
_CONSTRUCTION_TOKEN = object()


class AuroraClient:
    """Process-wide Aurora client with categorized high-level operations.

    ``Atomic`` means one complete typed endpoint operation, not a concurrency
    transaction. Abstract operations project or construct simpler values
    through an atomic operation.
    """

    _instance = None
    _instance_lock = threading.Lock()

    def __init__(self, native, token=None):
        """[Client] Wrap the native singleton; direct construction is prohibited."""
        if token is not _CONSTRUCTION_TOKEN:
            raise TypeError("use AuroraClient.get_instance()")
        self._native = native

    @classmethod
    def get_instance(cls):
        """[Client] Return the process-wide client without starting transport."""
        with cls._instance_lock:
            if cls._instance is None:
                native = _sdk.AuroraClient.get_instance()
                cls._instance = cls(native, _CONSTRUCTION_TOKEN)
            return cls._instance

    def config(self):
        """[Client] Return a copy of the canonical connection configuration."""
        return self._native.config()

    def configure(self, options):
        """[Client] Replace the connection configuration while stopped."""
        return self._native.configure(options)

    def set_domain_id(self, domain_id):
        """[Client] Set the DDS domain used by the next transport session."""
        return self._native.set_domain_id(domain_id)

    def set_user_id(self, user_id):
        """[Client] Set the SDK-local diagnostic label for the next session."""
        return self._native.set_user_id(user_id)

    def domain_id(self):
        """[Client] Return the configured DDS domain."""
        return self._native.domain_id()

    def user_id(self):
        """[Client] Return the configured SDK-local diagnostic label."""
        return self._native.user_id()

    def state(self):
        """[Client] Return local lifecycle state, not robot readiness."""
        return self._native.state()

    def start(self):
        """[Client] Start transport from the frozen stopped-stage configuration."""
        return self._native.start()

    def stop(self):
        """[Client] Synchronously stop and drain the active transport session."""
        return self._native.stop()

    def endpoint_match_status(self):
        """[Client] Return enabled and matched status for every endpoint."""
        return self._native.endpoint_match_status()

    def is_fsm_service_matched(self):
        """[Client] Report whether the enabled FSM service has a server match."""
        return self._native.is_fsm_service_matched()

    def wait_for_endpoints(self, timeout):
        """[Client] Wait for all enabled endpoints to match."""
        return self._native.wait_for_endpoints(timeout)

    def wait_for_fsm_state(self, state_id, timeout):
        """[Abstract wait] Wait for the latest Aurora snapshot to report an FSM ID."""
        return self._native.wait_for_fsm_state(state_id, timeout)

    def get_aurora_state(self):
        """[Atomic getter] Return the latest complete Aurora-state snapshot."""
        return self._native.get_aurora_state()

    def get_motion_state(self):
        """[Atomic getter] Return the latest complete motion-state snapshot."""
        return self._native.get_motion_state()

    def get_control_group_state(self):
        """[Atomic getter] Return the latest complete control-group snapshot."""
        return self._native.get_control_group_state()

    def get_estimator_data(self):
        """[Atomic getter] Return the latest complete estimator snapshot."""
        return self._native.get_estimator_data()

    def get_error_codes(self):
        """[Atomic getter] Return the latest asynchronous error-code snapshot."""
        return self._native.get_error_codes()

    def get_base_data(self):
        """[Abstract getter] Project base data from one estimator snapshot."""
        return self._native.get_base_data()

    def get_contact_data(self):
        """[Abstract getter] Project contact data from one estimator snapshot."""
        return self._native.get_contact_data()

    def get_stable_level(self):
        """[Abstract getter] Return stable level from one motion snapshot."""
        return self._native.get_stable_level()

    def get_stand_pose(self):
        """[Abstract getter] Return height, pitch, yaw, and stable level."""
        return self._native.get_stand_pose()

    def get_joint_state(self, group_name, field):
        """[Abstract getter] Return one measured joint field for a named group."""
        return self._native.get_joint_state(group_name, field)

    def get_joint_command(self, group_name, field):
        """[Abstract getter] Return one commanded joint field for a named group."""
        return self._native.get_joint_command(group_name, field)

    def get_cartesian_state(self, link_name):
        """[Abstract getter] Return a named link's measured Cartesian state."""
        return self._native.get_cartesian_state(link_name)

    def get_cartesian_command(self, link_name):
        """[Abstract getter] Return a named link's observed Cartesian command."""
        return self._native.get_cartesian_command(link_name)

    def get_last_motion_command(self):
        """[Client getter] Return the last locally published motion command."""
        return self._native.get_last_motion_command()

    def get_last_control_group_command(self):
        """[Client getter] Return the last locally published control-group command."""
        return self._native.get_last_control_group_command()

    def get_last_sub_controller_command(self):
        """[Client getter] Return the last locally published sub-controller command."""
        return self._native.get_last_sub_controller_command()

    def publish_motion_command(self, command):
        """[Atomic setter] Publish one complete motion command."""
        return self._native.publish_motion_command(command)

    def publish_control_group_command(self, command):
        """[Atomic setter] Publish one complete control-group command."""
        return self._native.publish_control_group_command(command)

    def publish_sub_controller_command(self, command):
        """[Atomic setter] Publish one complete sub-controller command."""
        return self._native.publish_sub_controller_command(command)

    def request_fsm_state(self, state_id):
        """[Atomic RPC] Request one FSM state ID and return Aurora's result."""
        return self._native.request_fsm_state(state_id)

    def set_velocity(
        self,
        vx,
        vy,
        yaw,
        duration,
        gait_type=0,
    ):
        """[Abstract setter] Build and publish base-frame velocity in SI units."""
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
        """[Abstract setter] Build and publish nominal-stand offsets in SI units."""
        return self._native.set_stand_pose(
            delta_height,
            delta_pitch,
            delta_yaw,
            duration,
        )

    def reset_unicontroller(self):
        """[Abstract setter] Build and publish a unicontroller reset command."""
        return self._native.reset_unicontroller()
