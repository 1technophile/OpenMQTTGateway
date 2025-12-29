#!/bin/bash
# Builds firmware for specified PlatformIO environment
# Used by: CI/CD pipelines and local development
# Usage: ./build_firmware.sh <environment> [OPTIONS]

set -euo pipefail

# Constants
readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Load shared configuration (colors, logging functions, paths)
if [[ -f "${SCRIPT_DIR}/ci_00_config.sh" ]]; then
    source "${SCRIPT_DIR}/ci_00_config.sh"
else
    echo "ERROR: ci_00_config.sh not found" >&2
    exit 1
fi

# Set absolute path for BUILD_DIR
BUILD_DIR="${PROJECT_ROOT}/${BUILD_DIR}"

# Script-specific logging function
log_build() { echo -e "${BLUE}[BUILD]${NC} $*"; }

# Function to validate environment name
validate_environment() {
    local env="$1"
    
    if [[ -z "$env" ]]; then
        log_error "Environment name is required"
        return 1
    fi
    
    # Check if environment exists in platformio.ini or environments.ini
    if ! grep -q "^\[env:${env}\]" "${PROJECT_ROOT}/platformio.ini" "${PROJECT_ROOT}/environments.ini" 2>/dev/null; then
        log_warn "Environment '${env}' not found in configuration files"
        log_warn "Proceeding anyway (PlatformIO will validate)"
    fi
    
    log_info "Building environment: $env"
}

# Function to setup build environment variables
setup_build_env() {
    local enable_dev_ota="${1:-false}"
    
    export PYTHONIOENCODING=utf-8
    export PYTHONUTF8=1
    
    if [[ "$enable_dev_ota" == "true" ]]; then
        export PLATFORMIO_BUILD_FLAGS='"-DDEVELOPMENTOTA=true"'
        log_info "Development OTA enabled"
    fi
}

# Function to check PlatformIO availability
check_platformio() {
    if ! command -v platformio >/dev/null 2>&1; then
        log_error "PlatformIO not found. Run setup_build_env.sh first"
        return 1
    fi
}

# Function to clean build artifacts
clean_build() {
    local env="$1"
    local env_dir="${BUILD_DIR}/${env}"
    
    if [[ -d "$env_dir" ]]; then
        log_info "Cleaning previous build artifacts for: $env"
        rm -rf "$env_dir"
    fi
}

# Function to run PlatformIO build
run_build() {
    local env="$1"
    local clean="${2:-false}"
    local verbose="${3:-false}"
    
    log_build "Starting build for environment: $env"
    
    local build_cmd="platformio run -e $env"
    
    if [[ "$clean" == "true" ]]; then
        build_cmd="platformio run -e $env --target clean && $build_cmd"
    fi
    
    if [[ "$verbose" == "true" ]]; then
        build_cmd="$build_cmd --verbose"
    fi
    
    # Execute build with timing
    local start_time
    start_time=$(date +%s)
    
    if eval "$build_cmd"; then
        local end_time
        end_time=$(date +%s)
        local duration=$((end_time - start_time))
        
        log_build "Build completed successfully in ${duration}s"
        return 0
    else
        log_error "Build failed for environment: $env"
        return 1
    fi
}

# Function to verify build artifacts
verify_artifacts() {
    local env="$1"
    local env_dir="${BUILD_DIR}/${env}"
    
    log_info "Verifying build artifacts..."
    
    local artifacts_found=0
    local firmware="${env_dir}/firmware.bin"
    local partitions="${env_dir}/partitions.bin"
    local bootloader="${env_dir}/bootloader.bin"
    
    if [[ -f "$firmware" ]]; then
        local size
        size=$(stat -f%z "$firmware" 2>/dev/null || stat -c%s "$firmware" 2>/dev/null)
        log_info "✓ firmware.bin (${size} bytes)"
        ((artifacts_found++))
    else
        log_warn "✗ firmware.bin not found"
    fi
    
    if [[ -f "$partitions" ]]; then
        log_info "✓ partitions.bin"
        ((artifacts_found++))
    fi
    
    if [[ -f "$bootloader" ]]; then
        log_info "✓ bootloader.bin"
        ((artifacts_found++))
    fi
    
    if [[ $artifacts_found -eq 0 ]]; then
        log_error "No build artifacts found"
        return 1
    fi
    
    log_info "Found ${artifacts_found} artifact(s)"
}

# Function to show build summary
show_build_summary() {
    local env="$1"
    local env_dir="${BUILD_DIR}/${env}"
    
    echo ""
    echo "═══════════════════════════════════════"
    echo "  Build Summary: $env"
    echo "═══════════════════════════════════════"
    
    if [[ -d "$env_dir" ]]; then
        find "$env_dir" -name "*.bin" -o -name "*.elf" | while read -r file; do
            local size
            size=$(stat -f%z "$file" 2>/dev/null || stat -c%s "$file" 2>/dev/null)
            local size_kb=$((size / 1024))
            echo "  $(basename "$file"): ${size_kb} KB"
        done
    fi
    
    echo "═══════════════════════════════════════"
}

# Show usage
usage() {
    cat << EOF
Usage: $0 <environment> [OPTIONS]

Build firmware for a specific PlatformIO environment.

Arguments:
    environment     PlatformIO environment name (e.g., esp32dev-all-test)

Options:
    --dev-ota       Enable development OTA build flags
    --clean         Clean build artifacts before building
    --verbose       Enable verbose build output
    --no-verify     Skip artifact verification
    --help          Show this help message

Examples:
    $0 esp32dev-all-test
    $0 esp32dev-bt --dev-ota
    $0 theengs-bridge --clean --verbose

EOF
}

# Main execution
main() {
    local environment=""
    local enable_dev_ota=false
    local clean_build_flag=false
    local verbose=false
    local verify=true
    
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --dev-ota)
                enable_dev_ota=true
                shift
                ;;
            --clean)
                clean_build_flag=true
                shift
                ;;
            --verbose)
                verbose=true
                shift
                ;;
            --no-verify)
                verify=false
                shift
                ;;
            --help|-h)
                usage
                exit 0
                ;;
            -*)
                log_error "Unknown option: $1"
                usage
                exit 1
                ;;
            *)
                environment="$1"
                shift
                ;;
        esac
    done
    
    # Validate inputs
    if [[ -z "$environment" ]]; then
        log_error "Environment name is required"
        usage
        exit 1
    fi
    
    # Change to project root
    cd "$PROJECT_ROOT"
    
    # Check prerequisites
    check_platformio || exit 1
    
    # Validate environment
    validate_environment "$environment" || exit 1
    
    # Setup build environment
    setup_build_env "$enable_dev_ota"
    
    # Clean if requested
    if [[ "$clean_build_flag" == "true" ]]; then
        clean_build "$environment"
    fi
    
    # Run build
    run_build "$environment" "$clean_build_flag" "$verbose" || exit 1
    
    # Verify artifacts
    if [[ "$verify" == "true" ]]; then
        verify_artifacts "$environment" || exit 1
    fi
    
    # Show summary
    show_build_summary "$environment"
    
    log_info "Build process completed successfully"
}

# Run main if executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
