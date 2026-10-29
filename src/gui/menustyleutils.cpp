// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#include "gui/menustyleutils.h"
#include "core/startupdiagnostics.h"

#include "core/colorscheme.h"
#include "core/settings.h"
#include "gui/dockcomboboxchevron.h"
#include "gui/gtkmenupalette.h"
#include "gui/oklchutils.h"
#include "gui/uiconfig.h"

#include <QApplication>
#include <QAbstractItemView>
#include <QComboBox>
#include <QEvent>
#include <QIconEngine>
#include <QJsonDocument>
#include <QImage>
#include <QLabel>
#include <QLibrary>
#include <QMenu>
#include <QMenuBar>
#include <QPainter>
#include <QPointer>
#include <QScopedValueRollback>
#include <QProxyStyle>
#include <QStyle>
#include <QStyleHints>
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
#include <QAccessibilityHints>
#endif
#include <QStyleOptionComboBox>
#include <QTimer>
#include <QSysInfo>
#include <QVariant>
#include <QWidgetAction>

namespace GtkMenuPalette {
namespace {
constexpr unsigned Hover = 1 << 1;       // GTK_STATE_FLAG_PRELIGHT
constexpr unsigned Disabled = 1 << 3;    // GTK_STATE_FLAG_INSENSITIVE
constexpr unsigned Backdrop = 1 << 6;    // GTK_STATE_FLAG_BACKDROP

QColor over(const QColor& foreground, const QColor& background)
{
    const qreal alpha = foreground.alphaF();
    return QColor::fromRgbF(foreground.redF() * alpha + background.redF() * (1 - alpha),
                            foreground.greenF() * alpha + background.greenF() * (1 - alpha),
                            foreground.blueF() * alpha + background.blueF() * (1 - alpha));
}

struct CachedPalette {
    QPalette source;
    std::optional<QPalette> colors;
    bool loaded = false;
};
CachedPalette cachedPalette;

bool usesGtkColors()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    // GTK's global palette can disagree with the application's explicit mode.
    if (Settings::instance()->systemColorMode != Settings::SystemColorModeFollowSystem)
        return false;
#endif
    if (QSysInfo::kernelType() != QLatin1String("linux"))
        return false;
    const QString platform = QApplication::platformName();
    if (platform != QLatin1String("xcb") && !platform.startsWith(QLatin1String("wayland")))
        return false;
    const QByteArray theme = qgetenv("QT_QPA_PLATFORMTHEME").toLower();
    if (!theme.isEmpty() && theme != "gtk3" && theme != "gnome")
        return false;
    if (theme.isEmpty() && !qgetenv("XDG_CURRENT_DESKTOP").toLower().split(':').contains("gnome"))
        return false;
    QStyle* style = QApplication::style();
    while (auto* proxy = qobject_cast<QProxyStyle*>(style))
        style = proxy->baseStyle();
    // Dedicated styles such as Breeze and Adwaita already draw their own menus.
    return style->objectName().compare(QLatin1String("fusion"), Qt::CaseInsensitive) == 0;
}

const Api* gtkApi()
{
    static QLibrary library(QStringLiteral("libgtk-3.so.0"));
    static Api api{};
    static bool resolved = false;
    if (!resolved) {
        resolved = true;
        library.setLoadHints(QLibrary::PreventUnloadHint);
        if (!library.load())
            return nullptr;
#define RESOLVE(member, symbol) \
        api.member = reinterpret_cast<decltype(api.member)>(library.resolve(symbol));
        RESOLVE(popoverNew, "gtk_popover_new")
        RESOLVE(modelButtonNew, "gtk_model_button_new")
        RESOLVE(add, "gtk_container_add")
        RESOLVE(context, "gtk_widget_get_style_context")
        RESOLVE(setState, "gtk_style_context_set_state")
        RESOLVE(color, "gtk_style_context_get_color")
        RESOLVE(background, "gtk_render_background")
        RESOLVE(destroyWidget, "gtk_widget_destroy")
        RESOLVE(refSink, "g_object_ref_sink")
        RESOLVE(unref, "g_object_unref")
        RESOLVE(surfaceForData, "cairo_image_surface_create_for_data")
        RESOLVE(createPainter, "cairo_create")
        RESOLVE(destroyPainter, "cairo_destroy")
        RESOLVE(flushSurface, "cairo_surface_flush")
        RESOLVE(destroySurface, "cairo_surface_destroy")
#undef RESOLVE
    }
    const auto display = reinterpret_cast<void* (*)()>(library.resolve("gdk_display_get_default"));
    if (!display || !api.popoverNew || !api.modelButtonNew
        || !api.add || !api.context || !api.setState || !api.color || !api.background
        || !api.destroyWidget || !api.refSink || !api.unref || !api.surfaceForData || !api.createPainter
        || !api.destroyPainter || !api.flushSurface || !api.destroySurface)
        return nullptr;
    if (!display()) {
        // The GTK platform plugin may be absent even on GNOME. Match Qt's
        // backend and preserve its Xlib handler when initializing GTK ourselves.
        const auto initialize = reinterpret_cast<int (*)(int*, char***)>(library.resolve("gtk_init_check"));
        const auto disableLocale = reinterpret_cast<void (*)()>(library.resolve("gtk_disable_setlocale"));
        const auto backends = reinterpret_cast<void (*)(const char*)>(library.resolve("gdk_set_allowed_backends"));
        if (!initialize || !disableLocale || !backends)
            return nullptr;
        using ErrorHandler = int (*)(void*, void*);
        const auto setErrorHandler = reinterpret_cast<ErrorHandler (*)(ErrorHandler)>(library.resolve("XSetErrorHandler"));
        const ErrorHandler handler = setErrorHandler ? setErrorHandler(nullptr) : nullptr;
        disableLocale();
        backends(QApplication::platformName().startsWith(QLatin1String("wayland")) ? "wayland,x11" : "x11,wayland");
        const bool initialized = initialize(nullptr, nullptr);
        if (setErrorHandler)
            setErrorHandler(handler);
        if (!initialized)
            return nullptr;
    }
    static bool watchingTheme = false;
    if (!watchingTheme) {
        const auto settings = reinterpret_cast<void* (*)()>(library.resolve("gtk_settings_get_default"));
        using Callback = void (*)();
        using DestroyNotify = void (*)(void*, void*);
        const auto connectSignal = reinterpret_cast<unsigned long (*)(void*, const char*, Callback,
            void*, DestroyNotify, int)>(library.resolve("g_signal_connect_data"));
        if (settings && connectSignal) {
            const auto changed = +[](void*, void*, void*) {
                if (qApp)
                    QTimer::singleShot(0, qApp, []() { MenuStyle::refresh(); });
            };
            connectSignal(settings(), "notify::gtk-theme-name", reinterpret_cast<Callback>(changed), nullptr, nullptr, 0);
            connectSignal(settings(), "notify::gtk-application-prefer-dark-theme", reinterpret_cast<Callback>(changed), nullptr, nullptr, 0);
            watchingTheme = true;
        }
    }
    return &api;
}
}

QPalette read(const Api& api, const QPalette& fallback)
{
    QPalette palette = fallback;
    // Modern GNOME menus use popover model buttons. Legacy GtkMenu items
    // still use the list accent in Yaru, which differs from popup hover.
    void* menu = api.popoverNew(nullptr);
    api.refSink(menu);
    void* item = api.modelButtonNew();
    api.add(menu, item);
    void* menuContext = api.context(menu);
    void* itemContext = api.context(item);
    const auto background = [&api](void* context, unsigned state, const QColor& base) {
        QImage image(64, 64, QImage::Format_ARGB32_Premultiplied);
        image.fill(base);
        // Cairo's ARGB32 format has the same native byte order as QImage's.
        void* surface = api.surfaceForData(image.bits(), 0, image.width(), image.height(), image.bytesPerLine());
        void* painter = api.createPainter(surface);
        api.setState(context, state);
        api.background(context, painter, 0, 0, image.width(), image.height());
        api.destroyPainter(painter);
        api.flushSurface(surface);
        const QColor result = image.pixelColor(32, 32);
        api.destroySurface(surface);
        return result;
    };
    const auto foreground = [&api](void* context, unsigned state, const QColor& base) {
        Rgba color{};
        api.color(context, state, &color);
        return over(QColor::fromRgbF(color.red, color.green, color.blue, color.alpha), base);
    };
    for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
        const unsigned state = group == QPalette::Inactive ? Backdrop
            : group == QPalette::Disabled ? Disabled : 0;
        const QColor fill = background(menuContext, state, fallback.color(group, QPalette::Window));
        api.setState(itemContext, state);
        const QColor text = foreground(itemContext, state, fill);
        const QColor selected = background(itemContext, state | Hover, fill);
        const QColor selectedText = foreground(itemContext, state | Hover, selected);
        for (const QPalette::ColorRole role : {QPalette::Window, QPalette::Base, QPalette::Button})
            palette.setColor(group, role, fill);
        for (const QPalette::ColorRole role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText})
            palette.setColor(group, role, text);
        palette.setColor(group, QPalette::Highlight, selected);
        palette.setColor(group, QPalette::HighlightedText, selectedText);
    }
    api.destroyWidget(menu);
    api.unref(menu);
    return palette;
}

std::optional<QPalette> systemPalette(const QPalette& fallback)
{
    if (!usesGtkColors())
        return std::nullopt;
    CachedPalette& cached = cachedPalette;
    if (!cached.loaded || cached.source != fallback) {
        cached.source = fallback;
        cached.colors.reset();
        cached.loaded = true;
        if (const Api* api = gtkApi())
            cached.colors = read(*api, fallback);
    }
    return cached.colors;
}

void invalidate()
{
    cachedPalette.loaded = false;
}
}

namespace {
constexpr auto ThemeColorsProperty = "speedcrunchMenuThemeColors";
constexpr auto ThemeSchemeProperty = "speedcrunchMenuThemeScheme";
constexpr auto NativeBarBackgroundProperty = "speedcrunchNativeMenuBarStyledBackground";
constexpr auto SystemPaletteSourceProperty = "speedcrunchSystemMenuPaletteSource";
constexpr auto SystemPaletteProperty = "speedcrunchSystemMenuPalette";

QString themeKey()
{
    const Settings* settings = Settings::instance();
    return settings->colorScheme + QLatin1Char('|') + settings->customColorSchemeJson;
}

struct MenuColors {
    QColor background;
    QColor foreground;
    QColor selectedBackground;
    QColor selectedForeground;
};

struct CachedThemeColors {
    QString key;
    QColor fallbackBackground;
    MenuColors menu;
    MenuColors bar;
    bool valid = false;
};
CachedThemeColors cachedThemeColors;

class MenuIconEngine : public QIconEngine {
public:
    MenuIconEngine(const QIcon& source, const QPalette& palette)
        : m_source(source), m_palette(palette) {}

    QIconEngine* clone() const override { return new MenuIconEngine(m_source, m_palette); }
    bool isNull() override { return m_source.isNull(); }
    QString iconName() override { return m_source.name(); }
    QSize actualSize(const QSize& size, QIcon::Mode mode, QIcon::State state) override
    {
        return m_source.actualSize(size, mode, state);
    }
    QList<QSize> availableSizes(QIcon::Mode mode, QIcon::State state) override
    {
        return m_source.availableSizes(mode, state);
    }
    QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override
    {
        return themedPixmap(size, mode, state, 1.0);
    }
    QPixmap scaledPixmap(const QSize& size, QIcon::Mode mode, QIcon::State state,
                         qreal scale) override
    {
#if QT_VERSION < QT_VERSION_CHECK(6, 8, 0)
        // Older QIcon versions pass physical pixels to the engine.
        return themedPixmap(size / scale, mode, state, scale);
#else
        return themedPixmap(size, mode, state, scale);
#endif
    }
    void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) override
    {
        const QPixmap image = themedPixmap(rect.size(), mode, state, painter->device()->devicePixelRatioF());
        const QRectF target(QRectF(rect).center() - QPointF(image.deviceIndependentSize().width() / 2,
                                                         image.deviceIndependentSize().height() / 2),
                            image.deviceIndependentSize());
        painter->drawPixmap(target, image, QRectF(image.rect()));
    }

private:
    QPixmap themedPixmap(const QSize& size, QIcon::Mode mode, QIcon::State state, qreal scale)
    {
        QPixmap result = m_source.pixmap(size, scale, mode, state);
        if (result.isNull())
            return result;
        const QPalette::ColorGroup group = mode == QIcon::Disabled ? QPalette::Disabled : QPalette::Active;
        const QPalette::ColorRole role = mode == QIcon::Active || mode == QIcon::Selected
            ? QPalette::HighlightedText : QPalette::WindowText;
        QColor foreground = m_palette.color(group, role);
        if (mode == QIcon::Disabled
            && foreground == m_palette.color(QPalette::Active, QPalette::WindowText)) {
            const auto maximumAlpha = [](const QPixmap& pixmap) {
                const QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
                int maximum = 0;
                for (int y = 0; y < image.height(); ++y) {
                    const auto* row = reinterpret_cast<const QRgb*>(image.constScanLine(y));
                    for (int x = 0; x < image.width(); ++x) {
                        maximum = qMax(maximum, qAlpha(row[x]));
                        if (maximum == 255)
                            return maximum;
                    }
                }
                return maximum;
            };
            const QPixmap enabled = m_source.pixmap(size, scale, QIcon::Normal, state);
            // Recoloring removes gray disabled tints from opaque glyphs.
            // Fade those, but preserve opacity supplied by the source icon.
            if (maximumAlpha(result) >= maximumAlpha(enabled))
                foreground.setAlphaF(foreground.alphaF() * 0.5);
        }
        QPainter painter(&result);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(QRectF(QPointF(), result.deviceIndependentSize()), foreground);
        return result;
    }
    QIcon m_source;
    QPalette m_palette;
};

void updateMenuIcons(QMenu* menu, const QPalette* palette)
{
    if (menu == nullptr)
        return;
    for (QAction* action : menu->actions()) {
        constexpr auto SourceProperty = "speedcrunchMenuIconSource";
        constexpr auto ThemedKeyProperty = "speedcrunchMenuIconKey";
        constexpr auto PaletteProperty = "speedcrunchMenuIconPalette";
        const QIcon current = action->icon();
        const bool hasSource = action->property(SourceProperty).isValid();
        const bool usesThemedIcon = hasSource
            && current.cacheKey() == action->property(ThemedKeyProperty).toLongLong();
        const QIcon source = usesThemedIcon ? action->property(SourceProperty).value<QIcon>() : current;
        if (palette != nullptr && !source.isNull()) {
            if (usesThemedIcon && action->property(PaletteProperty).value<QPalette>() == *palette)
                continue;
            // Platform icons can ignore the menu's palette. Keep the original
            // so System appearance can restore it.
            const QIcon themed = MenuStyle::themedIcon(source, *palette);
            action->setProperty(SourceProperty, source);
            action->setProperty(ThemedKeyProperty, themed.cacheKey());
            action->setProperty(PaletteProperty, *palette);
            action->setIcon(themed);
        } else if (hasSource) {
            action->setProperty(SourceProperty, QVariant());
            action->setProperty(ThemedKeyProperty, QVariant());
            action->setProperty(PaletteProperty, QVariant());
            if (usesThemedIcon)
                action->setIcon(source);
        }
    }
}

MenuColors themeColors(const QWidget* widget)
{
    const QVariantList saved = widget->property(ThemeColorsProperty).toList();
    const QString key = themeKey();
    if (saved.size() == 4 && widget->property(ThemeSchemeProperty).toString() == key) {
        return {saved.at(0).value<QColor>(), saved.at(1).value<QColor>(),
                saved.at(2).value<QColor>(), saved.at(3).value<QColor>()};
    }

    const QColor fallback = QApplication::palette().color(QPalette::Base);
    CachedThemeColors& cached = cachedThemeColors;
    const bool isBar = qobject_cast<const QMenuBar*>(widget) != nullptr;
    if (cached.valid && cached.key == key && cached.fallbackBackground == fallback) {
        StartupDiagnostics::Scope timing("Reuse menu theme colors");
        return isBar ? cached.bar : cached.menu;
    }
    StartupDiagnostics::Scope timing("Generate menu theme colors");
    const Settings* settings = Settings::instance();
    ColorScheme scheme = settings->colorScheme == QLatin1String("Custom")
        ? ColorScheme(QJsonDocument::fromJson(settings->customColorSchemeJson.toUtf8()))
        : ColorScheme::loadByName(settings->colorScheme);
    if (!scheme.isValid())
        scheme = ColorScheme::loadByName(QStringLiteral("Terminal"));
    const QColor base = scheme.isValid() ? scheme.colorForRole(ColorScheme::Background)
                                        : fallback;
    const QVector<QColor> shades = generateOklchShades(base, 6, themePolarityForBackground(base));
    const QColor selected = shades.at(UiConfig::DockUnfocusedSelectedItemShade);
    const QColor selectedText = aaForegroundForBackground(selected);
    const auto colors = [&](int shade) {
        const QColor background = shades.at(shade);
        return MenuColors{background, aaForegroundForBackground(background), selected, selectedText};
    };
    cached.key = key;
    cached.fallbackBackground = fallback;
    cached.menu = colors(UiConfig::DockHeaderShade);
    cached.bar = colors(UiConfig::WindowBackgroundShade);
    cached.valid = true;
    return isBar ? cached.bar : cached.menu;
}

QPalette fullyResolvedPalette(QPalette palette)
{
    // Resolve every role explicitly so a themed parent cannot supply menu
    // text, selection, or disabled colors through palette inheritance.
    for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive,
                                            QPalette::Disabled}) {
        for (int role = 0; role < QPalette::NColorRoles; ++role) {
            const auto colorRole = static_cast<QPalette::ColorRole>(role);
            if (colorRole != QPalette::NoRole)
                palette.setBrush(group, colorRole, palette.brush(group, colorRole));
        }
    }
    return palette;
}

QPalette fullyResolvedSystemPalette(const QWidget* widget)
{
    QPalette palette = QApplication::palette(widget);
    if (qobject_cast<const QMenu*>(widget)) {
        if (const auto gtk = GtkMenuPalette::systemPalette(palette))
            palette = *gtk;
    }
    return fullyResolvedPalette(palette);
}

void refreshSystemColorModePalette()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    struct Override {
        QPalette original;
        Settings::SystemColorMode mode;
    };
    static std::optional<Override> activeOverride;
    const auto release = []() {
        if (!activeOverride)
            return;
        const QPalette original = activeOverride->original;
        activeOverride.reset();
        // Keep the original resolve mask so system colors can change again.
        QApplication::setPalette(original);
    };
    const auto mode = Settings::instance()->systemColorMode;
    if (activeOverride && activeOverride->mode != mode)
        release();
    if (mode == Settings::SystemColorModeFollowSystem)
        return;

    QStyle* style = QApplication::style();
    while (auto* proxy = qobject_cast<QProxyStyle*>(style))
        style = proxy->baseStyle();
    if (style->objectName().compare(QLatin1String("fusion"), Qt::CaseInsensitive) != 0) {
        release();
        return;
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    if (QGuiApplication::styleHints()->accessibility()->contrastPreference()
        == Qt::ContrastPreference::HighContrast) {
        release();
        return;
    }
#else
    // Earlier Windows backends report Unknown for a contrast theme.
    if (QGuiApplication::platformName() == QLatin1String("windows")
        && QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Unknown) {
        release();
        return;
    }
#endif
    const bool dark = mode == Settings::SystemColorModeDark;
    const auto isDark = [](const QPalette& palette) {
        return palette.color(QPalette::Window).lightness()
            < palette.color(QPalette::WindowText).lightness();
    };
    // Qt 6.8/6.9 GTK caches its fallback palette across color-mode changes.
    // Fusion's standard palette is generated afresh from the requested mode.
    const QPalette fresh = QApplication::style()->standardPalette();
    if (isDark(fresh) != dark) {
        release();
        return; // The platform does not support this override.
    }
    if (!activeOverride && isDark(QApplication::palette()) == dark)
        return;
    if (!activeOverride)
        activeOverride = Override {QApplication::palette(), mode};
    QApplication::setPalette(fresh);
#endif
}

class MenuAppearanceFilter : public QObject {
public:
    explicit MenuAppearanceFilter(QObject* parent) : QObject(parent) {}

    void apply(QWidget* widget)
    {
        if (m_updating || widget == nullptr)
            return;
        QScopedValueRollback<bool> guard(m_updating, true);
        applyWidget(widget);
        for (QMenu* child : widget->findChildren<QMenu*>())
            applyWidget(child);
    }

    void refresh()
    {
        if (m_updating)
            return;
        QScopedValueRollback<bool> guard(m_updating, true);
        refreshSystemColorModePalette();
        for (QWidget* widget : QApplication::allWidgets())
            applyWidget(widget);
    }

    void scheduleRefresh()
    {
        if (m_refreshPending)
            return;
        m_refreshPending = true;
        QTimer::singleShot(0, this, [this]() {
            m_refreshPending = false;
            refresh();
        });
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (m_updating)
            return false;
        if ((watched == qApp && event->type() == QEvent::ApplicationPaletteChange)
            || event->type() == QEvent::ThemeChange) {
            // Let the platform finish updating its palette before refreshing
            // menus, including menus that are already open.
            GtkMenuPalette::invalidate();
            scheduleRefresh();
        }
        if (event->type() == QEvent::Polish || event->type() == QEvent::Show
            || event->type() == QEvent::ActionAdded || event->type() == QEvent::ActionChanged
            || event->type() == QEvent::ApplicationPaletteChange
            || event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange) {
            QWidget* widget = qobject_cast<QWidget*>(watched);
            if ((qobject_cast<QMenu*>(widget) || qobject_cast<QMenuBar*>(widget)
                 || (qobject_cast<QComboBox*>(widget)
                     && widget->property("speedcrunchDockComboBox").toBool()))
                && (event->type() == QEvent::Polish || event->type() == QEvent::Show
                    || widget->testAttribute(Qt::WA_WState_Polished))) {
                if (event->type() == QEvent::StyleChange)
                    widget->setProperty(SystemPaletteSourceProperty, QVariant());
                apply(widget);
                if (event->type() == QEvent::StyleChange || event->type() == QEvent::Polish)
                    scheduleRefresh();
            }
        }
        return false;
    }

private:
    void applyWidget(QWidget* widget)
    {
        if (auto* combo = qobject_cast<QComboBox*>(widget)) {
            if (combo->property("speedcrunchDockComboBox").toBool())
                DockComboBoxChevron::refreshPopupAppearance(combo);
            return;
        }
        auto* bar = qobject_cast<QMenuBar*>(widget);
        if (bar == nullptr && !qobject_cast<QMenu*>(widget))
            return;
        StartupDiagnostics::Scope timing("Apply menu appearance");

        if (Settings::instance()->menuAppearance == Settings::MenuAppearanceSystem
            || (bar != nullptr && bar->isNativeMenuBar())) {
            const bool hadTheme = !widget->styleSheet().isEmpty();
            if (hadTheme)
                widget->setStyleSheet(QString());
            const QPalette source = fullyResolvedSystemPalette(widget);
            if (hadTheme || !widget->property(SystemPaletteSourceProperty).isValid()
                || widget->property(SystemPaletteSourceProperty).value<QPalette>() != source) {
                // Clear Qt's saved stylesheet palette before letting the desktop
                // style restore its own menu colors, including header text.
                widget->style()->unpolish(widget);
                widget->setPalette(source);
                widget->style()->polish(widget);
                widget->setProperty(SystemPaletteSourceProperty, source);
                widget->setProperty(SystemPaletteProperty, fullyResolvedPalette(widget->palette()));
            }
            const QPalette palette = widget->property(SystemPaletteProperty).value<QPalette>();
            if (widget->palette() != palette || widget->palette().resolveMask() != palette.resolveMask())
                widget->setPalette(palette);
            // Some desktop styles leave the bar transparent and rely on its
            // parent to paint the background. Use the system brush here too.
            if (bar != nullptr && !bar->isNativeMenuBar()) {
                // Removing a stylesheet does not restore this painting flag.
                // Otherwise desktop styles can still paint a themed surface.
                if (bar->property(NativeBarBackgroundProperty).isValid())
                    bar->setAttribute(Qt::WA_StyledBackground,
                                      bar->property(NativeBarBackgroundProperty).toBool());
                bar->setAutoFillBackground(true);
                bar->update();
            }
            if (auto* menu = qobject_cast<QMenu*>(widget)) {
                updateMenuIcons(menu, nullptr);
                for (QAction* action : menu->actions()) {
                    auto* widgetAction = qobject_cast<QWidgetAction*>(action);
                    QWidget* content = widgetAction != nullptr ? widgetAction->defaultWidget() : nullptr;
                    if (content == nullptr)
                        continue;
                    content->setPalette(palette);
                    for (QWidget* control : content->findChildren<QWidget*>()) {
                        control->setPalette(qobject_cast<QLabel*>(control)
                                                ? palette : fullyResolvedSystemPalette(control));
                    }
                }
            }
            return;
        }

        const MenuColors colors = themeColors(widget);
        widget->setProperty(SystemPaletteSourceProperty, QVariant());
        if (bar != nullptr && !bar->property(NativeBarBackgroundProperty).isValid())
            bar->setProperty(NativeBarBackgroundProperty, bar->testAttribute(Qt::WA_StyledBackground));
        QPalette palette = fullyResolvedSystemPalette(widget);
        for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive,
                                                QPalette::Disabled}) {
            for (const QPalette::ColorRole role : {QPalette::Window, QPalette::Base, QPalette::Button})
                palette.setColor(group, role, colors.background);
            for (const QPalette::ColorRole role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText})
                palette.setColor(group, role, colors.foreground);
            palette.setColor(group, QPalette::Highlight, colors.selectedBackground);
            palette.setColor(group, QPalette::HighlightedText, colors.selectedForeground);
        }
        QString style = (bar != nullptr
            ? QStringLiteral(
                "QMenuBar { background-color: %1; color: %2; border: none; }"
                "QMenuBar::item { background: transparent; color: %2; }"
                "QMenuBar::item:selected { background-color: %3; color: %4; }"
                "QMenuBar::item:pressed { background-color: %3; color: %4; }")
            : QStringLiteral(
                "QMenu { background-color: %1; color: %2;"
                " border: 1px solid %3; border-radius: %5px; }"
                "QMenu::item:selected { background-color: %3; color: %4; }"))
            .arg(colors.background.name(), colors.foreground.name(),
                 colors.selectedBackground.name(), colors.selectedForeground.name());
        if (bar == nullptr) {
            style = style.arg(UiConfig::ControlCornerRadius);
            QColor disabledForeground = colors.foreground;
            disabledForeground.setAlphaF(disabledForeground.alphaF() * 0.5);
            style += QStringLiteral("QMenu::item:disabled { color: %1; }")
                         .arg(disabledForeground.name(QColor::HexArgb));
        }
        if (widget->styleSheet() != style)
            widget->setStyleSheet(style);
        if (widget->palette() != palette)
            widget->setPalette(palette);
        updateMenuIcons(qobject_cast<QMenu*>(widget), &palette);
    }

    bool m_updating = false;
    bool m_refreshPending = false;
};

QPointer<MenuAppearanceFilter>& appearanceFilter()
{
    static QPointer<MenuAppearanceFilter> filter;
    return filter;
}
}

void MenuStyle::install()
{
    if (appearanceFilter() || qApp == nullptr)
        return;
    appearanceFilter() = new MenuAppearanceFilter(qApp);
    qApp->installEventFilter(appearanceFilter());
}

void MenuStyle::refresh()
{
    // A theme file can change without changing its saved name.
    cachedThemeColors.valid = false;
    GtkMenuPalette::invalidate();
    install();
    if (appearanceFilter()) {
        appearanceFilter()->refresh();
        // Finish after native style handlers and popup dismissal as well.
        appearanceFilter()->scheduleRefresh();
    }
}

void MenuStyle::apply(QWidget* widget)
{
    install();
    if (appearanceFilter())
        appearanceFilter()->apply(widget);
}

QIcon MenuStyle::themedIcon(const QIcon& source, const QPalette& palette)
{
    return source.isNull() ? QIcon() : QIcon(new MenuIconEngine(source, palette));
}

QPalette MenuStyle::systemPalette(const QWidget* widget)
{
    return fullyResolvedSystemPalette(widget);
}

QPalette MenuStyle::systemComboPopupPalette(const QComboBox* combo)
{
    QStyleOptionComboBox option;
    option.initFrom(combo);
    option.editable = combo->isEditable();
    const bool menu = QApplication::style()->styleHint(QStyle::SH_ComboBox_Popup, &option, combo);
    // macOS gives menus and lists different selection colors. A menu-style
    // dropdown must use the menu palette even though its view is a QListView.
    QPalette palette = menu ? QApplication::palette("QMenu") : QApplication::palette(combo->view());
    if (menu) {
        if (const auto gtk = GtkMenuPalette::systemPalette(palette))
            palette = *gtk;
    }
    return fullyResolvedPalette(palette);
}

void MenuStyle::setThemeColors(QMenu* menu, const QColor& background, const QColor& foreground,
                              const QColor& selectedBackground, const QColor& selectedForeground)
{
    if (menu == nullptr)
        return;
    const QVariantList colors {background, foreground, selectedBackground, selectedForeground};
    const QString key = themeKey();
    const auto save = [&colors, &key](QMenu* target) {
        target->setProperty(ThemeColorsProperty, colors);
        target->setProperty(ThemeSchemeProperty, key);
    };
    save(menu);
    for (QMenu* child : menu->findChildren<QMenu*>())
        save(child);
    apply(menu);
}
