#!/usr/bin/env bash
#
# Remove all build output and rebuild SpeedCrunch from scratch.
#
# Usage: scripts/build.sh [options]
#
#   (no options)   Clean, then configure and build a Release app in ./build
#   --clean-only   Only remove build output, don't rebuild
#   --dmg          Clean, then run scripts/generate-macos-dmg.sh (macOS only).
#                  CODESIGN_IDENTITY / NOTARY_PROFILE are passed through, so
#                  set them for a signed and notarized DMG.
#   --tests        Also build the tests (-DBUILD_TESTING=ON) and run ctest
#   --all          Also delete packaged DMGs in build/dmg (kept by default)
#   --dry-run      Print what would be deleted and run, without doing it
#   -h, --help     Show this help
#
# Environment:
#   BUILD_TYPE     CMake build type (default: Release)
#   JOBS           Parallel build jobs (default: number of CPUs)
#   CMAKE_PREFIX_PATH  Qt 6 location; on macOS defaults to Homebrew's qt
#   DEVELOPER_DIR  macOS toolchain. If unset and the active developer dir is
#                  the Command Line Tools (which can lose the C++ standard
#                  headers after macOS updates), Xcode.app is used when present.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="$ROOT_DIR/src"
BUILD_DIR="$ROOT_DIR/build"
BUILD_TYPE="${BUILD_TYPE:-Release}"

mode="build"
with_tests=0
delete_dmgs=0
dry_run=0

usage() { sed -n '2,/^set -euo/p' "${BASH_SOURCE[0]}" | sed '$d' | sed 's/^# \{0,1\}//'; }

while [[ $# -gt 0 ]]; do
  case "$1" in
    --clean-only) mode="clean" ;;
    --dmg) mode="dmg" ;;
    --tests) with_tests=1 ;;
    --all) delete_dmgs=1 ;;
    --dry-run) dry_run=1 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
  shift
done

run() {
  echo "+ $*"
  if [[ $dry_run -eq 0 ]]; then "$@"; fi
}

if [[ "$(uname -s)" == "Darwin" && -z "${DEVELOPER_DIR:-}" ]]; then
  if [[ "$(xcode-select -p 2>/dev/null)" == /Library/Developer/CommandLineTools* ]] \
     && [[ -d /Applications/Xcode.app/Contents/Developer ]]; then
    export DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer
    echo "Using Xcode toolchain: $DEVELOPER_DIR"
  fi
fi

if [[ -z "${JOBS:-}" ]]; then
  JOBS="$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)"
fi

# --- Clean -------------------------------------------------------------------
echo "Cleaning build output in $ROOT_DIR"

targets=(
  "$ROOT_DIR/cmake-build-debug"
  "$ROOT_DIR/Testing"
  "$SRC_DIR/Testing"
)
shopt -s nullglob
for d in "$ROOT_DIR"/build-* "$SRC_DIR"/build-*; do targets+=("$d"); done
shopt -u nullglob

for t in "${targets[@]}"; do
  [[ -e "$t" ]] && run rm -rf "$t"
done

# build/ (and the legacy src/build/) hold packaged DMGs in dmg/; keep those
# unless --all was given.
for b in "$BUILD_DIR" "$SRC_DIR/build"; do
  [[ -d "$b" ]] || continue
  if [[ $delete_dmgs -eq 1 ]]; then
    run rm -rf "$b"
    continue
  fi
  shopt -s nullglob dotglob
  for e in "$b"/*; do
    [[ "$(basename "$e")" == "dmg" ]] && continue
    run rm -rf "$e"
  done
  shopt -u nullglob dotglob
  [[ -d "$b/dmg" ]] && echo "Keeping packaged DMGs in $b/dmg (use --all to delete)"
done

if [[ "$mode" == "clean" ]]; then
  echo "Clean done."
  exit 0
fi

# --- Build -------------------------------------------------------------------
if [[ "$mode" == "dmg" ]]; then
  if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "--dmg is only supported on macOS." >&2
    exit 1
  fi
  run "$ROOT_DIR/scripts/generate-macos-dmg.sh"
  exit 0
fi

cmake_args=(-S "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE")
if [[ -z "${CMAKE_PREFIX_PATH:-}" ]] && command -v brew >/dev/null 2>&1; then
  if qt_prefix="$(brew --prefix qt 2>/dev/null)" && [[ -d "$qt_prefix" ]]; then
    cmake_args+=(-DCMAKE_PREFIX_PATH="$qt_prefix")
  fi
fi
if [[ $with_tests -eq 1 ]]; then
  cmake_args+=(-DBUILD_TESTING=ON)
else
  cmake_args+=(-DBUILD_TESTING=OFF)
fi

run cmake "${cmake_args[@]}"
run cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" --parallel "$JOBS"

if [[ $with_tests -eq 1 ]]; then
  run ctest --test-dir "$BUILD_DIR" -C "$BUILD_TYPE" --output-on-failure
fi

echo "Build done: $BUILD_DIR"
