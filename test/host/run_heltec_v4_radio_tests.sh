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

c++ -std=c++17 -Wall -Wextra -Werror \
  -I"${repo_root}/main" \
  "${repo_root}/test/host/lora_payload_codec_test.cpp" \
  "${repo_root}/main/lora/LoRaPayloadCodec.cpp" \
  -o "${build_dir}/lora_payload_codec_test"

"${build_dir}/lora_payload_codec_test"

facade_sources=(
  "${repo_root}/test/host/lora_radio_facade_test.cpp"
  "${repo_root}/main/lora/LoRaRadio.cpp"
  "${repo_root}/main/lora/LoRaRadioConfig.cpp"
)

c++ -std=c++17 -Wall -Wextra -Werror \
  -I"${repo_root}/test/host/fakes" \
  -I"${repo_root}/main" \
  "${facade_sources[@]}" \
  -o "${build_dir}/lora_radio_legacy_test"

"${build_dir}/lora_radio_legacy_test"

c++ -std=c++17 -Wall -Wextra -Werror \
  -DLORA_RADIO_SX1262 -DLORA_KCT8103L \
  -DLORA_SCK=9 -DLORA_MISO=11 -DLORA_MOSI=10 -DLORA_SS=8 \
  -DLORA_RST=12 -DLORA_DIO1=14 -DLORA_BUSY=13 \
  -DLORA_PA_POWER=7 -DLORA_PA_CSD=2 -DLORA_PA_CTX=5 -DLORA_RX_LNA=0 \
  -I"${repo_root}/test/host/fakes" \
  -I"${repo_root}/main" \
  "${facade_sources[@]}" \
  -o "${build_dir}/lora_radio_sx1262_test"

"${build_dir}/lora_radio_sx1262_test"
