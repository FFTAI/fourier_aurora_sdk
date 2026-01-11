# Default State Specification

**Default state** is entry state of Aurora, where no controller is running. 

## State specification

State name | Task name | Joystick mapping | DDS mapping | Frequency
-----------|-----------|------------------|-------------|-------------
Default    | (no task) | LB+RB            | 0           | -

Avaliable for hanging | Avaliable for standing | Auto Protection Switch
----------------------|------------------------|----------------
Yes                   | No                     | No

## Joystick Control

### Enter Default State

After initailize *AuroraCore*, press bumper `LB` and `RB` at the same time to enter default state.

## Client Control

Velocity control | Stand pose control | Joint control | Joint parameter control
-----------------|--------------------|---------------|-------------------
No               | No                 | No            | No

### Enter Default State

After initailize *AuroraCore*, use aurora client's `set_fsm_state` function to enter default state.

```python
client = AuroraClient.get_instance(domain_id=123, robot_name="fouriern1", serial_number=None)   # initialize aurora client
time.sleep(1)

client.set_fsm_state(0)     # change to default state
```




