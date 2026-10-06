/*
    SPDX-FileCopyrightText: 2021 Jonah Brüchert <jbb@kaidan.im>

    SPDX-License-Identifier: LGPL-2.0-or-later

    Copied unchanged from qqc2-desktop-style (KDE, commit 20471364a82a), see README.md in this directory.
*/

#ifndef KIRIGAMIPLASMADESKTOPUNITS_H
#define KIRIGAMIPLASMADESKTOPUNITS_H

#include <QObject>
#include <QPropertyNotifier>

#include <Kirigami/Platform/Units>

class AnimationSpeedProvider;

class PlasmaDesktopUnits : public Kirigami::Platform::Units
{
    Q_OBJECT

public:
    explicit PlasmaDesktopUnits(QObject *parent = nullptr);

    void updateAnimationSpeed();

private:
    std::unique_ptr<AnimationSpeedProvider> m_animationSpeedProvider;
    QPropertyNotifier m_notifier;
};

#endif
