---
status: accepted
---

# Pause protected commands during lease replacement

`replaceLease()` makes the client-owned lease locally unusable before its first
wire transmission. Protected command publication remains paused until the SDK
has a safe definitive replacement outcome.

## Consequences

Replacement and protected publication share a linearization boundary. An
old-token command either completes its DDS write before the first replacement
transmission or is rejected locally; it cannot be published after Aurora may
have switched to the fresh token.

Same-stream command calls acquire their stream mutex before the brief lease-gate
admission step. They therefore serialize before consuming an in-flight permit;
different streams may still publish concurrently. Authority-changing operations
close only the gate and wait for admitted permits, never a stream mutex, avoiding
a reverse lock edge and queued same-stream work in the drain count.

Successful replacement atomically installs the new token and resumes commands.
A definitive rejection restores the unchanged old lease only while its
Conservative Client Deadline remains usable. An uncertain timeout does not
resume commands; after the public abandonment boundary, even exact late
resolution is cleanup-only and cannot restore authority.
