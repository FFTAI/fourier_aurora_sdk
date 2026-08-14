# Source Name Assignment

`source_name` is the required, human-readable identity of an SDK client and of
its lease-management operations. Direct C++ and Python SDK callers must set it
explicitly before `start()`. Valid names match `[A-Za-z0-9._-]{1,64}` and are
compared exactly, including case.

Choose names by stable module role when a deployment benefits from predictable
authority snapshots and logs:

| Source Name | Stable module role | Owner | Emergency eligible | Notes |
|---|---|---|:---:|---|
|  |  |  | no |  |

Concurrently running modules should use distinct names. Aurora does not use
Source Name as the command token: after registration, the authority-issued
64-bit `lease_id` identifies commands. Reusing a live holder's Source Name can
still collide with that holder's lease-management lifecycle.

Emergency eligibility must match Aurora's exact, case-sensitive
`LeaseConfig.emergency_source_names` allowlist; enabling the SDK opt-in alone
does not grant that privilege.

The packaged `aurora-client` wrapper is the exception to explicit naming. It
generates `aurora-client-NNNN` once per valid robot-facing invocation unless
the user supplies `--source_name`.
