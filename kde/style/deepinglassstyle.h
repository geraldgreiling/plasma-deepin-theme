/*
 * SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "glassconfig.h"

#include <QPaintEvent>
#include <QPointer>
#include <QProxyStyle>
#include <QStyleOption>
#include <QSet>

namespace DeepinGlass
{

/**
 * Qt Widgets style for the Deepin look.
 *
 * Breeze does the heavy lifting (metrics, animations, the many special cases of
 * KDE applications). On top of it this proxy style
 *  - makes main windows and dialogs translucent and asks KWin to blur behind
 *    them (frosted glass that adapts to whatever is behind the window),
 *  - paints translucent, blurred popup menus,
 *  - draws push buttons, line edits, combo boxes, spin boxes, check boxes,
 *    radio buttons, scroll bars and progress bars in the DTK manner:
 *    filled, 8 px rounded, no outlines, accent coloured focus ring.
 */
class Style : public QProxyStyle
{
    Q_OBJECT

public:
    Style();
    ~Style() override;

    void polish(QWidget *widget) override;
    void polish(QPalette &palette) override;
    void polish(QApplication *app) override;
    void unpolish(QWidget *widget) override;
    using QProxyStyle::polish;
    using QProxyStyle::unpolish;

    int styleHint(StyleHint hint, const QStyleOption *option = nullptr, const QWidget *widget = nullptr, QStyleHintReturn *returnData = nullptr) const override;
    int pixelMetric(PixelMetric metric, const QStyleOption *option = nullptr, const QWidget *widget = nullptr) const override;

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = nullptr) const override;
    void drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = nullptr) const override;
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget = nullptr) const override;

protected:
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    /// private style hint: answered with DeepinGlassMagic while this style is in use
    static constexpr int SH_DeepinGlassActive = SH_CustomBase + 0x4447;
    static constexpr int DeepinGlassMagic = 0x44474c53;
    /// Make a top level window translucent. Must happen before the native window exists.
    void tryMakeTranslucent(const QWidget *widget) const;
    bool isTranslucentWindow(const QWidget *widget) const;
    bool compositingActive() const;
    void updateBlur(QWidget *widget) const;
    void paintWindowBackground(QWidget *window, QPaintEvent *event) const;
    void makeContentTranslucent(QWidget *widget) const;
    qreal windowOpacity(bool active) const;
    QRect toolsAreaRect(const QWidget *window) const;
    /// QML in widget windows (QQuickWidget, e.g. System Settings): glass clear colour
    void updateQuickWidget(QWidget *quickWidget) const;
    /// item views: selections follow the window activation, not the keyboard focus
    static const QStyleOption *activeItemOption(const QStyleOption *option, const QWidget *widget, QStyleOptionViewItem &copy);

    void drawButtonPanel(const QStyleOption *option, QPainter *painter, bool isDefault, bool flat) const;
    void drawInputPanel(const QStyleOption *option, QPainter *painter, const QRect &rect) const;
    void drawCheckIndicator(const QStyleOption *option, QPainter *painter, bool radio) const;

    StyleConfig m_config;
    DecorationConfig m_decoConfig;
    bool m_excludedApplication = false;
    mutable QSet<const QWidget *> m_translucentWindows;
};

} // namespace DeepinGlass
