#!/bin/bash
# SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Run at login by tools/setup-gtk-glass.sh (autostart entry "Deepin Glass blur check").
#
# Better Blur DX is built for one exact KWin version. After a KWin update it fails
# to load until it is rebuilt, and as KWin's own blur effect is switched off for it,
# no window gets any blur: the glass turns into plain transparency. Then this loads
# KWin's blur effect for the session (Qt applications and the decoration get their
# blur back) and tells the user to rebuild the effect.
sleep 5
command -v qdbus6 >/dev/null || exit 0
[[ "$(kreadconfig6 --file kwinrc --group Plugins --key better_blur_dxEnabled 2>/dev/null)" == true ]] || exit 0
loaded="$(qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.loadedEffects 2>/dev/null)"
grep -qx better_blur_dx <<<"$loaded" && exit 0
# give it one more try (e.g. rebuilt while KWin was running)
qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.loadEffect better_blur_dx >/dev/null 2>&1
loaded="$(qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.loadedEffects 2>/dev/null)"
grep -qx better_blur_dx <<<"$loaded" && exit 0
qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.loadEffect blur >/dev/null 2>&1
msg="Better Blur DX passt nicht zur installierten KWin-Version und wurde nicht geladen. \
KWins eigener Blur ist für diese Sitzung aktiv (GTK-Programme ohne Weichzeichner). \
Abhilfe: Paket kwin-effects-better-blur-dx neu bauen (z. B. paru -S kwin-effects-better-blur-dx --rebuild) und neu anmelden."
if command -v notify-send >/dev/null; then
    notify-send -a "Deepin Glass" -i dialog-warning "Deepin Glass: Blur-Effekt" "$msg"
else
    echo "$msg" >&2
fi
