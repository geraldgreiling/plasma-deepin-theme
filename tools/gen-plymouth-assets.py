#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
# SPDX-License-Identifier: GPL-3.0-or-later
"""Render the images of the Plymouth theme (plymouth/plasma-deepin).

The sizes follow the Plasma splash screen (contents/splash/Splash.qml) at a
Kirigami grid unit of 18 px: logo 6 units, dots 0.55 units.

Usage: gen-plymouth-assets.py <deepin-icon-theme checkout>
Needs: python3-pil, rsvg-convert
"""
import os
import subprocess
import sys

from PIL import Image, ImageDraw

GU = 18
OUT = os.path.join(os.path.dirname(__file__), '..', 'plymouth', 'plasma-deepin')
SS = 4  # supersampling for smooth edges


def circle(size, color, path):
    big = Image.new('RGBA', (size * SS, size * SS), (0, 0, 0, 0))
    ImageDraw.Draw(big).ellipse((0, 0, size * SS - 1, size * SS - 1), fill=color)
    big.resize((size, size), Image.LANCZOS).save(path)


def rounded(w, h, r, fill, outline, path):
    big = Image.new('RGBA', (w * SS, h * SS), (0, 0, 0, 0))
    d = ImageDraw.Draw(big)
    d.rounded_rectangle((0, 0, w * SS - 1, h * SS - 1), radius=r * SS, fill=fill, outline=outline, width=SS)
    big.resize((w, h), Image.LANCZOS).save(path)


def main():
    src = sys.argv[1]
    os.makedirs(OUT, exist_ok=True)
    logo = round(GU * 6)
    svg = os.path.join(src, 'bloom', 'places', '128', 'deepin-launcher.svg')
    subprocess.run(['rsvg-convert', '-w', str(logo), '-h', str(logo), svg, '-o', os.path.join(OUT, 'logo.png')], check=True)
    circle(round(GU * 0.55), (0x00, 0x81, 0xff, 255), os.path.join(OUT, 'dot.png'))
    # password field: frosted white like the SDDM theme, 28 x 4.4 units at 10 px per unit
    rounded(280, 44, 10, (255, 255, 255, 64), (255, 255, 255, 90), os.path.join(OUT, 'entry.png'))
    circle(10, (255, 255, 255, 255), os.path.join(OUT, 'bullet.png'))
    print('assets written to', os.path.abspath(OUT))


if __name__ == '__main__':
    main()
