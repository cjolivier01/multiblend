#!/usr/bin/env bash
set -euo pipefail

prefix="/usr/local"
binary_path=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --prefix=*) prefix="${1#*=}"; shift ;;
    --prefix) shift; prefix="${1:-$prefix}"; shift || true ;;
    --binary=*) binary_path="${1#*=}"; shift ;;
    --binary) shift; binary_path="${1:-}"; shift || true ;;
    *) echo "Unknown arg: $1" >&2; exit 2 ;;
  esac
done

if [[ -z "${binary_path}" ]]; then
  echo "Missing --binary path (internal)." >&2
  exit 2
fi

dest_dir="${prefix%/}/bin"
dest_path="${dest_dir}/multiblend"

echo "Installing ${binary_path} -> ${dest_path}"
mkdir -p "${dest_dir}"

if command -v install >/dev/null 2>&1; then
  install -m 0755 "${binary_path}" "${dest_path}"
else
  cp -f "${binary_path}" "${dest_path}"
  chmod 0755 "${dest_path}"
fi

echo "Installed to ${dest_path}"

