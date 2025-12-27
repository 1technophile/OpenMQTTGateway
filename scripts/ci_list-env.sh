#!/bin/bash
# Lists all available PlatformIO environments for OpenMQTTGateway
# Usage: ./scripts/ci_list.sh [--full]

set -euo pipefail

# Constants
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
readonly SCRIPT_DIR
readonly PROJECT_ROOT

# Load shared configuration (colors, logging functions, paths)
if [[ -f "${SCRIPT_DIR}/ci_00_config.sh" ]]; then
    source "${SCRIPT_DIR}/ci_00_config.sh"
else
    echo "ERROR: ci_00_config.sh not found" >&2
    exit 1
fi


## use .github/workflows/environments.json to list environments
list_environments_from_Json() {
    local json_file="${PROJECT_ROOT}/.github/workflows/environments.json"
    if [[ ! -f "$json_file" ]]; then
        log_error "JSON file not found: $json_file"
        return 1
    fi

    if ! command -v jq >/dev/null 2>&1; then
        log_error "jq is required to read $json_file"
        return 1
    fi

    log_info "Available PlatformIO environments from JSON:"
    echo ""
    local envs=()
    while IFS= read -r env; do
        # Skip test environments
        if [[ ! "$env" =~ -test$ && ! "$env" =~ -all- ]]; then
            envs+=("$env")
        fi
    done < <(jq -r '.environments.all[]? // empty' "$json_file")

    # Sort and display unique environments
    if [[ ${#envs[@]} -gt 0 ]]; then
        printf '%s\n' "${envs[@]}" | sort -u | column -c 80
    else
        log_warn "No environments found in JSON file"
    fi
    echo ""
    log_info "Total: $(printf '%s\n' "${envs[@]}" | sort -u | wc -l) environments"

}


list_environments() {
    log_info "Available PlatformIO environments:"
    echo ""
    local env_files=("${PROJECT_ROOT}/platformio.ini" "${PROJECT_ROOT}/environments.ini")
    local envs=()
    for file in "${env_files[@]}"; do
        if [[ -f "$file" ]]; then
            while IFS= read -r line; do
                if [[ "$line" =~ ^\[env:([^\]]+)\] ]]; then
                    local env_name="${BASH_REMATCH[1]}"
                    envs+=("$env_name")
                fi
            done < "$file"
        fi
    done
    # Sort and display unique environments
    if [[ ${#envs[@]} -gt 0 ]]; then
        printf '%s\n' "${envs[@]}" | sort -u | column -c 80
    else
        log_warn "No environments found in configuration files"
    fi
    echo ""
    log_info "Total: $(printf '%s\n' "${envs[@]}" | sort -u | wc -l) environments"
}

# Main execution
usage() {
    cat <<EOF
Usage: ${0##*/} [--full]

List available PlatformIO environments for OpenMQTTGateway.

Options:
  --full    Parse platformio.ini and environments.ini for an exhaustive list
            (slower, includes all defined envs except *-test / *-all-)
  (default) Use .github/workflows/environments.json (fast, curated list)

Examples:
  ${0##*/}          # quick, curated list
  ${0##*/} --full   # exhaustive list from config files
EOF
}

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    USE_FULL=false
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --full)
                USE_FULL=true
                shift
                ;;
            -h|--help)
                usage
                exit 0
                ;;
            *)
                log_warn "Unknown option: $1"
                usage
                exit 1
                ;;
        esac
    done

    if [[ "$USE_FULL" == true ]]; then
        list_environments
    else
        list_environments_from_Json
    fi
fi
