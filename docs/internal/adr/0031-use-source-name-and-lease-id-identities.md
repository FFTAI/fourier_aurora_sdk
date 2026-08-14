---
status: accepted
---

# Use Source Name and Lease ID identities

Remove numeric `source_id` from the lease-management and command protocols.
Every direct SDK client supplies one human-readable Source Name matching
`[A-Za-z0-9._-]{1,64}`. Aurora compares it exactly and uses it for management
ownership, replay scope, authority snapshots, diagnostics, and the Emergency
Source allowlist.

After successful registration, the authority-issued nonzero 64-bit `lease_id`
is the sole lease identity carried by protected commands. Command admission
does not re-check Source Name. The probability of two live authority-issued
Lease IDs colliding is accepted as negligible; no additional Source ID is kept
for collision defense. Per-stream command `sequence_id` validation is unchanged.

`LeaseInfo` is a pod message because it is embedded by multiple command
messages. Move it from `fourier_msgs::msg` to `fourier_msgs_pod::msg`, retaining
only `lease_id` and `sequence_id`.

## Consequences

`ConnectionOptions.source_name` is mandatory for every C++ and Python client
configuration, including read-only configurations, and `start()` rejects a
missing or invalid name before transport creation. There is no mixed-version
wire compatibility and all participants must update together.

The packaged `aurora-client` wrapper remains convenient for interactive use:
after it has parsed a valid robot-facing command, it generates one
`aurora-client-NNNN` Source Name unless `--source_name` is supplied. Direct SDK
callers and standalone examples do not receive a default.

This supersedes ADR 0006. Older ADRs that describe Source ID remain historical
records of their original decisions unless separately superseded.
