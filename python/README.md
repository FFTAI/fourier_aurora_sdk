# fourier_aurora_client

This is the Python client for **Fourier Aurora** *(Advanced Unified Robot Operation and Resource Architecture)*. It allows you to interact with the Aurora API to retrieve data and perform actions on the Aurora platform.

If you are new to Aurora, please read the following documentation for basic understanding of the system:

- [Aurora Introduction](./docs/EN/introduction_EN.md)
- [Developer Guide](./docs/EN/developer_guide_EN.md)

## Installation

```python
pip install fourier-aurora-client
```

## Using the Client

After running Aurora, start another terminal and enter the python environment where fourier-aurora-client is installed. Run the example.

```python
cd examples
python3 client_usage.py
```

After running the example, the terminal will display that the Aurora and Aurora client are successfully subscribed and published to each other, and the DDS communication is established.

![图1](./images/matched.png)

For more detailed usage tutorials and API documentation, please refer to:
更为详尽的使用教程以及接口文档请参考:

- [usage tutorials](./docs/CN/client_usage_tutorial_CN.md)
- [API documentation](./docs/CN/client_CN.md)

## Note
If Aurora is running in a docker container and the client is run on the host machine, set the environment variable 

```python
export FASTDDS_BUILTIN_TRANSPORTS=UDPv4
```