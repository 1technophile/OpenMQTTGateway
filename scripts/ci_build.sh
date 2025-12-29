#!/bin/bash
# CI/CD agnostic wrapper for complete build pipeline
# Orchestrates all build scripts with a single command
# Usage: ./scripts/ci.sh <environment> [OPTIONS]

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

# Function to print banner
print_banner() {
    echo "╔════════════════════════════════════════╗"
    echo "║   OpenMQTTGateway CI/CD Build          ║"
    echo "╚════════════════════════════════════════╝"
    echo ""
}

# Function to print summary
print_summary() {
    local env="$1"
    local version="$2"
    local start_time="$3"
    local end_time
    end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    echo ""
    echo "╔════════════════════════════════════════╗"
    echo "║          Build Summary                 ║"
    echo "╚════════════════════════════════════════╝"
    echo "  Environment: $env"
    echo "  Version: $version"
    echo "  Duration: ${duration}s"
    echo "  Status: SUCCESS ✓"
    echo "╚════════════════════════════════════════╝"
}

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to get command version
get_command_version() {
    local cmd="$1"
    case "$cmd" in
        platformio)
            platformio --version 2>&1 | head -n1 | grep -oP '\d+\.\d+\.\d+' || echo "unknown"
            ;;
        python|python3)
            python3 --version 2>&1 | grep -oP '\d+\.\d+' || echo "unknown"
            ;;
        *)
            echo "unknown"
            ;;
    esac
}

# Function to verify required tools
verify_build_tools() {
    log_info "Verifying required build tools..."
    
    local missing_tools=()
    local version_mismatch=()
    
    # Check Python
    if ! command_exists python3; then
        missing_tools+=("python3")
    else
        local python_version
        python_version=$(get_command_version python3)
        log_info "✓ Python ${python_version} found"
    fi
    
    # Check PlatformIO
    if ! command_exists platformio; then
        missing_tools+=("platformio")
    else
        local pio_version
        pio_version=$(get_command_version platformio)
        log_info "✓ PlatformIO ${pio_version} found"
    fi
    
    # Check git (for version auto-generation)
    if ! command_exists git; then
        log_warn "git not found (optional, but recommended)"
    else
        log_info "✓ git found"
    fi
    
    # Report missing tools
    if [[ ${#missing_tools[@]} -gt 0 ]]; then
        log_error "Missing required tools: ${missing_tools[*]}"
        log_error ""
        log_error "Please install missing tools:"
        for tool in "${missing_tools[@]}"; do
            case "$tool" in
                python3)
                    log_error "  - Python 3: https://www.python.org/downloads/"
                    ;;
                platformio)
                    log_error "  - PlatformIO: pip3 install platformio"
                    log_error "    or: pip3 install ${PLATFORMIO_VERSION:-platformio}"
                    ;;
            esac
        done
        log_error ""
        log_error "Or skip this check with: --skip-verification"
        return 1
    fi
    
    log_info "All required tools are available"
    return 0
}

# Function to cleanup on error
cleanup_on_error() {
    log_error "Build failed, cleaning up..."
    # Restore any backups
    find . -name "*.bak" -type f -exec bash -c 'mv "$1" "${1%.bak}"' _ {} \; 2>/dev/null || true
}

# Function to list available environments
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
                    # Skip test environments
                    if [[ ! "$env_name" =~ -test$ && ! "$env_name" =~ -all- ]]; then
                        envs+=("$env_name")
                    fi
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

# Show usage
usage() {
    cat << EOF
Usage: $0 <environment> [OPTIONS]

Complete CI/CD build pipeline wrapper.

Arguments:
    environment     PlatformIO environment name

Options:
    --version [TAG]        Set version tag (if TAG omitted, auto-generated)
    --mode MODE            Build mode: 'prod' or 'dev' [default: prod]
                           'dev' enables OTA and development version
    --deploy-ready         Prepare for deployment (renamed artifacts)
    --output DIR           Output directory for artifacts [default: generated/artifacts/]
    --skip-verification    Skip build tools verification
    --clean                Clean build before starting
    --verbose              Enable verbose output
    --list-envs            List all available PlatformIO environments
    --help                 Show this help message

Environment Variables:
    CI              Set to 'true' in CI/CD environments
    BUILD_NUMBER    Build number from CI/CD
    GIT_COMMIT      Git commit hash for versioning

Examples:
    # List available environments
    $0 --list-envs

    # Local development build
    $0 esp32dev-all-test --mode dev

    # Production release build
    $0 esp32dev-bt --version v1.7.0 --mode prod --deploy-ready

    # CI/CD build (auto-detects version)
    $0 theengs-bridge --version --mode dev

EOF
}

# Main pipeline
main() {
    local environment=""
    local version=""
    local set_version=false
    local mode=""
    local deploy=false
    local output_dir=""
    local skip_verification=false
    local clean=false
    local verbose=false
    
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --version)
                set_version=true
                # Check if next argument is a version tag or another option
                if [[ $# -gt 1 && ! "$2" =~ ^-- ]]; then
                    version="$2"
                    shift 2
                else
                    shift
                fi
                ;;
            --mode)
                if [[ $# -lt 2 ]]; then
                    log_error "--mode requires an argument: 'prod' or 'dev'"
                    usage
                    exit 1
                fi
                if [[ "$2" != "prod" && "$2" != "dev" ]]; then
                    log_error "Invalid mode: $2. Must be 'prod' or 'dev'"
                    usage
                    exit 1
                fi
                mode="$2"
                shift 2
                ;;
            --deploy-ready)
                deploy=true
                shift
                ;;
            --output)
                if [[ $# -lt 2 ]]; then
                    log_error "--output requires a directory argument"
                    usage
                    exit 1
                fi
                output_dir="$2"
                shift 2
                ;;
            --skip-verification)
                skip_verification=true
                shift
                ;;
            --clean)
                clean=true
                shift
                ;;
            --verbose)
                verbose=true
                shift
                ;;
            --list-envs)
                list_environments
                exit 0
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
    
    # Set default mode if not specified
    if [[ -z "$mode" ]]; then
        mode="prod"
        log_info "Mode not specified, defaulting to production"
    fi
    
    # Validate environment
    if [[ -z "$environment" ]]; then
        log_error "Environment name is required"
        usage
        exit 1
    fi
    
    # Auto-generate version if --version flag is set but no tag provided
    if [[ "$set_version" == "true" && -z "$version" ]]; then
        if [[ "${CI:-false}" == "true" ]]; then
            # CI/CD environment
            version="${BUILD_NUMBER:-${GIT_COMMIT:-unknown}}"
        else
            # Local development
            version="local-$(date +%Y%m%d-%H%M%S)"
        fi
        log_info "Auto-generated version: $version"
    fi
    
    # Setup error handling
    trap cleanup_on_error ERR
    
    # Change to project root
    cd "$PROJECT_ROOT"
    
    # Start timer
    local start_time
    start_time=$(date +%s)
    
    # Print banner
    print_banner
    
    # Step 1: Verify build tools
    if [[ "$skip_verification" == "false" ]]; then
        log_step "1/4 Verifying build tools..."
        verify_build_tools || exit 1
        echo ""
    else
        log_warn "Skipping build tools verification (--skip-verification)"
        echo ""
    fi
    
    # Step 2: Set version (only if --version flag was provided)
    if [[ "$set_version" == "true" ]]; then
        log_step "2/4 Setting version: $version"
        if [[ "$mode" == "dev" ]]; then
            "${SCRIPT_DIR}/ci_set_version.sh" "$version" --dev || exit 1
        else
            "${SCRIPT_DIR}/ci_set_version.sh" "$version" --prod || exit 1
        fi
        echo ""
    else
        log_info "Skipping version setting (use --version to set version)"
        echo ""
    fi
    
    # Step 3: Build firmware
    log_step "3/4 Building firmware for: $environment"
    local build_opts=()
    [[ "$mode" == "dev" ]] && build_opts+=(--dev-ota)
    [[ "$clean" == "true" ]] && build_opts+=(--clean)
    [[ "$verbose" == "true" ]] && build_opts+=(--verbose)
    
    "${SCRIPT_DIR}/ci_build_firmware.sh" "$environment" "${build_opts[@]}" || exit 1
    echo ""
    
    # Step 4: Prepare artifacts
    log_step "4/4 Preparing artifacts..."
    local artifact_opts=()
    [[ "$deploy" == "true" ]] && artifact_opts+=(--deploy) || artifact_opts+=(--standard)
    artifact_opts+=(--manifest)
    [[ -n "$output_dir" ]] && artifact_opts+=(--output "$output_dir")
    
    "${SCRIPT_DIR}/ci_prepare_artifacts.sh" "$environment" "${artifact_opts[@]}" || exit 1
    echo ""
    
    # Print summary
    print_summary "$environment" "$version" "$start_time"
    
    log_info "✓ Complete build pipeline finished successfully"
}

# Run main if executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
