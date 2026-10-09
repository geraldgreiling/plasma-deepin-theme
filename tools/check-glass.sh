#!/bin/bash
# SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Checks why an application might not show the Deepin Glass look.
# Usage: tools/check-glass.sh [application, e.g. plasma-discover]
set -u

ok()   { printf '  \e[32mok\e[0m    %s\n' "$*"; }
bad()  { printf '  \e[31mfehlt\e[0m %s\n' "$*"; }
info() { printf '        %s\n' "$*"; }

PLUGINS="$(qtpaths6 --plugin-dir 2>/dev/null || qmake6 -query QT_INSTALL_PLUGINS 2>/dev/null || echo /usr/lib/qt6/plugins)"
CONF="${XDG_CONFIG_HOME:-$HOME/.config}"

echo "Sitzung"
info "XDG_SESSION_TYPE=${XDG_SESSION_TYPE:-?}  QT_QUICK_CONTROLS_STYLE=${QT_QUICK_CONTROLS_STYLE:-(nicht gesetzt)}"

echo "Installierte Plugins (in $PLUGINS)"
for f in org.kde.kdecoration3/org.plasmadeepin.glass.so styles/deepinglass6.so kf6/kirigami/platform/org.kde.desktop.deepinglass.so; do
    if [[ -f "$PLUGINS/$f" ]]; then ok "$f ($(date -r "$PLUGINS/$f" '+%F %T'))"; else bad "$f  ->  ./install.sh --native"; fi
done
info "Kirigami-Plugins: $(ls "$PLUGINS/kf6/kirigami/platform" 2>/dev/null | tr '\n' ' ')"
info "(org.kde.desktop.deepinglass.so muss vor org.kde.desktop.so stehen)"

echo "Einstellungen"
if [[ -f "$CONF/deepinglassrc" ]]; then
    ok "$CONF/deepinglassrc, ConfigVersion=$(kreadconfig6 --file deepinglassrc --group General --key ConfigVersion --default 1 2>/dev/null)"
    info "Style: $(kreadconfig6 --file kdeglobals --group KDE --key widgetStyle 2>/dev/null)   Dekoration: $(kreadconfig6 --file kwinrc --group org.kde.kdecoration2 --key library 2>/dev/null)"
    info "Blur-Effekt: $(kreadconfig6 --file kwinrc --group Plugins --key blurEnabled --default true 2>/dev/null)"
else
    bad "$CONF/deepinglassrc"
fi

APP="${1:-}"
if [[ -n "$APP" ]]; then
    echo "Laufende Instanzen von $APP"
    if pgrep -x "$APP" >/dev/null; then
        bad "$APP läuft bereits (PID $(pgrep -x "$APP" | tr '\n' ' ')) - beenden und neu starten, sonst wird nur das alte Fenster aktiviert"
    else
        ok "keine"
    fi
    echo "Welches Kirigami-Plugin lädt $APP? (startet das Programm für 10 Sekunden)"
    QT_LOGGING_RULES="kf.kirigami.platform.debug=true" DEEPINGLASS_DEBUG=1 timeout 10 "$APP" 2>&1 \
        | grep -i "Loading style plugin\|Failed to find\|DeepinGlass:" | head -n 6 | sed 's/^/        /'
    info "Erwartet: org.kde.desktop.deepinglass.so, danach 'DeepinGlass: window ... color QColor(ARGB 0.72 ...) ... alpha 8'"
fi
