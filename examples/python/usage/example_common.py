from __future__ import annotations

import argparse
import os
import signal
import sys
import threading
import time
from typing import Callable, TypeVar

T = TypeVar("T")

_stop_requested = False


def _handle_stop_signal(signum, frame):  # noqa: ARG001
    global _stop_requested
    _stop_requested = True


def install_stop_signal_handlers() -> None:
    signal.signal(signal.SIGINT, signal.default_int_handler)
    signal.signal(signal.SIGTERM, _handle_stop_signal)


def stop_requested() -> bool:
    return _stop_requested


def make_parser(default_user_id: str, description: str) -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument("domain_id", nargs="?", type=int, default=123)
    parser.add_argument("user_id", nargs="?", default=default_user_id)
    return parser


def wait_for(getter: Callable[[], T], name: str, attempts: int = 25, interval: float = 0.2) -> T:
    last_error: Exception | None = None
    for _ in range(attempts):
        try:
            return getter()
        except Exception as error:  # noqa: BLE001
            last_error = error
            time.sleep(interval)
    if last_error is not None:
        raise last_error
    return getter()


def close_client(client, exit_code: int = 0) -> None:
    if client is None:
        return

    done = threading.Event()
    error_holder: list[Exception] = []

    def close_worker() -> None:
        try:
            client.close()
            time.sleep(0.5)
        except Exception as error:  # noqa: BLE001
            error_holder.append(error)
        finally:
            done.set()

    thread = threading.Thread(target=close_worker, daemon=True)
    thread.start()
    thread.join(timeout=2.5)

    if not done.is_set():
        print("AuroraClient close timed out; forcing process exit.", file=sys.stderr)
        sys.stderr.flush()
        sys.stdout.flush()
        os._exit(exit_code)

    if error_holder:
        print(f"AuroraClient close failed: {error_holder[0]}", file=sys.stderr)


def print_vector(values) -> None:
    print("[" + ", ".join(str(value) for value in values) + "]", end="")


def print_service_result(result) -> None:
    print(
        f"result_code={result.result_code} "
        f"success={'true' if result.success() else 'false'} "
        f"timed_out={'true' if result.timed_out else 'false'} "
        f"transport_error={'true' if result.transport_error else 'false'}",
        end="",
    )
    if result.message:
        print(f' message="{result.message}"', end="")
    print()


def wait_for_enter(prompt: str) -> bool:
    if stop_requested():
        return False
    try:
        input(prompt)
    except EOFError:
        return False
    return not stop_requested()
