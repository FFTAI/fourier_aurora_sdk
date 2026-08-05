---
status: accepted
---

# Require explicit Emergency-priority opt-in

The public SDK exposes `LeasePriority::Normal` and
`LeasePriority::Emergency`, with Normal as the acquisition default. A
disabled-by-default `ConnectionOptions::allow_emergency_lease_priority` guard
must be enabled before the SDK sends an Emergency acquisition or replacement.

## Consequences

Routine clients cannot request takeover accidentally. Enabling the SDK option
does not authorize Emergency priority: Aurora still checks the Source ID
against its privileged allowlist and may return `PRIORITY_NOT_ALLOWED`.

Resource-only replacement preserves the current priority by default. An
explicit downgrade from Emergency to Normal does not require the opt-in, while
every outgoing Emergency request does.
