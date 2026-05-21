#!/usr/bin/env bash
# release.sh — CARM release script
#
# Usage:
#   ./release.sh                   # release using current VERSION
#   ./release.sh 0.3.0             # bump VERSION to 0.3.0 and release
#   ./release.sh 0.3.0 --dry-run   # validate without producing zip
#
# What this script does:
#   1. Optionally updates VERSION to the specified version string
#   2. Validates VERSION matches the top entry in CHANGELOG.md
#   3. Runs a clean build (make clean && make all)
#   4. Runs the full test suite (make test) — aborts on failure
#   5. Produces carm-v{VERSION}.zip in the current directory
#
# The zip contains the full repo tree minus: build/, *.o, .DS_Store,
# __pycache__, and the zip itself.

set -euo pipefail

## ── Helpers ─────────────────────────────────────────────────────────────
red()    { printf "\033[31m%s\033[0m\n" "$*"; }
green()  { printf "\033[32m%s\033[0m\n" "$*"; }
yellow() { printf "\033[33m%s\033[0m\n" "$*"; }
bold()   { printf "\033[1m%s\033[0m\n" "$*"; }

die() { red "ERROR: $*"; exit 1; }

## ── Parse arguments ─────────────────────────────────────────────────────
NEW_VERSION=""
DRY_RUN=0

for arg in "$@"; do
    case "$arg" in
        --dry-run) DRY_RUN=1 ;;
        --*)       die "Unknown option: $arg" ;;
        *)         NEW_VERSION="$arg" ;;
    esac
done

## ── Step 1: Optionally bump VERSION ─────────────────────────────────────
if [ -n "$NEW_VERSION" ]; then
    # Validate format: X.Y.Z or X.Y.Z-suffix
    if ! echo "$NEW_VERSION" | grep -qE '^[0-9]+\.[0-9]+\.[0-9]+(-[a-zA-Z0-9._-]+)?$'; then
        die "Invalid version format: '$NEW_VERSION'. Expected X.Y.Z or X.Y.Z-suffix."
    fi
    bold "Bumping VERSION to $NEW_VERSION"
    echo "$NEW_VERSION" > VERSION
fi

VERSION=$(cat VERSION | tr -d '[:space:]')
[ -z "$VERSION" ] && die "VERSION file is empty."

bold "Releasing CARM v${VERSION}"
echo ""

## ── Step 2: Validate CHANGELOG ──────────────────────────────────────────
yellow "Checking CHANGELOG.md..."

# Extract the first version tag from CHANGELOG.md — must be [VERSION]
CHANGELOG_VERSION=$(grep -m1 '^## \[' CHANGELOG.md | sed 's/.*\[\([0-9][^]]*\)\].*/\1/')

if [ "$CHANGELOG_VERSION" != "$VERSION" ]; then
    die "VERSION ($VERSION) does not match top CHANGELOG.md entry ([$CHANGELOG_VERSION]).\n  Update CHANGELOG.md before releasing."
fi
green "  CHANGELOG.md top entry matches VERSION: [$VERSION]"

## ── Step 3: Clean build ─────────────────────────────────────────────────
yellow "Building..."
make clean > /dev/null
if ! make all 2>&1 | tee /tmp/carm_build.log | grep -E "^gcc|error:|warning:" ; then
    # No output from grep is fine — means no errors/warnings to show
    true
fi

# Check for actual errors
if grep -q "error:" /tmp/carm_build.log 2>/dev/null; then
    red "Build errors detected:"
    grep "error:" /tmp/carm_build.log
    die "Build failed."
fi
green "  Build succeeded (zero errors)"

## ── Step 4: Test suite ───────────────────────────────────────────────────
yellow "Running tests..."
if ! make test > /tmp/carm_test.log 2>&1; then
    red "Test suite failed:"
    cat /tmp/carm_test.log
    die "Tests failed — release aborted."
fi

# Verify all tests passed
PASSED=$(grep "Passed:" /tmp/carm_test.log | sed 's/\x1b\[[0-9;]*m//g' | grep -oE '[0-9]+' | tail -1)
FAILED=$(grep "Failed:" /tmp/carm_test.log | sed 's/\x1b\[[0-9;]*m//g' | grep -oE '[0-9]+' | tail -1)

if [ "${FAILED:-1}" != "0" ]; then
    red "Test failures detected ($FAILED failed):"
    cat /tmp/carm_test.log
    die "Tests failed — release aborted."
fi
green "  All ${PASSED} tests passed"

## ── Step 5: Produce zip ─────────────────────────────────────────────────
ZIP_NAME="carm-v${VERSION}.zip"

if [ "$DRY_RUN" = "1" ]; then
    yellow ""
    yellow "Dry run — skipping zip creation."
    yellow "Would produce: $ZIP_NAME"
    green ""
    green "✓ Dry run complete. Ready to release v${VERSION}."
    exit 0
fi

yellow "Creating $ZIP_NAME..."

# Remove any previous zip with same name
rm -f "$ZIP_NAME"

zip -r "$ZIP_NAME" . \
    --exclude "build/*" \
    --exclude "*.zip" \
    --exclude "*/.DS_Store" \
    --exclude "*/__pycache__/*" \
    --exclude "*.o" \
    --exclude ".git/*" \
    --exclude "/tmp/carm_*.log" \
    > /dev/null

SIZE=$(du -sh "$ZIP_NAME" | cut -f1)
green "  Created $ZIP_NAME ($SIZE)"

## ── Done ────────────────────────────────────────────────────────────────
echo ""
bold "✓ Released CARM v${VERSION}"
echo "  Zip: $ZIP_NAME"
echo ""
echo "  Next steps:"
echo "    git add VERSION CHANGELOG.md"
echo "    git commit -m \"Release v${VERSION}\""
echo "    git tag v${VERSION}"
echo "    git push && git push --tags"
echo ""
