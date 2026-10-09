/*
    Based on PlasmaDesktopTheme from qqc2-desktop-style (KDE, commit 20471364a82a).
    Changes for the Deepin Glass look (marked with "Deepin Glass"):
      - the background colours of the Window, View and Header colour sets get an
        alpha channel, so Kirigami windows, pages and tool bars become frosted glass
      - windows are given an alpha channel before they are created
    Everything else is unchanged; with [QtQuick] Translucent=false in deepinglassrc
    this plugin behaves like the original one.

    SPDX-FileCopyrightText: 2017 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "glasstheme.h"
#include "glasscontroller.h"

#if HAVE_QTDBUS
#include <QDBusConnection>
#endif

#include <QFontDatabase>
#include <QGuiApplication>
#include <QPalette>
#include <QQuickRenderControl>
#include <QQuickWindow>

#include <KColorScheme>
#include <KConfigGroup>
#include <KIconColors>

class StyleSingleton : public QObject
{
    Q_OBJECT

public:
    struct Colors {
        QPalette palette;
        KColorScheme selectionScheme;
        KColorScheme scheme;
    };

    explicit StyleSingleton()
        : QObject()
        , buttonScheme(QPalette::Active, KColorScheme::ColorSet::Button)
        , viewScheme(QPalette::Active, KColorScheme::ColorSet::View)
    {
#if HAVE_QTDBUS
        // Use DBus in order to listen for settings changes directly, as the
        // QApplication doesn't expose the font variants we're looking for,
        // namely smallFont.
        QDBusConnection::sessionBus().connect(QString(),
                                              QStringLiteral("/KDEPlatformTheme"),
                                              QStringLiteral("org.kde.KDEPlatformTheme"),
                                              QStringLiteral("refreshFonts"),
                                              this,
                                              SLOT(notifyWatchersConfigurationChange()));
#endif

        connect(qGuiApp, &QGuiApplication::fontDatabaseChanged, this, &StyleSingleton::notifyWatchersConfigurationChange);
        qGuiApp->installEventFilter(this);

        // NativeTextRendering is still distorted sometimes with fractional scale factors
        // Given Qt disables all hinting with native rendering when any scaling is used anyway
        // we can use Qt's rendering throughout
        // QTBUG-126577
        // Deepin Glass: text on translucent backgrounds must not use sub-pixel
        // anti-aliasing (coloured fringes), so glass windows use Qt's text rendering
        if (DeepinGlass::GlassController::self()->isEnabled()) {
            QQuickWindow::setTextRenderType(QQuickWindow::QtTextRendering);
        } else if (qApp->devicePixelRatio() == 1.0) {
            QQuickWindow::setTextRenderType(QQuickWindow::NativeTextRendering);
        } else {
            QQuickWindow::setTextRenderType(QQuickWindow::QtTextRendering);
        }
    }

    void refresh()
    {
        m_cache.clear();
        buttonScheme = KColorScheme(QPalette::Active, KColorScheme::ColorSet::Button);
        viewScheme = KColorScheme(QPalette::Active, KColorScheme::ColorSet::View);

        notifyWatchersPaletteChange();
    }

    Colors loadColors(Kirigami::Platform::PlatformTheme::ColorSet cs, QPalette::ColorGroup group)
    {
        const auto key = qMakePair(cs, group);
        auto it = m_cache.constFind(key);
        if (it != m_cache.constEnd()) {
            return *it;
        }

        using Kirigami::Platform::PlatformTheme;

        KColorScheme::ColorSet set;

        switch (cs) {
        case PlatformTheme::Button:
            set = KColorScheme::ColorSet::Button;
            break;
        case PlatformTheme::Selection:
            set = KColorScheme::ColorSet::Selection;
            break;
        case PlatformTheme::Tooltip:
            set = KColorScheme::ColorSet::Tooltip;
            break;
        case PlatformTheme::View:
            set = KColorScheme::ColorSet::View;
            break;
        case PlatformTheme::Complementary:
            set = KColorScheme::ColorSet::Complementary;
            break;
        case PlatformTheme::Header:
            set = KColorScheme::ColorSet::Header;
            break;
        case PlatformTheme::Window:
        default:
            set = KColorScheme::ColorSet::Window;
        }

        Colors ret = {{}, KColorScheme(group, KColorScheme::ColorSet::Selection), KColorScheme(group, set)};

        QPalette pal;
        for (auto state : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
            pal.setBrush(state, QPalette::WindowText, ret.scheme.foreground());
            pal.setBrush(state, QPalette::Window, ret.scheme.background());
            pal.setBrush(state, QPalette::Base, ret.scheme.background());
            pal.setBrush(state, QPalette::Text, ret.scheme.foreground());
            pal.setBrush(state, QPalette::Button, ret.scheme.background());
            pal.setBrush(state, QPalette::ButtonText, ret.scheme.foreground());
            pal.setBrush(state, QPalette::Highlight, ret.selectionScheme.background());
            pal.setBrush(state, QPalette::HighlightedText, ret.selectionScheme.foreground());
            pal.setBrush(state, QPalette::ToolTipBase, ret.scheme.background());
            pal.setBrush(state, QPalette::ToolTipText, ret.scheme.foreground());

            pal.setColor(state, QPalette::Light, ret.scheme.shade(KColorScheme::LightShade));
            pal.setColor(state, QPalette::Midlight, ret.scheme.shade(KColorScheme::MidlightShade));
            pal.setColor(state, QPalette::Mid, ret.scheme.shade(KColorScheme::MidShade));
            pal.setColor(state, QPalette::Dark, ret.scheme.shade(KColorScheme::DarkShade));
            pal.setColor(state, QPalette::Shadow, ret.scheme.shade(KColorScheme::ShadowShade));

            pal.setBrush(state, QPalette::AlternateBase, ret.scheme.background(KColorScheme::AlternateBackground));
            pal.setBrush(state, QPalette::Link, ret.scheme.foreground(KColorScheme::LinkText));
            pal.setBrush(state, QPalette::LinkVisited, ret.scheme.foreground(KColorScheme::VisitedText));
        }
        ret.palette = pal;
        m_cache.insert(key, ret);
        return ret;
    }

    void notifyWatchersPaletteChange()
    {
        for (auto watcher : std::as_const(watchers)) {
            watcher->syncColors();
        }
    }

    Q_SLOT void notifyWatchersConfigurationChange()
    {
        for (auto watcher : std::as_const(watchers)) {
            watcher->setDefaultFont(qApp->font());
            watcher->setSmallFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
            watcher->setFixedWidthFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        }
    }

    KColorScheme buttonScheme;
    KColorScheme viewScheme;

    QList<GlassTheme *> watchers;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (obj != qGuiApp) {
            return false;
        }

        if (event->type() == QEvent::ApplicationFontChange) {
            notifyWatchersConfigurationChange();
        }
        if (event->type() == QEvent::ApplicationPaletteChange) {
            refresh();
        }
        return false;
    }

private:
    QHash<QPair<Kirigami::Platform::PlatformTheme::ColorSet, QPalette::ColorGroup>, Colors> m_cache;
};

Q_GLOBAL_STATIC(StyleSingleton, s_style);

GlassTheme::GlassTheme(QObject *parent)
    : PlatformTheme(parent)
{
    setConstructing(true);
    setSupportsIconColoring(true);

    auto parentItem = qobject_cast<QQuickItem *>(parent);
    if (parentItem) {
        connect(parentItem, &QQuickItem::enabledChanged, this, &GlassTheme::syncColors);
        connect(parentItem, &QQuickItem::visibleChanged, this, &GlassTheme::syncColors);
        connect(parentItem, &QQuickItem::windowChanged, this, &GlassTheme::syncWindow);
        DeepinGlass::GlassController::self()->polishItem(parentItem); // Deepin Glass
    }

    s_style->watchers.append(this);

    // Deepin Glass: the theme of a Kirigami window is attached to the window itself and
    // created before the window is shown, i.e. before the native window exists
    if (auto window = qobject_cast<QQuickWindow *>(parent)) {
        DeepinGlass::GlassController::self()->prepareWindow(window);
    }

    setDefaultFont(qGuiApp->font());
    setSmallFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
    setFixedWidthFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    syncWindow();
    if (!m_window) {
        syncColors();
    }
    setConstructing(false);
}

GlassTheme::~GlassTheme()
{
    s_style->watchers.removeOne(this);
}

void GlassTheme::syncWindow()
{
    if (m_window) {
        disconnect(m_window.data(), &QWindow::activeChanged, this, &GlassTheme::syncColors);
    }

    QWindow *window = nullptr;

    auto parentItem = qobject_cast<QQuickItem *>(parent());
    if (parentItem) {
        QQuickWindow *qw = parentItem->window();

        window = QQuickRenderControl::renderWindowFor(qw);
        if (!window) {
            window = qw;
        }
        disconnect(m_sgConnection);
        if (qw && !qw->isSceneGraphInitialized() && qw != m_window) {
            m_sgConnection = connect(qw, &QQuickWindow::sceneGraphInitialized, this, &GlassTheme::syncWindow);
        } else if (!qw) {
            m_sgConnection = QMetaObject::Connection();
        }
    } else if (auto qw = qobject_cast<QQuickWindow *>(parent())) {
        // Deepin Glass: the theme attached to the window itself provides the window
        // colour, which follows the title bar's active/inactive opacity
        window = qw;
    }
    m_window = window;
    if (auto qw = qobject_cast<QQuickWindow *>(window)) {
        DeepinGlass::GlassController::self()->prepareWindow(qw); // Deepin Glass
    }
    if (window) {
        connect(m_window.data(), &QWindow::activeChanged, this, &GlassTheme::syncColors);
        syncColors();
    }
}

QIcon GlassTheme::iconFromTheme(const QString &name, const QColor &customColor)
{
    static auto useQtIconLoader = qApp->property("QQC2_DESKTOP_USE_QICON_FROM_THEME").toBool();
    if (useQtIconLoader) {
        return QIcon::fromTheme(name);
    }

    if (customColor != Qt::transparent) {
        KIconColors colors;
        colors.setText(customColor);
        return KDE::icon(name, colors);
    } else {
        return KDE::icon(name);
    }
}

void GlassTheme::syncColors()
{
    if (QCoreApplication::closingDown()) {
        return;
    }

    QPalette::ColorGroup group = (QPalette::ColorGroup)colorGroup();
    auto parentItem = qobject_cast<QQuickItem *>(parent());
    if (parentItem) {
        if (!parentItem->isVisible()) {
            return;
        }
        if (!parentItem->isEnabled()) {
            group = QPalette::Disabled;
            // Why also checking the window is exposed?
            // in the case of QQuickWidget the window() will never be active
            // and the widgets will always have the inactive palette.
            // better to always show it active than always show it inactive
        } else if (m_window && !m_window->isActive() && m_window->isExposed()) {
            group = QPalette::Inactive;
        }
    } else if (m_window && m_window == parent() && !m_window->isActive() && m_window->isExposed()) {
        group = QPalette::Inactive; // Deepin Glass, see syncWindow()
    }

    const auto colors = s_style->loadColors(colorSet(), group);

    Kirigami::Platform::PlatformThemeChangeTracker tracker(this);

    // foreground
    setTextColor(colors.scheme.foreground(KColorScheme::NormalText).color());
    setDisabledTextColor(colors.scheme.foreground(KColorScheme::InactiveText).color());
    setHighlightedTextColor(colors.selectionScheme.foreground(KColorScheme::NormalText).color());
    setActiveTextColor(colors.scheme.foreground(KColorScheme::ActiveText).color());
    setLinkColor(colors.scheme.foreground(KColorScheme::LinkText).color());
    setVisitedLinkColor(colors.scheme.foreground(KColorScheme::VisitedText).color());
    setNegativeTextColor(colors.scheme.foreground(KColorScheme::NegativeText).color());
    setNeutralTextColor(colors.scheme.foreground(KColorScheme::NeutralText).color());
    setPositiveTextColor(colors.scheme.foreground(KColorScheme::PositiveText).color());

    // background
    // Deepin Glass: translucent backgrounds for the colour sets that make up windows,
    // pages, views and tool bars
    const qreal glass = DeepinGlass::GlassController::self()->backgroundOpacity(colorSet(), parent(), group);
    setBackgroundColor(DeepinGlass::GlassController::withOpacity(colors.scheme.background(KColorScheme::NormalBackground).color(), glass));
    setAlternateBackgroundColor(DeepinGlass::GlassController::withOpacity(colors.scheme.background(KColorScheme::AlternateBackground).color(), glass));
    setHighlightColor(colors.selectionScheme.background(KColorScheme::NormalBackground).color());
    setActiveBackgroundColor(colors.scheme.background(KColorScheme::ActiveBackground).color());
    setLinkBackgroundColor(colors.scheme.background(KColorScheme::LinkBackground).color());
    setVisitedLinkBackgroundColor(colors.scheme.background(KColorScheme::VisitedBackground).color());
    setNegativeBackgroundColor(colors.scheme.background(KColorScheme::NegativeBackground).color());
    setNeutralBackgroundColor(colors.scheme.background(KColorScheme::NeutralBackground).color());
    setPositiveBackgroundColor(colors.scheme.background(KColorScheme::PositiveBackground).color());

    // decoration
    setHoverColor(colors.scheme.decoration(KColorScheme::HoverColor).color());
    setFocusColor(colors.scheme.decoration(KColorScheme::FocusColor).color());
    setFrameContrast(KColorScheme::frameContrast());
}

void GlassTheme::syncFrameContrast()
{
    if (QCoreApplication::closingDown()) {
        return;
    }

    QPalette::ColorGroup group = (QPalette::ColorGroup)colorGroup();
    auto parentItem = qobject_cast<QQuickItem *>(parent());
    if (parentItem) {
        if (!parentItem->isVisible()) {
            return;
        }
        if (!parentItem->isEnabled()) {
            group = QPalette::Disabled;
        } else if (m_window && !m_window->isActive() && m_window->isExposed()) {
            group = QPalette::Inactive;
        }
    }

    const auto colors = s_style->loadColors(colorSet(), group);

    Kirigami::Platform::PlatformThemeChangeTracker tracker(this);

    setTextColor(colors.scheme.foreground(KColorScheme::NormalText).color());
    setDisabledTextColor(colors.scheme.foreground(KColorScheme::InactiveText).color());

    // Deepin Glass
    setBackgroundColor(DeepinGlass::GlassController::withOpacity(colors.scheme.background(KColorScheme::NormalBackground).color(),
                                                                DeepinGlass::GlassController::self()->backgroundOpacity(colorSet(), parent(), group)));
    setFrameContrast(KColorScheme::frameContrast());
}

bool GlassTheme::event(QEvent *event)
{
    if (event->type() == Kirigami::Platform::PlatformThemeEvents::DataChangedEvent::type) {
        syncColors();
    }

    if (event->type() == Kirigami::Platform::PlatformThemeEvents::ColorSetChangedEvent::type) {
        syncColors();
    }

    if (event->type() == Kirigami::Platform::PlatformThemeEvents::ColorGroupChangedEvent::type) {
        syncColors();
    }

    if (event->type() == Kirigami::Platform::PlatformThemeEvents::FrameContrastChangedEvent::type) {
        syncFrameContrast();
    }

    return PlatformTheme::event(event);
}

#include "moc_glasstheme.cpp"
#include "glasstheme.moc"
