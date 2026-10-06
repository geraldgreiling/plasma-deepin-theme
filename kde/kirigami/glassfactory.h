/*
 * SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <Kirigami/Platform/PlatformPluginFactory>

class GlassFactory : public Kirigami::Platform::PlatformPluginFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PlatformPluginFactory_iid FILE "deepinglass.json")
    Q_INTERFACES(Kirigami::Platform::PlatformPluginFactory)

public:
    explicit GlassFactory(QObject *parent = nullptr);
    ~GlassFactory() override;

    Kirigami::Platform::PlatformTheme *createPlatformTheme(QObject *parent) override;
    Kirigami::Platform::Units *createUnits(QObject *parent) override;
};
