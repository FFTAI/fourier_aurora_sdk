#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
requested_commit="${FOURIER_AURORA_SDK_COMMIT:-}"

if [[ -n "$(git -C "${project_root}" status --porcelain=v1 --untracked-files=all)" ]]; then
    echo "Commit-bearing artifacts require a completely clean worktree." >&2
    git -C "${project_root}" status --short --untracked-files=all >&2
    exit 1
fi

head_commit="$(git -C "${project_root}" rev-parse --verify HEAD^{commit} 2>/dev/null || true)"
if [[ ! "${head_commit}" =~ ^[0-9a-f]{40}$ ]]; then
    echo "Unable to resolve the checked-out SDK HEAD." >&2
    exit 1
fi

if [[ -z "${requested_commit}" ]]; then
    selected_commit="${head_commit}"
else
    if [[ ! "${requested_commit}" =~ ^[0-9a-fA-F]{8,40}$ ]]; then
        echo "FOURIER_AURORA_SDK_COMMIT must be an 8-40 character Git hash." >&2
        exit 1
    fi
    selected_commit="$(
        git -C "${project_root}" rev-parse --verify "${requested_commit}^{commit}" 2>/dev/null || true
    )"
    if [[ "${selected_commit}" != "${head_commit}" ]]; then
        echo "FOURIER_AURORA_SDK_COMMIT must resolve to the checked-out HEAD." >&2
        exit 1
    fi
fi

printf '%s\n' "${selected_commit:0:8}"
