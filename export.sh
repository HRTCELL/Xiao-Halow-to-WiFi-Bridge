#!/usr/bin/env bash
# Copyright 2026
# SPDX-License-Identifier: Apache-2.0

# Source this file before running idf.py from any example:
#   source /path/to/xiao-halow-bridge/export.sh

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    echo "This script must be sourced, not executed:"
    echo "  source ${BASH_SOURCE[0]}"
    exit 1
fi

_xiao_halow_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export MMIOT_ROOT="${_xiao_halow_root}"

if [[ -n "${IDF_PATH:-}" && -f "${IDF_PATH}/export.sh" ]]; then
    # shellcheck source=/dev/null
    source "${IDF_PATH}/export.sh"
elif [[ -f "${HOME}/esp/esp-idf/export.sh" ]]; then
    # shellcheck source=/dev/null
    source "${HOME}/esp/esp-idf/export.sh"
else
    echo "ESP-IDF export.sh was not found."
    echo "Set IDF_PATH to your ESP-IDF checkout, then source this file again."
    return 1
fi

echo "MMIOT_ROOT=${MMIOT_ROOT}"
