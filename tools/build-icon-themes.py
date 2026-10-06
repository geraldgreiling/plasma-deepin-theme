#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build the Deepin Bloom icon and cursor themes for Plasma 6.

Source: linuxdeepin/deepin-icon-theme (GPL-3.0-or-later), checked out by
tools/fetch-deepin-icon-theme.sh. The icons are SVG (no DCI conversion is
needed for the "bloom" theme).

Output (in <out>):
  icons/Deepin-Bloom              light icon theme, inherits Breeze
  icons/Deepin-Bloom-Dark         dark variant, inherits Deepin-Bloom and Breeze Dark
  cursors/Deepin-Bloom-Cursors       XCursor theme
  cursors/Deepin-Bloom-Cursors-Dark

Cursors: deepin ships prebuilt XCursors with eight sizes (24 to 256 px), which
also covers fractional scaling. No SVG cursors (cursors_scalable) are generated:
the SVG source in cursors-src/ is older than the shipped XCursors (different
shapes and hotspots), so SVG cursors built from it would not match.

Requirements: python3.
Usage: build-icon-themes.py <deepin-icon-theme checkout> <out dir>
"""
import os
import re
import shutil
import sys

# KDE / freedesktop icon names that Deepin draws under a different name.
# alias -> (category, deepin icon)
KDE_ALIASES = {
    'start-here': ('places', 'deepin-launcher'),
    'start-here-kde': ('places', 'deepin-launcher'),
    'start-here-kde-plasma': ('places', 'deepin-launcher'),
    'start-here-symbolic': ('places', 'deepin-launcher'),
    'org.kde.dolphin': ('apps', 'dde-file-manager'),
    'system-file-manager': ('apps', 'dde-file-manager'),
    'org.kde.konsole': ('apps', 'deepin-terminal'),
    'konsole': ('apps', 'deepin-terminal'),
    'utilities-terminal': ('apps', 'deepin-terminal'),
    'systemsettings': ('apps', 'preferences-system'),
    'org.kde.systemsettings': ('apps', 'preferences-system'),
    'plasmadiscover': ('apps', 'deepin-app-store'),
    'org.kde.discover': ('apps', 'deepin-app-store'),
    'org.kde.kate': ('apps', 'deepin-editor'),
    'kate': ('apps', 'deepin-editor'),
    'org.kde.kwrite': ('apps', 'deepin-editor'),
    'kwrite': ('apps', 'deepin-editor'),
    'accessories-text-editor': ('apps', 'deepin-editor'),
    'org.kde.gwenview': ('apps', 'deepin-image-viewer'),
    'gwenview': ('apps', 'deepin-image-viewer'),
    'org.kde.okular': ('apps', 'deepin-reader'),
    'okular': ('apps', 'deepin-reader'),
    'org.kde.spectacle': ('apps', 'deepin-screen-recorder'),
    'spectacle': ('apps', 'deepin-screen-recorder'),
    'org.kde.ark': ('apps', 'deepin-compressor'),
    'ark': ('apps', 'deepin-compressor'),
    'org.kde.kcalc': ('apps', 'deepin-calculator'),
    'kcalc': ('apps', 'deepin-calculator'),
    'accessories-calculator': ('apps', 'deepin-calculator'),
    'org.kde.plasma-systemmonitor': ('apps', 'deepin-system-monitor'),
    'plasma-systemmonitor': ('apps', 'deepin-system-monitor'),
    'org.kde.ksysguard': ('apps', 'deepin-system-monitor'),
    'org.kde.elisa': ('apps', 'deepin-music'),
    'elisa': ('apps', 'deepin-music'),
    'org.kde.haruna': ('apps', 'deepin-movie'),
    'haruna': ('apps', 'deepin-movie'),
    'org.kde.dragonplayer': ('apps', 'deepin-movie'),
    'dragonplayer': ('apps', 'deepin-movie'),
    'org.kde.kamoso': ('apps', 'deepin-camera'),
    'kamoso': ('apps', 'deepin-camera'),
    'org.kde.merkuro.calendar': ('apps', 'dde-calendar'),
    'org.kde.korganizer': ('apps', 'dde-calendar'),
    'korganizer': ('apps', 'dde-calendar'),
    'org.kde.kmail2': ('apps', 'deepin-mail'),
    'kmail': ('apps', 'deepin-mail'),
    'org.kde.partitionmanager': ('apps', 'deepin-diskmanager'),
    'partitionmanager': ('apps', 'deepin-diskmanager'),
    'org.kde.kinfocenter': ('apps', 'deepin-devicemanager'),
    'hwinfo': ('apps', 'deepin-devicemanager'),
    'org.kde.kfontview': ('apps', 'deepin-font-manager'),
    'kfontview': ('apps', 'deepin-font-manager'),
    'org.kde.khelpcenter': ('apps', 'deepin-manual'),
    'help-browser': ('apps', 'deepin-manual'),
    'org.kde.kcolorchooser': ('apps', 'deepin-picker'),
    'kcolorchooser': ('apps', 'deepin-picker'),
    'org.kde.kolourpaint': ('apps', 'deepin-draw'),
    'kolourpaint': ('apps', 'deepin-draw'),
    'org.kde.kdeconnect.app': ('apps', 'deepin-phone-assistant'),
    'kdeconnect': ('apps', 'deepin-phone-assistant'),
    'org.kde.krdc': ('apps', 'deepin-remote-assistance'),
    'krdc': ('apps', 'deepin-remote-assistance'),
    'org.kde.skanpage': ('apps', 'deepin-scanner'),
    'skanpage': ('apps', 'deepin-scanner'),
    'org.kde.isoimagewriter': ('apps', 'deepin-boot-maker'),
    'org.kde.kweather': ('apps', 'deepin-home'),
}


def write_index(path, name, comment, inherits):
    """Rewrite index.theme: new name and inheritance, keep the directory list."""
    with open(path) as fh:
        text = fh.read()
    head, _, rest = text.partition('\n[')  # first section is [Icon Theme]
    lines = []
    for line in head.splitlines():
        if line.startswith(('Name=', 'Name[', 'Comment', 'Inherits=')):
            continue
        lines.append(line)
    out = [lines[0], f'Name={name}', f'Comment={comment}', f'Inherits={inherits}'] + lines[1:]
    with open(path, 'w') as fh:
        fh.write('\n'.join(out) + '\n\n[' + rest)


CONTEXTS = {'actions': 'Actions', 'apps': 'Applications', 'devices': 'Devices', 'emblems': 'Emblems',
            'mimetypes': 'MimeTypes', 'places': 'Places', 'status': 'Status', 'categories': 'Categories'}


def declare_missing_directories(theme_dir):
    """Upstream ships some size directories (e.g. status/24) that index.theme does not list."""
    path = os.path.join(theme_dir, 'index.theme')
    with open(path) as fh:
        text = fh.read()
    declared = set(re.search(r'^Directories=(.*)$', text, re.M).group(1).split(','))
    missing = []
    for cat in sorted(os.listdir(theme_dir)):
        if cat not in CONTEXTS or not os.path.isdir(os.path.join(theme_dir, cat)):
            continue
        for size in sorted(os.listdir(os.path.join(theme_dir, cat)), key=lambda v: int(v) if v.isdigit() else 0):
            if size.isdigit() and f'{cat}/{size}' not in declared:
                missing.append((cat, size))
    if not missing:
        return 0
    text = re.sub(r'^(Directories=.*)$', lambda m: m.group(1) + ''.join(f',{c}/{s}' for c, s in missing), text, count=1, flags=re.M)
    text = text.rstrip('\n') + '\n'
    for cat, size in missing:
        text += f'\n[{cat}/{size}]\nSize={size}\nContext={CONTEXTS[cat]}\nType=Fixed\n'
    with open(path, 'w') as fh:
        fh.write(text)
    return len(missing)


def copy_theme(src, dst):
    if os.path.exists(dst):
        shutil.rmtree(dst)
    shutil.copytree(src, dst, symlinks=True, ignore=shutil.ignore_patterns('cursors', 'cursor.theme', 'icon-theme.cache'))


def add_aliases(theme_dir):
    created = 0
    for alias, (category, target) in KDE_ALIASES.items():
        cat_dir = os.path.join(theme_dir, category)
        if not os.path.isdir(cat_dir):
            continue
        for size in os.listdir(cat_dir):
            d = os.path.join(cat_dir, size)
            src = os.path.join(d, target + '.svg')
            dst = os.path.join(d, alias + '.svg')
            if os.path.exists(src) and not os.path.lexists(dst):
                os.symlink(target + '.svg', dst)
                created += 1
    return created


def build_icons(src, out):
    icons = os.path.join(out, 'icons')
    os.makedirs(icons, exist_ok=True)
    light = os.path.join(icons, 'Deepin-Bloom')
    dark = os.path.join(icons, 'Deepin-Bloom-Dark')
    copy_theme(os.path.join(src, 'bloom'), light)
    write_index(os.path.join(light, 'index.theme'), 'Deepin Bloom',
                'Deepin bloom icons for Plasma (from linuxdeepin/deepin-icon-theme)', 'breeze,hicolor')
    n = add_aliases(light)
    copy_theme(os.path.join(src, 'bloom-dark'), dark)
    write_index(os.path.join(dark, 'index.theme'), 'Deepin Bloom Dark',
                'Deepin bloom icons, dark variant (from linuxdeepin/deepin-icon-theme)', 'Deepin-Bloom,breeze-dark,hicolor')
    for theme in (light, dark):
        declare_missing_directories(theme)
        shutil.copy(os.path.join(src, 'LICENSES', 'GPL-3.0-or-later.txt'), os.path.join(theme, 'LICENSE'))
    print(f'icons: {light} (+{n} KDE aliases), {dark}')


# --------------------------------------------------------------------------- cursors

def build_cursors(src, out):
    cursors = os.path.join(out, 'cursors')
    os.makedirs(cursors, exist_ok=True)
    for variant, name, title in (('bloom', 'Deepin-Bloom-Cursors', 'Deepin Bloom Cursors'),
                                 ('bloom-dark', 'Deepin-Bloom-Cursors-Dark', 'Deepin Bloom Cursors Dark')):
        dst = os.path.join(cursors, name)
        if os.path.exists(dst):
            shutil.rmtree(dst)
        os.makedirs(dst)
        # prebuilt XCursors shipped by deepin (built from cursors-src with xcursorgen)
        shutil.copytree(os.path.join(src, variant, 'cursors'), os.path.join(dst, 'cursors'), symlinks=True)
        with open(os.path.join(dst, 'index.theme'), 'w') as fh:
            fh.write(f'[Icon Theme]\nName={title}\nComment=Deepin bloom cursors (from linuxdeepin/deepin-icon-theme)\nInherits=breeze_cursors\n')
        shutil.copy(os.path.join(src, 'LICENSES', 'GPL-3.0-or-later.txt'), os.path.join(dst, 'LICENSE'))
        print(f'cursors: {dst}')


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)
    src, out = sys.argv[1], sys.argv[2]
    build_icons(src, out)
    build_cursors(src, out)


if __name__ == '__main__':
    main()
