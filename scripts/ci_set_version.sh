#!/bin/bash
# Updates version tags in firmware configuration and JSON files
# Used by: CI/CD pipelines for versioning builds
# Usage: ./set_version.sh <version_tag> [--dev]

set -euo pipefail

# Constants
readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
readonly USER_CONFIG="${PROJECT_ROOT}/main/User_config.h"
readonly LATEST_VERSION_PROD="${SCRIPT_DIR}/latest_version.json"
readonly LATEST_VERSION_DEV="${SCRIPT_DIR}/latest_version_dev.json"

# Load shared configuration (colors, logging functions, paths)
if [[ -f "${SCRIPT_DIR}/ci_00_config.sh" ]]; then
    source "${SCRIPT_DIR}/ci_00_config.sh"
else
    echo "ERROR: ci_00_config.sh not found" >&2
    exit 1
fi

# Function to validate version tag
validate_version() {
    local version="$1"
    
    if [[ -z "$version" ]] || [[ "$version" == "version_tag" ]]; then
        log_error "Invalid version tag: '$version'"
        return 1
    fi
    
    log_info "Version tag validated: $version"
}

# Function to backup files
backup_file() {
    local file="$1"
    
    if [[ -f "$file" ]]; then
        cp "$file" "${file}.bak"
        log_info "Backed up: $file"
    fi
}

# Function to replace version in file
replace_version() {
    local file="$1"
    local version="$2"
    
    if [[ ! -f "$file" ]]; then
        log_warn "File not found: $file (skipping)"
        return 0
    fi
    
    # Backup before modification
    backup_file "$file"
    
    # Replace version_tag placeholder
    if sed -i "s/version_tag/${version}/g" "$file"; then
        log_info "Updated version in: $file"
    else
        log_error "Failed to update version in: $file"
        return 1
    fi
}

# Function to set production version
set_production_version() {
    local version="$1"
    
    log_info "Setting PRODUCTION version: $version"
    
    replace_version "$USER_CONFIG" "$version" || return 1
    replace_version "$VERSION_JSON" "$version" || return 1
}

# Function to set development version
set_development_version() {
    local version="$1"
    
    log_info "Setting DEVELOPMENT version: $version"
    
    replace_version "$USER_CONFIG" "$version" || return 1
    replace_version "$VERSION_DEV_JSON" "$version" || return 1
}

# Function to restore backups
restore_backups() {
    log_warn "Restoring backups..."
    
    for file in "$USER_CONFIG" "$VERSION_JSON" "$VERSION_DEV_JSON"; do
        if [[ -f "${file}.bak" ]]; then
            mv "${file}.bak" "$file"
            log_info "Restored: $file"
        fi
    done
}

# Function to clean backups
clean_backups() {
    for file in "$USER_CONFIG" "$VERSION_JSON" "$VERSION_DEV_JSON"; do
        if [[ -f "${file}.bak" ]]; then
            rm "${file}.bak"
        fi
    done
}

# Show usage
usage() {
    cat << EOF
Usage: $0 <version_tag> [OPTIONS]

Update version tags in firmware configuration files.

Arguments:
    version_tag     Version string to inject (e.g., v1.2.3, abc123, dev-20230101)

Options:
    --dev           Use development version files (latest_version_dev.json)
    --prod          Use production version files (latest_version.json) [default]
    --help          Show this help message

Examples:
    $0 v1.2.3                    # Production release
    $0 abc123 --dev              # Development build
    $0 \${{ github.sha }} --dev    # CI/CD with commit SHA

EOF
}

# Main execution
main() {
    local version=""
    local is_dev=false
    
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --dev)
                is_dev=true
                shift
                ;;
            --prod)
                is_dev=false
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
                version="$1"
                shift
                ;;
        esac
    done
    
    # Validate version
    if [[ -z "$version" ]]; then
        log_error "Version tag is required"
        usage
        exit 1
    fi
    
    validate_version "$version" || exit 1
    
    # Change to project root
    cd "$PROJECT_ROOT"
    
    # Set version with error handling
    if [[ "$is_dev" == true ]]; then
        set_development_version "$version" || {
            restore_backups
            exit 1
        }
    else
        set_production_version "$version" || {
            restore_backups
            exit 1
        }
    fi
    
    # Clean up backups on success
    clean_backups
    
    log_info "Version update completed successfully"
}

# Run main if executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
