# plasma-deepin-theme

A global theme for **KDE Plasma 6** with the look and feel of the Deepin desktop:
frosted glass windows and dock that take on the colours of whatever is behind
them, DTK colours and radii, Deepin "bloom" icons and cursors, a matching login
screen and matching GTK 3/4 themes.

![Deepin Light](docs/screenshots/plasma-light.png)
![Deepin Dark](docs/screenshots/plasma-dark.png)

*Screenshots from a software-rendered test session: KWin composited with
QPainter there, so blur, window shadows and rounded bottom corners are not
visible. On real hardware (OpenGL compositing) the glass is blurred.*

This is an independent community project. It is not affiliated with or
endorsed by Deepin / UnionTech. "Deepin" is used only to describe the look.

## Components

| Component | Name | Distribution |
|---|---|---|
| Window decoration (C++, KDecoration3, blur, rounded corners) | **Deepin Glass** | AUR: `plasma-deepin-glass` / `plasma-deepin-glass-git` |
| Application style (C++, Qt 6, based on Breeze, translucent + blurred windows) | **Deepin Glass** | AUR (same package) |
| Glass for Qt Quick/Kirigami apps (C++, Kirigami platform plugin) | Deepin Glass | AUR (same package) |
| Colour schemes | Deepin Light, Deepin Dark | KDE Store |
| Plasma style (panel/dock, popups, tooltips; follows the colour scheme) | Deepin | KDE Store |
| Global themes (defaults, centred floating dock, splash screen) | Deepin Light, Deepin Dark | KDE Store |
| Icons | Deepin Bloom, Deepin Bloom Dark | KDE Store |
| Cursors | Deepin Bloom Cursors (+ Dark) | KDE Store |
| Login screen (SDDM, Qt 6) | Deepin (Plasma) | KDE Store |
| Boot animation (Plymouth, looks like the splash screen, with LUKS password prompt) | Deepin (Plasma) | KDE Store |
| GTK 3 / GTK 4 themes (+ stylesheet for libadwaita apps) | Deepin-Glass, Deepin-Glass-Dark, Deepin-Glass-Translucent(-Dark) | KDE Store |

## Installation

### From the AUR (decoration + application style)

```sh
paru -S plasma-deepin-glass-git     # or yay, or makepkg in packaging/aur/
```

### From the KDE Store

System Settings → Colours & Themes → *Get New…* in the respective page
(Colours, Plasma Style, Icons, Cursors, Login Screen, Global Theme).
The Plymouth theme is installed by hand: unpack it to `/usr/share/plymouth/themes/`
and run `sudo plymouth-set-default-theme -R plasma-deepin`.
Install the parts first, then the global theme: a global theme cannot pull in
its dependencies by itself.

### From this repository (current user, no root)

```sh
./install.sh                # colours, Plasma style, global themes, icons, cursors, GTK
./install.sh --native       # also build + install decoration and app style (sudo)
./install.sh --sddm         # also install the SDDM theme (sudo)
./install.sh --plymouth     # also install + activate the Plymouth boot animation (sudo, rebuilds the initramfs)
./install.sh --libadwaita   # also write ~/.config/gtk-4.0/gtk.css (a backup is kept)
./install.sh --uninstall
```

Build dependencies for `--native` on Arch/CachyOS:
`cmake extra-cmake-modules kdecoration breeze kconfig kcoreaddons kwindowsystem qt6-base`.

Then: System Settings → Global Theme → **Deepin Light** or **Deepin Dark**
(tick *Use desktop layout from theme* to get the centred dock).
GTK: System Settings → Application Style → *Configure GNOME/GTK Application Style* → *Deepin-Glass*.

## Settings of the glass effect

Decoration, application style and the Kirigami plugin read
`~/.config/deepinglassrc`. The file is created with all defaults and comments
the first time one of them runs (or by `./install.sh`); the template is
[`kde/common/deepinglassrc.default`](kde/common/deepinglassrc.default).
Sections: `[Decoration]`, `[Style]` (Qt Widgets applications) and `[QtQuick]`
(Kirigami applications).

By default every window is **one uniform glass surface** with the opacity of
the title bar: `WindowOpacity=auto` follows `[Decoration] ActiveOpacity` /
`InactiveOpacity`, while side bars, views, pages and headers add nothing
(`SidebarOpacity`, `ViewOpacity`, `PageOpacity`, `HeaderOpacity` = 0). Raise
those values to give individual areas an extra layer. Menus, dialogs and
sheets stay mostly opaque for readability.

A `deepinglassrc` from an older version (`ConfigVersion` < 2) is replaced by
the new template once; the old file is kept as `deepinglassrc.old`.

Apply decoration changes with `qdbus6 org.kde.KWin /KWin reconfigure`;
applications pick up changes on restart. Blur has to be enabled in
System Settings → Desktop Effects → *Blur*.

## How the glass reaches Qt Quick applications

Kirigami applications (Discover, System Monitor, Elisa, …) take their colours
from a Kirigami platform plugin, normally `org.kde.desktop` from
qqc2-desktop-style. This project installs `org.kde.desktop.deepinglass`, a copy
of that plugin with translucent Window/View/Header backgrounds. Kirigami loads
the first plugin whose file name contains the style name, and directory entries
are sorted by name, so this plugin is used instead of the original. It also
gives Kirigami windows an alpha channel and asks KWin to blur behind them.
Dialogs, sheets and menus inside a window stay opaque. `[QtQuick]
Translucent=false` turns it back into the unchanged original; uninstalling the
package restores the original plugin.

## Glass for GTK applications (optional)

GTK applications cannot ask KWin for blur, so the default GTK themes are opaque
and only the title bar (drawn by KWin) is glass. With the third party KWin effect
[Better Blur DX](https://github.com/xarblu/kwin-effects-better-blur-dx), which can
blur behind any window, GTK 3/4 applications get the same glass:

```sh
paru -S kwin-effects-better-blur-dx
./install.sh --gtk-glass        # or tools/setup-gtk-glass.sh
```

The script replaces KWin's blur effect with Better Blur DX (it handles the blur
requests of the decoration and the Qt applications as well), adds the GTK
applications it finds to the effect's force-blur list and selects
*Deepin-Glass-Translucent* with the opacities from `deepinglassrc`. Run it again
after installing GTK applications; `tools/setup-gtk-glass.sh --off` undoes it.
Better Blur DX is built for one exact KWin version and must be rebuilt after KWin
updates. libadwaita applications ignore GTK themes and stay opaque.

## Troubleshooting

`tools/check-glass.sh plasma-discover` lists the installed plugins, the
settings, whether the application is still running (single-instance
applications like Discover only reactivate the old window) and which Kirigami
plugin it loads. `DEEPINGLASS_DEBUG=1` makes the Kirigami plugin log its
window setup.

## Limitations

* **Glass in applications** works for Qt Widgets applications (Dolphin, Kate,
  Konsole, …), Kirigami applications (Discover, …), QML in Qt Widgets windows
  (System Settings) and QQuickView windows (Spectacle). GTK applications only
  with the optional setup above; libadwaita applications stay opaque.
* The Kirigami plugin is a copy of qqc2-desktop-style's plugin
  (`kde/kirigami/README.md`); new upstream features need to be merged by hand.
  GTK applications cannot request blur from KWin, so the GTK themes are opaque.
* Applications that render with OpenGL/video overlays are kept opaque by an
  exclusion list (see `kde/style/deepinglassstyle.cpp`, extendable via
  `ExcludedApplications`).
* **Lock screen:** since Plasma 6 the lock screen is part of the Plasma shell
  package, not of the global theme, so it cannot be replaced here. It uses the
  Plasma style and colour scheme of this theme.
* **Cursors** are shipped as XCursors (sizes 24–256 px). The SVG source in
  deepin-icon-theme is older than the shipped XCursors, so no SVG cursors
  are generated.
* **libadwaita** ignores GTK themes; `gtk/libadwaita/gtk.css` adapts colours
  and radii through the user stylesheet only.

## Repository layout

```
kde/decoration/     KDecoration3 plugin "Deepin Glass"
kde/style/          Qt 6 style plugin "DeepinGlass" (QProxyStyle on top of Breeze)
kde/kirigami/       Kirigami platform plugin (glass for Qt Quick applications)
kde/common/         shared settings (deepinglassrc + default template) and shadow renderer
color-schemes/      Deepin Light / Dark            (generated: tools/gen-colors.py)
plasma/desktoptheme Plasma style "plasma-deepin"   (generated: tools/gen-plasma-style.py)
plasma/look-and-feel global themes, layout script, splash screen
sddm/plasma-deepin  SDDM theme (Qt 6)
plymouth/           Plymouth boot animation (images: tools/gen-plymouth-assets.py)
gtk/                GTK 3/4 themes + libadwaita sheet (generated: tools/gen-gtk-themes.py)
tools/              generators, icon/cursor build, KDE Store packaging
packaging/aur/      PKGBUILDs
tests/              widget gallery and QML screenshot helper
```

`tools/make-store-packages.sh` builds all KDE Store archives into `dist/`.

## Sources of the design values

Colours, radii and opacities come from the DTK source code
([dtkgui](https://github.com/linuxdeepin/dtkgui) `dguiapplicationhelper.cpp`,
[dtkwidget](https://github.com/linuxdeepin/dtkwidget) `dstyle.cpp`,
`dblureffectwidget.cpp`). Values that are not from DTK are marked "own" in
`tools/gen-colors.py`.

## Licence

GPL-3.0-or-later (see `LICENSE`).

* Icons and cursors are built from
  [linuxdeepin/deepin-icon-theme](https://github.com/linuxdeepin/deepin-icon-theme)
  (GPL-3.0-or-later, © UnionTech Software Technology Co., Ltd.). They are not
  stored in this repository but fetched at a pinned commit.
* The GTK themes are generated from the stylesheets built into GTK
  (LGPL-2.1-or-later, see `LICENSES/`).
* `kde/kirigami` contains code from qqc2-desktop-style (LGPL-2.0-or-later /
  LGPL-2.1-or-later).
* The structure of the window decoration follows the Breeze decoration,
  the translucency technique of the style follows Kvantum (both GPL).
