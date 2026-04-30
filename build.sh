#!/usr/bin/env bash
# ZMK firmware build script
# Supports two build methods:
#   ./build.sh           → trigger GitHub Actions build and download firmware
#   ./build.sh local     → build locally using Docker (slow on Apple Silicon!)
#   ./build.sh clean     → clean local build cache
#
# GitHub Actions mode (default):
#   Commits and pushes current changes to a build branch, triggers CI,
#   waits for completion, and downloads the .uf2 files to firmware/
#
# Local Docker mode:
#   Uses zmkfirmware/zmk-build-arm Docker image. First build takes ~30 min
#   to download ZMK+Zephyr deps. Subsequent builds are faster (~2 min).
#   NOTE: Very slow on Apple Silicon Macs due to x86 emulation via Rosetta.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUTPUT_DIR="$SCRIPT_DIR/firmware"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

#──────────────────────────────────────────────
# GitHub Actions build (default)
#──────────────────────────────────────────────
build_ci() {
    echo -e "${YELLOW}▶ Building via GitHub Actions...${NC}"

    # Get repo info from git remote
    local remote_url
    remote_url=$(git -C "$SCRIPT_DIR" remote get-url origin 2>/dev/null)
    local owner repo
    if [[ "$remote_url" =~ github.com[:/]([^/]+)/([^/.]+) ]]; then
        owner="${BASH_REMATCH[1]}"
        repo="${BASH_REMATCH[2]}"
    else
        echo -e "${RED}✗ Could not parse GitHub owner/repo from remote: $remote_url${NC}"
        exit 1
    fi

    # Check for uncommitted changes
    if ! git -C "$SCRIPT_DIR" diff --quiet HEAD 2>/dev/null; then
        echo -e "${YELLOW}  You have uncommitted changes. Commit them first.${NC}"
        exit 1
    fi

    # Push current branch
    local branch
    branch=$(git -C "$SCRIPT_DIR" rev-parse --abbrev-ref HEAD)
    echo -e "${CYAN}  Pushing ${branch} to origin...${NC}"
    git -C "$SCRIPT_DIR" push origin "$branch" 2>&1 | tail -3

    # Wait for workflow to start
    echo -e "${CYAN}  Waiting for GitHub Actions workflow to start...${NC}"
    sleep 5

    # Find the latest workflow run
    local run_id status conclusion
    for i in $(seq 1 60); do
        run_id=$(gh api "repos/$owner/$repo/actions/runs?branch=$branch&per_page=1" \
            --jq '.workflow_runs[0].id' 2>/dev/null || echo "")
        status=$(gh api "repos/$owner/$repo/actions/runs?branch=$branch&per_page=1" \
            --jq '.workflow_runs[0].status' 2>/dev/null || echo "")

        if [ -n "$run_id" ] && [ "$status" = "completed" ]; then
            conclusion=$(gh api "repos/$owner/$repo/actions/runs/$run_id" \
                --jq '.conclusion' 2>/dev/null || echo "")
            break
        elif [ -n "$run_id" ] && [ "$status" != "completed" ]; then
            printf "\r  ⏳ Run #%s status: %-15s (attempt %d/60)" "$run_id" "$status" "$i"
            sleep 10
        else
            printf "\r  ⏳ Waiting for run to appear... (attempt %d/60)" "$i"
            sleep 5
        fi
    done
    echo ""

    if [ "$conclusion" != "success" ]; then
        echo -e "${RED}✗ Build failed (conclusion: $conclusion)${NC}"
        echo -e "${RED}  Check: https://github.com/$owner/$repo/actions/runs/$run_id${NC}"
        exit 1
    fi

    echo -e "${GREEN}✓ Build succeeded!${NC}"

    # Download artifacts
    mkdir -p "$OUTPUT_DIR"
    echo -e "${CYAN}  Downloading firmware artifacts...${NC}"
    cd "$OUTPUT_DIR"
    gh run download "$run_id" -R "$owner/$repo" 2>&1 || true

    # Move .uf2 files to firmware root
    find . -name "*.uf2" -mindepth 2 -exec mv {} . \; 2>/dev/null
    find . -type d -empty -delete 2>/dev/null

    echo ""
    echo -e "${GREEN}═══════════════════════════════════════════${NC}"
    echo -e "${GREEN}  Firmware ready in: firmware/             ${NC}"
    echo -e "${GREEN}═══════════════════════════════════════════${NC}"
    ls -lh "$OUTPUT_DIR"/*.uf2 2>/dev/null || echo "  (no .uf2 files found)"
}

#──────────────────────────────────────────────
# Local Docker build
#──────────────────────────────────────────────
build_local() {
    local DOCKER_IMAGE="zmkfirmware/zmk-build-arm:3.5"
    local WORKSPACE="$SCRIPT_DIR/.zmk_build"
    local BOARD="nice_nano_v2"
    local side="${1:-both}"

    # Warn on Apple Silicon
    if [ "$(uname -m)" = "arm64" ]; then
        echo -e "${YELLOW}⚠ Apple Silicon detected — Docker build uses x86 emulation (slow!)${NC}"
        echo -e "${YELLOW}  Consider using './build.sh' (GitHub Actions) instead.${NC}"
        echo ""
    fi

    echo -e "${YELLOW}▶ Checking Docker image...${NC}"
    if ! docker image inspect "$DOCKER_IMAGE" &>/dev/null; then
        echo -e "${YELLOW}  Pulling ${DOCKER_IMAGE} (first time only)...${NC}"
        docker pull "$DOCKER_IMAGE"
    fi

    mkdir -p "$WORKSPACE"

    local shields_left="corne_left nice_view_adapter nice_view"
    local shields_right="corne_right nice_view_adapter nice_view"

    build_docker_side() {
        local s=$1 shields=$2
        echo -e "${YELLOW}▶ Building ${s} half...${NC}"
        docker run --rm \
            -v "$WORKSPACE:/zmk_ws" \
            -v "$SCRIPT_DIR/config:/zmk_ws/config" \
            -w /zmk_ws \
            "$DOCKER_IMAGE" \
            sh -c "
                set -e
                if [ ! -f .west/config ]; then
                    echo '==> Initializing west workspace...'
                    west init -l config
                    echo '==> Fetching modules (first time)...'
                    west update
                    west zephyr-export
                elif [ ! -d zmk ] || [ ! -f zephyr/.cmake/ZephyrConfig.cmake ]; then
                    echo '==> Fetching/re-registering modules...'
                    west update
                    west zephyr-export
                else
                    echo '==> Modules cached, skipping west update'
                fi
                echo '==> Building ${s}...'
                west build -s zmk/app -b ${BOARD} -d build/${s} -p auto \
                    -- -DSHIELD=\"${shields}\" -DZMK_CONFIG=/zmk_ws/config
            "
        local uf2="$WORKSPACE/build/${s}/zephyr/zmk.uf2"
        if [ -f "$uf2" ]; then
            mkdir -p "$OUTPUT_DIR"
            cp "$uf2" "$OUTPUT_DIR/${BOARD}-${s}.uf2"
            echo -e "${GREEN}✓ ${s} → firmware/${BOARD}-${s}.uf2${NC}"
        else
            echo -e "${RED}✗ ${s} build failed${NC}"
            return 1
        fi
    }

    case "$side" in
        left)  build_docker_side "left" "$shields_left" ;;
        right) build_docker_side "right" "$shields_right" ;;
        both)
            build_docker_side "left" "$shields_left"
            build_docker_side "right" "$shields_right"
            ;;
    esac

    echo ""
    echo -e "${GREEN}═══════════════════════════════════════════${NC}"
    echo -e "${GREEN}  Firmware ready in: firmware/             ${NC}"
    echo -e "${GREEN}═══════════════════════════════════════════${NC}"
    ls -lh "$OUTPUT_DIR"/*.uf2 2>/dev/null || true
}

#──────────────────────────────────────────────
# Main
#──────────────────────────────────────────────
case "${1:-ci}" in
    ci|"")     build_ci ;;
    local)     build_local "${2:-both}" ;;
    update)
        echo -e "${YELLOW}▶ Updating ZMK/Zephyr modules...${NC}"
        local WORKSPACE="$SCRIPT_DIR/.zmk_build"
        docker run --rm \
            -v "$WORKSPACE:/zmk_ws" \
            -v "$SCRIPT_DIR/config:/zmk_ws/config" \
            -w /zmk_ws \
            "zmkfirmware/zmk-build-arm:3.5" \
            sh -c "west update && west zephyr-export"
        echo -e "${GREEN}✓ Modules updated${NC}"
        ;;
    clean)
        echo -e "${YELLOW}▶ Cleaning local build cache...${NC}"
        rm -rf "$SCRIPT_DIR/.zmk_build/build" "$SCRIPT_DIR/.zmk_build/zephyr/.cmake" "$OUTPUT_DIR"
        echo -e "${GREEN}✓ Cleaned (modules kept, CMake cache cleared)${NC}"
        echo -e "${CYAN}  Use './build.sh nuke' to remove everything including modules${NC}"
        ;;
    nuke)
        echo -e "${YELLOW}▶ Removing entire build workspace...${NC}"
        rm -rf "$SCRIPT_DIR/.zmk_build" "$OUTPUT_DIR"
        echo -e "${GREEN}✓ Nuked${NC}"
        ;;
    help|-h|--help)
        echo "Usage: ./build.sh [command]"
        echo ""
        echo "Commands:"
        echo "  (default)       Build via GitHub Actions (fast, recommended)"
        echo "  local [side]    Build locally with Docker (side: left|right|both)"
        echo "  update          Force-update ZMK/Zephyr modules in Docker cache"
        echo "  clean           Remove build output (keeps cached modules)"
        echo "  nuke            Remove everything including cached modules"
        echo "  help            Show this help"
        ;;
    *)
        echo "Unknown command: $1 (try: ./build.sh help)"
        exit 1
        ;;
esac


