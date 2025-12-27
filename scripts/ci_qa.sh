#!/bin/bash
# CI/CD Quality Assurance (QA) - Code Linting and Formatting
# Checks and fixes code formatting using clang-format
# Usage: ./scripts/ci_qa.sh [OPTIONS]

set -euo pipefail

# Constants
readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Load shared configuration
if [[ -f "${SCRIPT_DIR}/ci_00_config.sh" ]]; then
    source "${SCRIPT_DIR}/ci_00_config.sh"
else
    echo "ERROR: ci_00_config.sh not found" >&2
    exit 1
fi

# Default values
CHECK_MODE=true
FIX_MODE=false
FORMAT_ONLY=false
SOURCE_DIR="main"
EXTENSIONS="h,ino,cpp"
CLANG_FORMAT_VERSION="9"
VERBOSE=false

# Function to check if clang-format is available
check_clang_format() {
    local version="$1"
    local cmd="clang-format-${version}"
    
    if command -v "$cmd" >/dev/null 2>&1; then
        echo "$cmd"
        return 0
    fi
    
    # Try without version suffix
    if command -v clang-format >/dev/null 2>&1; then
        echo "clang-format"
        return 0
    fi
    
    return 1
}

# Function to find files to check
find_files() {
    local source="$1"
    local extensions="$2"
    
    if [[ ! -d "${PROJECT_ROOT}/${source}" ]]; then
        return 1
    fi
    
    local find_patterns=()
    IFS=',' read -ra exts <<< "$extensions"
    for ext in "${exts[@]}"; do
        find_patterns+=(-name "*.${ext}" -o)
    done
    # Remove last -o
    unset 'find_patterns[-1]'
    
    local files
    files=$(find "${PROJECT_ROOT}/${source}" -type f \( "${find_patterns[@]}" \) 2>/dev/null || true)
    
    if [[ -z "$files" ]]; then
        return 1
    fi
    
    echo "$files"
}

# Function to check formatting
check_formatting() {
    local clang_format_cmd="$1"
    local files="$2"
    
    log_info "Checking code formatting..."
    
    local failed_files=()
    local checked_count=0
    local has_issues=false
    
    while IFS= read -r file; do
        if [[ -z "$file" ]]; then
            continue
        fi
        
        checked_count=$((checked_count + 1))
        
        if [[ "$VERBOSE" == true ]]; then
            log_info "Checking: $file"
        fi
        
        # Check if file needs formatting and capture diff
        local diff_output
        diff_output=$("$clang_format_cmd" --dry-run --Werror "$file" 2>&1)
        local format_result=$?
        
        if [[ $format_result -ne 0 ]]; then
            failed_files+=("$file")
            has_issues=true
            
            # Show the actual formatting differences
            echo ""
            log_warn "⚠ Formatting issues in: $file"
            echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
            
            # Generate and show diff with colors
            local actual_diff
            actual_diff=$(diff -u "$file" <("$clang_format_cmd" "$file") 2>/dev/null || true)
            
            if [[ -n "$actual_diff" ]]; then
                echo "$actual_diff" | head -50 | while IFS= read -r line; do
                    if [[ "$line" =~ ^-[^-] ]]; then
                        echo -e "\033[31m$line\033[0m"  # Red for removed lines
                    elif [[ "$line" =~ ^+[^+] ]]; then
                        echo -e "\033[32m$line\033[0m"  # Green for added lines
                    elif [[ "$line" =~ ^@@ ]]; then
                        echo -e "\033[36m$line\033[0m"  # Cyan for line numbers
                    else
                        echo "$line"
                    fi
                done
            else
                echo "$diff_output"
            fi
            echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        else
            if [[ "$VERBOSE" == true ]]; then
                log_info "  ✓ OK"
            fi
        fi
    done <<< "$files"
    
    echo ""
    log_info "Checked ${checked_count} files"
    
    if [[ $has_issues == true ]]; then
        echo ""
        log_error "Found ${#failed_files[@]} files with formatting issues:"
        for file in "${failed_files[@]}"; do
            log_error "  - $file"
        done
        echo ""
        log_error "To fix these issues automatically, run:"
        log_error "  $0 --fix"
        echo ""
        return 1
    fi
    
    log_info "✓ All files are properly formatted"
    return 0
}

# Function to fix formatting
fix_formatting() {
    local clang_format_cmd="$1"
    local files="$2"
    
    log_info "Fixing code formatting..."
    
    local fixed_count=0
    local total_count=0
    
    while IFS= read -r file; do
        if [[ -z "$file" ]]; then
            continue
        fi
        
        total_count=$((total_count + 1))
        
        if [[ "$VERBOSE" == true ]]; then
            log_info "Processing: $file"
        fi
        
        # Apply formatting in-place
        if "$clang_format_cmd" -i "$file" 2>/dev/null; then
            fixed_count=$((fixed_count + 1))
            if [[ "$VERBOSE" == true ]]; then
                log_info "  ✓ Formatted"
            fi
        else
            if [[ "$VERBOSE" == true ]]; then
                log_warn "  ✗ Failed to format"
            fi
        fi
    done <<< "$files"
    
    echo ""
    log_info "Processed ${total_count} files"
    log_info "✓ Formatting applied to ${fixed_count} files"
    
    if [[ $fixed_count -gt 0 ]]; then
        log_warn ""
        log_warn "Files have been modified. Please review and commit the changes:"
        log_warn "  git diff"
        log_warn "  git add -u"
        log_warn "  git commit -m 'style: apply clang-format'"
    fi
}

# Function to run all QA checks
run_all_checks() {
    log_info "Running all quality assurance checks..."
    
    local all_passed=true
    
    # Format check
    log_info "═══ Code Formatting ═══"
    if ! run_format_check; then
        all_passed=false
    fi
    echo ""
    
    # Future: Add more checks here
    # - cppcheck static analysis
    # - code complexity metrics
    # - TODO/FIXME detection
    # - license header validation
    
    if [[ "$all_passed" == false ]]; then
        log_error "Some QA checks failed"
        return 1
    fi
    
    log_info "✓ All QA checks passed"
    return 0
}

# Function to run format check
run_format_check() {
    log_info "Checking for clang-format version ${CLANG_FORMAT_VERSION}..."
    
    local clang_format_cmd
    clang_format_cmd=$(check_clang_format "$CLANG_FORMAT_VERSION")
    
    if [[ $? -ne 0 ]] || [[ -z "$clang_format_cmd" ]]; then
        log_error "clang-format not found"
        log_error "Please install clang-format:"
        log_error "  Ubuntu/Debian: sudo apt-get install clang-format-${CLANG_FORMAT_VERSION}"
        log_error "  macOS: brew install clang-format"
        return 1
    fi
    
    if [[ "$clang_format_cmd" == "clang-format-${CLANG_FORMAT_VERSION}" ]]; then
        log_info "✓ clang-format-${CLANG_FORMAT_VERSION} found"
    else
        local installed_version
        installed_version=$(clang-format --version | grep -oP '\d+\.\d+' | head -1 || echo "unknown")
        log_warn "clang-format-${CLANG_FORMAT_VERSION} not found, using clang-format (version ${installed_version})"
    fi
    
    log_info "Finding files in '${SOURCE_DIR}' with extensions: ${EXTENSIONS}"
    
    local files
    files=$(find_files "$SOURCE_DIR" "$EXTENSIONS")
    
    if [[ $? -ne 0 ]] || [[ -z "$files" ]]; then
        log_error "Source directory not found: ${PROJECT_ROOT}/${SOURCE_DIR}"
        return 1
    fi
    
    local file_count
    file_count=$(echo "$files" | wc -l)
    log_info "Found ${file_count} files to check"
    
    if [[ "$FIX_MODE" == true ]]; then
        fix_formatting "$clang_format_cmd" "$files"
    else
        check_formatting "$clang_format_cmd" "$files"
    fi
}

# Function to show usage
usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Run quality assurance checks on OpenMQTTGateway code.

Options:
    --check                  Check formatting only (CI mode) [default]
    --fix                    Fix formatting issues automatically
    --format                 Run only format checks
    --all                    Run all QA checks [default]
    --source DIR             Source directory to check [default: main]
    --extensions EXTS        File extensions (comma-separated) [default: h,ino,cpp]
    --clang-format-version V clang-format version [default: 9]
    --verbose                Enable verbose output
    --help                   Show this help message

Examples:
    # Check formatting (CI mode)
    $0 --check

    # Fix formatting issues
    $0 --fix

    # Check specific directory
    $0 --check --source lib/LEDManager

    # Check with custom extensions
    $0 --check --extensions h,cpp

    # Verbose output
    $0 --check --verbose

EOF
    exit 0
}

# Parse command line arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --check)
                CHECK_MODE=true
                FIX_MODE=false
                shift
                ;;
            --fix)
                FIX_MODE=true
                CHECK_MODE=false
                shift
                ;;
            --format)
                FORMAT_ONLY=true
                shift
                ;;
            --all)
                FORMAT_ONLY=false
                shift
                ;;
            --source)
                SOURCE_DIR="$2"
                shift 2
                ;;
            --extensions)
                EXTENSIONS="$2"
                shift 2
                ;;
            --clang-format-version)
                CLANG_FORMAT_VERSION="$2"
                shift 2
                ;;
            --verbose)
                VERBOSE=true
                shift
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
    
    log_info "Starting QA pipeline..."
    
    if [[ "$FIX_MODE" == true ]]; then
        log_info "Mode: FIX (will modify files)"
    else
        log_info "Mode: CHECK (read-only)"
    fi
    
    # Run checks
    local result=0
    if [[ "$FORMAT_ONLY" == true ]]; then
        run_format_check || result=$?
    else
        run_all_checks || result=$?
    fi
    
    local end_time
    end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    echo ""
    echo "╔════════════════════════════════════════╗"
    echo "║          QA Pipeline Summary           ║"
    echo "╚════════════════════════════════════════╝"
    echo "  Duration: ${duration}s"
    
    if [[ $result -eq 0 ]]; then
        echo "  Status: SUCCESS ✓"
        echo "╚════════════════════════════════════════╝"
        return 0
    else
        echo "  Status: FAILED ✗"
        echo "╚════════════════════════════════════════╝"
        return 1
    fi
}

# Execute main function
main "$@"
