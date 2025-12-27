#!/bin/bash
# shellcheck disable=SC2034
# Build Scripts Configuration
# Used by: All build scripts for centralized configuration

# Centralized Output Directory Structure
# All CI/CD generated files go under generated/
ARTIFACTS_DIR="generated/artifacts"
SITE_DIR="generated/site"
REPORTS_DIR="generated/reports"

# PlatformIO Directory Configuration
BUILD_DIR=".pio/build"

# ============================================================================
# Colors - ANSI color codes for terminal output
# ============================================================================
readonly RED='\033[0;31m'
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly BLUE='\033[0;34m'
readonly NC='\033[0m'  # No Color

# ============================================================================
# Logging Functions - Standardized logging across all build scripts
# ============================================================================
log_info() { echo -e "${GREEN}[INFO]${NC} $*" >&2; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $*" >&2; }
log_error() { echo -e "${RED}[ERROR]${NC} $*" >&2; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $*" >&2; }
log_step() { echo -e "${BLUE}[STEP]${NC} $*" >&2; }
