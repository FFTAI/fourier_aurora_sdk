# Dependency Discovery

Stage 1 depends on internal system packages:

- `fourierdds`
- generated `fourier_dds_msgs`
- FastDDS/FastCDR transitively through those packages

The local source config files expose CMake package names:

- `fourier_dds`
- `fourier_dds_msgs`

The current checked-in generated message tree is missing several Aurora SDK target headers (`RobotMotionCmd`, `RobotMotionState`, `RobotControlGroupCmd`, `RobotControlGroupState`). Build and install `fourierddsmsgs` from current IDL before enabling the concrete DDS transport.
