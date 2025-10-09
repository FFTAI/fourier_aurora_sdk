# fourier_aurora_client

This is the Python client for **Fourier Aurora** *(Advanced Unified Robot Operation and Resource Architecture)*. It allows you to interact with the Aurora API to retrieve data and perform actions on the Aurora platform. Aurora client can be installed on the robot chest computer or any other devices to communicate with Aurora server.

If you are new to Aurora, please read the following documentation for basic understanding of the system:

- [Aurora Introduction](../doc/EN/introduction_EN.md)

## Installation

### Dependencies

- Python >= 3.9

For [demo_walk.py](./example/gr2/demo_walk.py) example, some extra dependencies are required:

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

Aurora client can be used to interact with Aurora server through DDS protocol. You can send commands to Aurora, receive states and sensor data from Aurora. Before using the client, make sure Aurora server is running on the robot. You can refer to [Getting Started](../README.md) for how to start Aurora server.

**NOTE: The Domain ID of the client should be the same as that of the server. The default Domain ID of Aurora is 123, which can be configured in the config file when starting the server.**

### API

For API specification of fourier aurora client , please refer to [API Documentation](./docs/EN/API_document_EN.md)

### Example

Under [example](./example) section, some client usage example is provided.

#### demo_client_usage.py

Avaliable for all robot types, goes through an example on setting Aurora's state and command.

#### demo_move_joints.py

Avaliable for all robot types, goes through an example on setting upper body's joint angles.

#### demo_get_states.py

Avaliable for all robot types, goes through an example on receiving aurora published states.

#### demo_walk.py

Avaliable for GR-2, goes through an example on applying locomotion policy on robot using user command task.
