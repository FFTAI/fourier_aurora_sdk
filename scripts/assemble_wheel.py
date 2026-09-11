#!/usr/bin/env python3
"""Assemble a fourier_aurora_client wheel from a staged package tree.

Used by the arm64 cross flow, where scikit-build-core cannot tag a
cross-compiled extension (it derives wheel tags from the host interpreter).
The staged tree must contain exactly one top-level package directory
(fourier_aurora_client/) holding __init__.py, client.py, the compiled
extension, and the vendored SDK shared library.
"""

import argparse
import base64
import csv
import hashlib
import io
import os
import sys
import zipfile

DIST_NAME = "fourier-aurora-client"
PACKAGE_DIR = "fourier_aurora_client"


def _record_hash(data):
    digest = hashlib.sha256(data).digest()
    return "sha256=" + base64.urlsafe_b64encode(digest).rstrip(b"=").decode("ascii")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--package-dir", required=True,
                        help="staged tree containing the fourier_aurora_client package")
    parser.add_argument("--version", required=True)
    parser.add_argument("--tag", required=True,
                        help="single wheel tag, e.g. cp312-cp312-linux_aarch64")
    parser.add_argument("--build-tag", default="",
                        help="optional build tag, e.g. 1g0123abcd")
    parser.add_argument("--license", required=True, dest="license_path")
    parser.add_argument("--requires-python", default="<3.14,>=3.10")
    parser.add_argument("--outdir", required=True)
    args = parser.parse_args()

    package_root = os.path.join(args.package_dir, PACKAGE_DIR)
    if not os.path.isdir(package_root):
        print(f"staged package directory missing: {package_root}", file=sys.stderr)
        return 2

    dist_info = f"{PACKAGE_DIR}-{args.version}.dist-info"
    metadata = (
        "Metadata-Version: 2.1\n"
        f"Name: {DIST_NAME}\n"
        f"Version: {args.version}\n"
        "Summary: Aurora C++ SDK with Python bindings\n"
        "License-File: LICENSE\n"
        f"Requires-Python: {args.requires_python}\n"
    )
    wheel_meta_lines = [
        "Wheel-Version: 1.0",
        "Generator: fourier-aurora-sdk-ci",
        "Root-Is-Purelib: false",
    ]
    if args.build_tag:
        wheel_meta_lines.append(f"Build: {args.build_tag}")
    wheel_meta_lines.append(f"Tag: {args.tag}")
    wheel_meta = "\n".join(wheel_meta_lines) + "\n"

    with open(args.license_path, "rb") as handle:
        license_data = handle.read()

    entries = []
    for root, _dirs, files in os.walk(package_root):
        for name in sorted(files):
            full = os.path.join(root, name)
            arc = os.path.relpath(full, args.package_dir)
            entries.append((arc, full))
    entries.sort(key=lambda item: item[0])

    metadata_files = {
        f"{dist_info}/METADATA": metadata.encode("utf-8"),
        f"{dist_info}/WHEEL": wheel_meta.encode("utf-8"),
        f"{dist_info}/licenses/LICENSE": license_data,
    }

    filename_parts = [PACKAGE_DIR, args.version]
    if args.build_tag:
        filename_parts.append(args.build_tag)
    filename_parts.append(args.tag)
    wheel_name = "-".join(filename_parts) + ".whl"
    os.makedirs(args.outdir, exist_ok=True)
    wheel_path = os.path.join(args.outdir, wheel_name)

    record_rows = []
    with zipfile.ZipFile(wheel_path, "w", zipfile.ZIP_DEFLATED) as zf:
        for arc, full in entries:
            with open(full, "rb") as handle:
                data = handle.read()
            zf.writestr(arc, data)
            record_rows.append((arc, _record_hash(data), str(len(data))))
        for arc, data in sorted(metadata_files.items()):
            zf.writestr(arc, data)
            record_rows.append((arc, _record_hash(data), str(len(data))))
        record_arc = f"{dist_info}/RECORD"
        record_rows.append((record_arc, "", ""))
        buffer = io.StringIO()
        writer = csv.writer(buffer, lineterminator="\n")
        writer.writerows(record_rows)
        zf.writestr(record_arc, buffer.getvalue().encode("utf-8"))

    print(wheel_path)
    return 0


if __name__ == "__main__":
    sys.exit(main())
