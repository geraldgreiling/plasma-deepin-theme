#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
# SPDX-License-Identifier: GPL-3.0-or-later
"""Generate the Deepin Glass GTK 3 and GTK 4 themes.

Base: the stylesheets compiled into GTK itself (GTK 3 "Adwaita", GTK 4
"Default", LGPL-2.1-or-later). They are extracted from the installed libgtk
with `gresource`, recoloured to the DTK palette and extended with Deepin
shapes (8 px controls, filled buttons and entries, flat title buttons with a
red close button, 12 px popovers and window corners).

Additionally a stylesheet for libadwaita applications is written
(gtk/libadwaita/gtk.css), to be placed in ~/.config/gtk-4.0/gtk.css.

Usage: gen-gtk-themes.py [out dir]   (needs gresource, libgtk-3 and libgtk-4)
"""
import colorsys
import os
import re
import subprocess
import sys

GTK3_LIB = 'libgtk-3.so.0'
GTK4_LIB = 'libgtk-4.so.1'
LIB_DIRS = ['/usr/lib', '/usr/lib64', '/usr/lib/x86_64-linux-gnu']

# DTK palette (linuxdeepin/dtkgui, dguiapplicationhelper.cpp), see tools/gen-colors.py
PALETTE = {
    'light': dict(bg='#f8f8f8', fg='#252525', base='#ffffff', button='#e5e5e5', header='#f8f8f8',
                  accent='#0081ff', accent_fg='#ffffff', warning='#ff5736', dim='#636363'),
    'dark': dict(bg='#252525', fg='#dedede', base='#282828', button='#444444', header='#252525',
                 accent='#0059d2', accent_fg='#f1f6ff', warning='#e43f2e', dim='#a8a8a8'),
}

# exact replacements of the Adwaita base colours
EXACT = {
    'light': {'#f6f5f4': '#f8f8f8', '#2e3436': '#252525', '#3584e4': '#0081ff'},
    'dark': {'#353535': '#252525', '#2d2d2d': '#282828', '#eeeeec': '#dedede', '#15539e': '#0059d2', '#3584e4': '#0081ff'},
}
# anchors for the generic blue -> DTK accent mapping
ANCHOR = {'light': ('#3584e4', '#0081ff'), 'dark': ('#15539e', '#0059d2')}


def find_lib(name):
    for d in LIB_DIRS:
        p = os.path.join(d, name)
        if os.path.exists(p):
            return p
    raise SystemExit(f'{name} not found')


def extract(lib, path):
    return subprocess.run(['gresource', 'extract', find_lib(lib), path], capture_output=True, text=True, check=True).stdout


def hx(c):
    c = c.lstrip('#')
    if len(c) == 3:
        c = ''.join(ch * 2 for ch in c)
    return tuple(int(c[i:i + 2], 16) / 255 for i in (0, 2, 4))


def to_hex(rgb):
    return '#%02x%02x%02x' % tuple(max(0, min(255, round(v * 255))) for v in rgb)


def map_rgb(rgb, variant):
    exact = EXACT[variant].get(to_hex(rgb))
    if exact:
        return hx(exact)
    h, l, s = colorsys.rgb_to_hls(*rgb)
    deg = h * 360
    if s >= 0.35 and 195 <= deg <= 235 and 0.08 < l < 0.95:
        src, dst = ANCHOR[variant]
        _, ls, _ = colorsys.rgb_to_hls(*hx(src))
        hd, ld, sd = colorsys.rgb_to_hls(*hx(dst))
        return colorsys.hls_to_rgb(hd, min(0.97, l * ld / ls), sd)
    if s >= 0.5 and (deg >= 345 or deg <= 12) and 0.2 < l < 0.8:
        hd, _, _ = colorsys.rgb_to_hls(*hx(PALETTE[variant]['warning']))
        return colorsys.hls_to_rgb(hd, l, s)
    if s < 0.25:
        # Adwaita neutrals are slightly warm, DTK greys are neutral
        y = 0.2126 * rgb[0] + 0.7152 * rgb[1] + 0.0722 * rgb[2]
        return (y, y, y)
    return rgb


def recolor(css, variant):
    def hex_sub(m):
        return to_hex(map_rgb(hx(m.group(0)), variant))

    def rgba_sub(m):
        parts = [p.strip() for p in m.group(2).split(',')]
        if len(parts) < 3:
            return m.group(0)
        try:
            rgb = tuple(float(p) / 255 for p in parts[:3])
        except ValueError:
            return m.group(0)
        r, g, b = (round(v * 255) for v in map_rgb(rgb, variant))
        rest = ', '.join(parts[3:])
        return f'{m.group(1)}({r}, {g}, {b}{", " + rest if rest else ""})'

    css = re.sub(r'#[0-9a-fA-F]{6}\b|#[0-9a-fA-F]{3}\b', hex_sub, css)
    css = re.sub(r'\b(rgba?)\(([^()]*)\)', rgba_sub, css)
    return css


def defines(variant):
    p = PALETTE[variant]
    dark = variant == 'dark'
    hover = 'rgba(255, 255, 255, 0.08)' if dark else 'rgba(0, 0, 0, 0.05)'
    active = 'rgba(255, 255, 255, 0.14)' if dark else 'rgba(0, 0, 0, 0.10)'
    border = 'rgba(255, 255, 255, 0.10)' if dark else 'rgba(0, 0, 0, 0.06)'
    entry = 'rgba(255, 255, 255, 0.06)' if dark else 'rgba(0, 0, 0, 0.05)'
    return f"""
/* ---------------------------------------------------------------- Deepin Glass */
@define-color deepin_bg {p['bg']};
@define-color deepin_fg {p['fg']};
@define-color deepin_base {p['base']};
@define-color deepin_button {p['button']};
@define-color deepin_header {p['header']};
@define-color deepin_accent {p['accent']};
@define-color deepin_accent_fg {p['accent_fg']};
@define-color deepin_warning {p['warning']};
@define-color deepin_dim {p['dim']};
@define-color deepin_hover {hover};
@define-color deepin_active {active};
@define-color deepin_border {border};
@define-color deepin_entry {entry};
@define-color accent_color {p['accent']};
@define-color accent_bg_color {p['accent']};
@define-color accent_fg_color {p['accent_fg']};
@define-color theme_selected_bg_color {p['accent']};
@define-color theme_selected_fg_color {p['accent_fg']};
"""


COMMON_RULES = """
/* buttons: filled, 8 px radius, no outline (DTK) */
button { border-radius: 8px; border: none; box-shadow: none; text-shadow: none; -gtk-icon-shadow: none;
         background-image: none; background-color: @deepin_button; color: @deepin_fg; }
button:hover { background-image: image(@deepin_hover); }
button:active, button:checked { background-image: image(@deepin_active); }
button:disabled { opacity: 0.55; }
button.flat, button.image-button.flat, headerbar button, .titlebar button, popover button.flat, modelbutton {
    background-color: transparent; }
button.flat:hover, headerbar button:hover, .titlebar button:hover { background-image: image(@deepin_hover); }
button.flat:active, button.flat:checked, headerbar button:active, headerbar button:checked { background-image: image(@deepin_active); }
button.suggested-action, button.default { background-color: @deepin_accent; color: @deepin_accent_fg; }
button.suggested-action:hover { background-image: image(rgba(255, 255, 255, 0.10)); }
button.destructive-action { background-color: @deepin_warning; color: #ffffff; }

/* entries: filled, focus ring in the accent colour */
entry, spinbutton:not(.vertical), spinbutton.vertical > text {
    border-radius: 8px; border-color: @deepin_border; box-shadow: none; background-image: none;
    background-color: @deepin_entry; }
entry:focus-within, entry:focus, spinbutton:focus-within {
    border-color: @deepin_accent; box-shadow: inset 0 0 0 1px @deepin_accent; }

/* header bars: same colour as the window, no separator, centred title */
headerbar, .titlebar:not(headerbar) {
    background-image: none; background-color: @deepin_header; box-shadow: none; border-bottom: 1px solid @deepin_border;
    min-height: 46px; }
headerbar:backdrop { background-color: @deepin_bg; }
headerbar .title { font-weight: normal; }

/* popovers and menus */
popover > contents, popover.background > contents, menu, .menu, .context-menu {
    border-radius: 12px; }
popover modelbutton, menu menuitem { border-radius: 6px; }
popover modelbutton:hover, menu menuitem:hover { background-color: @deepin_accent; color: @deepin_accent_fg; }

/* selection in lists: rounded accent rows */
list > row:selected, listview > row:selected, row:selected {
    background-color: @deepin_accent; color: @deepin_accent_fg; }

/* check boxes and radio buttons */
check, radio { border-radius: 4px; border: 1px solid alpha(@deepin_fg, 0.45); background-image: none; background-color: @deepin_base; box-shadow: none; }
radio { border-radius: 100%; }
check:checked, check:indeterminate, radio:checked { background-color: @deepin_accent; border-color: @deepin_accent; color: @deepin_accent_fg; }

/* switches */
switch { border-radius: 14px; border: none; background-color: alpha(@deepin_fg, 0.18); background-image: none; }
switch:checked { background-color: @deepin_accent; }
switch slider { border-radius: 100%; border: none; background-color: #ffffff; background-image: none;
                box-shadow: 0 1px 2px rgba(0, 0, 0, 0.25); }

/* progress bars and sliders */
progressbar trough, scale trough { border-radius: 3px; border: none; background-color: alpha(@deepin_fg, 0.12); background-image: none; }
progressbar progress, scale highlight { border-radius: 3px; border: none; background-color: @deepin_accent; background-image: none; }
scale slider { border-radius: 100%; }

/* scroll bars: thin, no trough */
scrollbar { background-color: transparent; border: none; }
scrollbar slider { border-radius: 8px; background-color: alpha(@deepin_fg, 0.35); border: 2px solid transparent; min-width: 6px; min-height: 6px; }
scrollbar slider:hover { background-color: alpha(@deepin_fg, 0.5); }
scrollbar slider:active { background-color: alpha(@deepin_fg, 0.65); }

/* tooltips */
tooltip, tooltip.background { border-radius: 8px; }
"""

GTK3_RULES = """
/* window corners for client side decorations */
window.csd, window.csd decoration, decoration { border-radius: 12px 12px 12px 12px; }
.maximized decoration, .tiled decoration, .fullscreen decoration { border-radius: 0; }
headerbar, .titlebar { border-radius: 12px 12px 0 0; }
.maximized headerbar, .tiled headerbar, .maximized .titlebar { border-radius: 0; }

/* title buttons: flat, rounded hover, red close button */
headerbar button.titlebutton, .titlebar button.titlebutton {
    min-width: 30px; min-height: 30px; padding: 0; margin: 0 1px; border-radius: 8px;
    background-color: transparent; background-image: none; }
headerbar button.titlebutton:hover { background-image: image(@deepin_hover); }
headerbar button.titlebutton.close:hover, .titlebar button.titlebutton.close:hover {
    background-image: image(@deepin_warning); color: #ffffff; }
"""

GTK4_RULES = """
/* window corners for client side decorations */
window.csd, window.csd > .titlebar, window.csd > headerbar { border-radius: 12px 12px 12px 12px; }
window.csd > .titlebar, window.csd > headerbar, window.csd headerbar.titlebar { border-radius: 12px 12px 0 0; }
window.maximized, window.tiled, window.fullscreen, window.maximized headerbar, window.tiled headerbar { border-radius: 0; }

/* title buttons: flat, rounded hover, red close button */
windowcontrols > button { min-width: 30px; min-height: 30px; padding: 0; margin: 0 1px; border-radius: 8px;
                          background-color: transparent; background-image: none; box-shadow: none; }
windowcontrols > button > image { background-color: transparent; box-shadow: none; padding: 4px; }
windowcontrols > button:hover { background-image: image(@deepin_hover); }
windowcontrols > button.close:hover { background-image: image(@deepin_warning); color: #ffffff; }
"""


# Translucent variants ("Deepin-Glass-Translucent"): one glass surface per window like
# the Qt applications. GTK cannot ask KWin for blur, so these are meant to be used
# together with a force-blur KWin effect (tools/setup-gtk-glass.sh). The opacities
# are the defaults of [Decoration] in deepinglassrc; setup-gtk-glass.sh replaces
# the marked values with the user's settings.
GLASS_RULES = """
/* ------------------------------------------------ Deepin Glass: translucent window */
window.background, dialog.background { background-color: alpha(@deepin_bg, 0.72 /*deepinglass:active*/); }
window.background:backdrop, dialog.background:backdrop { background-color: alpha(@deepin_bg, 0.58 /*deepinglass:inactive*/); }

/* everything lying on the window surface adds no layer of its own */
headerbar, headerbar:backdrop, .titlebar:not(headerbar), .titlebar:backdrop, toolbar, .toolbar, actionbar > revealer > box,
searchbar > revealer > box, .inline-toolbar, statusbar, infobar:not(.info):not(.warning):not(.error):not(.question),
.sidebar, stacksidebar, placessidebar, .navigation-sidebar, sidebar, paned, scrolledwindow, viewport,
.view, view, iconview, treeview.view, textview > text, list, listview, columnview, gridview,
notebook > stack, notebook > stack:not(:only-child), notebook > header, stack, frame > border, .frame, flowbox, flowboxchild {
    background-color: transparent; background-image: none; }
headerbar, .titlebar:not(headerbar) { box-shadow: none; }

/* rows and cells: only hover and selection are painted */
list > row, listview > row, columnview > listview > row, gridview > child, flowboxchild { background-color: transparent; }
"""

# GTK 3 draws the title bar outside the window background node
GLASS_RULES_GTK3 = """
headerbar.titlebar, .titlebar:not(headerbar) { background-color: alpha(@deepin_bg, 0.72 /*deepinglass:active*/); }
headerbar.titlebar:backdrop, .titlebar:not(headerbar):backdrop { background-color: alpha(@deepin_bg, 0.58 /*deepinglass:inactive*/); }
"""


def header(kind, upstream):
    return f"""/*
 * Deepin Glass - {kind}
 * SPDX-FileCopyrightText: GTK contributors (base stylesheet "{upstream}", LGPL-2.1-or-later)
 * SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
 * SPDX-License-Identifier: LGPL-2.1-or-later AND GPL-3.0-or-later
 *
 * Generated by tools/gen-gtk-themes.py - edit the generator, not this file.
 */
"""


def gtk_theme(lib, base_path, variant, asset_prefix, rules, kind, upstream, glass=False):
    css = extract(lib, base_path)
    css = css.replace('url("assets/', f'url("resource://{asset_prefix}/assets/')
    css = recolor(css, variant)
    glass_rules = (GLASS_RULES + (GLASS_RULES_GTK3 if rules is GTK3_RULES else '')) if glass else ''
    return header(kind, upstream) + defines(variant) + css + COMMON_RULES + rules + glass_rules


LIBADWAITA = """/*
 * Deepin Glass for libadwaita applications.
 * Copy to ~/.config/gtk-4.0/gtk.css (./install.sh --libadwaita does that).
 * libadwaita ignores GTK themes but loads this user stylesheet.
 * SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
:root {
  --accent-bg-color: #0081ff;
  --accent-fg-color: #ffffff;
  --accent-color: #0081ff;
  --window-bg-color: #f8f8f8;
  --window-fg-color: #252525;
  --view-bg-color: #ffffff;
  --view-fg-color: #252525;
  --headerbar-bg-color: #f8f8f8;
  --headerbar-fg-color: #252525;
  --headerbar-backdrop-color: #f8f8f8;
  --sidebar-bg-color: #f8f8f8;
  --secondary-sidebar-bg-color: #f5f5f5;
  --popover-bg-color: #ffffff;
  --dialog-bg-color: #f8f8f8;
  --card-bg-color: #ffffff;
  --destructive-bg-color: #ff5736;
  --error-bg-color: #ff5736;
  --window-radius: 12px;
}
@media (prefers-color-scheme: dark) {
  :root {
    --accent-bg-color: #0059d2;
    --accent-fg-color: #f1f6ff;
    --accent-color: #3d8bff;
    --window-bg-color: #252525;
    --window-fg-color: #dedede;
    --view-bg-color: #282828;
    --view-fg-color: #dedede;
    --headerbar-bg-color: #252525;
    --headerbar-fg-color: #dedede;
    --headerbar-backdrop-color: #252525;
    --sidebar-bg-color: #252525;
    --secondary-sidebar-bg-color: #222222;
    --popover-bg-color: #2d2d2d;
    --dialog-bg-color: #252525;
    --card-bg-color: #2c2c2c;
    --destructive-bg-color: #e43f2e;
    --error-bg-color: #e43f2e;
  }
}
/* older libadwaita versions (< 1.6) use named colours */
@define-color accent_bg_color #0081ff;
@define-color accent_color #0081ff;
@define-color accent_fg_color #ffffff;

button, entry, spinbutton, dropdown > button, combobox > box > button { border-radius: 8px; }
popover > contents { border-radius: 12px; }
windowcontrols > button.close:hover > image { background-color: #ff5736; color: #ffffff; }
"""


# Glass variant of the libadwaita stylesheet (tools/setup-gtk-glass.sh installs it,
# together with a force-blur KWin effect). Only the window surface is painted, with
# the title bar's opacity; header bars, sidebars and views add no layer, floating
# parts (popovers, dialogs, menus) stay opaque. The marked values are replaced with
# the opacities from deepinglassrc.
LIBADWAITA_GLASS = """
/* ------------------------------------------------ Deepin Glass: translucent window */
:root {
  --window-bg-color: rgba(248, 248, 248, 0.72 /*deepinglass:active*/);
  --view-bg-color: transparent;
  --headerbar-bg-color: transparent;
  --headerbar-backdrop-color: transparent;
  --headerbar-shade-color: rgba(0, 0, 0, 0.06);
  --sidebar-bg-color: transparent;
  --sidebar-backdrop-color: transparent;
  --secondary-sidebar-bg-color: transparent;
  --secondary-sidebar-backdrop-color: transparent;
  --card-bg-color: rgba(255, 255, 255, 0.45);
  --thumbnail-bg-color: rgba(255, 255, 255, 0.45);
}
@media (prefers-color-scheme: dark) {
  :root {
    --window-bg-color: rgba(37, 37, 37, 0.72 /*deepinglass:active*/);
    --headerbar-shade-color: rgba(0, 0, 0, 0.25);
    --card-bg-color: rgba(255, 255, 255, 0.06);
    --thumbnail-bg-color: rgba(255, 255, 255, 0.06);
  }
}
window.background:backdrop { background-color: rgba(248, 248, 248, 0.58 /*deepinglass:inactive*/); }
@media (prefers-color-scheme: dark) {
  window.background:backdrop { background-color: rgba(37, 37, 37, 0.58 /*deepinglass:inactive*/); }
}
/* older libadwaita versions (< 1.6) use named colours */
@define-color window_bg_color alpha(#f8f8f8, 0.72 /*deepinglass:active*/);
@define-color view_bg_color transparent;
@define-color headerbar_bg_color transparent;
@define-color headerbar_backdrop_color transparent;
@define-color sidebar_bg_color transparent;
@define-color secondary_sidebar_bg_color transparent;
@define-color card_bg_color alpha(#ffffff, 0.45);
"""


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), '..', 'gtk')
    themes = {
        'Deepin-Glass': ('light', False),
        'Deepin-Glass-Dark': ('dark', False),
        'Deepin-Glass-Translucent': ('light', True),
        'Deepin-Glass-Translucent-Dark': ('dark', True),
    }
    for name, (default_variant, glass) in themes.items():
        root = os.path.join(out, name)
        for d in ('gtk-3.0', 'gtk-4.0'):
            os.makedirs(os.path.join(root, d), exist_ok=True)
        for variant, suffix in ((default_variant, ''), ('dark', '-dark')):
            if suffix and default_variant == 'dark':
                continue
            g3 = gtk_theme(GTK3_LIB, f'/org/gtk/libgtk/theme/Adwaita/gtk-contained{"-dark" if variant == "dark" else ""}.css',
                           variant, '/org/gtk/libgtk/theme/Adwaita', GTK3_RULES, f'GTK 3 ({variant})', 'Adwaita', glass)
            g4 = gtk_theme(GTK4_LIB, f'/org/gtk/libgtk/theme/Default/Default-{variant}.css',
                           variant, '/org/gtk/libgtk/theme/Default', GTK4_RULES, f'GTK 4 ({variant})', 'Default', glass)
            with open(os.path.join(root, 'gtk-3.0', f'gtk{suffix}.css'), 'w') as fh:
                fh.write(g3)
            with open(os.path.join(root, 'gtk-4.0', f'gtk{suffix}.css'), 'w') as fh:
                fh.write(g4)
        with open(os.path.join(root, 'index.theme'), 'w') as fh:
            fh.write(f"""[Desktop Entry]
Type=X-GNOME-Metatheme
Name={name}
Comment=GTK theme in the style of the Deepin desktop, matching the Deepin Glass Plasma theme{' (translucent, needs a force-blur KWin effect)' if glass else ''}
Encoding=UTF-8

[X-GNOME-Metatheme]
GtkTheme={name}
IconTheme={'Deepin-Bloom-Dark' if default_variant == 'dark' else 'Deepin-Bloom'}
CursorTheme=Deepin-Bloom-Cursors
ButtonLayout=:minimize,maximize,close
""")
    os.makedirs(os.path.join(out, 'libadwaita'), exist_ok=True)
    with open(os.path.join(out, 'libadwaita', 'gtk.css'), 'w') as fh:
        fh.write(LIBADWAITA)
    with open(os.path.join(out, 'libadwaita', 'gtk-glass.css'), 'w') as fh:
        fh.write(LIBADWAITA.replace('Copy to ~/.config/gtk-4.0/gtk.css (./install.sh --libadwaita does that).',
                                    'Glass variant, installed by tools/setup-gtk-glass.sh as ~/.config/gtk-4.0/gtk.css.')
                 + LIBADWAITA_GLASS)
    print('GTK themes written to', out)


if __name__ == '__main__':
    main()
