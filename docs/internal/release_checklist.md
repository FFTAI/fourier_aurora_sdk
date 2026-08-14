# Release Checklist

SDK versioning is independent from the Aurora runtime version. SDK `3.0.0`
may support an Aurora `4.0.x` runtime when that pair is explicitly tested and
recorded; matching their major versions would incorrectly imply one shared
compatibility contract.

## Stage One

- Linux amd64 C++ Debian package built with CPython disabled.
- Linux amd64 CPython 3.10 standard-GIL wheel.
- Exact `fourier_dds 1.2.0-1` and `fourier-dds-msgs 0.2.0-1` dependency pair
  recorded in `REVISIONS.yaml` and enforced by Debian metadata.
- Internal Debian names use `3.0.0+g<eight-character-head>` and internal wheel
  names use build tag `1g<eight-character-head>`.
- Commit-bearing builds require a completely clean worktree and a hash that
  resolves to the checked-out `HEAD`.
- Public-style clean names are verification inputs only. Do not publish or tag
  `3.0.0` during this internal stage.

## CI Integration

- Build and execute the Linux arm64 Debian and CPython 3.10 cells on an
  approved arm64 environment.
- Build architecture-matched FourierDDS and generated-message packages from
  the same pinned revisions; do not reuse the amd64 Debian artifacts.
- Apply the same package manifest, ELF, isolated consumer, and DDS loopback
  checks used for amd64.
- Add default Release CI with tests disabled, then dedicated test jobs for the
  fixed Debug, Release, ASan+UBSan, TSan, and DDS profiles.

## Public Release

- Decide whether any internal `2.0.0+g<hash>` Debian or metadata-version
  `2.0.0` wheel escaped the controlled channel. If it did, select at least
  `3.0.0` for a clean public upgrade; never replace distributed content under
  the same clean version.
- Produce Linux Debian packages for amd64 and arm64.
- Produce standard-GIL Linux wheels for CPython 3.9 through 3.13 on amd64 and
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
