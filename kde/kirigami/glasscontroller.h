/*
 * SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "glassconfig.h"

#include <Kirigami/Platform/PlatformTheme>

#include <QColor>
#include <QObject>
#include <QPalette>
#include <QPointer>
#include <QHash>
#include <QSet>

class QQuickItem;
class QQuickWindow;

namespace DeepinGlass
{

/**
 * Process wide state of the Deepin Glass Kirigami integration:
 * decides whether glass is used in this application, gives Kirigami windows an
 * alpha channel before they are created and asks KWin to blur behind them.
 */
class GlassController : public QObject
{
    Q_OBJECT

public:
    static GlassController *self();

    bool isEnabled() const
    {
        return m_enabled;
    }

    /// Opacity factor for the background colours of a colour set (1 = unchanged).
    /// @p themeParent is the object the Kirigami theme is attached to, @p group the
    /// colour group (active / inactive window) the colours are computed for.
    qreal backgroundOpacity(Kirigami::Platform::PlatformTheme::ColorSet set, QObject *themeParent, QPalette::ColorGroup group) const;

    /// Called for every window a Kirigami theme gets attached to. @p ownTheme: the
    /// theme is attached to the window itself and provides its colour (Kirigami
    /// ApplicationWindow); otherwise the application sets the colour (QQuickView).
    void prepareWindow(QQuickWindow *window, bool ownTheme = false);

    /// Offscreen window of a QQuickWidget that the Deepin Glass widget style made
    /// translucent (dynamic property set by the style).
    static bool isGlassOffscreenWindow(const QQuickWindow *window);
    /// Top level windows that get glass: normal windows and dialogs with a title bar.
    static bool isGlassCandidate(const QQuickWindow *window);

    /// Called for every item a Kirigami theme gets attached to: hides fills that
    /// only make sense below an opaque surface (see the implementation).
    void polishItem(QQuickItem *item);

    static QColor withOpacity(QColor color, qreal opacity)
    {
        if (opacity < 1.0) {
            color.setAlphaF(color.alphaF() * opacity);
        }
        return color;
    }

protected:
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    GlassController();
    void updateBlur(QQuickWindow *window);
    /// Sets the clear colour of a glass window: windows without a theme of their own
    /// get the title bar's opacity, and hardware backends get the colour premultiplied
    /// (see the implementation).
    void applyClearColor(QQuickWindow *window);
    /// Opacity of the window surface: the configured value or the title bar's.
    qreal windowOpacity(bool active) const;

    QuickConfig m_config;
    DecorationConfig m_decoConfig;
    bool m_enabled = false;
    QSet<QQuickWindow *> m_windows;
    QSet<QQuickWindow *> m_themedWindows;
    QHash<QQuickWindow *, QColor> m_applied; // colour set by applyClearColor()
    QHash<QQuickWindow *, QColor> m_requested; // colour set by the application
};

} // namespace DeepinGlass
