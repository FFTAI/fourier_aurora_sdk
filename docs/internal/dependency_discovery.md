# Dependency Discovery

> **3.0.0 release amendment:** The coordinated Source Name protocol requires
> `fourier-dds-msgs 0.2.0-1`. The 0.1.0 artifact and hash below remain the last
> committed baseline; replace them with the verified 0.2.0 artifact metadata
> when the coordinated component worktrees receive commit hashes.

Stage one depends on this exact amd64 package pair:

| Component | Package version | Source revision | Validated artifact |
| --- | --- | --- | --- |
| FourierDDS | `fourier_dds 1.2.0-1` | `d209ad3c339540ae737ae8f24722186c1631bba2` | `fourierdds-1.2.0-d209ad3c-amd64.deb` |
| Generated messages | `fourier-dds-msgs 0.1.0-1` | `18a24585984e04be401cf1acc0151d5b5e95b2cd` | `fourierddsmsgs-0.1.0-18a24585-0e930b.deb` |

Validated SHA-256 values:

```text
3767a594d985e466019f64212ca7585b950d322fbf72f8bb31845317144bab90  fourierdds-1.2.0-d209ad3c-amd64.deb
b594b0d74a4d4962233e16a5d8004f843d8319712fb5c1c0927fb9c08e89bd00  fourierddsmsgs-0.1.0-18a24585-0e930b.deb
```

FastDDS and FastCDR are runtime dependencies supplied by the build image and
the pinned packages. `REVISIONS.yaml` is the machine-readable compatibility
mapping for SDK `2.0.0`.

Extract both Debian artifacts beneath one staging root and pass its
`opt/fftai` directory as the only SDK dependency prefix:

```bash
-DFOURIER_AURORA_SDK_DDS_PREFIX=/tmp/aurora_sdk_impl/prefix/opt/fftai
```

CMake resolves only these package and message locations from that prefix:

- `fourier_dds`
- `lib/cmake/fourier_dds/fourier_ddsConfig.cmake`
- `fourier_dds_msgs/include/fourier_dds_msgs`
- `fourier_dds_msgs/lib`

The configure intentionally uses `NO_DEFAULT_PATH`. It must fail if the exact
staged prefix is missing; do not combine headers from a source checkout with
libraries from `/opt` or another package revision.

The generated Aurora headers include `RobotMotionCmd`, `RobotMotionState`,
`RobotControlGroupCmd`, and `RobotControlGroupState`. In a normal installation
they are under:

```text
/opt/fftai/fourier_dds_msgs/include/fourier_dds_msgs
```

Do not copy generated classes into this SDK repository. The Linux arm64 CI jobs
must consume architecture-matched packages built from the same pinned source
revisions; the amd64 artifacts above are not arm64 validation.

The internal Python wheel deliberately does not bundle or declare these Debian
runtime packages through pip. Install the exact native dependency pair and make
their library directories visible to the loader. The eventual public release
must either document a matching apt runtime or bundle legally redistributable
native dependencies under an approved Linux platform policy.
