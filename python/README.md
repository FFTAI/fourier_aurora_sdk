# fourier_aurora_client

This is the Python client for **Fourier Aurora** *(Advanced Unified Robot Operation and Resource Architecture)*. It allows you to interact with the Aurora API to retrieve data and perform actions on the Aurora platform.

If you are new to Aurora, please read the following documentation for basic understanding of the system:

- [Aurora Introduction](../doc/EN/introduction_EN.md)

## Installation

### Dependencies

- Python >= 3.9

For [demo_walk.py](../../../example/gr2/demo_walk.py) example, some extra dependencies are required:

- numpy >= 2.0.0
- torch >= 2.8.0
- pygame == 2.6.1
- ischedule == 1.2.7

### Installing

```bash
sudo apt install python3-pip
pip install fourier-aurora-client
```

## Usage

Under [example](../../../example) section, some client usage example is provided.

### demo_client_usage.py

Avaliable for all robot types, goes through an example on setting Aurora's state and command.

### demo_move_joints.py

Avaliable for all robot types, goes through an example on setting upper body's joint angles.

### demo_get_states.py

Avaliable for all robot types, goes through an example on receiving aurora published states.

### demo_walk.py

Avaliable for GR2, goes through an example on applying locomotion policy on robot using user command task.

## Note

If Aurora is running in a docker container and the client is run on the host machine, set the environment variable.

```python
export FASTDDS_BUILTIN_TRANSPORTS=UDPv4
```
