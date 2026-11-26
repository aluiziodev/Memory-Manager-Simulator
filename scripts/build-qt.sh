#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${project_dir}/build-qt"

cmake -S "${project_dir}" -B "${build_dir}" -G Ninja -DMEMSIM_BUILD_VISUAL=ON
cmake --build "${build_dir}" --parallel
ctest --test-dir "${build_dir}" --output-on-failure

printf '\nPronto. Execute: %s/memsim-visual\n' "${build_dir}"
