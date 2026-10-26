#!/bin/bash
# SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Collects everything needed to find out why a GTK window is transparent but not
# blurred (Better Blur DX setup of tools/setup-gtk-glass.sh). Click the affected
# window when asked. The report is written next to this checkout:
#   ../gtk-glass-diag.txt
#
#   tools/diag-gtk-glass.sh --debug-on    let KWin log what Better Blur DX does
#                                         (takes effect after logging in again)
#   tools/diag-gtk-glass.sh --debug-off   switch that off again
cd "$(dirname "${BASH_SOURCE[0]}")/.." || exit 1
OUT="$(cd .. && pwd)/gtk-glass-diag.txt"
CONF="${XDG_CONFIG_HOME:-$HOME/.config}"
RULES="$CONF/QtProject/qtlogging.ini"
RULE="kwin_effect_better_blur_dx*.debug=true"

case "${1:-}" in
--debug-on)
    mkdir -p "$(dirname "$RULES")"
    if ! grep -qF "$RULE" "$RULES" 2>/dev/null; then
        if grep -q '^\[Rules\]' "$RULES" 2>/dev/null; then
            sed -i "/^\[Rules\]/a $RULE" "$RULES"
        else
            printf '[Rules]\n%s\n' "$RULE" >>"$RULES"
        fi
    fi
    echo "Debug output of Better Blur DX is on. Log out and in, start the affected"
    echo "application and one that is blurred correctly, then run tools/diag-gtk-glass.sh"
    exit 0
    ;;
--debug-off)
    [[ -f "$RULES" ]] && sed -i "\|^${RULE//\*/\\*}\$|d" "$RULES"
    echo "Debug output of Better Blur DX is off (after logging in again)."
    exit 0
    ;;
esac

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
    grep -F "$RULE" "$RULES" >/dev/null 2>&1 && echo "(debug output is on)"
    journalctl --user -b --no-pager -o cat 2>/dev/null | grep -iE -A8 'better_blur_dx' | tail -n 400
} >>"$OUT" 2>&1

echo "Report written to $OUT"
