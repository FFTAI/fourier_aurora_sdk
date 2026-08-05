---
status: accepted
---

# Replace user ID with one module-level lease Source ID

The SDK's former string `user_id` is replaced by the protocol's integer
`source_id`, shared by every Managed Control Lease from the module's
process-wide client. “Module-level” means one stable deployed process role, not
one ephemeral OS process invocation, so the ID remains unchanged across module
restarts. A separate `source_name` is optional and display-only; when omitted,
the SDK transmits `aurora_sdk_{source_id}`, using the decimal Source ID. This
avoids two names for the same intended identity while keeping authorization and
sequence state independent of an operator-facing label.

## Consequences

Source ID is supplied by the deployment or application and remains stable for
the same logical command source across process restarts. The SDK does not
generate it, hash a string into it, or allow acquisitions under several Source
IDs through one high-level singleton. A module may hold at most one active
lease under that Source ID and changes its complete resource set through
atomic replacement. Privileged-source configuration is keyed by numeric Source
ID; changing `source_name` changes only diagnostics. SDK lease-management
state is memory-only under ADR 0010.

Zero is reserved as unconfigured and is never transmitted as a Lease Holder.
When lease management or any protected command publisher is enabled,
`start()` requires a nonzero Source ID and fails before transport creation
otherwise. Subscriber-only and lease-exempt-service-only configurations may
run without one.

`ConnectionOptions` contains `uint32_t source_id`, optional `source_name`, and the
Emergency opt-in. `configure()` while Stopped is the sole identity mutator;
`config()` returns the optional configured name, while `LeaseResult` exposes the
effective fallback name used on the wire. SDK 2.0.1 removes the unreleased
string-only `setUserId()` / `userId()` pair without a compatibility wrapper or a
second convenience setter.
