#!/usr/bin/env python3
"""Check a release source tarball without extracting or executing its contents."""

import argparse
from pathlib import PurePosixPath
import re
import tarfile


REQUIRED = {
    "LICENSE", "VERSION", "REVISIONS.yaml", "README.md", "README_CN.md",
    "AGENTS.md", "CHANGELOG", "CMakeLists.txt", "pyproject.toml",
    "include/fourier_aurora_sdk/aurora_client.hpp",
    "src/aurora_client.cpp", "src/internal/transport.hpp",
    "python/bindings/module.cpp", "python/fourier_aurora_client/__init__.py",
    "scripts/build_deb.sh", "scripts/build_wheel.sh", "scripts/build_packages.sh",
    "scripts/check_source_package.py", "tests/cpp/test_client_contract.cpp",
    "docs/overview.md", "docs/overview_CN.md", "docs/cpp_api.md",
    "docs/cpp_api_CN.md", "docs/python_api.md", "docs/python_api_CN.md",
    "docs/examples_and_clients.md", "docs/examples_and_clients_CN.md",
}
PRIVATE_PREFIXES = ("docs/internal/", "ci/", "fcb-output/")
PRIVATE_FILES = {"CONTEXT.md"}
PRIVATE_URL = re.compile(rb"(?:https?://|ssh://)[^\s<>\"']*\.fftaicorp\.com\b")


def check_archive(filename):
    files = set()
    roots = set()
    with tarfile.open(filename) as archive:
        for member in archive:
            path = PurePosixPath(member.name)
            if path.is_absolute() or ".." in path.parts:
                raise ValueError(f"Unsafe archive path: {member.name}")
            if not member.isdir() and not member.isfile():
                raise ValueError(f"Unsupported archive entry: {member.name}")
            if member.isdir():
                continue
            if len(path.parts) < 2:
                raise ValueError("Source files must be inside one top-level directory")
            roots.add(path.parts[0])
            relative = PurePosixPath(*path.parts[1:])
            name = relative.as_posix()
            if name in files:
                raise ValueError(f"Duplicate archive entry: {name}")
            if name in PRIVATE_FILES or name.startswith(PRIVATE_PREFIXES):
                raise ValueError(f"Private material in source package: {name}")
            if any(part in {".git", ".venv", "dist", "__pycache__"}
                   or part.startswith("build") for part in relative.parts[:-1]):
                raise ValueError(f"Build or checkout output in source package: {name}")
            if name.endswith((".deb", ".whl", ".so", ".pyc")) or ".so." in relative.name:
                raise ValueError(f"Binary artifact in source package: {name}")
            stream = archive.extractfile(member)
            assert stream is not None
            content = stream.read()
            if content.startswith(b"\x7fELF"):
                raise ValueError(f"ELF binary in source package: {name}")
            if PRIVATE_URL.search(content):
                raise ValueError(f"Private service URL in source package: {name}")
            files.add(name)
    if len(roots) != 1:
        raise ValueError("Source package must contain exactly one top-level directory")
    missing = REQUIRED - files
    if missing:
        raise ValueError("Missing release sources: " + ", ".join(sorted(missing)))
    return len(files)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", help="Source .tar or .tar.gz archive")
    args = parser.parse_args()
    try:
        count = check_archive(args.archive)
    except (OSError, tarfile.TarError, ValueError) as error:
        parser.exit(1, f"Source package check failed: {error}\n")
    print(f"Source package passed content checks ({count} files).")


if __name__ == "__main__":
    main()
