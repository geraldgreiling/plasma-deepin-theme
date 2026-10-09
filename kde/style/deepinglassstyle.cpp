/*
 * SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * The technique of enabling WA_TranslucentBackground from styleHint() - i.e.
 * before Qt creates the native window - follows the approach documented in
 * Kvantum by Pedram Pourang (GPL-3.0-or-later).
 */
#include "deepinglassstyle.h"

#include <KWindowEffects>
#include <KWindowSystem>
#if __has_include(<KX11Extras>)
#include <KX11Extras>
#define DEEPINGLASS_HAVE_X11EXTRAS 1
#endif

#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QFileInfo>
#include <QLineEdit>
#include <QMainWindow>
#include <QMdiSubWindow>
#include <QMenu>
#include <QMenuBar>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QStyleFactory>
#include <QStyleOption>
#include <QToolBar>
#include <QWindow>

#include <cmath>

namespace DeepinGlass
{
namespace
{
constexpr qreal ControlRadius = 8.0; // DTK PM_FrameRadius (normal size mode)
constexpr qreal MenuRadius = 5.0; // matches the Breeze menu shadow
constexpr int FocusWidth = 2; // DTK PM_FocusBorderWidth

QColor mix(const QColor &a, const QColor &b, qreal t)
{
    return QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * t,
                            a.greenF() + (b.greenF() - a.greenF()) * t,
                            a.blueF() + (b.blueF() - a.blueF()) * t,
                            a.alphaF() + (b.alphaF() - a.alphaF()) * t);
}

QColor withAlpha(QColor c, qreal alpha)
{
    c.setAlphaF(std::clamp(alpha, 0.0, 1.0));
    return c;
}

bool isDark(const QPalette &palette)
{
    return qGray(palette.color(QPalette::Window).rgb()) < 128;
}

QRegion roundedRegion(const QRect &rect, int radius)
{
    QRegion region(rect);
    if (radius <= 0) {
        return region;
    }
    for (int y = 0; y < radius; ++y) {
        const qreal dy = radius - y - 0.5;
        const int inset = int(std::round(radius - std::sqrt(std::max<qreal>(0, radius * radius - dy * dy))));
        if (inset <= 0) {
            continue;
        }
        region -= QRect(rect.left(), rect.top() + y, inset, 1);
        region -= QRect(rect.right() - inset + 1, rect.top() + y, inset, 1);
        region -= QRect(rect.left(), rect.bottom() - y, inset, 1);
        region -= QRect(rect.right() - inset + 1, rect.bottom() - y, inset, 1);
    }
    return region;
}

// Applications that render their main content with OpenGL/Vulkan, video overlays
// or QtQuick (QQuickWidget) inside a widget window and must stay opaque.
// Extendable with [Style] ExcludedApplications.
const QStringList s_builtinExcluded = {
    QStringLiteral("plasmashell"), QStringLiteral("kwin_wayland"), QStringLiteral("kwin_x11"), QStringLiteral("krunner"),
    QStringLiteral("ksplashqml"), QStringLiteral("kscreenlocker_greet"), QStringLiteral("sddm-greeter"), QStringLiteral("sddm-greeter-qt6"),
    QStringLiteral("krita"), QStringLiteral("kdenlive"), QStringLiteral("vlc"), QStringLiteral("smplayer"), QStringLiteral("obs"),
    QStringLiteral("VirtualBox"), QStringLiteral("VirtualBoxVM"), QStringLiteral("virt-manager"), QStringLiteral("steam"),
    QStringLiteral("haruna"), QStringLiteral("mpv"), QStringLiteral("dragon"), QStringLiteral("freecad"), QStringLiteral("FreeCAD"),
    QStringLiteral("blender"), QStringLiteral("qtcreator"), QStringLiteral("systemsettings"), QStringLiteral("kinfocenter"), QStringLiteral("wireshark"), QStringLiteral("libreoffice"), QStringLiteral("soffice.bin"),
};
} // namespace

Style::Style()
    : QProxyStyle(QStyleFactory::create(QStringLiteral("Breeze")))
{
    setObjectName(QStringLiteral("DeepinGlass"));
    m_config = StyleConfig::load();
    m_decoConfig = DecorationConfig::load();
    const QString app = QFileInfo(QCoreApplication::applicationFilePath()).fileName();
    m_excludedApplication = !m_config.translucent || s_builtinExcluded.contains(app) || m_config.excluded.contains(app)
        || QCoreApplication::applicationName() == QLatin1String("plasmashell");
}

Style::~Style() = default;

bool Style::compositingActive() const
{
    if (KWindowSystem::isPlatformWayland()) {
        return true;
    }
#ifdef DEEPINGLASS_HAVE_X11EXTRAS
    if (KWindowSystem::isPlatformX11()) {
        return KX11Extras::compositingActive();
    }
#endif
    return false;
}

void Style::polish(QApplication *app)
{
    QProxyStyle::polish(app);
}

void Style::polish(QPalette &palette)
{
    QProxyStyle::polish(palette);
    if (m_excludedApplication || !compositingActive() || m_config.viewOpacity >= 1.0) {
        return;
    }
    // Content areas (file views, lists, text fields) are painted with the Base colour by
    // the applications themselves, often outside of the style (e.g. Dolphin's view is a
    // QGraphicsView that uses the application palette). A translucent Base colour is
    // the only way to let the glass shine through there too.
    for (auto group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
        palette.setColor(group, QPalette::Base, withAlpha(palette.color(group, QPalette::Base), m_config.viewOpacity));
        palette.setColor(group, QPalette::AlternateBase, withAlpha(palette.color(group, QPalette::AlternateBase), m_config.viewOpacity));
    }
}

//____________________________________________________________________________
// Translucent windows

void Style::tryMakeTranslucent(const QWidget *constWidget) const
{
    if (!constWidget || m_excludedApplication || !constWidget->isWindow()) {
        return;
    }
    if (m_translucentWindows.contains(constWidget)) {
        return;
    }
    QWidget *widget = const_cast<QWidget *>(constWidget);
    // too late: the native window exists already, or translucency is set up by the application itself
    if (widget->testAttribute(Qt::WA_WState_Created) || widget->windowHandle() || widget->testAttribute(Qt::WA_TranslucentBackground)
        || widget->testAttribute(Qt::WA_NoSystemBackground) || widget->testAttribute(Qt::WA_PaintOnScreen) || widget->autoFillBackground()) {
        return;
    }
    switch (widget->windowType()) {
    case Qt::Window:
    case Qt::Dialog:
        break;
    default:
        return;
    }
    if (widget->windowFlags().testFlag(Qt::FramelessWindowHint) || widget->windowFlags().testFlag(Qt::X11BypassWindowManagerHint)) {
        return;
    }
    // only real application windows; floating frames, splash screens etc. stay as they are
    if (!(qobject_cast<QMainWindow *>(widget) || qobject_cast<QDialog *>(widget))) {
        return;
    }
    if (widget->inherits("QSplashScreen") || widget->inherits("KScreenSaver")) {
        return;
    }
    if (QWidget *p = widget->parentWidget(); p && qobject_cast<QMdiSubWindow *>(p)) {
        return;
    }
    if (auto mw = qobject_cast<QMainWindow *>(widget)) {
        if (mw->parentWidget()) {
            return; // embedded main window
        }
        if (mw->styleSheet().contains(QLatin1String("background"))) {
            return;
        }
        if (QWidget *cw = mw->centralWidget(); cw && (cw->autoFillBackground() || cw->styleSheet().contains(QLatin1String("background")))) {
            return;
        }
    }
    if (!compositingActive()) {
        return;
    }

    // Before the native window is created this also gives the window an alpha channel.
    widget->setAttribute(Qt::WA_TranslucentBackground);
    m_translucentWindows.insert(widget);
    widget->installEventFilter(const_cast<Style *>(this));
    connect(widget, &QObject::destroyed, this, [this, widget] {
        m_translucentWindows.remove(widget);
    });
}

bool Style::isTranslucentWindow(const QWidget *widget) const
{
    return widget && m_translucentWindows.contains(widget);
}

void Style::updateBlur(QWidget *widget) const
{
    if (!widget || !widget->windowHandle()) {
        return;
    }
    if (auto menu = qobject_cast<QMenu *>(widget)) {
        KWindowEffects::enableBlurBehind(widget->windowHandle(), true, roundedRegion(menu->rect(), int(MenuRadius)));
        return;
    }
    KWindowEffects::enableBlurBehind(widget->windowHandle(), true, QRegion());
}

QRect Style::toolsAreaRect(const QWidget *window) const
{
    auto mw = qobject_cast<const QMainWindow *>(window);
    if (!mw) {
        return QRect();
    }
    int bottom = 0;
    if (QWidget *menu = mw->menuWidget(); menu && menu->isVisible()) {
        bottom = std::max(bottom, menu->geometry().bottom() + 1);
    }
    const auto toolBars = mw->findChildren<QToolBar *>(QString(), Qt::FindDirectChildrenOnly);
    for (QToolBar *tb : toolBars) {
        if (tb->isVisible() && !tb->isFloating() && mw->toolBarArea(tb) == Qt::TopToolBarArea) {
            bottom = std::max(bottom, tb->geometry().bottom() + 1);
        }
    }
    return bottom > 0 ? QRect(0, 0, mw->width(), bottom) : QRect();
}

void Style::paintWindowBackground(QWidget *window, QPaintEvent *event) const
{
    QPainter painter(window);
    painter.setClipRegion(event->region());
    const QPalette &pal = window->palette();
    const auto group = window->isActiveWindow() ? QPalette::Active : QPalette::Inactive;

    // One glass surface for the whole window with the opacity of the title bar
    // (the colour schemes use the window colour for the title bar as well).
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(window->rect(), withAlpha(pal.color(group, QPalette::Window), windowOpacity(window->isActiveWindow())));
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    // a hairline below the tool bars of KDE applications
    const QRect tools = toolsAreaRect(window);
    if (tools.isValid()) {
        painter.setPen(withAlpha(pal.color(QPalette::WindowText), 0.06));
        painter.drawLine(tools.bottomLeft(), tools.bottomRight());
    }
}

qreal Style::windowOpacity(bool active) const
{
    if (m_config.windowOpacity >= 0) {
        return m_config.windowOpacity;
    }
    return active ? m_decoConfig.activeOpacity : m_decoConfig.inactiveOpacity;
}

void Style::makeContentTranslucent(QWidget *widget) const
{
    if (m_config.viewOpacity >= 1.0) {
        return;
    }
    // only touch opaque content colours, so this neither stacks up nor fights with
    // colours that are translucent on purpose (side panels, inactive split views)
    QPalette pal = widget->palette();
    bool changed = false;
    for (auto group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
        for (auto role : {QPalette::Base, QPalette::AlternateBase}) {
            const QColor c = pal.color(group, role);
            if (c.alpha() == 255) {
                pal.setColor(group, role, withAlpha(c, m_config.viewOpacity));
                changed = true;
            }
        }
    }
    if (changed) {
        widget->setPalette(pal);
    }
}

bool Style::eventFilter(QObject *object, QEvent *event)
{
    auto widget = qobject_cast<QWidget *>(object);
    if (!widget) {
        return QProxyStyle::eventFilter(object, event);
    }
    switch (event->type()) {
    case QEvent::PaletteChange:
        if (qobject_cast<QAbstractScrollArea *>(widget) && isTranslucentWindow(widget->window())) {
            makeContentTranslucent(widget);
        }
        break;
    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate:
        // active and inactive windows have different opacities, like the title bar
        if (isTranslucentWindow(widget)) {
            widget->update();
        }
        break;
    case QEvent::Paint:
        if (isTranslucentWindow(widget)) {
            paintWindowBackground(widget, static_cast<QPaintEvent *>(event));
        }
        break;
    case QEvent::Show:
    case QEvent::Resize:
        if (isTranslucentWindow(widget) || (qobject_cast<QMenu *>(widget) && widget->testAttribute(Qt::WA_TranslucentBackground))) {
            updateBlur(widget);
        }
        break;
    default:
        break;
    }
    return QProxyStyle::eventFilter(object, event);
}

void Style::polish(QWidget *widget)
{
    if (!widget) {
        return;
    }
    tryMakeTranslucent(widget);
    QProxyStyle::polish(widget);

    if (m_excludedApplication || !compositingActive()) {
        return;
    }

    if (auto menu = qobject_cast<QMenu *>(widget)) {
        if (menu->testAttribute(Qt::WA_TranslucentBackground) && m_config.menuOpacity < 1.0) {
            menu->removeEventFilter(this);
            menu->installEventFilter(this);
        }
        return;
    }

    // side panels (Dolphin places, settings sidebars): let the window glass shine through
    if (auto area = qobject_cast<QAbstractScrollArea *>(widget)) {
        const bool sidePanel = area->property("_kde_side_panel_view").toBool() || area->inherits("KFilePlacesView");
        if (!sidePanel && isTranslucentWindow(area->window())) {
            // content views: some applications set an opaque palette of their own
            // (Dolphin takes the view colour straight from KColorScheme), so keep watching
            makeContentTranslucent(area);
            area->removeEventFilter(this);
            area->installEventFilter(this);
        }
        if (sidePanel && area->viewport() && isTranslucentWindow(area->window())) {
            QPalette pal = area->palette();
            for (auto group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
                pal.setColor(group, QPalette::Base, withAlpha(pal.color(group, QPalette::Window), m_config.sidebarOpacity));
            }
            area->setPalette(pal);
            area->viewport()->setAutoFillBackground(m_config.sidebarOpacity > 0);
        }
    }
}

void Style::unpolish(QWidget *widget)
{
    if (widget && (isTranslucentWindow(widget) || qobject_cast<QMenu *>(widget))) {
        widget->removeEventFilter(this);
    }
    QProxyStyle::unpolish(widget);
}

int Style::styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget, QStyleHintReturn *returnData) const
{
    // styleHint() is queried while widgets are constructed, i.e. before their native
    // window exists. That is the only point where WA_TranslucentBackground still works.
    tryMakeTranslucent(widget);
    return QProxyStyle::styleHint(hint, option, widget, returnData);
}

int Style::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    switch (metric) {
    case PM_ScrollBarExtent:
        return 10;
    default:
        break;
    }
    return QProxyStyle::pixelMetric(metric, option, widget);
}

//____________________________________________________________________________
// Painting helpers

void Style::drawButtonPanel(const QStyleOption *option, QPainter *painter, bool isDefault, bool flat) const
{
    const QPalette &pal = option->palette;
    const bool enabled = option->state & State_Enabled;
    const bool hover = enabled && (option->state & State_MouseOver);
    const bool sunken = option->state & (State_Sunken | State_On);
    const bool focus = enabled && (option->state & State_HasFocus);
    const bool dark = isDark(pal);

    QRectF r = QRectF(option->rect).adjusted(1, 1, -1, -1);
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(Qt::NoPen);

    QColor bg;
    if (isDefault && enabled) {
        bg = pal.color(QPalette::Highlight);
        if (sunken) {
            bg = bg.darker(115);
        } else if (hover) {
            bg = bg.lighter(110);
        }
    } else {
        if (flat && !hover && !sunken) {
            painter->restore();
            return;
        }
        bg = pal.color(QPalette::Button);
        const QColor fg = pal.color(QPalette::ButtonText);
        if (sunken) {
            bg = mix(bg, fg, 0.15);
        } else if (hover) {
            bg = mix(bg, fg, dark ? 0.08 : 0.05);
        }
    }

    // DTK buttons have a slight vertical gradient and a 1px drop shadow
    if (!dark && !sunken) {
        painter->setBrush(QColor(0, 0, 0, 18));
        painter->drawRoundedRect(r.translated(0, 1), ControlRadius, ControlRadius);
    }
    QLinearGradient gradient(r.topLeft(), r.bottomLeft());
    gradient.setColorAt(0, bg.lighter(dark ? 106 : 102));
    gradient.setColorAt(1, bg);
    painter->setBrush(gradient);
    painter->drawRoundedRect(r, ControlRadius, ControlRadius);

    if (focus) {
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(pal.color(QPalette::Highlight), FocusWidth));
        painter->drawRoundedRect(r.adjusted(0.5, 0.5, -0.5, -0.5), ControlRadius, ControlRadius);
        if (isDefault) {
            painter->setPen(QPen(pal.color(QPalette::HighlightedText), 1));
            painter->drawRoundedRect(r.adjusted(2.5, 2.5, -2.5, -2.5), ControlRadius - 2, ControlRadius - 2);
        }
    }
    painter->restore();
}

void Style::drawInputPanel(const QStyleOption *option, QPainter *painter, const QRect &rect) const
{
    const QPalette &pal = option->palette;
    const bool enabled = option->state & State_Enabled;
    const bool focus = enabled && (option->state & State_HasFocus);
    const bool hover = enabled && (option->state & State_MouseOver);
    const bool dark = isDark(pal);

    QRectF r = QRectF(rect).adjusted(1, 1, -1, -1);
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(Qt::NoPen);
    // DTK: line edits are filled with the "ItemBackground"-like tint, no outline
    QColor bg;
    if (pal.color(QPalette::Base).alpha() < 255) {
        // glass: a light tint of the text colour on top of the window surface
        bg = withAlpha(pal.color(QPalette::Text), (dark ? 0.08 : 0.05) + (hover && !focus ? 0.03 : 0.0));
    } else {
        bg = mix(pal.color(QPalette::Base), pal.color(QPalette::Text), dark ? 0.06 : 0.04);
        if (hover && !focus) {
            bg = mix(bg, pal.color(QPalette::Text), 0.03);
        }
    }
    if (!enabled) {
        bg = withAlpha(bg, 0.6);
    }
    painter->setBrush(bg);
    painter->drawRoundedRect(r, ControlRadius, ControlRadius);
    if (focus) {
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(pal.color(QPalette::Highlight), FocusWidth));
        painter->drawRoundedRect(r.adjusted(1, 1, -1, -1), ControlRadius - 1, ControlRadius - 1);
    } else {
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(withAlpha(pal.color(QPalette::Text), dark ? 0.10 : 0.06), 1));
        painter->drawRoundedRect(r.adjusted(0.5, 0.5, -0.5, -0.5), ControlRadius, ControlRadius);
    }
    painter->restore();
}

void Style::drawCheckIndicator(const QStyleOption *option, QPainter *painter, bool radio) const
{
    const QPalette &pal = option->palette;
    const bool enabled = option->state & State_Enabled;
    const bool on = option->state & State_On;
    const bool partial = option->state & State_NoChange;
    const bool hover = enabled && (option->state & State_MouseOver);
    const bool dark = isDark(pal);

    const int side = std::min(option->rect.width(), option->rect.height()) - 2;
    QRectF r(0, 0, side, side);
    r.moveCenter(QRectF(option->rect).center());

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    const QColor accent = enabled ? pal.color(QPalette::Highlight) : mix(pal.color(QPalette::Highlight), pal.color(QPalette::Window), 0.5);
    const qreal radius = radio ? side / 2.0 : 4.0;

    if (on || partial) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(hover ? accent.lighter(110) : accent);
        painter->drawRoundedRect(r, radius, radius);
        QPen pen(pal.color(QPalette::HighlightedText), radio ? 0 : 1.6);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        if (radio) {
            painter->setBrush(pal.color(QPalette::HighlightedText));
            const qreal dot = side * 0.38;
            painter->drawEllipse(QRectF(r.center().x() - dot / 2, r.center().y() - dot / 2, dot, dot));
        } else if (partial) {
            painter->setPen(pen);
            painter->drawLine(QPointF(r.left() + side * 0.28, r.center().y()), QPointF(r.right() - side * 0.28, r.center().y()));
        } else {
            painter->setPen(pen);
            painter->setBrush(Qt::NoBrush);
            QPainterPath check;
            check.moveTo(r.left() + side * 0.24, r.top() + side * 0.52);
            check.lineTo(r.left() + side * 0.43, r.top() + side * 0.70);
            check.lineTo(r.left() + side * 0.77, r.top() + side * 0.32);
            painter->drawPath(check);
        }
    } else {
        painter->setBrush(withAlpha(pal.color(QPalette::Base), dark ? 0.15 : 0.9));
        QColor border = hover ? accent : withAlpha(pal.color(QPalette::Text), enabled ? 0.45 : 0.2);
        painter->setPen(QPen(border, 1.2));
        painter->drawRoundedRect(r.adjusted(0.6, 0.6, -0.6, -0.6), radius, radius);
    }
    painter->restore();
}

//____________________________________________________________________________
// Primitives / controls

void Style::drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    switch (element) {
    case PE_PanelMenu:
        if (widget && widget->testAttribute(Qt::WA_TranslucentBackground) && !m_excludedApplication && compositingActive()) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(QPen(withAlpha(option->palette.color(QPalette::WindowText), isDark(option->palette) ? 0.14 : 0.10), 1));
            painter->setBrush(withAlpha(option->palette.color(QPalette::Window), m_config.menuOpacity));
            painter->drawRoundedRect(QRectF(option->rect).adjusted(0.5, 0.5, -0.5, -0.5), MenuRadius, MenuRadius);
            painter->restore();
            return;
        }
        break;
    case PE_IndicatorCheckBox:
    case PE_IndicatorItemViewItemCheck:
        drawCheckIndicator(option, painter, false);
        return;
    case PE_IndicatorRadioButton:
        drawCheckIndicator(option, painter, true);
        return;
    case PE_PanelLineEdit:
        // line edits inside combo boxes / spin boxes are painted by their parent
        if (widget && widget->parentWidget()
            && (qobject_cast<const QComboBox *>(widget->parentWidget()) || widget->parentWidget()->inherits("QAbstractSpinBox"))) {
            return;
        }
        if (auto frame = qstyleoption_cast<const QStyleOptionFrame *>(option); frame && frame->lineWidth > 0) {
            drawInputPanel(option, painter, option->rect);
            return;
        }
        break;
    case PE_FrameLineEdit:
        // the frame is part of the panel
        if (widget && qobject_cast<const QLineEdit *>(widget)) {
            return;
        }
        break;
    default:
        break;
    }
    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

void Style::drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    switch (element) {
    case CE_PushButtonBevel:
        if (auto button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            const bool flat = button->features & QStyleOptionButton::Flat;
            const bool isDefault = button->features & QStyleOptionButton::DefaultButton;
            if (button->features & QStyleOptionButton::CommandLinkButton) {
                break;
            }
            drawButtonPanel(option, painter, isDefault, flat);
            if (button->features & QStyleOptionButton::HasMenu) {
                const int mbi = proxy()->pixelMetric(PM_MenuButtonIndicator, button, widget);
                QStyleOptionButton arrow = *button;
                arrow.rect = QRect(button->rect.right() - mbi - 4, button->rect.top(), mbi, button->rect.height());
                proxy()->drawPrimitive(PE_IndicatorArrowDown, &arrow, painter, widget);
            }
            return;
        }
        break;
    case CE_PushButtonLabel:
        if (auto button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            if ((button->features & QStyleOptionButton::DefaultButton) && (button->state & State_Enabled)) {
                QStyleOptionButton copy = *button;
                copy.palette.setColor(QPalette::ButtonText, button->palette.color(QPalette::HighlightedText));
                copy.palette.setColor(QPalette::WindowText, button->palette.color(QPalette::HighlightedText));
                QProxyStyle::drawControl(element, &copy, painter, widget);
                return;
            }
        }
        break;
    case CE_ProgressBar:
        // let QCommonStyle split it into groove/contents/label, which then come back here
        QCommonStyle::drawControl(element, option, painter, widget);
        return;
    case CE_ProgressBarGroove:
        if (auto bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
            const bool horizontal = bar->state & State_Horizontal;
            QRectF r(bar->rect);
            const qreal thickness = 6;
            if (horizontal) {
                r = QRectF(r.left(), r.center().y() - thickness / 2, r.width(), thickness);
            } else {
                r = QRectF(r.center().x() - thickness / 2, r.top(), thickness, r.height());
            }
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(Qt::NoPen);
            painter->setBrush(withAlpha(bar->palette.color(QPalette::WindowText), 0.10));
            painter->drawRoundedRect(r, thickness / 2, thickness / 2);
            painter->restore();
            return;
        }
        break;
    case CE_ProgressBarContents:
        if (auto bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
            const bool horizontal = bar->state & State_Horizontal;
            QRectF r(bar->rect);
            const qreal thickness = 6;
            if (bar->maximum <= bar->minimum) {
                break; // busy indicator: keep Breeze's animation
            }
            // Breeze already reduces the contents rect to the filled part
            const QRectF chunk = horizontal ? QRectF(r.left(), r.center().y() - thickness / 2, r.width(), thickness)
                                            : QRectF(r.center().x() - thickness / 2, r.top(), thickness, r.height());
            if (chunk.isEmpty()) {
                return;
            }
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(Qt::NoPen);
            QColor accent = bar->palette.color(QPalette::Highlight);
            QLinearGradient g(chunk.topLeft(), horizontal ? chunk.topRight() : chunk.bottomLeft());
            g.setColorAt(0, accent.lighter(115));
            g.setColorAt(1, accent);
            painter->setBrush(g);
            painter->drawRoundedRect(chunk, thickness / 2, thickness / 2);
            painter->restore();
            return;
        }
        break;
    case CE_ScrollBarSlider:
        if (auto bar = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            const bool horizontal = bar->orientation == Qt::Horizontal;
            const bool hover = bar->state & State_MouseOver;
            const bool active = bar->activeSubControls & SC_ScrollBarSlider;
            const qreal thickness = (hover || active) ? 8 : 5;
            QRectF r(bar->rect);
            if (horizontal) {
                r = QRectF(r.left() + 2, r.bottom() - thickness - 1, r.width() - 4, thickness);
            } else {
                r = QRectF(r.right() - thickness - 1, r.top() + 2, thickness, r.height() - 4);
            }
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(Qt::NoPen);
            const qreal alpha = (bar->state & State_Sunken) ? 0.65 : (active || hover ? 0.5 : 0.35);
            painter->setBrush(withAlpha(bar->palette.color(QPalette::WindowText), alpha));
            painter->drawRoundedRect(r, thickness / 2, thickness / 2);
            painter->restore();
            return;
        }
        break;
    case CE_ScrollBarAddPage:
    case CE_ScrollBarSubPage:
    case CE_ScrollBarAddLine:
    case CE_ScrollBarSubLine:
    case CE_ScrollBarFirst:
    case CE_ScrollBarLast:
        // DTK style scroll bars: no groove, no arrows
        return;
    default:
        break;
    }
    QProxyStyle::drawControl(element, option, painter, widget);
}

void Style::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget) const
{
    switch (control) {
    case CC_ScrollBar:
        // bypass Breeze's groove/separator painting, QCommonStyle calls back into drawControl()
        QCommonStyle::drawComplexControl(control, option, painter, widget);
        return;
    case CC_ComboBox:
        if (auto combo = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            if (combo->subControls & SC_ComboBoxFrame && combo->frame) {
                if (combo->editable) {
                    drawInputPanel(option, painter, option->rect);
                } else {
                    drawButtonPanel(option, painter, false, false);
                }
                QStyleOptionComboBox copy = *combo;
                copy.subControls &= ~SC_ComboBoxFrame;
                QProxyStyle::drawComplexControl(control, &copy, painter, widget);
                return;
            }
        }
        break;
    case CC_SpinBox:
        if (auto spin = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            if (spin->subControls & SC_SpinBoxFrame && spin->frame) {
                drawInputPanel(option, painter, option->rect);
                QStyleOptionSpinBox copy = *spin;
                copy.subControls &= ~SC_SpinBoxFrame;
                QProxyStyle::drawComplexControl(control, &copy, painter, widget);
                return;
            }
        }
        break;
    default:
        break;
    }
    QProxyStyle::drawComplexControl(control, option, painter, widget);
}

} // namespace DeepinGlass
