# Visual checks

These helpers were used to produce the screenshots in `docs/screenshots` and
to check the components without a running Plasma session.

* `gallery/` – Qt Widgets gallery. `gallery -style DeepinGlass [--grab out.png]`,
  `--backdrop` shows a colourful full-screen window to look through the glass.
* `qmlshot/` – renders a QML file to PNG: splash screen (`--stage 2`) and the
  SDDM theme (`--sddm` provides mock `sddm`, `userModel`, `sessionModel`).

Window decoration, application style and dock were checked in a nested
`kwin_wayland --x11-display` session inside Xvfb (software rendering). In that
setup KWin composites with QPainter, so **blur, decoration shadows and rounded
client corners are not visible in the screenshots**; they need KWin's OpenGL
compositor on real hardware.
