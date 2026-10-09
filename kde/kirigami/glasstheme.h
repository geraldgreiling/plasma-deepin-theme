/*
    Based on PlasmaDesktopTheme from qqc2-desktop-style (KDE, commit 20471364a82a).
    Changes: translucent backgrounds for the Deepin Glass look, see glasstheme.cpp.

    SPDX-FileCopyrightText: 2017 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef DEEPINGLASS_KIRIGAMI_THEME_H
#define DEEPINGLASS_KIRIGAMI_THEME_H

#include <Kirigami/Platform/PlatformTheme>

#include <QColor>
#include <QIcon>
#include <QObject>
#include <QPointer>
#include <QQuickItem>

class StyleSingleton;

class GlassTheme : public Kirigami::Platform::PlatformTheme
{
    Q_OBJECT

public:
    explicit GlassTheme(QObject *parent = nullptr);
    ~GlassTheme() override;

    Q_INVOKABLE QIcon iconFromTheme(const QString &name, const QColor &customColor = Qt::transparent) override;

    void syncWindow();
    void syncColors();
    void syncFrameContrast();
    /// Deepin Glass: recomputes the colours of all themes
    static void syncAllColors();

protected:
    bool event(QEvent *event) override;

private:
    friend class StyleSingleton;
    QPointer<QWindow> m_window;
    QMetaObject::Connection m_sgConnection;
};

#endif // DEEPINGLASS_KIRIGAMI_THEME_H
