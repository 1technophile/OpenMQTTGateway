# OpenMQTTGateway Fork - Branching Strategy

## Overview
This document outlines the branching strategy for the custom OpenMQTTGateway fork, maintaining synchronization with the upstream repository while developing custom features.

## Repository Structure

### Remotes
- **origin**: `git@github.com:Odyno/OpenMQTTGateway.git` (your fork)
- **upstream**: `https://github.com/1technophile/OpenMQTTGateway.git` (upstream)

### Branch Hierarchy

```
origin/main (production-ready custom build)
├── origin/development (integration branch)
│   ├── feature/filter_rf_signal (current feature)
│   ├── feature/enhanced_ble
│   └── feature/custom_sensors
├── origin/release/x.x.x-o (release branches)
└── sync/upstream-YYYY-MM-DD (upstream sync branches)

upstream/development (upstream main branch)
```

## Branch Types

### 1. Main Branches

#### `main`
- **Purpose**: Production-ready custom build
- **Protection**: Protected, requires PR and tests
- **Deployment**: Automatically builds and creates releases
- **Merges from**: `development` via tested PRs only

#### `development`
- **Purpose**: Integration of custom features with upstream
- **Protection**: Requires PR review
- **Testing**: All tests must pass
- **Merges from**: Feature branches and sync branches

### 2. Feature Branches

#### Pattern: `feature/[description]`
- **Examples**: 
  - `feature/filter_rf_signal`
  - `feature/enhanced_ble_scanning`
  - `feature/custom_sensor_support`
- **Lifetime**: Created from `development`, merged back via PR
- **Testing**: Must include tests for new functionality

### 3. Sync Branches

#### Pattern: `sync/upstream-YYYY-MM-DD`
- **Purpose**: Merge upstream changes safely
- **Created by**: `scripts/sync-upstream.ps1`
- **Process**: 
  1. Create from `development`
  2. Merge `upstream/development`
  3. Resolve conflicts if any
  4. Test custom features still work
  5. Create PR to `development`

### 4. Release Branches

#### Pattern: `release/x.x.x-o[n]`
- **Purpose**: Prepare releases with custom suffix
- **Examples**: `release/1.8.0-o1`, `release/1.8.1-o2`
- **Process**:
  1. Branch from `development`
  2. Version bump with `scripts/version-manager.ps1`
  3. Final testing
  4. Merge to `main`
  5. Tag and release

## Versioning Strategy

### Version Format: `X.Y.Z-o[N]`
- **X.Y.Z**: Upstream version base
- **-o**: Odyno custom build identifier
- **[N]**: Optional sequential number for same upstream version

### Examples:
- `1.8.0-o1` - First custom build based on upstream 1.8.0
- `1.8.0-o2` - Second custom build with additional features
- `1.8.1-o` - Custom build based on upstream 1.8.1

## Workflow Procedures

### 1. Daily Development

```powershell
# Start new feature
git checkout development
git pull origin development
git checkout -b feature/my-new-feature

# Develop and test
# ... make changes ...
pio test

# Push and create PR
git push origin feature/my-new-feature
# Create PR via GitHub UI: feature/my-new-feature -> development
```

### 2. Upstream Synchronization (Weekly/Monthly)

```powershell
# Check for upstream changes
.\scripts\sync-upstream.ps1 -Interactive

# If conflicts, resolve manually then:
git add .
git commit -m "resolve: merge conflicts with upstream"
git push origin sync/upstream-YYYY-MM-DD

# Create PR: sync/upstream-YYYY-MM-DD -> development
```

### 3. Release Process

```powershell
# Create release branch
git checkout development
git pull origin development
git checkout -b release/1.8.0-o1

# Bump version
.\scripts\version-manager.ps1 minor

# Test thoroughly
pio test
pio run -e esp32dev-all-test

# Merge to main
git checkout main
git merge release/1.8.0-o1 --no-ff

# Create tag and release
.\scripts\version-manager.ps1 tag
git push origin main --tags
```

## Automation

### GitHub Actions
- **Custom Release**: Triggered on version tags (`v*-o*`)
- **PR Testing**: Runs on all PRs to `main` and `development`
- **Upstream Sync**: Weekly check for upstream changes

### Scripts
- `scripts/sync-upstream.ps1` - Upstream synchronization
- `scripts/version-manager.ps1` - Version management
- `scripts/test-custom-features.ps1` - Custom feature validation

## Best Practices

### 1. Keep Custom Changes Modular
- Use preprocessor directives (`#ifdef CUSTOM_FEATURE`)
- Separate custom code in dedicated files when possible
- Document custom modifications clearly

### 2. Test Strategy
- All custom features must have tests
- Test suite runs on multiple ESP32/ESP8266 environments
- Integration tests for MQTT and custom protocols

### 3. Documentation
- Update `README.md` for custom features
- Maintain `CHANGELOG.md` for releases
- Document configuration changes

### 4. Conflict Resolution
- Prefer upstream changes unless they break custom features
- Document why upstream changes were rejected
- Keep custom patches minimal and focused

## Emergency Procedures

### Hotfix Process
1. Branch from `main`: `hotfix/critical-fix`
2. Make minimal changes
3. Test thoroughly
4. Merge to both `main` and `development`
5. Tag new patch version

### Rollback Process
1. Identify last known good tag
2. Create branch from tag: `rollback/to-v1.8.0-o1`
3. Test and deploy
4. Investigate and fix issues in development

## Monitoring and Metrics

### Key Metrics
- Upstream sync frequency (target: weekly)
- Custom feature test coverage (target: >80%)
- Release frequency (target: monthly for features, as-needed for fixes)
- Merge conflict resolution time (target: <24h)

### Tools
- GitHub Actions for CI/CD
- PlatformIO for testing
- GitHub Issues for tracking
- GitHub Projects for feature planning