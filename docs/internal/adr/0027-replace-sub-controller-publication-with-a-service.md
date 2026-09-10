---
status: accepted
---

# Replace sub-controller publication with a service

The SDK removes `publishSubControllerCommand()` and
`Endpoint::SubControllerCommandPublisher`. It adds the lease-exempt synchronous
`setSubController()` and `Endpoint::SubControllerService`, while
`resetUnicontroller()` delegates through the new service.

## Consequences

Aurora acknowledges accepted, no active Unicontroller, state-switch busy,
invalid request, and internal error outcomes. The SDK maps them to stable public
results and uses the existing service timeout rather than LeaseOption.

The service uses wire `uint32` values `0..4` in the order accepted, no active
Unicontroller, switch busy, invalid request, and internal error. Accepted maps to
SDK `SUCCESS`; failures map to reserved public codes `320..323`. Any other wire
value is `INVALID_AURORA_RESPONSE`.

`getLastSubControllerCommand()` updates only for an Aurora-accepted service
call. It no longer means merely written to DDS. Python mirrors
`set_sub_controller()` and retains `reset_unicontroller()`; neither operation
requires lease registration.
