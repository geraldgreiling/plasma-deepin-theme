#!/bin/bash
# SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Glass for GTK 3/4 applications (optional).
#
# GTK applications cannot ask KWin for blur, so the regular GTK themes are opaque.
# This script
#   * installs "Deepin-Glass-Translucent(-Dark)" with the opacities of deepinglassrc,
#   * replaces KWin's blur effect with "Better Blur DX" (third party, AUR package
#     kwin-effects-better-blur-dx), which also blurs behind windows it is told to,
#   * tells it to blur the GTK applications found on this system (including
#     libadwaita and Flatpak applications),
#   * selects the translucent GTK theme and installs the translucent libadwaita
#     stylesheet as ~/.config/gtk-4.0/gtk.css (an existing foreign file is backed up),
#   * lets Flatpak applications read that stylesheet.
# Qt applications and the window decoration keep working: Better Blur DX handles
# their blur requests like KWin's own effect.
#
#   tools/setup-gtk-glass.sh            set up (again, e.g. after installing GTK apps)
#   tools/setup-gtk-glass.sh --off      back to KWin's blur and the opaque GTK theme
#   tools/setup-gtk-glass.sh --list     only print the GTK applications that would be blurred
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
DATA="${XDG_DATA_HOME:-$HOME/.local/share}"
CONF="${XDG_CONFIG_HOME:-$HOME/.config}"
GROUP="Effect-better-blur-dx"

dark=false
if kreadconfig6 --file kdeglobals --group General --key ColorScheme 2>/dev/null | grep -qi dark; then
    dark=true
fi

# --------------------------------------------------------------------- helpers
set_gtk_theme() {
    local theme="$1"
    for v in 3.0 4.0; do
        mkdir -p "$CONF/gtk-$v"
        kwriteconfig6 --file "$CONF/gtk-$v/settings.ini" --group Settings --key gtk-theme-name "$theme"
    done
    if command -v gsettings >/dev/null; then
        gsettings set org.gnome.desktop.interface gtk-theme "$theme" 2>/dev/null || true
    fi
    # Plasma's GTK integration (kded) keeps xsettingsd and the portal in sync
    qdbus6 org.kde.kded6 /modules/gtkconfig org.kde.gtkconfig.setGtkTheme "$theme" >/dev/null 2>&1 || true
}

reload_kwin() {
    qdbus6 org.kde.KWin /KWin reconfigure >/dev/null 2>&1 || true
    if [[ "$1" == on ]]; then
        qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.unloadEffect blur >/dev/null 2>&1 || true
        qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.loadEffect better_blur_dx >/dev/null 2>&1 || true
    else
        qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.unloadEffect better_blur_dx >/dev/null 2>&1 || true
        qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.loadEffect blur >/dev/null 2>&1 || true
    fi
}

# Window classes of the installed GTK 3/4 applications. On Wayland KWin matches the
# app id, usually the desktop file name; the binary name and StartupWMClass cover
# X11 and older applications. libadwaita applications ignore GTK themes but load
# the user stylesheet ~/.config/gtk-4.0/gtk.css, which gets the glass as well.
# Flatpak applications cannot be inspected; their app ids are all added (blur
# behind an opaque window costs a little GPU time but is invisible).
gtk_classes() {
    local dirs=(/usr/share/applications "$DATA/applications")
    local f exe bin libs
    find "${dirs[@]}" -maxdepth 1 -name '*.desktop' 2>/dev/null | while IFS= read -r f; do
        grep -q '^NoDisplay=true' "$f" && continue
        exe="$(grep -m1 '^Exec=' "$f" | sed 's/^Exec=//; s/^env \+\([A-Za-z_]\+=[^ ]* \+\)*//' | awk '{print $1}')"
        [[ -n "$exe" ]] || continue
        bin="$(command -v "$exe" 2>/dev/null || true)"
        [[ -n "$bin" && -f "$bin" ]] || continue
        libs="$(ldd "$bin" 2>/dev/null || true)"
        grep -q 'libgtk-[34]\.so' <<<"$libs" || continue
        basename "$f" .desktop
        basename "$bin"
        grep -m1 '^StartupWMClass=' "$f" | sed 's/^StartupWMClass=//' || true
    done | sort -u || true
    if command -v flatpak >/dev/null; then
        flatpak list --app --columns=application 2>/dev/null || true
    fi
}

LIBADWAITA_CSS="$CONF/gtk-4.0/gtk.css"
is_our_stylesheet() {
    [[ -f "$LIBADWAITA_CSS" ]] && grep -q 'Deepin Glass for libadwaita applications' "$LIBADWAITA_CSS"
}

# ------------------------------------------------------------------------ --list
if [[ "${1:-}" == --list ]]; then
    gtk_classes
    exit 0
fi

# ------------------------------------------------------------------------- --off
if [[ "${1:-}" == --off ]]; then
    kwriteconfig6 --file kwinrc --group Plugins --key better_blur_dxEnabled false
    kwriteconfig6 --file kwinrc --group Plugins --key blurEnabled true
    reload_kwin off
    set_gtk_theme "$($dark && echo Deepin-Glass-Dark || echo Deepin-Glass)"
    if is_our_stylesheet; then
        cp gtk/libadwaita/gtk.css "$LIBADWAITA_CSS"
    fi
    echo "KWin blur and the opaque GTK themes are active again. Restart GTK applications."
    exit 0
fi

# -------------------------------------------------------------------------- setup
PLUGINS="$(qtpaths6 --plugin-dir 2>/dev/null || echo /usr/lib/qt6/plugins)"
if [[ ! -f "$PLUGINS/kwin/effects/plugins/better_blur_dx.so" ]]; then
    cat <<'MSG'
The KWin effect "Better Blur DX" is not installed. Install it first, e.g.

    paru -S kwin-effects-better-blur-dx      (or yay -S ...)

then run this script again. Note: it is built for one exact KWin version and has
to be rebuilt after KWin updates (the AUR helper does that on the next upgrade).
MSG
    exit 1
fi

echo ":: translucent GTK themes"
active="$(kreadconfig6 --file deepinglassrc --group Decoration --key ActiveOpacity --default 0.72)"
inactive="$(kreadconfig6 --file deepinglassrc --group Decoration --key InactiveOpacity --default 0.58)"
mkdir -p "$DATA/themes"
for t in Deepin-Glass-Translucent Deepin-Glass-Translucent-Dark; do
    rm -rf "${DATA:?}/themes/$t"
    cp -r "gtk/$t" "$DATA/themes/"
    find "$DATA/themes/$t" -name '*.css' -exec sed -i \
        -e "s|0\.72 /\*deepinglass:active\*/|$active /*deepinglass:active*/|g" \
        -e "s|0\.58 /\*deepinglass:inactive\*/|$inactive /*deepinglass:inactive*/|g" {} +
done
echo "   opacity $active (active) / $inactive (inactive), from ~/.config/deepinglassrc"

echo ":: libadwaita stylesheet (~/.config/gtk-4.0/gtk.css)"
mkdir -p "$CONF/gtk-4.0"
if [[ -f "$LIBADWAITA_CSS" ]] && ! is_our_stylesheet; then
    backup="$LIBADWAITA_CSS.bak-$(date +%Y%m%d%H%M%S)"
    cp "$LIBADWAITA_CSS" "$backup"
    echo "   your previous file is kept as $backup"
fi
sed -e "s|0\.72 /\*deepinglass:active\*/|$active /*deepinglass:active*/|g" \
    -e "s|0\.58 /\*deepinglass:inactive\*/|$inactive /*deepinglass:inactive*/|g" \
    gtk/libadwaita/gtk-glass.css >"$LIBADWAITA_CSS"
if command -v flatpak >/dev/null; then
    # read-only access to the stylesheet for all Flatpak applications
    flatpak override --user --filesystem=xdg-config/gtk-4.0:ro
    echo "   Flatpak applications may read it (flatpak override --user --filesystem=xdg-config/gtk-4.0:ro)"
fi

echo ":: GTK applications to blur"
existing="$(kreadconfig6 --file kwinrc --group "$GROUP" --key WindowClasses 2>/dev/null || true)"
# drop the effect's placeholder entries
existing="$(grep -vx 'class[123]' <<<"$existing" || true)"
classes="$( { [[ -n "$existing" ]] && echo "$existing"; gtk_classes; } | grep -v '^$' | sort -u)"
sed 's/^/   /' <<<"$classes"

echo ":: KWin: Better Blur DX instead of the blur effect"
kwriteconfig6 --file kwinrc --group "$GROUP" --key WindowClasses "$classes"
kwriteconfig6 --file kwinrc --group "$GROUP" --key BlurMatching true
kwriteconfig6 --file kwinrc --group "$GROUP" --key BlurNonMatching false
# the window decoration requests its blur itself, with rounded corners
kwriteconfig6 --file kwinrc --group "$GROUP" --key BlurDecorations false
radius="$(kreadconfig6 --file deepinglassrc --group Decoration --key CornerRadius --default 12)"
kwriteconfig6 --file kwinrc --group "$GROUP" --key CornerRadius "$radius"
kwriteconfig6 --file kwinrc --group Plugins --key blurEnabled false
kwriteconfig6 --file kwinrc --group Plugins --key better_blur_dxEnabled true
reload_kwin on

echo ":: GTK theme"
theme="$($dark && echo Deepin-Glass-Translucent-Dark || echo Deepin-Glass-Translucent)"
set_gtk_theme "$theme"
echo "   $theme"

cat <<'MSG'

Done. Restart GTK applications (if blur is missing, log out and in once).
More applications: System Settings > Desktop Effects > Better Blur DX > Force blur,
or run this script again after installing new GTK applications.
Undo: tools/setup-gtk-glass.sh --off
MSG
