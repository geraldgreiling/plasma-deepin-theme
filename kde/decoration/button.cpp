/*
 * SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Title bar buttons in the Deepin style: thin line symbols, a rounded
 * translucent background on hover, red background for the close button.
 */
#include "button.h"
#include "decoration.h"

#include <KDecoration3/DecoratedWindow>

#include <QPainter>
#include <QPainterPath>

namespace DeepinGlass
{
using KDecoration3::DecorationButtonType;

Button::Button(DecorationButtonType type, Decoration *decoration, QObject *parent)
    : KDecoration3::DecorationButton(type, decoration, parent)
    , m_hoverAnimation(new QVariantAnimation(this))
{
    m_hoverAnimation->setStartValue(0.0);
    m_hoverAnimation->setEndValue(1.0);
    m_hoverAnimation->setDuration(120);
    m_hoverAnimation->setEasingCurve(QEasingCurve::InOutQuad);
    connect(m_hoverAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        m_hover = v.toReal();
        update();
    });
    connect(this, &KDecoration3::DecorationButton::hoveredChanged, this, [this](bool hovered) {
        m_hoverAnimation->setDirection(hovered ? QAbstractAnimation::Forward : QAbstractAnimation::Backward);
        if (m_hoverAnimation->state() != QAbstractAnimation::Running) {
            m_hoverAnimation->start();
        }
    });
    connect(this, &KDecoration3::DecorationButton::pressedChanged, this, [this] {
        update();
    });
}

// used by the KDecoration preview (kcm)
Button::Button(QObject *parent, const QVariantList &args)
    : Button(args.at(0).value<DecorationButtonType>(), args.at(1).value<Decoration *>(), parent)
{
    setGeometry(QRectF(0, 0, 40, 40));
}

Button *Button::create(DecorationButtonType type, KDecoration3::Decoration *decoration, QObject *parent)
{
    auto d = qobject_cast<Decoration *>(decoration);
    if (!d) {
        return nullptr;
    }
    auto b = new Button(type, d, parent);
    const auto w = d->window();
    switch (type) {
    case DecorationButtonType::Close:
        b->setVisible(w->isCloseable());
        connect(w, &KDecoration3::DecoratedWindow::closeableChanged, b, &Button::setVisible);
        break;
    case DecorationButtonType::Maximize:
        b->setVisible(w->isMaximizeable());
        connect(w, &KDecoration3::DecoratedWindow::maximizeableChanged, b, &Button::setVisible);
        connect(w, &KDecoration3::DecoratedWindow::maximizedChanged, b, [b] {
            b->update();
        });
        break;
    case DecorationButtonType::Minimize:
        b->setVisible(w->isMinimizeable());
        connect(w, &KDecoration3::DecoratedWindow::minimizeableChanged, b, &Button::setVisible);
        break;
    case DecorationButtonType::ContextHelp:
        b->setVisible(w->providesContextHelp());
        connect(w, &KDecoration3::DecoratedWindow::providesContextHelpChanged, b, &Button::setVisible);
        break;
    case DecorationButtonType::Shade:
        b->setVisible(w->isShadeable());
        connect(w, &KDecoration3::DecoratedWindow::shadeableChanged, b, &Button::setVisible);
        break;
    case DecorationButtonType::Menu:
        connect(w, &KDecoration3::DecoratedWindow::iconChanged, b, [b] {
            b->update();
        });
        break;
    default:
        break;
    }
    return b;
}

void Button::paint(QPainter *painter, const QRectF &repaintArea)
{
    Q_UNUSED(repaintArea)
    auto d = qobject_cast<Decoration *>(decoration());
    if (!d || type() == DecorationButtonType::Spacer) {
        return;
    }

    const QRectF g = geometry();
    const qreal side = std::min(g.width(), g.height());
    // the hover background is an inset rounded square, like DTK's title bar buttons
    const qreal inset = std::round(side * 0.16);
    const QRectF box(g.center().x() - side / 2 + inset, g.center().y() - side / 2 + inset, side - 2 * inset, side - 2 * inset);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    QColor fg = d->foregroundColor();
    const bool dark = d->isDarkTheme();
    const bool closeButton = type() == DecorationButtonType::Close;
    qreal hover = m_hover;
    if (isPressed()) {
        hover = 1.4;
    }
    if (hover > 0 && isEnabled()) {
        QColor bg;
        if (closeButton) {
            bg = QColor(0xff, 0x57, 0x36); // DTK TextWarning
            bg.setAlphaF(std::min(1.0, 0.9 * hover));
            fg = QColor::fromRgbF(fg.redF() + (1 - fg.redF()) * std::min(1.0, hover),
                                  fg.greenF() + (1 - fg.greenF()) * std::min(1.0, hover),
                                  fg.blueF() + (1 - fg.blueF()) * std::min(1.0, hover));
        } else {
            bg = dark ? QColor(255, 255, 255) : QColor(0, 0, 0);
            bg.setAlphaF((dark ? 0.12 : 0.08) * hover);
        }
        painter->setPen(Qt::NoPen);
        painter->setBrush(bg);
        painter->drawRoundedRect(box, 8, 8);
    }
    if (isChecked() && !closeButton) {
        QColor bg = dark ? QColor(255, 255, 255) : QColor(0, 0, 0);
        bg.setAlphaF(dark ? 0.10 : 0.06);
        painter->setPen(Qt::NoPen);
        painter->setBrush(bg);
        painter->drawRoundedRect(box, 8, 8);
    }
    if (!isEnabled()) {
        fg.setAlphaF(fg.alphaF() * 0.4);
    }

    if (type() == DecorationButtonType::Menu) {
        const qreal iconSize = std::min<qreal>(18, box.width() - 4);
        const QRectF iconRect(box.center().x() - iconSize / 2, box.center().y() - iconSize / 2, iconSize, iconSize);
        d->window()->icon().paint(painter, iconRect.toRect());
    } else {
        drawSymbol(painter, box, fg);
    }
    painter->restore();
}

void Button::drawSymbol(QPainter *painter, const QRectF &box, const QColor &color) const
{
    // symbols are drawn on a 10x10 grid centred in the button
    const qreal s = std::round(std::min<qreal>(10, box.width() * 0.4));
    const QPointF c = box.center();
    const QRectF r(std::round(c.x() - s / 2) + 0.5, std::round(c.y() - s / 2) + 0.5, s, s);

    QPen pen(color, 1.2);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);

    auto d = qobject_cast<Decoration *>(decoration());
    switch (type()) {
    case DecorationButtonType::Close:
        painter->drawLine(r.topLeft(), r.bottomRight());
        painter->drawLine(r.topRight(), r.bottomLeft());
        break;
    case DecorationButtonType::Maximize:
        if (d && d->window()->isMaximized()) {
            // restore: two overlapping rounded squares
            const qreal o = std::round(s * 0.25);
            painter->drawRoundedRect(QRectF(r.left(), r.top() + o, s - o, s - o), 1.5, 1.5);
            QPainterPath back;
            back.moveTo(r.left() + o, r.top() + o);
            back.lineTo(r.left() + o, r.top() + 1.5);
            back.quadTo(r.left() + o, r.top(), r.left() + o + 1.5, r.top());
            back.lineTo(r.right() - 1.5, r.top());
            back.quadTo(r.right(), r.top(), r.right(), r.top() + 1.5);
            back.lineTo(r.right(), r.bottom() - o - 1.5);
            back.quadTo(r.right(), r.bottom() - o, r.right() - 1.5, r.bottom() - o);
            back.lineTo(r.right() - o, r.bottom() - o);
            painter->drawPath(back);
        } else {
            painter->drawRoundedRect(r, 1.5, 1.5);
        }
        break;
    case DecorationButtonType::Minimize:
        painter->drawLine(QPointF(r.left(), c.y() + 0.5), QPointF(r.right(), c.y() + 0.5));
        break;
    case DecorationButtonType::ApplicationMenu:
        // Deepin "option" button: three horizontal lines
        painter->drawLine(QPointF(r.left(), r.top() + 1), QPointF(r.right(), r.top() + 1));
        painter->drawLine(QPointF(r.left(), c.y()), QPointF(r.right(), c.y()));
        painter->drawLine(QPointF(r.left(), r.bottom() - 1), QPointF(r.right(), r.bottom() - 1));
        break;
    case DecorationButtonType::OnAllDesktops:
        if (isChecked()) {
            painter->setBrush(color);
        }
        painter->drawEllipse(QRectF(c.x() - s * 0.3, c.y() - s * 0.3, s * 0.6, s * 0.6));
        break;
    case DecorationButtonType::KeepAbove: {
        QPolygonF chevron({QPointF(r.left(), c.y() + s * 0.2), QPointF(c.x(), c.y() - s * 0.3), QPointF(r.right(), c.y() + s * 0.2)});
        painter->drawPolyline(chevron);
        break;
    }
    case DecorationButtonType::KeepBelow: {
        QPolygonF chevron({QPointF(r.left(), c.y() - s * 0.2), QPointF(c.x(), c.y() + s * 0.3), QPointF(r.right(), c.y() - s * 0.2)});
        painter->drawPolyline(chevron);
        break;
    }
    case DecorationButtonType::Shade:
        painter->drawLine(QPointF(r.left(), r.top() + 1), QPointF(r.right(), r.top() + 1));
        if (isChecked()) {
            painter->drawPolyline(QPolygonF({QPointF(r.left() + 2, c.y() + 2), QPointF(c.x(), c.y() + s * 0.4), QPointF(r.right() - 2, c.y() + 2)}));
        } else {
            painter->drawPolyline(QPolygonF({QPointF(r.left() + 2, r.bottom()), QPointF(c.x(), c.y() + 1), QPointF(r.right() - 2, r.bottom())}));
        }
        break;
    case DecorationButtonType::ContextHelp: {
        QPainterPath q;
        q.moveTo(r.left() + s * 0.2, r.top() + s * 0.3);
        q.cubicTo(r.left() + s * 0.2, r.top() - s * 0.05, r.right() - s * 0.2, r.top() - s * 0.05, r.right() - s * 0.2, r.top() + s * 0.3);
        q.cubicTo(r.right() - s * 0.2, r.top() + s * 0.55, c.x(), c.y(), c.x(), c.y() + s * 0.2);
        painter->drawPath(q);
        painter->drawPoint(QPointF(c.x(), r.bottom()));
        break;
    }
    case DecorationButtonType::ExcludeFromCapture:
        painter->drawEllipse(r);
        if (isChecked()) {
            painter->drawLine(r.topLeft() + QPointF(1.5, 1.5), r.bottomRight() - QPointF(1.5, 1.5));
        }
        break;
    default:
        break;
    }
}

} // namespace DeepinGlass
