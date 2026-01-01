#!/usr/bin/env bash
set -euo pipefail

build_dir="${BUILD_DIR:-build}"
build_type="${BUILD_TYPE:-Debug}"
do_clean=0
build_tests="${BUILD_TESTS:-ON}"
run_tests=0

usage() {
  echo "Usage: $0 [--clean] [--tests on|off] [--run-tests]"
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --clean)
      do_clean=1
      ;;
    --tests)
      if [ "${2:-}" = "on" ]; then
        build_tests="ON"
        shift
      elif [ "${2:-}" = "off" ]; then
        build_tests="OFF"
        shift
      else
        echo "Expected 'on' or 'off' after --tests"
        usage
        exit 1
      fi
      ;;
    --run-tests)
      run_tests=1
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1"
      usage
      exit 1
      ;;
  esac
  shift
done

if [ "$do_clean" -eq 1 ]; then
  rm -rf "${build_dir}"
fi

cmake -S . -B "${build_dir}" \
  -DCMAKE_BUILD_TYPE="${build_type}" \
  -DPEAUDIO_BUILD_TESTS="${build_tests}"
cmake --build "${build_dir}"

if [ "$run_tests" -eq 1 ]; then
  ctest --test-dir "${build_dir}"
fi
