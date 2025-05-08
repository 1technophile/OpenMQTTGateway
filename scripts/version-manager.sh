#!/bin/bash
# OpenMQTTGateway - Version Manager
# Manages versioning for custom OpenMQTTGateway fork

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
WHITE='\033[1;37m'
NC='\033[0m' # No Color

# Default values
ACTION=""
CUSTOM_SUFFIX="o"
DRY_RUN=false

# Version file paths
VERSION_FILE="main/version.h"
PACKAGE_FILE="package.json"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        patch|minor|major|show|tag)
            ACTION="$1"
            shift
            ;;
        --suffix)
            CUSTOM_SUFFIX="$2"
            shift 2
            ;;
        --dry-run)
            DRY_RUN=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 {patch|minor|major|show|tag} [--suffix SUFFIX] [--dry-run]"
            echo ""
            echo "Actions:"
            echo "  patch   Bump patch version (x.y.Z)"
            echo "  minor   Bump minor version (x.Y.z)"
            echo "  major   Bump major version (X.y.z)"
            echo "  show    Display current version info"
            echo "  tag     Create git tag for current version"
            echo ""
            echo "Options:"
            echo "  --suffix SUFFIX  Custom suffix (default: 'o')"
            echo "  --dry-run        Show what would be done"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

if [[ -z "$ACTION" ]]; then
    echo -e "${RED}❌ Action required. Use --help for usage information${NC}"
    exit 1
fi

get_upstream_version() {
    # Get the latest upstream version
    git fetch upstream --tags --quiet 2>/dev/null || true
    local upstream_tag=$(git tag -l --sort=-version:refname | grep -E '^v?[0-9]+\.[0-9]+\.[0-9]+$' | head -1)
    if [[ -n "$upstream_tag" ]]; then
        echo "${upstream_tag#v}"
    else
        echo "1.7.0"  # Fallback
    fi
}

get_current_version() {
    local version=""
    
    # Try version.h first
    if [[ -f "$VERSION_FILE" ]]; then
        version=$(grep -o '#define\s\+version\s\+"[^"]\+"' "$VERSION_FILE" 2>/dev/null | sed 's/.*"\([^"]*\)".*/\1/' || true)
    fi
    
    # Try package.json if version.h didn't work
    if [[ -z "$version" && -f "$PACKAGE_FILE" ]]; then
        if command -v jq >/dev/null 2>&1; then
            version=$(jq -r '.version' "$PACKAGE_FILE" 2>/dev/null || true)
        else
            version=$(grep -o '"version":\s*"[^"]*"' "$PACKAGE_FILE" 2>/dev/null | sed 's/.*"\([^"]*\)".*/\1/' || true)
        fi
    fi
    
    # Fallback
    if [[ -z "$version" ]]; then
        version="1.0.0"
    fi
    
    echo "$version"
}

set_version() {
    local new_version="$1"
    
    echo -e "${GREEN}📝 Updating version to: $new_version${NC}"
    
    # Update version.h if exists
    if [[ -f "$VERSION_FILE" ]]; then
        if [[ "$DRY_RUN" == false ]]; then
            sed -i.bak "s/#define\s\+version\s\+\"[^\"]*\"/#define version \"$new_version\"/" "$VERSION_FILE"
            rm -f "${VERSION_FILE}.bak"
            echo -e "${GREEN}✅ Updated $VERSION_FILE${NC}"
        fi
    fi
    
    # Update package.json if exists
    if [[ -f "$PACKAGE_FILE" ]]; then
        if [[ "$DRY_RUN" == false ]]; then
            if command -v jq >/dev/null 2>&1; then
                jq ".version = \"$new_version\"" "$PACKAGE_FILE" > "${PACKAGE_FILE}.tmp" && mv "${PACKAGE_FILE}.tmp" "$PACKAGE_FILE"
            else
                sed -i.bak "s/\"version\":\s*\"[^\"]*\"/\"version\": \"$new_version\"/" "$PACKAGE_FILE"
                rm -f "${PACKAGE_FILE}.bak"
            fi
            echo -e "${GREEN}✅ Updated $PACKAGE_FILE${NC}"
        fi
    fi
}

new_version() {
    local type="$1"
    local current=$(get_current_version)
    
    # Extract version numbers (remove any suffix)
    local version_base=$(echo "$current" | sed 's/-.*$//')
    IFS='.' read -ra parts <<< "$version_base"
    
    # Ensure we have 3 parts
    while [[ ${#parts[@]} -lt 3 ]]; do
        parts+=(0)
    done
    
    case "$type" in
        "major")
            parts[0]=$((parts[0] + 1))
            parts[1]=0
            parts[2]=0
            ;;
        "minor")
            parts[1]=$((parts[1] + 1))
            parts[2]=0
            ;;
        "patch")
            parts[2]=$((parts[2] + 1))
            ;;
    esac
    
    echo "${parts[0]}.${parts[1]}.${parts[2]}-$CUSTOM_SUFFIX"
}

# Main logic
echo -e "${CYAN}🔖 OpenMQTTGateway Version Manager${NC}"
echo -e "${CYAN}===================================${NC}"

current_version=$(get_current_version)
upstream_version=$(get_upstream_version)

case "$ACTION" in
    "show")
        echo -e "${BLUE}📊 Version Information:${NC}"
        echo -e "${WHITE}  Current: $current_version${NC}"
        echo -e "${WHITE}  Upstream: $upstream_version${NC}"
        
        # Show recent tags
        echo -e "${BLUE}📋 Recent releases:${NC}"
        git tag -l --sort=-version:refname | grep -E "${CUSTOM_SUFFIX}[0-9]*$" | head -5 | while read -r tag; do
            echo -e "${WHITE}  $tag${NC}"
        done
        ;;
    
    "tag")
        tag_name="v$current_version"
        echo -e "${GREEN}🏷️  Creating tag: $tag_name${NC}"
        
        if [[ "$DRY_RUN" == false ]]; then
            git tag -a "$tag_name" -m "Release $current_version - Custom OpenMQTTGateway build"
            git push origin "$tag_name"
            echo -e "${GREEN}✅ Tag created and pushed${NC}"
        else
            echo -e "${MAGENTA}🏃‍♂️ DRY RUN - Would create tag: $tag_name${NC}"
        fi
        ;;
    
    *)
        new_ver=$(new_version "$ACTION")
        echo -e "${YELLOW}⬆️  Bumping version: $current_version → $new_ver${NC}"
        
        if [[ "$DRY_RUN" == false ]]; then
            set_version "$new_ver"
            
            # Commit version change
            git add "$VERSION_FILE" "$PACKAGE_FILE" 2>/dev/null || true
            git commit -m "chore: bump version to $new_ver"
            
            echo -e "${GREEN}✅ Version bumped and committed${NC}"
            echo -e "${CYAN}🔧 Next steps:${NC}"
            echo -e "${WHITE}  1. Test the build: pio run -e esp32dev-all-test${NC}"
            echo -e "${WHITE}  2. Create tag: ./scripts/version-manager.sh tag${NC}"
            echo -e "${WHITE}  3. Push changes: git push origin${NC}"
        else
            echo -e "${MAGENTA}🏃‍♂️ DRY RUN - Would set version to: $new_ver${NC}"
        fi
        ;;
esac