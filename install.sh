#!/bin/bash
# SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Install the Deepin theme for the current user (no root needed), straight from
# this checkout. Useful for testing before the KDE Store / AUR packages exist.
#
#   ./install.sh                 colours, Plasma style, global themes, icons, cursors, GTK
#   ./install.sh --libadwaita    additionally write ~/.config/gtk-4.0/gtk.css (backup is kept)
#   ./install.sh --sddm          additionally install the SDDM theme (uses sudo)
#   ./install.sh --plymouth      additionally install and activate the Plymouth boot animation (uses sudo)
#   ./install.sh --native        additionally build and install decoration + app style (uses sudo)
#   ./install.sh --gtk-glass     additionally glass for GTK applications (needs the AUR package
#                                kwin-effects-better-blur-dx, see tools/setup-gtk-glass.sh)
#   ./install.sh --uninstall     remove everything this script installed for the user
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"
ROOT="$PWD"
DATA="${XDG_DATA_HOME:-$HOME/.local/share}"
CONF="${XDG_CONFIG_HOME:-$HOME/.config}"

ARGS=" $* "

if [[ "$ARGS" == *" --uninstall "* ]]; then
    rm -fv "$DATA/color-schemes/DeepinLight.colors" "$DATA/color-schemes/DeepinDark.colors"
    rm -rf "$DATA/plasma/desktoptheme/plasma-deepin" \
           "$DATA/plasma/look-and-feel/org.plasmadeepin.light.desktop" \
           "$DATA/plasma/look-and-feel/org.plasmadeepin.dark.desktop" \
           "$DATA/icons/Deepin-Bloom" "$DATA/icons/Deepin-Bloom-Dark" \
           "$DATA/icons/Deepin-Bloom-Cursors" "$DATA/icons/Deepin-Bloom-Cursors-Dark" \
           "$DATA/themes/Deepin-Glass" "$DATA/themes/Deepin-Glass-Dark" \
           "$DATA/themes/Deepin-Glass-Translucent" "$DATA/themes/Deepin-Glass-Translucent-Dark"
    echo "Removed. ~/.config/gtk-4.0/gtk.css and system wide parts (SDDM, AUR package) are left alone."
    echo "If you used --gtk-glass: tools/setup-gtk-glass.sh --off restores KWin's blur effect."
    exit 0
fi

echo ":: colour schemes, Plasma style, global themes"
mkdir -p "$DATA/color-schemes" "$DATA/plasma/desktoptheme" "$DATA/plasma/look-and-feel" "$DATA/icons" "$DATA/themes"
cp color-schemes/*.colors "$DATA/color-schemes/"
rm -rf "$DATA/plasma/desktoptheme/plasma-deepin"
cp -r plasma/desktoptheme/plasma-deepin "$DATA/plasma/desktoptheme/"
for lnf in org.plasmadeepin.light.desktop org.plasmadeepin.dark.desktop; do
    rm -rf "$DATA/plasma/look-and-feel/$lnf"
    cp -r "plasma/look-and-feel/$lnf" "$DATA/plasma/look-and-feel/"
done

echo ":: default settings (~/.config/deepinglassrc)"
mkdir -p "$CONF"
[[ -f "$CONF/deepinglassrc" ]] || cp kde/common/deepinglassrc.default "$CONF/deepinglassrc"

echo ":: GTK 3/4 themes"
for t in Deepin-Glass Deepin-Glass-Dark Deepin-Glass-Translucent Deepin-Glass-Translucent-Dark; do
    rm -rf "$DATA/themes/$t"
    cp -r "gtk/$t" "$DATA/themes/"
done

echo ":: icons and cursors (downloads linuxdeepin/deepin-icon-theme)"
tools/fetch-deepin-icon-theme.sh "$ROOT/build/deepin-icon-theme"
rm -rf "$ROOT/build/icon-themes"
python3 tools/build-icon-themes.py "$ROOT/build/deepin-icon-theme" "$ROOT/build/icon-themes"
for t in Deepin-Bloom Deepin-Bloom-Dark; do rm -rf "$DATA/icons/$t"; cp -a "build/icon-themes/icons/$t" "$DATA/icons/"; done
for t in Deepin-Bloom-Cursors Deepin-Bloom-Cursors-Dark; do rm -rf "$DATA/icons/$t"; cp -a "build/icon-themes/cursors/$t" "$DATA/icons/"; done

if [[ "$ARGS" == *" --libadwaita "* ]]; then
    echo ":: libadwaita stylesheet"
    mkdir -p "$CONF/gtk-4.0"
    if [[ -f "$CONF/gtk-4.0/gtk.css" ]] && ! cmp -s "$CONF/gtk-4.0/gtk.css" gtk/libadwaita/gtk.css; then
        cp "$CONF/gtk-4.0/gtk.css" "$CONF/gtk-4.0/gtk.css.bak-$(date +%Y%m%d%H%M%S)"
    fi
    cp gtk/libadwaita/gtk.css "$CONF/gtk-4.0/gtk.css"
fi

if [[ "$ARGS" == *" --sddm "* ]]; then
    echo ":: SDDM theme (sudo)"
    sudo rm -rf /usr/share/sddm/themes/plasma-deepin
    sudo cp -r sddm/plasma-deepin /usr/share/sddm/themes/
    echo "   select it in System Settings > Colours & Themes > Login Screen (SDDM)"
fi

if [[ "$ARGS" == *" --plymouth "* ]]; then
    echo ":: Plymouth boot animation (sudo)"
    sudo rm -rf /usr/share/plymouth/themes/plasma-deepin
    sudo cp -r plymouth/plasma-deepin /usr/share/plymouth/themes/
    if command -v plymouth-set-default-theme >/dev/null; then
        # -R rebuilds the initramfs so the theme is used at the next boot
        sudo plymouth-set-default-theme -R plasma-deepin
    else
        echo "   plymouth-set-default-theme not found: set Theme=plasma-deepin in /etc/plymouth/plymouthd.conf"
        echo "   and rebuild the initramfs (mkinitcpio -P or dracut --regenerate-all --force)"
    fi
fi

if [[ "$ARGS" == *" --native "* ]]; then
    echo ":: window decoration and application style (sudo for the install step)"
    cmake -B build/native -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
    cmake --build build/native
    sudo cmake --install build/native
fi

if [[ "$ARGS" == *" --gtk-glass "* ]]; then
    echo ":: glass for GTK applications"
    tools/setup-gtk-glass.sh
fi

cat <<'MSG'

Done. Next steps:
  System Settings > Colours & Themes > Global Theme > "Deepin Light" or "Deepin Dark"
  (tick "Use desktop layout from theme" for the centred dock).
  Window decoration "Deepin Glass" and application style "Deepin Glass" need the
  AUR package plasma-deepin-glass(-git) or ./install.sh --native.
  GTK: System Settings > Colours & Themes > Application Style > Configure GNOME/GTK
  Application Style > "Deepin-Glass".
MSG
