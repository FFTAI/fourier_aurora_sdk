# Fourier Aurora SDK

This is the SDK repository for **Fourier Aurora** *(Advanced Unified Robot Operation and Resource Architecture)*.

If you are new to Aurora, please read the following documentation for basic understanding of the system:

- [Aurora Introduction](./doc/EN/introduction_EN.md)
- [Developer Guide](./doc/EN/developer_guide_EN.md)

For video introduction, please check the following link: [Video Introduction](https://pan.baidu.com/s/1Zcq6ZnGziW1BQBPGOnmG_A?pwd=upiv)

## v1.2.0 Release

Support Robots:

- Fourier GR-1
- Fourier GR-2
- Fourier GR-3

Prerequisites:
- Actuator version:
  - Communication firmware version 0.3.12.31 or above.
  - Driver firmware version 0.2.10.30 or above.
  - NOTE: Actuator version can be upgraded using **FSA Assistant**. Click [FSA Assistant for Linux](https://fsa-1302548221.cos.ap-shanghai.myqcloud.com/tool/FSA_Assistant/FSA_Assistant_V0.0.1.24_155_31_x64_Linux_2025-07-08.tar.gz) to download the latest version.
- Submodule: 
  - Aurora base environment.
  - **fourier_dds** version 1.1.0 or above. 
  - **fourier_hardware** version 1.1.2 or above.
  - NOTE: aurora base environment is provided in the docker image. **fourier_dds** and **fourier_hardware** can be installed and upgraded using deb packages.

## Installation

Please refer to [Installation Guide](./doc/EN/installation_EN.md) for installation instructions.

## Quick Start

Please follow the steps below to get started with controllers provided using joystick. If you are not familiar with the joystick, please refer to the [Joystick Tutorial](./doc/EN/joystick_tutorial_EN.md) for more information.

### Starting Container

You can start the container by running the following command in your terminal under the **root directory** of the repository:

```bash
(sudo) bash docker_run.bash
```

NOTE: Please make sure that you have installed fourier-aurora and relative modules in the docker image as described in the [Installation Guide](./doc/EN/installation_EN.md).

### Preparation before Running Aurora

Aurora can control the robot in simulation or on the real robot. Before running Aurora, you need to follow the steps below:

#### Simulation

For simulation usage, you need to start another terminal and enter the container you just started using the following command:

```bash
(sudo) docker exec -it fourier_aurora_sdk bash
```

Then, you need to start the simulation environment by running the following command:

```bash
python3 sim/start_simulate_root.py
```

Follow the instructions and it will start the simulation environment. The robot will be placed in the starting position.

#### Real Robot

To control the real robot, please make sure that the following prerequisites are met:

- Actuator versions are compatible.
- Actuators are in Power-on state(purple light flashing slowly).
- Network communication is well established(all actuators ip pingable).
- Robot is in a neutral hanging state.

### Starting Aurora

To start Aurora, run the following command in the terminal:

```bash
AuroraCore --name gr2 --type 0
```

Here, `--name` refers to the robot name for Aurora. Avaliable robot names are: *gr1*, *gr2*, *gr3*.
`--type` refers to run type for Aurora, *0* for run on simulator, *1* for run on real robot.

The following message will appear in the terminal if the program is running successfully:

```bash
[info] FSM state run, enter "Default" mode by default
```

This message indicates that the self-check is passed(real robot) and the Aurora program is ready to run tasks.

### Using joystick to Control the Robot

Now you can use the joystick to control the robot. Please refer to the [Joystick Tutorial](./doc/EN/joystick_tutorial_EN.md) for joystick usage.

## Issues

Please report any issues or bugs! We will do our best to fix them.

## License

[Apache 2.0](LICENSE) © Fourier
