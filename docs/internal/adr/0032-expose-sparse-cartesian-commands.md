# Expose sparse Cartesian commands under one generic lease resource

The SDK exposes Cartesian commands as sparse pose, twist, and wrench vectors keyed by an opaque caller-defined name, because fixed arrays cannot distinguish omitted fields from zero commands and Aurora cannot infer lease ownership from arbitrary link aliases. Every publication containing Cartesian entries therefore requires `AuroraResource::CartesianCommand`; mixed messages additionally require their joint-group resources, while frame interpretation remains the caller and consuming task's responsibility.
