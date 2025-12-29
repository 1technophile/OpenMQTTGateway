# CI/CD Scripts Documentation

This documentation describes the CI/CD scripts used to build OpenMQTTGateway firmware and documentation. These scripts work in GitHub Actions, locally, and in any CI/CD environment.

- [Overview](#cicd-scripts-documentation)
- [Quick Reference](#quick-reference)
  - [Script Hierarchy](#script-hierarchy)
  - [Script Description](#script-description)
  - [Output Structure](#output-structure)
- [Commands](#commands)
  - [ci.sh - Main Entry Point](#commands)
  - [ci.sh build - Build Firmware](#cish-build---build-firmware)
  - [ci.sh site - Build Documentation](#cish-site---build-documentation)
  - [ci.sh qa - Code Formatting Check](#cish-qa---code-formatting-check)
- [Internal Scripts](#internal-scripts)
  - [ci_set_version.sh](#ci_set_versionsh)
  - [ci_build_firmware.sh](#ci_build_firmwaresh)
  - [ci_prepare_artifacts.sh](#ci_prepare_artifactssh)
  - [ci_00_config.sh](#ci_00_configsh)
- [Python Helper Scripts](#python-helper-scripts)
  - [generate_board_docs.py](#generate_board_docspy)
  - [gen_wu.py](#gen_wupy)
- [Environment Variables](#environment-variables)
- [Exit Codes](#exit-codes)
- [Environment Detection](#environment-detection)



## Quick Reference

### Script Hierarchy

```
ci.sh (dispatcher)
├── build → ci_build.sh → ci_set_version.sh
│                       → ci_build_firmware.sh
│                       → ci_prepare_artifacts.sh
├── site → ci_site.sh → generate_board_docs.py
│                     → gen_wu.py
├── qa → ci_qa.sh → clang-format
└── all → qa + build + site (sequential)
```

### Script Description

| Script | Purpose | Called By |
|--------|---------|-----------|
| `ci.sh` | Main command dispatcher | User/GitHub Actions |
| `ci_build.sh` | Build firmware orchestrator | ci.sh build |
| `ci_site.sh` | Documentation build orchestrator | ci.sh site |
| `ci_qa.sh` | Code formatting checker | ci.sh qa |
| `ci_set_version.sh` | Version injection in firmware | ci_build.sh |
| `ci_build_firmware.sh` | PlatformIO build execution | ci_build.sh |
| `ci_prepare_artifacts.sh` | Artifact packaging | ci_build.sh |
| `ci_00_config.sh` | Shared configuration and functions | All scripts |


### Output Structure

Build outputs are organized in the project root:

```
.pio/build/<environment>/     # PlatformIO build outputs
├── firmware.bin              # Main firmware binary
├── bootloader.bin           # ESP32 bootloader
└── partitions.bin           # ESP32 partition table

generated/
├── artifacts/               # Packaged firmware artifacts
└── site/                   # Built documentation (VuePress output)

scripts/
├── latest_version.json     # Production version metadata
└── latest_version_dev.json # Development version metadata
```

## Commands

`ci.sh` is the main Entry Point. Command dispatcher that routes to specialized scripts.

**Usage:**
```bash
./scripts/ci.sh <command> [OPTIONS]
```

**Commands:**
- `build` - Build firmware for specified environment
- `site` or `docs` - Build documentation website
- `qa` or `lint` - Run code formatting checks
- `all` or `pipeline` - Run complete pipeline (qa + build + site)

**Examples:**
```bash
# Get Help
./scripts/ci.sh build --help
./scripts/ci.sh qa --help
./scripts/ci.sh site --help

# Build firmware
./scripts/ci.sh build esp32dev-ble --mode dev
./scripts/ci.sh build esp32dev-all-test --version v1.8.0 --deploy-ready

# Build documentation
./scripts/ci.sh site --mode prod
./scripts/ci.sh site --mode dev --url-prefix /dev/

# Check code formatting
./scripts/ci.sh qa --check
./scripts/ci.sh qa --fix

# Run complete pipeline
./scripts/ci.sh all esp32dev-ble --version v1.8.0
./scripts/ci.sh all esp32dev-ble --no-site
```

**Options for `all` command:**
- `--no-site` - Skip documentation build step
- All options from `build` command are passed through

---

### ci.sh build - Build Firmware

Orchestrates complete firmware build: version injection, compilation, artifact packaging.

**Usage:**
```bash
./scripts/ci.sh build <environment> [OPTIONS]
```

**Required Arguments:**
- `<environment>` - PlatformIO environment name (e.g., esp32dev-ble, nodemcuv2-rf)

**Options:**
- `--version <tag>` - Version string to inject (default: auto-generated from git)
- `--mode <dev|prod>` - Build mode (default: prod)
  - `dev` - Enables development OTA, sets DEVELOPMENTOTA=true
  - `prod` - Standard production build
- `--deploy-ready` - Package artifacts with deployment naming (env-firmware.bin)
- `--output <dir>` - Output directory for artifacts (default: generated/artifacts)
- `--skip-verification` - Skip build tools verification
- `--clean` - Clean previous build before starting
- `--verbose` - Enable verbose PlatformIO output
- `--list-envs` - List all available PlatformIO environments
- `--help` - Show help message

**Execution Flow:**
```
ci.sh build esp32dev-ble --version v1.8.0 --mode prod --deploy-ready
  │
  ├─> ci_build.sh (orchestrator)
  │     ├─> verify_build_tools() - Check python3, platformio, git
  │     ├─> ci_set_version.sh v1.8.0 - Inject version in User_config.h
  │     ├─> ci_build_firmware.sh esp32dev-ble - Execute PlatformIO build
  │     └─> ci_prepare_artifacts.sh esp32dev-ble --deploy - Package binaries
  │
  └─> Outputs in generated/artifacts/
        ├─ esp32dev-ble-firmware.bin
        ├─ esp32dev-ble-bootloader.bin
        └─ esp32dev-ble-partitions.bin
```

**Examples:**
```bash
# Development build
./scripts/ci.sh build esp32dev-ble --mode dev

# Production build with version
./scripts/ci.sh build esp32dev-ble --version v1.8.0 --mode prod

# Deploy-ready build
./scripts/ci.sh build esp32dev-all-test --version v1.8.0 --deploy-ready

# Clean build with verbose output
./scripts/ci.sh build nodemcuv2-rf --clean --verbose

# List available environments
./scripts/ci.sh build --list-envs
```

**Environment Variables:**
- `CI` - Set to 'true' in CI/CD environments
- `BUILD_NUMBER` - Build number from CI/CD system
- `GIT_COMMIT` - Git commit hash for auto-versioning
- `PLATFORMIO_BUILD_FLAGS` - Additional PlatformIO flags (set by script when --mode dev)

**Output Files:**
- Standard mode: `firmware.bin`, `partitions.bin` in generated/artifacts/
- Deploy mode: `<env>-firmware.bin`, `<env>-bootloader.bin`, `<env>-partitions.bin`

---

### ci.sh site - Build Documentation

Builds VuePress documentation website with version management and WebUploader manifest generation.

**Usage:**
```bash
./scripts/ci.sh site [OPTIONS]
```

**Options:**
- `--mode <dev|prod>` - Documentation mode (default: prod)
  - `dev` - Development documentation with watermark
  - `prod` - Production documentation
- `--version-source <release|custom>` - Version source (default: release)
  - `release` - Use git tag as version
  - `custom` - Use custom version string
- `--custom-version <version>` - Custom version string (requires --version-source custom)
- `--url-prefix <path>` - Base URL path (default: /)
  - Example: `/dev/` for development subdirectory
- `--no-webuploader` - Skip WebUploader manifest generation
- `--webuploader-args <args>` - Additional arguments for gen_wu.py
- `--preview` - Open browser after build (local development)
- `--help` - Show help message

**Execution Flow:**
```
ci.sh site --mode prod --version-source release
  │
  ├─> ci_site.sh (orchestrator)
  │     ├─> check_requirements() - Verify node, npm, python3, pip3
  │     ├─> install_dependencies() - npm install, pip3 install packages
  │     ├─> download_common_config() - Fetch from theengs.io
  │     ├─> get_version() - Extract from git tag or use custom
  │     ├─> set_version() - Update VuePress config and JSON files
  │     ├─> set_url_prefix() - Set base path in config
  │     ├─> generate_board_docs.py - Auto-generate board documentation
  │     ├─> npm run docs:build - Build VuePress site
  │     └─> gen_wu.py - Generate WebUploader manifest
  │
  └─> Outputs in generated/site/
        ├─ index.html
        ├─ assets/
        └─ [board documentation pages]
```

**Examples:**
```bash
# Production documentation
./scripts/ci.sh site --mode prod

# Development documentation with custom version
./scripts/ci.sh site --mode dev --version-source custom --custom-version "DEVELOPMENT SHA:abc123"

# Documentation for /dev/ subdirectory
./scripts/ci.sh site --mode dev --url-prefix /dev/

# Skip WebUploader manifest
./scripts/ci.sh site --no-webuploader

# Local preview
./scripts/ci.sh site --preview
```

**Required Tools:**
- Node.js (for VuePress)
- npm (for package management)
- Python 3 (for board docs generator)
- pip3 (for Python dependencies: requests, pandas, markdown, pytablereader, tabulate)

**Output Files:**
- `generated/site/` - Complete static website
- `scripts/latest_version.json` - Production version metadata (updated)
- `scripts/latest_version_dev.json` - Development version metadata (updated)

---

### ci.sh qa - Code Formatting Check

Checks and fixes code formatting using clang-format.

**Usage:**
```bash
./scripts/ci.sh qa [OPTIONS]
```

**Options:**
- `--check` - Check formatting without modifying files (default)
- `--fix` - Automatically fix formatting issues
- `--source <dir>` - Source directory to check (default: main)
- `--extensions <list>` - File extensions to check, comma-separated (default: h,ino,cpp)
- `--clang-format-version <ver>` - clang-format version to use (default: 9)
- `--verbose` - Show detailed output for each file
- `--help` - Show help message

**Execution Flow:**
```
ci.sh qa --check --source main --extensions h,ino
  │
  ├─> ci_qa.sh (orchestrator)
  │     ├─> check_clang_format() - Find clang-format-9 or clang-format
  │     ├─> find_files() - Locate files matching extensions in source dir
  │     └─> check_formatting() - Run clang-format --dry-run --Werror
  │           └─> Report files with formatting issues
  │
  └─> Exit code: 0 (pass) or 1 (formatting issues found)
```

**Examples:**
```bash
# Check formatting (CI mode)
./scripts/ci.sh qa --check

# Fix formatting automatically
./scripts/ci.sh qa --fix

# Check specific directory
./scripts/ci.sh qa --check --source lib

# Check only .h and .ino files
./scripts/ci.sh qa --check --extensions h,ino

# Check with verbose output
./scripts/ci.sh qa --check --verbose

# Use different clang-format version
./scripts/ci.sh qa --check --clang-format-version 11
```

**Required Tools:**
- clang-format (version specified, default: 9)
  - Install: `sudo apt-get install clang-format-9`

**Output:**
- Check mode: Lists files with formatting issues and shows diffs
- Fix mode: Modifies files in-place and reports changes
- Exit code 0: All files properly formatted
- Exit code 1: Formatting issues found (in check mode)

---

## Internal Scripts

These scripts are called by the main orchestrators. It can be invoked directly but is not raccomanded.

### ci_set_version.sh

Injects version string into firmware configuration files.

**Called By:** `ci_build.sh`

**Usage:**
```bash
./scripts/ci_set_version.sh <version> [--dev]
```

**Arguments:**
- `<version>` - Version string to inject (e.g., v1.8.0 or abc123)
- `--dev` - Development mode (updates latest_version_dev.json)

**Files Modified:**
- `main/User_config.h` - Replaces "version_tag" with actual version
- `scripts/latest_version.json` - Production version metadata
- `scripts/latest_version_dev.json` - Development version metadata (--dev mode)

**Behavior:**
- Creates .bak backup files before modification
- Replaces all occurrences of "version_tag" string
- Validates version string (must not be empty or "version_tag")
- Cleans up backup files on success

---

### ci_build_firmware.sh

Executes PlatformIO build for specified environment.

**Called By:** `ci_build.sh`

**Usage:**
```bash
./scripts/ci_build_firmware.sh <environment> [OPTIONS]
```

**Arguments:**
- `<environment>` - PlatformIO environment name

**Options:**
- `--dev-ota` - Enable development OTA (sets PLATFORMIO_BUILD_FLAGS)
- `--clean` - Clean before build
- `--verbose` - Verbose PlatformIO output

**Environment Variables Set:**
- `PYTHONIOENCODING=utf-8`
- `PYTHONUTF8=1`
- `PLATFORMIO_BUILD_FLAGS="-DDEVELOPMENTOTA=true"` (when --dev-ota)

**PlatformIO Command:**
```bash
platformio run -e <environment> [--verbose]
```

**Output Location:**
- `.pio/build/<environment>/firmware.bin`
- `.pio/build/<environment>/bootloader.bin` (ESP32 only)
- `.pio/build/<environment>/partitions.bin` (ESP32 only)

---

### ci_prepare_artifacts.sh

Packages firmware binaries from PlatformIO build directory.

**Called By:** `ci_build.sh`

**Usage:**
```bash
./scripts/ci_prepare_artifacts.sh <environment> [OPTIONS]
```

**Arguments:**
- `<environment>` - PlatformIO environment name

**Options:**
- `--deploy` - Use deployment naming (prefix with environment name)
- `--output <dir>` - Output directory (default: generated/artifacts)

**Behavior:**

Standard mode (no --deploy):
- Copies: `firmware.bin`, `partitions.bin`
- Does NOT copy: `bootloader.bin`

Deploy mode (with --deploy):
- Copies and renames:
  - `firmware.bin` → `<env>-firmware.bin`
  - `bootloader.bin` → `<env>-bootloader.bin`
  - `partitions.bin` → `<env>-partitions.bin`

**Source Location:**
- `.pio/build/<environment>/`

**Output Location:**
- Specified by `--output` or default `generated/artifacts/`

---

### ci_00_config.sh

Shared configuration and helper functions for all CI scripts.

**Sourced By:** All ci_*.sh scripts

**Provides:**
- Color codes for terminal output (BLUE, GREEN, RED, YELLOW, NC)
- Logging functions: `log_info()`, `log_warn()`, `log_error()`, `log_success()`
- Path constants: `BUILD_DIR`, `ARTIFACTS_DIR`, `SITE_DIR`
- Common utility functions

**Constants Defined:**
- `BUILD_DIR=".pio/build"` - PlatformIO build directory
- `ARTIFACTS_DIR="generated/artifacts"` - Artifact output directory
- `SITE_DIR="generated/site"` - Documentation output directory

**Logging Functions:**
```bash
log_info "message"    # Blue [INFO] prefix
log_warn "message"    # Yellow [WARN] prefix
log_error "message"   # Red [ERROR] prefix
log_success "message" # Green [SUCCESS] prefix
```

---

## Python Helper Scripts

Other scripts are present and used as internal scripts and it's used as retrocompatibility. Below the lists:
 - `generate_board_docs.py`
 - `gen_wu.py`

### generate_board_docs.py

Auto-generates board-specific documentation pages from platformio.ini.

**Called By:** `ci_site.sh`

**Usage:**
```bash
python3 ./scripts/generate_board_docs.py
```

**Input:**
- `platformio.ini` - Board configurations
- `environments.ini` - Additional environments

**Output:**
- Markdown files in `docs/` directory for each board configuration

**Purpose:**
- Creates documentation pages for each hardware board
- Extracts configuration details from PlatformIO environment definitions
- Formats technical specifications and pin mappings

---

### gen_wu.py

Generates WebUploader manifest for OTA firmware updates.

**Called By:** `ci_site.sh`

**Usage:**
```bash
python3 ./scripts/gen_wu.py [--dev] [repository]
```

**Arguments:**
- `--dev` - Generate development manifest
- `repository` - GitHub repository name (e.g., 1technophile/OpenMQTTGateway)

**Input:**
- `.pio/build/<env>/firmware.bin` - Compiled firmware files
- `scripts/latest_version.json` or `scripts/latest_version_dev.json`

**Output:**
- WebUploader manifest JSON file in `docs/.vuepress/public/`

**Purpose:**
- Creates manifest for web-based firmware updater
- Lists available firmware files with metadata
- Used by documentation site for OTA updates

---

## Environment Variables

Scripts respect these environment variables:

- `PYTHONIOENCODING=utf-8`: Python encoding
- `PYTHONUTF8=1`: UTF-8 mode
- `PLATFORMIO_BUILD_FLAGS`: Custom build flags
- `ESP32_PLATFORM_VERSION`: Extracted automatically

---

## Exit Codes

All scripts use standard exit codes:

- `0` - Success
- `1` - General error or failure
- `2` - Missing required tools or dependencies

Scripts use `set -euo pipefail` for strict error handling:
- `-e` - Exit on error
- `-u` - Exit on undefined variable
- `-o pipefail` - Exit on pipe failure

---

## Environment Detection

Scripts automatically detect if running in CI/CD:

```bash
if [[ "${CI:-false}" == "true" ]]; then
    # Running in CI/CD
    # Disable interactive prompts
    # Use different output formatting
fi
```

CI/CD environments typically set:
- `CI=true`
- `GITHUB_ACTIONS=true` (GitHub Actions)
- `BUILD_NUMBER` (build number)
- `GIT_COMMIT` (commit hash)


---

This documentation reflects the current implementation of CI/CD scripts. All scripts are located in `./scripts/` directory.

For GitHub Actions workflow documentation, see `.github/workflows/README.md`.
