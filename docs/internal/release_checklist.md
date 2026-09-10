# Release Checklist

SDK versioning is independent from the Aurora runtime version. SDK `1.0.1`
may support an Aurora `4.0.x` runtime when that pair is explicitly tested and
recorded; matching their major versions would incorrectly imply one shared
compatibility contract.

## Stage One

- Linux amd64 C++ Debian package built with CPython disabled.
- Separate Linux amd64 standard-GIL wheels for CPython 3.10 through 3.13.
- Exact `fourier_dds 1.2.0-1` and `fourier-dds-msgs 0.2.0-1` dependency pair
  recorded in `REVISIONS.yaml` and enforced by Debian metadata.
- Internal Debian names use `1.0.1+g<eight-character-head>` and internal wheel
  names use build tag `1g<eight-character-head>`.
- Commit-bearing builds require a completely clean worktree and a hash that
  resolves to the checked-out `HEAD`.
- Public-style clean names are verification inputs only. Do not publish or tag
  `1.0.1` during this internal stage.

## CI Integration

- Build and execute the Linux arm64 Debian and CPython 3.10 through 3.13 cells
  on an approved arm64 environment.
- Build architecture-matched FourierDDS and generated-message packages from
  the same pinned revisions; do not reuse the amd64 Debian artifacts.
- Apply the same package manifest, ELF, isolated consumer, and DDS loopback
  checks used for amd64.
- Add default Release CI with tests disabled, then dedicated test jobs for the
  fixed Debug, Release, ASan+UBSan, TSan, and DDS profiles.

## Public Release

- Confirm that no earlier unreleased development version escaped the controlled
  channel; never replace distributed content under the same clean version.
- Produce Linux Debian packages for amd64 and arm64.
- Produce standard-GIL Linux wheels for CPython 3.10 through 3.13 on amd64 and
  arm64. Exclude free-threaded Python, 32-bit targets, Windows, and macOS.
- Choose and prove a Linux/glibc policy. Do not relabel an unverified
  `linux_*` wheel as manylinux.
- Decide whether wheels bundle legally redistributable native dependencies or
  require a documented exact apt runtime.
- Remove commit build tags from final public artifact names.
- Establish the first supported C++ API/ABI baseline and add cross-release
  comparison from that release onward.
- Complete real-robot recipe validation records and add standalone GR3 Mini
  recipes under their own robot directory.
- Publish external tutorials, robot capability tables, governance and support
  policy, `SECURITY.md`, and the approved disclosure contact/process.
- Complete third-party dependency, license, and attribution review; include
  every required license and approved `NOTICE` text in source and binary
  artifacts.
