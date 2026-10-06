#!/bin/bash
# SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Fetch linuxdeepin/deepin-icon-theme at a pinned commit.
# Usage: tools/fetch-deepin-icon-theme.sh [target dir]
set -euo pipefail

REPO="https://github.com/linuxdeepin/deepin-icon-theme.git"
# Pinned upstream commit (master, 2026-06-22). Update deliberately and re-check the result.
COMMIT="5d11ba4e4013a4d1e1c601265d55c66b95184922"
TARGET="${1:-build/deepin-icon-theme}"

if [[ -d "$TARGET/.git" ]]; then
    git -C "$TARGET" fetch --depth 1 origin "$COMMIT"
else
    mkdir -p "$TARGET"
    git -C "$TARGET" init -q
    git -C "$TARGET" remote add origin "$REPO"
    git -C "$TARGET" fetch --depth 1 origin "$COMMIT"
fi
git -C "$TARGET" -c advice.detachedHead=false checkout -q FETCH_HEAD
echo "deepin-icon-theme $COMMIT -> $TARGET"
