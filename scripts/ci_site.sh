#!/bin/bash
# CI/CD Site/Documentation Builder
# This script builds and deploys VuePress documentation with version management.
# Usage: ./scripts/ci_site.sh [OPTIONS] - Specify options for the script execution.

set -euo pipefail

# Constants
# Resolve the folder containing this script so relative paths work
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
DOCS_DIR="${PROJECT_ROOT}/docs"
readonly SCRIPT_DIR
readonly PROJECT_ROOT
readonly DOCS_DIR


# Load shared configuration
if [[ -f "${SCRIPT_DIR}/ci_00_config.sh" ]]; then
    source "${SCRIPT_DIR}/ci_00_config.sh"
else
    echo "ERROR: ci_00_config.sh not found" >&2
    exit 1
fi

# Final output directory for the site
readonly SITE_OUTPUT_DIR="${PROJECT_ROOT}/${SITE_DIR}"

# Default values
MODE="prod"  # Set the default mode to production
CURL_INSECURE=false  # Allow curl to skip TLS verification (use only when needed)
CLEAN=false  # Clean generated/site before build
LEGACY_OPENSSL=false  # Use --openssl-legacy-provider for older Node.js versions

# Function to check required tools
check_requirements() {
    log_info "Checking required tools..."
    
    local missing_tools=()

    if ! command -v node >/dev/null 2>&1; then
        missing_tools+=("node")  # Node.js is required
    else
        local node_version
        node_version=$(node --version)
        log_info "✓ Node.js ${node_version} found"
    fi
    
    if ! command -v npm >/dev/null 2>&1; then
        missing_tools+=("npm")  # npm is required
    else
        local npm_version
        npm_version=$(npm --version)
        log_info "✓ npm ${npm_version} found"
    fi
    
    if ! command -v openssl >/dev/null 2>&1; then
        missing_tools+=("openssl")  # OpenSSL is required
    else
        local openssl_version
        openssl_version=$(openssl version | grep -oP '\d+\.\d+\.\d+' || echo "unknown")
        log_info "✓ OpenSSL ${openssl_version} found"
    fi
    
    if [[ ${#missing_tools[@]} -gt 0 ]]; then
        log_error "Missing required tools: ${missing_tools[*]}"
        return 1
    fi
    
    log_info "All required tools are available"
    return 0
}

# Function to install dependencies
install_dependencies() {
    log_info "Installing dependencies..."
    
    # Install Node dependencies
    log_info "Installing Node.js dependencies..."
    cd "${PROJECT_ROOT}"
    npm install --quiet || {
        log_error "Failed to install Node.js dependencies"
        return 1
    }
    
    log_info "Dependencies installed successfully"
}

# Function to download common config
download_common_config() {
    log_info "Downloading common configuration..."
    
    local config_url="https://www.theengs.io/commonConfig.js"
    local config_dest="${DOCS_DIR}/.vuepress/public/commonConfig.js"
    local curl_opts="-sSf"

    # Optionally disable TLS verification if explicitly requested
    if [[ "${CURL_INSECURE}" == "true" ]]; then
        curl_opts+="k"
        log_warn "curl is running with --insecure; TLS verification is disabled"
    fi
    
    mkdir -p "$(dirname "$config_dest")"
    
    if curl ${curl_opts} -o "$config_dest" "$config_url"; then
        log_info "✓ Common config downloaded"
    else
        log_warn "Failed to download common config, continuing anyway..."
    fi
}

create_configuration_files() {
    local url_prefix="$1"
    local version="$2"
    local mode="$3"
    # Note: dest is always "generated/site" (from defaults.json).
    # The dev subdirectory is handled by destination_dir in the GitHub Actions deploy step.

    # download common config
    download_common_config


    ## Create a meta.json file on config folder
    local meta_file="${DOCS_DIR}/.vuepress/meta.json"
    cat > "$meta_file" <<EOF
{
    "title": "Theengs OpenMQTTGateway",
    "version": "${version}",
    "url_prefix": "${url_prefix}",
    "mode": "${mode}"
}
EOF
    log_info "✓ Generated meta.json at ${meta_file}"
}

# Function to build documentation
build_docs() {
    log_info "Building documentation..."
    
    cd "${PROJECT_ROOT}"
    
    # Set Node options for compatibility with older Node.js versions (if requested)
    if [[ "$LEGACY_OPENSSL" == true ]]; then
        log_info "Using --openssl-legacy-provider for older Node.js compatibility"
        export NODE_OPTIONS="--openssl-legacy-provider"
    fi
    
    npm run docs:build || {
        log_error "Documentation build failed"
        return 1
    }
    log_info "✓ Documentation built successfully"
}

# Function to preview documentation
preview_docs() {
    log_info "Starting local preview server..."
    
    cd "${PROJECT_ROOT}"
    
    npm run site:preview || {
        log_error "Failed to start preview server"
        return 1
    }
    
    log_info "Preview server running at https://localhost:8443"
    log_info "Press Ctrl+C to stop the preview server"
}

# Function to clean generated site folder
clean_build() {
    local site_folder="${PROJECT_ROOT}/generated/site"
    if [[ -d "$site_folder" ]]; then
        log_info "Cleaning ${site_folder}..."
        rm -rf "$site_folder"
        log_info "✓ Cleaned ${site_folder}"
    else
        log_info "Nothing to clean: ${site_folder} does not exist"
    fi
}

# Show usage
usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Build and deploy OpenMQTTGateway documentation with VuePress.
Generates site structure and meta.json file.

OPTIONS:
    --mode MODE
        Build mode: 'prod' (production) or 'dev' (development) [default: dev]

    --url-prefix PATH
        Base URL path for links (e.g., '/' for root, '/dev/' for dev) [default: /dev/]

    --version VERSION
        Version string written to meta.json (e.g., '1.8.0', 'edge') [default: edge]

    --preview
        Start local HTTPS preview server after build (https://localhost:8443)

    --clean
        Remove generated/site folder before build

    --insecure-curl
        Allow curl to skip TLS verification (use only if cert errors block download)

    --legacy-openssl
        Use --openssl-legacy-provider for older Node.js versions (pre-17)

    --help
        Show this message and exit

EXAMPLES:
    # Production build
    $0 --mode prod --url-prefix / --version 1.8.0

    # Development build with preview
    $0 --mode dev --url-prefix /dev/ --version edge --preview

    # Clean and rebuild
    $0 --clean --mode prod --version 1.8.0

EOF
}



# Main execution
main() {
    local version="edge"
    local mode="dev"
    local url_prefix=""
    local url_prefix_set=false
    local do_preview=false

# Parse command line arguments

    while [[ $# -gt 0 ]]; do
        case "$1" in
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
            --url-prefix)
                url_prefix="$2"
                url_prefix_set=true
                shift 2
                ;;
            --version)
                version="$2"
                shift 2
                ;;
            --preview)
                do_preview=true
                shift
                ;;
            --clean)
                CLEAN=true
                shift
                ;;
            --insecure-curl)
                CURL_INSECURE=true
                shift
                ;;
            --legacy-openssl)
                LEGACY_OPENSSL=true
                shift
                ;;
            --help)
                usage
                exit 0
                ;;
            *)
                log_error "Unknown option: $1"
                usage
                exit 0
                ;;
        esac
    done

    # Set url_prefix based on mode if not explicitly provided
    if [[ "$url_prefix_set" == false ]]; then
        if [[ "$mode" == "prod" ]]; then
            url_prefix="/"
        else
            url_prefix="/dev/"
        fi
    fi

    local start_time
    start_time=$(date +%s)
    
    
    log_info "Starting site build pipeline..."
    log_info "Mode: $mode"
    log_info "URL prefix: $url_prefix"
    
    # Clean if requested
    if [[ "$CLEAN" == "true" ]]; then
        clean_build
    fi
    
    # Check requirements
    check_requirements || exit 1
    
    # Install dependencies
    install_dependencies || exit 1

    # Create configuration files (meta.json)
    create_configuration_files "$url_prefix" "$version" "$mode" || exit 1
    
    # Build documentation
    build_docs || exit 1
    
    # Preview if requested
    if [[ "$do_preview" == true ]]; then
        preview_docs
    fi
    
    local end_time
    end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    echo ""
    echo "╔════════════════════════════════════════╗"
    echo "║        Site Build Summary              ║"
    echo "╚════════════════════════════════════════╝"
    echo "  Mode: $MODE"
    echo "  Version: $version"
    echo "  Duration: ${duration}s"
    echo "  Output: $SITE_OUTPUT_DIR"
    echo "  Status: SUCCESS ✓"
    echo "╚════════════════════════════════════════╝"
}


# Run main if executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
