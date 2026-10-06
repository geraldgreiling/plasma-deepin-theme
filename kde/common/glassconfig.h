/*
 * SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Shared settings of the Deepin Glass window decoration and application style.
 * Both read ~/.config/deepinglassrc, so one file controls the whole glass look:
 *
 *   [Decoration]
 *   TitleBarHeight=40        logical pixels
 *   CornerRadius=12          window corner radius
 *   ActiveOpacity=0.72       0..1, opacity of the title bar of the active window
 *   InactiveOpacity=0.58     0..1, inactive windows
 *   Blur=true                ask KWin to blur behind the title bar
 *   ShadowSize=36            0 disables the shadow
 *   ShadowStrength=0.38      0..1
 *   Outline=true             thin outline around the window
 *   CenterTitle=true
 *
 *   [Style]
 *   Translucent=true         frosted window backgrounds for Qt Widgets applications
 *   WindowOpacity=0.78       0..1, opacity of the window background
 *   MenuOpacity=0.82         0..1, popup menus
 *   SidebarOpacity=0.0       0..1, opacity of side panels (Dolphin places, ...) on top of the window
 *   ExcludedApplications=    comma separated list of executable names that stay opaque
 */
#pragma once

#include <KConfigGroup>
#include <KSharedConfig>
#include <QStringList>
#include <QtGlobal>

namespace DeepinGlass
{

inline KSharedConfig::Ptr config()
{
    return KSharedConfig::openConfig(QStringLiteral("deepinglassrc"), KConfig::SimpleConfig);
}

struct DecorationConfig {
    int titleBarHeight = 40;
    int cornerRadius = 12;
    qreal activeOpacity = 0.72;
    qreal inactiveOpacity = 0.58;
    bool blur = true;
    int shadowSize = 36;
    qreal shadowStrength = 0.38;
    bool outline = true;
    bool centerTitle = true;

    static DecorationConfig load()
    {
        auto cfg = config();
        cfg->reparseConfiguration();
        const KConfigGroup g(cfg, QStringLiteral("Decoration"));
        DecorationConfig c;
        c.titleBarHeight = qBound(24, g.readEntry("TitleBarHeight", c.titleBarHeight), 80);
        c.cornerRadius = qBound(0, g.readEntry("CornerRadius", c.cornerRadius), 32);
        c.activeOpacity = qBound(0.0, g.readEntry("ActiveOpacity", c.activeOpacity), 1.0);
        c.inactiveOpacity = qBound(0.0, g.readEntry("InactiveOpacity", c.inactiveOpacity), 1.0);
        c.blur = g.readEntry("Blur", c.blur);
        c.shadowSize = qBound(0, g.readEntry("ShadowSize", c.shadowSize), 96);
        c.shadowStrength = qBound(0.0, g.readEntry("ShadowStrength", c.shadowStrength), 1.0);
        c.outline = g.readEntry("Outline", c.outline);
        c.centerTitle = g.readEntry("CenterTitle", c.centerTitle);
        return c;
    }
};

struct StyleConfig {
    bool translucent = true;
    qreal windowOpacity = 0.78;
    qreal menuOpacity = 0.82;
    qreal sidebarOpacity = 0.0;
    QStringList excluded;

    static StyleConfig load()
    {
        auto cfg = config();
        cfg->reparseConfiguration();
        const KConfigGroup g(cfg, QStringLiteral("Style"));
        StyleConfig c;
        c.translucent = g.readEntry("Translucent", c.translucent);
        c.windowOpacity = qBound(0.0, g.readEntry("WindowOpacity", c.windowOpacity), 1.0);
        c.menuOpacity = qBound(0.0, g.readEntry("MenuOpacity", c.menuOpacity), 1.0);
        c.sidebarOpacity = qBound(0.0, g.readEntry("SidebarOpacity", c.sidebarOpacity), 1.0);
        c.excluded = g.readEntry("ExcludedApplications", QStringList());
        return c;
    }
};

} // namespace DeepinGlass
