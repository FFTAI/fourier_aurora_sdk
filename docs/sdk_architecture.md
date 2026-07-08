# Aurora SDK Architecture

The SDK uses a canonical C++ client under `fourier::aurora_sdk` and exposes Python through pybind11.

Public APIs use SDK-owned structs only. Generated DDS message types and `fourierdds` entities are implementation details inside the transport layer.

Stage 1 uses a singleton client. `close()` tears down callbacks, transport endpoints, cached state, and singleton ownership so a later `getInstance()` can create a new client.

The C++ callback extension points run inline on the DDS callback thread after SDK cache update. Python does not expose these callback hooks in stage 1.

DDS transport is intentionally single-backend (`fourierdds`). A mock transport exists for unit tests and does not represent a public backend abstraction.
