# Joystick Tutorial

Aurora uses a joystick as the default input device. Users can utilize the joystick for both control command input and state transitions.

This tutorial will guide you through setting up and using the joystick with your Aurora system.

## Joystick Setup

1. Connect your device to the computer. Aurora uses the linux built-in driver `joystick.h` to read from user's joystick. To verify that your device is correctly loaded in the system, runn the following command in your terminal:

    ```bash
    ls /dev/input
    ```

    You should see a device named `js0`. This is the device file from which Aurora reads joystick data.

2. Start Aurora and check the log output. Your joystick is successfully connected to Aurora unless you see the following error message:

    ```bash
    [error] joystick init fail
    [error]         task enter failed:   JoyStickTask
    ```

    If the error message occurs, please try to rerun the setup process.

    **Note:**

    If you are running Aurora in a Docker container, please connect your joystick before starting the container. By default, Docker only recognizes devices that were present when the container was launched. If you plug in a joystick after the container has started, the device file will not be automatically available inside the running container.

## Joystick Usage in Aurora

The joystick serves two primary functions in Aurora: changing system states and inputting control commands.

### Change State

In Aurora, each FSM state is mapped to a specific combination of bumpers and buttons. The following table shows the joystick mappings for each FSM state:

| Buttons   | State Name            | State Description                     |
|-----------|-----------------------|---------------------------------------|
| LB+RB     | Default               | Default state, do nothing             |
| LT+RT     | SecurityProtection    | Safety state, stops all actuator      |
| LB+A      | JointStand            | All actuators move to zero position   |
| LB+B      | PdStand               | Robot stand on flat surface           |
| RB+A      | UserController_A      | Reserved for user-defined task        |
| RB+B      | UserController_B      | Reserved for user-defined task        |
| RB+Y      | UserController_C      | Reserved for user-defined task        |
| RB+X      | UserController_D      | Reserved for user-defined task        |
| RB+X      | UserController_E      | Reserved for user-defined task        |
| RB+Y      | UserController_F      | Reserved for user-defined task        |

### Input Control Commands

#### PdStand

In the PdStand state, the directional buttons are used to control the robot's pose. The `up/down` buttons adjust the height of the robot's upper body, while the `left/right` buttons control the pitch of the upper body.

#### UserController_A

In the gr2 expansion package, a simple locomotion task is provided, which is bound by default to the *UserController_A* state. The task uses left and right stick to control the robot's movement. The left stick's vertical axis controls longitudinal (forward) movement, the left stick's horizontal axis controls lateral (sideways) movement, and the right stick's horizontal axis controls yaw (rotation) movement.

## Joystick Layout Specification

The joystick driver in Aurora is designed based on the Xbox controller layout. Therefore, using an Xbox-style controller is recommended for the best experience. If your controller supports multiple output modes, please switch it to Xbox/PC mode before configuring.

If you need to use Aurora with a different controller layout, please note that button mappings may differ. For PlayStation controllers, the `△` button corresponds to the `X` button in this tutorial, and the `□` button corresponds to the `Y` button. Switch controllers are not supported.
