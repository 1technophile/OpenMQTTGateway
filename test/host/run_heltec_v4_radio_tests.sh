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

# The radio backend must remain an empty translation unit when ZgatewayLORA
# is disabled so non-LoRa environments need neither radio library.
c++ -std=c++17 -Wall -Wextra -Werror \
  -c "${repo_root}/main/lora/LoRaRadio.cpp" \
  -o "${build_dir}/lora_radio_disabled.o"

facade_sources=(
  "${repo_root}/test/host/lora_radio_facade_test.cpp"
  "${repo_root}/main/lora/LoRaRadio.cpp"
  "${repo_root}/main/lora/LoRaRadioConfig.cpp"
)

c++ -std=c++17 -Wall -Wextra -Werror \
  -DZgatewayLORA \
  -I"${repo_root}/test/host/fakes" \
  -I"${repo_root}/main" \
  "${facade_sources[@]}" \
  -o "${build_dir}/lora_radio_legacy_test"

"${build_dir}/lora_radio_legacy_test"

c++ -std=c++17 -Wall -Wextra -Werror \
  -DZgatewayLORA -DLORA_RADIO_SX1262 -DLORA_KCT8103L \
  -DLORA_SCK=9 -DLORA_MISO=11 -DLORA_MOSI=10 -DLORA_SS=8 \
  -DLORA_RST=12 -DLORA_DIO1=14 -DLORA_BUSY=13 \
  -DLORA_PA_POWER=7 -DLORA_PA_CSD=2 -DLORA_PA_CTX=5 -DLORA_RX_LNA=0 \
  -I"${repo_root}/test/host/fakes" \
  -I"${repo_root}/main" \
  "${facade_sources[@]}" \
  -o "${build_dir}/lora_radio_sx1262_test"

"${build_dir}/lora_radio_sx1262_test"

# Compile the actual configuration functions from the monolithic gateway with
# real ArduinoJson and fake hardware/NVS. Keep source-of-truth in the gateway.
arduinojson_include="${ARDUINOJSON_INCLUDE:-${repo_root}/.pio/libdeps/heltec-wifi-lora-32-v4/ArduinoJson/src}"
if [[ ! -f "${arduinojson_include}/ArduinoJson.h" ]]; then
  echo "ArduinoJson required: build the V4 environment first or set ARDUINOJSON_INCLUDE" >&2
  exit 1
fi
{
  sed -n '/^#define subjectLORAtoMQTT/,/^#define repeatLORAwMQTT/p' "${repo_root}/main/config_LORA.h"
  sed -n '/^template <typename T>/,$p' "${repo_root}/main/TheengsCommon.h"
  awk '/^void LORAConfig_defaults/{printing=1} /^void setupLORA/{printing=0} printing' "${repo_root}/main/gatewayLORA.cpp"
  awk '/^String stateLORAMeasures\(\) \{/{printing=1} printing{print} printing && /^}/{exit}' "${repo_root}/main/gatewayLORA.cpp"
} > "${build_dir}/lora_gateway_config_under_test.h"
awk '/^void handleLA\(\) \{/{printing=1} printing{print} printing && /^}/{exit}' "${repo_root}/main/webUI.cpp" > "${build_dir}/lora_web_handler_under_test.h"
c++ -std=c++17 -Wall -Wextra -Werror \
  -DZgatewayLORA -DLORA_RADIO_SX1262 -DLORA_KCT8103L \
  -DLORA_SCK=9 -DLORA_MISO=11 -DLORA_MOSI=10 -DLORA_SS=8 \
  -DLORA_RST=12 -DLORA_DIO1=14 -DLORA_BUSY=13 \
  -DLORA_PA_POWER=7 -DLORA_PA_CSD=2 -DLORA_PA_CTX=5 -DLORA_RX_LNA=0 \
  -I"${repo_root}/test/host/fakes" -I"${repo_root}/main" \
  -I"${build_dir}" -I"${arduinojson_include}" \
  "${repo_root}/test/host/lora_gateway_config_test.cpp" \
  "${repo_root}/main/lora/LoRaRadio.cpp" \
  "${repo_root}/main/lora/LoRaRadioConfig.cpp" \
  -o "${build_dir}/lora_gateway_config_test"
"${build_dir}/lora_gateway_config_test"

c++ -std=c++17 -Wall -Wextra -Werror -DZgatewayLORA \
  -I"${repo_root}/test/host/fakes" -I"${repo_root}/main" \
  -I"${build_dir}" -I"${arduinojson_include}" \
  "${repo_root}/test/host/lora_gateway_config_test.cpp" \
  "${repo_root}/main/lora/LoRaRadio.cpp" \
  "${repo_root}/main/lora/LoRaRadioConfig.cpp" \
  -o "${build_dir}/lora_gateway_config_legacy_test"
"${build_dir}/lora_gateway_config_legacy_test"
