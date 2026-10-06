#!/bin/bash
# SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Build the archives that are uploaded to the KDE Store (store.kde.org),
# one per component, into dist/.
#   Color schemes ........ DeepinLight.colors, DeepinDark.colors
#   Plasma style ......... plasma-deepin-plasma-style.tar.gz
#   Global themes ........ org.plasmadeepin.light.desktop.tar.gz, org.plasmadeepin.dark.desktop.tar.gz
#   Icons ................ Deepin-Bloom-icons.tar.xz
#   Cursors .............. Deepin-Bloom-cursors.tar.xz
#   SDDM ................. plasma-deepin-sddm.tar.gz
#   Plymouth ............. plasma-deepin-plymouth.tar.gz
#   GTK 3/4 .............. Deepin-Glass-gtk.tar.gz
# The window decoration and the application style are native plugins and are
# distributed through the AUR (packaging/aur), not the KDE Store.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
ROOT="$PWD"
DIST="$ROOT/dist"
BUILD="$ROOT/build"
rm -rf "$DIST"
mkdir -p "$DIST" "$BUILD"

cp color-schemes/DeepinLight.colors color-schemes/DeepinDark.colors "$DIST/"
tar -C plasma/desktoptheme -czf "$DIST/plasma-deepin-plasma-style.tar.gz" plasma-deepin
for lnf in org.plasmadeepin.light.desktop org.plasmadeepin.dark.desktop; do
    tar -C plasma/look-and-feel -czf "$DIST/$lnf.tar.gz" "$lnf"
done
tar -C sddm -czf "$DIST/plasma-deepin-sddm.tar.gz" plasma-deepin
tar -C plymouth -czf "$DIST/plasma-deepin-plymouth.tar.gz" plasma-deepin
tar -C gtk -czf "$DIST/Deepin-Glass-gtk.tar.gz" Deepin-Glass Deepin-Glass-Dark libadwaita

# icons and cursors are built from linuxdeepin/deepin-icon-theme
tools/fetch-deepin-icon-theme.sh "$BUILD/deepin-icon-theme"
rm -rf "$BUILD/icon-themes"
python3 tools/build-icon-themes.py "$BUILD/deepin-icon-theme" "$BUILD/icon-themes"
tar -C "$BUILD/icon-themes/icons" -cJf "$DIST/Deepin-Bloom-icons.tar.xz" Deepin-Bloom Deepin-Bloom-Dark
tar -C "$BUILD/icon-themes/cursors" -cJf "$DIST/Deepin-Bloom-cursors.tar.xz" Deepin-Bloom-Cursors Deepin-Bloom-Cursors-Dark

ls -lh "$DIST"
