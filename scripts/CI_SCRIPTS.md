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
  - [ci_list-env.sh](#ci_list-envsh)
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
├── build → ci_build.sh → ci_build_firmware.sh
│                       → ci_prepare_artifacts.sh (when --deploy-ready)
├── site → ci_site.sh
├── qa → ci_qa.sh
├── list-env → ci_list-env.sh
└── all → qa + build (all envs with --mode) + site sequential
```

### Script Description

| Script | Purpose | Called By |
|--------|---------|-----------|
| `ci.sh` | Main command dispatcher | User/GitHub Actions |
| `ci_build.sh` | Build firmware orchestrator | ci.sh build |
| `ci_site.sh` | Documentation build orchestrator | ci.sh site |
| `ci_00_config.sh` | Shared configuration loader | ci.sh |
| `ci_qa.sh` | Quality assurance checks | ci.sh qa |
| `ci_list-env.sh` | List PlatformIO environments (fast JSON or full scan) | ci.sh list-env / direct |
| `ci_build_firmware.sh` | PlatformIO build execution | ci_build.sh |
| `ci_prepare_artifacts.sh` | Artifact packaging (when requested) | ci_build.sh |


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
```

## Commands

`ci.sh` is the main entry point. It dispatches to the specialized scripts.

**Usage:**
```bash
./scripts/ci.sh <command> [OPTIONS]
```

**Commands:**
- `build` - Build firmware for a PlatformIO environment (optionally prepare artifacts)
- `site` or `docs` - Build documentation site
- `qa` or `lint` - Run formatting checks
- `all` or `pipeline` - Run qa → build (all environments) → site with injected mode
- `list-env` - List available PlatformIO environments (JSON fast list or full ini scan)

**Examples:**
```bash
# Help per command
./scripts/ci.sh build --help
./scripts/ci.sh site --help
./scripts/ci.sh qa --help
./scripts/ci.sh list-env --help

# Build firmware
./scripts/ci.sh build esp32dev-all-test --mode dev
./scripts/ci.sh build esp32dev-bt --version v1.8.0 --deploy-ready --output generated/artifacts

# Build docs
./scripts/ci.sh site --mode prod --url-prefix /
./scripts/ci.sh site --mode dev --preview

# QA
./scripts/ci.sh qa --check
./scripts/ci.sh qa --fix --verbose

# Full pipeline (qa + build all envs + site)
./scripts/ci.sh all --mode dev
./scripts/ci.sh all --mode prod --preview
```

---

### ci.sh build - Build Firmware

Runs the build pipeline (tool checks → PlatformIO build → optional artifact packaging).

**Usage:**
```bash
./scripts/ci.sh build <environment> [OPTIONS]
```

**Required Argument:**
- `<environment>` PlatformIO environment name (e.g., esp32dev-ble)

**Options:**
- `--mode <prod|dev>` Build mode (default: prod). `dev` enables OTA flags in the PlatformIO build.
- `--deploy-ready` Copy/rename build outputs and libs via ci_prepare_artifacts.sh.
- `--version [TAG]` Set version used for `OMG_VERSION` and artifact folder naming. If TAG is omitted the script auto-generates (CI: BUILD_NUMBER/GIT_COMMIT, local: timestamp).
- `--output <dir>` Output directory for packaged artifacts (only used when `--deploy-ready` is set, default `generated/artifacts`).
- `--skip-verification` Skip the tool availability checks.
- `--clean` Clean the PlatformIO environment before building.
- `--verbose` Verbose PlatformIO output.
- `--help` Show help.

**Behavior:**
- Tool check verifies python3, platformio, git (can be skipped).
- Builds via ci_build_firmware.sh (adds `--dev-ota` when `--mode dev`).
- Packaging runs only when `--deploy-ready` is provided; artifacts land under `generated/artifacts/firmware_build/` with env-prefixed filenames plus zipped libraries. 
<!-- - Packaging runs only when `--deploy-ready` is provided; artifacts land under `generated/artifacts[/<version>]/firmware_build/` with env-prefixed filenames plus zipped libraries. -->

**Examples:**
```bash
# Dev build with OTA flags
./scripts/ci.sh build esp32dev-ble --mode dev

# Prod build with auto-version and deployable artifacts
./scripts/ci.sh build esp32dev-bt --version --deploy-ready --output generated/artifacts

# Clean + verbose build
./scripts/ci.sh build nodemcuv2-rf --clean --verbose
```

---

### ci.sh site - Build Documentation

Builds the VuePress documentation site.

**Usage:**
```bash
./scripts/ci.sh site [OPTIONS]
```

**Options:**
- `--mode <prod|dev>` Build mode (default: dev).
- `--url-prefix <path>` Base URL path for links (e.g., '/' for root, '/dev/' for dev) (default: /dev/).
- `--version <tag>` Version string written to meta.json (default: edge).
- `--preview` Start the local HTTPS preview server after building (https://localhost:8443).
- `--clean` Remove generated/site folder before building.
- `--insecure-curl` Allow curl to skip TLS verification when downloading common config.
- `--help` Show help.

**Behavior:**
- Checks for node, npm, openssl; installs npm deps; downloads commonConfig.js.
- Writes docs/.vuepress/meta.json with mode/url_prefix/version; builds via `npm run docs:build`.
- Preview mode runs `npm run site:preview`.

**Examples:**
```bash
# Production build
./scripts/ci.sh site --mode prod --url-prefix /

# Development build with preview
./scripts/ci.sh site --mode dev --url-prefix /dev/ --version edge --preview

# Clean then build with custom version
./scripts/ci.sh site --clean --version 1.8.0
```

---

### ci.sh qa - Code Formatting Check

Checks and fixes code formatting using clang-format.

**Usage:**
```bash
./scripts/ci.sh qa [OPTIONS]
```

**Options:**
- `--check` Check formatting only (default)
- `--fix` Apply formatting in place
- `--format` Run only format checks (same as default today)
- `--all` Future hook to run all QA checks (current implementation runs formatting)
- `--source <dir>` Source directory (default: main)
- `--extensions <list>` File extensions (comma-separated, default: h,ino,cpp)
- `--clang-format-version <ver>` clang-format version to use (default: 9)
- `--verbose` Verbose output
- `--help` Show help

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

### ci.sh all - Complete Pipeline

Runs the complete CI/CD pipeline (qa → build all environments → site) with mode injected to all steps.

**Usage:**
```bash
./scripts/ci.sh all --mode <dev|prod> [--preview]
```

**Required Options:**
- `--mode <dev|prod>` Build mode (required). Injected to all pipeline steps:
  - `qa`: Always runs `--check`
  - `build`: Passes mode to all PlatformIO environments (dev enables OTA flags)
  - `site`: Passes mode for documentation generation

**Optional Options:**
- `--preview` Start local HTTPS preview server at https://localhost:8443 after building the site
- `--help` Show help

**Behavior:**
- No environment argument needed; builds **all** available environments
- All three steps (qa, build, site) receive the same `--mode` value
- If any step fails, the pipeline aborts
- Site build warnings do not abort the pipeline (continues with success status)

**Execution Flow:**
```
ci.sh all --mode dev --preview
  │
  ├─> Step 1: ci_qa.sh --check
  │     └─> Exit on failure
  │
  ├─> Step 2: ci_build.sh --mode dev (builds all environments)
  │     └─> Exit on failure
  │
  └─> Step 3: ci_site.sh --mode dev --preview
        ├─> Build documentation
        └─> Start preview server at https://localhost:8443
```

**Examples:**
```bash
# Complete pipeline in dev mode
./scripts/ci.sh all --mode dev

# Complete pipeline in prod mode with site preview
./scripts/ci.sh all --mode prod --preview

# Help for this command
./scripts/ci.sh all --help
```

**Output:**
- Step-by-step progress messages for each pipeline phase
- Final summary showing mode, preview status, duration, and overall status
- Exit code 0: All steps successful
- Exit code 1: Any step failed

---

### ci.sh list-env - List Environments

Lists PlatformIO environments available to build.

**Usage:**
```bash
./scripts/ci.sh list-env [--full]
```

**Options:**
- Default: read .github/workflows/environments.json (requires jq) and show curated list.
- `--full` Parse platformio.ini and environments.ini for an exhaustive list (skips *-test and *-all-).
- `--help` Show help.

**Examples:**
```bash
./scripts/ci.sh list-env
./scripts/ci.sh list-env --full
```

---

## Internal Scripts

These scripts are called by the main orchestrators. They can be run directly for troubleshooting, but the preferred entrypoints are the ci.sh commands.

### ci_list-env.sh

Lists PlatformIO environments for OpenMQTTGateway.

**Called By:** `ci.sh list-env` or direct call

**Usage:**
```bash
./scripts/ci_list-env.sh [--full]
```

**Options:**
- Default: read .github/workflows/environments.json for a curated list (needs jq)
- `--full` - Parse platformio.ini and environments.ini for all envs except *-test and *-all-
- `-h|--help` - Show help

**Output:**
- Shows sorted environments in columns and prints the total count

**Examples:**
```bash
./scripts/ci_list-env.sh
./scripts/ci_list-env.sh --full
```

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
- `--version <tag>` - Set OMG_VERSION for the build (passed through from ci_build.sh)
- `--dev-ota` - Enable development OTA (sets PLATFORMIO_BUILD_FLAGS)
- `--clean` - Clean before build
- `--verbose` - Verbose PlatformIO output
- `--no-verify` - Skip artifact verification after build

**Environment Variables Set:**
- `PYTHONIOENCODING=utf-8`
- `PYTHONUTF8=1`
- `PLATFORMIO_BUILD_FLAGS="-DDEVELOPMENTOTA=true"` (when --dev-ota)
- `OMG_VERSION` (when --version is provided)

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

Packages firmware binaries and libraries from a PlatformIO build directory; can also only create source archive when no environment is provided.

**Called By:** `ci_build.sh`

**Usage:**
```bash
./scripts/ci_prepare_artifacts.sh <environment> [OPTIONS]
```

**Arguments:**
- `<environment>` - PlatformIO environment name (optional; if omitted only source archive is created)

**Options:**
- `--output <dir>` Output directory (default: generated/artifacts)
- `--version <tag>` Append a version subfolder inside the output directory
- `--clean` Clean output directory before writing
- `--help` Show help

**Behavior:**
- If `version` is provided, outputs go to `<output>/<version>/firmware_build/`; otherwise to `<output>/firmware_build/`.
- With an environment: copies and renames firmware.bin/partitions.bin/bootloader.bin/boot_app0.bin to `<env>-*.bin`, then zips libraries for that env into `*-libraries.tgz`.
- Without an environment: only creates `OpenMQTTGateway_sources.tgz` from `main` and `LICENSE.txt`.
- Lists the prepared artifacts and their sizes at the end.

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

Legacy helper scripts are kept for compatibility; they are not called by the current ci_site.sh flow. Below the list:
 - `generate_board_docs.py`
 - `gen_wu.py`

### generate_board_docs.py

Auto-generates board-specific documentation pages from platformio.ini.

**Called By:** Not invoked by current ci_site.sh (legacy helper)

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

**Called By:** Not invoked by current ci_site.sh (legacy helper)

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

- `CI`/`BUILD_NUMBER`/`GIT_COMMIT`: Used by ci_build.sh to auto-generate version when `--version` flag has no tag
- `PYTHONIOENCODING=utf-8`, `PYTHONUTF8=1`: Python encoding settings set by ci_build_firmware.sh
- `PLATFORMIO_BUILD_FLAGS`: Set to include development OTA flag when `--dev-ota` is used
- `OMG_VERSION`: Set when `--version` is passed to ci_build.sh/ci_build_firmware.sh

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
