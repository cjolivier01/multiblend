#!/usr/bin/env bash
set -euo pipefail

print_usage() {
  cat <<'USAGE'
Usage: ./install.sh [--prefix PATH] [-- ARGS...]

Runs Bazel's install_tree target. If --prefix is provided, the staged binary
tree is mirrored into PATH. Extra arguments after -- are forwarded to
tools/install_tree.sh.
USAGE
}

PREFIX=""
FORWARD=()

while [[ $# -gt 0 ]]; do
  case "$1" in
    -h|--help)
      print_usage
      exit 0
      ;;
    --prefix)
      if [[ $# -lt 2 ]]; then
        echo "--prefix requires a path argument" >&2
        exit 2
      fi
      PREFIX="$2"
      shift 2
      ;;
    --prefix=*)
      PREFIX="${1#*=}"
      shift 1
      ;;
    --)
      shift
      FORWARD+=("$@")
      break
      ;;
    *)
      FORWARD+=("$1")
      shift 1
      ;;
  esac
done

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROOT}"

RUN_ARGS=()
if [[ -n "${PREFIX}" ]]; then
  RUN_ARGS+=(--prefix="${PREFIX}")
fi

exec bazel run //:install_tree -- "${RUN_ARGS[@]}" "${FORWARD[@]}"
