/*
 * SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "glassconfig.h"

#include <KDecoration3/Decoration>
#include <KDecoration3/DecorationButtonGroup>

#include <QVariantAnimation>

namespace DeepinGlass
{

class Decoration : public KDecoration3::Decoration
{
    Q_OBJECT

public:
    explicit Decoration(QObject *parent = nullptr, const QVariantList &args = QVariantList());
    ~Decoration() override;

    bool init() override;
    void paint(QPainter *painter, const QRectF &repaintArea) override;

    /// colours used by the buttons
    QColor foregroundColor() const;
    QColor titleBarColor() const;
    bool isDarkTheme() const;
    qreal titleBarOpacity() const;

    const DecorationConfig &glassConfig() const
    {
        return m_config;
    }
    qreal buttonSize() const;
    qreal cornerRadius() const;

private Q_SLOTS:
    void reconfigure();
    void recalculateBorders();
    void updateTitleBar();
    void updateButtonsGeometry();
    void updateBlur();
    void updateShadow();

private:
    bool isMaximized() const;
    bool isTiled() const;
    bool drawRoundedCorners() const;
    qreal sideBorder() const;
    qreal bottomBorder() const;
    void paintTitleBar(QPainter *painter, const QRectF &repaintArea);
    QRectF captionRect() const;

    DecorationConfig m_config;
    KDecoration3::DecorationButtonGroup *m_leftButtons = nullptr;
    KDecoration3::DecorationButtonGroup *m_rightButtons = nullptr;
    QVariantAnimation *m_activeAnimation = nullptr;
    qreal m_activeProgress = 1.0;
};

} // namespace DeepinGlass
