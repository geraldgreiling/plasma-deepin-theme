/*
 * SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "glassfactory.h"
#include "glasscontroller.h"
#include "glasstheme.h"
#include "plasmadesktopunits.h"

GlassFactory::GlassFactory(QObject *parent)
    : Kirigami::Platform::PlatformPluginFactory(parent)
{
    // The plugin is loaded while the first QML file is compiled, before any window
    // exists: the earliest point to request alpha channels for the windows.
    DeepinGlass::GlassController::self();
}

GlassFactory::~GlassFactory() = default;

Kirigami::Platform::PlatformTheme *GlassFactory::createPlatformTheme(QObject *parent)
{
    Q_ASSERT(parent);
    return new GlassTheme(parent);
}

Kirigami::Platform::Units *GlassFactory::createUnits(QObject *parent)
{
    Q_ASSERT(parent);
    return new PlasmaDesktopUnits(parent);
}

#include "moc_glassfactory.cpp"
