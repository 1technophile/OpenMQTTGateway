#!/bin/bash
# shellcheck disable=SC2015
# Prepares firmware artifacts for upload or deployment
# Used by: CI/CD pipelines for artifact packaging
# Usage: ./prepare_artifacts.sh <environment> [OPTIONS]

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

# Set absolute paths
BUILD_DIR="${PROJECT_ROOT}/${BUILD_DIR}"
DEFAULT_OUTPUT_DIR="${PROJECT_ROOT}/${ARTIFACTS_DIR}"

# Function to create output directory
prepare_output_dir() {
    local output_dir="$1"
    local clean_flag="${2:-false}"
    
    if [[ -d "$output_dir" ]]; then
        if [[ "$clean_flag" == "true" ]]; then
            log_warn "Cleaning and recreating output directory: $output_dir"
            rm -rf "$output_dir"
            mkdir -p "$output_dir"
        else
            log_warn "Output directory already exists and will be reused: $output_dir"
        fi
    else
        mkdir -p "$output_dir"
        log_info "Created output directory: $output_dir"
    fi
}

# Function to copy artifact with optional renaming
copy_artifact() {
    local source="$1"
    local dest="$2"
    local artifact_type="$3"
    
    if [[ ! -f "$source" ]]; then
        log_warn "${artifact_type} not found: $source"
        return 1
    fi
    
    if cp "$source" "$dest"; then
        local size
        size=$(stat -f%z "$dest" 2>/dev/null || stat -c%s "$dest" 2>/dev/null)
        local size_kb=$((size / 1024))
        log_info "✓ Copied ${artifact_type}: $(basename "$dest") (${size_kb} KB)"
        return 0
    else
        log_error "Failed to copy ${artifact_type}: $source"
        return 1
    fi
}


# Function to prepare deployment artifacts (with renaming)
prepare_artifacts() {
    local env="$1"
    local output_dir="$2"
    local env_dir="${BUILD_DIR}/${env}"
    
    log_info "Preparing firmware directory for: $env"
    local copied=0
    
    # Copy and rename firmware.bin
    if copy_artifact "${env_dir}/firmware.bin" "${output_dir}/${env}-firmware.bin" "firmware"; then
        ((copied++))
    fi
    
    # Copy and rename partitions.bin (optional)
    copy_artifact "${env_dir}/partitions.bin" "${output_dir}/${env}-partitions.bin" "partitions" && ((copied++)) || true
    
    # Copy and rename bootloader.bin (optional)
    copy_artifact "${env_dir}/bootloader.bin" "${output_dir}/${env}-bootloader.bin" "bootloader" && ((copied++)) || true
    
    # Copy boot_app0.bin if exists (ESP32 specific)
    copy_artifact "${env_dir}/boot_app0.bin" "${output_dir}/${env}-boot_app0.bin" "boot_app0" && ((copied++)) || true
    
    if [[ $copied -eq 0 ]]; then
        log_error "No artifacts were copied"
        return 1
    fi
    
    log_info "Copied ${copied} artifact(s) in deployment mode"
}

prepare_libraries() {
    local env="$1"
    local output_dir="$2"
    local env_dir="${BUILD_DIR}/${env}"
  
    # Process libraries: create temp copy with renamed folders, zip, preserve originals
    log_info "Processing libraries for environment: $env"
    TEMP_LIBDEPS=$(mktemp -p "$output_dir" -d) || { echo "Failed to create temp directory"; return 1; }

    cp -r .pio/libdeps/"$env" "$TEMP_LIBDEPS/" || { log_error "Failed to copy libdeps for $env"; return 1; }

    (
    cd "$TEMP_LIBDEPS"
    log_step "Replace space by _ in folder names (temp copy only)"
    find . -type d -name "* *" | while read -r FNAME; do 
        mv "$FNAME" "${FNAME// /_}"
    done
    
    log_step "Zipping libraries per board"
    for i in */; do
        tar -czf "${i%/}-libraries.tgz" "$i" > /dev/null
    done
    
    mv ./*.tgz "${output_dir}"
    )

    rm -rf "$TEMP_LIBDEPS"
    log_info "✓ Created library archives in: $output_dir"
}

prepare_sources() {
    local output_dir="$1"
    
    log_info "Preparing source code archive"
    
    # Create and move sources tar.gz (newly generated, safe to move)
    if tar -czf "${output_dir}/OpenMQTTGateway_sources.tgz" main LICENSE.txt > /dev/null; then
        log_info "✓ Created source archive: OpenMQTTGateway_sources.tgz"
    else
        log_error "Failed to create source archive"
        return 1
    fi
}








# Function to list artifacts
list_artifacts() {
    local output_dir="$1"
    
    echo ""
    echo "═══════════════════════════════════════"
    echo "  Prepared Artifacts"
    echo "═══════════════════════════════════════"
    
    if [[ -d "$output_dir" ]]; then
        find "$output_dir" -type f | sort | while read -r file; do
            local size
            size=$(stat -f%z "$file" 2>/dev/null || stat -c%s "$file" 2>/dev/null)
            local size_kb=$((size / 1024))
            echo "  $(basename "$file"): ${size_kb} KB"
        done
    else
        echo "  No artifacts found"
    fi
    
    echo "═══════════════════════════════════════"
}

# Show usage
usage() {
    cat << EOF
Usage: $0 <environment> [OPTIONS]

Prepare artifacts for upload or deployment.

Arguments:
    environment     PlatformIO environment name, if omitted will be created source archive only. 

Options:
    --clean         Clean existing output directory before preparing artifacts
    --output DIR    Output directory [default: generated/artifacts/]
    --help          Show this help message

Examples:
    $0 esp32dev-all-test
    $0 esp32dev-bt --deploy --manifest
    $0 theengs-bridge --output build/artifacts --compress

EOF
}

# Main execution
main() {
    local environment=""
    local output_dir="$DEFAULT_OUTPUT_DIR"
    local clean_flag=false
    #local version=""   ## WILL BE USED WHEN THE VERSION ITSELF AFFECTS THE ARTIFACTS NAMING
    
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --output)
                output_dir="$2"
                shift 2
                ;;
            -v|--version)
                if [[ -z "${2:-}" ]]; then
                    log_error "-v|--version requires a version string"
                    return 1
                fi
                #version="$2"
                shift 2
                ;;
            --clean)
                clean_flag=true
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
    
    # Change to project root
    cd "$PROJECT_ROOT"

    # TODO FOR NEXT STEP MULTI RELEASE: TAG, RC, edge
    #if [[ -n "$version" ]]; then
    #    # Sanitize version string for directory name
    #    safe_version=$(echo "$version" | sed 's/[^a-zA-Z0-9._-]/_/g')
    #    output_dir="${output_dir}/${safe_version}" 
    #fi


    # Validate inputs
    if [[ -z "$environment" ]]; then
        log_info "No environment specified, only preparing source archive"
        
        # Create output directory
        prepare_output_dir "$output_dir" "$clean_flag"

        # Prepare source code archive
        prepare_sources "$output_dir" || exit 1
    else
        # Check if build directory exists
        if [[ ! -d "${BUILD_DIR}/${environment}" ]]; then
            log_error "Build directory not found for environment: $environment"
            log_error "Run build_firmware.sh first"
            exit 1
        fi

        #normalize output directory path
        #output_dir="${output_dir}/firmware-${environment}"
        output_dir="${output_dir}/firmware_build"


        # Create output directory
        prepare_output_dir "$output_dir" "$clean_flag"
        
        # Prepare artifacts based on mode
        prepare_artifacts "$environment" "$output_dir" || exit 1

        # Prepare libraries
        prepare_libraries "$environment" "$output_dir" || exit 1        
    fi
    
    # Show summary
    list_artifacts "$output_dir"
    
    log_info "Artifact preparation completed successfully"
}

# Run main if executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
