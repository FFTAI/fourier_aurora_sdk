#!/usr/bin/env python3
from __future__ import annotations

import time

from example_common import close_client, install_stop_signal_handlers, make_parser, print_service_result
from fourier_aurora_client import AuroraClient


def main() -> int:
    parser = make_parser("python_set_state", "Repeatedly request Aurora FSM state changes.")
    args = parser.parse_args()
    install_stop_signal_handlers()

    client = None
    exit_code = 0
    try:
        client = AuroraClient.get_instance(domain_id=args.domain_id, user_id=args.user_id)
        time.sleep(1.0)

        while True:
            value = input("FSM state id> ").strip()
            if not value:
                continue
            if not value.isdigit():
                print("Invalid state id: enter an integer.")
                continue

            state_id = int(value)
            result = client.requestFsmState(state_id)
            print(f"requestFsmState({state_id}) ", end="")
            print_service_result(result)
            time.sleep(1.0)
    except (EOFError, KeyboardInterrupt):
        return 0
    except Exception as error:  # noqa: BLE001
        print(f"set_state failed: {error}")
        exit_code = 1
        return 1
    finally:
        close_client(client, exit_code)


if __name__ == "__main__":
    raise SystemExit(main())
