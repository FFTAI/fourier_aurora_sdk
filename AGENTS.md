# Developer and Agent Guide

Fourier Aurora SDK provides a C++17 client and Python bindings for Aurora robots.
These guidelines apply to this repository.

## Start here

- Quick start: [English](README.md) / [中文](README_CN.md).
- Repository layout, dependencies, build and test commands:
  [English](docs/overview.md) / [中文](docs/overview_CN.md).
- API references: [C++](docs/cpp_api.md) / [C++ 中文](docs/cpp_api_CN.md),
  [Python](docs/python_api.md) / [Python 中文](docs/python_api_CN.md).
- Examples and CLI usage: [English](docs/examples_and_clients.md) /
  [中文](docs/examples_and_clients_CN.md).
- Package version: [VERSION](VERSION). Runtime compatibility:
  [REVISIONS.yaml](REVISIONS.yaml).

## Making changes

- Read the relevant code and documentation first. Keep changes focused and
  preserve unrelated work.
- Implement shared behavior in `src/`. Keep Python bindings and wrappers in
  `python/` aligned with the C++ API in `include/fourier_aurora_sdk/`.
- Keep generated DDS types behind the transport implementation; public APIs
  use SDK-owned types.
- When behavior or an API changes, update the relevant tests, examples, and
  both English and Chinese user documentation.
- Keep source and package contents suitable for public distribution. Avoid
  private service addresses, credentials, and environment-specific build paths.

## Validating changes

Prepare the dependencies described in the build guide, then run checks relevant
to the change. A typical code-change check is:

```bash
bash scripts/check_version_consistency.sh
JOBS=4 bash scripts/test_debug.sh
```

The debug profile runs C++ and Python tests with a mock transport. For transport
changes, also use [the DDS loopback check](scripts/test_dds_transport.sh).
Run robot-command examples on hardware only when explicitly requested.

For packaging changes, follow the [package validation guide](docs/overview.md#validate-source-and-binary-packages)
([中文](docs/overview_CN.md#验证源码包和安装包)), including source archive checks.
Report what changed, which checks ran, and any remaining validation limits.
