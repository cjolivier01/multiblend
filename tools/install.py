#!/usr/bin/env python3
import argparse
import os
import shutil
import sys

def default_install_prefix() -> str: 
    if "CONDA_PREFIX" in os.environ:
        return os.path.join(os.environ["CONDA_PREFIX"])
    return "/usr/local"

def main():
    parser = argparse.ArgumentParser(description="Install multiblend binary into <prefix>/bin")
    parser.add_argument("--prefix", default=default_install_prefix(), help="installation prefix (default: /usr/local)")
    parser.add_argument("--binary", default=None, help="path to built multiblend (internal)")
    args, extra = parser.parse_known_args()

    # In bazel run, the runfiles location exposes the data file. Try to resolve from env if not given.
    binary = args.binary
    if binary is None:
        # Best-effort: look for multiblend alongside runfiles tree
        runfiles = os.environ.get("RUNFILES_DIR") or os.environ.get("TEST_SRCDIR")
        if runfiles:
            candidate = os.path.join(runfiles, "__main__/multiblend")
            if os.path.exists(candidate):
                binary = candidate
    if binary is None or not os.path.exists(binary):
        print("Error: cannot locate built multiblend binary; pass --binary or run via bazel run with data dependency.", file=sys.stderr)
        return 2

    prefix = args.prefix.rstrip("/")
    dest_dir = os.path.join(prefix, "bin")
    os.makedirs(dest_dir, exist_ok=True)
    dest = os.path.join(dest_dir, "multiblend")

    shutil.copy2(binary, dest)
    os.chmod(dest, 0o755)
    print(f"Installed to {dest}")
    return 0

if __name__ == "__main__":
    sys.exit(main())

