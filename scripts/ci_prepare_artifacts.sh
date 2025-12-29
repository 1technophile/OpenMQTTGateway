#!/bin/bash
# Prepares firmware artifacts for upload or deployment
# Used by: CI/CD pipelines for artifact packaging
# Usage: ./prepare_artifacts.sh <environment> [OPTIONS]

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

# Set absolute paths
BUILD_DIR="${PROJECT_ROOT}/${BUILD_DIR}"
DEFAULT_OUTPUT_DIR="${PROJECT_ROOT}/${ARTIFACTS_DIR}"

# Function to create output directory
create_output_dir() {
    local output_dir="$1"
    
    if [[ -d "$output_dir" ]]; then
        log_warn "Output directory already exists: $output_dir"
        log_info "Cleaning existing artifacts..."
        rm -rf "$output_dir"
    fi
    
    mkdir -p "$output_dir"
    log_info "Created output directory: $output_dir"
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

# Function to prepare standard artifacts (no renaming)
prepare_standard_artifacts() {
    local env="$1"
    local output_dir="$2"
    local env_dir="${BUILD_DIR}/${env}"
    
    log_info "Preparing STANDARD artifacts for: $env"
    
    local copied=0
    
    # Copy firmware.bin (required)
    if copy_artifact "${env_dir}/firmware.bin" "${output_dir}/firmware.bin" "firmware"; then
        ((copied++))
    fi
    
    # Copy partitions.bin (optional)
    copy_artifact "${env_dir}/partitions.bin" "${output_dir}/partitions.bin" "partitions" && ((copied++)) || true
    
    # Note: bootloader.bin is NOT copied in standard mode (only needed for deployment)
    
    if [[ $copied -eq 0 ]]; then
        log_error "No artifacts were copied"
        return 1
    fi
    
    log_info "Copied ${copied} artifact(s) in standard mode"
}

# Function to prepare deployment artifacts (with renaming)
prepare_deployment_artifacts() {
    local env="$1"
    local output_dir="$2"
    local env_dir="${BUILD_DIR}/${env}"
    
    log_info "Preparing DEPLOYMENT artifacts for: $env"
    
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

# Function to create manifest file
create_manifest() {
    local env="$1"
    local output_dir="$2"
    local manifest="${output_dir}/manifest.txt"
    
    log_info "Creating artifact manifest..."
    
    {
        echo "Environment: $env"
        echo "Build Date: $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
        echo "Build Host: $(hostname)"
        echo ""
        echo "Artifacts:"
        
        find "$output_dir" -type f -name "*.bin" | sort | while read -r file; do
            local size
            size=$(stat -f%z "$file" 2>/dev/null || stat -c%s "$file" 2>/dev/null)
            local size_kb=$((size / 1024))
            local md5sum_val
            md5sum_val=$(md5sum "$file" 2>/dev/null | cut -d' ' -f1 || md5 -q "$file" 2>/dev/null)
            echo "  - $(basename "$file"): ${size_kb} KB (MD5: ${md5sum_val})"
        done
    } > "$manifest"
    
    log_info "Manifest created: $manifest"
}

# Function to compress artifacts (optional)
compress_artifacts() {
    local output_dir="$1"
    local archive_name="$2"
    
    log_info "Compressing artifacts..."
    
    local archive="${output_dir}/${archive_name}.tar.gz"
    
    if tar -czf "$archive" -C "$output_dir" .; then
        local size
        size=$(stat -f%z "$archive" 2>/dev/null || stat -c%s "$archive" 2>/dev/null)
        local size_kb=$((size / 1024))
        log_info "Archive created: ${archive_name}.tar.gz (${size_kb} KB)"
    else
        log_error "Failed to create archive"
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

Prepare firmware artifacts for upload or deployment.

Arguments:
    environment     PlatformIO environment name

Options:
    --deploy        Prepare for deployment (rename with environment prefix)
    --standard      Prepare standard artifacts (no renaming) [default]
    --output DIR    Output directory [default: generated/artifacts/]
    --manifest      Create manifest file with artifact metadata
    --compress      Compress artifacts into tar.gz archive
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
    local mode="standard"
    local output_dir="$DEFAULT_OUTPUT_DIR"
    local create_manifest_flag=false
    local compress_flag=false
    
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --deploy)
                mode="deploy"
                shift
                ;;
            --standard)
                mode="standard"
                shift
                ;;
            --output)
                output_dir="$2"
                shift 2
                ;;
            --manifest)
                create_manifest_flag=true
                shift
                ;;
            --compress)
                compress_flag=true
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
    
    # Check if build directory exists
    if [[ ! -d "${BUILD_DIR}/${environment}" ]]; then
        log_error "Build directory not found for environment: $environment"
        log_error "Run build_firmware.sh first"
        exit 1
    fi
    
    # Create output directory
    create_output_dir "$output_dir"
    
    # Prepare artifacts based on mode
    case "$mode" in
        standard)
            prepare_standard_artifacts "$environment" "$output_dir" || exit 1
            ;;
        deploy)
            prepare_deployment_artifacts "$environment" "$output_dir" || exit 1
            ;;
        *)
            log_error "Unknown mode: $mode"
            exit 1
            ;;
    esac
    
    # Create manifest if requested
    if [[ "$create_manifest_flag" == "true" ]]; then
        create_manifest "$environment" "$output_dir"
    fi
    
    # Compress if requested
    if [[ "$compress_flag" == "true" ]]; then
        compress_artifacts "$output_dir" "$environment"
    fi
    
    # Show summary
    list_artifacts "$output_dir"
    
    log_info "Artifact preparation completed successfully"
}

# Run main if executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
