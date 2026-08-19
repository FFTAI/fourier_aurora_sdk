---
status: superseded
---

# Release the unreleased lease API as 2.0.1

This decision was superseded before release when the first supported package
and ABI line was reset to 1.0.1.

SDK 2.0.0 has not been released. The lease-capable C++/Python surface therefore
becomes the first supported API on ABI line 2 and is published as 2.0.1 with
`SOVERSION 2` unchanged.

## Consequences

Aurora, message IDLs, SDK artifacts, examples, and manifest pins cut over as one
tested set. There is no lease-bypass configuration or misleading compatibility
wrapper for the removed sub-controller publisher. Development artifacts labeled
2.0.0 are superseded and are not promised wire or API compatibility.

Package version and endpoint readiness provide feature detection. Fake clocks,
mock ManageLease transport, and authority fault injection remain internal test
seams rather than production options.
