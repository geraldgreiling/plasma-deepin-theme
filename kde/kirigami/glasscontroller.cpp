/*
 * SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "glasscontroller.h"
#include "glasstheme.h"

#include <KWindowEffects>
#include <KWindowSystem>
#if __has_include(<KX11Extras>)
#include <KX11Extras>
#define DEEPINGLASS_HAVE_X11EXTRAS 1
#endif

#include <QCoreApplication>
#include <QGuiApplication>
#include <QPalette>
#include <QDebug>
#include <QEvent>
#include <QFileInfo>
#include <QQuickItem>
#include <QQuickRenderControl>
#include <QQuickWindow>
#include <QSGRendererInterface>
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
    if (auto item = qobject_cast<QQuickItem *>(themeParent); item && item->window()) {
        QQuickWindow *window = item->window();
        if (isGlassOffscreenWindow(window)) {
            // QML in a QQuickWidget of a glass widget window (System Settings):
            // the widget window paints the glass, see the application style
        } else if (window->objectName() == QLatin1String("QQuickWidgetOffscreenWindow") || QQuickRenderControl::renderWindowFor(window)) {
            // other embedded QML (opaque widget window, other offscreen rendering)
            return 1.0;
        } else if (!isGlassCandidate(window)) {
            // windows that are not glass (frameless overlays, popup windows)
            return 1.0;
        }
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
    if (auto window = qobject_cast<QQuickWindow *>(themeParent)) {
        return isGlassCandidate(window) ? windowOpacity(group != QPalette::Inactive) : 1.0;
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

bool GlassController::isGlassOffscreenWindow(const QQuickWindow *window)
{
    return window && window->property("_deepinglass_glass").toBool();
}

bool GlassController::isGlassCandidate(const QQuickWindow *window)
{
    const Qt::WindowType type = window->type();
    return (type == Qt::Window || type == Qt::Dialog) && !window->flags().testFlag(Qt::FramelessWindowHint)
        && window->objectName() != QLatin1String("QQuickWidgetOffscreenWindow")
        && !QQuickRenderControl::renderWindowFor(const_cast<QQuickWindow *>(window));
}

void GlassController::prepareWindow(QQuickWindow *window, bool ownTheme)
{
    if (!m_enabled || !window) {
        return;
    }
    if (ownTheme) {
        m_themedWindows.insert(window);
    }
    if (m_windows.contains(window)) {
        return;
    }
    // popups and tool tips keep their own (often rounded) shape; frameless windows are
    // overlays (e.g. Spectacle's region selection) without a title bar to match;
    // QQuickWidget renders offscreen into a widget window, see the application style
    if (!isGlassCandidate(window)) {
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
        m_themedWindows.remove(window);
        m_applied.remove(window);
        m_requested.remove(window);
    });
    connect(window, &QQuickWindow::colorChanged, this, [this, window] {
        applyClearColor(window);
        updateBlur(window);
    });
    connect(window, &QWindow::activeChanged, this, [this, window] {
        if (!m_themedWindows.contains(window)) {
            applyClearColor(window); // the theme of themed windows does it
        }
    });
    applyClearColor(window);
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

void GlassController::applyClearColor(QQuickWindow *window)
{
    QColor requested = window->color();
    if (m_applied.contains(window) && requested == m_applied.value(window)) {
        requested = m_requested.value(window); // our own change, or re-applying
    }
    m_requested.insert(window, requested);
    QColor color = requested;
    if (m_themedWindows.contains(window)) {
        if (requested.alpha() == 255) {
            return; // opaque on purpose (e.g. excluded colour set)
        }
    } else {
        // Windows without a theme of their own (QQuickView, e.g. Spectacle) take their
        // colour from the application palette (Window or Base, the latter made
        // translucent for content views by the widget style). Those get the glass
        // with the title bar's opacity; other colours are the application's own.
        const QPalette pal = qGuiApp->palette();
        const QRgb rgb = requested.rgb();
        if (rgb != pal.color(QPalette::Window).rgb() && rgb != pal.color(QPalette::Base).rgb()) {
            return;
        }
        color.setAlphaF(windowOpacity(window->isActive()));
    }
    // The colour is straight alpha. With OpenGL/Vulkan Qt Quick clears the
    // premultiplied swap chain with exactly this value, so e.g. (0.97, 0.97, 0.97, 0.72)
    // adds more light than its alpha allows and the window turns out (nearly) opaque
    // white. The software backend clears with QPainter, which premultiplies itself.
    if (QQuickWindow::graphicsApi() != QSGRendererInterface::Software) {
        color = QColor::fromRgbF(color.redF() * color.alphaF(), color.greenF() * color.alphaF(), color.blueF() * color.alphaF(), color.alphaF());
    }
    m_applied.insert(window, color);
    if (window->color() != color) {
        // a C++ setter does not remove a QML binding: when the binding changes the
        // colour again, this runs again
        window->setColor(color);
    }
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
    if (event->type() == QEvent::DynamicPropertyChange
        && static_cast<QDynamicPropertyChangeEvent *>(event)->propertyName() == "_deepinglass_glass") {
        // the widget style turned a QQuickWidget into glass after its QML was loaded
        GlassTheme::syncAllColors();
    }
    if (event->type() == QEvent::Show || event->type() == QEvent::Expose) {
        if (auto window = qobject_cast<QQuickWindow *>(object); window && m_windows.contains(window)) {
            updateBlur(window);
        }
    }
    return false;
}

} // namespace DeepinGlass
