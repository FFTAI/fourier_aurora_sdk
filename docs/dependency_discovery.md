# Dependency Discovery

Stage 1 depends on internal system packages:

- `fourierdds`
- generated `fourier_dds_msgs`
- FastDDS/FastCDR transitively through those packages

The local source config files expose CMake package names:

- `fourier_dds`
- `fourier_dds_msgs`

In `aurora_medium_latest_container`, the generated Aurora headers, including
`RobotMotionCmd`, `RobotMotionState`, `RobotControlGroupCmd`, and
`RobotControlGroupState`, are installed under:

```text
/opt/fftai/fourier_dds_msgs/include/fourier_dds_msgs
```

Record the installed message-package revision before building. If that package
must be restored, use the matching message Debian artifact under
`/workspace/output/`; do not copy generated classes into this SDK repository or
mix headers and libraries from different message-package revisions.
