import importlib

import pytest

from fourier_aurora_client.client import AuroraClient

native = importlib.import_module(
    "fourier_aurora_client._fourier_aurora_sdk"
)


@pytest.fixture(autouse=True)
def reset_native_singleton():
    native._testing_reset()
    AuroraClient._instance = None
    yield
    AuroraClient._instance = None
    native._testing_reset()
