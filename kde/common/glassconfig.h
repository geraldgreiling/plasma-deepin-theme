/*
 * SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Shared settings of the Deepin Glass window decoration, the Qt Widgets
 * application style and the Kirigami (Qt Quick) integration.
 * All of them read ~/.config/deepinglassrc. If the file does not exist it is
 * created from kde/common/deepinglassrc.default (defaults with comments).
 */
#pragma once

#include <KConfigGroup>
#include <KSharedConfig>
#include <QDir>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QStringList>
#include <QtGlobal>

// Default file content, generated from kde/common/deepinglassrc.default by CMake
#include "deepinglass_defaultconfig.h"

namespace DeepinGlass
{


/// Creates ~/.config/deepinglassrc with the defaults and comments if it does not exist yet.
inline void ensureConfigFile()
{
    static bool checked = false;
    if (checked) {
        return;
    }
    checked = true;
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    if (dir.isEmpty()) {
        return;
    }
    const QString path = dir + QStringLiteral("/deepinglassrc");
    if (QFileInfo::exists(path)) {
        return;
    }
    QDir().mkpath(dir);
    QSaveFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(s_defaultConfig);
        file.commit();
    }
}

inline KSharedConfig::Ptr config()
{
    ensureConfigFile();
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
    qreal viewOpacity = 0.55;
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
        c.viewOpacity = qBound(0.0, g.readEntry("ViewOpacity", c.viewOpacity), 1.0);
        c.excluded = g.readEntry("ExcludedApplications", QStringList());
        return c;
    }
};

struct QuickConfig {
    bool translucent = true;
    qreal windowOpacity = 0.5;
    qreal viewOpacity = 0.4;
    qreal headerOpacity = 0.45;
    QStringList excluded;

    static QuickConfig load()
    {
        auto cfg = config();
        cfg->reparseConfiguration();
        const KConfigGroup g(cfg, QStringLiteral("QtQuick"));
        QuickConfig c;
        c.translucent = g.readEntry("Translucent", c.translucent);
        c.windowOpacity = qBound(0.0, g.readEntry("WindowOpacity", c.windowOpacity), 1.0);
        c.viewOpacity = qBound(0.0, g.readEntry("ViewOpacity", c.viewOpacity), 1.0);
        c.headerOpacity = qBound(0.0, g.readEntry("HeaderOpacity", c.headerOpacity), 1.0);
        c.excluded = g.readEntry("ExcludedApplications", QStringList());
        return c;
    }
};

} // namespace DeepinGlass
