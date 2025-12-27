#!/bin/bash
# CI/CD Main Entry Point - Command Dispatcher
# Routes commands to specialized scripts for build, site, qa, and all
# Usage: ./scripts/ci.sh <command> [OPTIONS]

set -euo pipefail

# Constants
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly SCRIPT_DIR


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
    security    Run security vulnerability scan using Trivy
    all         Run complete pipeline (qa + build + site)
    list-env    List available environments for building firmware

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

    # Run security scan
    $0 security
    $0 security --scan-type config --exit-code 1
    $0 security --severity UNKNOWN,LOW,MEDIUM,HIGH,CRITICAL

    # Run complete pipeline (all envs, mode required)
    $0 all --mode dev
    $0 all --mode prod --preview
    $0 all --mode dev -e esp32dev-bt

    # List available environments
    $0 list-env

Get help for specific commands:
    $0 build --help
    $0 site --help
    $0 qa --help
    $0 security --help
    $0 list-env --help

EOF
    exit 0
}



# Function to get list of environments to build
# Uses ci_list-env.sh (no parameters) and returns one env per line
get_environments() {
    # Run the curated list script and normalize output
    "${SCRIPT_DIR}/ci_list-env.sh" \
        | sed -r 's/\x1B\[[0-9;]*[mK]//g' \
        | tr '\t ' '\n' \
        | sed '/^\s*$/d' \
        | grep -E '^[A-Za-z0-9._-]+$' \
        | sort -u
}

# Function to run complete pipeline using the underlying scripts
# Usage: run_all_pipeline --mode <dev|prod> [--preview]
run_all_pipeline() {
    local start_time
    start_time=$(date +%s)

    local mode=""
    local preview=false
    local env_override=""
    local version=""
    local do_clean=false

    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --mode)
                if [[ -z "${2:-}" ]]; then
                    log_error "--mode requires an argument (dev or prod)"
                    return 1
                fi
                mode="$2"
                if [[ "$mode" != "dev" && "$mode" != "prod" ]]; then
                    log_error "Invalid mode: $mode. Must be 'dev' or 'prod'"
                    return 1
                fi
                shift 2
                ;;
            --clean)
                do_clean=true
                shift
                ;;
            --preview)
                preview=true
                shift
                ;;
            -e|--env)
                if [[ -z "${2:-}" ]]; then
                    log_error "-e|--env requires an environment name"
                    return 1
                fi
                env_override="$2"
                shift 2
                ;;
            -v|--version)
                if [[ -z "${2:-}" ]]; then
                    log_error "-v|--version requires a version string"
                    return 1
                fi
                version="$2"
                shift 2
                ;;
            --help|-h)
                echo "Usage: $0 all --mode <dev|prod> [--preview]"
                echo ""
                echo "Options:"
                echo "  --mode <dev|prod>  Build mode (required)"
                echo "  --preview          Optional Show site in local at http://localhost:8443"
                echo "  -e, --env <name>   Optional Build only the specified environment"
                return 0
                ;;
            *)
                log_error "Unknown option: $1"
                return 1
                ;;
        esac
    done

    # Validate mode is provided
    if [[ -z "$mode" ]]; then
        log_error "--mode is required. Usage: $0 all --mode <dev|prod> [--preview]"
        return 1
    fi

    log_info "Starting complete CI/CD pipeline (mode: $mode, preview: $preview)..."
    echo ""

    # Step 1: Quality Assurance
    log_info "═══ Step 1/3: Quality Assurance ═══"
    log_info "RUN: ---> ${SCRIPT_DIR}/ci_qa.sh --check"
    if ! "${SCRIPT_DIR}/ci_qa.sh" --check; then
        log_error "QA checks failed. Pipeline aborted."
        return 1
    fi
    echo ""

    # Step 2: Build Firmware
    log_info "═══ Step 2/3: Build Firmware ═══"
    
    # Get list of environments
    local -a environments
    if [[ -n "$env_override" ]]; then
        log_info "Using single environment override: $env_override"
        environments=("$env_override")
    else
        mapfile -t environments < <(get_environments)
    fi
    
    if [[ ${#environments[@]} -eq 0 ]]; then
        log_error "No environments found to build"
        return 1
    fi
    
    log_info "Found ${#environments[@]} environments to build"
    echo ""
    
    local build_count=0
    local failed_builds=()
    local build_args=()

    if [[ -n "$version" ]]; then
        build_args+=("--version" "$version")
        log_info "Using version override: $version"
    fi
    build_args+=("--mode" "$mode")
    build_args+=("--deploy-ready")
    if [[ "$do_clean" == true ]]; then
        build_args+=("--clean")
    fi

    for env in "${environments[@]}"; do
        ((++build_count))
        log_info "[$build_count/${#environments[@]}] Building: $env"
        
        set +e
        log_info "RUN: ---> ${SCRIPT_DIR}/ci_build.sh" "$env" "${build_args[@]}"
        "${SCRIPT_DIR}/ci_build.sh" "$env" "${build_args[@]}"
        local rc=$?
        set -e
        if [[ $rc -ne 0 ]]; then
            log_error "Build failed for environment: $env"
            failed_builds+=("$env")
        fi
    done
    
    echo ""
    if [[ ${#failed_builds[@]} -gt 0 ]]; then
        log_error "Build failed for ${#failed_builds[@]} environment(s):"
        printf '  - %s\n' "${failed_builds[@]}"
        return 1
    fi
    
    log_success "All environments built successfully (${#environments[@]} total)"
    echo ""

    # Step 3: Build Site
    log_info "═══ Step 3/3: Build Documentation ═══"
    local site_args=("--mode" "$mode")
    if [[ "$preview" == true ]]; then
        site_args+=("--preview")
    fi
    if [[ -n "$version" ]]; then
        site_args+=("--version" "$version")
    fi
    if [[ "$do_clean" == true ]]; then
        site_args+=("--clean")
    fi
    
    log_info "RUN: --->${SCRIPT_DIR}/ci_site.sh" "${site_args[@]}"
    if ! "${SCRIPT_DIR}/ci_site.sh" "${site_args[@]}"; then
        log_warn "Site build failed, but continuing..."
    fi
    echo ""

    local end_time
    end_time=$(date +%s)
    local duration=$((end_time - start_time))

    echo ""
    echo "╔════════════════════════════════════════╗"
    echo "║     Complete Pipeline Summary          ║"
    echo "╚════════════════════════════════════════╝"
    echo "  Mode: $mode"
    echo "  Preview: $preview"
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
            log_info "Executing build pipeline..."
            "${SCRIPT_DIR}/ci_build.sh" "$@"
            ;;
        site|docs)
            log_info "Executing site pipeline..."
            "${SCRIPT_DIR}/ci_site.sh" "$@"
            ;;
        qa|lint)
            log_info "Executing QA pipeline..."
            "${SCRIPT_DIR}/ci_qa.sh" "$@"
            ;;
        security)
            log_info "Executing security scan..."
            "${SCRIPT_DIR}/ci_security.sh" "$@"
            ;;
        list-env)
            log_info "Executing list-env pipeline..."
            "${SCRIPT_DIR}/ci_list-env.sh" "$@"
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
