// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mainwindowtesthelpers.h"

using namespace MainWindowTestHelpers;

class TestMenusUi : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init() { UiTestFixture::resetSettings(); }
    void menu_bar_keeps_themed_contrast_with_platform_palette_data();
    void menu_bar_keeps_themed_contrast_with_platform_palette();
    void menu_appearance_defaults_to_theme_and_persists();
    void menu_bar_restores_native_painting_after_theme_switch_data();
    void menu_bar_restores_native_painting_after_theme_switch();
    void system_menus_restore_style_polished_foreground_and_hover_colors_data();
    void system_menus_restore_style_polished_foreground_and_hover_colors();
    void gtk_menus_use_hover_colors_instead_of_the_list_accent_data();
    void gtk_menus_use_hover_colors_instead_of_the_list_accent();
    void dock_dropdowns_follow_menu_appearance_data();
    void dock_dropdowns_follow_menu_appearance();
    void system_dropdowns_use_the_native_popup_selection_palette_data();
    void system_dropdowns_use_the_native_popup_selection_palette();
    void macos_system_menus_use_cocoa_style_and_selection_colors();
    void editor_context_menu_paints_system_background_data();
    void editor_context_menu_paints_system_background();
    void menu_appearance_switches_all_windows_without_changing_other_controls();
    void system_menus_preserve_platform_roles_and_follow_palette_changes_data();
    void system_menus_preserve_platform_roles_and_follow_palette_changes();
    void precision_menu_editor_uses_system_colors();
    void dock_context_menus_follow_menu_theme_data();
    void dock_context_menus_follow_menu_theme();
    void precision_menu_editor_uses_themed_colors_data();
    void precision_menu_editor_uses_themed_colors();
    void menu_theme_cache_keeps_bar_and_popup_colors_separate();
    void menu_theme_cache_reloads_named_themes_on_refresh();
    void menu_theme_cache_follows_the_fallback_palette();
    void themed_menu_icons_preserve_shape_and_follow_item_state_data();
    void themed_menu_icons_preserve_shape_and_follow_item_state();
    void context_menu_icons_follow_theme_changes_data();
    void context_menu_icons_follow_theme_changes();
    void disabled_menu_icons_keep_visible_state_data();
    void disabled_menu_icons_keep_visible_state();
    void disabled_context_menu_labels_are_dimmed_data();
    void disabled_context_menu_labels_are_dimmed();
};

void TestMenusUi::disabled_context_menu_labels_are_dimmed_data()
{
    QTest::addColumn<QString>("styleName");
    QTest::addColumn<QString>("background");
    for (const QString& style : {QStringLiteral("Fusion"), QStringLiteral("Windows")}) {
        QTest::newRow(qPrintable(style + QStringLiteral("-dark"))) << style << QStringLiteral("#232136");
        QTest::newRow(qPrintable(style + QStringLiteral("-light"))) << style << QStringLiteral("#e5eee8");
    }
}

void TestMenusUi::disabled_context_menu_labels_are_dimmed()
{
    QFETCH(QString, styleName);
    QFETCH(QString, background);
    MainWindowStateGuard guard;
    guard.settings->menuAppearance = Settings::MenuAppearanceSpeedCrunch;
    guard.settings->colorScheme = QStringLiteral("Custom");
    guard.settings->customColorSchemeJson = themeJsonString(
        QJsonObject{{QStringLiteral("background"), background}});
    QScopedPointer<QStyle> style(QStyleFactory::create(styleName));
    QVERIFY(style != nullptr);
    QMenu menu;
    menu.setStyle(style.data());
    menu.setFont(QFont(menu.font().family(), 16));
    QPixmap glyph(16, 16);
    glyph.fill(Qt::transparent);
    QPainter painter(&glyph);
    painter.fillRect(QRect(3, 3, 10, 10), Qt::black);
    painter.end();
    const QIcon original(glyph);
    QAction* action = menu.addAction(original, QStringLiteral("Copy selected expression\tCtrl+C"));
    menu.ensurePolished();
    MenuStyle::apply(&menu);
    menu.popup(QPoint(20, 20));
    QCoreApplication::processEvents();
    const auto coverage = [&](const QImage& image, const QRect& rect) {
        const QColor foreground = menu.palette().color(QPalette::WindowText);
        const QColor fill = menu.palette().color(QPalette::Window);
        const int dr = foreground.red() - fill.red();
        const int dg = foreground.green() - fill.green();
        const int db = foreground.blue() - fill.blue();
        const int distance = dr * dr + dg * dg + db * db;
        qreal maximum = 0;
        const QRect bounded = imageRectForWidgetRect(image, rect).intersected(image.rect());
        for (int y = bounded.top(); y <= bounded.bottom(); ++y) {
            for (int x = bounded.left(); x <= bounded.right(); ++x) {
                const QColor pixel = image.pixelColor(x, y);
                const qreal value = qreal((pixel.red() - fill.red()) * dr
                    + (pixel.green() - fill.green()) * dg + (pixel.blue() - fill.blue()) * db) / distance;
                maximum = qMax(maximum, value);
            }
        }
        return maximum;
    };
    const QRect row = menu.actionGeometry(action);
    const int shortcutWidth = menu.fontMetrics().horizontalAdvance(QStringLiteral("Ctrl+C")) + 20;
    const QRect label = row.adjusted(36, 2, -shortcutWidth, -2);
    const QRect shortcut(row.right() - shortcutWidth + 4, row.top() + 2,
                         shortcutWidth - 12, row.height() - 4);
    const QImage enabled = menu.grab().toImage();
    for (const QRect& rect : {label, shortcut})
        QVERIFY(coverage(enabled, rect) > 0.8);
    action->setEnabled(false);
    const QImage disabled = menu.grab().toImage();
    for (const QRect& rect : {label, shortcut}) {
        const qreal disabledCoverage = coverage(disabled, rect);
        QVERIFY2(disabledCoverage > 0.25 && disabledCoverage < 0.65,
                 qPrintable(QStringLiteral("Disabled text coverage: %1").arg(disabledCoverage)));
    }
    action->setEnabled(true);
    const QImage restored = menu.grab().toImage();
    for (const QRect& rect : {label, shortcut})
        QVERIFY(coverage(restored, rect) > 0.8);
    menu.hide();
    guard.settings->menuAppearance = Settings::MenuAppearanceSystem;
    MenuStyle::refresh();
    QCOMPARE(action->icon().cacheKey(), original.cacheKey());
}

void TestMenusUi::disabled_menu_icons_keep_visible_state_data()
{
    QTest::addColumn<QString>("background");
    QTest::addColumn<bool>("sourceFadesDisabled");
    QTest::addColumn<qreal>("scale");
    for (const bool dark : {false, true}) {
        for (const bool sourceFadesDisabled : {false, true}) {
            for (const qreal scale : {1.0, 1.25, 2.0}) {
                const QByteArray name = QStringLiteral("dark-%1-native-fade-%2-scale-%3")
                    .arg(dark).arg(sourceFadesDisabled).arg(scale).toLatin1();
                QTest::newRow(name.constData())
                    << (dark ? QStringLiteral("#232136") : QStringLiteral("#e5eee8"))
                    << sourceFadesDisabled << scale;
            }
        }
    }
}

void TestMenusUi::disabled_menu_icons_keep_visible_state()
{
    QFETCH(QString, background);
    QFETCH(bool, sourceFadesDisabled);
    QFETCH(qreal, scale);
    MainWindowStateGuard guard;
    guard.settings->menuAppearance = Settings::MenuAppearanceSpeedCrunch;
    guard.settings->colorScheme = QStringLiteral("Custom");
    guard.settings->customColorSchemeJson = themeJsonString(
        QJsonObject{{QStringLiteral("background"), background}});
    QIcon source;
    for (const QIcon::Mode mode : {QIcon::Normal, QIcon::Disabled}) {
        QPixmap glyph(32, 32);
        glyph.setDevicePixelRatio(2.0);
        glyph.fill(Qt::transparent);
        QPainter painter(&glyph);
        // Windows font icons use an opaque gray tint. Other engines can
        // reduce opacity, which must survive recoloring without another fade.
        const QColor color = mode == QIcon::Disabled
            ? QColor(128, 128, 128, sourceFadesDisabled ? 96 : 255) : QColor(Qt::black);
        painter.fillRect(QRect(3, 3, 10, 10), color);
        painter.end();
        source.addPixmap(glyph, mode);
    }
    QMenu menu;
    QAction* action = menu.addAction(source, QStringLiteral("Copy"));
    menu.ensurePolished();
    MenuStyle::apply(&menu);
    const QImage enabled = action->icon().pixmap(QSize(16, 16), scale, QIcon::Normal).toImage();
    const QImage disabled = action->icon().pixmap(QSize(16, 16), scale, QIcon::Disabled).toImage();
    const QPoint center(disabled.width() / 2, disabled.height() / 2);
    QCOMPARE(enabled.pixelColor(center).alpha(), 255);
    QCOMPARE(disabled.pixelColor(center).alpha(), sourceFadesDisabled ? 96 : 128);
    QCOMPARE(disabled.pixelColor(0, 0).alpha(), 0);
    const auto composite = [&](const QImage& glyph) {
        QImage surface(glyph.size(), QImage::Format_ARGB32_Premultiplied);
        surface.setDevicePixelRatio(glyph.devicePixelRatio());
        surface.fill(menu.palette().color(QPalette::Window));
        QPainter painter(&surface);
        painter.drawImage(QPointF(), glyph);
        return surface;
    };
    const QColor enabledPixel = composite(enabled).pixelColor(center);
    const QColor disabledPixel = composite(disabled).pixelColor(center);
    const QColor fill = menu.palette().color(QPalette::Window);
    QVERIFY(disabledPixel != enabledPixel);
    QVERIFY(qAbs(qGray(disabledPixel.rgb()) - qGray(fill.rgb()))
            < qAbs(qGray(enabledPixel.rgb()) - qGray(fill.rgb())));
    action->setEnabled(false);
    menu.popup(QPoint(20, 20));
    QCoreApplication::processEvents();
    const QImage rendered = menu.grab().toImage();
    const QRect row = imageRectForWidgetRect(rendered, menu.actionGeometry(action));
    const QRect iconArea(row.x(), row.y(), qRound(24 * rendered.devicePixelRatio()), row.height());
    QVERIFY(firstPixelMatchingColor(rendered, iconArea, disabledPixel, 4) != QPoint(-1, -1));
    action->setEnabled(true);
    const QImage restored = menu.grab().toImage();
    QVERIFY(firstPixelMatchingColor(restored, iconArea, enabledPixel, 4) != QPoint(-1, -1));
    menu.hide();
    guard.settings->menuAppearance = Settings::MenuAppearanceSystem;
    MenuStyle::refresh();
    QCOMPARE(action->icon().cacheKey(), source.cacheKey());
}

void TestMenusUi::themed_menu_icons_preserve_shape_and_follow_item_state_data()
{
    QTest::addColumn<int>("mode");
    QTest::addColumn<bool>("checked");
    QTest::addColumn<qreal>("scale");
    for (const QIcon::Mode mode : {QIcon::Normal, QIcon::Active, QIcon::Selected, QIcon::Disabled}) {
        for (const bool checked : {false, true}) {
            for (const qreal scale : {1.0, 1.25, 2.0}) {
                const QByteArray name = QStringLiteral("mode-%1-checked-%2-scale-%3")
                    .arg(int(mode)).arg(checked).arg(scale).toLatin1();
                QTest::newRow(name.constData()) << int(mode) << checked << scale;
            }
        }
    }
}

void TestMenusUi::themed_menu_icons_preserve_shape_and_follow_item_state()
{
    QFETCH(int, mode);
    QFETCH(bool, checked);
    QFETCH(qreal, scale);
    const auto iconMode = static_cast<QIcon::Mode>(mode);
    const auto state = checked ? QIcon::On : QIcon::Off;
    QIcon source;
    for (const QIcon::State sourceState : {QIcon::Off, QIcon::On}) {
        QPixmap mask(32, 32);
        mask.setDevicePixelRatio(2.0);
        mask.fill(Qt::transparent);
        QPainter painter(&mask);
        painter.fillRect(sourceState == QIcon::On ? QRect(3, 2, 4, 12) : QRect(2, 3, 12, 4), Qt::black);
        painter.fillRect(QRect(10, 10, 3, 3), QColor(0, 0, 0, 128));
        painter.end();
        for (const QIcon::Mode sourceMode : {QIcon::Normal, QIcon::Active, QIcon::Selected, QIcon::Disabled})
            source.addPixmap(mask, sourceMode, sourceState);
    }
    const qint64 sourceKey = source.cacheKey();
    const QPixmap original = source.pixmap(QSize(16, 16), scale, iconMode, state);
    QPalette palette;
    palette.setColor(QPalette::Active, QPalette::WindowText, QColor("#ecf4fa"));
    palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor("#a9afb9"));
    palette.setColor(QPalette::Active, QPalette::HighlightedText, QColor("#292035"));
    const QIcon themed = MenuStyle::themedIcon(source, palette);
    const QPixmap actual = themed.pixmap(QSize(16, 16), scale, iconMode, state);
    QCOMPARE(actual.size(), original.size());
    QCOMPARE(actual.devicePixelRatio(), original.devicePixelRatio());
    const QColor expected = palette.color(iconMode == QIcon::Disabled ? QPalette::Disabled : QPalette::Active,
        iconMode == QIcon::Active || iconMode == QIcon::Selected ? QPalette::HighlightedText : QPalette::WindowText);
    const QImage image = actual.toImage();
    const QImage mask = original.toImage();
    int opaquePixels = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor pixel = image.pixelColor(x, y);
            QCOMPARE(pixel.alpha(), mask.pixelColor(x, y).alpha());
            if (pixel.alpha() == 255) {
                QCOMPARE(pixel, expected);
                ++opaquePixels;
            }
        }
    }
    QVERIFY(opaquePixels > 0);
    QImage painted(actual.size(), QImage::Format_ARGB32_Premultiplied);
    painted.setDevicePixelRatio(actual.devicePixelRatio());
    painted.fill(Qt::transparent);
    QPainter painter(&painted);
    themed.paint(&painter, QRect(QPoint(), actual.deviceIndependentSize().toSize()), Qt::AlignCenter, iconMode, state);
    painter.end();
    QCOMPARE(painted, image.convertToFormat(QImage::Format_ARGB32_Premultiplied));
    QCOMPARE(source.cacheKey(), sourceKey);
    QCOMPARE(source.pixmap(QSize(16, 16), scale, iconMode, state).toImage(), mask);
    QVERIFY(MenuStyle::themedIcon(QIcon(), palette).isNull());
}

void TestMenusUi::context_menu_icons_follow_theme_changes_data()
{
    QTest::addColumn<QString>("styleName");
    QTest::addColumn<bool>("classic");
    QTest::newRow("fusion-modern") << QStringLiteral("Fusion") << false;
    QTest::newRow("fusion-classic") << QStringLiteral("Fusion") << true;
    QTest::newRow("windows-modern") << QStringLiteral("Windows") << false;
    QTest::newRow("windows-classic") << QStringLiteral("Windows") << true;
}

void TestMenusUi::context_menu_icons_follow_theme_changes()
{
    QFETCH(QString, styleName);
    QFETCH(bool, classic);
    MainWindowStateGuard guard;
    guard.settings->menuAppearance = Settings::MenuAppearanceSystem;
    guard.settings->colorScheme = QStringLiteral("Custom");
    guard.settings->editorDisplayStyle = classic ? Settings::EditorDisplayClassic : Settings::EditorDisplayModern;
    QScopedPointer<QStyle> style(QStyleFactory::create(styleName));
    QVERIFY(style != nullptr);
    Editor editor;
    editor.insert(QStringLiteral("123"));
    editor.selectAll();
    QScopedPointer<QMenu> menu(editor.createStandardContextMenu());
    menu->setStyle(style.data());
    menu->ensurePolished();
    MenuStyle::apply(menu.data());
    QAction* copy = menu->findChild<QAction*>(QStringLiteral("edit-copy"));
    QVERIFY(copy != nullptr);
    const QIcon native = copy->icon();
    // Supply a glyph on offscreen platforms without native icons.
    QPixmap glyph(16, 16);
    glyph.fill(Qt::transparent);
    QPainter painter(&glyph);
    painter.fillRect(QRect(4, 3, 8, 10), Qt::black);
    painter.end();
    if (copy->icon().isNull())
        copy->setIcon(QIcon(glyph));
    const QIcon original = copy->icon();
    const auto verify = [&](QAction* action) {
        QVERIFY(!action->icon().isNull());
        for (const QIcon::Mode mode : {QIcon::Normal, QIcon::Active, QIcon::Selected, QIcon::Disabled}) {
            const QImage image = action->icon().pixmap(QSize(16, 16), mode).toImage();
            const QColor expected = menu->palette().color(mode == QIcon::Disabled ? QPalette::Disabled : QPalette::Active,
                mode == QIcon::Active || mode == QIcon::Selected ? QPalette::HighlightedText : QPalette::WindowText);
            QVERIFY(firstPixelMatchingColor(image, image.rect(), expected, 4) != QPoint(-1, -1));
        }
        menu->popup(QPoint(20, 20));
        QCoreApplication::processEvents();
        for (const bool selected : {false, true}) {
            menu->setActiveAction(selected ? action : nullptr);
            const QImage rendered = menu->grab().toImage();
            const qreal dpr = rendered.devicePixelRatio();
            const QRect row = menu->actionGeometry(action);
            const QRect iconArea(qRound(row.x() * dpr), qRound(row.y() * dpr),
                                 qRound(qMin(24, row.width()) * dpr), qRound(row.height() * dpr));
            QVERIFY(firstPixelMatchingColor(rendered, iconArea,
                menu->palette().color(selected ? QPalette::HighlightedText : QPalette::WindowText), 4)
                != QPoint(-1, -1));
        }
        menu->setActiveAction(nullptr);
        menu->hide();
    };
    for (const QString& background : {QStringLiteral("#232136"), QStringLiteral("#e5eee8")}) {
        guard.settings->menuAppearance = Settings::MenuAppearanceSpeedCrunch;
        guard.settings->customColorSchemeJson = themeJsonString(
            QJsonObject{{QStringLiteral("background"), background}});
        MenuStyle::refresh();
        QCoreApplication::processEvents();
        verify(copy);
        const qint64 themedKey = copy->icon().cacheKey();
        MenuStyle::apply(menu.data());
        QCOMPARE(copy->icon().cacheKey(), themedKey);
        QIcon replacement(glyph);
        copy->setIcon(replacement);
        verify(copy);
        QAction* added = menu->addAction(QIcon(glyph), QStringLiteral("Later action"));
        verify(added);
        guard.settings->menuAppearance = Settings::MenuAppearanceSystem;
        MenuStyle::refresh();
        QCOMPARE(copy->icon().cacheKey(), replacement.cacheKey());
        copy->setIcon(original);
    }
    if (!native.isNull())
        QCOMPARE(copy->icon().cacheKey(), native.cacheKey());
}

void TestMenusUi::menu_theme_cache_keeps_bar_and_popup_colors_separate()
{
    MainWindowStateGuard guard;
    guard.settings->menuAppearance = Settings::MenuAppearanceSpeedCrunch;
    guard.settings->colorScheme = QStringLiteral("Custom");
    QMenu popup;
    QMenuBar bar;
    bar.setNativeMenuBar(false);
    for (const QColor base : {QColor("#232136"), QColor("#e5eee8")}) {
        guard.settings->customColorSchemeJson = themeJsonString(
            QJsonObject{{QStringLiteral("background"), base.name()}});
        MenuStyle::apply(&popup);
        MenuStyle::apply(&bar);
        const auto shades = generateOklchShades(base, 6, themePolarityForBackground(base));
        QCOMPARE(popup.palette().color(QPalette::Window), shades.at(UiConfig::DockHeaderShade));
        QCOMPARE(bar.palette().color(QPalette::Window), shades.at(UiConfig::WindowBackgroundShade));
        QCOMPARE(popup.palette().color(QPalette::Highlight), shades.at(UiConfig::DockUnfocusedSelectedItemShade));
        QCOMPARE(bar.palette().color(QPalette::Highlight), popup.palette().color(QPalette::Highlight));
        QMenu override;
        const QColor overrideFill("#123456");
        MenuStyle::setThemeColors(&override, overrideFill, Qt::white, Qt::blue, Qt::white);
        QCOMPARE(override.palette().color(QPalette::Window), overrideFill);
        MenuStyle::apply(&popup);
        MenuStyle::apply(&bar);
        QCOMPARE(popup.palette().color(QPalette::Window), shades.at(UiConfig::DockHeaderShade));
        QCOMPARE(bar.palette().color(QPalette::Window), shades.at(UiConfig::WindowBackgroundShade));
    }
}

void TestMenusUi::menu_theme_cache_reloads_named_themes_on_refresh()
{
    MainWindowStateGuard guard;
    guard.settings->menuAppearance = Settings::MenuAppearanceSpeedCrunch;
    guard.settings->colorScheme = QStringLiteral("Menu cache test");
    const QString directory = ColorScheme::fileSystemSearchPaths().first();
    QVERIFY(QDir().mkpath(directory));
    const QString path = QDir(directory).filePath(guard.settings->colorScheme + QStringLiteral(".json"));
    const auto removeTheme = qScopeGuard([&]() { QFile::remove(path); });
    QMenu popup;
    for (const QColor base : {QColor("#232136"), QColor("#e5eee8")}) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        const QByteArray json = themeJsonString(
            QJsonObject{{QStringLiteral("background"), base.name()}}).toUtf8();
        QCOMPARE(file.write(json), qint64(json.size()));
        file.close();
        MenuStyle::refresh();
        MenuStyle::apply(&popup);
        const auto shades = generateOklchShades(base, 6, themePolarityForBackground(base));
        QCOMPARE(popup.palette().color(QPalette::Window), shades.at(UiConfig::DockHeaderShade));
    }
}

void TestMenusUi::menu_theme_cache_follows_the_fallback_palette()
{
    MainWindowStateGuard guard;
    guard.settings->menuAppearance = Settings::MenuAppearanceSpeedCrunch;
    guard.settings->colorScheme = QStringLiteral("Custom");
    guard.settings->customColorSchemeJson = themeJsonString(QJsonObject());
    const QPalette original = QApplication::palette();
    const auto restorePalette = qScopeGuard([&]() {
        QApplication::setPalette(original);
        MenuStyle::refresh();
    });
    QMenu popup;
    for (const QColor base : {QColor("#232136"), QColor("#e5eee8")}) {
        QPalette palette = original;
        palette.setColor(QPalette::Base, base);
        QApplication::setPalette(palette);
        MenuStyle::apply(&popup);
        const auto shades = generateOklchShades(base, 6, themePolarityForBackground(base));
        QCOMPARE(popup.palette().color(QPalette::Window), shades.at(UiConfig::DockHeaderShade));
    }
}

void TestMenusUi::initTestCase()
{
    // Fail before a file-operation test can open an unhandled error dialog.
    for (const QString& path : {Settings::getConfigPath(), Settings::getDataPath(),
                                Settings::getCachePath()}) {
        QVERIFY2(QDir().mkpath(path),
                 qPrintable(QStringLiteral("Cannot create test storage at %1").arg(path)));
        QTemporaryFile probe(QDir(path).filePath(QStringLiteral("storage-probe-XXXXXX")));
        QVERIFY2(probe.open(),
                 qPrintable(QStringLiteral("Cannot write test storage at %1: %2")
                                .arg(path, probe.errorString())));
    }
}

void TestMenusUi::menu_bar_keeps_themed_contrast_with_platform_palette_data()
{
    QTest::addColumn<bool>("darkPlatform");
    QTest::addColumn<QString>("background");
    QTest::newRow("dark-desktop-dark-theme") << true << QStringLiteral("#232136");
    QTest::newRow("light-desktop-dark-theme") << false << QStringLiteral("#232136");
    QTest::newRow("dark-desktop-light-theme") << true << QStringLiteral("#e5eee8");
    QTest::newRow("light-desktop-light-theme") << false << QStringLiteral("#e5eee8");
}

void TestMenusUi::menu_bar_keeps_themed_contrast_with_platform_palette()
{
    QFETCH(bool, darkPlatform);
    QFETCH(QString, background);
    MainWindowStateGuard guard;
    guard.settings->menuAppearance = Settings::MenuAppearanceSpeedCrunch;
    guard.settings->colorScheme = QStringLiteral("Custom");
    guard.settings->customColorSchemeJson = themeJsonString(
        QJsonObject{{QStringLiteral("background"), background}});

    MainWindow window;
    QMenuBar* bar = window.menuBar();
    bar->setNativeMenuBar(false);
    QPalette platformPalette = QApplication::palette();
    for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive,
                                            QPalette::Disabled}) {
        platformPalette.setColor(group, QPalette::Window,
                                 darkPlatform ? QColor("#1e1e1e") : QColor("#efefef"));
        platformPalette.setColor(group, QPalette::WindowText,
                                 darkPlatform ? QColor("#eff0f1") : QColor("#232629"));
        platformPalette.setColor(group, QPalette::ButtonText,
                                 platformPalette.color(group, QPalette::WindowText));
    }
    auto* platformStyle = new MenuBarPaletteTestStyle(platformPalette);
    platformStyle->setParent(bar);
    bar->setStyle(platformStyle);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    window.colorSchemeChanged();
    QCoreApplication::processEvents();

    const auto verifyTheme = [bar](const QColor& base) {
        const QVector<QColor> shades = generateOklchShades(base, 6, themePolarityForBackground(base));
        const QColor fill = shades.at(UiConfig::WindowBackgroundShade);
        const QColor text = aaForegroundForBackground(fill);
        const QColor selectedFill = shades.at(UiConfig::DockUnfocusedSelectedItemShade);
        const QColor selectedText = aaForegroundForBackground(selectedFill);
        for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive,
                                                QPalette::Disabled}) {
            QCOMPARE(bar->palette().color(group, QPalette::Window), fill);
            QCOMPARE(bar->palette().color(group, QPalette::WindowText), text);
            QCOMPARE(bar->palette().color(group, QPalette::ButtonText), text);
            QCOMPARE(bar->palette().color(group, QPalette::Highlight), selectedFill);
            QCOMPARE(bar->palette().color(group, QPalette::HighlightedText), selectedText);
        }

        const QImage rendered = bar->grab().toImage();
        const qreal dpr = rendered.devicePixelRatio();
        QCOMPARE(rendered.pixelColor(qRound((bar->width() - 4) * dpr),
                                     qRound(bar->height() / 2.0 * dpr)).name(), fill.name());

        // Check painted glyphs as well as palette roles. Native styles may
        // choose a different text role or selection color when drawing items.
        for (const QStyle::State state : {QStyle::State(QStyle::State_Enabled | QStyle::State_Active),
                                          QStyle::State(QStyle::State_Enabled),
                                          QStyle::State(QStyle::State_None),
                                          QStyle::State(QStyle::State_Enabled | QStyle::State_Selected),
                                          QStyle::State(QStyle::State_Enabled | QStyle::State_Selected
                                                        | QStyle::State_Sunken)}) {
            QStyleOptionMenuItem option;
            option.initFrom(bar);
            option.state = state;
            option.rect = QRect(0, 0, 100, bar->height());
            option.text = QStringLiteral("Session");
            const bool selected = state.testFlag(QStyle::State_Selected);
            const QColor expectedFill = selected ? selectedFill : fill;
            const QColor expectedText = selected ? selectedText : text;
            QImage item(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
            item.fill(fill);
            QPainter painter(&item);
            painter.setFont(bar->font());
            bar->style()->drawControl(QStyle::CE_MenuBarItem, &option, &painter, bar);
            painter.end();
            QCOMPARE(item.pixelColor(1, 1).name(), expectedFill.name());
            int textPixels = 0;
            for (int y = 0; y < item.height(); ++y) {
                for (int x = 0; x < item.width(); ++x) {
                    const QColor pixel = item.pixelColor(x, y);
                    if (qAbs(pixel.red() - expectedText.red()) < 24
                        && qAbs(pixel.green() - expectedText.green()) < 24
                        && qAbs(pixel.blue() - expectedText.blue()) < 24)
                        ++textPixels;
                }
            }
            QVERIFY2(textPixels > 10, "Menu bar text does not use the contrasting theme color.");
        }
    };
    verifyTheme(QColor(background));

    const QColor switchedBackground(background == QLatin1String("#232136")
                                    ? QStringLiteral("#e5eee8") : QStringLiteral("#232136"));
    guard.settings->customColorSchemeJson = themeJsonString(
        QJsonObject{{QStringLiteral("background"), switchedBackground.name()}});
    window.colorSchemeChanged();
    QCoreApplication::processEvents();
    verifyTheme(switchedBackground);
}

void TestMenusUi::menu_bar_restores_native_painting_after_theme_switch_data()
{
    QTest::addColumn<QString>("styleName");
    QTest::newRow("fusion") << QStringLiteral("Fusion");
    QTest::newRow("windows") << QStringLiteral("Windows");
}

void TestMenusUi::menu_bar_restores_native_painting_after_theme_switch()
{
    QFETCH(QString, styleName);
    MainWindowStateGuard guard;
    guard.settings->menuAppearance = Settings::MenuAppearanceSystem;
    MainWindow window(false);
    QMenuBar* bar = window.menuBar();
    bar->setNativeMenuBar(false);
    QScopedPointer<QStyle> style(QStyleFactory::create(styleName));
    QVERIFY(style != nullptr);
    bar->setStyle(style.data());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    MenuStyle::refresh();
    QCoreApplication::processEvents();
    const bool nativeStyledBackground = bar->testAttribute(Qt::WA_StyledBackground);
    const QImage nativeRendering = bar->grab().toImage();
    QMenu* appearance = window.findChild<QMenu*>(QStringLiteral("MenuAppearanceMenu"));
    QVERIFY(appearance != nullptr);
    for (int cycle = 0; cycle < 2; ++cycle) {
        appearance->actions().at(1)->trigger();
        QCoreApplication::processEvents();
        QVERIFY(bar->testAttribute(Qt::WA_StyledBackground));
        // Switch from an open menu, as users do, then let the style finish.
        bool switched = false;
        QTimer::singleShot(0, &window, [&]() {
            appearance->actions().at(0)->trigger();
            appearance->close();
            switched = true;
        });
        appearance->exec(bar->mapToGlobal(QPoint(0, bar->height())));
        QVERIFY(switched);
        QCoreApplication::processEvents();
        QVERIFY(bar->styleSheet().isEmpty());
        QCOMPARE(bar->testAttribute(Qt::WA_StyledBackground), nativeStyledBackground);
        QCOMPARE(bar->grab().toImage(), nativeRendering);
    }
    bar->setStyle(nullptr);
}

void TestMenusUi::dock_dropdowns_follow_menu_appearance_data()
{
    QTest::addColumn<bool>("darkSystem");
    QTest::addColumn<bool>("floating");
    QTest::newRow("dark-attached") << true << false;
    QTest::newRow("light-attached") << false << false;
    QTest::newRow("dark-floating") << true << true;
    QTest::newRow("light-floating") << false << true;
}

void TestMenusUi::dock_dropdowns_follow_menu_appearance()
{
    QFETCH(bool, darkSystem);
    QFETCH(bool, floating);
    MainWindowStateGuard guard;
    const QPalette original = QApplication::palette();
    const auto restorePalette = qScopeGuard([&]() {
        QApplication::setPalette(original);
        MenuStyle::refresh();
    });
    QPalette native = original;
    for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
        for (const QPalette::ColorRole role : {QPalette::Window, QPalette::Base, QPalette::Button})
            native.setColor(group, role, QColor(darkSystem ? "#202428" : "#eeeeee"));
        for (const QPalette::ColorRole role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText})
            native.setColor(group, role, QColor(group == QPalette::Disabled ? "#808890"
                                                                         : (darkSystem ? "#f0f0f0" : "#202020")));
        native.setColor(group, QPalette::Highlight, QColor("#354faf"));
        native.setColor(group, QPalette::HighlightedText, QColor("#ffffff"));
    }
    QApplication::setPalette(native);
    guard.settings->menuAppearance = Settings::MenuAppearanceSystem;
    guard.settings->colorScheme = QStringLiteral("Custom");
    guard.settings->customColorSchemeJson = themeJsonString(
        QJsonObject{{QStringLiteral("background"), darkSystem ? "#e5eee8" : "#232136"}});
    guard.settings->constantsDockVisible = true;
    guard.settings->functionsDockVisible = true;
    MainWindow window(false);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QMenu* appearance = window.findChild<QMenu*>(QStringLiteral("MenuAppearanceMenu"));
    QVERIFY(appearance != nullptr);
    for (const char* dockName : {"ConstantsDock", "FunctionsDock"}) {
        appearance->actions().at(0)->trigger();
        QDockWidget* dock = window.findChild<QDockWidget*>(QString::fromLatin1(dockName));
        QVERIFY(dock != nullptr);
        dock->setFloating(floating);
        dock->show();
        QComboBox* combo = dock->findChild<QComboBox*>();
        QVERIFY(combo != nullptr);
        QCoreApplication::processEvents();
        QAbstractItemView* view = combo->view();
        const auto nativeFrame = view->frameShape();
        const bool nativeViewportFill = view->viewport()->autoFillBackground();
        const QPalette closedPalette = combo->palette();
        for (const int mode : {0, 1, 0, 1, 0}) {
            appearance->actions().at(mode)->trigger();
            combo->showPopup();
            const auto closePopup = qScopeGuard([combo]() { combo->hidePopup(); });
            QCoreApplication::processEvents();
            QVERIFY(view->isVisible());
            QWidget* popup = view->window();
            QCOMPARE(combo->palette(), closedPalette);
            if (mode == 0) {
                QVERIFY(view->styleSheet().isEmpty());
                QVERIFY(view->verticalScrollBar()->styleSheet().isEmpty());
                QVERIFY(!combo->styleSheet().contains(QStringLiteral("QComboBox QAbstractItemView")));
                QCOMPARE(view->frameShape(), nativeFrame);
                QCOMPARE(view->viewport()->autoFillBackground(), nativeViewportFill);
                QVERIFY(popup->mask().isEmpty());
                for (QWidget* surface : {static_cast<QWidget*>(view), view->viewport(), popup}) {
                    const QPalette expected = MenuStyle::systemPalette(surface);
                    for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive,
                                                            QPalette::Disabled}) {
                        for (const QPalette::ColorRole role : {QPalette::Base, QPalette::Text,
                                                               QPalette::Highlight, QPalette::HighlightedText})
                            QCOMPARE(surface->palette().color(group, role), expected.color(group, role));
                    }
                }
                const QImage image = view->viewport()->grab().toImage();
                int backgroundPixels = 0;
                for (int y = 0; y < image.height(); ++y) {
                    for (int x = 0; x < image.width(); ++x)
                        backgroundPixels += image.pixelColor(x, y).name() == native.color(QPalette::Base).name();
                }
                QVERIFY2(backgroundPixels > image.width() * image.height() / 3,
                         qPrintable(QStringLiteral("%1: expected %2, view %3, viewport %4, painted %5")
                             .arg(QString::fromLatin1(dockName), native.color(QPalette::Base).name(),
                                  view->palette().color(QPalette::Base).name(),
                                  view->viewport()->palette().color(QPalette::Base).name(),
                                  image.pixelColor(5, 5).name())));
            } else {
                QVERIFY(!view->styleSheet().isEmpty());
                QVERIFY(!view->verticalScrollBar()->styleSheet().isEmpty());
                QVERIFY(!popup->mask().isEmpty());
                QCOMPARE(view->frameShape(), QFrame::NoFrame);
                QVERIFY(view->palette().color(QPalette::Base) != native.color(QPalette::Base));
            }
            // Refresh an already-open dropdown without changing other controls.
            appearance->actions().at(1 - mode)->trigger();
            QCoreApplication::processEvents();
            QCOMPARE(view->styleSheet().isEmpty(), mode == 1);
        }
        appearance->actions().at(0)->trigger();
        combo->showPopup();
        const auto closePopup = qScopeGuard([combo]() { combo->hidePopup(); });
        for (const QPalette::ColorRole role : {QPalette::Window, QPalette::Base, QPalette::Button})
            native.setColor(role, QColor(darkSystem ? "#292e36" : "#e1f0fd"));
        native.setColor(QPalette::Highlight, QColor("#6850aa"));
        QApplication::setPalette(native);
        QCoreApplication::processEvents();
        QCOMPARE(view->palette().color(QPalette::Base), native.color(QPalette::Base));
        QCOMPARE(view->palette().color(QPalette::Highlight), native.color(QPalette::Highlight));
        guard.settings->customColorSchemeJson = themeJsonString(
            QJsonObject{{QStringLiteral("background"), QStringLiteral("#433229")}});
        window.colorSchemeChanged();
        QCoreApplication::processEvents();
        QVERIFY(view->styleSheet().isEmpty());
        QCOMPARE(view->palette().color(QPalette::Base), native.color(QPalette::Base));
        QCOMPARE(view->viewport()->palette().color(QPalette::Base), native.color(QPalette::Base));
    }
}

void TestMenusUi::system_dropdowns_use_the_native_popup_selection_palette_data()
{
    QTest::addColumn<bool>("editable");
    QTest::newRow("menu-popup") << false;
    QTest::newRow("list-popup") << true;
}

void TestMenusUi::system_dropdowns_use_the_native_popup_selection_palette()
{
    QFETCH(bool, editable);
    MainWindowStateGuard guard;
    const QPalette originalMenu = QApplication::palette("QMenu");
    const QPalette originalList = QApplication::palette("QListView");
    const auto restorePalettes = qScopeGuard([&]() {
        QApplication::setPalette(originalMenu, "QMenu");
        QApplication::setPalette(originalList, "QListView");
        MenuStyle::refresh();
    });
    QPalette nativeMenu = originalMenu;
    QPalette nativeList = originalList;
    for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
        nativeMenu.setColor(group, QPalette::Highlight, QColor("#427dc0"));
        nativeMenu.setColor(group, QPalette::HighlightedText, QColor("#fefefe"));
        nativeList.setColor(group, QPalette::Highlight, QColor("#1a4266"));
        nativeList.setColor(group, QPalette::HighlightedText, QColor("#eeeeee"));
    }
    QApplication::setPalette(nativeMenu, "QMenu");
    QApplication::setPalette(nativeList, "QListView");
    guard.settings->menuAppearance = Settings::MenuAppearanceSystem;
    guard.settings->constantsDockVisible = true;
    MainWindow window(false);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto* dock = window.findChild<QDockWidget*>(QStringLiteral("ConstantsDock"));
    QVERIFY(dock != nullptr);
    auto* combo = dock->findChild<QComboBox*>();
    QVERIFY(combo != nullptr);
    combo->setEditable(editable);
    QStyleOptionComboBox option;
    option.initFrom(combo);
    option.editable = editable;
    const bool menuPopup = QApplication::style()->styleHint(QStyle::SH_ComboBox_Popup, &option, combo);
    const QPalette expected = menuPopup ? nativeMenu : nativeList;
    QMenu* appearance = window.findChild<QMenu*>(QStringLiteral("MenuAppearanceMenu"));
    QVERIFY(appearance != nullptr);
    for (const int mode : {0, 1, 0}) {
        appearance->actions().at(mode)->trigger();
        combo->showPopup();
        const auto closePopup = qScopeGuard([combo]() { combo->hidePopup(); });
        QCoreApplication::processEvents();
        if (mode != 0)
            continue;
        for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
            QCOMPARE(combo->view()->palette().color(group, QPalette::Highlight),
                     expected.color(group, QPalette::Highlight));
            QCOMPARE(combo->view()->palette().color(group, QPalette::HighlightedText),
                     expected.color(group, QPalette::HighlightedText));
        }
        QStyleOptionViewItem selected;
        selected.initFrom(combo->view());
        selected.rect = QRect(0, 0, 240, 40);
        selected.state = QStyle::State_Active | QStyle::State_Enabled | QStyle::State_Selected;
        selected.palette.setCurrentColorGroup(QPalette::Active);
        QImage image(selected.rect.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        combo->itemDelegate()->paint(&painter, selected, combo->model()->index(0, combo->modelColumn()));
        painter.end();
        QCOMPARE(image.pixelColor(220, 20).name(), expected.color(QPalette::Active, QPalette::Highlight).name());
    }
}

void TestMenusUi::macos_system_menus_use_cocoa_style_and_selection_colors()
{
#if defined(Q_OS_MACOS)
    if (QApplication::platformName() != QLatin1String("cocoa")) {
        QTest::qSkip("Native macOS theme verification requires the Cocoa backend.", __FILE__, __LINE__);
        return;
    }
    MainWindowStateGuard guard;
    guard.settings->menuAppearance = Settings::MenuAppearanceSystem;
    guard.settings->constantsDockVisible = true;
    MainWindow window(false);
    QStyle* nativeStyle = QApplication::style();
    while (auto* proxy = qobject_cast<QProxyStyle*>(nativeStyle))
        nativeStyle = proxy->baseStyle();
    QCOMPARE(nativeStyle->objectName(), QStringLiteral("macos"));
    const QPalette nativeMenu = QApplication::palette("QMenu");
    QScopedPointer<QMenu> menu(window.findChild<Editor*>()->createStandardContextMenu());
    menu->ensurePolished();
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto* dock = window.findChild<QDockWidget*>(QStringLiteral("ConstantsDock"));
    QVERIFY(dock != nullptr);
    auto* combo = dock->findChild<QComboBox*>();
    QVERIFY(combo != nullptr);
    combo->showPopup();
    const auto closePopup = qScopeGuard([combo]() { combo->hidePopup(); });
    QCoreApplication::processEvents();
    for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
        QCOMPARE(menu->palette().color(group, QPalette::Highlight), nativeMenu.color(group, QPalette::Highlight));
        QCOMPARE(combo->view()->palette().color(group, QPalette::Highlight),
                 nativeMenu.color(group, QPalette::Highlight));
        QCOMPARE(combo->view()->palette().color(group, QPalette::HighlightedText),
                 nativeMenu.color(group, QPalette::HighlightedText));
    }
#else
    QTest::qSkip("Native macOS theme verification requires macOS.", __FILE__, __LINE__);
#endif
}

void TestMenusUi::editor_context_menu_paints_system_background_data()
{
    QTest::addColumn<QString>("editorBackground");
    QTest::newRow("dark-editor") << QStringLiteral("#232136");
    QTest::newRow("light-editor") << QStringLiteral("#e5eee8");
}

void TestMenusUi::editor_context_menu_paints_system_background()
{
    QFETCH(QString, editorBackground);
    MainWindowStateGuard guard;
    guard.settings->menuAppearance = Settings::MenuAppearanceSystem;
    guard.settings->colorScheme = QStringLiteral("Custom");
    guard.settings->customColorSchemeJson = themeJsonString(
        QJsonObject{{QStringLiteral("background"), editorBackground}});
    MainWindow window(false);
    auto* editor = window.findChild<Editor*>();
    QVERIFY(editor != nullptr);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    const auto openContextMenu = [&]() {
        const QPoint pos(8, 8);
        QContextMenuEvent event(QContextMenuEvent::Mouse, pos,
                                editor->viewport()->mapToGlobal(pos));
        QCoreApplication::sendEvent(editor->viewport(), &event);
        auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
        if (menu != nullptr)
            menu->setAttribute(Qt::WA_DeleteOnClose, false);
        return menu;
    };
    QScopedPointer<QMenu> context(openContextMenu());
    QVERIFY(context != nullptr);
    QCOMPARE(context->parentWidget(), editor->viewport());

    // Palette checks alone miss inherited stylesheets that suppress painting.
    const auto panelImage = [](QMenu* menu) {
        menu->ensurePolished();
        QImage image(180, 120, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QStyleOption option;
        option.initFrom(menu);
        option.rect = image.rect();
        option.state = QStyle::State_Enabled | QStyle::State_Active;
        QPainter painter(&image);
        menu->style()->drawPrimitive(QStyle::PE_PanelMenu, &option, &painter, menu);
        painter.end();
        return image;
    };
    const auto verifyBackground = [&](QMenu* menu) {
        QMenu reference;
        const QImage expected = panelImage(&reference);
        const QPoint center = expected.rect().center();
        QVERIFY(expected.pixelColor(center).alpha() > 0);
        const QImage actual = panelImage(menu);
        QCOMPARE(actual.pixelColor(center), expected.pixelColor(center));
    };
    verifyBackground(context.data());
    context->close();

    // Check both a retained popup and a newly created one after switching back.
    guard.settings->menuAppearance = Settings::MenuAppearanceSpeedCrunch;
    MenuStyle::refresh();
    context->ensurePolished();
    const QImage themed = panelImage(context.data());
    QCOMPARE(themed.pixelColor(themed.rect().center()).rgba(),
             context->palette().color(QPalette::Window).rgba());
    guard.settings->menuAppearance = Settings::MenuAppearanceSystem;
    MenuStyle::refresh();
    QCoreApplication::processEvents();
    verifyBackground(context.data());
    QScopedPointer<QMenu> fresh(openContextMenu());
    QVERIFY(fresh != nullptr);
    verifyBackground(fresh.data());
}

void TestMenusUi::menu_appearance_defaults_to_theme_and_persists()
{
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;
    QCOMPARE(settings->menuAppearance, Settings::MenuAppearanceSpeedCrunch);
    const QString scheme = settings->colorScheme;
    for (const Settings::MenuAppearance appearance : {Settings::MenuAppearanceSpeedCrunch,
                                                     Settings::MenuAppearanceSystem}) {
        settings->menuAppearance = appearance;
        settings->save();
        settings->menuAppearance = appearance == Settings::MenuAppearanceSystem
            ? Settings::MenuAppearanceSpeedCrunch : Settings::MenuAppearanceSystem;
        settings->load();
        QCOMPARE(settings->menuAppearance, appearance);
        QCOMPARE(settings->colorScheme, scheme);
    }
    QSettings persisted(Settings::getConfigPath() + QStringLiteral("/SpeedCrunch.ini"),
                        QSettings::IniFormat);
    const QString key = QStringLiteral("SpeedCrunch/Display/MenuAppearance");
    persisted.remove(key);
    persisted.sync();
    settings->load();
    QCOMPARE(settings->menuAppearance, Settings::MenuAppearanceSpeedCrunch);
    persisted.setValue(key, 99);
    persisted.sync();
    settings->load();
    QCOMPARE(settings->menuAppearance, Settings::MenuAppearanceSpeedCrunch);
}

void TestMenusUi::menu_appearance_switches_all_windows_without_changing_other_controls()
{
    MainWindowStateGuard guard;
    guard.settings->menuAppearance = Settings::MenuAppearanceSystem;
    guard.settings->colorScheme = QStringLiteral("Custom");
    guard.settings->customColorSchemeJson = themeJsonString(
        QJsonObject{{QStringLiteral("background"), QStringLiteral("#232136")}});
    guard.settings->constantsDockVisible = true;
    MainWindow first(false);
    MainWindow second(false);
    first.menuBar()->setNativeMenuBar(false);
    second.menuBar()->setNativeMenuBar(false);
    first.show();
    second.show();
    QVERIFY(QTest::qWaitForWindowExposed(&first));
    QVERIFY(QTest::qWaitForWindowExposed(&second));
    MenuStyle::refresh();
    QMenu* firstAppearance = first.findChild<QMenu*>(QStringLiteral("MenuAppearanceMenu"));
    QMenu* secondAppearance = second.findChild<QMenu*>(QStringLiteral("MenuAppearanceMenu"));
    QVERIFY(firstAppearance != nullptr);
    QVERIFY(secondAppearance != nullptr);
    QVERIFY(firstAppearance->actions().size() >= 2);
    QVERIFY(firstAppearance->actions().at(0)->isChecked());
    QVERIFY(secondAppearance->actions().at(0)->isChecked());
    Editor* editor = first.findChild<Editor*>();
    ResultDisplay* display = first.findChild<ResultDisplay*>();
    QDockWidget* constants = first.findChild<QDockWidget*>(QStringLiteral("ConstantsDock"));
    QVERIFY(editor != nullptr);
    QVERIFY(display != nullptr);
    QVERIFY(constants != nullptr);
    const QPalette editorPalette = editor->palette();
    const QPalette displayPalette = display->palette();
    const QPalette dockPalette = constants->palette();
    const QString editorStyle = editor->styleSheet();

    // Keep a context menu alive across both switches to catch stale palettes
    // and styles that would otherwise survive until the next popup.
    QScopedPointer<QMenu> context(editor->createStandardContextMenu());
    context->ensurePolished();
    QVERIFY(context->styleSheet().isEmpty());
    firstAppearance->actions().at(1)->trigger();
    QCOMPARE(guard.settings->menuAppearance, Settings::MenuAppearanceSpeedCrunch);
    QVERIFY(firstAppearance->actions().at(1)->isChecked());
    QVERIFY(secondAppearance->actions().at(1)->isChecked());
    for (QMenuBar* bar : {first.menuBar(), second.menuBar()})
        QVERIFY(!bar->styleSheet().isEmpty());
    QVERIFY(!context->styleSheet().isEmpty());
    QMenu* child = context->addMenu(QStringLiteral("Nested"));
    child->addAction(QStringLiteral("Disabled"))->setEnabled(false);
    child->ensurePolished();
    QVERIFY(!child->styleSheet().isEmpty());
    QCOMPARE(child->palette().color(QPalette::Window).name(), context->palette().color(QPalette::Window).name());

    secondAppearance->actions().at(0)->trigger();
    QCOMPARE(guard.settings->menuAppearance, Settings::MenuAppearanceSystem);
    QVERIFY(firstAppearance->actions().at(0)->isChecked());
    QVERIFY(secondAppearance->actions().at(0)->isChecked());
    // Desktop styles can change palettes while menus are polished. Check
    // color restoration separately with an explicit style palette.
    for (QWidget* menu : {static_cast<QWidget*>(first.menuBar()),
                          static_cast<QWidget*>(second.menuBar()),
                          static_cast<QWidget*>(context.data()), static_cast<QWidget*>(child)}) {
        QVERIFY(menu->styleSheet().isEmpty());
    }
    QCOMPARE(editor->palette(), editorPalette);
    QCOMPARE(display->palette(), displayPalette);
    QCOMPARE(constants->palette(), dockPalette);
    QCOMPARE(editor->styleSheet(), editorStyle);
}

void TestMenusUi::system_menus_restore_style_polished_foreground_and_hover_colors_data()
{
    QTest::addColumn<QColor>("background");
    QTest::newRow("light-style") << QColor("#eeeeee");
    QTest::newRow("dark-style") << QColor("#454358");
}

void TestMenusUi::system_menus_restore_style_polished_foreground_and_hover_colors()
{
    QFETCH(QColor, background);
    MainWindowStateGuard guard;
    guard.settings->menuAppearance = Settings::MenuAppearanceSpeedCrunch;
    guard.settings->colorScheme = QStringLiteral("Custom");
    guard.settings->customColorSchemeJson = themeJsonString(
        QJsonObject{{QStringLiteral("background"), QStringLiteral("#232136")}});
    MainWindow window(false);
    QMenuBar* bar = window.menuBar();
    bar->setNativeMenuBar(false);
    QPalette native = QApplication::palette(bar);
    const bool dark = background.lightness() < 128;
    const QColor foreground(dark ? "#eeeeee" : "#242424");
    const QColor disabledForeground(dark ? "#999999" : "#777777");
    for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
        native.setColor(group, QPalette::Window, background);
        native.setColor(group, QPalette::Button, background);
        for (const QPalette::ColorRole role : {QPalette::WindowText, QPalette::ButtonText})
            native.setColor(group, role, group == QPalette::Disabled ? disabledForeground : foreground);
        native.setColor(group, QPalette::Highlight, QColor(dark ? "#656378" : "#cccccc"));
        native.setColor(group, QPalette::HighlightedText, foreground);
    }
    auto* style = new MenuBarPaletteTestStyle(native);
    style->setParent(bar);
    bar->setStyle(style);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QMenu* appearance = window.findChild<QMenu*>(QStringLiteral("MenuAppearanceMenu"));
    QVERIFY(appearance != nullptr);
    for (int cycle = 0; cycle < 3; ++cycle) {
        appearance->actions().at(1)->trigger();
        QCoreApplication::processEvents();
        appearance->actions().at(0)->trigger();
        QCoreApplication::processEvents();
        QVERIFY(bar->styleSheet().isEmpty());
        for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
            for (const QPalette::ColorRole role : {QPalette::Window, QPalette::WindowText,
                                                   QPalette::Button, QPalette::ButtonText, QPalette::Highlight,
                                                   QPalette::HighlightedText})
                QCOMPARE(bar->palette().color(group, role), native.color(group, role));
        }
        for (const QStyle::State state : {QStyle::State(QStyle::State_Enabled | QStyle::State_Active),
                                          QStyle::State(QStyle::State_Enabled),
                                          QStyle::State(QStyle::State_None)}) {
            QStyleOptionMenuItem option;
            option.initFrom(bar);
            option.state = state;
            option.palette.setCurrentColorGroup(state & QStyle::State_Enabled
                ? state & QStyle::State_Active ? QPalette::Active : QPalette::Inactive : QPalette::Disabled);
            option.rect = QRect(0, 0, 200, 32);
            option.text = QStringLiteral("Menu foreground");
            option.font = bar->font();
            QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
            image.fill(native.color(QPalette::Window));
            QPainter painter(&image);
            bar->style()->drawControl(QStyle::CE_MenuBarItem, &option, &painter, bar);
            painter.end();
            QVERIFY(firstPixelMatchingColor(image, image.rect(),
                native.color(option.palette.currentColorGroup(), QPalette::ButtonText), 12) != QPoint(-1, -1));
        }
    }
}

void TestMenusUi::gtk_menus_use_hover_colors_instead_of_the_list_accent_data()
{
    QTest::addColumn<bool>("dark");
    QTest::newRow("light-menu") << false;
    QTest::newRow("dark-menu") << true;
}

void TestMenusUi::gtk_menus_use_hover_colors_instead_of_the_list_accent()
{
    QFETCH(bool, dark);
    // Model GTK's CSS states and Cairo's pixel buffer without requiring a
    // GNOME session. This also checks translucent disabled text and cleanup.
    struct Widget {
        unsigned state = 0;
        Widget* child = nullptr;
        bool destroyed = false;
    };
    static bool darkTheme;
    static int widgetCount;
    static int surfaceCount;
    darkTheme = dark;
    widgetCount = surfaceCount = 0;
    GtkMenuPalette::Api api{};
    api.popoverNew = [](void*) -> void* { ++widgetCount; return new Widget; };
    api.modelButtonNew = []() -> void* { ++widgetCount; return new Widget; };
    api.add = [](void* menu, void* item) { static_cast<Widget*>(menu)->child = static_cast<Widget*>(item); };
    api.context = [](void* widget) { return widget; };
    api.setState = [](void* widget, unsigned state) { static_cast<Widget*>(widget)->state = state; };
    api.color = [](void*, unsigned state, GtkMenuPalette::Rgba* color) {
        const double component = state & (1 << 3) ? 0 : darkTheme ? 238.0 / 255 : 36.0 / 255;
        *color = {component, component, component, state & (1 << 3) ? 0.5 : 1};
    };
    api.background = [](void* context, void* painter, double, double, double, double) {
        auto* widget = static_cast<Widget*>(context);
        auto* image = static_cast<QImage*>(painter);
        if (widget->child) {
            image->fill(QColor(darkTheme ? "#202020" : "#f0f0f0"));
        } else if (widget->state & (1 << 1)) {
            // GTK themes can express hover colors with translucent CSS.
            QPainter paint(image);
            paint.fillRect(image->rect(), darkTheme ? QColor(255, 255, 255, 32) : QColor(0, 0, 0, 32));
        }
    };
    api.destroyWidget = [](void* widget) {
        auto* menu = static_cast<Widget*>(widget);
        delete menu->child;
        menu->child = nullptr;
        menu->destroyed = true;
        --widgetCount;
    };
    api.refSink = [](void* widget) { return widget; };
    api.unref = [](void* widget) {
        auto* menu = static_cast<Widget*>(widget);
        Q_ASSERT(menu->destroyed);
        delete menu;
        --widgetCount;
    };
    api.surfaceForData = [](unsigned char* data, int format, int width, int height, int stride) -> void* {
        Q_ASSERT(format == 0);
        ++surfaceCount;
        return new QImage(data, width, height, stride, QImage::Format_ARGB32_Premultiplied);
    };
    api.createPainter = [](void* surface) { return surface; };
    api.destroyPainter = [](void*) {};
    api.flushSurface = [](void*) {};
    api.destroySurface = [](void* surface) { delete static_cast<QImage*>(surface); --surfaceCount; };
    QPalette fallback;
    fallback.setColor(QPalette::Highlight, QColor("#e95420"));
    const QPalette palette = GtkMenuPalette::read(api, fallback);
    const QColor fill(dark ? "#202020" : "#f0f0f0");
    const QColor selected(dark ? "#3c3c3c" : "#d2d2d2");
    for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
        QCOMPARE(palette.color(group, QPalette::Window), fill);
        QCOMPARE(palette.color(group, QPalette::Highlight), selected);
        if (group != QPalette::Disabled) {
            QCOMPARE(palette.color(group, QPalette::ButtonText), QColor(dark ? "#eeeeee" : "#242424"));
            QCOMPARE(palette.color(group, QPalette::HighlightedText), palette.color(group, QPalette::ButtonText));
        }
    }
    QCOMPARE(palette.color(QPalette::Disabled, QPalette::ButtonText),
             QColor(dark ? "#101010" : "#787878"));
    QCOMPARE(palette.color(QPalette::Link), fallback.color(QPalette::Link));
    QCOMPARE(fallback.color(QPalette::Highlight), QColor("#e95420"));
    QCOMPARE(widgetCount, 0);
    QCOMPARE(surfaceCount, 0);
}

void TestMenusUi::system_menus_preserve_platform_roles_and_follow_palette_changes_data()
{
    QTest::addColumn<bool>("darkPlatform");
    QTest::newRow("dark-system-light-calculator") << true;
    QTest::newRow("light-system-dark-calculator") << false;
}

void TestMenusUi::system_menus_preserve_platform_roles_and_follow_palette_changes()
{
    QFETCH(bool, darkPlatform);
    MainWindowStateGuard guard;
    guard.settings->menuAppearance = Settings::MenuAppearanceSystem;
    const QPalette original = QApplication::palette();
    const QPalette originalMenu = QApplication::palette("QMenu");
    const QPalette originalBar = QApplication::palette("QMenuBar");
    const auto restorePalette = qScopeGuard([&]() {
        QApplication::setPalette(original);
        QApplication::setPalette(originalMenu, "QMenu");
        QApplication::setPalette(originalBar, "QMenuBar");
        MenuStyle::refresh();
    });
    const auto platformPalette = [&original](bool dark) {
        QPalette palette = original;
        for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive,
                                                QPalette::Disabled}) {
            const QColor fill(dark ? "#202428" : "#eeeeee");
            const QColor text(group == QPalette::Disabled ? "#808890" : (dark ? "#f0f0f0" : "#202020"));
            for (const QPalette::ColorRole role : {QPalette::Window, QPalette::Base, QPalette::Button})
                palette.setColor(group, role, fill);
            for (const QPalette::ColorRole role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText})
                palette.setColor(group, role, text);
            palette.setColor(group, QPalette::Highlight, QColor("#354faf"));
            palette.setColor(group, QPalette::HighlightedText, QColor("#ffffff"));
        }
        return palette;
    };
    QPalette menuPalette = platformPalette(darkPlatform);
    QPalette barPalette = menuPalette;
    barPalette.setColor(QPalette::Highlight, QColor("#246044"));
    QApplication::setPalette(menuPalette, "QMenu");
    QApplication::setPalette(barPalette, "QMenuBar");
    guard.settings->colorScheme = QStringLiteral("Custom");
    guard.settings->customColorSchemeJson = themeJsonString(QJsonObject{
        {QStringLiteral("background"), darkPlatform ? QStringLiteral("#e5eee8") : QStringLiteral("#232136")}
    });
    MainWindow window(false);
    window.menuBar()->setNativeMenuBar(false);
    auto* transparentStyle = new TransparentMenuBarTestStyle;
    transparentStyle->setParent(window.menuBar());
    window.menuBar()->setStyle(transparentStyle);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    MenuStyle::refresh();
    QScopedPointer<QMenu> editorMenu(window.findChild<Editor*>()->createStandardContextMenu());
    editorMenu->ensurePolished();
    MenuTestResultDisplay result;
    QScopedPointer<QMenu> resultMenu(result.createContextMenu(QPoint()));
    resultMenu->ensurePolished();
    const auto verifyPalette = [](QWidget* widget, const QPalette& expected) {
        widget->ensurePolished();
        QVERIFY(widget->styleSheet().isEmpty());
        for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive,
                                                QPalette::Disabled}) {
            for (const QPalette::ColorRole role : {QPalette::Window, QPalette::Text,
                                                   QPalette::WindowText, QPalette::ButtonText,
                                                   QPalette::Highlight, QPalette::HighlightedText}) {
                QCOMPARE(widget->palette().color(group, role), expected.color(group, role));
            }
        }
    };
    verifyPalette(window.menuBar(), barPalette);
    const QImage barImage = window.menuBar()->grab().toImage();
    const QPoint backgroundPixel(barImage.width() - 4, barImage.height() / 2);
    QCOMPARE(barImage.pixelColor(backgroundPixel), barPalette.color(QPalette::Button));
    for (QMenu* menu : window.findChildren<QMenu*>())
        verifyPalette(menu, menuPalette);
    verifyPalette(editorMenu.data(), menuPalette);
    verifyPalette(resultMenu.data(), menuPalette);
    for (QMenu* submenu : resultMenu->findChildren<QMenu*>())
        verifyPalette(submenu, menuPalette);

    for (bool enabled : {true, false}) {
        QStyleOptionMenuItem option;
        option.initFrom(editorMenu.data());
        option.menuItemType = QStyleOptionMenuItem::Normal;
        option.rect = QRect(0, 0, 160, 28);
        option.text = QStringLiteral("Menu item");
        option.state = enabled ? QStyle::State(QStyle::State_Enabled | QStyle::State_Selected)
                               : QStyle::State(QStyle::State_None);
        option.palette.setCurrentColorGroup(enabled ? QPalette::Active : QPalette::Disabled);
        option.font = editorMenu->font();
        const QColor fill = menuPalette.color(enabled ? QPalette::Highlight : QPalette::Window);
        const QColor text = enabled ? menuPalette.color(QPalette::HighlightedText)
                                   : menuPalette.color(QPalette::Disabled, QPalette::Text);
        QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(fill);
        QPainter painter(&image);
        painter.setFont(option.font);
        editorMenu->style()->drawControl(QStyle::CE_MenuItem, &option, &painter, editorMenu.data());
        painter.end();
        QVERIFY(firstPixelMatchingColor(image, image.rect(), text, 12) != QPoint(-1, -1));
    }

    menuPalette = platformPalette(!darkPlatform);
    barPalette = menuPalette;
    barPalette.setColor(QPalette::Highlight, QColor("#246044"));
    QApplication::setPalette(menuPalette, "QMenu");
    QApplication::setPalette(barPalette, "QMenuBar");
    QCoreApplication::processEvents();
    verifyPalette(window.menuBar(), barPalette);
    verifyPalette(editorMenu.data(), menuPalette);
    verifyPalette(resultMenu.data(), menuPalette);
}

void TestMenusUi::precision_menu_editor_uses_system_colors()
{
    MainWindowStateGuard guard;
    guard.settings->menuAppearance = Settings::MenuAppearanceSystem;
    guard.settings->statusBarVisible = true;
    guard.settings->resultPrecision = -1;
    MainWindow window(false);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    bool inspected = false;
    QTimer::singleShot(0, &window, [&]() {
        QMenu* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
        if (menu == nullptr)
            return;
        const auto closeMenu = qScopeGuard([menu]() { menu->close(); });
        QSpinBox* spin = menu->findChild<QSpinBox*>();
        QVERIFY(spin != nullptr);
        QVERIFY(!spin->isEnabled());
        QVERIFY(spin->styleSheet().isEmpty());
        QVERIFY(menu->styleSheet().isEmpty());
        QCOMPARE(spin->palette().color(QPalette::Disabled, QPalette::Base),
                 QApplication::palette(spin).color(QPalette::Disabled, QPalette::Base));
        QCOMPARE(spin->palette().color(QPalette::Disabled, QPalette::Text),
                 QApplication::palette(spin).color(QPalette::Disabled, QPalette::Text));
        inspected = true;
    });
    QVERIFY(QMetaObject::invokeMethod(&window, "showPrecisionContextMenu", Qt::DirectConnection,
                                      Q_ARG(QPoint, QPoint())));
    QVERIFY(inspected);
}

void TestMenusUi::dock_context_menus_follow_menu_theme_data()
{
    QTest::addColumn<bool>("darkSystem");
    QTest::addColumn<bool>("floating");
    QTest::newRow("dark-system-attached") << true << false;
    QTest::newRow("light-system-attached") << false << false;
    QTest::newRow("dark-system-floating") << true << true;
    QTest::newRow("light-system-floating") << false << true;
}

void TestMenusUi::dock_context_menus_follow_menu_theme()
{
    QFETCH(bool, darkSystem);
    QFETCH(bool, floating);
    MainWindowStateGuard guard;
    const QPalette originalMenuPalette = QApplication::palette("QMenu");
    const auto restorePalette = qScopeGuard([&]() {
        QApplication::setPalette(originalMenuPalette, "QMenu");
        MenuStyle::refresh();
    });
    QPalette native = originalMenuPalette;
    for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
        const QColor background(darkSystem ? "#202428" : "#eeeeee");
        const QColor foreground(group == QPalette::Disabled ? "#808890"
                                                            : (darkSystem ? "#f0f0f0" : "#202020"));
        for (const QPalette::ColorRole role : {QPalette::Window, QPalette::Base, QPalette::Button})
            native.setColor(group, role, background);
        for (const QPalette::ColorRole role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText})
            native.setColor(group, role, foreground);
        native.setColor(group, QPalette::Highlight, QColor("#354faf"));
        native.setColor(group, QPalette::HighlightedText, QColor("#ffffff"));
    }
    QApplication::setPalette(native, "QMenu");
    const QColor calculatorBackground(darkSystem ? "#e5eee8" : "#232136");
    guard.settings->colorScheme = QStringLiteral("Custom");
    guard.settings->customColorSchemeJson = themeJsonString(
        QJsonObject{{QStringLiteral("background"), calculatorBackground.name()}});
    guard.settings->historyDockVisible = true;
    guard.settings->variablesDockVisible = true;
    guard.settings->userFunctionsDockVisible = true;
    guard.settings->userUnitsDockVisible = true;
    Session historySession;
    historySession.addHistoryEntry(HistoryEntry(QStringLiteral("1+1"), Quantity(2)));
    MainWindow window(false);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QMenu* appearance = window.findChild<QMenu*>(QStringLiteral("MenuAppearanceMenu"));
    QVERIFY(appearance != nullptr);
    HistoryWidget* history = window.findChild<HistoryWidget*>();
    QVERIFY(history != nullptr);
    history->setSession(&historySession);

    const QVector<QColor> shades = generateOklchShades(
        calculatorBackground, 6, themePolarityForBackground(calculatorBackground));
    for (const char* dockName : {"HistoryDock", "VariablesDock", "UserFunctionsDock", "UserUnitsDock"}) {
        QDockWidget* dock = window.findChild<QDockWidget*>(QString::fromLatin1(dockName));
        QVERIFY(dock != nullptr);
        dock->setFloating(floating);
        dock->show();
        dock->raise();
        QCoreApplication::processEvents();
        QAbstractItemView* list = dock->findChild<QAbstractItemView*>();
        QVERIFY(list != nullptr);
        const QPalette listPalette = list->palette();
        const QString listStyle = list->styleSheet();
        for (const int mode : {0, 1, 0}) {
            appearance->actions().at(mode)->trigger();
            const QColor expectedBackground = mode == 0 ? native.color(QPalette::Window)
                                                        : shades.at(UiConfig::DockHeaderShade);
            const auto inspectPopup = [&](QWidget* target, const QPoint& pos) {
                bool inspected = false;
                QTimer inspector;
                inspector.setSingleShot(true);
                connect(&inspector, &QTimer::timeout, &window, [&]() {
                    auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
                    QVERIFY2(menu != nullptr, dockName);
                    const auto closeMenu = qScopeGuard([menu]() { menu->close(); });
                    QVERIFY(!menu->actions().isEmpty());
                    QCOMPARE(menu->palette().color(QPalette::Window).name(), expectedBackground.name());
                    if (mode == 0) {
                        QVERIFY(menu->styleSheet().isEmpty());
                        for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive,
                                                                QPalette::Disabled}) {
                            for (const QPalette::ColorRole role : {QPalette::WindowText, QPalette::Text,
                                                                   QPalette::Highlight, QPalette::HighlightedText})
                                QCOMPARE(menu->palette().color(group, role), native.color(group, role));
                        }
                    } else {
                        QVERIFY(!menu->styleSheet().isEmpty());
                    }
                    // Check the pixels too. An inherited stylesheet can replace
                    // a correct menu palette when the popup is painted.
                    const QImage image = menu->grab().toImage();
                    QColor paintedBackground = expectedBackground;
                    if (mode == 0) {
                        // Native styles may shade the palette's background.
                        // Compare with the same style without a themed parent.
                        QMenu reference;
                        reference.ensurePolished();
                        QStyleOption option;
                        option.initFrom(&reference);
                        option.rect = image.rect();
                        QImage nativeBackground(image.size(), QImage::Format_ARGB32_Premultiplied);
                        nativeBackground.fill(native.color(QPalette::Window));
                        QPainter painter(&nativeBackground);
                        reference.style()->drawPrimitive(QStyle::PE_PanelMenu, &option, &painter, &reference);
                        painter.end();
                        paintedBackground = nativeBackground.pixelColor(nativeBackground.rect().center());
                    }
                    const QRect interior = image.rect().adjusted(5, 5, -5, -5);
                    int matchingPixels = 0;
                    for (int y = interior.top(); y <= interior.bottom(); ++y) {
                        for (int x = interior.left(); x <= interior.right(); ++x) {
                            if (image.pixelColor(x, y).name() == paintedBackground.name())
                                ++matchingPixels;
                        }
                    }
                    QVERIFY2(matchingPixels > interior.width() * interior.height() / 2, dockName);
                    inspected = true;
                });
                inspector.start(0);
                QContextMenuEvent event(QContextMenuEvent::Mouse, pos, target->mapToGlobal(pos));
                QCoreApplication::sendEvent(target, &event);
                QCoreApplication::processEvents();
                inspector.stop();
                QVERIFY2(inspected, dockName);
            };
            const QPoint listPos = list->model()->rowCount() > 0
                ? list->visualRect(list->model()->index(0, 0)).center() : QPoint(8, 8);
            inspectPopup(list->viewport(), listPos);
            if (QLineEdit* search = dock->findChild<QLineEdit*>())
                inspectPopup(search, QPoint(8, 8));
            QCOMPARE(list->palette(), listPalette);
            QCOMPARE(list->styleSheet(), listStyle);
        }
    }
}

void TestMenusUi::precision_menu_editor_uses_themed_colors_data()
{
    QTest::addColumn<bool>("smallFont");
    QTest::newRow("default-font") << false;
    QTest::newRow("small-antialiased-font") << true;
}

void TestMenusUi::precision_menu_editor_uses_themed_colors()
{
    QFETCH(bool, smallFont);
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;
    settings->menuAppearance = Settings::MenuAppearanceSpeedCrunch;

    settings->colorScheme = QStringLiteral("Custom");
    settings->customColorSchemeJson = themeJsonString(QJsonObject{
        {QStringLiteral("background"), QStringLiteral("#300a24")}
    });
    settings->statusBarVisible = true;
    settings->resultPrecision = 8;
    settings->hasNumberFormatStyleSetting = true;

    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QString failure;
    QColor menuTextColor;
    QColor labelTextColor;
    QColor spinBackgroundColor;
    QColor spinTextColor;
    QColor spinArrowColor;
    QImage spinImage;
    qreal spinImageDevicePixelRatio = 1.0;
    QRect spinEditRect;
    QTimer::singleShot(0, &window, [&]() {
        QMenu* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
        if (menu == nullptr) {
            failure = QStringLiteral("Precision popup menu was not found.");
            return;
        }

        QLabel* precisionLabel = nullptr;
        for (QLabel* label : menu->findChildren<QLabel*>()) {
            if (label->text() == QStringLiteral("Decimal places:")) {
                precisionLabel = label;
                break;
            }
        }
        if (precisionLabel == nullptr) {
            failure = QStringLiteral("Precision editor label was not found.");
            menu->close();
            return;
        }
        QSpinBox* precisionSpin = menu->findChild<QSpinBox*>();
        if (precisionSpin == nullptr) {
            failure = QStringLiteral("Precision editor spin box was not found.");
            menu->close();
            return;
        }

        if (smallFont) {
            QFont font = precisionSpin->font();
            font.setPixelSize(8);
            font.setWeight(QFont::Thin);
            font.setStyleStrategy(QFont::PreferAntialias);
            precisionSpin->setFont(font);
        }

        menuTextColor = menu->palette().color(QPalette::WindowText);
        labelTextColor = precisionLabel->palette().color(QPalette::WindowText);
        spinBackgroundColor = precisionSpin->palette().color(QPalette::Base);
        spinTextColor = precisionSpin->palette().color(QPalette::Text);
        spinArrowColor = precisionSpin->palette().color(QPalette::ButtonText);

        QStyleOptionSpinBox option;
        option.initFrom(precisionSpin);
        option.subControls = QStyle::SC_All;
        spinEditRect = precisionSpin->style()->subControlRect(QStyle::CC_SpinBox,
                                                              &option,
                                                              QStyle::SC_SpinBoxEditField,
                                                              precisionSpin);
        const QPixmap spinPixmap = precisionSpin->grab();
        spinImageDevicePixelRatio = spinPixmap.devicePixelRatio();
        spinImage = spinPixmap.toImage();
        menu->close();
    });

    QVERIFY(QMetaObject::invokeMethod(&window,
                                      "showPrecisionContextMenu",
                                      Qt::DirectConnection,
                                      Q_ARG(QPoint, QPoint())));
    QVERIFY2(failure.isEmpty(), qPrintable(failure));
    QVERIFY(menuTextColor.isValid());
    QCOMPARE(labelTextColor.name(), menuTextColor.name());

    const QColor base(QStringLiteral("#300a24"));
    const QVector<QColor> shades = generateOklchShades(base, 6, ThemePolarity::Dark);
    const QVector<QColor> foregrounds = aaForegroundsForBackgrounds(shades);
    QCOMPARE(spinBackgroundColor.name(),
             shades.at(UiConfig::DockUnfocusedSelectedItemShade).name());
    QCOMPARE(spinTextColor.name(),
             foregrounds.at(UiConfig::DockUnfocusedSelectedItemShade).name());
    QCOMPARE(spinArrowColor.name(),
             foregrounds.at(UiConfig::DockUnfocusedSelectedItemShade).name());

    const auto imageRect = [spinImageDevicePixelRatio](const QRect& logicalRect) {
        return QRect(qRound(logicalRect.x() * spinImageDevicePixelRatio),
                     qRound(logicalRect.y() * spinImageDevicePixelRatio),
                     qRound(logicalRect.width() * spinImageDevicePixelRatio),
                     qRound(logicalRect.height() * spinImageDevicePixelRatio));
    };
    const QColor expectedBackground =
        shades.at(UiConfig::DockUnfocusedSelectedItemShade);
    const QColor expectedForeground =
        foregrounds.at(UiConfig::DockUnfocusedSelectedItemShade);
    QVERIFY(firstPixelMatchingColor(spinImage,
                                    imageRect(spinEditRect),
                                    expectedBackground,
                                    4) != QPoint(-1, -1));
    QVERIFY(firstPixelMatchingColorBlend(spinImage,
                                        imageRect(spinEditRect),
                                        expectedForeground,
                                        expectedBackground,
                                        8) != QPoint(-1, -1));
    const int arrowColumnWidth = qRound(18 * spinImageDevicePixelRatio);
    const QRect arrowColumn(spinImage.width() - arrowColumnWidth,
                            0,
                            arrowColumnWidth,
                            spinImage.height());
    const int arrowInset = qRound(3 * spinImageDevicePixelRatio);
    const QRect arrowInterior = arrowColumn.adjusted(arrowInset,
                                                     arrowInset,
                                                     -arrowInset,
                                                     -arrowInset);
    const QRect upArrowImageRect(arrowInterior.x(),
                                 arrowInterior.y(),
                                 arrowInterior.width(),
                                 arrowInterior.height() / 2);
    const QRect downArrowImageRect(arrowInterior.x(),
                                   arrowInterior.center().y() + 1,
                                   arrowInterior.width(),
                                   arrowInterior.height() - arrowInterior.height() / 2);
    QVERIFY(firstPixelDistinctFromColor(spinImage,
                                        upArrowImageRect,
                                        expectedBackground,
                                        48) != QPoint(-1, -1));
    QVERIFY(firstPixelDistinctFromColor(spinImage,
                                        downArrowImageRect,
                                        expectedBackground,
                                        48) != QPoint(-1, -1));
}

int main(int argc, char** argv)
{
    return UiTestFixture::run<TestMenusUi>(argc, argv);
}

#include "testmenusui.moc"
