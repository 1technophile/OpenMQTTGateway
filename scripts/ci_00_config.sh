# Build Scripts Configuration
# Used by: All build scripts for centralized configuration

# Python Configuration
PYTHON_VERSION="3.13"
PLATFORMIO_VERSION="https://github.com/pioarduino/platformio-core/archive/refs/tags/v6.1.18.zip"

# Centralized Output Directory Structure
# All CI/CD generated files go under generated/
GENERATED_BASE_DIR="generated"
ARTIFACTS_DIR="${GENERATED_BASE_DIR}/artifacts"
SITE_DIR="${GENERATED_BASE_DIR}/site"
REPORTS_DIR="${GENERATED_BASE_DIR}/reports"

# PlatformIO Directory Configuration
BUILD_DIR=".pio/build"
SCRIPTS_DIR="scripts"

# Build Configuration
DEFAULT_ENVIRONMENT="esp32dev-all-test"
ENABLE_VERBOSE_BUILD="false"
ENABLE_BUILD_CACHE="true"

# Artifact Configuration
ARTIFACT_RETENTION_DAYS="7"
CREATE_MANIFEST="true"
COMPRESS_ARTIFACTS="false"

# Version Configuration
VERSION_FILE_PROD="scripts/latest_version.json"
VERSION_FILE_DEV="scripts/latest_version_dev.json"
USER_CONFIG_FILE="main/User_config.h"

# Logging Configuration
ENABLE_COLOR_OUTPUT="true"
LOG_LEVEL="INFO"  # DEBUG, INFO, WARN, ERROR

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
log_step() { echo -e "${BLUE}[STEP]${NC} $*" >&2; }

# Advanced Options
ENABLE_CCACHE="false"
CCACHE_DIR=".ccache"
MAX_BUILD_JOBS="4"
