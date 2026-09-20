#!/usr/bin/env python3
"""
Project Decoder Script
Extracts all 5G Base Station Simulation project files and directory structure from project_bundle.txt.

Usage:
    python3 decode.py [bundle_file.txt] [target_directory]
"""

import os
import sys
import json
import base64

def decode_bundle(bundle_file="project_bundle.txt", target_dir="."):
    if not os.path.exists(bundle_file):
        print(f"[ERROR] Bundle file '{bundle_file}' not found.")
        sys.exit(1)

    print(f"=========================================================")
    print(f"  5G Base Station Simulation Project Decoder           ")
    print(f"=========================================================")
    print(f"[DECODER] Reading bundle file '{bundle_file}'...")

    with open(bundle_file, "r", encoding="utf-8") as f:
        data = json.load(f)

    extracted_count = 0
    for rel_path, content_b64 in data.items():
        out_path = os.path.normpath(os.path.join(target_dir, rel_path))
        os.makedirs(os.path.dirname(out_path), exist_ok=True)

        content_bytes = base64.b64decode(content_b64)
        with open(out_path, "wb") as out_file:
            out_file.write(content_bytes)

        # Preserve executable permissions for scripts
        if rel_path.endswith(".sh") or rel_path.endswith(".py"):
            try:
                os.chmod(out_path, 0o755)
            except Exception:
                pass

        print(f"  [+] Extracted: {rel_path}")
        extracted_count += 1

    print(f"=========================================================")
    print(f"[DECODER SUCCESS] Extracted {extracted_count} project files to '{target_dir}'.")
    print(f"=========================================================")
    print(f"Next steps to build and run:")
    print(f"  1. chmod +x manage.sh run_sim.sh scripts/*.sh")
    print(f"  2. ./manage.sh build")
    print(f"  3. ./run_sim.sh")
    print(f"=========================================================")

if __name__ == "__main__":
    bundle_path = sys.argv[1] if len(sys.argv) > 1 else "project_bundle.txt"
    out_dir = sys.argv[2] if len(sys.argv) > 2 else "."
    decode_bundle(bundle_path, out_dir)
