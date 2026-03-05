#!/bin/bash
# OpenMQTTGateway Security Scan Script
# Runs Trivy vulnerability scanner and generates reports
# Based on task-security-scan.yml workflow
# Usage: ./scripts/ci_security.sh [OPTIONS]

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

# Default values
SCAN_TYPE="fs"
SEVERITY="HIGH,CRITICAL"
EXIT_CODE_ON_VULN="0"
SCAN_PATH="."
EXCLUDE_PATHS=""
GENERATE_SBOM=false  # Off by default

OUTPUT_DIR="${PROJECT_ROOT}/${REPORTS_DIR}"

# Show usage
usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Security vulnerability scanning using Trivy

Options:
    --scan-type <type>      Type of scan: fs (filesystem), config, or image
                            Default: fs
    
    --severity <levels>     Severity levels to report (comma-separated)
                            Options: UNKNOWN,LOW,MEDIUM,HIGH,CRITICAL
                            Default: HIGH,CRITICAL
    
    --scan-path <path>      Path to scan (default: entire repository)
                            Default: .
    
    --exit-code <code>      Exit code when vulnerabilities found
                            0 = continue (don't fail)
                            1 = fail build
                            Default: 0
    
    --exclude <paths>       Paths to exclude from scan (comma-separated)
                            Example: node_modules,test,docs
                            Default: (none)
    
    --generate-sbom         Generate Software Bill of Materials (SBOM)
                            Creates CycloneDX and SPDX JSON formats
                            Default: off
    
    --output-dir <dir>      Directory to save reports
                            Default: ./generated/reports
    
    -h, --help              Show this help message

Examples:
    # Scan filesystem for HIGH and CRITICAL issues
    $0

    # Scan config files and fail if vulnerabilities found
    $0 --scan-type config --exit-code 1

    # Scan specific path for all severity levels
    $0 --scan-path ./main --severity UNKNOWN,LOW,MEDIUM,HIGH,CRITICAL

    # Scan with excluded paths
    $0 --exclude node_modules,test,docs

    # Scan with multiple options
    $0 --scan-path ./main --exclude test --severity HIGH,CRITICAL --exit-code 1

    # Scan with SBOM generation
    $0 --generate-sbom

    # Complete scan with SBOM and custom severity
    $0 --severity UNKNOWN,LOW,MEDIUM,HIGH,CRITICAL --generate-sbom

EOF
    exit 0
}

# Function to check if Trivy is installed
check_trivy() {
    if ! command -v trivy &> /dev/null; then
        log_error "Trivy is not installed. Install it from: https://github.com/aquasecurity/trivy"
        return 1
    fi
    log_success "Trivy found: $(trivy --version)"
}

# Function to check if jq is installed (for JSON parsing)
check_jq() {
    if ! command -v jq &> /dev/null; then
        log_warn "jq is not installed. Some report features will be limited."
        return 1
    fi
    return 0
}

# Function to run Trivy scan with specified format
# Usage: run_trivy_scan <format> <filename> [exit_code]
run_trivy_scan() {
    local format="$1"
    local filename="$2"
    local exit_code="${3:-0}"  # Default to 0 for non-critical scans
    local output_file="${OUTPUT_DIR}/${filename}"
    
    log_info "Running Trivy $SCAN_TYPE scan ($format format)..."
    
    mkdir -p "$OUTPUT_DIR"
    
    # Build Trivy command with all enabled scanners
    local trivy_cmd=(
        trivy "$SCAN_TYPE" "$SCAN_PATH"
        --format "$format"
        --output "$output_file"
        --severity "$SEVERITY"
        --exit-code "$exit_code"
    )
    
    # Add exclude paths if provided
    if [[ -n "$EXCLUDE_PATHS" ]]; then
        IFS=',' read -r -a exclude_dirs <<< "$EXCLUDE_PATHS"
        for skip_dir in "${exclude_dirs[@]}"; do
            if [[ -n "$skip_dir" ]]; then
                trivy_cmd+=(--skip-dirs "$skip_dir")
            fi
        done
    fi
    
    log_info "Executing Trivy ${SCAN_TYPE} scan"
    
    if "${trivy_cmd[@]}" 2>&1 | tee -a "${OUTPUT_DIR}/trivy-scan.log"; then
        log_success "${format^^} report generated: $output_file"
        return 0
    else
        local rc=$?
        if [[ $rc -eq 1 && "$exit_code" == "1" ]]; then
            log_error "Vulnerabilities found (exit code: $rc)"
            return $rc
        fi
        log_success "Scan completed with exit code: $rc"
        return 0
    fi
}

# Function to create summary report
create_summary_report() {
    log_info "Creating security summary report..."
    
    local summary_file="${OUTPUT_DIR}/security-summary.md"
    
    {
        echo "# 🔒 Security Scan Results"
        echo ""
        echo "**Scan Type**: $SCAN_TYPE"
        echo "**Path Scanned**: $SCAN_PATH"
        echo "**Severity Filter**: $SEVERITY"
        echo "**Scan Date**: $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
        echo ""
    } > "$summary_file"
    
    # Parse SARIF if it exists
    local sarif_file="${OUTPUT_DIR}/trivy-results.sarif"
    if [[ -f "$sarif_file" ]]; then
        log_info "Parsing SARIF results..."
        
        if check_jq; then
            # Count vulnerabilities by severity
            local vuln_count
            vuln_count=$(jq '[.runs[].results[]] | length' "$sarif_file" 2>/dev/null || echo "0")
            
            local critical high medium low
            critical=$(jq '[.runs[].results[] | select(.level == "error")] | length' "$sarif_file" 2>/dev/null || echo "0")
            high=$(jq '[.runs[].results[] | select(.level == "warning")] | length' "$sarif_file" 2>/dev/null || echo "0")
            medium=$(jq '[.runs[].results[] | select(.level == "note")] | length' "$sarif_file" 2>/dev/null || echo "0")
            low=$(jq '[.runs[].results[] | select(.level == "none")] | length' "$sarif_file" 2>/dev/null || echo "0")
            
            {
                echo "## Vulnerability Summary"
                echo ""
                echo "**Total Vulnerabilities**: ${vuln_count}"
                echo ""
                echo "- 🔴 **Critical**: ${critical}"
                echo "- 🟠 **High**: ${high}"
                echo "- 🟡 **Medium**: ${medium}"
                echo "- 🟢 **Low**: ${low}"
                echo ""
            } >> "$summary_file"
            
            # List vulnerability details
            if [[ "$vuln_count" -gt 0 ]]; then
                {
                    echo "## Vulnerability Details"
                    echo ""
                } >> "$summary_file"
                
                jq -r '.runs[].results[] | 
                    "### \(.level | ascii_upcase): \(.ruleId)\n" +
                    "**Location**: \(.locations[0].physicalLocation.artifactLocation.uri // "N/A")\n" +
                    "**Message**: \(.message.text)\n" +
                    (if .properties.CVE then "**CVE**: \(.properties.CVE)\n" else "" end) +
                    (if .properties.cvss then "**CVSS Score**: \(.properties.cvss)\n" else "" end) +
                    ""' "$sarif_file" >> "$summary_file" 2>/dev/null || \
                    echo "Could not parse vulnerability details" >> "$summary_file"
            fi
        else
            echo "⚠️  jq not available for detailed SARIF parsing" >> "$summary_file"
        fi
    else
        echo "⚠️  SARIF file not found" >> "$summary_file"
    fi
    
    # Add available report formats
    {
        echo "## Report Formats Available"
        echo ""
        if [[ -f "${OUTPUT_DIR}/trivy-results.sarif" ]]; then
            echo "- ✅ \`trivy-results.sarif\` - SARIF format (for GitHub/IDE integration)"
        fi
        if [[ -f "${OUTPUT_DIR}/trivy-report.json" ]]; then
            echo "- ✅ \`trivy-report.json\` - JSON format (for automation)"
        fi
        if [[ -f "${OUTPUT_DIR}/trivy-report.txt" ]]; then
            echo "- ✅ \`trivy-report.txt\` - Human-readable table"
        fi
        echo ""
    } >> "$summary_file"
    
    # Add footer
    {
        echo "---"
        echo ""
        echo "Generated by OpenMQTTGateway CI/CD Security Scan"
        echo "For more information, see: https://github.com/aquasecurity/trivy"
    } >> "$summary_file"
    
    log_success "Summary report generated: $summary_file"
}

# Function to check for critical vulnerabilities and fail if needed
check_critical_vulnerabilities() {
    if [[ "$EXIT_CODE_ON_VULN" != "1" ]]; then
        return 0
    fi
    
    local sarif_file="${OUTPUT_DIR}/trivy-results.sarif"
    
    if [[ ! -f "$sarif_file" ]]; then
        log_warn "SARIF file not found, skipping critical check"
        return 0
    fi
    
    if check_jq; then
        local critical
        critical=$(jq '[.runs[].results[] | select(.level == "error")] | length' "$sarif_file" 2>/dev/null || echo "0")
        
        if [[ "$critical" -gt 0 ]]; then
            log_error "❌ Found ${critical} critical vulnerabilities!"
            log_error "Review the security reports in: $OUTPUT_DIR"
            return 1
        fi
    fi
    
    return 0
}

# Main execution
main() {
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --scan-type)
                if [[ -z "${2:-}" ]]; then
                    log_error "--scan-type requires an argument"
                    return 1
                fi
                SCAN_TYPE="$2"
                shift 2
                ;;
            --severity)
                if [[ -z "${2:-}" ]]; then
                    log_error "--severity requires an argument"
                    return 1
                fi
                SEVERITY="$2"
                shift 2
                ;;
            --scan-path)
                if [[ -z "${2:-}" ]]; then
                    log_error "--scan-path requires an argument"
                    return 1
                fi
                SCAN_PATH="$2"
                shift 2
                ;;
            --exit-code)
                if [[ -z "${2:-}" ]]; then
                    log_error "--exit-code requires an argument"
                    return 1
                fi
                EXIT_CODE_ON_VULN="$2"
                shift 2
                ;;
            --exclude)
                if [[ -z "${2:-}" ]]; then
                    log_error "--exclude requires an argument"
                    return 1
                fi
                EXCLUDE_PATHS="$2"
                shift 2
                ;;
            --generate-sbom)
                GENERATE_SBOM=true
                shift
                ;;
            --output-dir)
                if [[ -z "${2:-}" ]]; then
                    log_error "--output-dir requires an argument"
                    return 1
                fi
                OUTPUT_DIR="$2"
                shift 2
                ;;
            -h|--help)
                usage
                ;;
            *)
                log_error "Unknown option: $1"
                echo ""
                usage
                ;;
        esac
    done
    
    # Pre-flight checks
    log_info "Running security scan checks..."
    if ! check_trivy; then
        log_error "Required tool check failed"
        return 1
    fi
    check_jq || true
    
    echo ""
    log_info "═══ Security Scan Configuration ═══"
    echo "  Scan Type: $SCAN_TYPE"
    echo "  Path: $SCAN_PATH"
    echo "  Severity: $SEVERITY"
    echo "  Exit on Vulnerabilities: $EXIT_CODE_ON_VULN"
    echo "  Excluded Paths: ${EXCLUDE_PATHS:-none}"
    echo "  Generate SBOM: ${GENERATE_SBOM:-false}"
    echo "  Output Directory: $OUTPUT_DIR"
    echo ""
    
    # Run scans
    if ! run_trivy_scan "sarif" "trivy-results.sarif" "$EXIT_CODE_ON_VULN"; then
        if [[ "$EXIT_CODE_ON_VULN" == "1" ]]; then
            return 1
        fi
    fi
    
    run_trivy_scan "json" "trivy-report.json" "0"
    run_trivy_scan "table" "trivy-report.txt" "0"
    
    # Create summary
    create_summary_report
    
    # Generate SBOM if requested
    if [[ "$GENERATE_SBOM" == "true" ]]; then
        log_info "Generating Software Bill of Materials (SBOM)..."
        
        local sbom_dir="${OUTPUT_DIR}/sbom"
        mkdir -p "$sbom_dir"
        
        # Generate CycloneDX SBOM
        log_info "Generating CycloneDX SBOM..."
        if trivy "$SCAN_TYPE" "$SCAN_PATH" \
            --format cyclonedx \
            --output "$sbom_dir/sbom-cyclonedx.json" \
            --exit-code 0 2>&1 | grep -i "generated\|error" || true; then
            if [[ -f "$sbom_dir/sbom-cyclonedx.json" ]]; then
                log_success "CycloneDX SBOM generated: $sbom_dir/sbom-cyclonedx.json"
            fi
        fi
        
        # Generate SPDX SBOM
        log_info "Generating SPDX SBOM..."
        if trivy "$SCAN_TYPE" "$SCAN_PATH" \
            --format spdx-json \
            --output "$sbom_dir/sbom-spdx.json" \
            --exit-code 0 2>&1 | grep -i "generated\|error" || true; then
            if [[ -f "$sbom_dir/sbom-spdx.json" ]]; then
                log_success "SPDX SBOM generated: $sbom_dir/sbom-spdx.json"
            fi
        fi
        
        if [[ -f "$sbom_dir/sbom-cyclonedx.json" ]] || [[ -f "$sbom_dir/sbom-spdx.json" ]]; then
            log_success "SBOM generation completed"
            echo ""
        fi
    fi
    # Check for critical vulnerabilities
    if ! check_critical_vulnerabilities; then
        return 1
    fi
    
    echo ""
    log_success "Security scan completed successfully!"
    log_info "Reports saved to: $OUTPUT_DIR"
    
    return 0
}

# Execute main function
main "$@"
