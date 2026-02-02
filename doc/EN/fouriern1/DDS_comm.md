# DDS Communication Guide

## Introduction to DDS

The OMG Data Distribution Service (DDS) is a middleware protocol and API standard for data-centric connectivity from the OMG. It integrates the components of a system together, providing low-latency data connectivity, extreme reliability, and a scalable architecture that business and mission-critical Internet of Things (IoT) applications need.

In a distributed system, middleware is the software layer that lies between the operating system and applications. It enables the various components of a system to more easily communicate and share data. It simplifies the development of distributed systems by letting software developers focus on the specific purpose of their applications rather than the mechanics of passing information between applications and systems.

![aurora_dataflow](../../image/DDS_data_centricity.jpg)

(source: https://www.dds-foundation.org)

## Fourier Aurora Client

**Fourier Aurora Client** is the Python client for Aurora. It allows you to interact with the Aurora API to retrieve data and perform actions on the Aurora platform through DDS middleware. Aurora client can be installed on the robot chest computer or any other devices to communicate with Aurora server.

For installation of fourier aurora client, please refer to quick start section (needs link).

### Client Usage

Before using fourier aurora client, please make sure **Aurora** is started. To initialze fourier aurora client, use `get_instance` function. At the end of the code, please use `close` funtion to apply clean up.

```python
from fourier_aurora_client import AuroraClient

client = AuroraClient.get_instance(domain_id=123, robot_name="fouriern1", namespace=None, is_ros_compatible=False)

# execution contents here ...

client.close()
```




