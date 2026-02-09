# Joint Stand State Reference

**Joint Stand State** slowly moves all joints to its zero position. This state is usually for checking if all actuators are working fine and all joint's zero positions are accurate. 

## State specification

State name  | Task name      | Joystick mapping | DDS mapping | Frequency
------------|----------------|------------------|-------------|-------------
Joint Stand | JointStandTask | LB+A             | 1           | 400Hz

Avaliable for hanging | Avaliable for standing | Auto Protection Switch
----------------------|------------------------|----------------
Yes                   | No                     | No

## Joystick Control

### Enter Joint Stand State

After initailize *AuroraCore*, press bumper `LB` and button `A` at the same time to enter joint stand state.

## Client Control

Velocity control | Stand pose control | Joint control | Joint parameter control
-----------------|--------------------|---------------|-------------------
No               | No                 | No            | No

### Enter Joint Stand State

After initailize *AuroraCore*, use aurora client's `set_fsm_state` function to enter joint stand state.

```python
client = AuroraClient.get_instance(domain_id=123, robot_name="gr3")   # initialize aurora client
time.sleep(1)

client.set_fsm_state(1)     # change to joint stand state
```




