/*
 * SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "glasscontroller.h"

#include <KWindowEffects>
#include <KWindowSystem>
#if __has_include(<KX11Extras>)
#include <KX11Extras>
#define DEEPINGLASS_HAVE_X11EXTRAS 1
#endif

#include <QCoreApplication>
#include <QDebug>
#include <QEvent>
#include <QFileInfo>
#include <QQuickItem>
#include <QQuickRenderControl>
#include <QQuickWindow>
#include <QSurfaceFormat>

namespace DeepinGlass
{
namespace
{
// Plasma's own surfaces and the login/lock screens keep their look.
const QStringList s_builtinExcluded = {
    QStringLiteral("plasmashell"),
    QStringLiteral("krunner"),
    QStringLiteral("kwin_wayland"),
    QStringLiteral("kwin_x11"),
    QStringLiteral("ksplashqml"),
    QStringLiteral("kscreenlocker_greet"),
    QStringLiteral("sddm-greeter"),
    QStringLiteral("sddm-greeter-qt6"),
    QStringLiteral("plasma-emojier"),
    QStringLiteral("spectacle"),
};

bool compositingActive()
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
} // namespace

GlassController *GlassController::self()
{
    static GlassController *s_self = new GlassController;
    return s_self;
}

GlassController::GlassController()
    : QObject(QCoreApplication::instance())
{
    m_config = QuickConfig::load();
    m_decoConfig = DecorationConfig::load();
    const QString app = QFileInfo(QCoreApplication::applicationFilePath()).fileName();
    m_enabled = m_config.translucent && !s_builtinExcluded.contains(app) && !m_config.excluded.contains(app) && compositingActive();
    if (m_enabled) {
        // windows created from now on get an alpha channel
        QQuickWindow::setDefaultAlphaBuffer(true);
        QCoreApplication::instance()->installEventFilter(this);
    }
}

qreal GlassController::backgroundOpacity(Kirigami::Platform::PlatformTheme::ColorSet set, QObject *themeParent, QPalette::ColorGroup group) const
{
    if (!m_enabled) {
        return 1.0;
    }
    // Dialogs, sheets and menus inside a window float above its content: the blur
    // only covers what is behind the window, so they stay opaque to remain readable.
    // Drawers (Kirigami's side bars) are part of the window and stay glass.
    auto isFloatingPopup = [](const QObject *popup) {
        return popup && popup->inherits("QQuickPopup") && !popup->inherits("QQuickDrawer");
    };
    if (isFloatingPopup(themeParent)) {
        return 1.0;
    }
    if (auto item = qobject_cast<QQuickItem *>(themeParent); item && item->window()
        && QQuickRenderControl::renderWindowFor(item->window())) {
        // QML embedded in a Qt Widgets window (QQuickWidget): the widget window
        // decides about translucency, see the application style
        return 1.0;
    }
    for (auto item = qobject_cast<QQuickItem *>(themeParent); item; item = item->parentItem()) {
        if (item->inherits("QQuickPopupItem")) {
            // the popup item belongs to its QQuickPopup (QObject parent)
            if (!item->parent() || isFloatingPopup(item->parent())) {
                return 1.0;
            }
        }
    }
    using Kirigami::Platform::PlatformTheme;
    if (set != PlatformTheme::Window && set != PlatformTheme::View && set != PlatformTheme::Header) {
        // buttons, selection, tooltips, complementary areas stay opaque
        return 1.0;
    }
    // The window itself is the one glass surface, with the opacity of the title bar.
    // Everything drawn on top of it (pages, side bars, views, tool bars) only adds
    // the configured extra layers, by default none, so the window looks uniform.
    if (qobject_cast<QQuickWindow *>(themeParent)) {
        return windowOpacity(group != QPalette::Inactive);
    }
    switch (set) {
    case PlatformTheme::View:
        return m_config.viewOpacity;
    case PlatformTheme::Header:
        return m_config.headerOpacity;
    default:
        return m_config.pageOpacity;
    }
}

qreal GlassController::windowOpacity(bool active) const
{
    if (m_config.windowOpacity >= 0) {
        return m_config.windowOpacity;
    }
    return active ? m_decoConfig.activeOpacity : m_decoConfig.inactiveOpacity;
}

void GlassController::prepareWindow(QQuickWindow *window)
{
    if (!m_enabled || !window || m_windows.contains(window)) {
        return;
    }
    // popups and tool tips keep their own (often rounded) shape
    const Qt::WindowType type = window->type();
    if (type != Qt::Window && type != Qt::Dialog) {
        return;
    }
    if (qEnvironmentVariableIsSet("DEEPINGLASS_DEBUG")) {
        qWarning() << "DeepinGlass: prepare" << window << "created already:" << bool(window->handle());
    }
    if (!window->handle()) {
        QSurfaceFormat format = window->requestedFormat();
        format.setAlphaBufferSize(8);
        window->setFormat(format);
    }
    m_windows.insert(window);
    connect(window, &QObject::destroyed, this, [this, window] {
        m_windows.remove(window);
    });
    connect(window, &QQuickWindow::colorChanged, this, [this, window] {
        updateBlur(window);
    });
    if (window->isVisible()) {
        updateBlur(window);
    }
}

void GlassController::polishItem(QQuickItem *item)
{
    if (!m_enabled || !item->inherits("QQuickRectangle")) {
        return;
    }
    // Kirigami's card background (DefaultCardBackground, a ShadowedRectangle) draws a
    // "basic drop shadow": a rectangle with 60 % of the background colour behind the
    // card, offset by a pixel. Below an opaque card only that pixel shows; below a
    // glass card it shows through as a milky fill and makes the card look opaque.
    // z and the parent are set after the theme is created, hence the deferred check.
    QMetaObject::invokeMethod(
        item,
        [this, item] {
            QQuickItem *card = item->parentItem();
            if (!card || !card->inherits("ShadowedRectangle") || item->z() >= 0) {
                return;
            }
            if (backgroundOpacity(Kirigami::Platform::PlatformTheme::View, card, QPalette::Active) < 1.0) {
                item->setOpacity(0.0);
            }
        },
        Qt::QueuedConnection);
}

void GlassController::updateBlur(QQuickWindow *window)
{
    if (!window->handle()) {
        return;
    }
    const bool translucent = window->color().alpha() < 255 && window->format().hasAlpha();
    if (qEnvironmentVariableIsSet("DEEPINGLASS_DEBUG")) {
        qWarning() << "DeepinGlass: window" << window << "color" << window->color() << "requested alpha"
                   << window->requestedFormat().alphaBufferSize() << "actual alpha" << window->format().alphaBufferSize();
    }
    KWindowEffects::enableBlurBehind(window, translucent, QRegion());
}

bool GlassController::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::Show || event->type() == QEvent::Expose) {
        if (auto window = qobject_cast<QQuickWindow *>(object); window && m_windows.contains(window)) {
            updateBlur(window);
        }
    }
    return false;
}

} // namespace DeepinGlass
