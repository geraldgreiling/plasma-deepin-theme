/*
 * SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <KDecoration3/DecorationButton>

#include <QVariantAnimation>

namespace DeepinGlass
{
class Decoration;

class Button : public KDecoration3::DecorationButton
{
    Q_OBJECT

public:
    explicit Button(QObject *parent, const QVariantList &args);
    Button(KDecoration3::DecorationButtonType type, Decoration *decoration, QObject *parent = nullptr);

    static Button *create(KDecoration3::DecorationButtonType type, KDecoration3::Decoration *decoration, QObject *parent);

    void paint(QPainter *painter, const QRectF &repaintArea) override;

private:
    void drawSymbol(QPainter *painter, const QRectF &box, const QColor &color) const;
    QVariantAnimation *m_hoverAnimation;
    qreal m_hover = 0.0;
};

} // namespace DeepinGlass
