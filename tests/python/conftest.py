import importlib

import pytest

native = importlib.import_module(
    "fourier_aurora_client._fourier_aurora_sdk"
)


@pytest.fixture(autouse=True)
def reset_native_singleton():
    native._testing_reset()
    yield
    native._testing_reset()
