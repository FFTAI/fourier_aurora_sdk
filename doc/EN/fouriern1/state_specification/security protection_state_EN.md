# Security Protection State Specification

**Security Protection State** is prepared for scenerios where robot goes off control. When entering security protection state, the controller will first set all actuators to damping mode for 2 seconds, then set all actuators to zero torque mode. The user could still read joint's states in zero torque mode.

Some controller comes with auto protection switch mechanism. When they finds themselves beyond the controllable range, they will automatically switch to security protection state.

## State specification

State name          | Task name              | Joystick mapping | DDS mapping | Frequency
--------------------|------------------------|------------------|-------------|-------------
Security Protection | SecurityProtectionTask | LT+RT            | 1           | 400Hz

Avaliable for hanging | Avaliable for standing | Auto Protection Switch
----------------------|------------------------|----------------
Yes                   | No                     | No

## Joystick Control

### Enter Secutiry Protection State

After initailize *AuroraCore*, press trigger `LT` and `RT` at the same time to enter security protection state.

## Client Control

Velocity control | Stand pose control | Joint control | Joint parameter control
-----------------|--------------------|---------------|-------------------
No               | No                 | No            | No

### Enter Security Protection State

After initailize *AuroraCore*, use aurora client's `set_fsm_state` function to enter security protection state.

```python
client = AuroraClient.get_instance(domain_id=123, robot_name="fouriern1", serial_number=None)   # initialize aurora client
time.sleep(1)

client.set_fsm_state(9)     # change to security protection state
```




