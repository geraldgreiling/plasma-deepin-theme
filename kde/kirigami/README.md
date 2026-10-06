# Kirigami platform plugin "Deepin Glass"

Gives Qt Quick / Kirigami applications (Discover, System Monitor, Elisa, …) the
frosted glass look.

* `glasstheme.*` – copy of `PlasmaDesktopTheme` from
  [qqc2-desktop-style](https://invent.kde.org/frameworks/qqc2-desktop-style)
  (LGPL-2.0-or-later, commit 20471364a82a). Changes are marked with
  "Deepin Glass": background colours of the Window, View and Header colour
  sets get an alpha channel.
* `plasmadesktopunits.*`, `animationspeedprovider.*` – copied unchanged from the
  same commit (LGPL-2.0-or-later / LGPL-2.1-or-later).
* `glasscontroller.*`, `glassfactory.*` – new: decide per application whether
  glass is used, give Kirigami windows an alpha channel before they are created
  and ask KWin to blur behind them.

Settings: section `[QtQuick]` in `~/.config/deepinglassrc`.

When qqc2-desktop-style changes its plugin, the copied files should be updated.
