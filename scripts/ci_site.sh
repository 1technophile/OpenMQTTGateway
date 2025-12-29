#!/bin/bash
# CI/CD Site/Documentation Builder
# Builds and deploys VuePress documentation with version management
# Usage: ./scripts/ci_site.sh [OPTIONS]

set -euo pipefail

# Constants
readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
readonly DOCS_DIR="${PROJECT_ROOT}/docs"
readonly VUEPRESS_CONFIG="${DOCS_DIR}/.vuepress/config.js"
readonly VUEPRESS_BUILD_DIR="${DOCS_DIR}/.vuepress/dist"

# Load shared configuration
if [[ -f "${SCRIPT_DIR}/ci_00_config.sh" ]]; then
    source "${SCRIPT_DIR}/ci_00_config.sh"
else
    echo "ERROR: ci_00_config.sh not found" >&2
    exit 1
fi

# Final output directory for site
readonly SITE_OUTPUT_DIR="${PROJECT_ROOT}/${SITE_DIR}"

# Default values
MODE="prod"
URL_PREFIX="/"
CUSTOM_VERSION=""
VERSION_SOURCE="release"
PREVIEW=false
GENERATE_WEBUPLOADER=true
WEBUPLOADER_ARGS=""
DEPLOY_DIR="."
RUN_PAGESPEED=false
PAGESPEED_URL="https://docs.openmqttgateway.com/"

# Function to check required tools
check_requirements() {
    log_info "Checking required tools..."
    
    local missing_tools=()
    
    if ! command -v node >/dev/null 2>&1; then
        missing_tools+=("node")
    else
        local node_version
        node_version=$(node --version)
        log_info "✓ Node.js ${node_version} found"
    fi
    
    if ! command -v npm >/dev/null 2>&1; then
        missing_tools+=("npm")
    else
        local npm_version
        npm_version=$(npm --version)
        log_info "✓ npm ${npm_version} found"
    fi
    
    if ! command -v python3 >/dev/null 2>&1; then
        missing_tools+=("python3")
    else
        local python_version
        python_version=$(python3 --version | grep -oP '\d+\.\d+' || echo "unknown")
        log_info "✓ Python ${python_version} found"
    fi
    
    if ! command -v pip3 >/dev/null 2>&1; then
        missing_tools+=("pip3")
    else
        local pip_version
        pip_version=$(pip3 --version | grep -oP '\d+\.\d+' || echo "unknown")
        log_info "✓ pip ${pip_version} found"
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
    
    # Upgrade pip first (if pip module is available)
    if python3 -m pip --version >/dev/null 2>&1; then
        log_info "Upgrading pip..."
        python3 -m pip install --upgrade pip --quiet || {
            log_warn "Failed to upgrade pip, continuing with existing version..."
        }
    fi
    
    # Install Python dependencies (without --user if in virtualenv)
    log_info "Installing Python dependencies..."
    pip3 install requests pandas markdown pytablereader tabulate || {
        log_error "Failed to install Python dependencies"
        return 1
    }
    
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
    
    mkdir -p "$(dirname "$config_dest")"
    
    if curl -sSf -o "$config_dest" "$config_url"; then
        log_info "✓ Common config downloaded"
    else
        log_warn "Failed to download common config, continuing anyway..."
    fi
}

# Function to get version
get_version() {
    local version=""
    
    if [[ "$VERSION_SOURCE" == "custom" && -n "$CUSTOM_VERSION" ]]; then
        version="$CUSTOM_VERSION"
        log_info "Using custom version: $version"
    elif [[ "$VERSION_SOURCE" == "release" ]]; then
        # Try to get latest git tag (simulating GitHub release)
        if command -v git >/dev/null 2>&1 && [[ -d "${PROJECT_ROOT}/.git" ]]; then
            version=$(git describe --tags --abbrev=0 2>/dev/null || echo "development")
            log_info "Using release version from git: $version"
        else
            version="development"
            log_warn "Git not available, using version: $version"
        fi
    else
        # Auto-detect from git
        if command -v git >/dev/null 2>&1 && [[ -d "${PROJECT_ROOT}/.git" ]]; then
            version=$(git describe --tags --abbrev=0 2>/dev/null || echo "development")
            log_info "Using git version: $version"
        else
            version="development"
            log_warn "Git not available, using version: $version"
        fi
    fi
    
    echo "$version"
}

# Function to set version in config files
set_version() {
    local version="$1"
    
    log_info "Setting version: $version"
    
    # Update VuePress config
    if [[ -f "$VUEPRESS_CONFIG" ]]; then
        sed -i "s|version_tag|${version}|g" "$VUEPRESS_CONFIG"
    fi
    
    # Update version JSON file based on version source
    if [[ "$VERSION_SOURCE" == "custom" ]]; then
        # Custom version updates dev file
        local version_file="${SCRIPT_DIR}/latest_version_dev.json"
        if [[ -f "$version_file" ]]; then
            sed -i "s|version_tag|${version}|g" "$version_file"
        fi
    else
        # Release version updates production file
        local version_file="${SCRIPT_DIR}/latest_version.json"
        if [[ -f "$version_file" ]]; then
            sed -i "s|version_tag|${version}|g" "$version_file"
        fi
    fi
}

# Function to set URL prefix (base path)
set_url_prefix() {
    local url_prefix="$1"
    
    if [[ "$url_prefix" != "/" ]]; then
        log_info "Setting URL prefix: $url_prefix"
        sed -i "s|base: '/'|base: '${url_prefix}'|g" "$VUEPRESS_CONFIG"
    fi
}

# Function to generate board documentation
generate_board_docs() {
    log_info "Generating board documentation..."
    
    local generator="${SCRIPT_DIR}/generate_board_docs.py"
    
    if [[ -f "$generator" ]]; then
        cd "${PROJECT_ROOT}"
        python3 "$generator" || {
            log_warn "Board documentation generation failed, continuing..."
        }
    else
        log_warn "Board documentation generator not found, skipping..."
    fi
}

# Function to generate WebUploader manifest
generate_webuploader() {
    if [[ "$GENERATE_WEBUPLOADER" != true ]]; then
        log_info "Skipping WebUploader generation"
        return 0
    fi
    
    log_info "Generating WebUploader manifest..."
    
    local generator="${SCRIPT_DIR}/gen_wu.py"
    
    if [[ -f "$generator" ]]; then
        cd "${PROJECT_ROOT}"
        python3 "$generator" $WEBUPLOADER_ARGS || {
            log_warn "WebUploader generation failed, continuing..."
        }
    else
        log_warn "WebUploader generator not found, skipping..."
    fi
}

# Function to build documentation
build_docs() {
    log_info "Building documentation..."
    
    cd "${PROJECT_ROOT}"
    
    # Set Node options for compatibility with newer Node.js versions
    export NODE_OPTIONS="--openssl-legacy-provider"
    
    npm run docs:build || {
        log_error "Documentation build failed"
        return 1
    }
    
    if [[ -d "$VUEPRESS_BUILD_DIR" ]]; then
        log_info "✓ Documentation built successfully"
        
        # Copy to centralized output directory
        log_info "Copying site to: $SITE_OUTPUT_DIR"
        mkdir -p "$SITE_OUTPUT_DIR"
        rm -rf "${SITE_OUTPUT_DIR}"/*
        cp -r "${VUEPRESS_BUILD_DIR}"/* "${SITE_OUTPUT_DIR}/"
        
        log_info "  VuePress output: $VUEPRESS_BUILD_DIR"
        log_info "  Final output: $SITE_OUTPUT_DIR"
    else
        log_error "Build output directory not found: $VUEPRESS_BUILD_DIR"
        return 1
    fi
}

# Function to preview documentation
preview_docs() {
    log_info "Starting documentation preview..."
    
    if [[ ! -d "$SITE_OUTPUT_DIR" ]]; then
        log_error "Build output not found. Run build first."
        return 1
    fi
    
    log_info "Preview server starting at: http://localhost:8080"
    log_info "Press Ctrl+C to stop"
    
    cd "$SITE_OUTPUT_DIR"
    python3 -m http.server 8080
}

# Show usage
usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Build and deploy OpenMQTTGateway documentation site using VuePress.

This script handles the complete documentation build pipeline including:
- Installing Node.js and Python dependencies
- Downloading shared configuration from theengs.io
- Version management (from git tags or custom version)
- Generating board documentation from environments
- Creating WebUploader manifest for firmware updates
- Building VuePress static site
- Optional local preview or deployment to GitHub Pages

OPTIONS:
    --mode MODE
        Build mode: 'prod' or 'dev' [default: prod]
        - prod: Production documentation with release version
        - dev: Development documentation with custom version tag

    --url-prefix PATH
        URL prefix for documentation routing (VuePress base path) [default: /]
        Controls how URLs are generated in the built site.
        Examples:
          --url-prefix /          # Production site at root URL
          --url-prefix /dev/      # Development site at /dev/ URL path
        Note: Should match --deploy-dir for correct link generation

    --custom-version TAG
        Override version tag displayed in documentation
        Example: --custom-version "v1.8.0-beta"
        Note: Automatically sets --version-source to 'custom'

    --version-source SOURCE
        Source for version information: 'release' or 'custom' [default: release]
        - release: Use latest git tag from repository
        - custom: Use version from --custom-version parameter

    --preview
        Start local HTTP server to preview built documentation
        Server will run at http://localhost:8080
        Press Ctrl+C to stop the preview server

    --deploy-dir DIR
        Deployment directory on GitHub Pages [default: .]
        Controls where files are copied in the gh-pages branch.
        Examples:
          --deploy-dir .          # Deploy to root of gh-pages branch
          --deploy-dir dev        # Deploy to dev/ folder in gh-pages
        Note: Should match --url-prefix for correct site structure

    --no-webuploader
        Skip WebUploader manifest generation
        By default, generates manifest for web-based firmware updates

    --webuploader-args ARGS
        Additional arguments passed to gen_wu.py script
        Example: --webuploader-args "--dev"

    --run-pagespeed
        Run Google PageSpeed Insights after deployment
        Requires APIKEY to be configured in workflow

    --pagespeed-url URL
        URL to test with PageSpeed Insights
        Default: https://docs.openmqttgateway.com/

    --help
        Show this help message and exit

EXAMPLES:

    # Build production documentation with latest release version
    $0 --mode prod

    # Build and preview development documentation locally
    $0 --mode dev --url-prefix /dev/ --preview

    # Build with custom version tag
    $0 --custom-version "v1.8.0-beta" --version-source custom

    # Build without WebUploader manifest
    $0 --mode prod --no-webuploader

    # Build for dev environment with custom WebUploader args
    $0 --mode dev --url-prefix /dev/ --deploy-dir dev --webuploader-args "--dev"

WORKFLOW:
    1. Check requirements (Node.js, npm, Python)
    2. Install dependencies (npm packages, Python libraries)
    3. Download common configuration from theengs.io
    4. Determine version (from git tags or custom)
    5. Generate board documentation from PlatformIO environments
    6. Generate WebUploader manifest (optional)
    7. Build VuePress static site
    8. Preview (optional)

OUTPUT:
    Built documentation will be in: generated/site/

EOF
    exit 0
}

# Parse command line arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --mode)
                MODE="$2"
                shift 2
                ;;
            --url-prefix|--base-path)
                URL_PREFIX="$2"
                shift 2
                ;;
            --deploy-dir|--destination-dir)
                DEPLOY_DIR="$2"
                shift 2
                ;;
            --custom-version)
                CUSTOM_VERSION="$2"
                VERSION_SOURCE="custom"
                shift 2
                ;;
            --version-source)
                VERSION_SOURCE="$2"
                shift 2
                ;;
            --preview)
                PREVIEW=true
                shift
                ;;

            --no-webuploader)
                GENERATE_WEBUPLOADER=false
                shift
                ;;
            --webuploader-args)
                WEBUPLOADER_ARGS="$2"
                shift 2
                ;;
            --run-pagespeed)
                RUN_PAGESPEED=true
                shift
                ;;
            --pagespeed-url)
                PAGESPEED_URL="$2"
                shift 2
                ;;
            --help)
                usage
                ;;
            *)
                log_error "Unknown option: $1"
                usage
                ;;
        esac
    done
}

# Main execution
main() {
    local start_time
    start_time=$(date +%s)
    
    parse_args "$@"
    
    log_info "Starting site build pipeline..."
    log_info "Mode: $MODE"
    log_info "URL prefix: $URL_PREFIX"
    log_info "Deploy directory: $DEPLOY_DIR"
    
    # Check requirements
    check_requirements || exit 1
    
    # Install dependencies
    install_dependencies || exit 1
    
    # Download common config
    download_common_config
    
    # Get and set version
    local version
    version=$(get_version)
    set_version "$version"
    
    # Set URL prefix
    set_url_prefix "$URL_PREFIX"
    
    # Generate board documentation
    generate_board_docs
    
    # Generate WebUploader manifest
    generate_webuploader
    
    # Build documentation
    build_docs || exit 1
    
    # Preview if requested
    if [[ "$PREVIEW" == true ]]; then
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

# Execute main function
main "$@"
