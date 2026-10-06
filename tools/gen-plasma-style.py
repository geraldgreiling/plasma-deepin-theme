#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
# SPDX-License-Identifier: GPL-3.0-or-later
"""Generate the SVGs of the "Deepin" Plasma style.

Every frame is a nine-slice frame as expected by Plasma::FrameSvg:
  <prefix>topleft, top, topright, left, center, right, bottomleft, bottom, bottomright
  <prefix>mask-*      same geometry, used by KWin for the blur region
  <prefix>shadow-*    soft shadow drawn outside the frame
  <prefix>hint-*-margin   content margins

Colours use the ColorScheme-* classes so the style follows the active
colour scheme (Deepin Light / Deepin Dark).

Variants (lookup order is done by Plasma):
  translucent/  used when KWin compositing + blur are active (frosted glass)
  opaque/       used without compositing
  (base)        fallback
"""
import os
import sys

STYLE = """<style type="text/css" id="current-color-scheme">
  .ColorScheme-Text { color:#232629; }
  .ColorScheme-Background { color:#eff0f1; }
  .ColorScheme-Highlight { color:#0081ff; }
  .ColorScheme-NegativeText { color:#ff5736; }
  .ColorScheme-NeutralText { color:#ff9b00; }
</style>"""

C = 32  # size of the stretchable centre tiles


def f(v):
    s = ('%.4f' % v).rstrip('0').rstrip('.')
    return s if s not in ('-0', '') else '0'


class Frame:
    """Builds the SVG elements of one nine-slice frame."""

    def __init__(self, prefix, radius, opacity, border_opacity=0.10, shadow=0, shadow_opacity=0.0,
                 margin=None, fill_class='ColorScheme-Background', border_class='ColorScheme-Text',
                 inner_glow=0.0, mask=True):
        self.p = prefix
        self.r = radius
        self.op = opacity
        self.bop = border_opacity
        self.s = shadow
        self.sop = shadow_opacity
        self.m = margin if margin is not None else max(4, radius // 2 + 2)
        self.fill = fill_class
        self.border = border_class
        self.glow = inner_glow
        self.mask = mask

    # --- helpers -------------------------------------------------------
    def _fill(self, d):
        return f'<path class="{self.fill}" style="fill:currentColor;opacity:{f(self.op)}" d="{d}"/>'

    # Outlines are drawn as filled shapes: QtSvg includes the stroke width in the
    # element bounds, which would shrink the tiles and leave seams between them.
    def _line(self, d):
        if self.bop <= 0:
            return ''
        return f'<path class="{self.border}" style="fill:currentColor;opacity:{f(self.bop)}" d="{d}"/>'

    def _glow(self, d):
        # one-pixel light edge along the top, the "glass" highlight
        if self.glow <= 0:
            return ''
        return f'<path style="fill:#ffffff;opacity:{f(self.glow)}" d="{d}"/>'

    @staticmethod
    def ring(r, t, corner):
        """Quarter ring between radius r and r - t for the given corner, in tile coordinates."""
        ri = r - t
        if corner == 'tl':
            return f'M0,{r} A{r},{r} 0 0 1 {r},0 V{f(t)} A{f(ri)},{f(ri)} 0 0 0 {f(t)},{r} Z'
        if corner == 'tr':
            return f'M0,0 A{r},{r} 0 0 1 {r},{r} H{f(ri)} A{f(ri)},{f(ri)} 0 0 0 0,{f(t)} Z'
        if corner == 'bl':
            return f'M0,0 A{r},{r} 0 0 0 {r},{r} V{f(ri)} A{f(ri)},{f(ri)} 0 0 1 {f(t)},0 Z'
        return f'M{r},0 A{r},{r} 0 0 1 0,{r} V{f(ri)} A{f(ri)},{f(ri)} 0 0 0 {f(ri)},0 Z'

    def frame(self, ox, oy):
        r, p = self.r, self.p
        out = []
        X = [ox, ox + r, ox + r + C]
        Y = [oy, oy + r, oy + r + C]
        # corners (fill + outline arc)
        out.append(f'<g id="{p}topleft" transform="translate({f(X[0])},{f(Y[0])})">'
                   + self._fill(f'M0,{r} A{r},{r} 0 0 1 {r},0 L{r},{r} Z')
                   + self._line(self.ring(r, 1, 'tl')) + '</g>')
        out.append(f'<g id="{p}topright" transform="translate({f(X[2])},{f(Y[0])})">'
                   + self._fill(f'M0,0 A{r},{r} 0 0 1 {r},{r} L0,{r} Z')
                   + self._line(self.ring(r, 1, 'tr')) + '</g>')
        out.append(f'<g id="{p}bottomleft" transform="translate({f(X[0])},{f(Y[2])})">'
                   + self._fill(f'M0,0 L{r},0 L{r},{r} A{r},{r} 0 0 1 0,0 Z')
                   + self._line(self.ring(r, 1, 'bl')) + '</g>')
        out.append(f'<g id="{p}bottomright" transform="translate({f(X[2])},{f(Y[2])})">'
                   + self._fill(f'M0,0 L{r},0 A{r},{r} 0 0 1 0,{r} Z')
                   + self._line(self.ring(r, 1, 'br')) + '</g>')
        # edges
        out.append(f'<g id="{p}top" transform="translate({f(X[1])},{f(Y[0])})">'
                   + self._fill(f'M0,0 H{C} V{r} H0 Z') + self._line(f'M0,0 H{C} V1 H0 Z')
                   + self._glow(f'M0,1 H{C} V2 H0 Z') + '</g>')
        out.append(f'<g id="{p}bottom" transform="translate({f(X[1])},{f(Y[2])})">'
                   + self._fill(f'M0,0 H{C} V{r} H0 Z') + self._line(f'M0,{r - 1} H{C} V{r} H0 Z') + '</g>')
        out.append(f'<g id="{p}left" transform="translate({f(X[0])},{f(Y[1])})">'
                   + self._fill(f'M0,0 H{r} V{C} H0 Z') + self._line(f'M0,0 H1 V{C} H0 Z') + '</g>')
        out.append(f'<g id="{p}right" transform="translate({f(X[2])},{f(Y[1])})">'
                   + self._fill(f'M0,0 H{r} V{C} H0 Z') + self._line(f'M{r - 1},0 H{r} V{C} H{r - 1} Z') + '</g>')
        out.append(f'<g id="{p}center" transform="translate({f(X[1])},{f(Y[1])})">'
                   + self._fill(f'M0,0 H{C} V{C} H0 Z') + '</g>')
        # content margin hints
        m = self.m
        out.append(f'<rect id="{p}hint-top-margin" x="{f(X[1])}" y="{f(Y[0])}" width="4" height="{m}" style="fill:#ff00ff;opacity:0"/>')
        out.append(f'<rect id="{p}hint-bottom-margin" x="{f(X[1])}" y="{f(Y[2] + r - m)}" width="4" height="{m}" style="fill:#ff00ff;opacity:0"/>')
        out.append(f'<rect id="{p}hint-left-margin" x="{f(X[0])}" y="{f(Y[1])}" width="{m}" height="4" style="fill:#ff00ff;opacity:0"/>')
        out.append(f'<rect id="{p}hint-right-margin" x="{f(X[2] + r - m)}" y="{f(Y[1])}" width="{m}" height="4" style="fill:#ff00ff;opacity:0"/>')
        return out

    def masks(self, ox, oy):
        r, p = self.r, self.p
        X = [ox, ox + r, ox + r + C]
        Y = [oy, oy + r, oy + r + C]
        return [
            f'<path id="{p}mask-topleft" d="M{f(X[0])},{f(Y[1])} A{r},{r} 0 0 1 {f(X[1])},{f(Y[0])} L{f(X[1])},{f(Y[1])} Z"/>',
            f'<path id="{p}mask-topright" d="M{f(X[2])},{f(Y[0])} A{r},{r} 0 0 1 {f(X[2] + r)},{f(Y[1])} L{f(X[2])},{f(Y[1])} Z"/>',
            f'<path id="{p}mask-bottomleft" d="M{f(X[0])},{f(Y[2])} L{f(X[1])},{f(Y[2])} L{f(X[1])},{f(Y[2] + r)} A{r},{r} 0 0 1 {f(X[0])},{f(Y[2])} Z"/>',
            f'<path id="{p}mask-bottomright" d="M{f(X[2])},{f(Y[2])} L{f(X[2] + r)},{f(Y[2])} A{r},{r} 0 0 1 {f(X[2])},{f(Y[2] + r)} Z"/>',
            f'<rect id="{p}mask-top" x="{f(X[1])}" y="{f(Y[0])}" width="{C}" height="{r}"/>',
            f'<rect id="{p}mask-bottom" x="{f(X[1])}" y="{f(Y[2])}" width="{C}" height="{r}"/>',
            f'<rect id="{p}mask-left" x="{f(X[0])}" y="{f(Y[1])}" width="{r}" height="{C}"/>',
            f'<rect id="{p}mask-right" x="{f(X[2])}" y="{f(Y[1])}" width="{r}" height="{C}"/>',
            f'<rect id="{p}mask-center" x="{f(X[1])}" y="{f(Y[1])}" width="{C}" height="{C}"/>',
        ]

    def shadows(self, ox, oy, defs):
        """Shadow tiles. (ox, oy) is the top-left corner of the shadow area."""
        if self.s <= 0:
            return []
        s, r, p, a = self.s, self.r, self.p, self.sop
        T = s + r
        gid = (p or 'x') + 'sh'
        # falloff: roughly gaussian
        stops = [(0.0, 1.0), (0.25, 0.55), (0.5, 0.22), (0.75, 0.06), (1.0, 0.0)]
        lin = ''.join(f'<stop offset="{f(o)}" style="stop-color:#000;stop-opacity:{f(a * v)}"/>' for o, v in stops)
        rad = (f'<stop offset="{f(r / T)}" style="stop-color:#000;stop-opacity:{f(a)}"/>'
               + ''.join(f'<stop offset="{f((r + o * s) / T)}" style="stop-color:#000;stop-opacity:{f(a * v)}"/>'
                         for o, v in stops[1:]))
        defs.append(f'<linearGradient id="{gid}-lin" x1="0" y1="0" x2="0" y2="1">{lin}</linearGradient>')
        defs.append(f'<radialGradient id="{gid}-rad" gradientUnits="userSpaceOnUse" cx="{T}" cy="{T}" r="{T}">{rad}</radialGradient>')
        # vertical gradient going *up* from the frame edge, used for the top side
        defs.append(f'<linearGradient id="{gid}-up" href="#{gid}-lin" xlink:href="#{gid}-lin" x1="0" y1="1" x2="0" y2="0"/>')
        corner = f'M0,0 H{T} V{s} A{r},{r} 0 0 0 {s},{T} H0 Z'
        corner_fill = f'<path style="fill:url(#{gid}-rad)" d="{corner}"/>'
        X = [ox, ox + T, ox + T + C]
        Y = [oy, oy + T, oy + T + C]
        out = []
        # corners: draw the top-left one and rotate it for the others
        out.append(f'<g id="{p}shadow-topleft" transform="translate({f(X[0])},{f(Y[0])})">{corner_fill}</g>')
        out.append(f'<g id="{p}shadow-topright" transform="translate({f(X[2] + T)},{f(Y[0])}) scale(-1,1)">{corner_fill}</g>')
        out.append(f'<g id="{p}shadow-bottomleft" transform="translate({f(X[0])},{f(Y[2] + T)}) scale(1,-1)">{corner_fill}</g>')
        out.append(f'<g id="{p}shadow-bottomright" transform="translate({f(X[2] + T)},{f(Y[2] + T)}) scale(-1,-1)">{corner_fill}</g>')
        inv = 'style="fill:#000;opacity:0.001"'
        out.append(f'<g id="{p}shadow-top" transform="translate({f(X[1])},{f(Y[0])})">'
                   f'<rect width="{C}" height="{s}" style="fill:url(#{gid}-up)"/>'
                   f'<rect y="{s}" width="{C}" height="{r}" {inv}/></g>')
        out.append(f'<g id="{p}shadow-bottom" transform="translate({f(X[1])},{f(Y[2])})">'
                   f'<rect width="{C}" height="{r}" {inv}/>'
                   f'<rect y="{r}" width="{C}" height="{s}" style="fill:url(#{gid}-lin)"/></g>')
        out.append(f'<g id="{p}shadow-left" transform="translate({f(X[0])},{f(Y[1])})">'
                   f'<rect width="{s}" height="{C}" style="fill:url(#{gid}-hl)"/>'
                   f'<rect x="{s}" width="{r}" height="{C}" {inv}/></g>')
        out.append(f'<g id="{p}shadow-right" transform="translate({f(X[2])},{f(Y[1])})">'
                   f'<rect width="{r}" height="{C}" {inv}/>'
                   f'<rect x="{r}" width="{s}" height="{C}" style="fill:url(#{gid}-h)"/></g>')
        defs.append(f'<linearGradient id="{gid}-h" href="#{gid}-lin" xlink:href="#{gid}-lin" x1="0" y1="0" x2="1" y2="0"/>')
        defs.append(f'<linearGradient id="{gid}-hl" href="#{gid}-lin" xlink:href="#{gid}-lin" x1="1" y1="0" x2="0" y2="0"/>')
        out.append(f'<g id="{p}shadow-center" transform="translate({f(X[1])},{f(Y[1])})"><rect width="{C}" height="{C}" {inv}/></g>')
        # how far the shadow extends beyond the frame
        out.append(f'<rect id="{p}shadow-hint-top-margin" x="{f(X[1])}" y="{f(Y[0])}" width="2" height="{s}" style="opacity:0"/>')
        out.append(f'<rect id="{p}shadow-hint-bottom-margin" x="{f(X[1])}" y="{f(Y[2] + r)}" width="2" height="{s}" style="opacity:0"/>')
        out.append(f'<rect id="{p}shadow-hint-left-margin" x="{f(X[0])}" y="{f(Y[1])}" width="{s}" height="2" style="opacity:0"/>')
        out.append(f'<rect id="{p}shadow-hint-right-margin" x="{f(X[2] + r)}" y="{f(Y[1])}" width="{s}" height="2" style="opacity:0"/>')
        return out

    def size(self):
        r, s = self.r, self.s
        return 2 * (r + s) + C


def write_svg(path, frames, extra=None):
    defs = []
    body = []
    x = 0
    height = 0
    for fr in frames:
        w = fr.size()
        body += fr.shadows(x, 0, defs)
        body += fr.frame(x + fr.s, fr.s)
        x += w + 8
        if fr.mask:
            mw = 2 * fr.r + C
            body += fr.masks(x, fr.s)
            x += mw + 8
        height = max(height, w)
    if extra:
        body += extra
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w') as fh:
        fh.write('<?xml version="1.0" encoding="UTF-8"?>\n')
        fh.write('<!-- SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors -->\n')
        fh.write('<!-- SPDX-License-Identifier: GPL-3.0-or-later -->\n')
        fh.write('<!-- Generated by tools/gen-plasma-style.py -->\n')
        fh.write(f'<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" '
                 f'version="1.1" width="{x}" height="{height}" viewBox="0 0 {x} {height}">\n')
        fh.write('<defs>' + STYLE + ''.join(defs) + '</defs>\n')
        fh.write('\n'.join(body))
        fh.write('\n</svg>\n')


# Design values -------------------------------------------------------------
# Radii: DTK uses 8 px for controls (PM_FrameRadius), 18 px for floating
# widgets / top-level windows (PM_TopLevelWindowRadius). The dock is drawn
# with a 16 px radius.
# Opacity: DTK's DBlurEffectWidget uses mask alpha 102/255 (= 0.4) when the
# compositor blurs behind the window, 204/255 (= 0.8) otherwise.
VARIANTS = {
    # name: (opacity panel, opacity dialog, opacity tooltip, shadow opacity)
    'translucent': (0.55, 0.70, 0.80, 0.22),
    'base': (0.92, 0.95, 0.95, 0.22),
    'opaque': (1.0, 1.0, 1.0, 0.25),
}


def generate(root):
    for variant, (op_panel, op_dialog, op_tip, sop) in VARIANTS.items():
        sub = '' if variant == 'base' else variant + '/'
        write_svg(os.path.join(root, sub + 'widgets/panel-background.svg'),
                  [Frame('', 16, op_panel, 0.10, shadow=14, shadow_opacity=sop, margin=6, inner_glow=0.25 if variant == 'translucent' else 0)])
        write_svg(os.path.join(root, sub + 'dialogs/background.svg'),
                  [Frame('', 12, op_dialog, 0.10, shadow=18, shadow_opacity=sop, margin=8, inner_glow=0.20 if variant == 'translucent' else 0)])
        write_svg(os.path.join(root, sub + 'widgets/tooltip.svg'),
                  [Frame('', 8, op_tip, 0.08, shadow=10, shadow_opacity=sop * 0.8, margin=6)])
        write_svg(os.path.join(root, sub + 'widgets/background.svg'),
                  [Frame('', 18, op_dialog, 0.08, shadow=16, shadow_opacity=sop, margin=10, inner_glow=0.20 if variant == 'translucent' else 0)])

    # Task manager: hover / focus / attention / minimized states, no masks or shadows
    tasks = [
        Frame('normal-', 10, 0.0, 0.0, margin=6, mask=False),
        Frame('hover-', 10, 0.12, 0.08, margin=6, fill_class='ColorScheme-Text', mask=False),
        Frame('focus-', 10, 0.18, 0.10, margin=6, fill_class='ColorScheme-Text', mask=False),
        Frame('attention-', 10, 0.35, 0.0, margin=6, fill_class='ColorScheme-NeutralText', mask=False),
        Frame('minimized-', 10, 0.0, 0.0, margin=6, mask=False),
    ]
    write_svg(os.path.join(root, 'widgets/tasks.svg'), tasks)

    # View items in popups (kickoff, lists): rounded 8px like DTK item backgrounds
    items = [
        Frame('normal-', 8, 0.0, 0.0, margin=4, mask=False),
        Frame('hover-', 8, 0.10, 0.0, margin=4, fill_class='ColorScheme-Text', mask=False),
        Frame('selected-', 8, 0.25, 0.0, margin=4, fill_class='ColorScheme-Highlight', mask=False),
        Frame('selected+hover-', 8, 0.35, 0.0, margin=4, fill_class='ColorScheme-Highlight', mask=False),
    ]
    write_svg(os.path.join(root, 'widgets/viewitem.svg'), items)


if __name__ == '__main__':
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), '..', 'plasma', 'desktoptheme', 'plasma-deepin')
    generate(out)
    print('generated in', out)
