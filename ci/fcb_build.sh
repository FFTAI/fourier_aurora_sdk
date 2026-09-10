#!/usr/bin/env bash
set -euo pipefail
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${FCB_HELPER_DIR:?FCB_HELPER_DIR is required}/fcb_entrypoint.sh"
fcb_init fourier_aurora_sdk "$repo_root"
if fcb_dry_run "ci/build_packages.sh arch=$FCB_ARCH jobs=$FCB_JOBS"; then exit 0; fi
bash "$repo_root/ci/build_packages.sh"
fcb_collect_artifacts deb "$repo_root/dist"
fcb_collect_artifacts whl "$repo_root/dist"
