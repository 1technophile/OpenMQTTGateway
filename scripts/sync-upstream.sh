#!/bin/bash
# OpenMQTTGateway - Upstream Sync Script
# Synchronizes with the original repository and manages versioning

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
DRY_RUN=false
INTERACTIVE=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --dry-run)
            DRY_RUN=true
            shift
            ;;
        --interactive)
            INTERACTIVE=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [--dry-run] [--interactive]"
            echo "  --dry-run     Show what would be done without making changes"
            echo "  --interactive Ask for confirmation before proceeding"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

echo -e "${CYAN}🔄 OpenMQTTGateway Fork Sync Utility${NC}"
echo -e "${CYAN}=====================================${NC}"

# Check if we're in the right directory
if [[ ! -f "platformio.ini" ]]; then
    echo -e "${RED}❌ Must be run from OpenMQTTGateway root directory${NC}"
    exit 1
fi

# Fetch latest from both remotes
echo -e "${YELLOW}📡 Fetching latest changes...${NC}"
git fetch upstream
git fetch origin

# Check for upstream changes
UPSTREAM_COMMITS=$(git rev-list --count HEAD..upstream/development 2>/dev/null || echo "0")
echo -e "${GREEN}📊 Upstream has $UPSTREAM_COMMITS new commits${NC}"

if [[ $UPSTREAM_COMMITS -eq 0 ]]; then
    echo -e "${GREEN}✅ Already up to date with upstream${NC}"
    exit 0
fi

# Show what's new upstream
echo -e "${BLUE}🆕 New upstream changes:${NC}"
git log --oneline --no-merges HEAD..upstream/development | head -10

if [[ $INTERACTIVE == true ]]; then
    echo -n "Continue with sync? (y/N): "
    read -r response
    if [[ ! $response =~ ^[Yy]$ ]]; then
        echo -e "${RED}❌ Sync cancelled by user${NC}"
        exit 0
    fi
fi

if [[ $DRY_RUN == true ]]; then
    echo -e "${MAGENTA}🏃‍♂️ DRY RUN - Would merge upstream changes${NC}"
    exit 0
fi

# Create sync branch
SYNC_BRANCH="sync/upstream-$(date +%Y-%m-%d)"
echo -e "${GREEN}🌿 Creating sync branch: $SYNC_BRANCH${NC}"

git checkout -b "$SYNC_BRANCH" development

# Merge upstream
echo -e "${YELLOW}🔀 Merging upstream changes...${NC}"
if git merge upstream/development --no-edit; then
    echo -e "${GREEN}✅ Merge successful!${NC}"
    
    # Push sync branch
    git push origin "$SYNC_BRANCH"
    echo -e "${GREEN}📤 Sync branch pushed to origin${NC}"
    
    echo -e "${CYAN}🔧 Next steps:${NC}"
    echo -e "${WHITE}  1. Review changes in branch: $SYNC_BRANCH${NC}"
    echo -e "${WHITE}  2. Test your custom features${NC}"
    echo -e "${WHITE}  3. Create PR: $SYNC_BRANCH -> development${NC}"
    
else
    echo -e "${RED}❌ Merge conflicts detected!${NC}"
    echo -e "${YELLOW}🛠️  Resolve conflicts manually and run:${NC}"
    echo -e "${WHITE}   git add .${NC}"
    echo -e "${WHITE}   git commit${NC}"
    echo -e "${WHITE}   git push origin $SYNC_BRANCH${NC}"
    exit 1
fi

echo -e "${GREEN}✅ Sync completed successfully!${NC}"