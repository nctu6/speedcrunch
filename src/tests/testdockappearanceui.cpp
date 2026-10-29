// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mainwindowtesthelpers.h"
#include "core/startupdiagnostics.h"

using namespace MainWindowTestHelpers;

class TestDockAppearanceUi : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init() { UiTestFixture::resetSettings(); }
    void dock_refresh_reuses_unchanged_appearance_work_data();
    void dock_refresh_reuses_unchanged_appearance_work();
    void themed_header_metrics_refresh_after_font_change_data();
    void themed_header_metrics_refresh_after_font_change();
    void dock_appearance_defaults_to_theme_and_persists();
    void dock_appearance_switches_all_windows_data();
    void dock_appearance_switches_all_windows();
    void dock_tabs_follow_appearance_data();
    void dock_tabs_follow_appearance();
    void system_docks_follow_palette_and_theme_changes();
    void system_dock_separators_use_thin_system_colors_data();
    void system_dock_separators_use_thin_system_colors();
    void dock_headers_follow_appearance_data();
    void dock_headers_follow_appearance();
    void system_dock_headers_paint_system_background_data();
    void system_dock_headers_paint_system_background();
    void system_dock_controls_keep_aligned_rows_data();
    void system_dock_controls_keep_aligned_rows();
    void dock_header_buttons_render_full_size_data();
    void dock_header_buttons_render_full_size();
    void dock_header_buttons_render_full_size_on_first_run_data();
    void dock_header_buttons_render_full_size_on_first_run();
    void dock_header_buttons_stay_centered_in_title_bar_data();
    void dock_header_buttons_stay_centered_in_title_bar();
    void dock_header_buttons_keep_theme_after_redocking_data();
    void dock_header_buttons_keep_theme_after_redocking();
    void themed_headers_keep_native_scrollbar_styles();
};

void TestDockAppearanceUi::dock_refresh_reuses_unchanged_appearance_work_data()
{
    QTest::addColumn<bool>("system");
    QTest::newRow("theme") << false;
    QTest::newRow("system") << true;
}

void TestDockAppearanceUi::dock_refresh_reuses_unchanged_appearance_work()
{
    QFETCH(bool, system);
    MainWindowStateGuard guard;
    guard.settings->constantsDockVisible = true;
    guard.settings->dockAppearance = system ? Settings::DockAppearanceSystem : Settings::DockAppearanceSpeedCrunch;
    guard.settings->sessionLayoutJson.clear();
    guard.settings->windowState.clear();
    MainWindow window(false);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QCoreApplication::processEvents();
    StartupDiagnostics::begin();
    const auto finishRecording = qScopeGuard([]() {
        StartupDiagnostics::firstPaintFinished();
        StartupDiagnostics::finishWhenIdle();
    });
    QVERIFY(QMetaObject::invokeMethod(&window, "colorSchemeChanged", Qt::DirectConnection));
    QCoreApplication::processEvents();
    const auto snapshot = StartupDiagnostics::snapshot();
    int skippedChromePalettes = 0;
    for (const auto& assignment : snapshot.appearanceAssignments) {
        const auto& call = snapshot.appearanceCalls.at(assignment.call - 1);
        if (call.name == QLatin1String("Apply dock chrome appearance")
            && call.context == QLatin1String("Deferred dock refresh") && assignment.palette) {
            skippedChromePalettes += assignment.skipped;
        }
    }
    QVERIFY(skippedChromePalettes > 0);
    if (!system) {
        int reusedHeaders = 0;
        for (const auto& phase : snapshot.phases) {
            if (phase.name == QLatin1String("Reuse native dock header metrics"))
                reusedHeaders += phase.calls;
        }
        QVERIFY(reusedHeaders > 0);
    }
}

void TestDockAppearanceUi::themed_header_metrics_refresh_after_font_change_data()
{
    QTest::addColumn<bool>("modelFont");
    QTest::newRow("widget-font") << false;
    QTest::newRow("model-font") << true;
}

void TestDockAppearanceUi::themed_header_metrics_refresh_after_font_change()
{
    QFETCH(bool, modelFont);
    MainWindowStateGuard guard;
    guard.settings->constantsDockVisible = true;
    guard.settings->dockAppearance = Settings::DockAppearanceSpeedCrunch;
    guard.settings->sessionLayoutJson.clear();
    guard.settings->windowState.clear();
    MainWindow window(false);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QCoreApplication::processEvents();
    QDockWidget* constants = window.findChild<QDockWidget*>(QStringLiteral("ConstantsDock"));
    QVERIFY(constants);
    QTreeWidget* table = constants->findChild<QTreeWidget*>();
    QVERIFY(table);
    QHeaderView* header = table->header();
    const int oldHeight = header->sizeHint().height();
    StartupDiagnostics::begin();
    const auto finishRecording = qScopeGuard([]() {
        StartupDiagnostics::firstPaintFinished();
        StartupDiagnostics::finishWhenIdle();
    });
    QVERIFY(QMetaObject::invokeMethod(&window, "colorSchemeChanged", Qt::DirectConnection));
    const auto measuredHeaders = []() {
        for (const auto& phase : StartupDiagnostics::snapshot().phases) {
            if (phase.name == QLatin1String("Measure native dock header metrics"))
                return phase.calls;
        }
        return 0;
    };
    const int initialMeasurements = measuredHeaders();
    QVERIFY(initialMeasurements > 0);
    QFont font = header->font();
    font.setPixelSize(header->fontMetrics().height() + 12);
    if (modelFont)
        table->headerItem()->setFont(0, font);
    else
        header->setFont(font);
    QCoreApplication::processEvents();
    QVERIFY(measuredHeaders() > initialMeasurements);
    QVERIFY(header->sizeHint().height() > oldHeight);
}

void TestDockAppearanceUi::themed_headers_keep_native_scrollbar_styles()
{
    MainWindowStateGuard guard;
    guard.settings->constantsDockVisible = true;
    guard.settings->functionsDockVisible = true;
    guard.settings->variablesDockVisible = true;
    guard.settings->userFunctionsDockVisible = true;
    guard.settings->userUnitsDockVisible = true;
    MainWindow window(false);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QMenu* appearance = window.findChild<QMenu*>(QStringLiteral("DockAppearanceMenu"));
    QVERIFY(appearance);
    for (int mode : {1, 0, 1}) {
        appearance->actions().at(mode)->trigger();
        QCoreApplication::processEvents();
        int headersChecked = 0;
        for (QDockWidget* dock : window.findChildren<QDockWidget*>()) {
            for (QHeaderView* header : dock->widget()->findChildren<QHeaderView*>()) {
                QVERIFY(header->horizontalScrollBar()->styleSheet().isEmpty());
                QVERIFY(header->verticalScrollBar()->styleSheet().isEmpty());
                QVERIFY(header->cornerWidget() == nullptr);
                ++headersChecked;
            }
        }
        QVERIFY(headersChecked > 0);
    }
}

void TestDockAppearanceUi::initTestCase()
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

void TestDockAppearanceUi::system_dock_separators_use_thin_system_colors_data()
{
    QTest::addColumn<bool>("systemAtStartup");
    QTest::addColumn<bool>("classic");
    for (bool system : {false, true}) {
        for (bool classic : {false, true}) {
            const QString name = QStringLiteral("%1-%2")
                .arg(system ? "system-startup" : "theme-startup", classic ? "classic" : "modern");
            QTest::newRow(qPrintable(name)) << system << classic;
        }
    }
}

void TestDockAppearanceUi::system_dock_separators_use_thin_system_colors()
{
    QFETCH(bool, systemAtStartup);
    QFETCH(bool, classic);
    MainWindowStateGuard guard;
    const QPalette original = QApplication::palette();
    const auto restorePalette = qScopeGuard([original]() { QApplication::setPalette(original); });
    QPalette native = original;
    native.setColor(QPalette::Window, QColor("#eeeeee"));
    native.setColor(QPalette::Mid, QColor("#999999"));
    native.setColor(QPalette::Highlight, QColor("#2878b8"));
    QApplication::setPalette(native);
    guard.settings->dockAppearance = systemAtStartup
        ? Settings::DockAppearanceSystem : Settings::DockAppearanceSpeedCrunch;
    guard.settings->editorDisplayStyle = classic ? Settings::EditorDisplayClassic : Settings::EditorDisplayModern;
    guard.settings->constantsDockVisible = true;
    guard.settings->functionsDockVisible = true;
    guard.settings->windowState.clear();
    guard.settings->sessionLayoutJson.clear();
    guard.settings->colorScheme = QStringLiteral("Custom");
    guard.settings->customColorSchemeJson = themeJsonString(
        QJsonObject{{QStringLiteral("background"), QStringLiteral("#631840")}});
    MainWindow first(false);
    MainWindow second(false);
    first.setAnimated(false);
    second.setAnimated(false);
    first.resize(1100, 760);
    first.show();
    second.show();
    QVERIFY(QTest::qWaitForWindowExposed(&first));
    QVERIFY(QTest::qWaitForWindowExposed(&second));
    auto* constants = first.findChild<QDockWidget*>(QStringLiteral("ConstantsDock"));
    auto* functions = first.findChild<QDockWidget*>(QStringLiteral("FunctionsDock"));
    QVERIFY(constants);
    QVERIFY(functions);
    first.removeDockWidget(functions);
    first.addDockWidget(Qt::RightDockWidgetArea, functions);
    first.splitDockWidget(constants, functions, Qt::Vertical);
    functions->show();
    QMenu* appearance = first.findChild<QMenu*>(QStringLiteral("DockAppearanceMenu"));
    QVERIFY(appearance);
    QCursor::setPos(first.mapToGlobal(QPoint(-20, -20)));
    QCoreApplication::processEvents();

    const auto verify = [&](bool system) {
        for (MainWindow* window : {&first, &second}) {
            const QColor normal = system ? native.color(QPalette::Mid)
                : window->property("speedcrunchDockSeparatorNormalColor").value<QColor>();
            const QColor active = system ? native.color(QPalette::Highlight)
                : window->property("speedcrunchDockSeparatorActiveColor").value<QColor>();
            const int width = system ? 1 : UiConfig::DockSplitterStrokeWidth;
            // Include a native one-pixel handle so painting outside its bounds is visible.
            for (int extent : {1, 8}) {
                for (bool horizontal : {false, true}) {
                    const QSize size = horizontal ? QSize(32, extent) : QSize(extent, 32);
                    for (QStyle::State state : {QStyle::State(QStyle::State_None),
                                                QStyle::State(QStyle::State_MouseOver),
                                                QStyle::State(QStyle::State_Sunken)}) {
                        const QImage image = dockSeparatorPrimitiveImage(window, state,
                            QRect(QPoint(0, 0), size), size + QSize(2, 2));
                        const int start = (extent - 1) / 2 - (width - 1) / 2;
                        for (int offset = 0; offset < extent + 2; ++offset) {
                            const QColor pixel = horizontal ? image.pixelColor(16, offset)
                                                           : image.pixelColor(offset, 16);
                            if (offset >= start && offset < start + width)
                                QCOMPARE(pixel.rgba(), (state == QStyle::State_None ? normal : active).rgba());
                            else if (system && offset < extent)
                                QCOMPARE(pixel, native.color(QPalette::Window));
                            else
                                QCOMPARE(pixel.alpha(), 0);
                        }
                    }
                }
            }
        }
        if (!system)
            return;
        // Check actual window painting at the dock/editor and dock/dock boundaries.
        const QRect upper = constants->geometry();
        const QRect lower = functions->geometry();
        const QRect center = first.centralWidget()->geometry();
        const QRect vertical(center.right() + 1, upper.top(), upper.left() - center.right() - 1, upper.height());
        const QRect horizontal(upper.left(), upper.bottom() + 1, upper.width(), lower.top() - upper.bottom() - 1);
        const QImage image = first.grab().toImage();
        for (const QRect& gap : {vertical, horizontal}) {
            QVERIFY(!gap.isEmpty());
            const QPoint sample = gap.center();
            const qreal ratio = image.devicePixelRatio();
            QCOMPARE(image.pixelColor(qRound(sample.x() * ratio), qRound(sample.y() * ratio)),
                     native.color(QPalette::Mid));
            if (gap.width() > 1 && gap.height() > 1) {
                const QPoint edge = gap.width() > gap.height()
                    ? QPoint(sample.x(), gap.top()) : QPoint(gap.left(), sample.y());
                QCOMPARE(image.pixelColor(qRound(edge.x() * ratio), qRound(edge.y() * ratio)),
                         native.color(QPalette::Window));
            }
        }
    };
    verify(systemAtStartup);
    if (QTest::currentTestFailed())
        return;
    for (bool system : {true, false, true}) {
        appearance->actions().at(system ? 0 : 1)->trigger();
        QCoreApplication::processEvents();
        verify(system);
        if (QTest::currentTestFailed())
            return;
    }
    constants->setFloating(true);
    QCoreApplication::processEvents();
    constants->setFloating(false);
    QTRY_COMPARE(constants->geometry().left(), functions->geometry().left());
    verify(true);
    if (QTest::currentTestFailed())
        return;
    native.setColor(QPalette::Window, QColor("#252525"));
    native.setColor(QPalette::Mid, QColor("#686868"));
    native.setColor(QPalette::Highlight, QColor("#6cb5ee"));
    QApplication::setPalette(native);
    for (MainWindow* window : {&first, &second})
        QTRY_COMPARE(window->property("speedcrunchDockSeparatorNormalColor").value<QColor>(), native.color(QPalette::Mid));
    verify(true);
}

void TestDockAppearanceUi::dock_appearance_defaults_to_theme_and_persists()
{
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;
    QSettings persisted(Settings::getConfigPath() + QStringLiteral("/SpeedCrunch.ini"),
                        QSettings::IniFormat);
    const QString key = QStringLiteral("SpeedCrunch/Display/DockAppearance");
    persisted.remove(key);
    persisted.sync();
    settings->load();
    QCOMPARE(settings->dockAppearance, Settings::DockAppearanceSpeedCrunch);
    const QString scheme = settings->colorScheme;
    const Settings::MenuAppearance menus = settings->menuAppearance;
    for (const auto appearance : {Settings::DockAppearanceSystem, Settings::DockAppearanceSpeedCrunch}) {
        settings->dockAppearance = appearance;
        settings->save();
        settings->dockAppearance = appearance == Settings::DockAppearanceSystem
            ? Settings::DockAppearanceSpeedCrunch : Settings::DockAppearanceSystem;
        settings->load();
        QCOMPARE(settings->dockAppearance, appearance);
        QCOMPARE(settings->colorScheme, scheme);
        QCOMPARE(settings->menuAppearance, menus);
    }
    persisted.setValue(key, 99);
    persisted.sync();
    settings->load();
    QCOMPARE(settings->dockAppearance, Settings::DockAppearanceSpeedCrunch);
}

void TestDockAppearanceUi::dock_appearance_switches_all_windows_data()
{
    QTest::addColumn<bool>("floating");
    QTest::addColumn<bool>("themedMenus");
    QTest::newRow("attached-system-menus") << false << false;
    QTest::newRow("attached-themed-menus") << false << true;
    QTest::newRow("floating-system-menus") << true << false;
    QTest::newRow("floating-themed-menus") << true << true;
}

void TestDockAppearanceUi::dock_appearance_switches_all_windows()
{
    QFETCH(bool, floating);
    QFETCH(bool, themedMenus);
    MainWindowStateGuard guard;
    guard.settings->dockAppearance = Settings::DockAppearanceSpeedCrunch;
    guard.settings->menuAppearance = themedMenus ? Settings::MenuAppearanceSpeedCrunch : Settings::MenuAppearanceSystem;
    guard.settings->colorScheme = QStringLiteral("Custom");
    guard.settings->customColorSchemeJson = themeJsonString(
        QJsonObject{{QStringLiteral("background"), QStringLiteral("#232136")}});
    guard.settings->constantsDockVisible = true;
    guard.settings->formulaBookDockVisible = true;
    guard.settings->bitfieldVisible = true;
    MainWindow first(false);
    MainWindow second(false);
    first.show();
    second.show();
    QVERIFY(QTest::qWaitForWindowExposed(&first));
    QVERIFY(QTest::qWaitForWindowExposed(&second));
    QCoreApplication::processEvents();
    QMenu* firstAppearance = first.findChild<QMenu*>(QStringLiteral("DockAppearanceMenu"));
    QMenu* secondAppearance = second.findChild<QMenu*>(QStringLiteral("DockAppearanceMenu"));
    QVERIFY(firstAppearance);
    QVERIFY(secondAppearance);
    QVERIFY(firstAppearance->actions().size() >= 2);
    QVERIFY(firstAppearance->actions().at(1)->isChecked());
    QVERIFY(secondAppearance->actions().at(1)->isChecked());
    QDockWidget* constants = first.findChild<QDockWidget*>(QStringLiteral("ConstantsDock"));
    QVERIFY(constants);
    constants->setFloating(floating);
    constants->show();
    QTest::qWait(50);
    QLineEdit* search = constants->findChild<QLineEdit*>();
    QComboBox* combo = constants->findChild<QComboBox*>();
    QTreeWidget* table = constants->findChild<QTreeWidget*>();
    QVERIFY(search);
    QVERIFY(combo);
    QVERIFY(table);
    const QString themedSearchStyle = search->styleSheet();
    const QString themedComboStyle = combo->styleSheet();
    const QColor themedBase = table->palette().color(QPalette::Base);
    Editor* editor = first.findChild<Editor*>();
    QVERIFY(editor);
    const QPalette editorPalette = editor->palette();
    const QString editorStyle = editor->styleSheet();
    const QString dockTitleStyle = constants->styleSheet();
    for (int cycle = 0; cycle < 2; ++cycle) {
        firstAppearance->actions().at(0)->trigger();
        QCoreApplication::processEvents();
        QCOMPARE(guard.settings->dockAppearance, Settings::DockAppearanceSystem);
        QVERIFY(firstAppearance->actions().at(0)->isChecked());
        QVERIFY(secondAppearance->actions().at(0)->isChecked());
        QVERIFY(!firstAppearance->actions().at(1)->isChecked());
        for (const auto group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
            for (const auto role : {QPalette::Base, QPalette::Text, QPalette::Highlight, QPalette::HighlightedText})
                QCOMPARE(editor->palette().color(group, role).rgba(), editorPalette.color(group, role).rgba());
        }
        QCOMPARE(editor->styleSheet(), editorStyle);
        QVERIFY(constants->styleSheet().isEmpty());
        for (MainWindow* window : {&first, &second}) {
            for (QDockWidget* dock : window->findChildren<QDockWidget*>()) {
                QVERIFY(dock->widget());
                QCOMPARE(dock->widget()->palette().color(QPalette::Window).rgba(),
                         MenuStyle::systemPalette(dock->widget()).color(QPalette::Window).rgba());
                for (QLineEdit* edit : dock->widget()->findChildren<QLineEdit*>()) {
                    QVERIFY(edit->styleSheet().isEmpty());
                    QCOMPARE(edit->palette().color(QPalette::Base).rgba(),
                             MenuStyle::systemPalette(edit).color(QPalette::Base).rgba());
                    if (auto* themed = dynamic_cast<ThemedLineEdit*>(edit))
                        QVERIFY(!themed->cursorColor().isValid());
                }
            }
        }
        QVERIFY(search->styleSheet().isEmpty());
        QVERIFY(combo->styleSheet().isEmpty());
        QVERIFY(table->styleSheet().isEmpty());
        QCOMPARE(table->frameShape(), QFrame::NoFrame);
        QVERIFY(table->verticalScrollBar()->styleSheet().isEmpty());
        QVERIFY(table->cornerWidget());
        QVERIFY(table->cornerWidget()->styleSheet().isEmpty());
        QVERIFY(!table->property("dockListHoverBackground").isValid());
        QCOMPARE(table->palette().color(QPalette::Base).rgba(),
                 MenuStyle::systemPalette(table).color(QPalette::Base).rgba());
        search->setFocus();
        QCoreApplication::processEvents();
        combo->showPopup();
        QCoreApplication::processEvents();
        QVERIFY(combo->view()->styleSheet().isEmpty());
        QCOMPARE(combo->view()->palette().color(QPalette::Base).rgba(),
                 MenuStyle::systemComboPopupPalette(combo).color(QPalette::Base).rgba());
        QVERIFY(!combo->findChild<QWidget*>(QStringLiteral("speedcrunchDockComboBoxChevron"))->isVisible());
        combo->hidePopup();
        QVERIFY(table->model()->rowCount() > 0);
        const QModelIndex index = table->model()->index(0, 0);
        QStyleOptionViewItem option;
        option.initFrom(table);
        option.widget = table;
        option.rect = QRect(0, 0, 240, 30);
        option.state |= QStyle::State_Selected | QStyle::State_HasFocus | QStyle::State_MouseOver;
        QImage actual(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
        QImage expected(actual.size(), actual.format());
        actual.fill(Qt::transparent);
        expected.fill(Qt::transparent);
        QPainter actualPainter(&actual);
        table->itemDelegate()->paint(&actualPainter, option, index);
        actualPainter.end();
        QPainter expectedPainter(&expected);
        QStyledItemDelegate nativeDelegate;
        nativeDelegate.paint(&expectedPainter, option, index);
        expectedPainter.end();
        QCOMPARE(actual, expected);
        QTextBrowser* browser = first.findChild<QTextBrowser*>();
        QVERIFY(browser);
        QCOMPARE(browser->palette().color(QPalette::Base).rgba(),
                 MenuStyle::systemPalette(browser).color(QPalette::Base).rgba());
        BitFieldWidget* bits = first.findChild<BitFieldWidget*>();
        QVERIFY(bits);
        for (QPushButton* button : bits->findChildren<QPushButton*>())
            QVERIFY(button->styleSheet().isEmpty());
        secondAppearance->actions().at(1)->trigger();
        QCoreApplication::processEvents();
        QCOMPARE(guard.settings->dockAppearance, Settings::DockAppearanceSpeedCrunch);
        QVERIFY(firstAppearance->actions().at(1)->isChecked());
        QCOMPARE(constants->styleSheet(), dockTitleStyle);
        QCOMPARE(search->styleSheet(), themedSearchStyle);
        QCOMPARE(combo->styleSheet(), themedComboStyle);
        QCOMPARE(table->palette().color(QPalette::Base).rgba(), themedBase.rgba());
    }
    firstAppearance->actions().at(0)->trigger();
    MainWindow third(false);
    QMenu* thirdAppearance = third.findChild<QMenu*>(QStringLiteral("DockAppearanceMenu"));
    QVERIFY(thirdAppearance);
    QVERIFY(thirdAppearance->actions().at(0)->isChecked());
    QDockWidget* thirdConstants = third.findChild<QDockWidget*>(QStringLiteral("ConstantsDock"));
    QVERIFY(thirdConstants);
    QVERIFY(thirdConstants->findChild<QLineEdit*>()->styleSheet().isEmpty());
    QCOMPARE(guard.settings->menuAppearance,
             themedMenus ? Settings::MenuAppearanceSpeedCrunch : Settings::MenuAppearanceSystem);
}

void TestDockAppearanceUi::dock_tabs_follow_appearance_data()
{
    QTest::addColumn<bool>("systemAtStartup");
    QTest::addColumn<bool>("dark");
    for (bool system : {false, true}) {
        for (bool dark : {false, true}) {
            const QString name = QStringLiteral("%1-%2")
                .arg(system ? "system-startup" : "theme-startup", dark ? "dark" : "light");
            QTest::newRow(qPrintable(name)) << system << dark;
        }
    }
}

void TestDockAppearanceUi::dock_tabs_follow_appearance()
{
    QFETCH(bool, systemAtStartup);
    QFETCH(bool, dark);
    MainWindowStateGuard guard;
    const QPalette original = QApplication::palette();
    const auto restorePalette = qScopeGuard([original]() { QApplication::setPalette(original); });
    QPalette native = original;
    native.setColor(QPalette::Window, QColor(dark ? "#252525" : "#eeeeee"));
    native.setColor(QPalette::WindowText, QColor(dark ? "#eeeeee" : "#252525"));
    native.setColor(QPalette::Button, native.color(QPalette::Window));
    native.setColor(QPalette::ButtonText, native.color(QPalette::WindowText));
    QApplication::setPalette(native);
    guard.settings->dockAppearance = systemAtStartup ? Settings::DockAppearanceSystem : Settings::DockAppearanceSpeedCrunch;
    guard.settings->menuAppearance = Settings::MenuAppearanceSpeedCrunch;
    guard.settings->constantsDockVisible = true;
    guard.settings->functionsDockVisible = true;
    guard.settings->windowState.clear();
    guard.settings->sessionLayoutJson.clear();
    guard.settings->colorScheme = QStringLiteral("Custom");
    guard.settings->customColorSchemeJson = themeJsonString(
        QJsonObject{{QStringLiteral("background"), QStringLiteral("#631840")}});
    MainWindow first(false);
    MainWindow second(false);
    first.show();
    second.show();
    QVERIFY(QTest::qWaitForWindowExposed(&first));
    QVERIFY(QTest::qWaitForWindowExposed(&second));
    QMenu* appearance = first.findChild<QMenu*>(QStringLiteral("DockAppearanceMenu"));
    QVERIFY(appearance);
    const auto findTabs = [](MainWindow* window) -> QTabBar* {
        for (QTabBar* bar : window->findChildren<QTabBar*>()) {
            for (int index = 0; index < bar->count(); ++index) {
                if (bar->tabText(index) == QStringLiteral("Constants"))
                    return bar;
            }
        }
        return nullptr;
    };
    const auto verify = [&](MainWindow* window, bool system) {
        QTabBar* bar = findTabs(window);
        QVERIFY(bar);
        if (!system) {
            QVERIFY(bar->styleSheet().contains(QStringLiteral("border-radius: 10px")));
            QVERIFY(!bar->drawBase());
            return;
        }
        QVERIFY(bar->styleSheet().isEmpty());
        QVERIFY(bar->drawBase());
        QCOMPARE(bar->cursor().shape(), Qt::ArrowCursor);
        for (const auto group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
            for (const auto role : {QPalette::Window, QPalette::WindowText, QPalette::Button,
                                     QPalette::ButtonText, QPalette::Highlight, QPalette::HighlightedText}) {
                QTRY_COMPARE(bar->palette().color(group, role).rgba(),
                             MenuStyle::systemPalette(bar).color(group, role).rgba());
            }
        }
        const QPoint unusedSpace(bar->width() - 2, bar->height() / 2);
        bool insideTab = false;
        for (int index = 0; index < bar->count(); ++index)
            insideTab |= bar->tabRect(index).contains(unusedSpace);
        if (!insideTab && bar->isVisible()) {
            const QImage image = window->grab().toImage();
            const QPoint mapped = bar->mapTo(window, unusedSpace);
            QCOMPARE(image.pixelColor(qRound(mapped.x() * window->devicePixelRatioF()),
                                       qRound(mapped.y() * window->devicePixelRatioF())).rgba(),
                     MenuStyle::systemPalette(bar).color(QPalette::Window).rgba());
        }
        for (bool selected : {false, true}) {
            QStyleOptionTab option;
            option.initFrom(bar);
            option.rect = QRect(0, 0, 140, 32);
            option.shape = bar->shape();
            option.text = QStringLiteral("Constants");
            option.position = QStyleOptionTab::Beginning;
            option.state.setFlag(QStyle::State_Selected, selected);
            QImage actual(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
            QImage expected(actual.size(), actual.format());
            actual.fill(Qt::transparent);
            expected.fill(Qt::transparent);
            QPainter actualPainter(&actual);
            bar->style()->drawControl(QStyle::CE_TabBarTab, &option, &actualPainter, bar);
            actualPainter.end();
            QPainter expectedPainter(&expected);
            QApplication::style()->drawControl(QStyle::CE_TabBarTab, &option, &expectedPainter, bar);
            expectedPainter.end();
            QCOMPARE(actual, expected);
        }
    };
    QCoreApplication::processEvents();
    verify(&first, systemAtStartup);
    verify(&second, systemAtStartup);
    for (bool system : {true, false, true, false}) {
        appearance->actions().at(system ? 0 : 1)->trigger();
        QCoreApplication::processEvents();
        verify(&first, system);
        verify(&second, system);
    }
    appearance->actions().at(0)->trigger();
    native.setColor(QPalette::Window, QColor(dark ? "#353535" : "#dddddd"));
    QApplication::setPalette(native);
    QCoreApplication::processEvents();
    verify(&first, true);
    verify(&second, true);
    QDockWidget* functions = first.findChild<QDockWidget*>(QStringLiteral("FunctionsDock"));
    QDockWidget* constants = first.findChild<QDockWidget*>(QStringLiteral("ConstantsDock"));
    QVERIFY(functions);
    QVERIFY(constants);
    functions->setFloating(true);
    functions->setFloating(false);
    first.tabifyDockWidget(constants, functions);
    functions->show();
    QCoreApplication::processEvents();
    verify(&first, true);
    QVERIFY(QMetaObject::invokeMethod(&first, "setHistoryDockVisible", Q_ARG(bool, true)));
    QCoreApplication::processEvents();
    verify(&first, true);
    QCOMPARE(guard.settings->menuAppearance, Settings::MenuAppearanceSpeedCrunch);
}

void TestDockAppearanceUi::dock_headers_follow_appearance_data()
{
    QTest::addColumn<bool>("systemAtStartup");
    QTest::addColumn<bool>("floating");
    QTest::addColumn<bool>("dark");
    for (bool system : {false, true}) {
        for (bool floating : {false, true}) {
            for (bool dark : {false, true}) {
                const QString name = QStringLiteral("%1-%2-%3")
                    .arg(system ? "system-startup" : "theme-startup",
                         floating ? "floating" : "attached", dark ? "dark" : "light");
                QTest::newRow(qPrintable(name)) << system << floating << dark;
            }
        }
    }
}

void TestDockAppearanceUi::dock_headers_follow_appearance()
{
    QFETCH(bool, systemAtStartup);
    QFETCH(bool, floating);
    QFETCH(bool, dark);
    MainWindowStateGuard guard;
    const QPalette original = QApplication::palette();
    const auto restorePalette = qScopeGuard([original]() { QApplication::setPalette(original); });
    QPalette native = original;
    native.setColor(QPalette::Window, QColor(dark ? "#252525" : "#eeeeee"));
    native.setColor(QPalette::WindowText, QColor(dark ? "#eeeeee" : "#252525"));
    native.setColor(QPalette::Button, native.color(QPalette::Window));
    native.setColor(QPalette::ButtonText, native.color(QPalette::WindowText));
    QApplication::setPalette(native);
    guard.settings->dockAppearance = systemAtStartup ? Settings::DockAppearanceSystem : Settings::DockAppearanceSpeedCrunch;
    guard.settings->constantsDockVisible = true;
    guard.settings->functionsDockVisible = true;
    guard.settings->formulaBookDockVisible = true;
    guard.settings->bitfieldVisible = true;
    guard.settings->windowState.clear();
    guard.settings->sessionLayoutJson.clear();
    guard.settings->colorScheme = QStringLiteral("Custom");
    guard.settings->customColorSchemeJson = themeJsonString(
        QJsonObject{{QStringLiteral("background"), QStringLiteral("#631840")}});
    MainWindow first(false);
    MainWindow second(false);
    first.show();
    second.show();
    QVERIFY(QTest::qWaitForWindowExposed(&first));
    QVERIFY(QTest::qWaitForWindowExposed(&second));
    QMenu* appearance = first.findChild<QMenu*>(QStringLiteral("DockAppearanceMenu"));
    QVERIFY(appearance);
    QDockWidget* constants = first.findChild<QDockWidget*>(QStringLiteral("ConstantsDock"));
    QVERIFY(constants);
    constants->setFloating(floating);
    constants->show();
    QDockWidget reference;
    reference.setPalette(native);
    reference.ensurePolished();
    const QStringList names {QStringLiteral("qt_dockwidget_closebutton"),
                             QStringLiteral("qt_dockwidget_floatbutton")};
    const auto verifyNative = [&](QDockWidget* dock) {
        QVERIFY(dock->styleSheet().isEmpty());
        for (auto group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
            for (auto role : {QPalette::Window, QPalette::WindowText, QPalette::Button, QPalette::ButtonText})
                QCOMPARE(dock->palette().color(group, role).rgba(), native.color(group, role).rgba());
        }
        QStyleOptionDockWidget option;
        option.initFrom(dock);
        option.rect = QRect(0, 0, 240, 30);
        option.title = dock->windowTitle();
        option.closable = dock->features().testFlag(QDockWidget::DockWidgetClosable);
        option.floatable = dock->features().testFlag(QDockWidget::DockWidgetFloatable);
        option.movable = dock->features().testFlag(QDockWidget::DockWidgetMovable);
        option.verticalTitleBar = dock->features().testFlag(QDockWidget::DockWidgetVerticalTitleBar);
        QImage actual(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
        actual.fill(Qt::transparent);
        QImage expected = actual;
        QPainter actualPainter(&actual);
        dock->style()->drawControl(QStyle::CE_DockWidgetTitle, &option, &actualPainter, dock);
        actualPainter.end();
        QPainter expectedPainter(&expected);
        QApplication::style()->drawControl(QStyle::CE_DockWidgetTitle, &option, &expectedPainter, dock);
        expectedPainter.end();
        QCOMPARE(actual, expected);
        for (const QString& name : names) {
            QAbstractButton* button = dock->findChild<QAbstractButton*>(name, Qt::FindDirectChildrenOnly);
            QAbstractButton* referenceButton = reference.findChild<QAbstractButton*>(name);
            QVERIFY(button);
            QVERIFY(referenceButton);
            QVERIFY(!button->property("speedcrunchDockHeaderButton").toBool());
            QVERIFY(button->styleSheet().isEmpty());
            QCOMPARE(button->minimumSize(), referenceButton->minimumSize());
            QCOMPARE(button->maximumSize(), referenceButton->maximumSize());
            QCOMPARE(button->iconSize(), referenceButton->iconSize());
            QCOMPARE(button->cursor().shape(), referenceButton->cursor().shape());
            const auto standard = name == names.first() ? QStyle::SP_TitleBarCloseButton : QStyle::SP_TitleBarNormalButton;
            const QImage icon = button->icon().pixmap(QSize(24, 24)).toImage();
            const QImage nativeIcon = QApplication::style()->standardIcon(standard, &option, dock).pixmap(QSize(24, 24)).toImage();
            QCOMPARE(icon, nativeIcon);
            const QPoint center = button->rect().center();
            QEnterEvent enter(center, button->mapTo(button->window(), center), button->mapToGlobal(center));
            QCoreApplication::sendEvent(button, &enter);
            QCOMPARE(button->icon().pixmap(QSize(24, 24)).toImage(), nativeIcon);
        }
    };
    for (bool system : {systemAtStartup, !systemAtStartup, systemAtStartup, true}) {
        appearance->actions().at(system ? 0 : 1)->trigger();
        QCoreApplication::processEvents();
        for (MainWindow* window : {&first, &second}) {
            for (QDockWidget* dock : window->findChildren<QDockWidget*>()) {
                if (system) {
                    verifyNative(dock);
                    if (QTest::currentTestFailed())
                        return;
                } else {
                    QVERIFY(!dock->styleSheet().isEmpty());
                    for (const QString& name : names) {
                        QAbstractButton* button = dock->findChild<QAbstractButton*>(name, Qt::FindDirectChildrenOnly);
                        QVERIFY(button);
                        QVERIFY(button->property("speedcrunchDockHeaderButton").toBool());
                        QCOMPARE(button->size(), QSize(18, 18));
                    }
                }
            }
        }
    }
    for (bool floatingState : {true, false}) {
        constants->setFloating(floatingState);
        QCoreApplication::processEvents();
        verifyNative(constants);
        if (QTest::currentTestFailed())
            return;
    }
    native.setColor(QPalette::Window, QColor(dark ? "#353535" : "#dddddd"));
    native.setColor(QPalette::Button, native.color(QPalette::Window));
    QApplication::setPalette(native);
    QCoreApplication::processEvents();
    for (MainWindow* window : {&first, &second}) {
        for (QDockWidget* dock : window->findChildren<QDockWidget*>()) {
            QTRY_COMPARE(dock->palette().color(QPalette::Window).rgba(), native.color(QPalette::Window).rgba());
            verifyNative(dock);
            if (QTest::currentTestFailed())
                return;
        }
    }
    MainWindow third(false);
    for (QDockWidget* dock : third.findChildren<QDockWidget*>()) {
        verifyNative(dock);
        if (QTest::currentTestFailed())
            return;
    }
    QAbstractButton* floatButton = constants->findChild<QAbstractButton*>(names.last());
    QTest::mouseClick(floatButton, Qt::LeftButton);
    QVERIFY(constants->isFloating());
    constants->setFloating(false);
    QCoreApplication::processEvents();
    verifyNative(constants);
    if (QTest::currentTestFailed())
        return;
    QAbstractButton* closeButton = constants->findChild<QAbstractButton*>(names.first());
    QTest::mouseClick(closeButton, Qt::LeftButton);
    QVERIFY(!constants->isVisible());
}

void TestDockAppearanceUi::system_dock_headers_paint_system_background_data()
{
    QTest::addColumn<bool>("switchFromTheme");
    QTest::addColumn<bool>("darkSystem");
    for (bool switching : {false, true}) {
        for (bool dark : {false, true}) {
            const QString name = QStringLiteral("%1-%2")
                .arg(switching ? "switch-from-theme" : "system-startup",
                     dark ? "dark-system" : "light-system");
            QTest::newRow(qPrintable(name)) << switching << dark;
        }
    }
}

void TestDockAppearanceUi::system_dock_headers_paint_system_background()
{
    QFETCH(bool, switchFromTheme);
    QFETCH(bool, darkSystem);
    MainWindowStateGuard guard;
    const QPalette original = QApplication::palette();
    const auto restorePalette = qScopeGuard([original]() { QApplication::setPalette(original); });
    QPalette native = original;
    native.setColor(QPalette::Window, QColor(darkSystem ? "#252525" : "#eeeeee"));
    native.setColor(QPalette::WindowText, QColor(darkSystem ? "#eeeeee" : "#252525"));
    QApplication::setPalette(native);
    guard.settings->dockAppearance = switchFromTheme
        ? Settings::DockAppearanceSpeedCrunch : Settings::DockAppearanceSystem;
    guard.settings->constantsDockVisible = true;
    guard.settings->windowState.clear();
    guard.settings->sessionLayoutJson.clear();
    guard.settings->colorScheme = QStringLiteral("Custom");
    guard.settings->customColorSchemeJson = themeJsonString(
        QJsonObject{{QStringLiteral("background"), QStringLiteral("#631840")}});
    MainWindow window(false);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QDockWidget* dock = window.findChild<QDockWidget*>(QStringLiteral("ConstantsDock"));
    QVERIFY(dock);
    if (switchFromTheme) {
        QMenu* appearance = window.findChild<QMenu*>(QStringLiteral("DockAppearanceMenu"));
        QVERIFY(appearance);
        appearance->actions().first()->trigger();
    }
    QCoreApplication::processEvents();

    // Render the whole window so a transparent header exposes its parent fill.
    // Painting CE_DockWidgetTitle alone cannot detect that background leak.
    const auto verifyHeader = [&]() {
        const QAbstractButton* button = dock->findChild<QAbstractButton*>(
            QStringLiteral("qt_dockwidget_floatbutton"), Qt::FindDirectChildrenOnly);
        QVERIFY(button);
        const QPoint sample(button->geometry().left() - 8, button->geometry().center().y());
        QVERIFY(dock->rect().contains(sample));
        QVERIFY(sample.y() < dock->widget()->geometry().top());
        QStyleOptionDockWidget option;
        option.initFrom(dock);
        option.rect = QRect(0, 0, dock->width(), dock->widget()->geometry().top());
        option.title = dock->windowTitle();
        option.closable = dock->features().testFlag(QDockWidget::DockWidgetClosable);
        option.floatable = dock->features().testFlag(QDockWidget::DockWidgetFloatable);
        option.movable = dock->features().testFlag(QDockWidget::DockWidgetMovable);
        // Preserve any background the native title painter adds, such as the
        // Windows style's gradient, over the system window surface.
        QImage expected(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
        expected.fill(native.color(QPalette::Window));
        QPainter painter(&expected);
        QApplication::style()->drawControl(QStyle::CE_DockWidgetTitle, &option, &painter, dock);
        painter.end();
        const QImage image = window.grab().toImage();
        const QPoint position = dock->mapTo(&window, sample);
        QCOMPARE(image.pixelColor(qRound(position.x() * image.devicePixelRatio()),
                                  qRound(position.y() * image.devicePixelRatio())),
                 expected.pixelColor(sample));
    };
    verifyHeader();
    if (QTest::currentTestFailed())
        return;

    native.setColor(QPalette::Window, QColor(darkSystem ? "#353535" : "#dddddd"));
    QApplication::setPalette(native);
    QTRY_COMPARE(dock->palette().color(QPalette::Window), native.color(QPalette::Window));
    verifyHeader();
}

void TestDockAppearanceUi::system_dock_controls_keep_aligned_rows_data()
{
    QTest::addColumn<bool>("switchFromTheme");
    QTest::addColumn<bool>("wide");
    QTest::addColumn<bool>("subdomains");
    QTest::addColumn<bool>("largeFont");
    for (bool switching : {false, true}) {
        for (bool wide : {false, true}) {
            for (bool subdomains : {false, true}) {
                for (bool largeFont : {false, true}) {
                    const QString name = QStringLiteral("%1-%2-%3-%4")
                        .arg(switching ? "switch" : "startup", wide ? "wide" : "compact",
                             subdomains ? "subdomains" : "no-subdomains", largeFont ? "large-font" : "default-font");
                    QTest::newRow(qPrintable(name)) << switching << wide << subdomains << largeFont;
                }
            }
        }
    }
}

void TestDockAppearanceUi::system_dock_controls_keep_aligned_rows()
{
    QFETCH(bool, switchFromTheme);
    QFETCH(bool, wide);
    QFETCH(bool, subdomains);
    QFETCH(bool, largeFont);
    MainWindowStateGuard guard;
    guard.settings->dockAppearance = switchFromTheme ? Settings::DockAppearanceSpeedCrunch : Settings::DockAppearanceSystem;
    guard.settings->constantsDockVisible = true;
    guard.settings->constantsDockDomain.clear();
    guard.settings->constantsDockSubdomain.clear();
    guard.settings->functionsDockVisible = true;
    guard.settings->sessionLayoutJson.clear();
    guard.settings->windowState.clear();
    MainWindow window(false);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    if (largeFont) {
        QFont font = window.font();
        font.setPointSize(font.pointSize() + 4);
        window.setFont(font);
    }
    if (switchFromTheme) {
        QMenu* appearance = window.findChild<QMenu*>(QStringLiteral("DockAppearanceMenu"));
        QVERIFY(appearance);
        appearance->actions().at(0)->trigger();
    }
    for (const char* name : {"ConstantsDock", "FunctionsDock"}) {
        QDockWidget* dock = window.findChild<QDockWidget*>(QString::fromLatin1(name));
        QVERIFY(dock);
        dock->setFloating(true);
        dock->resize(wide ? 900 : 380, 480);
        dock->show();
        if (auto* constants = qobject_cast<ConstantsWidget*>(dock->widget())) {
            QComboBox* domain = constants->findChild<QComboBox*>();
            QVERIFY(domain);
            constants->restoreState(domain->itemText(subdomains ? 0 : 1), QString(), QString());
        }
        QTest::qWait(50);
        QWidget* content = dock->widget();
        QLineEdit* search = content->findChild<QLineEdit*>();
        QVERIFY(search);
        const auto layoutRect = [content](QComboBox* combo) {
            QStyleOptionComboBox option;
            option.initFrom(combo);
            option.rect = combo->rect();
            QRect rect = combo->style()->subElementRect(QStyle::SE_ComboBoxLayoutItem, &option, combo);
            if (rect.isEmpty())
                rect = combo->rect();
            rect.translate(combo->mapTo(content, QPoint()));
            return rect;
        };
        QList<QRect> combos;
        for (QComboBox* combo : content->findChildren<QComboBox*>()) {
            if (combo->isVisible()) {
                combos.append(layoutRect(combo));
            }
        }
        QVERIFY(!combos.isEmpty());
        std::sort(combos.begin(), combos.end(), [](const QRect& a, const QRect& b) {
            return a.top() == b.top() ? a.left() < b.left() : a.top() < b.top();
        });
        const QRect searchRect(search->mapTo(content, QPoint()), search->size());
        QCOMPARE(combos.first().left(), searchRect.left());
        QCOMPARE(combos.last().right(), searchRect.right());
        QCOMPARE(combos.first().top(), UiConfig::DockSearchRowVerticalPadding);
        QCOMPARE(searchRect.top() - (combos.last().bottom() + 1), UiConfig::DockSearchRowVerticalPadding);
        if (QString::fromLatin1(name) == QStringLiteral("ConstantsDock"))
            QCOMPARE(combos.size(), subdomains ? 2 : 1);
        if (combos.size() == 2) {
            if (wide)
                QCOMPARE(combos.first().top(), combos.last().top());
            else
                QCOMPARE(combos.last().top() - (combos.first().bottom() + 1), UiConfig::DockSearchRowVerticalPadding);
        }
    }
}

void TestDockAppearanceUi::system_docks_follow_palette_and_theme_changes()
{
    MainWindowStateGuard guard;
    const QPalette original = QApplication::palette();
    const auto restorePalette = qScopeGuard([original]() { QApplication::setPalette(original); });
    guard.settings->dockAppearance = Settings::DockAppearanceSpeedCrunch;
    guard.settings->constantsDockVisible = true;
    MainWindow window(false);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QDockWidget* dock = window.findChild<QDockWidget*>(QStringLiteral("ConstantsDock"));
    QVERIFY(dock);
    QLineEdit* search = dock->findChild<QLineEdit*>();
    QTreeWidget* table = dock->findChild<QTreeWidget*>();
    QVERIFY(search);
    QVERIFY(table);
    search->setText(QStringLiteral("no matching constant 987654321"));
    QLabel* noMatch = nullptr;
    QTRY_VERIFY(([&]() {
        for (QLabel* label : table->viewport()->findChildren<QLabel*>()) {
            if (label->property("dockListNoMatchLabel").toBool())
                noMatch = label;
        }
        return noMatch != nullptr;
    })());
    QMenu* appearance = window.findChild<QMenu*>(QStringLiteral("DockAppearanceMenu"));
    QVERIFY(appearance);
    appearance->actions().at(0)->trigger();
    QVERIFY(!noMatch->styleSheet().contains(QStringLiteral("color:")));
    QPalette changed = original;
    changed.setColor(QPalette::Base, QColor("#304050"));
    changed.setColor(QPalette::Text, QColor("#f1f2f3"));
    changed.setColor(QPalette::Window, QColor("#405060"));
    changed.setColor(QPalette::WindowText, QColor("#f1f2f3"));
    changed.setColor(QPalette::Highlight, QColor("#108030"));
    QApplication::setPalette(changed);
    QTRY_COMPARE(search->palette().color(QPalette::Base).rgba(), changed.color(QPalette::Base).rgba());
    QTRY_COMPARE(table->palette().color(QPalette::Highlight).rgba(), changed.color(QPalette::Highlight).rgba());
    QTRY_COMPARE(noMatch->palette().color(QPalette::WindowText).rgba(), changed.color(QPalette::WindowText).rgba());
    guard.settings->colorScheme = QStringLiteral("Custom");
    guard.settings->customColorSchemeJson = themeJsonString(
        QJsonObject{{QStringLiteral("background"), QStringLiteral("#730a24")}});
    window.colorSchemeChanged();
    QCoreApplication::processEvents();
    QCOMPARE(search->palette().color(QPalette::Base).rgba(), changed.color(QPalette::Base).rgba());
    QVERIFY(search->styleSheet().isEmpty());
    QVERIFY(table->styleSheet().isEmpty());
}

void TestDockAppearanceUi::dock_header_buttons_render_full_size_data()
{
    QTest::addColumn<bool>("framed");
    QTest::addColumn<QString>("background");
    QTest::newRow("dark-framed") << true << QStringLiteral("#402034");
    QTest::newRow("dark-unframed") << false << QStringLiteral("#402034");
    QTest::newRow("light-framed") << true << QStringLiteral("#f4e8ee");
    QTest::newRow("light-unframed") << false << QStringLiteral("#f4e8ee");
}

void TestDockAppearanceUi::dock_header_buttons_render_full_size()
{
    QFETCH(bool, framed);
    QFETCH(QString, background);
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;
    settings->colorScheme = QStringLiteral("Custom");
    settings->customColorSchemeJson = themeJsonString(
        QJsonObject{{QStringLiteral("background"), background}});
    settings->windowState.clear();
    settings->constantsDockVisible = true;

    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QDockWidget* dock = window.findChild<QDockWidget*>(QStringLiteral("ConstantsDock"));
    QVERIFY(dock != nullptr);
    dock->setFloating(false);
    dock->show();
    dock->raise();
    QCoreApplication::processEvents();
    QAbstractButton* closeButton = dock->findChild<QAbstractButton*>(
        QStringLiteral("qt_dockwidget_closebutton"));
    QAbstractButton* floatButton = dock->findChild<QAbstractButton*>(
        QStringLiteral("qt_dockwidget_floatbutton"));
    QVERIFY(closeButton != nullptr);
    QVERIFY(floatButton != nullptr);

    for (QAbstractButton* button : {closeButton, floatButton}) {
        auto* style = new DockHeaderButtonTestStyle(framed);
        style->setParent(button);
        button->setStyle(style);
        QCOMPARE(button->style()->styleHint(QStyle::SH_DockWidget_ButtonsHaveFrame,
                                           nullptr, button), int(framed));
        QCOMPARE(button->style()->pixelMetric(QStyle::PM_SmallIconSize,
                                              nullptr, button), 16);
        QVERIFY(button->isVisible());

        for (bool hovered : {false, true}) {
            QEvent hoverEvent(hovered ? QEvent::Enter : QEvent::Leave);
            QCoreApplication::sendEvent(button, &hoverEvent);
            const QImage rendered = button->grab().toImage();
            const qreal dpr = rendered.devicePixelRatio();
            const QImage expected = button->icon().pixmap(button->iconSize(), dpr).toImage();
            QCOMPARE(rendered.size(), expected.size());

            // Compare the actual widget's symbol with the full-size source icon.
            // Checking iconSize() alone misses Qt's private dock-button scaling.
            const QRect symbolRect(qRound(4 * dpr), qRound(4 * dpr),
                                   qRound(10 * dpr), qRound(10 * dpr));
            for (int y = symbolRect.top(); y <= symbolRect.bottom(); ++y) {
                for (int x = symbolRect.left(); x <= symbolRect.right(); ++x) {
                    QVERIFY2(colorsAreClose(rendered.pixelColor(x, y),
                                            expected.pixelColor(x, y), 3),
                             qPrintable(QStringLiteral("%1 symbol differs at %2,%3")
                                            .arg(button->objectName()).arg(x).arg(y)));
                }
            }
        }
    }

    QTest::mouseClick(floatButton, Qt::LeftButton);
    QTRY_VERIFY(dock->isFloating());
    dock->setFloating(false);
    QTRY_VERIFY(closeButton->isVisible());
    QTest::mouseClick(closeButton, Qt::LeftButton);
    QTRY_VERIFY(!dock->isVisible());
}

void TestDockAppearanceUi::dock_header_buttons_render_full_size_on_first_run_data()
{
    dock_header_buttons_render_full_size_data();
}

void TestDockAppearanceUi::dock_header_buttons_render_full_size_on_first_run()
{
    Settings* settings = Settings::instance();
    const bool oldHasNumberFormatStyleSetting = settings->hasNumberFormatStyleSetting;
    const auto restoreSettings = qScopeGuard([&]() {
        settings->hasNumberFormatStyleSetting = oldHasNumberFormatStyleSetting;
    });
    settings->hasNumberFormatStyleSetting = false;
    // Exercise the shared fixture with fresh first-run settings; no window
    // guard should need to suppress this dialog on its own.
    UiTestFixture::resetSettings();
    QVERIFY(settings->hasNumberFormatStyleSetting);
    dock_header_buttons_render_full_size();
    QVERIFY(QApplication::activeModalWidget() == nullptr);
    QVERIFY(settings->hasNumberFormatStyleSetting);
}

void TestDockAppearanceUi::dock_header_buttons_stay_centered_in_title_bar_data()
{
    QTest::addColumn<int>("fontPixelSize");
    QTest::addColumn<bool>("verticalTitleBar");
    QTest::newRow("small-horizontal") << 9 << false;
    QTest::newRow("normal-horizontal") << 13 << false;
    QTest::newRow("large-horizontal") << 22 << false;
    QTest::newRow("small-vertical") << 9 << true;
    QTest::newRow("large-vertical") << 22 << true;
}

void TestDockAppearanceUi::dock_header_buttons_stay_centered_in_title_bar()
{
    QFETCH(int, fontPixelSize);
    QFETCH(bool, verticalTitleBar);
    MainWindowStateGuard guard;
    guard.settings->windowState.clear();
    guard.settings->constantsDockVisible = true;

    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QDockWidget* dock = window.findChild<QDockWidget*>(QStringLiteral("ConstantsDock"));
    QVERIFY(dock != nullptr);
    dock->setFloating(false);
    dock->show();
    dock->raise();
    if (verticalTitleBar)
        dock->setFeatures(dock->features() | QDockWidget::DockWidgetVerticalTitleBar);
    QFont font = dock->font();
    font.setPixelSize(fontPixelSize);
    dock->setFont(font);

    for (const QSize size : {QSize(800, 600), QSize(1050, 750)}) {
        window.resize(size);
        QCoreApplication::processEvents();
        const int titleExtent = verticalTitleBar
            ? dock->widget()->geometry().left() : dock->widget()->geometry().top();
        QVERIFY(titleExtent >= 18);
        for (const QString& name : {QStringLiteral("qt_dockwidget_closebutton"),
                                    QStringLiteral("qt_dockwidget_floatbutton")}) {
            QAbstractButton* button = dock->findChild<QAbstractButton*>(name);
            QVERIFY(button != nullptr);
            QVERIFY(button->isVisible());
            const QRect rect = button->geometry();
            const int twiceButtonCenter = verticalTitleBar
                ? 2 * rect.x() + rect.width() : 2 * rect.y() + rect.height();
            QVERIFY2(qAbs(twiceButtonCenter - titleExtent) <= 1,
                     qPrintable(QStringLiteral("%1 center %2 differs from title center %3")
                                    .arg(name).arg(twiceButtonCenter / 2.0)
                                    .arg(titleExtent / 2.0)));
        }
    }
}

void TestDockAppearanceUi::dock_header_buttons_keep_theme_after_redocking_data()
{
    QTest::addColumn<QString>("background");
    QTest::addColumn<QString>("dockName");
    QTest::newRow("dark-constants") << QStringLiteral("#402034") << QStringLiteral("ConstantsDock");
    QTest::newRow("light-constants") << QStringLiteral("#f4e8ee") << QStringLiteral("ConstantsDock");
    QTest::newRow("dark-functions") << QStringLiteral("#402034") << QStringLiteral("FunctionsDock");
    QTest::newRow("light-functions") << QStringLiteral("#f4e8ee") << QStringLiteral("FunctionsDock");
}

void TestDockAppearanceUi::dock_header_buttons_keep_theme_after_redocking()
{
    QFETCH(QString, background);
    QFETCH(QString, dockName);
    MainWindowStateGuard guard;
    guard.settings->colorScheme = QStringLiteral("Custom");
    guard.settings->customColorSchemeJson = themeJsonString(
        QJsonObject{{QStringLiteral("background"), background}});
    guard.settings->windowState.clear();
    guard.settings->constantsDockVisible = true;
    guard.settings->functionsDockVisible = true;

    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QDockWidget* dock = window.findChild<QDockWidget*>(dockName);
    QVERIFY(dock != nullptr);
    dock->setFloating(false);
    dock->show();
    dock->raise();
    QCoreApplication::processEvents();

    const QStringList buttonNames {QStringLiteral("qt_dockwidget_closebutton"),
                                   QStringLiteral("qt_dockwidget_floatbutton")};
    QList<QAbstractButton*> buttons;
    QList<QImage> expectedIcons;
    for (const QString& name : buttonNames) {
        QAbstractButton* button = dock->findChild<QAbstractButton*>(name);
        QVERIFY(button != nullptr);
        button->setAttribute(Qt::WA_UnderMouse, false);
        QEvent leaveEvent(QEvent::Leave);
        QCoreApplication::sendEvent(button, &leaveEvent);
        const QImage icon = button->icon().pixmap(button->iconSize()).toImage();
        QVERIFY(!icon.isNull());
        QVERIFY(colorsAreClose(icon.pixelColor(2, 9),
                               button->property("speedcrunchDockHeaderButtonFill").value<QColor>()));
        buttons.append(button);
        expectedIcons.append(icon);
    }

    for (int cycle = 0; cycle < 2; ++cycle) {
        dock->setFloating(true);
        QVERIFY(dock->isFloating());
        dock->setFloating(false);
        QVERIFY(!dock->isFloating());
        // Check before a paint, queued theme update, or hover can repair an icon.
        for (bool processEvents : {false, true}) {
            if (processEvents)
                QCoreApplication::processEvents();
            for (int i = 0; i < buttons.size(); ++i) {
                QVERIFY(!buttons.at(i)->underMouse());
                const QImage icon = buttons.at(i)->icon().pixmap(buttons.at(i)->iconSize()).toImage();
                QVERIFY2(icon == expectedIcons.at(i), qPrintable(buttonNames.at(i)));
            }
        }
    }
}

int main(int argc, char** argv)
{
    return UiTestFixture::run<TestDockAppearanceUi>(argc, argv);
}

#include "testdockappearanceui.moc"
