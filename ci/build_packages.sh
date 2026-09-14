#!/usr/bin/env bash
set -euo pipefail

# Compatibility entrypoint for existing direct CI callers.
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
exec bash "${repo_root}/scripts/build_packages.sh" "$@"
