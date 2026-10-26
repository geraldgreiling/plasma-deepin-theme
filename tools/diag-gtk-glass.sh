#!/bin/bash
# SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Collects everything needed to find out why a GTK window is transparent but not
# blurred (Better Blur DX setup of tools/setup-gtk-glass.sh). Click the affected
# window when asked. The report is written next to this checkout:
#   ../gtk-glass-diag.txt
cd "$(dirname "${BASH_SOURCE[0]}")/.." || exit 1
OUT="$(cd .. && pwd)/gtk-glass-diag.txt"
CONF="${XDG_CONFIG_HOME:-$HOME/.config}"

section() { printf '\n===== %s\n' "$*"; }

{
    section "Session"
    echo "XDG_SESSION_TYPE=${XDG_SESSION_TYPE:-?}  GDK_BACKEND=${GDK_BACKEND:-(unset)}"

    section "Packages"
    command -v pacman >/dev/null && pacman -Q kwin kwin-effects-better-blur-dx kwin-effects-better-blur-dx-git gtk3 2>&1

    section "Effect plugin files"
    PLUGINS="$(qtpaths6 --plugin-dir 2>/dev/null || echo /usr/lib/qt6/plugins)"
    ls -l "$PLUGINS"/kwin/effects/plugins/ 2>&1 | grep -i blur

    section "kwinrc [Plugins] (blur)"
    sed -n '/^\[Plugins\]/,/^\[/p' "$CONF/kwinrc" | grep -i blur

    section "kwinrc [Effect-better-blur-dx]"
    sed -n '/^\[Effect-better-blur-dx\]/,/^\[/p' "$CONF/kwinrc"

    section "Loaded effects (blur)"
    qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.loadedEffects 2>&1 | grep -i blur
    echo "isEffectLoaded(better_blur_dx): $(qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.isEffectLoaded better_blur_dx 2>&1)"
    echo "isEffectSupported(better_blur_dx): $(qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.isEffectSupported better_blur_dx 2>&1)"

    section "KWin support information (compositing, blur)"
    qdbus6 org.kde.KWin /KWin supportInformation 2>&1 | grep -iE -A3 'compositing|^blur|better_blur|openGL renderer' | head -n 60

    section "GTK theme"
    grep -h gtk-theme-name "$CONF/gtk-3.0/settings.ini" "$CONF/gtk-4.0/settings.ini" 2>&1

    section "Clicked window"
} >"$OUT" 2>&1

echo "Click the window that is transparent but not blurred ..."
qdbus6 org.kde.KWin /KWin org.kde.KWin.queryWindowInfo >>"$OUT" 2>&1

{
    section "KWin log (blur)"
    journalctl --user -b --no-pager 2>/dev/null | grep -iE 'better.?blur|bbdx|blur' | tail -n 40
} >>"$OUT" 2>&1

echo "Report written to $OUT"
