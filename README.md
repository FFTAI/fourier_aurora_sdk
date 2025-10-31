# Fourier Aurora SDK

This is the SDK repository for **Fourier Aurora** *(Advanced Unified Robot Operation and Resource Architecture)*.

If you are new to Aurora, please read the following documentation for basic understanding of the system:

- [Aurora Introduction](./doc/EN/introduction_EN.md)
- [Developer Guide](./doc/EN/developer_guide_EN.md)

## v1.2.1 Release

Support Robots:

- GR-1P
- GR-2
- GR-3
- Fourier-N1(new!)

Prerequisites:

- Actuator version:
  - Communication firmware version 0.3.12.31 or above.
  - Driver firmware version 0.2.10.30 or above.
  - NOTE: Actuator version can be upgraded using **FSA Assistant**. Click [FSA Assistant for Linux](https://fsa-1302548221.cos.ap-shanghai.myqcloud.com/tool/FSA_Assistant/FSA_Assistant_V0.0.1.24_155_31_x64_Linux_2025-07-08.tar.gz) to download the latest version.
 
## Installation

Please refer to [Installation Guide](./doc/EN/installation_EN.md) for installation instructions.

## Quick Start

Please follow the steps below to get started with controllers provided using joystick. If you are not familiar with the joystick, please refer to the [Joystick Tutorial](./doc/EN/joystick_tutorial_EN.md) for more information.

### Starting Container

You can start the container by running the following command in your terminal under the **root directory** of the repository, please modify the docker image in `docker_run.bash` if necessary. You may need `(sudo) docker images` command to check the available docker images on your system.

```bash
(sudo) bash docker_run.bash
```

NOTE: Please make sure that you have the right docker image as described in the [Installation Guide](./doc/EN/installation_EN.md).

### Preparation before Running Aurora

Aurora can control the robot in simulation or on the real robot. Before running Aurora, you need to follow the steps below:

#### 1. Running for Simulation

For simulation usage, you need to start another terminal and enter the container you just started using the following command:

```bash
(sudo) docker exec -it fourier_aurora_server bash
```

Then, you need to start the simulation environment by running the following command:

```bash
python3 sim/start_simulate_root.py
```

Follow the instructions and it will start the simulation environment. The robot will be placed in the starting position.

#### 2. Running for Real Robot

To control the real robot, please make sure that the following prerequisites are met:

- Actuator versions are compatible.
- Actuators are in Power-on state(purple light flashing slowly).
- Network communication is well established(all actuators ip pingable).
- Robot is in a neutral hanging state.
- (ONLY FOR GR-1P IN THE FIRST CREATION OF A CONTAINER) Calibrate the joints using pins. Please run `python3 /opt/fftai/fourier_aurora/scripts/calib/gr1t2Calibration.py` in the container with all pins inserted. **After calibration, please remove the pins.**

### Starting Aurora

To start Aurora, run the following command in the terminal:

```bash
AuroraCore --type 0
```

Here, `--type` refers to run type for Aurora, *0* for run on simulator, *1* for run on real robot.

You can also use the following command to start Aurora with a specific configuration file:

```bash
AuroraCore --config <path_to_your_config_file>
```
An example configuration file can be found in `config/config.yaml` in this repository.

The following message will appear in the terminal if the program is running successfully:

```bash
[info] FSM state run, enter "Default" mode by default
```

This message indicates that the self-check is passed(real robot) and the Aurora program is ready to run tasks.

### 1. Using joystick to Control the Robot

Now you can use the joystick to control the robot. Please refer to the [Joystick Tutorial](./doc/EN/joystick_tutorial_EN.md) for joystick usage.

### 2. Using Client to Control the Robot

You can also use the Aurora client to control the robot. Please refer to the [Client Tutorial](./python/README.md) for client usage.

## Issues

Please report any issues or bugs! We will do our best to fix them.

## License

[Apache 2.0](LICENSE) © Fourier
