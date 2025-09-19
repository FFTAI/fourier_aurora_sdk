from enum import Enum


class WholeBodyFsmState(Enum):
    Default = 0
    JointStand = 1
    PdStand = 2
    UserController_A = 3
    UserController_B = 4
    UserController_C = 5
    UserController_D = 6
    UserController_E = 7
    UserController_F = 8
    SecurityProtection = 9
    UserCmd = 10
    UpperBodyUserCmd = 11

class UpperBodyFsmState(Enum):
    Default = 0
    UpperBodyActState = 1
    RemoteState = 2

class VelocityCommandSource(Enum):
    Joystick = 0
    Handheld = 1
    Navigation = 2

