// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mainwindowtesthelpers.h"
#include "core/numberformatter.h"
#include "core/mathdsl.h"
#include "gui/diagnosticsdialog.h"

#include <QOperatingSystemVersion>
#include <QStyleHints>

using namespace MainWindowTestHelpers;

class TestSettingsUi : public QObject {
    Q_OBJECT

private slots:
    void loading_current_settings_does_not_rewrite_storage();
    void startup_editor_preferences_data();
    void startup_editor_preferences();
    void system_color_mode_persistence_data();
    void system_color_mode_persistence();
    void system_color_mode_actions_update_windows_and_preserve_theme();
    void system_color_mode_updates_native_palettes();
    void system_color_mode_refreshes_controls_without_platform_notifications();
    void system_color_mode_replaces_stale_fusion_palette_and_releases_override();
    void trailing_zeroes_formatting_data();
    void trailing_zeroes_formatting();
    void trailing_zeroes_setting_survives_reload();
    void trailing_zeroes_action_updates_windows_and_preserves_history();
    void initTestCase();
    void init() { UiTestFixture::resetSettings(); }
    void about_box_shows_os_between_application_and_qt_versions();
    void update_checks_follow_build_option();
    void ui_test_fixture_resets_persisted_layout_and_first_run_preference();
    void editing_input_digit_grouping_updates_all_open_editors();
    void editing_automatic_input_adjustments_updates_all_open_editors();
    void raw_input_preserves_auto_ans_setting_data();
    void raw_input_preserves_auto_ans_setting();
    void raw_input_preserves_multiline_paste_evaluation_data();
    void raw_input_preserves_multiline_paste_evaluation();
    void editing_cursor_blinking_updates_all_open_editors();
    void editing_escape_clears_editor_is_shared_and_resets_to_off();
    void escape_clears_editor_survives_settings_reload();
    void accept_completion_with_enter_is_shared_and_resets_to_off();
    void accept_completion_with_enter_survives_settings_reload();
    void number_format_is_shared_and_refreshes_all_input_panes();
    void display_font_changes_update_all_sessions_and_windows_data();
    void display_font_changes_update_all_sessions_and_windows();
    void font_dialog_uses_platform_appropriate_options();
    void appearance_defaults_select_theme_and_modern_styles();
    void settings_reset_restores_persisted_defaults();
    void settings_reset_can_be_cancelled_data();
    void settings_reset_can_be_cancelled();
    void settings_reset_updates_all_windows_and_preserves_sessions();
    void settings_reset_restores_dock_column_orders();
    void manual_preserves_text_weights_and_emphasis();
    void calculation_settings_dialog_matches_notation_precision_layout();
    void user_definitions_menu_opens_working_dialog_data();
    void user_definitions_menu_opens_working_dialog();
    void user_definitions_load_after_session_restore_data();
    void user_definitions_load_after_session_restore();
    void user_definitions_hint_follows_theme_contrast_data();
    void user_definitions_hint_follows_theme_contrast();
    void quit_shortcut_triggers_menu_action_from_editor_and_window();
    void notation_shortcuts_change_only_active_window_data();
    void notation_shortcuts_change_only_active_window();
};

void TestSettingsUi::loading_current_settings_does_not_rewrite_storage()
{
    PrivateSessionStorage storage;
    Settings* settings = Settings::instance();
    settings->colorScheme = QStringLiteral("Duskfox");
    settings->save();
    const QString path = Settings::getConfigPath() + QStringLiteral("/SpeedCrunch.ini");
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadWrite));
    const QByteArray original = file.readAll();
    const QDateTime timestamp = QDateTime::fromSecsSinceEpoch(1000000000);
    QVERIFY(file.setFileTime(timestamp, QFileDevice::FileModificationTime));
    file.close();
    const QDateTime before = QFileInfo(path).lastModified();
    settings->colorScheme.clear();
    settings->load();
    QCOMPARE(settings->colorScheme, QStringLiteral("Duskfox"));
    QCOMPARE(QFileInfo(path).lastModified(), before);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), original);
}

void TestSettingsUi::startup_editor_preferences_data()
{
    QTest::addColumn<bool>("highlighting");
    QTest::addColumn<bool>("grouping");
    QTest::addColumn<bool>("adjustments");
    for (bool highlighting : {false, true}) {
        for (bool grouping : {false, true}) {
            for (bool adjustments : {false, true}) {
                QTest::addRow("highlight-%d-group-%d-adjust-%d", highlighting, grouping, adjustments)
                    << highlighting << grouping << adjustments;
            }
        }
    }
}

void TestSettingsUi::startup_editor_preferences()
{
    QFETCH(bool, highlighting);
    QFETCH(bool, grouping);
    QFETCH(bool, adjustments);
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    auto* settings = guard.settings;
    settings->syntaxHighlighting = highlighting;
    settings->inputDigitGrouping = grouping;
    settings->automaticInputAdjustments = adjustments;
    settings->numberFormatStyle = Settings::NumberFormatThreeDigitSpaceDot;
    settings->applyNumberFormatStyle();
    MainWindow window(false);
    Editor* editor = window.findChild<Editor*>();
    QVERIFY(editor);
    auto* groupingAction = window.findChild<QAction*>(QStringLiteral("InputDigitGroupingAction"));
    auto* adjustmentsAction = window.findChild<QAction*>(QStringLiteral("AutomaticInputAdjustmentsAction"));
    QVERIFY(groupingAction && adjustmentsAction);
    QCOMPARE(groupingAction->isChecked(), grouping);
    QCOMPARE(adjustmentsAction->isChecked(), adjustments);
    QCOMPARE(settings->syntaxHighlighting, highlighting);
    editor->setText(QStringLiteral("1000"));
    const auto hasGrouping = [editor]() {
        for (const auto& range : editor->document()->firstBlock().layout()->formats()) {
            if (range.format.hasProperty(SyntaxHighlighter::InputDigitSeparator))
                return true;
        }
        return false;
    };
    QCOMPARE(hasGrouping(), grouping);
    groupingAction->trigger();
    QCOMPARE(hasGrouping(), !grouping);
    QCOMPARE(editor->text(), QStringLiteral("1000"));
    adjustmentsAction->trigger();
    QCOMPARE(settings->automaticInputAdjustments, !adjustments);
    editor->clear();
    editor->setAutoCompletionEnabled(false);
    QTest::keyClicks(editor, "2^3");
    if (adjustments)
        QCOMPARE(editor->text(), QStringLiteral("2^3"));
    else
        QVERIFY(editor->text() != QStringLiteral("2^3"));
}

void TestSettingsUi::system_color_mode_persistence_data()
{
    QTest::addColumn<QVariant>("stored");
    QTest::addColumn<int>("expected");
    QTest::newRow("missing") << QVariant() << 0;
    QTest::newRow("follow-system") << QVariant(0) << 0;
    QTest::newRow("light") << QVariant(1) << 1;
    QTest::newRow("dark") << QVariant(2) << 2;
    QTest::newRow("negative") << QVariant(-1) << 0;
    QTest::newRow("unknown") << QVariant(3) << 0;
    QTest::newRow("invalid") << QVariant(QStringLiteral("invalid")) << 0;
}

void TestSettingsUi::system_color_mode_persistence()
{
    QFETCH(QVariant, stored);
    QFETCH(int, expected);
    Settings* settings = Settings::instance();
    QSettings persisted(Settings::getConfigPath() + QStringLiteral("/SpeedCrunch.ini"),
                        QSettings::IniFormat);
    const QString key = QStringLiteral("SpeedCrunch/Display/SystemColorMode");
    if (stored.isValid())
        persisted.setValue(key, stored);
    else
        persisted.remove(key);
    persisted.sync();
    settings->load();
    QCOMPARE(int(settings->systemColorMode), expected);
    settings->save();
    persisted.sync();
    QCOMPARE(persisted.value(key).toInt(), expected);
    settings->systemColorMode = Settings::SystemColorModeDark;
    QVERIFY(settings->resetToDefaults());
    QCOMPARE(settings->systemColorMode, Settings::SystemColorModeFollowSystem);
}

void TestSettingsUi::system_color_mode_actions_update_windows_and_preserve_theme()
{
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;
    const auto restoreMode = qScopeGuard([settings]() {
        settings->systemColorMode = Settings::SystemColorModeFollowSystem;
        MainWindow::initializeApplicationStyle();
    });
    settings->colorScheme = QStringLiteral("Tomorrow");
    MainWindow first(false);
    MainWindow second(false);
    auto* editor = first.findChild<Editor*>();
    auto* display = first.findChild<ResultDisplay*>();
    QVERIFY(editor);
    QVERIFY(display);
    editor->setText(QStringLiteral("12 + 34"));
    QVERIFY(QMetaObject::invokeMethod(&first, "evaluateEditorExpression", Qt::DirectConnection));
    editor->setText(QStringLiteral("56 + 78"));
    const QString history = display->toPlainText();
    const QColor editorBackground = editor->palette().color(QPalette::Base);
    const QColor displayBackground = display->palette().color(QPalette::Base);
    const QStringList labels {QStringLiteral("Follow System"), QStringLiteral("Light"),
                              QStringLiteral("Dark")};
    for (int mode : {1, 2, 0}) {
        auto* menu = first.findChild<QMenu*>(QStringLiteral("SystemColorModeMenu"));
        QVERIFY(menu);
#if QT_VERSION < QT_VERSION_CHECK(6, 8, 0)
        QVERIFY(!menu->menuAction()->isVisible());
        return;
#endif
        QAction* selected = nullptr;
        for (QAction* action : menu->actions()) {
            if (action->data().toInt() == mode)
                selected = action;
        }
        QVERIFY(selected);
        selected->trigger();
        QCOMPARE(int(settings->systemColorMode), mode);
        QCoreApplication::processEvents();
        MainWindow third(false);
        for (MainWindow* window : {&first, &second, &third}) {
            auto* choices = window->findChild<QMenu*>(QStringLiteral("SystemColorModeMenu"));
            QVERIFY(choices);
            for (QAction* action : choices->actions())
                QCOMPARE(action->isChecked(), action->data().toInt() == mode);
        }
        QCOMPARE(settings->colorScheme, QStringLiteral("Tomorrow"));
        QCOMPARE(settings->menuAppearance, Settings::MenuAppearanceSpeedCrunch);
        QCOMPARE(settings->dockAppearance, Settings::DockAppearanceSpeedCrunch);
        QCOMPARE(settings->statusBarAppearance, Settings::StatusBarAppearanceSpeedCrunch);
        QCOMPARE(editor->text(), QStringLiteral("56 + 78"));
        QCOMPARE(display->toPlainText(), history);
        QCOMPARE(editor->palette().color(QPalette::Base), editorBackground);
        QCOMPARE(display->palette().color(QPalette::Base), displayBackground);
        QVERIFY(appDiagnosticsReport(QString()).contains(
            QStringLiteral("System color mode: ") + labels.at(mode) + QLatin1Char('\n')));
        settings->save();
        settings->systemColorMode = Settings::SystemColorModeFollowSystem;
        settings->load();
        QCOMPARE(int(settings->systemColorMode), mode);
    }
}

void TestSettingsUi::system_color_mode_updates_native_palettes()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    const QString platform = QGuiApplication::platformName();
    if (platform != QLatin1String("cocoa") && platform != QLatin1String("windows")) {
        QTest::qSkip("Requires native Windows or macOS appearance support", __FILE__, __LINE__);
        return;
    }
    if (QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Unknown) {
        QTest::qSkip("System color scheme unavailable, for example with a contrast theme",
                     __FILE__, __LINE__);
        return;
    }
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;
    const auto restoreMode = qScopeGuard([settings]() {
        settings->systemColorMode = Settings::SystemColorModeFollowSystem;
        MainWindow::initializeApplicationStyle();
    });
    const Qt::ColorScheme systemScheme = QGuiApplication::styleHints()->colorScheme();
    settings->menuAppearance = Settings::MenuAppearanceSystem;
    settings->dockAppearance = Settings::DockAppearanceSystem;
    settings->statusBarAppearance = Settings::StatusBarAppearanceSystem;
    settings->statusBarVisible = true;
    settings->constantsDockVisible = true;
    // A saved override must take effect before the first window is created.
    settings->systemColorMode = Settings::SystemColorModeDark;
    settings->save();
    settings->load();
    MainWindow::initializeApplicationStyle();
    QTRY_COMPARE(QGuiApplication::styleHints()->colorScheme(), Qt::ColorScheme::Dark);
    MainWindow window(false);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto* menu = window.findChild<QMenu*>(QStringLiteral("SystemColorModeMenu"));
    auto* dock = window.findChild<QDockWidget*>(QStringLiteral("ConstantsDock"));
    auto* status = window.findChild<QStatusBar*>();
    QVERIFY(menu);
    QVERIFY(dock);
    QVERIFY(status);
    for (int mode : {1, 2, 0}) {
        const Qt::ColorScheme expected = mode == 1 ? Qt::ColorScheme::Light
            : mode == 2 ? Qt::ColorScheme::Dark : systemScheme;
        for (QAction* action : menu->actions()) {
            if (action->data().toInt() == mode)
                action->trigger();
        }
        QTRY_COMPARE(QGuiApplication::styleHints()->colorScheme(), expected);
        QTRY_COMPARE(dock->palette().color(QPalette::Window).rgba(),
                     MenuStyle::systemPalette(dock).color(QPalette::Window).rgba());
        QTRY_COMPARE(status->palette().color(QPalette::Window).rgba(),
                     MenuStyle::systemPalette(status).color(QPalette::Window).rgba());
        QCOMPARE(QApplication::palette().color(QPalette::Window).lightness() < 128,
                 expected == Qt::ColorScheme::Dark);
    }
    // Resetting settings must also remove a live override in every window.
    menu->actions().at(2)->trigger();
    QTimer::singleShot(0, []() {
        auto* dialog = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        QVERIFY(dialog);
        dialog->button(QMessageBox::Yes)->click();
    });
    QVERIFY(QMetaObject::invokeMethod(&window, "resetAllSettings", Qt::DirectConnection));
    QCOMPARE(settings->systemColorMode, Settings::SystemColorModeFollowSystem);
    QTRY_COMPARE(QGuiApplication::styleHints()->colorScheme(), systemScheme);
#else
    QTest::qSkip("Requires Qt 6.8 or newer", __FILE__, __LINE__);
#endif
}

void TestSettingsUi::system_color_mode_refreshes_controls_without_platform_notifications()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    class SuppressAppearanceNotifications : public QObject {
        bool eventFilter(QObject* object, QEvent* event) override
        {
            return qobject_cast<MainWindow*>(object)
                && (event->type() == QEvent::ApplicationPaletteChange
                    || event->type() == QEvent::ThemeChange);
        }
    } suppress;
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;
    const QPalette originalPalette = QApplication::palette();
    const auto restoreAppearance = qScopeGuard([settings, originalPalette]() {
        settings->systemColorMode = Settings::SystemColorModeFollowSystem;
        MainWindow::initializeApplicationStyle();
        QApplication::setPalette(originalPalette);
    });
    settings->dockAppearance = Settings::DockAppearanceSystem;
    settings->statusBarAppearance = Settings::StatusBarAppearanceSystem;
    settings->statusBarVisible = true;
    settings->constantsDockVisible = true;
    QPalette dark = originalPalette;
    dark.setColor(QPalette::Window, QColor("#202020"));
    dark.setColor(QPalette::WindowText, Qt::white);
    QApplication::setPalette(dark);
    MainWindow first(false);
    MainWindow second(false);
    first.show();
    second.show();
    QVERIFY(QTest::qWaitForWindowExposed(&first));
    QVERIFY(QTest::qWaitForWindowExposed(&second));
    QCoreApplication::processEvents();
    qApp->installEventFilter(&suppress);
    const auto removeFilter = qScopeGuard([&suppress]() { qApp->removeEventFilter(&suppress); });

    for (int mode : {1, 2, 0}) {
        const QColor background = mode == 1 ? QColor("#eeeeee") : QColor("#202020");
        QPalette source = originalPalette;
        source.setColor(QPalette::Window, background);
        source.setColor(QPalette::WindowText, mode == 1 ? Qt::black : Qt::white);
        QApplication::setPalette(source);
        QCoreApplication::processEvents();
        auto* menu = first.findChild<QMenu*>(QStringLiteral("SystemColorModeMenu"));
        QVERIFY(menu);
        for (QAction* action : menu->actions()) {
            if (action->data().toInt() == mode)
                action->trigger();
        }
        QCOMPARE(int(settings->systemColorMode), mode);
        for (MainWindow* window : {&first, &second}) {
            auto* dock = window->findChild<QDockWidget*>(QStringLiteral("ConstantsDock"));
            auto* status = window->findChild<QStatusBar*>();
            QVERIFY(dock);
            QVERIFY(status);
            QTRY_COMPARE(dock->palette().color(QPalette::Window).rgba(), background.rgba());
            QTRY_COMPARE(status->palette().color(QPalette::Window).rgba(), background.rgba());
        }
    }
#else
    QTest::qSkip("Requires Qt 6.8 or newer", __FILE__, __LINE__);
#endif
}

void TestSettingsUi::system_color_mode_replaces_stale_fusion_palette_and_releases_override()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    class ModeStyle : public QProxyStyle {
    public:
        ModeStyle() : QProxyStyle(QStringLiteral("Fusion")) {}
        QPalette standardPalette() const override
        {
            QPalette palette = QProxyStyle::standardPalette();
            const bool dark = Settings::instance()->systemColorMode != Settings::SystemColorModeLight;
            palette.setColor(QPalette::Window, dark ? QColor("#202020") : QColor("#eeeeee"));
            palette.setColor(QPalette::WindowText, dark ? Qt::white : Qt::black);
            return palette;
        }
    };
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;
    QStyle* originalStyle = QApplication::style();
    const QPalette originalPalette = QApplication::palette();
    originalStyle->setParent(nullptr);
    QApplication::setStyle(new ModeStyle);
    const auto restoreAppearance = qScopeGuard([settings, originalStyle, originalPalette]() {
        settings->systemColorMode = Settings::SystemColorModeFollowSystem;
        MenuStyle::refresh();
        QApplication::setStyle(originalStyle);
        QApplication::setPalette(originalPalette);
    });
    QPalette desktop = originalPalette;
    desktop.setColor(QPalette::Window, QColor("#202020"));
    desktop.setColor(QPalette::WindowText, Qt::white);
    QApplication::setPalette(desktop);
    settings->menuAppearance = Settings::MenuAppearanceSystem;
    settings->dockAppearance = Settings::DockAppearanceSystem;
    settings->statusBarAppearance = Settings::StatusBarAppearanceSystem;
    settings->statusBarVisible = true;
    settings->constantsDockVisible = true;
    MainWindow window(false);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto* menu = window.findChild<QMenu*>(QStringLiteral("SystemColorModeMenu"));
    auto* dock = window.findChild<QDockWidget*>(QStringLiteral("ConstantsDock"));
    auto* status = window.findChild<QStatusBar*>();
    QVERIFY(menu);
    QVERIFY(dock);
    QVERIFY(status);
    auto* editor = window.findChild<Editor*>();
    auto* display = window.findChild<ResultDisplay*>();
    QVERIFY(editor);
    QVERIFY(display);
    const QColor editorBackground = editor->palette().color(QPalette::Base);
    const QColor displayBackground = display->palette().color(QPalette::Base);
    for (int mode : {1, 2, 1, 0}) {
        // Emulate GTK's stale cached palette while Fusion can generate the new mode.
        for (QAction* action : menu->actions()) {
            if (action->data().toInt() == mode)
                action->trigger();
        }
        const QColor expected = mode == 1 ? QColor("#eeeeee") : QColor("#202020");
        QTRY_COMPARE(QApplication::palette().color(QPalette::Window), expected);
        QTRY_COMPARE(menu->palette().color(QPalette::Window), expected);
        QTRY_COMPARE(dock->palette().color(QPalette::Window), expected);
        QTRY_COMPARE(status->palette().color(QPalette::Window), expected);
        QCOMPARE(editor->palette().color(QPalette::Base), editorBackground);
        QCOMPARE(display->palette().color(QPalette::Base), displayBackground);
        QVERIFY(appDiagnosticsReport(QString()).contains(
            QStringLiteral("Application colors (background / text): ") + expected.name()
            + QStringLiteral(" / ") + QApplication::palette().color(QPalette::WindowText).name()));
    }
    QCOMPARE(QApplication::palette().resolveMask(), desktop.resolveMask());
    desktop.setColor(QPalette::Window, QColor("#eeeeee"));
    desktop.setColor(QPalette::WindowText, Qt::black);
    QApplication::setPalette(desktop);
    QTRY_COMPARE(dock->palette().color(QPalette::Window), QColor("#eeeeee"));
    QTRY_COMPARE(status->palette().color(QPalette::Window), QColor("#eeeeee"));
#else
    QTest::qSkip("Requires Qt 6.8 or newer", __FILE__, __LINE__);
#endif
}

void TestSettingsUi::trailing_zeroes_formatting_data()
{
    QTest::addColumn<QString>("expression");
    QTest::addColumn<char>("notation");
    QTest::addColumn<int>("precision");
    QTest::addColumn<QString>("padded");
    QTest::addColumn<QString>("trimmed");
    auto row = [](const char* name, const char* expression, char notation, int precision,
                  const char* padded, const char* trimmed) {
        QTest::newRow(name) << QString::fromUtf8(expression) << notation << precision
                           << QString::fromUtf8(padded) << QString::fromUtf8(trimmed);
    };
    row("fixed", "3.14", 'f', 3, "3.140", "3.14");
    row("integer", "1200", 'f', 3, "1200.000", "1200");
    row("negative", "-3.14", 'f', 3, "-3.140", "-3.14");
    row("zero", "0", 'f', 3, "0", "0");
    row("round-to-zero", "0.0001", 'f', 3, "0.000", "0");
    row("rounding-carry", "1.9999", 'f', 3, "2.000", "2");
    row("keep-fractional-zeroes", "1.2003", 'f', 3, "1.200", "1.2");
    row("keep-internal-zeroes", "1.203", 'f', 3, "1.203", "1.203");
    row("zero-precision", "12.3", 'f', 0, "12", "12");
    row("automatic-precision", "3.14", 'f', -1, "3.14", "3.14");
    row("general", "3.14", 'g', 3, "3.140", "3.14");
    row("scientific", "1200", 'e', 3, "1.200e3", "1.2e3");
    row("scientific-negative-exponent", "0.0012", 'e', 3, "1.200e-3", "1.2e-3");
    row("scientific-exponent-zeroes", "1e20", 'e', 3, "1.000e20", "1e20");
    row("engineering", "1200", 'n', 3, "1.200e3", "1.2e3");
    row("engineering-integer", "100000", 'n', 2, "100.e3", "100e3");
    row("binary", "1.5", 'b', 4, "0b1.1000", "0b1.1");
    row("octal", "1.5", 'o', 4, "0o1.4000", "0o1.4");
    row("hexadecimal", "1.625", 'h', 4, "0x1.A000", "0x1.A");
    row("complex", "1.5+2.25j", 'f', 3, "1.500+2.250j", "1.5+2.25j");
    row("unit", "1.5[m]", 'f', 3, "1.500[m]", "1.5[m]");
    row("list", "{1.5; 20}", 'f', 3, "{1.500; 20.000}", "{1.5; 20}");
    row("rational", "1/2", 'r', 3, "1 / 2", "1 / 2");
    row("sexagesimal", "1.5[s]", 's', 3, "0:00:01.500", "0:00:01.5");
}

void TestSettingsUi::trailing_zeroes_formatting()
{
    QFETCH(QString, expression);
    QFETCH(char, notation);
    QFETCH(int, precision);
    QFETCH(QString, padded);
    QFETCH(QString, trimmed);
    Settings* settings = Settings::instance();
    settings->imaginaryUnit = 'j';
    settings->numberFormatStyle = Settings::NumberFormatNoGroupingDot;
    settings->applyNumberFormatStyle();
    DMath::complexMode = true;
    Evaluator evaluator;
    evaluator.initializeBuiltInVariables();
    evaluator.setExpression(expression);
    const Quantity value = evaluator.eval();
    QVERIFY2(evaluator.error().isEmpty(), qPrintable(evaluator.error()));
    auto formatted = [&]() {
        return NumberFormatter::format(value, notation, precision, true, ComplexForm::Rectangular)
            .replace(QChar(0x2212), QLatin1Char('-'));
    };
    QCOMPARE(formatted(), padded);
    settings->removeTrailingZeroes = true;
    QCOMPARE(formatted(), trimmed);
    settings->numberFormatStyle = Settings::NumberFormatNoGroupingComma;
    settings->applyNumberFormatStyle();
    QCOMPARE(formatted(), QString(trimmed).replace(QLatin1Char('.'), QLatin1Char(',')));
    // The setting affects presentation, including per-line precision overrides.
    settings->numberFormatStyle = Settings::NumberFormatNoGroupingDot;
    settings->applyNumberFormatStyle();
    settings->resultFormat = notation;
    settings->resultPrecision = precision;
    QCOMPARE(NumberFormatter::format(value).replace(QChar(0x2212), QLatin1Char('-')), trimmed);
    settings->removeTrailingZeroes = false;
    QCOMPARE(formatted(), padded);
}

void TestSettingsUi::trailing_zeroes_setting_survives_reload()
{
    Settings* settings = Settings::instance();
    QVERIFY(!settings->removeTrailingZeroes);
    for (bool enabled : {true, false}) {
        settings->removeTrailingZeroes = enabled;
        settings->save();
        settings->removeTrailingZeroes = !enabled;
        settings->load();
        QCOMPARE(settings->removeTrailingZeroes, enabled);
    }
}

void TestSettingsUi::trailing_zeroes_action_updates_windows_and_preserves_history()
{
    Settings::instance()->resultFormat = 'f';
    Settings::instance()->resultPrecision = 3;
    QList<QString> previews;
    MainWindow first(false);
    MainWindow second(false);
    QVERIFY(QMetaObject::invokeMethod(&first, "splitActivePaneRight"));
    QAction* action = first.findChild<QAction*>(QStringLiteral("RemoveTrailingZeroesAction"));
    QAction* secondAction = second.findChild<QAction*>(QStringLiteral("RemoveTrailingZeroesAction"));
    QVERIFY(action);
    QVERIFY(secondAction);
    QVERIFY(!action->isChecked());
    ResultDisplay* display = first.findChild<ResultDisplay*>();
    QVERIFY(display);
    Editor* historyEditor = editorForDisplay(display);
    QVERIFY(historyEditor);
    historyEditor->setText(QStringLiteral("3.14"));
    QVERIFY(QMetaObject::invokeMethod(&first, "evaluateEditorExpression"));
    const QString history = display->toPlainText();
    QVERIFY(history.contains(QStringLiteral("3.140")));

    const QList<Editor*> editors = first.findChildren<Editor*>() + second.findChildren<Editor*>();
    for (Editor* editor : editors) {
        editor->setAutoCalcEnabled(true);
        editor->setText(QStringLiteral("1+0.5"));
        connect(editor, &Editor::autoCalcMessageAvailable, &first,
                [&previews](const QString& text) { previews.append(text); });
    }
    action->trigger();
    QVERIFY(Settings::instance()->removeTrailingZeroes);
    QVERIFY(secondAction->isChecked());
    QTRY_VERIFY(previews.size() >= editors.size());
    for (const QString& preview : previews) {
        QVERIFY2(preview.contains(QStringLiteral("1.5")), qPrintable(preview));
        QVERIFY2(!preview.contains(QStringLiteral("1.500")), qPrintable(preview));
    }
    QCOMPARE(display->toPlainText(), history);
    MainWindow third(false);
    QAction* thirdAction = third.findChild<QAction*>(QStringLiteral("RemoveTrailingZeroesAction"));
    QVERIFY(thirdAction);
    QVERIFY(thirdAction->isChecked());
    secondAction->trigger();
    QVERIFY(!action->isChecked());
    QVERIFY(!thirdAction->isChecked());
    action->trigger();
    QTimer::singleShot(0, &first, []() {
        auto* dialog = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        QVERIFY(dialog);
        dialog->button(QMessageBox::Yes)->click();
    });
    QVERIFY(QMetaObject::invokeMethod(&first, "resetAllSettings"));
    QVERIFY(!Settings::instance()->removeTrailingZeroes);
    QVERIFY(!action->isChecked());
    QVERIFY(!secondAction->isChecked());
    QVERIFY(!thirdAction->isChecked());
    QCOMPARE(display->toPlainText(), history);
}

void TestSettingsUi::initTestCase()
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

void TestSettingsUi::settings_reset_restores_dock_column_orders()
{
    MainWindow first(false);
    MainWindow second(false);
    QList<QHeaderView*> headers;
    for (MainWindow* window : {&first, &second}) {
        window->show();
        QVERIFY(QTest::qWaitForWindowExposed(window));
        for (const char* setter : {"setConstantsDockVisible", "setFunctionsDockVisible",
                                  "setVariablesDockVisible", "setUserFunctionsDockVisible",
                                  "setUserUnitsDockVisible"}) {
            QVERIFY(QMetaObject::invokeMethod(window, setter, Qt::DirectConnection,
                                             Q_ARG(bool, true), Q_ARG(bool, false)));
        }
        for (QDockWidget* dock : window->findChildren<QDockWidget*>()) {
            QTreeWidget* table = dock->findChild<QTreeWidget*>();
            if (!table)
                continue;
            QHeaderView* header = table->header();
            header->moveSection(header->count() - 1, 0);
            headers.append(header);
        }
    }
    QCOMPARE(headers.size(), 10);
    QTimer::singleShot(0, &first, []() {
        auto* confirmation = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        QVERIFY(confirmation);
        confirmation->button(QMessageBox::Yes)->click();
    });
    QVERIFY(QMetaObject::invokeMethod(&first, "resetAllSettings", Qt::DirectConnection));
    for (QHeaderView* header : headers) {
        for (int visual = 0; visual < header->count(); ++visual)
            QCOMPARE(header->logicalIndex(visual), visual);
    }
    Settings::instance()->load();
    QVERIFY(Settings::instance()->dockColumnOrdersJson.isEmpty());
    const QJsonArray windows = QJsonDocument::fromJson(Settings::instance()->sessionLayoutJson.toUtf8())
                                  .object().value(QStringLiteral("windows")).toArray();
    QCOMPARE(windows.size(), 2);
    for (const QJsonValue& window : windows) {
        const QJsonObject orders = window.toObject().value(QStringLiteral("dockColumnOrders")).toObject();
        QCOMPARE(orders.size(), 5);
        for (auto it = orders.begin(); it != orders.end(); ++it) {
            const QJsonArray order = it.value().toArray();
            for (int visual = 0; visual < order.size(); ++visual)
                QCOMPARE(order.at(visual).toInt(-1), visual);
        }
    }
}

void TestSettingsUi::about_box_shows_os_between_application_and_qt_versions()
{
    AboutBox dialog;
    const QTextEdit* textEdit = dialog.findChild<QTextEdit*>();
    QVERIFY(textEdit);

    QString applicationVersion = QStringLiteral("SpeedCrunch " SPEEDCRUNCH_VERSION);
#ifdef SPEEDCRUNCH_PORTABLE
    applicationVersion += QStringLiteral(" Portable");
#else
    QVERIFY(!textEdit->toPlainText().contains(QStringLiteral("Portable")));
#endif
    const QStringList lines = textEdit->toPlainText().split(QLatin1Char('\n'));
    const int versionLine = lines.indexOf(applicationVersion);
    QVERIFY(versionLine >= 0);
    QVERIFY(lines.size() > versionLine + 2);
    const QString osName = QSysInfo::prettyProductName()
        .remove(QLatin1Char('(')).remove(QLatin1Char(')'));
    QCOMPARE(lines.at(versionLine + 1), osName);
    QCOMPARE(lines.at(versionLine + 2), QStringLiteral("Qt " QT_VERSION_STR));
}

void TestSettingsUi::update_checks_follow_build_option()
{
    MainWindowStateGuard guard;
#ifndef SPEEDCRUNCH_ENABLE_UPDATE_CHECKS
    // A disabled build must work without the test-only launch-check bypass.
    qunsetenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK");
#endif

    MainWindow window;
    QCoreApplication::processEvents();
    QVERIFY(QMetaObject::invokeMethod(&window, "retranslateText", Qt::DirectConnection));

    int checkerCount = 0;
    for (QObject* child : window.children()) {
        if (child->inherits("VersionCheck"))
            ++checkerCount;
    }

    QAction* updateAction = nullptr;
    const QString updateText = MainWindow::tr("Check for &Updates");
    for (QAction* action : window.findChildren<QAction*>()) {
        if (action->text() == updateText) {
            QVERIFY(!updateAction);
            updateAction = action;
        }
    }

#ifdef SPEEDCRUNCH_ENABLE_UPDATE_CHECKS
    QCOMPARE(checkerCount, 1);
    QVERIFY(updateAction);
    QVERIFY(updateAction->isEnabled());
    QVERIFY(window.metaObject()->indexOfSlot("checkForUpdates()") >= 0);
#else
    QCOMPARE(checkerCount, 0);
    QVERIFY(!updateAction);
    QCOMPARE(window.metaObject()->indexOfSlot("checkForUpdates()"), -1);
#endif
}

void TestSettingsUi::editing_input_digit_grouping_updates_all_open_editors()
{
    Settings::instance()->numberFormatStyle = Settings::NumberFormatThreeDigitSpaceDot;
    Settings::instance()->applyNumberFormatStyle();
    MainWindow first;
    MainWindow second;
    QAction* firstAction = first.findChild<QAction*>(QStringLiteral("InputDigitGroupingAction"));
    QAction* secondAction = second.findChild<QAction*>(QStringLiteral("InputDigitGroupingAction"));
    QVERIFY(firstAction);
    QVERIFY(secondAction);
    QVERIFY(firstAction->isCheckable());
    QVERIFY(firstAction->isChecked());
    QVERIFY(QMetaObject::invokeMethod(&first, "splitActivePaneRight", Qt::DirectConnection));
    const QList<Editor*> editors = first.findChildren<Editor*>() + second.findChildren<Editor*>();
    QVERIFY(editors.size() >= 3);
    for (Editor* editor : editors)
        editor->setText(QStringLiteral("1000"));
    for (bool enabled : {false, true}) {
        firstAction->trigger();
        QCOMPARE(Settings::instance()->inputDigitGrouping, enabled);
        QCOMPARE(secondAction->isChecked(), enabled);
        for (Editor* editor : editors) {
            bool hasGap = false;
            for (const auto& range : editor->document()->firstBlock().layout()->formats())
                hasGap |= range.format.hasProperty(SyntaxHighlighter::InputDigitSeparator);
            QCOMPARE(hasGap, enabled);
            QCOMPARE(editor->text(), QStringLiteral("1000"));
        }
    }
}

void TestSettingsUi::editing_automatic_input_adjustments_updates_all_open_editors()
{
    Settings::instance()->numberFormatStyle = Settings::NumberFormatThreeDigitSpaceDot;
    Settings::instance()->applyNumberFormatStyle();
    MainWindow first;
    MainWindow second;
    QAction* firstAction = first.findChild<QAction*>(QStringLiteral("AutomaticInputAdjustmentsAction"));
    QAction* secondAction = second.findChild<QAction*>(QStringLiteral("AutomaticInputAdjustmentsAction"));
    QVERIFY(firstAction);
    QVERIFY(secondAction);
    QVERIFY(firstAction->isCheckable());
    QVERIFY(firstAction->isChecked());
    QVERIFY(QMetaObject::invokeMethod(&first, "splitActivePaneRight", Qt::DirectConnection));
    const QList<Editor*> editors = first.findChildren<Editor*>() + second.findChildren<Editor*>();
    QVERIFY(editors.size() >= 3);
    for (Editor* editor : editors) {
        editor->setAutoCompletionEnabled(false);
        editor->setText(QStringLiteral("1000"));
        editor->setCursorPosition(4);
    }
    firstAction->trigger();
    QVERIFY(!Settings::instance()->automaticInputAdjustments);
    QVERIFY(!secondAction->isChecked());
    for (Editor* editor : editors) {
        QCOMPARE(editor->text(), QStringLiteral("1000"));
        QCOMPARE(editor->cursorPosition(), 4);
        bool hasSeparator = false;
        for (const auto& range : editor->document()->firstBlock().layout()->formats())
            hasSeparator |= range.format.hasProperty(SyntaxHighlighter::InputDigitSeparator);
        QVERIFY(hasSeparator);
        QTest::keyClicks(editor, "+2^3@");
        QCOMPARE(editor->text(), QStringLiteral("1000+2^3@"));
    }
    QVERIFY(QMetaObject::invokeMethod(&first, "splitActivePaneDown", Qt::DirectConnection));
    MainWindow third;
    QVERIFY(!third.findChild<QAction*>(QStringLiteral("AutomaticInputAdjustmentsAction"))->isChecked());
    for (MainWindow* window : {&first, &second, &third}) {
        for (Editor* editor : window->findChildren<Editor*>()) {
            editor->clear();
            QTest::keyClicks(editor, "2^3");
            QCOMPARE(editor->text(), QStringLiteral("2^3"));
        }
    }
    secondAction->trigger();
    QVERIFY(Settings::instance()->automaticInputAdjustments);
    for (MainWindow* window : {&first, &second, &third}) {
        QVERIFY(window->findChild<QAction*>(QStringLiteral("AutomaticInputAdjustmentsAction"))->isChecked());
        for (Editor* editor : window->findChildren<Editor*>()) {
            QCOMPARE(editor->text(), QStringLiteral("2^3"));
            editor->clear();
            QTest::keyClicks(editor, "2^3");
            QCOMPARE(editor->text(), QString::fromUtf8("2³"));
        }
    }
    firstAction->trigger();
    QAction* resetAction = first.findChild<QAction*>(QStringLiteral("ResetAllSettingsAction"));
    QVERIFY(resetAction);
    QTimer::singleShot(0, &first, []() {
        auto* dialog = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        QVERIFY(dialog);
        dialog->button(QMessageBox::Yes)->click();
    });
    resetAction->trigger();
    QVERIFY(Settings::instance()->automaticInputAdjustments);
    for (MainWindow* window : {&first, &second, &third})
        QVERIFY(window->findChild<QAction*>(QStringLiteral("AutomaticInputAdjustmentsAction"))->isChecked());
}

void TestSettingsUi::raw_input_preserves_auto_ans_setting_data()
{
    QTest::addColumn<bool>("autoAns");
    QTest::newRow("auto-ans-enabled") << true;
    QTest::newRow("auto-ans-disabled") << false;
}

void TestSettingsUi::raw_input_preserves_auto_ans_setting()
{
    QFETCH(bool, autoAns);
    Settings::instance()->automaticInputAdjustments = false;
    Settings::instance()->autoAns = autoAns;
    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QCoreApplication::processEvents();
    Editor* editor = window.findChild<Editor*>();
    QVERIFY(editor);
    editor->setAutoCompletionEnabled(false);
    editor->setText(QStringLiteral("2+3"));
    QTest::keyClick(editor, Qt::Key_Return);
    QVERIFY(editor->evaluator()->getVariable(QStringLiteral("ans")).value() == Quantity(5));
    editor->clear();
    QTest::keyClicks(editor, "+2");
    const QString entered = QString(MathDsl::AddOp) + MathDsl::Dig2;
    QCOMPARE(editor->text(), autoAns ? QStringLiteral("ans") + entered : entered);
    editor->clear();
    QVERIFY(QMetaObject::invokeMethod(&window, "insertTextIntoEditor", Qt::DirectConnection,
                                     Q_ARG(QString, QStringLiteral("@**+"))));
    QCOMPARE(editor->text(), QStringLiteral("@**+"));
    editor->setText(QStringLiteral("2^3"));
    QCOMPARE(editor->text(), QStringLiteral("2^3"));
    editor->evaluate();
    QCOMPARE(editor->evaluator()->error(), QString());
    QCOMPARE(NumberFormatter::format(editor->evaluator()->getVariable(QStringLiteral("ans")).value()),
             QStringLiteral("8"));
}

void TestSettingsUi::raw_input_preserves_multiline_paste_evaluation_data()
{
    QTest::addColumn<bool>("rawInput");
    QTest::newRow("automatic-editing") << false;
    QTest::newRow("raw-input") << true;
}

void TestSettingsUi::raw_input_preserves_multiline_paste_evaluation()
{
    QFETCH(bool, rawInput);
    Settings::instance()->automaticInputAdjustments = !rawInput;
    Settings::instance()->leaveLastExpression = false;
    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QCoreApplication::processEvents();
    ResultDisplay* display = window.findChild<ResultDisplay*>();
    QVERIFY(display);
    Editor* editor = editorForDisplay(display);
    QVERIFY(editor);
    editor->setAutoCompletionEnabled(false);
    editor->clear();
    const int previousCount = display->session()->historySize();
    QSignalSpy started(editor, &Editor::bulkEvaluationStarted);
    QSignalSpy finished(editor, &Editor::bulkEvaluationFinished);
    QSignalSpy evaluated(editor, &Editor::returnPressed);
    const QString previousClipboard = QApplication::clipboard()->text();
    const auto restoreClipboard = qScopeGuard([&]() {
        QApplication::clipboard()->setText(previousClipboard);
    });
    QApplication::clipboard()->setText(QStringLiteral("2+3\n\n4*5\n2^3\n"));
    editor->paste();
    QCOMPARE(started.count(), 1);
    QCOMPARE(finished.count(), 1);
    QCOMPARE(evaluated.count(), 3);
    QCOMPARE(display->session()->historySize(), previousCount + 3);
    const QStringList expectedResults {QStringLiteral("5"), QStringLiteral("20"), QStringLiteral("8")};
    for (int i = 0; i < expectedResults.size(); ++i) {
        const HistoryEntry entry = display->session()->historyEntryAt(previousCount + i);
        QCOMPARE(NumberFormatter::format(entry.result()), expectedResults.at(i));
    }
    QCOMPARE(NumberFormatter::format(editor->evaluator()->getVariable(QStringLiteral("ans")).value()),
             expectedResults.last());
    QVERIFY(editor->text().isEmpty());
}

void TestSettingsUi::editing_cursor_blinking_updates_all_open_editors()
{
    MainWindow first;
    MainWindow second;
    QAction* firstAction = first.findChild<QAction*>(QStringLiteral("CursorBlinkingAction"));
    QAction* secondAction = second.findChild<QAction*>(QStringLiteral("CursorBlinkingAction"));
    QVERIFY(firstAction);
    QVERIFY(secondAction);
    QVERIFY(firstAction->isCheckable());
    QVERIFY(firstAction->isChecked());
    QVERIFY(QMetaObject::invokeMethod(&first, "splitActivePaneRight", Qt::DirectConnection));
    const QList<Editor*> editors = first.findChildren<Editor*>() + second.findChildren<Editor*>();
    QVERIFY(editors.size() >= 3);
    for (Editor* editor : editors)
        editor->setText(QStringLiteral("123"));
    firstAction->trigger();
    QVERIFY(!Settings::instance()->cursorBlinking);
    QVERIFY(!secondAction->isChecked());
    for (Editor* editor : editors) {
        QVERIFY(!editor->isCursorBlinkingEnabled());
        QCOMPARE(editor->text(), QStringLiteral("123"));
    }

    // Panes and windows opened after the change inherit the saved choice.
    QVERIFY(QMetaObject::invokeMethod(&first, "splitActivePaneDown", Qt::DirectConnection));
    MainWindow third;
    QVERIFY(!third.findChild<QAction*>(QStringLiteral("CursorBlinkingAction"))->isChecked());
    for (MainWindow* window : {&first, &second, &third}) {
        for (Editor* editor : window->findChildren<Editor*>())
            QVERIFY(!editor->isCursorBlinkingEnabled());
    }
    secondAction->trigger();
    QVERIFY(Settings::instance()->cursorBlinking);
    for (MainWindow* window : {&first, &second, &third}) {
        QVERIFY(window->findChild<QAction*>(QStringLiteral("CursorBlinkingAction"))->isChecked());
        for (Editor* editor : window->findChildren<Editor*>())
            QVERIFY(editor->isCursorBlinkingEnabled());
    }
    firstAction->trigger();
    QAction* resetAction = first.findChild<QAction*>(QStringLiteral("ResetAllSettingsAction"));
    QVERIFY(resetAction);
    QTimer::singleShot(0, &first, []() {
        auto* dialog = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        QVERIFY(dialog);
        dialog->button(QMessageBox::Yes)->click();
    });
    resetAction->trigger();
    for (MainWindow* window : {&first, &second, &third}) {
        QVERIFY(window->findChild<QAction*>(QStringLiteral("CursorBlinkingAction"))->isChecked());
        for (Editor* editor : window->findChildren<Editor*>())
            QVERIFY(editor->isCursorBlinkingEnabled());
    }
}

void TestSettingsUi::editing_escape_clears_editor_is_shared_and_resets_to_off()
{
    MainWindow first;
    MainWindow second;
    QAction* action = first.findChild<QAction*>(QStringLiteral("EscapeClearsEditorAction"));
    QAction* secondAction = second.findChild<QAction*>(QStringLiteral("EscapeClearsEditorAction"));
    QVERIFY(action);
    QVERIFY(secondAction);
    QVERIFY(action->isCheckable());
    QVERIFY(!action->isChecked());
    QVERIFY(!Settings::instance()->escapeClearsEditor);
    Editor* editor = first.findChild<Editor*>();
    QVERIFY(editor);
    editor->setText(QStringLiteral("123"));
    action->trigger();
    QVERIFY(Settings::instance()->escapeClearsEditor);
    QVERIFY(secondAction->isChecked());
    QCOMPARE(editor->text(), QStringLiteral("123"));
    MainWindow third;
    QAction* thirdAction = third.findChild<QAction*>(QStringLiteral("EscapeClearsEditorAction"));
    QVERIFY(thirdAction);
    QVERIFY(thirdAction->isChecked());
    secondAction->trigger();
    QVERIFY(!action->isChecked());
    QVERIFY(!thirdAction->isChecked());
    thirdAction->trigger();

    QAction* reset = first.findChild<QAction*>(QStringLiteral("ResetAllSettingsAction"));
    QVERIFY(reset);
    QTimer::singleShot(0, &first, []() {
        auto* dialog = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        QVERIFY(dialog);
        dialog->button(QMessageBox::Yes)->click();
    });
    reset->trigger();
    QVERIFY(!Settings::instance()->escapeClearsEditor);
    QVERIFY(!action->isChecked());
    QVERIFY(!secondAction->isChecked());
    QVERIFY(!thirdAction->isChecked());
    QCOMPARE(editor->text(), QStringLiteral("123"));
}

void TestSettingsUi::escape_clears_editor_survives_settings_reload()
{
    Settings* settings = Settings::instance();
    QVERIFY(!settings->escapeClearsEditor);
    for (bool enabled : {true, false}) {
        settings->escapeClearsEditor = enabled;
        settings->save();
        settings->escapeClearsEditor = !enabled;
        settings->load();
        QCOMPARE(settings->escapeClearsEditor, enabled);
    }
}

void TestSettingsUi::accept_completion_with_enter_is_shared_and_resets_to_off()
{
    MainWindow first;
    MainWindow second;
    QAction* action = first.findChild<QAction*>(QStringLiteral("AutoCompletionAcceptWithEnterAction"));
    QAction* secondAction = second.findChild<QAction*>(QStringLiteral("AutoCompletionAcceptWithEnterAction"));
    QVERIFY(action);
    QVERIFY(secondAction);
    QVERIFY(action->isCheckable());
    QVERIFY(!action->isChecked());
    QVERIFY(!Settings::instance()->autoCompletionAcceptWithEnter);
    Editor* editor = first.findChild<Editor*>();
    QVERIFY(editor);
    editor->setText(QStringLiteral("cos"));
    action->trigger();
    QVERIFY(Settings::instance()->autoCompletionAcceptWithEnter);
    QVERIFY(secondAction->isChecked());
    QCOMPARE(editor->text(), QStringLiteral("cos"));

    QVERIFY(QMetaObject::invokeMethod(&first, "splitActivePaneRight"));
    MainWindow third;
    QAction* thirdAction = third.findChild<QAction*>(QStringLiteral("AutoCompletionAcceptWithEnterAction"));
    QVERIFY(thirdAction);
    QVERIFY(thirdAction->isChecked());
    secondAction->trigger();
    QVERIFY(!Settings::instance()->autoCompletionAcceptWithEnter);
    QVERIFY(!action->isChecked());
    QVERIFY(!thirdAction->isChecked());
    thirdAction->trigger();

    QAction* reset = first.findChild<QAction*>(QStringLiteral("ResetAllSettingsAction"));
    QVERIFY(reset);
    QTimer::singleShot(0, &first, []() {
        auto* dialog = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        QVERIFY(dialog);
        dialog->button(QMessageBox::Yes)->click();
    });
    reset->trigger();
    QVERIFY(!Settings::instance()->autoCompletionAcceptWithEnter);
    QVERIFY(!action->isChecked());
    QVERIFY(!secondAction->isChecked());
    QVERIFY(!thirdAction->isChecked());
    QCOMPARE(editor->text(), QStringLiteral("cos"));
}

void TestSettingsUi::accept_completion_with_enter_survives_settings_reload()
{
    Settings* settings = Settings::instance();
    QVERIFY(!settings->autoCompletionAcceptWithEnter);
    for (bool enabled : {true, false}) {
        settings->autoCompletionAcceptWithEnter = enabled;
        settings->save();
        settings->autoCompletionAcceptWithEnter = !enabled;
        settings->load();
        QCOMPARE(settings->autoCompletionAcceptWithEnter, enabled);
    }
}

void TestSettingsUi::number_format_is_shared_and_refreshes_all_input_panes()
{
    Settings::instance()->inputDigitGrouping = true;
    MainWindow first;
    MainWindow second;
    QAction* numberFormatAction = nullptr;
    for (QAction* action : first.findChildren<QAction*>()) {
        if (action->text() == MainWindow::tr("Number Format..."))
            numberFormatAction = action;
    }
    QVERIFY(numberFormatAction);
    QVERIFY(QMetaObject::invokeMethod(&first, "splitActivePaneRight", Qt::DirectConnection));
    const QList<Editor*> editors = first.findChildren<Editor*>() + second.findChildren<Editor*>();
    QVERIFY(editors.size() >= 3);
    for (Editor* editor : editors)
        editor->setText(QStringLiteral("12345678.1234567"));
    for (auto style : {Settings::NumberFormatIndianCommaDot,
                       Settings::NumberFormatThreeDigitUnderscoreDotFraction,
                       Settings::NumberFormatNoGroupingDot}) {
        QTimer::singleShot(0, &first, [style]() {
            auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
            QVERIFY(dialog);
            auto* combo = dialog->findChild<QComboBox*>();
            QVERIFY(combo);
            // Select by example because the dialog's ordering differs from the enum.
            const QString example = style == Settings::NumberFormatIndianCommaDot
                ? QStringLiteral("12,34,567.12345")
                : style == Settings::NumberFormatThreeDigitUnderscoreDotFraction
                    ? QStringLiteral("1_234_567.123_45") : QStringLiteral("1234567.12345");
            const int index = combo->findText(example);
            QVERIFY(index >= 0);
            combo->setCurrentIndex(index);
            dialog->accept();
        });
        numberFormatAction->trigger();
        QCOMPARE(Settings::instance()->numberFormatStyle, style);
        const QList<int> expected = style == Settings::NumberFormatIndianCommaDot
            ? QList<int>{0, 2, 4}
            : style == Settings::NumberFormatThreeDigitUnderscoreDotFraction
                ? QList<int>{1, 4, 11, 14} : QList<int>{};
        for (Editor* editor : editors) {
            QList<int> positions;
            for (const auto& range : editor->document()->firstBlock().layout()->formats()) {
                if (range.format.hasProperty(SyntaxHighlighter::InputDigitSeparator)) {
                    for (int pos = range.start; pos < range.start + range.length; ++pos)
                        positions.append(pos);
                }
            }
            QCOMPARE(positions, expected);
            QCOMPARE(editor->text(), QStringLiteral("12345678.1234567"));
        }
    }
}

void TestSettingsUi::font_dialog_uses_platform_appropriate_options()
{
    const bool nativeDialogsDisabled = QApplication::testAttribute(Qt::AA_DontUseNativeDialogs);
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs, true);
    const auto restoreDialogs = qScopeGuard([nativeDialogsDisabled]() {
        QApplication::setAttribute(Qt::AA_DontUseNativeDialogs, nativeDialogsDisabled);
    });

    MainWindow window(false);
    const QString initialFont = Settings::instance()->displayFont;
    bool handledDialog = false;
    QFontDialog::FontDialogOptions actualOptions;
    QTimer::singleShot(0, &window, [&]() {
        auto* dialog = qobject_cast<QFontDialog*>(QApplication::activeModalWidget());
        QVERIFY(dialog);
        actualOptions = dialog->options();
        dialog->reject();
        handledDialog = true;
    });
    QVERIFY(QMetaObject::invokeMethod(&window, "showFontDialog", Qt::DirectConnection));
    QVERIFY(handledDialog);

    QFontDialog::FontDialogOptions expectedOptions;
#if defined(Q_OS_MACOS)
    if (QOperatingSystemVersion::current()
        >= QOperatingSystemVersion(QOperatingSystemVersion::MacOS, 27)) {
        expectedOptions |= QFontDialog::DontUseNativeDialog;
    }
#endif
    QCOMPARE(actualOptions, expectedOptions);
    QCOMPARE(Settings::instance()->displayFont, initialFont);
}

void TestSettingsUi::display_font_changes_update_all_sessions_and_windows_data()
{
    QTest::addColumn<QString>("operation");
    QTest::addColumn<int>("initialSize");
    QTest::addColumn<int>("expectedSize");
    QTest::newRow("accept-font-dialog") << QStringLiteral("accept") << 16 << 23;
    QTest::newRow("cancel-font-dialog") << QStringLiteral("cancel") << 16 << 16;
    QTest::newRow("increase-font-size") << QStringLiteral("increase") << 16 << 17;
    QTest::newRow("decrease-font-size") << QStringLiteral("decrease") << 16 << 15;
    QTest::newRow("maximum-font-size") << QStringLiteral("increase") << 96 << 96;
    QTest::newRow("minimum-font-size") << QStringLiteral("decrease") << 8 << 8;
}

void TestSettingsUi::display_font_changes_update_all_sessions_and_windows()
{
    QFETCH(QString, operation);
    QFETCH(int, initialSize);
    QFETCH(int, expectedSize);
    Settings* settings = Settings::instance();
    QFont initialFont;
    initialFont.setPointSize(initialSize);
    settings->displayFont = initialFont.toString();
    settings->autoCalc = false;
    const bool nativeDialogsDisabled = QApplication::testAttribute(Qt::AA_DontUseNativeDialogs);
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs, true);
    const auto restoreDialogs = qScopeGuard([nativeDialogsDisabled]() {
        QApplication::setAttribute(Qt::AA_DontUseNativeDialogs, nativeDialogsDisabled);
    });

    MainWindow first(false);
    MainWindow second(false);
    for (MainWindow* window : {&first, &second}) {
        QVERIFY(QMetaObject::invokeMethod(window, "splitActivePaneRight", Qt::DirectConnection));
        QVERIFY(QMetaObject::invokeMethod(window, "showNewSessionDialog", Qt::DirectConnection));
    }
    const QList<Editor*> editors = first.findChildren<Editor*>() + second.findChildren<Editor*>();
    const QList<ResultDisplay*> displays = first.findChildren<ResultDisplay*>() + second.findChildren<ResultDisplay*>();
    QCOMPARE(editors.size(), 4);
    QCOMPARE(displays.size(), 4);
    QMap<Editor*, QString> drafts;
    QMap<Editor*, int> cursors;
    QMap<ResultDisplay*, QString> results;
    for (Editor* editor : editors) {
        editor->setText(QStringLiteral("1234+56"));
        editor->setCursorPosition(3);
        drafts.insert(editor, editor->text());
        cursors.insert(editor, editor->cursorPosition());
    }
    for (ResultDisplay* display : displays) {
        Session* session = const_cast<Session*>(display->session());
        QVERIFY(session);
        session->addHistoryEntry(HistoryEntry(QStringLiteral("6*7"), Quantity(42)));
        display->refresh();
        results.insert(display, display->toPlainText());
    }

    QFont expectedFont = displays.constFirst()->font();
    expectedFont.setPointSize(expectedSize);
    if (operation == QLatin1String("accept") || operation == QLatin1String("cancel")) {
        bool handledDialog = false;
        QTimer::singleShot(0, &first, [&]() {
            auto* dialog = qobject_cast<QFontDialog*>(QApplication::activeModalWidget());
            QVERIFY(dialog);
            QFont chosenFont = expectedFont;
            chosenFont.setPointSize(23);
            chosenFont.setStyleName(QString());
            chosenFont.setBold(true);
            chosenFont.setItalic(true);
            dialog->setCurrentFont(chosenFont);
            if (operation == QLatin1String("accept")) {
                static_cast<QDialog*>(dialog)->accept();
                expectedFont = dialog->selectedFont();
            } else {
                dialog->reject();
            }
            handledDialog = true;
        });
        QVERIFY(QMetaObject::invokeMethod(&first, "showFontDialog", Qt::DirectConnection));
        QVERIFY(handledDialog);
        QCOMPARE(expectedFont.pointSize(), expectedSize);
    } else {
        // Exercise the editor signals used by Shift+Up and Shift+Down.
        const char* signal = operation == QLatin1String("increase")
            ? "shiftUpPressed" : "shiftDownPressed";
        QVERIFY(QMetaObject::invokeMethod(first.findChildren<Editor*>().constFirst(), signal,
                                         Qt::DirectConnection));
    }

    for (Editor* editor : editors) {
        QCOMPARE(editor->font(), expectedFont);
        QCOMPARE(editor->text(), drafts.value(editor));
        QCOMPARE(editor->cursorPosition(), cursors.value(editor));
    }
    for (ResultDisplay* display : displays) {
        QCOMPARE(display->font(), expectedFont);
        QCOMPARE(display->toPlainText(), results.value(display));
        QCOMPARE(display->session()->historySize(), 1);
    }
    QFont savedFont;
    QVERIFY(savedFont.fromString(settings->displayFont));
    QCOMPARE(savedFont, expectedFont);

    // Inactive tabs reuse their pane's widgets when selected.
    for (MainWindow* window : {&first, &second}) {
        int sessionCount = 0;
        for (ResultDisplay* display : window->findChildren<ResultDisplay*>()) {
            QTabBar* tabBar = tabBarForDisplay(display);
            QVERIFY(tabBar);
            sessionCount += tabBar->count();
            for (int index = 0; index < tabBar->count(); ++index)
                tabBar->setCurrentIndex(index);
        }
        QCOMPARE(sessionCount, 3);
        for (Editor* editor : window->findChildren<Editor*>())
            QCOMPARE(editor->font(), expectedFont);
        QVERIFY(QMetaObject::invokeMethod(window, "splitActivePaneDown", Qt::DirectConnection));
        for (ResultDisplay* display : window->findChildren<ResultDisplay*>())
            QCOMPARE(display->font(), expectedFont);
    }
    MainWindow third(false);
    // Qt can clear the style name while polishing a new widget.
    for (const QFont& font : {third.findChild<Editor*>()->font(),
                              third.findChild<ResultDisplay*>()->font()}) {
        QCOMPARE(font.family(), expectedFont.family());
        QCOMPARE(font.pointSizeF(), expectedFont.pointSizeF());
        QCOMPARE(font.weight(), expectedFont.weight());
        QCOMPARE(font.italic(), expectedFont.italic());
    }
    settings->save();
    settings->load();
    QVERIFY(savedFont.fromString(settings->displayFont));
    QCOMPARE(savedFont, expectedFont);
}

void TestSettingsUi::appearance_defaults_select_theme_and_modern_styles()
{
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;
    settings->menuAppearance = Settings::MenuAppearanceSystem;
    settings->dockAppearance = Settings::DockAppearanceSystem;
    settings->statusBarAppearance = Settings::StatusBarAppearanceSystem;
    settings->editorDisplayStyle = Settings::EditorDisplayClassic;
    settings->keypadAppearance = Settings::KeypadAppearanceClassic;
    QVERIFY(settings->resetToDefaults());
    QCOMPARE(settings->menuAppearance, Settings::MenuAppearanceSpeedCrunch);
    QCOMPARE(settings->dockAppearance, Settings::DockAppearanceSpeedCrunch);
    QCOMPARE(settings->statusBarAppearance, Settings::StatusBarAppearanceSpeedCrunch);
    QCOMPARE(settings->editorDisplayStyle, Settings::EditorDisplayModern);
    QCOMPARE(settings->keypadAppearance, Settings::KeypadAppearanceModern);

    MainWindow window(false);
    for (const QString& name : {QStringLiteral("MenuAppearanceMenu"),
                                QStringLiteral("DockAppearanceMenu"),
                                QStringLiteral("StatusBarAppearanceMenu"),
                                QStringLiteral("EditorDisplayStyleMenu"),
                                QStringLiteral("KeypadAppearanceMenu")}) {
        auto* menu = window.findChild<QMenu*>(name);
        QVERIFY(menu);
        QVERIFY2(menu->actions().at(1)->isChecked(), qPrintable(name));
        QVERIFY2(!menu->actions().at(0)->isChecked(), qPrintable(name));
    }
}

void TestSettingsUi::settings_reset_restores_persisted_defaults()
{
    Settings* settings = Settings::instance();
    const QString path = Settings::getConfigPath() + QStringLiteral("/SpeedCrunch.ini");
    QSettings persisted(path, QSettings::IniFormat);
    persisted.remove(QStringLiteral("SpeedCrunch"));
    persisted.sync();
    settings->load();
    settings->save();

    QMap<QString, QVariant> defaults;
    persisted.sync();
    for (const QString& key : persisted.allKeys())
        defaults.insert(key, persisted.value(key));

    settings->language = QStringLiteral("pt_PT");
    settings->autoAns = true;
    settings->autoCalc = false;
    settings->autoCompletion = false;
    settings->inputDigitGrouping = false;
    settings->numberFormatStyle = Settings::NumberFormatIndianCommaDot;
    settings->applyNumberFormatStyle();
    settings->resultFormat = 'h';
    settings->resultPrecision = 2;
    settings->imaginaryUnit = 'j';
    settings->resultRoundingMode = Settings::ResultRoundingHalfEven;
    settings->unitNegativeExponentStyle = Settings::UnitNegativeExponentFraction;
    settings->colorScheme = QStringLiteral("Custom");
    settings->customColorSchemeJson = QStringLiteral("{}");
    settings->menuAppearance = Settings::MenuAppearanceSpeedCrunch;
    settings->keypadMode = Settings::KeypadModeCustom;
    settings->keypadZoomPercent = 200;
    settings->customKeypad.buttons.clear();
    settings->windowGeometry = QByteArray("old geometry");
    settings->windowState = QByteArray("old dock layout");
    settings->sessionLayoutJson = QStringLiteral("{\"windows\":[]}");
    settings->startupUserDefinitions = QStringLiteral("saved_variable=7");
    settings->save();
    persisted.setValue(QStringLiteral("SpeedCrunch/General/HistoryNavigationWithUpDown"), false);
    persisted.setValue(QStringLiteral("SpeedCrunch/Format/Precision"), 3);
    persisted.setValue(QStringLiteral("UpdateCheck/TestMetadata"), 42);
    persisted.sync();

    QVERIFY(settings->resetToDefaults());
    settings->load();
    QVERIFY(settings->hasNumberFormatStyleSetting);
    QCOMPARE(settings->startupUserDefinitions, QStringLiteral("saved_variable=7"));
    QCOMPARE(runtimeResultRoundingMode(), char(Settings::ResultRoundingHalfAwayFromZero));
    QCOMPARE(runtimeUnitNegativeExponentStyle(), char(Settings::UnitNegativeExponentSuperscript));
    settings->save();
    persisted.sync();
    defaults[QStringLiteral("SpeedCrunch/Layout/SessionLayoutJson")] = QStringLiteral("{\"windows\":[]}");
    defaults[QStringLiteral("UpdateCheck/TestMetadata")] = 42;
    QMap<QString, QVariant> actual;
    for (const QString& key : persisted.allKeys())
        actual.insert(key, persisted.value(key));
    QCOMPARE(actual, defaults);
}

void TestSettingsUi::settings_reset_can_be_cancelled_data()
{
    QTest::addColumn<bool>("useEscape");
    QTest::newRow("no-button") << false;
    QTest::newRow("escape") << true;
}

void TestSettingsUi::settings_reset_can_be_cancelled()
{
    QFETCH(bool, useEscape);
    Settings* settings = Settings::instance();
    settings->autoAns = true;
    settings->numberFormatStyle = Settings::NumberFormatIndianCommaDot;
    settings->applyNumberFormatStyle();
    MainWindow window;
    settings->save();
    QFile persisted(Settings::getConfigPath() + QStringLiteral("/SpeedCrunch.ini"));
    QVERIFY(persisted.open(QIODevice::ReadOnly));
    const QByteArray before = persisted.readAll();
    persisted.close();
    QAction* action = window.findChild<QAction*>(QStringLiteral("ResetAllSettingsAction"));
    QVERIFY(action);
    QTimer::singleShot(0, &window, [useEscape]() {
        auto* dialog = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        QVERIFY(dialog);
        QCOMPARE(dialog->defaultButton(), dialog->button(QMessageBox::No));
        if (useEscape)
            QTest::keyClick(dialog, Qt::Key_Escape);
        else
            dialog->button(QMessageBox::No)->click();
    });
    action->trigger();
    QVERIFY(settings->autoAns);
    QCOMPARE(settings->numberFormatStyle, Settings::NumberFormatIndianCommaDot);
    QVERIFY(persisted.open(QIODevice::ReadOnly));
    QCOMPARE(persisted.readAll(), before);
}

void TestSettingsUi::settings_reset_updates_all_windows_and_preserves_sessions()
{
    Settings* settings = Settings::instance();
    settings->autoAns = true;
    settings->autoCalc = false;
    settings->autoCompletion = false;
    settings->inputDigitGrouping = false;
    settings->syntaxHighlighting = false;
    settings->hoverHighlightResults = false;
    settings->numberFormatStyle = Settings::NumberFormatIndianCommaDot;
    settings->applyNumberFormatStyle();
    settings->resultFormat = 'h';
    settings->resultPrecision = 2;
    settings->imaginaryUnit = 'j';
    settings->angleUnit = 'd';
    settings->resultRoundingMode = Settings::ResultRoundingHalfEven;
    settings->unitNegativeExponentStyle = Settings::UnitNegativeExponentFraction;
    settings->keypadMode = Settings::KeypadModeScientificNarrow;
    settings->keypadZoomPercent = 200;
    settings->constantsDockVisible = true;
    settings->constantsDockSearchText = QStringLiteral("old filter");
    settings->menuAppearance = Settings::MenuAppearanceSpeedCrunch;
    settings->colorScheme = QStringLiteral("Terminal");
    QFont customFont;
    customFont.setPointSize(24);
    settings->displayFont = customFont.toString();
    MainWindow first(false);
    MainWindow second(false);
    auto* floatingConstants = first.findChild<QDockWidget*>(QStringLiteral("ConstantsDock"));
    QVERIFY(floatingConstants);
    floatingConstants->setFloating(true);
    QVERIFY(QMetaObject::invokeMethod(&first, "splitActivePaneRight", Qt::DirectConnection));
    const QList<Editor*> editors = first.findChildren<Editor*>() + second.findChildren<Editor*>();
    const QList<ResultDisplay*> displays = first.findChildren<ResultDisplay*>() + second.findChildren<ResultDisplay*>();
    QVERIFY(editors.size() >= 3);
    settings->startupUserDefinitions = QStringLiteral("saved_variable=7");
    QFile originalDefinitions(Settings::getDataPath() + QStringLiteral("/definitions.json"));
    const bool hadDefinitions = originalDefinitions.exists();
    QByteArray originalContents;
    if (hadDefinitions) {
        QVERIFY(originalDefinitions.open(QIODevice::ReadOnly));
        originalContents = originalDefinitions.readAll();
        originalDefinitions.close();
    }
    const auto restoreDefinitions = qScopeGuard([&]() {
        if (hadDefinitions)
            writeFile(originalDefinitions.fileName(), originalContents);
        else
            QFile::remove(originalDefinitions.fileName());
    });
    UserDefinitions::saveFrom(settings);
    QFile definitions(Settings::getDataPath() + QStringLiteral("/definitions.json"));
    QVERIFY(definitions.open(QIODevice::ReadOnly));
    const QByteArray savedDefinitions = definitions.readAll();
    definitions.close();
    QMap<const Session*, QJsonObject> histories;
    for (ResultDisplay* display : displays) {
        Session* session = const_cast<Session*>(display->session());
        QVERIFY(session);
        if (histories.contains(session))
            continue;
        session->addHistoryEntry(HistoryEntry(QStringLiteral("6*7"), Quantity(42)));
        session->addVariable(Variable(QStringLiteral("kept_variable"), Quantity(7)));
        QJsonObject entry;
        session->historyEntryAtRef(0).serialize(entry);
        histories.insert(session, entry);
    }
    QMap<ResultDisplay*, QString> displayTexts;
    for (ResultDisplay* display : displays) {
        display->refresh();
        displayTexts.insert(display, display->toPlainText());
    }
    for (Editor* editor : editors)
        editor->setText(QStringLiteral("1234567+8"));
    QMap<Editor*, QString> editorTexts;
    for (Editor* editor : editors)
        editorTexts.insert(editor, editor->text());

    QAction* action = first.findChild<QAction*>(QStringLiteral("ResetAllSettingsAction"));
    QVERIFY(action);
    QTimer::singleShot(0, &first, []() {
        auto* dialog = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        QVERIFY(dialog);
        dialog->button(QMessageBox::Yes)->click();
    });
    action->trigger();
    QCoreApplication::processEvents();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCOMPARE(topLevelMainWindows().size(), 2);
    QCOMPARE(settings->resultFormat, 'f');
    QCOMPARE(settings->resultPrecision, -1);
    QCOMPARE(settings->imaginaryUnit, 'i');
    QCOMPARE(settings->angleUnit, 'r');
    QCOMPARE(settings->numberFormatStyle, Settings::NumberFormatNoGroupingDot);
    QVERIFY(!settings->autoAns);
    QVERIFY(settings->autoCalc);
    QVERIFY(settings->autoCompletion);
    QVERIFY(settings->syntaxHighlighting);
    QVERIFY(settings->hoverHighlightResults);
    QCOMPARE(settings->menuAppearance, Settings::MenuAppearanceSpeedCrunch);
    QCOMPARE(settings->colorScheme, QStringLiteral("Duskfox"));
    QCOMPARE(CMath::imaginaryUnitSymbol(), QChar('i'));
    Keypad defaultKeypad(Keypad::LayoutModeBasicWide);
    for (MainWindow* window : {&first, &second}) {
        QVERIFY(window->findChild<QAction*>(QStringLiteral("InputDigitGroupingAction"))->isChecked());
        for (QActionGroup* group : window->findChildren<QActionGroup*>()) {
            if (!group->isExclusive())
                continue;
            int checkedCount = 0;
            for (QAction* groupedAction : group->actions())
                checkedCount += groupedAction->isChecked() ? 1 : 0;
            QVERIFY(checkedCount <= 1);
        }
        auto* constants = window->findChild<ConstantsWidget*>();
        QVERIFY(constants);
        QVERIFY(constants->searchText().isEmpty());
        QVERIFY(window->findChild<QDockWidget*>(QStringLiteral("ConstantsDock"))->isHidden());
        QVERIFY(!window->findChild<QDockWidget*>(QStringLiteral("ConstantsDock"))->isFloating());
        auto* keypad = window->findChild<Keypad*>();
        QVERIFY(keypad);
        QCOMPARE(keypad->sizeHint(), defaultKeypad.sizeHint());
    }
    for (Editor* editor : editors) {
        QCOMPARE(editor->text(), editorTexts.value(editor));
        QCOMPARE(editor->font().pointSize(), QFont().pointSize());
    }
    for (ResultDisplay* display : displays) {
        QVERIFY(histories.contains(display->session()));
        QVERIFY(display->session()->hasVariable(QStringLiteral("kept_variable")));
        QCOMPARE(display->session()->historySize(), 1);
        QCOMPARE(display->toPlainText(), displayTexts.value(display));
        QJsonObject entry;
        display->session()->historyEntryAtRef(0).serialize(entry);
        QCOMPARE(entry, histories.value(display->session()));
    }
    QCOMPARE(settings->startupUserDefinitions, QStringLiteral("saved_variable=7"));
    QVERIFY(definitions.open(QIODevice::ReadOnly));
    QCOMPARE(definitions.readAll(), savedDefinitions);
    settings->load();
    QCOMPARE(settings->keypadMode, Settings::KeypadModeBasicWide);
    QCOMPARE(settings->keypadZoomPercent, 100);
    QCOMPARE(settings->numberFormatStyle, Settings::NumberFormatNoGroupingDot);
    QVERIFY(settings->hasNumberFormatStyleSetting);
    QVERIFY(!settings->sessionLayoutJson.isEmpty());
    QVERIFY(QApplication::activeModalWidget() == nullptr);
    QCOMPARE(first.findChildren<Editor*>().size() + second.findChildren<Editor*>().size(), editors.size());
}

void TestSettingsUi::ui_test_fixture_resets_persisted_layout_and_first_run_preference()
{
    Settings* settings = Settings::instance();
    settings->sessionLayoutJson = QStringLiteral("{\"windows\":[{\"id\":\"stale-window\"}]}");
    settings->windowState = QByteArray("stale dock state");
    settings->keypadMode = Settings::KeypadModeScientificNarrow;
    settings->hasNumberFormatStyleSetting = false;
    settings->save();

    const QString root = QString::fromUtf8(qgetenv("SPEEDCRUNCH_UI_TEST_STORAGE"));
    QVERIFY(!root.isEmpty());
    for (const QString& path : {Settings::getConfigPath(), Settings::getDataPath(),
                                Settings::getCachePath()})
        QVERIFY(path.startsWith(root + QLatin1Char('/')));

    UiTestFixture::resetSettings();
    QVERIFY(settings->sessionLayoutJson.isEmpty());
    QVERIFY(settings->windowState.isEmpty());
    QVERIFY(settings->hasNumberFormatStyleSetting);
    QVERIFY(!settings->windowPositionSave);
    QSettings persisted(Settings::getConfigPath() + QStringLiteral("/SpeedCrunch.ini"),
                        QSettings::IniFormat);
    QVERIFY(!persisted.contains(QStringLiteral("SpeedCrunch/General/SessionLayoutJson")));

    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QCoreApplication::processEvents();
    QCOMPARE(topLevelMainWindows().size(), 1);
    QVERIFY(QApplication::activeModalWidget() == nullptr);
}

void TestSettingsUi::manual_preserves_text_weights_and_emphasis()
{
    const QFont originalFont = QApplication::font();
    const auto restoreFont = qScopeGuard([originalFont]() {
        QApplication::setFont(originalFont);
    });
    QFont boldApplicationFont = originalFont;
    boldApplicationFont.setWeight(QFont::Bold);
    QApplication::setFont(boldApplicationFont);

    ManualWindow manual;
    manual.setHtml(QStringLiteral(
        "<h1>Heading</h1><p>Body text <strong>Bold emphasis</strong> "
        "<em>Italic emphasis</em> <a href=\"qthelp://manual/doc/next.html\">Link</a></p>"));

    const auto textFont = [&manual](const QString& text) {
        return manual.document()->find(text).charFormat().font()
            .resolve(manual.document()->defaultFont());
    };
    QCOMPARE(textFont(QStringLiteral("Body text")).weight(), QFont::Normal);
    QCOMPARE(textFont(QStringLiteral("Link")).weight(), QFont::Normal);
    QCOMPARE(textFont(QStringLiteral("Heading")).weight(), QFont::Bold);
    QCOMPARE(textFont(QStringLiteral("Bold emphasis")).weight(), QFont::Bold);
    QVERIFY(textFont(QStringLiteral("Italic emphasis")).italic());
    QVERIFY(!textFont(QStringLiteral("Body text")).italic());
#ifdef Q_OS_MACOS
    QVERIFY(textFont(QStringLiteral("Body text")).styleStrategy() & QFont::NoSubpixelAntialias);
    QVERIFY(textFont(QStringLiteral("Bold emphasis")).styleStrategy() & QFont::NoSubpixelAntialias);
#endif
}

void TestSettingsUi::calculation_settings_dialog_matches_notation_precision_layout()
{
    EvaluationContext context;
    context.main.fmt = 'f';
    context.main.prec = 3;
    context.angle = 'd';
    context.extras.append(ResultLineContext{'e', 5, ComplexForm::Default});

    ResultSlotsDialog dialog(QStringLiteral("Calculation Settings"), context);
    QCOMPARE(dialog.windowTitle(), QStringLiteral("Calculation Settings"));

    const QList<QLabel*> labels = dialog.findChildren<QLabel*>();
    for (QLabel* label : labels)
        QVERIFY(label->text() != QStringLiteral("Angle Mode"));

    const EvaluationContext updated = dialog.evaluationContext(context);
    QCOMPARE(updated.angle, 'd');
    QCOMPARE(updated.main.fmt, 'f');
    QCOMPARE(updated.main.prec, 3);
    QCOMPARE(updated.extras.size(), 1);
    QCOMPARE(updated.extras.at(0).fmt, 'e');
    QCOMPARE(updated.extras.at(0).prec, 5);
}

void TestSettingsUi::user_definitions_menu_opens_working_dialog_data()
{
    QTest::addColumn<QString>("operation");
    QTest::newRow("cancel") << QStringLiteral("Cancel");
    QTest::newRow("validate") << QStringLiteral("Validate");
    QTest::newRow("apply") << QStringLiteral("Apply");
    QTest::newRow("ok") << QStringLiteral("OK");
}

void TestSettingsUi::user_definitions_menu_opens_working_dialog()
{
    QFETCH(QString, operation);
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;
    settings->sessionLayoutJson.clear();
    settings->windowState.clear();
    settings->windowGeometry.clear();
    settings->hasNumberFormatStyleSetting = true;

    const QString oldDefinitions = settings->startupUserDefinitions;
    const QString definitionsPath = QDir(Settings::getDataPath()).filePath(QStringLiteral("definitions.json"));
    QFile definitionsFile(definitionsPath);
    const bool hadDefinitionsFile = definitionsFile.exists();
    QByteArray oldFileContents;
    if (hadDefinitionsFile) {
        QVERIFY(definitionsFile.open(QIODevice::ReadOnly));
        oldFileContents = definitionsFile.readAll();
        definitionsFile.close();
    }
    const auto restoreDefinitions = qScopeGuard([&]() {
        settings->startupUserDefinitions = oldDefinitions;
        if (hadDefinitionsFile)
            writeFile(definitionsPath, oldFileContents);
        else
            QFile::remove(definitionsPath);
    });
    Q_UNUSED(restoreDefinitions);

    const QString original = QStringLiteral("dialogexisting=7");
    const QString edited = QStringLiteral("dialogvalue=42\ndialogfunction(x)=x+1\n[dialogunit]=[metre]\n1+1");
    settings->startupUserDefinitions = original;
    UserDefinitions::saveFrom(settings);
    QVERIFY2(QFileInfo::exists(definitionsPath), qPrintable(definitionsPath));

    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    ResultDisplay* display = window.findChild<ResultDisplay*>();
    QVERIFY(display != nullptr);
    const Evaluator* evaluator = display->session()->evaluator();
    QVERIFY(evaluator->hasVariable(QStringLiteral("dialogexisting")));

    QMenu* settingsMenu = menuWithTitle(window.menuBar(), QStringLiteral("Se&ttings"));
    QVERIFY(settingsMenu != nullptr);
    QAction* action = directMenuActionWithText(settingsMenu, QStringLiteral("User &Definitions..."));
    QVERIFY(action != nullptr);

    bool visitedDialog = false;
    bool visitedResults = false;
    QTimer::singleShot(0, &window, [&]() {
        QDialog* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        QVERIFY(dialog != nullptr);
        const auto closeDialog = qScopeGuard([dialog]() {
            if (dialog->isVisible())
                dialog->reject();
        });
        Q_UNUSED(closeDialog);
        QCOMPARE(dialog->windowTitle(), QStringLiteral("User Definitions"));
        QPlainTextEdit* editor = dialog->findChild<QPlainTextEdit*>(QStringLiteral("UserDefinitionsEditor"));
        QPlainTextEdit* lineNumbers = dialog->findChild<QPlainTextEdit*>(QStringLiteral("UserDefinitionsLineNumbers"));
        QDialogButtonBox* buttons = dialog->findChild<QDialogButtonBox*>();
        QVERIFY(editor != nullptr);
        QVERIFY(lineNumbers != nullptr);
        QVERIFY(buttons != nullptr);
        QCOMPARE(editor->toPlainText(), original);
        editor->setPlainText(edited);
        QCOMPARE(lineNumbers->toPlainText(), QStringLiteral("1\n2\n3\n4"));
        visitedDialog = true;

        if (operation == QLatin1String("Cancel")) {
            buttons->button(QDialogButtonBox::Cancel)->click();
            return;
        }
        if (operation == QLatin1String("OK")) {
            buttons->button(QDialogButtonBox::Ok)->click();
            return;
        }

        QPushButton* operationButton = nullptr;
        for (QAbstractButton* button : buttons->buttons()) {
            if (button->text() == operation)
                operationButton = qobject_cast<QPushButton*>(button);
        }
        QVERIFY(operationButton != nullptr);
        QTimer::singleShot(0, dialog, [&]() {
            QMessageBox* results = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            QVERIFY(results != nullptr);
            const auto closeResults = qScopeGuard([results]() { results->accept(); });
            Q_UNUSED(closeResults);
            QVERIFY(results->text().contains(QStringLiteral("Imported variables: 1")));
            QVERIFY(results->text().contains(QStringLiteral("Imported functions: 1")));
            QVERIFY(results->text().contains(QStringLiteral("Imported units: 1")));
            QVERIFY(results->text().contains(QStringLiteral("Line numbers with errors: 4")));
            visitedResults = true;
        });
        operationButton->click();
        QVERIFY(dialog->isVisible());
        const bool applied = operation == QLatin1String("Apply");
        QCOMPARE(evaluator->hasVariable(QStringLiteral("dialogvalue")), applied);
        QCOMPARE(settings->startupUserDefinitions, applied ? edited : original);
        buttons->button(applied ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click();
    });
    action->trigger();
    QVERIFY(visitedDialog);
    QCOMPARE(visitedResults, operation == QLatin1String("Validate") || operation == QLatin1String("Apply"));

    const bool saved = operation == QLatin1String("OK") || operation == QLatin1String("Apply");
    QCOMPARE(evaluator->hasVariable(QStringLiteral("dialogvalue")), saved);
    QCOMPARE(evaluator->hasUserFunction(QStringLiteral("dialogfunction")), saved);
    QCOMPARE(evaluator->hasUserUnit(QStringLiteral("dialogunit")), saved);
    QCOMPARE(evaluator->hasVariable(QStringLiteral("dialogexisting")), !saved);
    QCOMPARE(settings->startupUserDefinitions, saved ? edited : original);
    UserDefinitions::loadInto(settings);
    QCOMPARE(settings->startupUserDefinitions, saved ? edited : original);
}

void TestSettingsUi::user_definitions_load_after_session_restore_data()
{
    QTest::addColumn<QString>("layoutKind");
    QTest::newRow("single-session") << QStringLiteral("single");
    QTest::newRow("tabs") << QStringLiteral("tabs");
    QTest::newRow("split-panes") << QStringLiteral("split");
    QTest::newRow("multiple-windows") << QStringLiteral("windows");
}

void TestSettingsUi::user_definitions_load_after_session_restore()
{
    QFETCH(QString, layoutKind);
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    Settings* settings = guard.settings;
    const QString oldDefinitions = settings->startupUserDefinitions;
    const auto restoreDefinitions = qScopeGuard([&]() {
        settings->startupUserDefinitions = oldDefinitions;
    });
    settings->startupUserDefinitions.clear();
    settings->sessionLayoutJson.clear();
    settings->openPrivateTabOnStartup = false;
    settings->variablesDockVisible = true;
    settings->userFunctionsDockVisible = true;
    settings->userUnitsDockVisible = true;

    const QStringList names = layoutKind == QLatin1String("single")
        ? QStringList { QStringLiteral("Untitled-1") }
        : QStringList { QStringLiteral("Saved-A"), QStringLiteral("Saved-B") };
    QVERIFY(QDir().mkpath(QFileInfo(storage.path(names.first())).absolutePath()));
    for (const QString& name : names) {
        Session saved;
        QVERIFY(saved.deSerialize(sessionJson(name), false));
        Evaluator* evaluator = saved.evaluator();
        for (const QString& expression : {QStringLiteral("startupvalue=7"),
                                         QStringLiteral("startupfunction(x)=x+1"),
                                         QStringLiteral("[startupunit]=2[metre]"),
                                         QStringLiteral("localonly=19")}) {
            evaluator->setExpression(expression);
            evaluator->setAllowGlobalUserDefinitionsOverride(true);
            evaluator->eval();
            QVERIFY2(evaluator->error().isEmpty(), qPrintable(evaluator->error()));
        }
        evaluator->setAllowGlobalUserDefinitionsOverride(false);
        QJsonObject json;
        saved.serialize(json);
        const QString path = layoutKind == QLatin1String("single")
            ? QDir(Settings::getDataPath()).filePath(QStringLiteral("history.json"))
            : storage.path(name);
        writeFile(path, QJsonDocument(json).toJson());
    }

    if (layoutKind != QLatin1String("single")) {
        QJsonObject first = startupSessionWindow(names.first(), QStringLiteral("first"));
        const QJsonObject second = startupSessionWindow(names.last(), QStringLiteral("second"));
        QJsonArray windows;
        if (layoutKind == QLatin1String("windows")) {
            windows = QJsonArray { first, second };
        } else {
            QJsonObject firstRoot = first.value(QStringLiteral("root")).toObject();
            const QJsonObject secondRoot = second.value(QStringLiteral("root")).toObject();
            if (layoutKind == QLatin1String("tabs")) {
                QJsonArray tabs = firstRoot.value(QStringLiteral("tabs")).toArray();
                tabs.append(secondRoot.value(QStringLiteral("tabs")).toArray().first());
                firstRoot.insert(QStringLiteral("tabs"), tabs);
            } else {
                firstRoot.insert(QStringLiteral("type"), QStringLiteral("pane"));
                QJsonObject secondPane = secondRoot;
                secondPane.insert(QStringLiteral("type"), QStringLiteral("pane"));
                firstRoot = QJsonObject {
                    { QStringLiteral("type"), QStringLiteral("split") },
                    { QStringLiteral("orientation"), QStringLiteral("horizontal") },
                    { QStringLiteral("active"), names.first() },
                    { QStringLiteral("children"), QJsonArray { firstRoot, secondPane } }
                };
            }
            first.insert(QStringLiteral("root"), firstRoot);
            windows.append(first);
        }
        settings->sessionLayoutJson = QString::fromUtf8(QJsonDocument(QJsonObject {
            { QStringLiteral("scheme"), 1 },
            { QStringLiteral("kind"), QStringLiteral("session-layout") },
            { QStringLiteral("activeWindow"), QStringLiteral("first") },
            { QStringLiteral("windows"), windows }
        }).toJson());
    }

    settings->startupUserDefinitions = QStringLiteral(
        "startupvalue=42\nstartupfunction(x)=2*x\n[startupunit]=3[metre]");
    UserDefinitions::saveFrom(settings);
    MainWindow restored;
    const auto cleanupWindows = qScopeGuard([&]() {
        for (MainWindow* window : topLevelMainWindows()) {
            if (window != &restored)
                delete window;
        }
    });
    restored.show();
    QTRY_COMPARE(topLevelMainWindows().size(), layoutKind == QLatin1String("windows") ? 2 : 1);
    QStringList checkedNames;
    for (MainWindow* window : topLevelMainWindows()) {
        QTRY_COMPARE(window->findChild<ResultDisplay*>()->session()->historySize(), 1);
        const int paneCount = layoutKind == QLatin1String("split") ? 2 : 1;
        QCOMPARE(window->findChildren<ResultDisplay*>().size(), paneCount);
        for (ResultDisplay* display : window->findChildren<ResultDisplay*>()) {
            QTabBar* tabs = tabBarForDisplay(display);
            QVERIFY(tabs);
            for (int index = 0; index < tabs->count(); ++index) {
                tabs->setCurrentIndex(index);
                Session* session = const_cast<Session*>(display->session());
                checkedNames.append(session->name());
                QCOMPARE(session->historySize(), 1);
                Evaluator* evaluator = session->evaluator();
                const QStringList expressions {
                    QStringLiteral("startupvalue"), QStringLiteral("startupfunction(startupvalue)"),
                    QStringLiteral("[startupunit]/[metre]"), QStringLiteral("localonly"),
                    QStringLiteral("ans")
                };
                const QList<int> expected { 42, 84, 3, 19, 42 };
                for (int i = 0; i < expressions.size(); ++i) {
                    evaluator->setExpression(expressions.at(i));
                    const Quantity result = evaluator->eval();
                    QVERIFY2(evaluator->error().isEmpty(), qPrintable(evaluator->error()));
                    QCOMPARE(result, Quantity(expected.at(i)));
                }
                evaluator->setExpression(QStringLiteral("startupvalue=1"));
                evaluator->eval();
                QVERIFY(!evaluator->error().isEmpty());
            }
        }
        VariableListWidget* variables = window->findChild<VariableListWidget*>();
        UserFunctionListWidget* functions = window->findChild<UserFunctionListWidget*>();
        UserUnitListWidget* units = window->findChild<UserUnitListWidget*>();
        QVERIFY(variables);
        QVERIFY(functions);
        QVERIFY(units);
        const QList<QPair<QWidget*, QString>> panels {
            { variables, QStringLiteral("startupvalue") },
            { functions, QStringLiteral("startupfunction") },
            { units, QStringLiteral("startupunit") }
        };
        for (const auto& panel : panels) {
            QWidget* parent = panel.first->parentWidget();
            while (parent && !qobject_cast<QDockWidget*>(parent))
                parent = parent->parentWidget();
            auto* dock = qobject_cast<QDockWidget*>(parent);
            QVERIFY(dock);
            dock->show();
            dock->raise();
            QTRY_VERIFY(panel.first->isVisible());
            QTreeWidget* list = panel.first->findChild<QTreeWidget*>();
            QVERIFY(list);
            QTRY_VERIFY(!list->findItems(panel.second, Qt::MatchContains).isEmpty());
        }
    }
    checkedNames.sort();
    QStringList expectedNames = names;
    expectedNames.sort();
    QCOMPARE(checkedNames, expectedNames);
}

void TestSettingsUi::user_definitions_hint_follows_theme_contrast_data()
{
    QTest::addColumn<QColor>("background");
    QTest::addColumn<QColor>("changedBackground");
    QTest::newRow("dark-to-light") << QColor(QStringLiteral("#222134")) << QColor(QStringLiteral("#f7f4e8"));
    QTest::newRow("light-to-dark") << QColor(QStringLiteral("#f7f4e8")) << QColor(QStringLiteral("#222134"));
}

void TestSettingsUi::user_definitions_hint_follows_theme_contrast()
{
    QFETCH(QColor, background);
    QFETCH(QColor, changedBackground);
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;
    settings->sessionLayoutJson.clear();
    settings->windowState.clear();
    settings->windowGeometry.clear();
    settings->hasNumberFormatStyleSetting = true;
    settings->colorScheme = QStringLiteral("Custom");
    settings->customColorSchemeJson = themeJsonString(QJsonObject{
        {QStringLiteral("background"), background.name()}
    });

    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    bool visitedDialog = false;
    QTimer::singleShot(0, &window, [&]() {
        QDialog* dialog = window.findChild<QDialog*>(QStringLiteral("UserDefinitionsDialog"));
        QVERIFY(dialog != nullptr);
        const auto closeDialog = qScopeGuard([dialog]() { dialog->reject(); });
        Q_UNUSED(closeDialog);
        QPlainTextEdit* editor = dialog->findChild<QPlainTextEdit*>(QStringLiteral("UserDefinitionsEditor"));
        QVERIFY(editor != nullptr);
        editor->clear();
        QVERIFY(!editor->placeholderText().isEmpty());

        const auto verifyHint = [editor](const QColor& expectedBackground) {
            const auto luminance = [](const QColor& color) {
                const auto linear = [](double channel) {
                    return channel <= 0.04045 ? channel / 12.92
                        : std::pow((channel + 0.055) / 1.055, 2.4);
                };
                return 0.2126 * linear(color.redF())
                    + 0.7152 * linear(color.greenF())
                    + 0.0722 * linear(color.blueF());
            };
            QCOMPARE(editor->palette().color(QPalette::Active, QPalette::Base), expectedBackground);
            for (const QPalette::ColorGroup group : {QPalette::Active,
                                                    QPalette::Inactive,
                                                    QPalette::Disabled}) {
                const QColor base = editor->palette().color(group, QPalette::Base);
                const QColor hint = editor->palette().color(group, QPalette::PlaceholderText);
                const double baseLuminance = luminance(base);
                const double hintLuminance = luminance(hint);
                const double contrast = (qMax(baseLuminance, hintLuminance) + 0.05)
                    / (qMin(baseLuminance, hintLuminance) + 0.05);
                QVERIFY2(contrast >= 7.0, qPrintable(QStringLiteral("Hint contrast is %1:1").arg(contrast)));
                QCOMPARE(hint.alpha(), 255);
                QCOMPARE(editor->viewport()->palette().color(group, QPalette::PlaceholderText), hint);
            }
        };
        verifyHint(background);
        settings->customColorSchemeJson = themeJsonString(QJsonObject{
            {QStringLiteral("background"), changedBackground.name()}
        });
        QVERIFY(QMetaObject::invokeMethod(&window, "colorSchemeChanged", Qt::DirectConnection));
        verifyHint(changedBackground);
        visitedDialog = true;
    });
    QVERIFY(QMetaObject::invokeMethod(&window, "showUserDefinitionsImportDialog", Qt::DirectConnection));
    QVERIFY(visitedDialog);
}

void TestSettingsUi::notation_shortcuts_change_only_active_window_data()
{
    QTest::addColumn<bool>("statusBarVisible");
    QTest::newRow("status-bar-shown") << true;
    QTest::newRow("status-bar-hidden") << false;
}

void TestSettingsUi::notation_shortcuts_change_only_active_window()
{
    QFETCH(bool, statusBarVisible);
    MainWindowStateGuard guard;
    guard.settings->sessionLayoutJson.clear();
    guard.settings->windowState.clear();
    guard.settings->windowGeometry.clear();
    guard.settings->statusBarVisible = statusBarVisible;
    guard.settings->resultFormat = 'r';

    MainWindow first(false);
    MainWindow second(false);
    for (MainWindow* window : {&first, &second}) {
        window->show();
        QVERIFY(QTest::qWaitForWindowExposed(window));
        QCOMPARE(window->statusBar()->isVisible(), statusBarVisible);
    }
    QSignalSpy firstChanges(&first, &MainWindow::resultFormatChanged);
    QSignalSpy secondChanges(&second, &MainWindow::resultFormatChanged);

    const QList<QPair<Qt::Key, char>> shortcuts {
        {Qt::Key_F2, 'g'}, {Qt::Key_F3, 'f'}, {Qt::Key_F4, 'n'}, {Qt::Key_F5, 'e'},
        {Qt::Key_F7, 'o'}, {Qt::Key_F8, 'h'}, {Qt::Key_F9, 's'}, {Qt::Key_F10, 'b'}
    };
    for (MainWindow* window : {&first, &second}) {
        window->raise();
        window->activateWindow();
        QTRY_COMPARE(QApplication::activeWindow(), static_cast<QWidget*>(window));
        Editor* editor = window->findChild<Editor*>();
        QVERIFY(editor);
        editor->setFocus();
        QTRY_VERIFY(editor->hasFocus());
        QSignalSpy& activeChanges = window == &first ? firstChanges : secondChanges;
        QSignalSpy& inactiveChanges = window == &first ? secondChanges : firstChanges;
        const qsizetype inactiveCount = inactiveChanges.count();
        for (const auto& shortcut : shortcuts) {
            const qsizetype activeCount = activeChanges.count();
            QTest::keyClick(editor, shortcut.first);
            QCOMPARE(guard.settings->resultFormat, shortcut.second);
            QCOMPARE(activeChanges.count(), activeCount + 1);
            QCOMPARE(inactiveChanges.count(), inactiveCount);
        }
    }
}

void TestSettingsUi::quit_shortcut_triggers_menu_action_from_editor_and_window()
{
    MainWindowStateGuard guard;
    guard.settings->sessionLayoutJson.clear();
    guard.settings->windowState.clear();
    guard.settings->windowGeometry.clear();
    guard.settings->hasNumberFormatStyleSetting = true;

    MainWindow window;
    window.show();
    window.activateWindow();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QMenu* sessionMenu = menuWithTitle(window.menuBar(), QStringLiteral("&Session"));
    QVERIFY(sessionMenu != nullptr);
    QAction* quitAction = directMenuActionWithText(sessionMenu, QStringLiteral("&Quit"));
    QVERIFY(quitAction != nullptr);
    QCOMPARE(quitAction->shortcut(), QKeySequence(Qt::CTRL | Qt::Key_Q));
    for (QShortcut* shortcut : window.findChildren<QShortcut*>())
        QVERIFY(!shortcut->keys().contains(quitAction->shortcut()));

    // Observe shortcut dispatch without terminating the test application.
    QVERIFY(quitAction->disconnect(qApp));
    QSignalSpy triggered(quitAction, &QAction::triggered);

    Editor* editor = window.findChild<Editor*>();
    QVERIFY(editor != nullptr);
    editor->setFocus();
    QTRY_VERIFY(editor->hasFocus());
    QTest::keyClick(editor, Qt::Key_Q, Qt::ControlModifier);
    QCOMPARE(triggered.count(), 1);

    window.setFocus();
    QTest::keyClick(&window, Qt::Key_Q, Qt::ControlModifier);
    QCOMPARE(triggered.count(), 2);
}

int main(int argc, char** argv)
{
    return UiTestFixture::run<TestSettingsUi>(argc, argv);
}

#include "testsettingsui.moc"
