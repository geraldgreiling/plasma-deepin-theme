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
#include <QSet>

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

    /// Called for every window a Kirigami theme gets attached to.
    void prepareWindow(QQuickWindow *window);

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
    /// Opacity of the window surface: the configured value or the title bar's.
    qreal windowOpacity(bool active) const;

    QuickConfig m_config;
    DecorationConfig m_decoConfig;
    bool m_enabled = false;
    QSet<QQuickWindow *> m_windows;
};

} // namespace DeepinGlass
