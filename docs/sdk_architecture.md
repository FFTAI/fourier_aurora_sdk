# Aurora SDK Architecture

The SDK uses a canonical C++ client under `fourier::aurora_sdk` and exposes
Python through pybind11.

Public high-level client APIs use SDK-owned structs. Generated DDS message
types are supplied by the separately managed message module and do not appear
in high-level client signatures.

Stage 1 supports one robot per process through a stable process-wide singleton.
Parameterless `getInstance()` initializes or returns the client without
starting transport. Configuration can be changed while the client is
`Stopped`; `start()` creates the configured endpoints, and synchronous `stop()`
tears down transport while preserving the client for later reconfiguration and
restart. Multi-robot control belongs to separate processes or another
orchestration layer.

The C++ callback extension points run inline on the DDS callback thread after
SDK cache update. Python does not expose these callback hooks in stage 1.

FourierDDS is the only stage-one backend and its concrete transport facade is
public. Backend selection, `ITransport`, mock injection, and singleton test
factories remain private test or implementation facilities.

Internally, a private adapter implements `ITransport` and owns the public
`FourierDdsTransport` facade by composition. This lets the client and tests use
one private transport contract without exposing inheritance or duplicating the
production FourierDDS implementation.

C++ callback handler aliases remain public in the SDK-owned `callbacks.hpp` so
the public client and transport facade do not depend on the private transport
interface. Python callback registration is not part of stage one.
