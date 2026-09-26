#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
build_dir="$(mktemp -d)"
trap 'rm -rf "${build_dir}"' EXIT

c++ -std=c++17 -Wall -Wextra -Werror \
  -I"${repo_root}/main" \
  "${repo_root}/test/host/heltec_v4_radio_config_test.cpp" \
  "${repo_root}/main/lora/LoRaRadioConfig.cpp" \
  -o "${build_dir}/heltec_v4_radio_config_test"

"${build_dir}/heltec_v4_radio_config_test"
