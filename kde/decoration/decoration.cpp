/*
 * SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * "Deepin Glass" window decoration: translucent, blurred title bar in the
 * style of the Deepin desktop, centred caption, flat buttons with rounded
 * hover backgrounds, rounded window corners and a large soft shadow.
 *
 * The overall structure (border calculation, button groups, shadow handling)
 * follows the Breeze decoration by Martin Gräßlin and Hugo Pereira Da Costa.
 */
#include "decoration.h"
#include "button.h"
#include "shadowrenderer.h"

#include <KDecoration3/DecoratedWindow>
#include <KDecoration3/DecorationSettings>
#include <KDecoration3/DecorationShadow>
#include <KDecoration3/ScaleHelpers>
#include <KPluginFactory>

#include <QDBusConnection>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>

#include <cmath>

K_PLUGIN_FACTORY_WITH_JSON(DeepinGlassDecorationFactory,
                           "deepinglass.json",
                           registerPlugin<DeepinGlass::Decoration>();
                           registerPlugin<DeepinGlass::Button>();)

namespace DeepinGlass
{
using KDecoration3::ColorGroup;
using KDecoration3::ColorRole;

namespace
{
struct ShadowKey {
    int size = -1;
    qreal strength = -1;
    qreal radius = -1;
    bool operator==(const ShadowKey &) const = default;
};

int g_decorationCount = 0;
ShadowKey g_shadowKey;
std::shared_ptr<KDecoration3::DecorationShadow> g_activeShadow;
std::shared_ptr<KDecoration3::DecorationShadow> g_inactiveShadow;

std::shared_ptr<KDecoration3::DecorationShadow> createShadow(int size, qreal strength, qreal radius)
{
    if (size <= 0 || strength <= 0) {
        return nullptr;
    }
    const int side = int(std::ceil(2 * radius)) + 2 * size + 2;
    const QSize box(side, side);
    const QPoint offset(0, std::max(1, size / 6));
    QImage image = renderShadow(box, radius, size, offset, strength);

    const int pad = size * 2;
    auto shadow = std::make_shared<KDecoration3::DecorationShadow>();
    shadow->setPadding(QMarginsF(pad, pad, pad, pad));
    shadow->setInnerShadowRect(QRectF(image.width() / 2.0, image.height() / 2.0, 1, 1));
    shadow->setShadow(image);
    return shadow;
}
} // namespace

Decoration::Decoration(QObject *parent, const QVariantList &args)
    : KDecoration3::Decoration(parent, args)
    , m_activeAnimation(new QVariantAnimation(this))
{
    ++g_decorationCount;
}

Decoration::~Decoration()
{
    if (--g_decorationCount == 0) {
        g_activeShadow.reset();
        g_inactiveShadow.reset();
        g_shadowKey = ShadowKey();
    }
}

bool Decoration::init()
{
    m_activeAnimation->setStartValue(0.0);
    m_activeAnimation->setEndValue(1.0);
    m_activeAnimation->setDuration(150);
    m_activeAnimation->setEasingCurve(QEasingCurve::InOutQuad);
    connect(m_activeAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_activeProgress = value.toReal();
        update();
    });
    m_activeProgress = window()->isActive() ? 1.0 : 0.0;

    // configuration changes: "kwin reconfigure" and KGlobalSettings notifications
    QDBusConnection::sessionBus().connect(QString(),
                                          QStringLiteral("/KGlobalSettings"),
                                          QStringLiteral("org.kde.KGlobalSettings"),
                                          QStringLiteral("notifyChange"),
                                          this,
                                          SLOT(reconfigure()));

    auto s = settings();
    connect(s.get(), &KDecoration3::DecorationSettings::reconfigured, this, &Decoration::reconfigure);
    connect(s.get(), &KDecoration3::DecorationSettings::borderSizeChanged, this, &Decoration::recalculateBorders);
    connect(s.get(), &KDecoration3::DecorationSettings::fontChanged, this, &Decoration::recalculateBorders);
    connect(s.get(), &KDecoration3::DecorationSettings::decorationButtonsLeftChanged, this, [this] {
        QTimer::singleShot(0, this, &Decoration::updateButtonsGeometry);
    });
    connect(s.get(), &KDecoration3::DecorationSettings::decorationButtonsRightChanged, this, [this] {
        QTimer::singleShot(0, this, &Decoration::updateButtonsGeometry);
    });

    auto w = window();
    connect(w, &KDecoration3::DecoratedWindow::activeChanged, this, [this](bool active) {
        m_activeAnimation->setDirection(active ? QAbstractAnimation::Forward : QAbstractAnimation::Backward);
        if (m_activeAnimation->state() != QAbstractAnimation::Running) {
            m_activeAnimation->start();
        }
        updateShadow();
        recalculateBorders();
    });
    connect(w, &KDecoration3::DecoratedWindow::captionChanged, this, [this] {
        update(titleBar());
    });
    connect(w, &KDecoration3::DecoratedWindow::paletteChanged, this, [this] {
        recalculateBorders();
        update();
    });
    for (auto signal : {&KDecoration3::DecoratedWindow::maximizedChanged,
                        &KDecoration3::DecoratedWindow::maximizedHorizontallyChanged,
                        &KDecoration3::DecoratedWindow::maximizedVerticallyChanged,
                        &KDecoration3::DecoratedWindow::shadedChanged}) {
        connect(w, signal, this, &Decoration::recalculateBorders);
    }
    connect(w, &KDecoration3::DecoratedWindow::adjacentScreenEdgesChanged, this, &Decoration::recalculateBorders);
    connect(w, &KDecoration3::DecoratedWindow::widthChanged, this, &Decoration::updateTitleBar);
    connect(w, &KDecoration3::DecoratedWindow::widthChanged, this, &Decoration::updateButtonsGeometry);
    connect(w, &KDecoration3::DecoratedWindow::sizeChanged, this, &Decoration::updateBlur);
    connect(w, &KDecoration3::DecoratedWindow::nextScaleChanged, this, &Decoration::recalculateBorders);
    connect(this, &KDecoration3::Decoration::bordersChanged, this, &Decoration::updateTitleBar);
    connect(this, &KDecoration3::Decoration::bordersChanged, this, &Decoration::updateButtonsGeometry);
    connect(this, &KDecoration3::Decoration::bordersChanged, this, &Decoration::updateBlur);

    m_leftButtons = new KDecoration3::DecorationButtonGroup(KDecoration3::DecorationButtonGroup::Position::Left, this, &Button::create);
    m_rightButtons = new KDecoration3::DecorationButtonGroup(KDecoration3::DecorationButtonGroup::Position::Right, this, &Button::create);

    reconfigure();
    return true;
}

void Decoration::reconfigure()
{
    m_config = DecorationConfig::load();
    recalculateBorders();
    updateButtonsGeometry();
    updateShadow();
    update();
}

bool Decoration::isMaximized() const
{
    return window()->isMaximized();
}

bool Decoration::isTiled() const
{
    return window()->adjacentScreenEdges() != Qt::Edges();
}

bool Decoration::drawRoundedCorners() const
{
    return settings()->isAlphaChannelSupported() && !isMaximized() && !isTiled() && m_config.cornerRadius > 0;
}

qreal Decoration::cornerRadius() const
{
    return drawRoundedCorners() ? KDecoration3::snapToPixelGrid(m_config.cornerRadius, window()->nextScale()) : 0.0;
}

qreal Decoration::sideBorder() const
{
    switch (settings()->borderSize()) {
    case KDecoration3::BorderSize::None:
    case KDecoration3::BorderSize::NoSides:
        return 0;
    case KDecoration3::BorderSize::Tiny:
        return 2;
    case KDecoration3::BorderSize::Normal:
        return 4;
    case KDecoration3::BorderSize::Large:
        return 6;
    case KDecoration3::BorderSize::VeryLarge:
        return 8;
    case KDecoration3::BorderSize::Huge:
        return 10;
    case KDecoration3::BorderSize::VeryHuge:
        return 12;
    case KDecoration3::BorderSize::Oversized:
        return 20;
    }
    return 0;
}

qreal Decoration::bottomBorder() const
{
    if (settings()->borderSize() == KDecoration3::BorderSize::NoSides) {
        return 4;
    }
    return sideBorder();
}

qreal Decoration::buttonSize() const
{
    return borderTop();
}

void Decoration::recalculateBorders()
{
    const qreal scale = window()->nextScale();
    const auto edges = window()->adjacentScreenEdges();
    const bool maximized = isMaximized();

    const qreal side = maximized ? 0 : KDecoration3::snapToPixelGrid(sideBorder(), scale);
    const qreal bottom = (maximized || window()->isShaded()) ? 0 : KDecoration3::snapToPixelGrid(bottomBorder(), scale);
    const QFontMetricsF fm(settings()->font());
    const qreal top = KDecoration3::snapToPixelGrid(std::max<qreal>(m_config.titleBarHeight, fm.height() + 12), scale);

    setBorders(QMarginsF((edges & Qt::LeftEdge) ? 0 : side, top, (edges & Qt::RightEdge) ? 0 : side, (edges & Qt::BottomEdge) ? 0 : bottom));

    // invisible resize area outside the window
    const qreal ext = maximized ? 0 : KDecoration3::snapToPixelGrid(std::max(4, settings()->largeSpacing()), scale);
    setResizeOnlyBorders(QMarginsF(side > 0 ? 0 : ext, ext, side > 0 ? 0 : ext, bottom > 0 ? 0 : ext));

    // rounded corners: the client area is clipped by KWin at the bottom corners
    const qreal r = cornerRadius();
    if (r > 0) {
        setBorderRadius(KDecoration3::BorderRadius(r, r, r, r));
    } else {
        setBorderRadius(KDecoration3::BorderRadius());
    }

    if (m_config.outline && !maximized && settings()->isAlphaChannelSupported()) {
        QColor color = isDarkTheme() ? QColor(255, 255, 255) : QColor(0, 0, 0);
        color.setAlphaF(isDarkTheme() ? 0.12 : (window()->isActive() ? 0.14 : 0.10));
        const qreal thickness = std::max(KDecoration3::pixelSize(scale), KDecoration3::snapToPixelGrid(1, scale));
        setBorderOutline(KDecoration3::BorderOutline(thickness, color, KDecoration3::BorderRadius(r, r, r, r)));
    } else {
        setBorderOutline(KDecoration3::BorderOutline());
    }

    setOpaque(!settings()->isAlphaChannelSupported());
    updateShadow();
    updateBlur();
    update();
}

void Decoration::updateTitleBar()
{
    setTitleBar(QRectF(0, 0, size().width(), borderTop()));
}

void Decoration::updateButtonsGeometry()
{
    if (!m_leftButtons || !m_rightButtons) {
        return;
    }
    const qreal bs = buttonSize();
    const auto buttons = m_leftButtons->buttons() + m_rightButtons->buttons();
    for (KDecoration3::DecorationButton *button : buttons) {
        button->setGeometry(QRectF(0, 0, bs, bs));
    }
    const qreal margin = isMaximized() ? 0 : std::max<qreal>(2, cornerRadius() / 3);
    m_leftButtons->setSpacing(0);
    m_rightButtons->setSpacing(0);
    m_leftButtons->setPos(QPointF(borderLeft() + margin, 0));
    m_rightButtons->setPos(QPointF(size().width() - borderRight() - margin - m_rightButtons->geometry().width(), 0));
    update();
}

void Decoration::updateBlur()
{
    if (!m_config.blur || !settings()->isAlphaChannelSupported()) {
        setBlurRegion(QRegion());
        return;
    }
    // Region covering the whole decoration (title bar and borders), with rounded top corners.
    const QRect full = rect().toAlignedRect();
    const int r = int(std::round(cornerRadius()));
    QRegion region(full);
    region -= QRect(full.x(), full.y() + int(borderTop()), full.width(), full.height() - int(borderTop()) - int(borderBottom()))
                  .adjusted(int(borderLeft()), 0, -int(borderRight()), 0);
    if (r > 0) {
        // carve the top corners out, one scanline-ish step per pixel row
        for (int y = 0; y < r; ++y) {
            const qreal dy = r - y - 0.5;
            const int inset = int(std::round(r - std::sqrt(std::max<qreal>(0, r * r - dy * dy))));
            if (inset > 0) {
                region -= QRect(full.left(), full.top() + y, inset, 1);
                region -= QRect(full.right() - inset + 1, full.top() + y, inset, 1);
            }
        }
        if (borderBottom() > 0) {
            for (int y = 0; y < r; ++y) {
                const qreal dy = r - y - 0.5;
                const int inset = int(std::round(r - std::sqrt(std::max<qreal>(0, r * r - dy * dy))));
                if (inset > 0) {
                    region -= QRect(full.left(), full.bottom() - y, inset, 1);
                    region -= QRect(full.right() - inset + 1, full.bottom() - y, inset, 1);
                }
            }
        }
    }
    setBlurRegion(region);
}

void Decoration::updateShadow()
{
    const ShadowKey key{m_config.shadowSize, m_config.shadowStrength, qreal(m_config.cornerRadius)};
    if (!(key == g_shadowKey)) {
        g_shadowKey = key;
        g_activeShadow = createShadow(m_config.shadowSize, m_config.shadowStrength, m_config.cornerRadius);
        g_inactiveShadow = createShadow(int(m_config.shadowSize * 0.75), m_config.shadowStrength * 0.55, m_config.cornerRadius);
    }
    setShadow(window()->isActive() ? g_activeShadow : g_inactiveShadow);
}

QColor Decoration::titleBarColor() const
{
    const QColor active = window()->color(ColorGroup::Active, ColorRole::TitleBar);
    const QColor inactive = window()->color(ColorGroup::Inactive, ColorRole::TitleBar);
    const qreal t = m_activeProgress;
    return QColor::fromRgbF(inactive.redF() + (active.redF() - inactive.redF()) * t,
                            inactive.greenF() + (active.greenF() - inactive.greenF()) * t,
                            inactive.blueF() + (active.blueF() - inactive.blueF()) * t);
}

QColor Decoration::foregroundColor() const
{
    const QColor active = window()->color(ColorGroup::Active, ColorRole::Foreground);
    const QColor inactive = window()->color(ColorGroup::Inactive, ColorRole::Foreground);
    const qreal t = m_activeProgress;
    return QColor::fromRgbF(inactive.redF() + (active.redF() - inactive.redF()) * t,
                            inactive.greenF() + (active.greenF() - inactive.greenF()) * t,
                            inactive.blueF() + (active.blueF() - inactive.blueF()) * t);
}

bool Decoration::isDarkTheme() const
{
    return qGray(window()->color(ColorGroup::Active, ColorRole::TitleBar).rgb()) < 128;
}

qreal Decoration::titleBarOpacity() const
{
    if (!settings()->isAlphaChannelSupported()) {
        return 1.0;
    }
    return m_config.inactiveOpacity + (m_config.activeOpacity - m_config.inactiveOpacity) * m_activeProgress;
}

void Decoration::paint(QPainter *painter, const QRectF &repaintArea)
{
    const QRectF r = rect();
    const qreal radius = cornerRadius();
    QColor bg = titleBarColor();
    bg.setAlphaF(titleBarOpacity());

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(Qt::NoPen);
    painter->setBrush(bg);

    // everything that is decoration (title bar + borders), the client area is left out
    QPainterPath outer;
    if (radius > 0) {
        outer.addRoundedRect(r, radius, radius);
    } else {
        outer.addRect(r);
    }
    QPainterPath client;
    client.addRect(QRectF(borderLeft(), borderTop(), r.width() - borderLeft() - borderRight(), r.height() - borderTop() - borderBottom()));
    if (window()->isShaded()) {
        painter->drawPath(outer);
    } else {
        painter->drawPath(outer.subtracted(client));
    }

    // glass highlight along the top edge
    if (settings()->isAlphaChannelSupported() && !isMaximized()) {
        QColor highlight(255, 255, 255);
        highlight.setAlphaF(isDarkTheme() ? 0.06 : 0.35);
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(highlight, 1));
        QPainterPath top;
        const qreal y = 1.5;
        top.moveTo(r.left() + radius, y);
        top.lineTo(r.right() - radius, y);
        painter->drawPath(top);
    }
    painter->restore();

    paintTitleBar(painter, repaintArea);
}

QRectF Decoration::captionRect() const
{
    const qreal leftEdge = m_leftButtons->buttons().isEmpty() ? borderLeft() + 12 : m_leftButtons->geometry().right() + 8;
    const qreal rightEdge = m_rightButtons->buttons().isEmpty() ? size().width() - borderRight() - 12 : m_rightButtons->geometry().left() - 8;
    return QRectF(leftEdge, 0, std::max<qreal>(0, rightEdge - leftEdge), borderTop());
}

void Decoration::paintTitleBar(QPainter *painter, const QRectF &repaintArea)
{
    const QRectF bar(0, 0, size().width(), borderTop());
    if (!bar.intersects(repaintArea)) {
        return;
    }

    // caption
    painter->save();
    painter->setFont(settings()->font());
    painter->setPen(foregroundColor());
    const QRectF available = captionRect();
    const QFontMetricsF fm(settings()->font());
    const QString caption = fm.elidedText(window()->caption(), Qt::ElideMiddle, available.width());
    const qreal textWidth = fm.horizontalAdvance(caption);
    QRectF textRect = available;
    Qt::Alignment align = Qt::AlignVCenter | Qt::AlignLeft;
    if (m_config.centerTitle) {
        // centre on the whole window if possible, otherwise inside the free space
        const qreal centeredLeft = (size().width() - textWidth) / 2.0;
        if (centeredLeft >= available.left() && centeredLeft + textWidth <= available.right()) {
            textRect = QRectF(centeredLeft, 0, textWidth + 1, borderTop());
        } else {
            align = Qt::AlignCenter;
        }
    }
    painter->drawText(textRect, align | Qt::TextSingleLine, caption);
    painter->restore();

    m_leftButtons->paint(painter, repaintArea);
    m_rightButtons->paint(painter, repaintArea);
}

} // namespace DeepinGlass

#include "decoration.moc"
