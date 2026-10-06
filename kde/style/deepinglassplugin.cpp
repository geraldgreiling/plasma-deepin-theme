/*
 * SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "deepinglassstyle.h"

#include <QStylePlugin>

namespace DeepinGlass
{
class StylePlugin : public QStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QStyleFactoryInterface" FILE "deepinglass.json")

public:
    QStyle *create(const QString &key) override
    {
        if (key.compare(QLatin1String("deepinglass"), Qt::CaseInsensitive) == 0) {
            return new Style;
        }
        return nullptr;
    }
};
} // namespace DeepinGlass

#include "deepinglassplugin.moc"
