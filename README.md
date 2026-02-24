# Fourier Aurora SDK

This is the SDK repository for **Fourier Aurora** *(Advanced Unified Robot Operation and Resource Architecture)*.

If you are new to Aurora, please refer to the fourier document center to learn about the system:

- [Fourier Document Center](https://support.fftai.com)

## v1.3.0 Release

Support Robots:

- GR-1P
- GR-2
- GR-3
- Fourier-N1

Prerequisites:

- Actuator version:
  - Communication firmware version 0.3.12.31 or above.
  - Driver firmware version 0.2.10.30 or above.
  - NOTE: Actuator version can be upgraded using **FSA Assistant**. Click [FSA Assistant for Linux](https://fsa-1302548221.cos.ap-shanghai.myqcloud.com/tool/FSA_Assistant/FSA_Assistant_V0.0.1.24_155_31_x64_Linux_2025-07-08.tar.gz) to download the latest version.
 
## Repository Structure

```
fourier_aurora_sdk/
├── config/               # Configuration files
│   └── config.yaml      # Main configuration file
├── python/              # Python SDK
│   ├── example/        # Example scripts for each robot model
│   │   ├── fouriern1/  # Fourier-N1 examples
│   │   ├── gr1p/       # GR-1P examples
│   │   ├── gr2/        # GR-2 examples
│   │   └── gr3/        # GR-3 examples
│   └── docs/           # API documentation
│       ├── CN/         # Chinese API docs
│       └── EN/         # English API docs
├── sim/                 # Simulation tools
│   └── mujoco/         # MuJoCo simulator integration
├── docker_run.bash      # Docker run script
├── LICENSE              # Apache 2.0 License
├── README.md            # This file
├── README_CN.md         # Chinese README
├── CHANGELOG            # Version history
└── VERSION              # Current version
```

## Installation

### Aurora

please refer to [Fourier Document Center](https://support.fftai.com).

## Fourier Aurora Client

```
pip install fourier_aurora_client==0.1.8
```

## Issues

Please report any issues or bugs! We will do our best to fix them.

## License

[Apache 2.0](LICENSE) © Fourier
