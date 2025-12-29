#!/bin/bash
# CI/CD Main Entry Point - Command Dispatcher
# Routes commands to specialized scripts for build, site, qa, and all
# Usage: ./scripts/ci.sh <command> [OPTIONS]

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
    echo "║   OpenMQTTGateway CI/CD Pipeline       ║"
    echo "╚════════════════════════════════════════╝"
    echo ""
}

# Show usage
usage() {
    cat << EOF
Usage: $0 <command> [OPTIONS]

OpenMQTTGateway CI/CD Pipeline - Main Entry Point

Commands:
    build       Build firmware for specified environment
    site        Build and deploy documentation/website
    qa          Run quality assurance checks (linting, formatting)
    all         Run complete pipeline (qa + build + site)

Examples:
    # Build firmware
    $0 build esp32dev-all-test --mode dev
    $0 build esp32dev-bt --version v1.8.0 --deploy-ready

    # Build and deploy documentation
    $0 site --mode prod --deploy
    $0 site --mode dev --preview

    # Run quality checks
    $0 qa --check
    $0 qa --fix

    # Run complete pipeline
    $0 all esp32dev-bt --version v1.8.0

Get help for specific commands:
    $0 build --help
    $0 site --help
    $0 qa --help

EOF
    exit 0
}

# Function to run build pipeline
run_build_pipeline() {
    log_info "Executing build pipeline..."
    "${SCRIPT_DIR}/ci_build.sh" "$@"
}

# Function to run site pipeline
run_site_pipeline() {
    log_info "Executing site pipeline..."
    "${SCRIPT_DIR}/ci_site.sh" "$@"
}

# Function to run QA pipeline
run_qa_pipeline() {
    log_info "Executing QA pipeline..."
    "${SCRIPT_DIR}/ci_qa.sh" "$@"
}

# Function to run complete pipeline
run_all_pipeline() {
    local start_time
    start_time=$(date +%s)
    
    log_info "Starting complete CI/CD pipeline..."
    echo ""
    
    # Step 1: Quality Assurance
    log_info "═══ Step 1/3: Quality Assurance ═══"
    run_qa_pipeline --check || {
        log_error "QA checks failed. Pipeline aborted."
        return 1
    }
    echo ""
    
    # Step 2: Build Firmware
    log_info "═══ Step 2/3: Build Firmware ═══"
    run_build_pipeline "$@" || {
        log_error "Build failed. Pipeline aborted."
        return 1
    }
    echo ""
    
    # Step 3: Build Site (only if not in --no-site mode)
    if [[ ! " $* " =~ " --no-site " ]]; then
        log_info "═══ Step 3/3: Build Documentation ═══"
        run_site_pipeline --mode prod || {
            log_warn "Site build failed, but continuing..."
        }
    else
        log_info "Skipping site build (--no-site flag)"
    fi
    
    local end_time
    end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    echo ""
    echo "╔════════════════════════════════════════╗"
    echo "║     Complete Pipeline Summary          ║"
    echo "╚════════════════════════════════════════╝"
    echo "  Total Duration: ${duration}s"
    echo "  Status: SUCCESS ✓"
    echo "╚════════════════════════════════════════╝"
}

# Main execution
main() {
    # Check if no arguments provided
    if [[ $# -eq 0 ]]; then
        print_banner
        usage
    fi
    
    # Get command
    local command="$1"
    shift || true
    
    # Handle help flags
    if [[ "$command" == "--help" || "$command" == "-h" ]]; then
        print_banner
        usage
    fi
    
    print_banner
    
    # Route to appropriate pipeline
    case "$command" in
        build)
            run_build_pipeline "$@"
            ;;
        site|docs)
            run_site_pipeline "$@"
            ;;
        qa|lint)
            run_qa_pipeline "$@"
            ;;
        all|pipeline)
            run_all_pipeline "$@"
            ;;
        *)
            log_error "Unknown command: $command"
            echo ""
            usage
            ;;
    esac
}

# Execute main function
main "$@"
