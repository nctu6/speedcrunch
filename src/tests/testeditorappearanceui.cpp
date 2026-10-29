// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mainwindowtesthelpers.h"
#include "editorgeometrytesthelpers.h"
#include "core/startupdiagnostics.h"
#include "gui/startupfontpreparation.h"

using namespace MainWindowTestHelpers;

class TestEditorAppearanceUi : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init() { UiTestFixture::resetSettings(); }
    void startup_font_preparation_finishes_before_teardown_data();
    void startup_font_preparation_finishes_before_teardown();
    void first_result_display_records_constructor_stages();
    void repeated_highlighting_preserves_styles_without_repolishing_data();
    void repeated_highlighting_preserves_styles_without_repolishing();
    void editor_display_style_defaults_to_modern_and_persists();
    void editor_display_style_switches_all_windows_and_preserves_work();
    void editor_display_style_preserves_result_selection_and_copy();
    void classic_editor_display_renders_and_tracks_active_pane_data();
    void classic_editor_display_renders_and_tracks_active_pane();
    void classic_editor_display_restores_split_panes();
    void classic_focus_line_follows_open_windows_and_sessions_data();
    void classic_focus_line_follows_open_windows_and_sessions();
    void classic_vertical_pane_dividers_contrast_with_input_data();
    void classic_vertical_pane_dividers_contrast_with_input();
    void classic_separator_thickness_preserves_input_geometry_data();
    void classic_separator_thickness_preserves_input_geometry();
    void typing_long_expressions_keeps_editor_height_stable_data();
    void typing_long_expressions_keeps_editor_height_stable();
    void editor_popups_follow_style_padding_data();
    void editor_popups_follow_style_padding();
    void live_result_follows_editor_height_on_history_recall_data();
    void live_result_follows_editor_height_on_history_recall();
};

void TestEditorAppearanceUi::initTestCase()
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

void TestEditorAppearanceUi::startup_font_preparation_finishes_before_teardown_data()
{
    QTest::addColumn<bool>("explicitWait");
    QTest::newRow("scope-exit") << false;
    QTest::newRow("repeated-wait") << true;
}

void TestEditorAppearanceUi::startup_font_preparation_finishes_before_teardown()
{
    QFETCH(bool, explicitWait);
    StartupDiagnostics::begin();
    const auto finishRecording = qScopeGuard([]() {
        StartupDiagnostics::firstPaintFinished();
        StartupDiagnostics::finishWhenIdle();
    });
    {
        StartupFontPreparation preparation;
        if (explicitWait) {
            preparation.wait();
            preparation.wait();
        }
    }
    const auto snapshot = StartupDiagnostics::snapshot();
    int workerCalls = 0;
    int waitCalls = 0;
    int asyncCalls = 0;
    for (const auto& phase : snapshot.phases) {
        if (phase.name == QStringLiteral("Prepare font database")) {
            workerCalls += phase.calls;
            QVERIFY(phase.worker);
        } else if (phase.name == QStringLiteral("Wait for font preparation")) {
            waitCalls += phase.calls;
            QVERIFY(!phase.worker);
        } else if (phase.name == QStringLiteral("Prepare font database (async total)")) {
            asyncCalls += phase.calls;
        }
        QCOMPARE(phase.running, 0);
    }
    const int expected = QApplication::platformName() == QLatin1String("cocoa") ? 1 : 0;
    QCOMPARE(workerCalls, expected);
    QCOMPARE(waitCalls, expected);
    QCOMPARE(asyncCalls, expected);
    StartupDiagnostics::firstPaintFinished();
    StartupDiagnostics::finishWhenIdle();
    QVERIFY(StartupDiagnostics::snapshot().complete);
}

void TestEditorAppearanceUi::first_result_display_records_constructor_stages()
{
    StartupDiagnostics::begin();
    const auto finishRecording = qScopeGuard([]() {
        StartupDiagnostics::firstPaintFinished();
        StartupDiagnostics::finishWhenIdle();
    });
    MainWindow window(false);
    const auto snapshot = StartupDiagnostics::snapshot();
    const auto findPhase = [&](const QString& name) -> const StartupDiagnostics::Phase* {
        for (const auto& phase : snapshot.phases) {
            if (phase.name == name)
                return &phase;
        }
        return nullptr;
    };
    const auto* display = findPhase(QStringLiteral("Construct initial result display"));
    const auto* controls = findPhase(QStringLiteral("Initialize result display controls"));
    const auto* editor = findPhase(QStringLiteral("Construct initial expression editor"));
    const auto* highlighter = findPhase(QStringLiteral("Initialize syntax highlighter"));
    const auto* pane = findPhase(QStringLiteral("Create and bind initial session pane"));
    QVERIFY(display);
    QVERIFY(controls);
    QVERIFY(editor);
    QVERIFY(highlighter);
    QVERIFY(pane);
    QCOMPARE(display->calls, 1);
    QCOMPARE(controls->calls, 1);
    QCOMPARE(editor->calls, 1);
    QCOMPARE(highlighter->calls, 2);
    QCOMPARE(pane->calls, 1);
    QVERIFY(display->firstStartNs <= highlighter->firstStartNs);
    QVERIFY(highlighter->firstStartNs <= controls->firstStartNs);
    QVERIFY(controls->firstStartNs + controls->totalNs
            <= display->firstStartNs + display->totalNs);
    QVERIFY(display->firstStartNs + display->totalNs <= editor->firstStartNs);
    QVERIFY(editor->firstStartNs + editor->totalNs <= pane->firstStartNs);
}

void TestEditorAppearanceUi::repeated_highlighting_preserves_styles_without_repolishing_data()
{
    QTest::addColumn<bool>("classic");
    QTest::newRow("modern") << false;
    QTest::newRow("classic") << true;
}

void TestEditorAppearanceUi::repeated_highlighting_preserves_styles_without_repolishing()
{
    QFETCH(bool, classic);
    MainWindowStateGuard guard;
    guard.settings->editorDisplayStyle = classic
        ? Settings::EditorDisplayClassic : Settings::EditorDisplayModern;
    MainWindow window(false);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto* display = window.findChild<ResultDisplay*>();
    Editor* editor = editorForDisplay(display);
    editor->setText(QStringLiteral("12+34"));
    editor->selectAll();
    display->rehighlight();
    editor->rehighlight();
    class StyleChangeCounter : public QObject {
    public:
        int changes = 0;
        bool eventFilter(QObject*, QEvent* event) override
        {
            if (event->type() == QEvent::StyleChange)
                ++changes;
            return false;
        }
    } counter;
    const QList<QWidget*> widgets {display, display->viewport(), editor, editor->viewport()};
    QStringList styles;
    QList<QPalette> palettes;
    for (QWidget* widget : widgets) {
        widget->installEventFilter(&counter);
        styles.append(widget->styleSheet());
        palettes.append(widget->palette());
    }
    for (int i = 0; i < 3; ++i) {
        display->rehighlight();
        editor->rehighlight();
    }
    QCOMPARE(counter.changes, 0);
    for (int i = 0; i < widgets.size(); ++i) {
        QCOMPARE(widgets.at(i)->styleSheet(), styles.at(i));
        QCOMPARE(widgets.at(i)->palette(), palettes.at(i));
    }
    QCOMPARE(editor->textCursor().selectedText(), QStringLiteral("12+34"));

    // Changed surface colors must still reach the existing controls.
    const QColor surface(QStringLiteral("#123456"));
    display->setThemeSurfaceColor(surface);
    display->rehighlight();
    editor->setThemeSurfaceColor(surface, surface);
    editor->rehighlight();
    QVERIFY(counter.changes > 0);
    QCOMPARE(display->viewport()->palette().color(QPalette::Base), surface);
    QCOMPARE(editor->viewport()->palette().color(QPalette::Base), surface);
    QCOMPARE(editor->textCursor().selectedText(), QStringLiteral("12+34"));
}

void TestEditorAppearanceUi::editor_display_style_defaults_to_modern_and_persists()
{
    Settings* settings = Settings::instance();
    QCOMPARE(settings->editorDisplayStyle, Settings::EditorDisplayModern);
    for (const auto style : {Settings::EditorDisplayClassic, Settings::EditorDisplayModern}) {
        settings->editorDisplayStyle = style;
        settings->save();
        settings->editorDisplayStyle = style == Settings::EditorDisplayModern
            ? Settings::EditorDisplayClassic : Settings::EditorDisplayModern;
        settings->load();
        QCOMPARE(settings->editorDisplayStyle, style);
    }
    QSettings persisted(Settings::getConfigPath() + QStringLiteral("/SpeedCrunch.ini"),
                        QSettings::IniFormat);
    const QString key = QStringLiteral("SpeedCrunch/Display/EditorDisplayStyle");
    persisted.remove(key);
    persisted.sync();
    settings->load();
    QCOMPARE(settings->editorDisplayStyle, Settings::EditorDisplayModern);
    persisted.setValue(key, 99);
    persisted.sync();
    settings->load();
    QCOMPARE(settings->editorDisplayStyle, Settings::EditorDisplayModern);
    settings->editorDisplayStyle = Settings::EditorDisplayClassic;
    settings->save();
    QVERIFY(settings->resetToDefaults());
    QCOMPARE(settings->editorDisplayStyle, Settings::EditorDisplayModern);
}

void TestEditorAppearanceUi::editor_display_style_switches_all_windows_and_preserves_work()
{
    Settings* settings = Settings::instance();
    settings->autoCalc = false;
    MainWindow first(false);
    MainWindow second(false);
    for (MainWindow* window : {&first, &second}) {
        QVERIFY(QMetaObject::invokeMethod(window, "splitActivePaneRight", Qt::DirectConnection));
        QVERIFY(QMetaObject::invokeMethod(window, "showNewSessionDialog", Qt::DirectConnection));
        window->resize(900, 600);
        window->show();
        QVERIFY(QTest::qWaitForWindowExposed(window));
    }
    const auto editors = first.findChildren<Editor*>() + second.findChildren<Editor*>();
    const auto displays = first.findChildren<ResultDisplay*>() + second.findChildren<ResultDisplay*>();
    QCOMPARE(editors.size(), 4);
    QCOMPARE(displays.size(), 4);
    QMap<Editor*, int> modernHeights;
    QMap<Editor*, int> modernInsets;
    QMap<Editor*, QColor> modernColors;
    QMap<ResultDisplay*, QString> results;
    QMap<ResultDisplay*, int> modernGutters;
    for (Editor* editor : editors) {
        editor->setText(QStringLiteral("1234+56"));
        QTextCursor cursor = editor->textCursor();
        cursor.setPosition(2);
        cursor.setPosition(5, QTextCursor::KeepAnchor);
        editor->setTextCursor(cursor);
        modernHeights.insert(editor, editor->height());
        modernInsets.insert(editor, editor->viewport()->x());
        modernColors.insert(editor, editor->palette().color(QPalette::Base));
    }
    for (ResultDisplay* display : displays) {
        Session* session = const_cast<Session*>(display->session());
        QVERIFY(session);
        session->addHistoryEntry(HistoryEntry(QStringLiteral("6*7"), Quantity(42)));
        display->refresh();
        results.insert(display, display->toPlainText());
        modernGutters.insert(display, display->viewport()->x());
    }
    QMenu* menu = first.findChild<QMenu*>(QStringLiteral("EditorDisplayStyleMenu"));
    QVERIFY(menu);
    QVERIFY(menu->actions().size() >= 2);
    QVERIFY(menu->actions().at(1)->isChecked());
    for (const bool classic : {true, false, true}) {
        menu->actions().at(classic ? 0 : 1)->trigger();
        QCOMPARE(settings->editorDisplayStyle,
                 classic ? Settings::EditorDisplayClassic : Settings::EditorDisplayModern);
        QCoreApplication::processEvents();
        for (MainWindow* window : {&first, &second}) {
            QMenu* windowMenu = window->findChild<QMenu*>(QStringLiteral("EditorDisplayStyleMenu"));
            QVERIFY(windowMenu);
            QCOMPARE(windowMenu->actions().at(0)->isChecked(), classic);
            QCOMPARE(windowMenu->actions().at(1)->isChecked(), !classic);
        }
        for (Editor* editor : editors) {
            QCOMPARE(editor->text(), QStringLiteral("1234+56"));
            QCOMPARE(editor->textCursor().anchor(), 2);
            QCOMPARE(editor->textCursor().position(), 5);
            if (classic) {
                QVERIFY(editor->height() < modernHeights.value(editor));
                QVERIFY(editor->viewport()->x() < modernInsets.value(editor));
            } else {
                QCOMPARE(editor->height(), modernHeights.value(editor));
                QCOMPARE(editor->viewport()->x(), modernInsets.value(editor));
            }
        }
        for (ResultDisplay* display : displays) {
            QCOMPARE(display->toPlainText(), results.value(display));
            QCOMPARE(display->session()->historySize(), 1);
            QCOMPARE(display->viewport()->x(), classic ? 4 : modernGutters.value(display));
        }
    }
    // Inactive tabs share their pane's widgets, and new panes inherit Classic.
    for (MainWindow* window : {&first, &second}) {
        for (ResultDisplay* display : window->findChildren<ResultDisplay*>()) {
            QTabBar* tabs = tabBarForDisplay(display);
            QVERIFY(tabs);
            for (int index = 0; index < tabs->count(); ++index) {
                tabs->setCurrentIndex(index);
                QCOMPARE(display->viewport()->x(), 4);
                Editor* editor = editorForDisplay(display);
                const QColor expected = modernColors.value(editor);
                QCOMPARE(editor->palette().color(QPalette::Base).rgba(), expected.rgba());
            }
        }
        QVERIFY(QMetaObject::invokeMethod(window, "splitActivePaneDown", Qt::DirectConnection));
        for (ResultDisplay* display : window->findChildren<ResultDisplay*>())
            QCOMPARE(display->viewport()->x(), 4);
    }
    MainWindow third(false);
    QCOMPARE(third.findChild<ResultDisplay*>()->viewport()->x(), 4);
    QVERIFY(third.findChild<QMenu*>(QStringLiteral("EditorDisplayStyleMenu"))->actions().at(0)->isChecked());
    QTimer::singleShot(0, &first, []() {
        auto* dialog = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        QVERIFY(dialog);
        dialog->button(QMessageBox::Yes)->click();
    });
    first.findChild<QAction*>(QStringLiteral("ResetAllSettingsAction"))->trigger();
    QCOMPARE(settings->editorDisplayStyle, Settings::EditorDisplayModern);
    for (MainWindow* window : {&first, &second, &third}) {
        QVERIFY(window->findChild<QMenu*>(QStringLiteral("EditorDisplayStyleMenu"))->actions().at(1)->isChecked());
        for (ResultDisplay* display : window->findChildren<ResultDisplay*>())
            QVERIFY(display->viewport()->x() > 0);
    }
}

void TestEditorAppearanceUi::editor_display_style_preserves_result_selection_and_copy()
{
    Settings::instance()->autoCalc = false;
    MainWindow window(false);
    ResultDisplay* display = window.findChild<ResultDisplay*>();
    Editor* editor = window.findChild<Editor*>();
    QVERIFY(display);
    QVERIFY(editor);
    editor->setText(QStringLiteral("123+456"));
    Session* session = const_cast<Session*>(display->session());
    session->addHistoryEntry(HistoryEntry(QStringLiteral("6*7"), Quantity(42)));
    display->refresh();
    QTextCursor selection = display->textCursor();
    selection.select(QTextCursor::Document);
    display->setTextCursor(selection);
    const QString selectedText = selection.selectedText();
    QVERIFY(!selectedText.isEmpty());
    QSignalSpy edits(editor, &Editor::textChanged);
    QMenu* menu = window.findChild<QMenu*>(QStringLiteral("EditorDisplayStyleMenu"));
    QVERIFY(menu);
    QAction* classic = directMenuActionWithText(menu, QStringLiteral("&Classic"));
    QVERIFY(classic);
    QActionGroup* styles = classic->actionGroup();
    QVERIFY(styles);
    for (QAction* action : styles->actions()) {
        action->trigger();
        QCOMPARE(display->textCursor().selectedText(), selectedText);
        QCOMPARE(editor->text(), QStringLiteral("123+456"));
        QCOMPARE(edits.count(), 0);
        QApplication::clipboard()->clear();
        QVERIFY(QMetaObject::invokeMethod(&window, "copy", Qt::DirectConnection));
        QCOMPARE(QApplication::clipboard()->text(), display->toPlainText());
    }
}

void TestEditorAppearanceUi::classic_editor_display_renders_and_tracks_active_pane_data()
{
    QTest::addColumn<QString>("background");
    QTest::newRow("dark") << QStringLiteral("#310b28");
    QTest::newRow("light") << QStringLiteral("#eeeeee");
}

void TestEditorAppearanceUi::classic_editor_display_renders_and_tracks_active_pane()
{
    QFETCH(QString, background);
    MainWindowStateGuard guard;
    guard.settings->editorDisplayStyle = Settings::EditorDisplayClassic;
    guard.settings->colorScheme = QStringLiteral("Custom");
    guard.settings->customColorSchemeJson = themeJsonString(
        QJsonObject{{QStringLiteral("background"), background}});
    guard.settings->constantsDockVisible = true;
    guard.settings->cursorBlinking = false;
    MainWindow window(false);
    window.resize(900, 600);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(QMetaObject::invokeMethod(&window, "splitActivePaneRight", Qt::DirectConnection));
    QCoreApplication::processEvents();
    const auto displays = window.findChildren<ResultDisplay*>();
    QCOMPARE(displays.size(), 2);
    QMenu* styles = window.findChild<QMenu*>(QStringLiteral("EditorDisplayStyleMenu"));
    QVERIFY(styles);
    styles->actions().at(1)->trigger();
    QCoreApplication::processEvents();
    const QColor inactiveColor = editorForDisplay(displays.first())->palette().color(QPalette::Base);
    styles->actions().at(0)->trigger();
    QCoreApplication::processEvents();
    const QColor primary = generatePrimaryFromBackground(QColor(background));
    const auto separatorColor = [](Editor* editor) {
        const QImage image = editor->grab().toImage();
        return image.pixelColor(image.width() / 2, 0).rgba();
    };
    QMap<Editor*, QRect> geometry;
    for (ResultDisplay* display : displays) {
        Editor* editor = editorForDisplay(display);
        QVERIFY(editor);
        QCOMPARE(editor->geometry().left(), display->geometry().left());
        QCOMPARE(editor->width(), display->width());
        QCOMPARE(editor->geometry().top(), display->geometry().bottom() + 1);
        const QColor expected = inactiveColor;
        QCOMPARE(editor->palette().color(QPalette::Base).rgba(), expected.rgba());
        const int editorTextX = editor->viewport()->x() + editor->cursorRect().x();
        const int displayTextX = display->viewport()->x() + display->cursorRect().x();
        QCOMPARE(editorTextX, displayTextX);
        QCOMPARE(editorTextX, UiConfig::ClassicTextInset);
        QVERIFY(editor->height() - editor->viewport()->geometry().bottom() - 1
                >= UiConfig::ClassicEditorVerticalPadding);
        geometry.insert(editor, editor->geometry());
        const QImage image = editor->grab().toImage();
        QCOMPARE(image.pixelColor(0, image.height() / 2).rgba(),
                 expected.rgba());
    }
    for (ResultDisplay* activeDisplay : displays) {
        Editor* activeEditor = editorForDisplay(activeDisplay);
        QTest::mouseClick(activeEditor->viewport(), Qt::LeftButton);
        QTRY_COMPARE(separatorColor(activeEditor), primary.rgba());
        for (ResultDisplay* display : displays) {
            Editor* editor = editorForDisplay(display);
            QCOMPARE(editor->geometry(), geometry.value(editor));
            const QColor expected = inactiveColor;
            QCOMPARE(editor->palette().color(QPalette::Base).rgba(), expected.rgba());
            QCOMPARE(editor->viewport()->palette().color(QPalette::Base).rgba(), expected.rgba());
            const QImage image = editor->grab().toImage();
            for (const QPoint point : {QPoint(0, image.height() / 2),
                                       QPoint(image.width() / 2, image.height() / 2),
                                       QPoint(image.width() - 1, image.height() - 1)}) {
                QCOMPARE(image.pixelColor(point).rgba(), expected.rgba());
            }
            if (display != activeDisplay)
                QTRY_COMPARE(separatorColor(editor), inactiveColor.rgba());
        }
        // Selecting history keeps the pane's input destination indicated.
        QTest::mouseClick(activeDisplay->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(20, 20));
        QTRY_COMPARE(separatorColor(activeEditor), primary.rgba());
    }
    QDockWidget* constants = window.findChild<QDockWidget*>(QStringLiteral("ConstantsDock"));
    QVERIFY(constants);
    auto* search = constants->findChild<QLineEdit*>();
    QVERIFY(search);
    search->setFocus();
    QTRY_VERIFY(search->hasFocus());
    for (ResultDisplay* display : displays) {
        QTRY_COMPARE(separatorColor(editorForDisplay(display)), inactiveColor.rgba());
        QCOMPARE(editorForDisplay(display)->palette().color(QPalette::Base).rgba(), inactiveColor.rgba());
    }

    // The inactive fill follows the Modern input color after a theme change.
    guard.settings->customColorSchemeJson = themeJsonString(
        QJsonObject{{QStringLiteral("background"), QStringLiteral("#213c31")}});
    QVERIFY(QMetaObject::invokeMethod(&window, "colorSchemeChanged", Qt::DirectConnection));
    styles->actions().at(1)->trigger();
    const QColor updatedInactiveColor = editorForDisplay(displays.first())->palette().color(QPalette::Base);
    styles->actions().at(0)->trigger();
    for (ResultDisplay* display : displays)
        QCOMPARE(editorForDisplay(display)->palette().color(QPalette::Base).rgba(),
                 updatedInactiveColor.rgba());
}

void TestEditorAppearanceUi::classic_editor_display_restores_split_panes()
{
    MainWindowStateGuard guard;
    guard.settings->editorDisplayStyle = Settings::EditorDisplayClassic;
    {
        MainWindow original(false);
        QVERIFY(QMetaObject::invokeMethod(&original, "splitActivePaneRight", Qt::DirectConnection));
        QVERIFY(QMetaObject::invokeMethod(&original, "showNewSessionDialog", Qt::DirectConnection));
        for (Editor* editor : original.findChildren<Editor*>())
            editor->setText(QStringLiteral("123+456"));
        original.persistSessionAndSettingsForShutdown();
    }
    QVERIFY(!guard.settings->sessionLayoutJson.isEmpty());
    MainWindow restored;
    restored.show();
    QCoreApplication::processEvents();
    const auto displays = restored.findChildren<ResultDisplay*>();
    QCOMPARE(displays.size(), 2);
    QMenu* menu = restored.findChild<QMenu*>(QStringLiteral("EditorDisplayStyleMenu"));
    QVERIFY(menu);
    QVERIFY(menu->actions().at(0)->isChecked());
    menu->actions().at(1)->trigger();
    const QColor inactiveColor = editorForDisplay(displays.first())->palette().color(QPalette::Base);
    menu->actions().at(0)->trigger();
    for (ResultDisplay* display : displays) {
        QCOMPARE(display->viewport()->x(), 4);
        Editor* editor = editorForDisplay(display);
        QVERIFY(editor);
        QCOMPARE(editor->text(), QStringLiteral("123+456"));
        const QColor expected = inactiveColor;
        QCOMPARE(editor->palette().color(QPalette::Base).rgba(), expected.rgba());
    }
    menu->actions().at(1)->trigger();
    for (ResultDisplay* display : displays)
        QVERIFY(display->viewport()->x() > 0);
}

void TestEditorAppearanceUi::classic_focus_line_follows_open_windows_and_sessions_data()
{
    QTest::addColumn<QString>("background");
    QTest::newRow("dark") << QStringLiteral("#310b28");
    QTest::newRow("light") << QStringLiteral("#eeeeee");
}

void TestEditorAppearanceUi::classic_focus_line_follows_open_windows_and_sessions()
{
    QFETCH(QString, background);
    MainWindowStateGuard guard;
    guard.settings->editorDisplayStyle = Settings::EditorDisplayClassic;
    guard.settings->colorScheme = QStringLiteral("Custom");
    guard.settings->customColorSchemeJson = themeJsonString(
        QJsonObject{{QStringLiteral("background"), background}});
    guard.settings->autoCalc = false;
    guard.settings->cursorBlinking = false;
    MainWindow window(false);
    window.resize(900, 600);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    Editor* editor = window.findChild<Editor*>();
    QVERIFY(editor);
    editor->setText(QStringLiteral("1234+56"));
    const QString draft = editor->text();
    QTest::mouseClick(editor->viewport(), Qt::LeftButton);
    QCoreApplication::processEvents();
    const int height = editor->height();
    const QRect viewport = editor->viewport()->geometry();
    const QColor fill = editor->palette().color(QPalette::Base);
    const QColor primary = generatePrimaryFromBackground(QColor(background));
    const auto topColor = [](Editor* input) {
        const QImage image = input->grab().toImage();
        return image.pixelColor(image.width() / 2, 0).rgba();
    };
    const auto checkActiveLine = [&](bool visible) {
        window.activateWindow();
        QCoreApplication::processEvents();
        QTest::mouseClick(editor->viewport(), Qt::LeftButton);
        QCoreApplication::processEvents();
        QTRY_VERIFY(editor->hasFocus());
        QTRY_COMPARE(topColor(editor), visible ? primary.rgba() : fill.rgba());
        QVERIFY(editor->cursorWidth() > 0);
        QCOMPARE(editor->height(), height);
        QCOMPARE(editor->viewport()->geometry().top(), viewport.top());
        QCOMPARE(editor->viewport()->height(), viewport.height());
        QCOMPARE(editor->palette().color(QPalette::Base).rgba(), fill.rgba());
    };
    checkActiveLine(false);
    QCOMPARE(editor->text(), draft);

    // Style changes keep the Modern outline and suppress the single Classic line.
    QMenu* styles = window.findChild<QMenu*>(QStringLiteral("EditorDisplayStyleMenu"));
    QVERIFY(styles);
    styles->actions().at(1)->trigger();
    QCoreApplication::processEvents();
    const QPixmap modern = editor->grab();
    QCOMPARE(modern.toImage().pixelColor(modern.width() / 2,
        qRound(UiConfig::ModernEditorTopMargin * modern.devicePixelRatio())).rgba(), primary.rgba());
    styles->actions().at(0)->trigger();
    checkActiveLine(false);

    QVERIFY(QMetaObject::invokeMethod(&window, "showNewSessionDialog", Qt::DirectConnection));
    checkActiveLine(false);
    QVERIFY(QMetaObject::invokeMethod(&window, "showNewSessionDialog", Qt::DirectConnection));
    checkActiveLine(false);
    QTabBar* tabs = tabBarForDisplay(window.findChild<ResultDisplay*>());
    QVERIFY(tabs);
    QCOMPARE(tabs->count(), 3);
    for (int index = 0; index < tabs->count(); ++index) {
        tabs->setCurrentIndex(index);
        checkActiveLine(false);
    }
    QVERIFY(QMetaObject::invokeMethod(&window, "closeCurrentSession", Qt::DirectConnection));
    checkActiveLine(false);
    tabs->setCurrentIndex(1);
    QVERIFY(QMetaObject::invokeMethod(&window, "closeCurrentSession", Qt::DirectConnection));
    checkActiveLine(false);
    QCOMPARE(editor->text(), draft);

    QVERIFY(QMetaObject::invokeMethod(&window, "showNewPrivateSession", Qt::DirectConnection));
    checkActiveLine(false);
    QVERIFY(QMetaObject::invokeMethod(&window, "closeCurrentSession", Qt::DirectConnection));
    checkActiveLine(false);

    QVERIFY(QMetaObject::invokeMethod(&window, "splitActivePaneRight", Qt::DirectConnection));
    checkActiveLine(true);
    QPointer<Editor> otherEditor;
    for (Editor* input : window.findChildren<Editor*>()) {
        if (input != editor)
            otherEditor = input;
    }
    QVERIFY(otherEditor);
    QTest::mouseClick(otherEditor->viewport(), Qt::LeftButton);
    QVERIFY(QMetaObject::invokeMethod(&window, "closeCurrentPane", Qt::DirectConnection));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    checkActiveLine(false);

    // Multiple tabs stay unmarked after the second window closes as well.
    QVERIFY(QMetaObject::invokeMethod(&window, "showNewSessionDialog", Qt::DirectConnection));
    checkActiveLine(false);
    {
        MainWindow second(false);
        second.show();
        QVERIFY(QTest::qWaitForWindowExposed(&second));
        checkActiveLine(true);
        second.close();
        checkActiveLine(false);
    }
    checkActiveLine(false);
    {
        // Destruction also updates the remaining window without a close event.
        MainWindow second(false);
        checkActiveLine(true);
    }
    checkActiveLine(false);
    QVERIFY(QMetaObject::invokeMethod(&window, "closeCurrentSession", Qt::DirectConnection));
    checkActiveLine(false);
    QCOMPARE(editor->text(), draft);
}

void TestEditorAppearanceUi::classic_vertical_pane_dividers_contrast_with_input_data()
{
    QTest::addColumn<QString>("background");
    QTest::addColumn<bool>("keypadVisible");
    QTest::newRow("dark") << QStringLiteral("#310b28") << false;
    QTest::newRow("light") << QStringLiteral("#eeeeee") << false;
    QTest::newRow("dark-keypad") << QStringLiteral("#310b28") << true;
    QTest::newRow("light-keypad") << QStringLiteral("#eeeeee") << true;
}

void TestEditorAppearanceUi::classic_vertical_pane_dividers_contrast_with_input()
{
    QFETCH(QString, background);
    QFETCH(bool, keypadVisible);
    MainWindowStateGuard guard;
    guard.settings->editorDisplayStyle = Settings::EditorDisplayClassic;
    guard.settings->colorScheme = QStringLiteral("Custom");
    guard.settings->customColorSchemeJson = themeJsonString(
        QJsonObject{{QStringLiteral("background"), background}});
    guard.settings->keypadMode = keypadVisible
        ? Settings::KeypadModeBasicWide : Settings::KeypadModeDisabled;
    MainWindow window(false);
    window.resize(1000, 900);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(QMetaObject::invokeMethod(&window, "splitActivePaneRight", Qt::DirectConnection));
    QVERIFY(QMetaObject::invokeMethod(&window, "splitActivePaneDown", Qt::DirectConnection));
    QCoreApplication::processEvents();
    QSplitter* root = window.findChild<QSplitter*>(QStringLiteral("MainSplitContainer"));
    QVERIFY(root);
    QCOMPARE(root->orientation(), Qt::Horizontal);
    QSplitterHandle* divider = root->handle(1);
    QVERIFY(divider);
    auto* styles = window.findChild<QMenu*>(QStringLiteral("EditorDisplayStyleMenu"));
    QVERIFY(styles);
    const auto displays = window.findChildren<ResultDisplay*>();
    QCOMPARE(displays.size(), 3);
    QTest::mouseMove(displays.first()->viewport(), QPoint(30, 30));
    for (const bool classic : {true, false, true}) {
        styles->actions().at(classic ? 0 : 1)->trigger();
        QCoreApplication::processEvents();
        QCOMPARE(root->handleWidth(), UiConfig::SessionPaneSplitterWidth);
        const QPixmap pixmap = divider->grab();
        const QImage image = pixmap.toImage();
        const QColor dividerColor = image.pixelColor(image.width() / 2, image.height() / 2);
        for (ResultDisplay* display : displays) {
            Editor* editor = editorForDisplay(display);
            QVERIFY(editor);
            const QColor fill = editor->palette().color(QPalette::Base);
            if (classic)
                QVERIFY(dividerColor.rgba() != fill.rgba());
            else
                QCOMPARE(dividerColor.rgba(), fill.rgba());
            const QPoint position = divider->mapFromGlobal(
                editor->mapToGlobal(QPoint(0, editor->height() / 2)));
            const int row = qRound(position.y() * pixmap.devicePixelRatio());
            QVERIFY(row >= 0 && row < image.height());
            QCOMPARE(image.pixelColor(image.width() / 2, row).rgba(), dividerColor.rgba());
            if (classic) {
                const QImage editorImage = editor->grab().toImage();
                QCOMPARE(editorImage.pixelColor(0, editorImage.height() / 2).rgba(), fill.rgba());
            }
        }
    }
}

void TestEditorAppearanceUi::classic_separator_thickness_preserves_input_geometry_data()
{
    QTest::addColumn<QString>("background");
    QTest::newRow("dark") << QStringLiteral("#310b28");
    QTest::newRow("light") << QStringLiteral("#eeeeee");
}

void TestEditorAppearanceUi::classic_separator_thickness_preserves_input_geometry()
{
    QFETCH(QString, background);
    MainWindowStateGuard guard;
    guard.settings->editorDisplayStyle = Settings::EditorDisplayClassic;
    guard.settings->colorScheme = QStringLiteral("Custom");
    guard.settings->customColorSchemeJson = themeJsonString(
        QJsonObject{{QStringLiteral("background"), background}});
    guard.settings->autoCalc = false;
    MainWindow window(false);
    window.resize(700, 600);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(QMetaObject::invokeMethod(&window, "splitActivePaneRight", Qt::DirectConnection));
    const auto editors = window.findChildren<Editor*>();
    QCOMPARE(editors.size(), 2);
    QMap<Editor*, QRect> geometry;
    QMap<Editor*, QRect> viewportGeometry;
    QMap<Editor*, QRect> cursorGeometry;
    QMap<Editor*, QTextCursor> cursors;
    for (Editor* editor : editors) {
        editor->setText(QStringLiteral("1234567890+").repeated(12));
        QCoreApplication::processEvents();
        QVERIFY(editor->document()->begin().layout()->lineCount() > 1);
        geometry.insert(editor, editor->geometry());
        viewportGeometry.insert(editor, editor->viewport()->geometry());
        cursorGeometry.insert(editor, editor->cursorRect());
        cursors.insert(editor, editor->textCursor());
    }
    const QColor primary = generatePrimaryFromBackground(QColor(background));
    const auto pixelAtRow = [](Editor* editor, int row) {
        const QPixmap pixmap = editor->grab();
        const QImage image = pixmap.toImage();
        return image.pixelColor(image.width() / 2, qRound(row * pixmap.devicePixelRatio())).rgba();
    };
    for (int cycle = 0; cycle < 2; ++cycle) {
        for (Editor* activeEditor : editors) {
            QTest::mouseClick(activeEditor->viewport(), Qt::LeftButton);
            activeEditor->setTextCursor(cursors.value(activeEditor));
            QTRY_VERIFY(activeEditor->hasFocus());
            QCoreApplication::processEvents();
            QTRY_COMPARE(pixelAtRow(activeEditor, 0), primary.rgba());
            for (Editor* editor : editors) {
                const QColor base = editor->palette().color(QPalette::Base);
                if (editor == activeEditor) {
                    QCOMPARE(pixelAtRow(editor, 1), primary.rgba());
                } else {
                    QCOMPARE(pixelAtRow(editor, 0), base.rgba());
                    QCOMPARE(pixelAtRow(editor, 1), base.rgba());
                }
                QCOMPARE(pixelAtRow(editor, 2), base.rgba());
                QCOMPARE(editor->geometry(), geometry.value(editor));
                QCOMPARE(editor->viewport()->geometry(), viewportGeometry.value(editor));
                QCOMPARE(editor->cursorRect().topLeft(), cursorGeometry.value(editor).topLeft());
                QCOMPARE(editor->cursorRect().height(), cursorGeometry.value(editor).height());
            }
        }
    }
}

void TestEditorAppearanceUi::typing_long_expressions_keeps_editor_height_stable_data()
{
    QTest::addColumn<bool>("classic");
    QTest::addColumn<bool>("split");
    QTest::newRow("modern-single-pane") << false << false;
    QTest::newRow("modern-split-panes") << false << true;
    QTest::newRow("classic-single-pane") << true << false;
    QTest::newRow("classic-split-panes") << true << true;
}

void TestEditorAppearanceUi::typing_long_expressions_keeps_editor_height_stable()
{
    QFETCH(bool, classic);
    QFETCH(bool, split);
    MainWindowStateGuard guard;
    guard.settings->editorDisplayStyle = classic
        ? Settings::EditorDisplayClassic : Settings::EditorDisplayModern;
    guard.settings->autoCalc = false;
    MainWindow window(false);
    window.resize(700, 600);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    if (split)
        QVERIFY(QMetaObject::invokeMethod(&window, "splitActivePaneRight", Qt::DirectConnection));
    Editor* editor = window.findChildren<Editor*>().last();
    editor->setAutoCompletionEnabled(false);
    QTest::mouseClick(editor->viewport(), Qt::LeftButton);
    QCoreApplication::processEvents();
    editor->setText(QString());
    QCoreApplication::processEvents();
    const int singleLineHeight = editor->height();
    const int singleLineTextHeight = EditorGeometryTestHelpers::visibleTextHeight(*editor);
    int cappedHeight = -1;

    class ResizeRecorder : public QObject {
    public:
        QList<int> heights;
        QList<int> viewportHeights;
        Editor* editor = nullptr;
        bool eventFilter(QObject* object, QEvent* event) override
        {
            if (event->type() == QEvent::Resize) {
                const int height = static_cast<QResizeEvent*>(event)->size().height();
                (object == editor ? heights : viewportHeights).append(height);
            }
            return false;
        }
    } recorder;
    recorder.editor = editor;
    editor->installEventFilter(&recorder);
    editor->viewport()->installEventFilter(&recorder);
    const QString input = QStringLiteral("123+456+789+").repeated(50);
    for (const QChar ch : input) {
        const int previousHeight = editor->height();
        const int previousViewportHeight = editor->viewport()->height();
        QTest::keyClicks(editor, QString(ch));
        QCoreApplication::processEvents();
        QVERIFY2(editor->height() >= previousHeight,
            qPrintable(QStringLiteral("Editor shrinks from %1 to %2 with %3 characters and %4 lines")
                .arg(previousHeight).arg(editor->height()).arg(editor->text().size())
                .arg(editor->document()->begin().layout()->lineCount())));
        for (const int height : recorder.heights)
            QVERIFY2(height >= previousHeight, qPrintable(QStringLiteral("Editor shrinks from %1 to %2")
                .arg(previousHeight).arg(height)));
        for (const int height : recorder.viewportHeights)
            QVERIFY2(height >= previousViewportHeight, qPrintable(QStringLiteral("Viewport shrinks from %1 to %2")
                .arg(previousViewportHeight).arg(height)));
        recorder.heights.clear();
        recorder.viewportHeights.clear();
        const int visibleLines = qBound(1, editor->document()->begin().layout()->lineCount(), 5);
        QCOMPARE(editor->height(), singleLineHeight
            + EditorGeometryTestHelpers::visibleTextHeight(*editor) - singleLineTextHeight);
        if (visibleLines == 5) {
            if (cappedHeight < 0)
                cappedHeight = editor->height();
            QCOMPARE(editor->height(), cappedHeight);
        }
        QVERIFY(editor->cursorRect().top() >= 0);
        QVERIFY(editor->cursorRect().bottom() < editor->viewport()->height());
    }
    QVERIFY(cappedHeight > singleLineHeight);
    QCOMPARE(editor->height(), cappedHeight);
    QVERIFY(editor->document()->begin().layout()->lineCount() > 5);
    editor->selectAll();
    QTest::keyClicks(editor, QStringLiteral("1"));
    QCoreApplication::processEvents();
    QCOMPARE(editor->height(), singleLineHeight);
    editor->undo();
    QCoreApplication::processEvents();
    QCOMPARE(editor->height(), cappedHeight);
    QVERIFY(editor->cursorRect().top() >= 0);
    QVERIFY(editor->cursorRect().bottom() < editor->viewport()->height());
}

void TestEditorAppearanceUi::editor_popups_follow_style_padding_data()
{
    QTest::addColumn<int>("initialStyle");
    QTest::addColumn<QString>("popupKind");
    for (const auto style : {Settings::EditorDisplayClassic, Settings::EditorDisplayModern}) {
        for (const QString& kind : {QStringLiteral("current"), QStringLiteral("selection"),
                                    QStringLiteral("error")}) {
            const QString name = QStringLiteral("%1-%2")
                .arg(style == Settings::EditorDisplayClassic ? QStringLiteral("classic")
                                                             : QStringLiteral("modern"), kind);
            QTest::newRow(qPrintable(name)) << int(style) << kind;
        }
    }
}

void TestEditorAppearanceUi::editor_popups_follow_style_padding()
{
    QFETCH(int, initialStyle);
    QFETCH(QString, popupKind);
    Settings* settings = Settings::instance();
    settings->editorDisplayStyle = static_cast<Settings::EditorDisplayStyle>(initialStyle);
    MainWindow window(false);
    window.resize(900, 600);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(QMetaObject::invokeMethod(&window, "splitActivePaneRight", Qt::DirectConnection));
    QVERIFY(QMetaObject::invokeMethod(&window, "splitActivePaneDown", Qt::DirectConnection));
    QCoreApplication::processEvents();
    const auto displays = window.findChildren<ResultDisplay*>();
    QCOMPARE(displays.size(), 3);
    auto* popup = window.findChild<QLabel*>(QStringLiteral("EditorStatePopup"));
    QVERIFY(popup);
    auto* styles = window.findChild<QMenu*>(QStringLiteral("EditorDisplayStyleMenu"));
    QVERIFY(styles);
    int operand = 24;
    for (ResultDisplay* display : displays) {
        Editor* editor = editorForDisplay(display);
        QVERIFY(editor);
        QTest::mouseClick(editor->viewport(), Qt::LeftButton);
        if (popupKind == QLatin1String("error")) {
            editor->setText(QStringLiteral("1/0"));
            QTest::keyClick(editor, Qt::Key_Return);
            QTRY_VERIFY(!popup->text().isEmpty());
            QVERIFY(!popup->text().contains(QStringLiteral("Current result:")));
        } else {
            editor->setText(QStringLiteral("1+%1").arg(operand++));
            editor->setCursorPosition(editor->text().size());
            editor->refreshAutoCalc();
            if (popupKind == QLatin1String("selection")) {
                QTextCursor selection = editor->textCursor();
                selection.setPosition(0);
                selection.setPosition(1, QTextCursor::KeepAnchor);
                editor->setTextCursor(selection);
                QTRY_VERIFY(popup->text().contains(QStringLiteral("Selection result:")));
                QCOMPARE(popup->text(), QStringLiteral("Selection result:<br/>1"));
            } else {
                QTRY_VERIFY(popup->text().contains(QStringLiteral("Current result:")));
                QCOMPARE(popup->text(), QStringLiteral("Current result:<br/>%1").arg(operand));
            }
        }
        QTRY_VERIFY(popup->isVisible());
        const QString message = popup->text();
        for (const auto style : {static_cast<Settings::EditorDisplayStyle>(initialStyle),
                                 initialStyle == Settings::EditorDisplayClassic
                                     ? Settings::EditorDisplayModern : Settings::EditorDisplayClassic}) {
            styles->actions().at(style == Settings::EditorDisplayClassic ? 0 : 1)->trigger();
            const int inset = style == Settings::EditorDisplayClassic
                ? UiConfig::ClassicTextInset : UiConfig::ResultTooltipStartMargin;
            QTRY_COMPARE(popup->mapToGlobal(QPoint(0, 0)).x(),
                         editor->mapToGlobal(QPoint(inset, 0)).x());
            QTRY_COMPARE(popup->mapToGlobal(QPoint(0, popup->height())).y(),
                         editor->mapToGlobal(QPoint(0, 0)).y());
            QCOMPARE(popup->text(), message);
            QVERIFY(popup->isVisible());
        }
    }
}

void TestEditorAppearanceUi::live_result_follows_editor_height_on_history_recall_data()
{
    QTest::addColumn<bool>("classic");
    QTest::addColumn<bool>("split");
    QTest::newRow("modern-single-pane") << false << false;
    QTest::newRow("modern-split-panes") << false << true;
    QTest::newRow("classic-single-pane") << true << false;
    QTest::newRow("classic-split-panes") << true << true;
}

void TestEditorAppearanceUi::live_result_follows_editor_height_on_history_recall()
{
    QFETCH(bool, classic);
    QFETCH(bool, split);
    MainWindowStateGuard guard;
    guard.settings->editorDisplayStyle = classic
        ? Settings::EditorDisplayClassic : Settings::EditorDisplayModern;
    guard.settings->upDownArrowBehavior = Settings::UpDownArrowBehaviorAlways;
    MainWindow window(false);
    window.resize(700, 600);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    if (split)
        QVERIFY(QMetaObject::invokeMethod(&window, "splitActivePaneRight", Qt::DirectConnection));
    Editor* editor = window.findChildren<Editor*>().last();
    editor->setAutoCompletionEnabled(false);
    editor->setAutoCalcEnabled(false);
    QTest::mouseClick(editor->viewport(), Qt::LeftButton);
    editor->setText(QStringLiteral("1+2+").repeated(300) + QStringLiteral("3"));
    const QString expression = editor->document()->toRawText();
    QTest::keyClick(editor, Qt::Key_Return);
    QTRY_VERIFY(editor->text().isEmpty());
    QCoreApplication::processEvents();
    const int singleLineHeight = editor->height();
    editor->setAutoCalcEnabled(true);

    QTest::keyClick(editor, Qt::Key_Up);
    QCOMPARE(editor->document()->toRawText(), expression);
    auto* popup = window.findChild<QLabel*>(QStringLiteral("EditorStatePopup"));
    QVERIFY(popup);
    QTRY_VERIFY(popup->isVisible());
    QVERIFY(popup->text().contains(QStringLiteral("Current result:")));
    QCoreApplication::processEvents();
    QVERIFY(editor->height() > singleLineHeight);
    QVERIFY(editor->document()->firstBlock().layout()->lineCount() > 5);
    const int inset = classic ? UiConfig::ClassicTextInset : UiConfig::ResultTooltipStartMargin;
    QCOMPARE(popup->mapToGlobal(QPoint(0, popup->height())),
             editor->mapToGlobal(QPoint(inset, 0)));

    QTest::keyClick(editor, Qt::Key_Down);
    QTRY_VERIFY(editor->text().isEmpty());
    QTRY_COMPARE(editor->height(), singleLineHeight);
    QVERIFY(!popup->isVisible());
}

int main(int argc, char** argv)
{
    return UiTestFixture::run<TestEditorAppearanceUi>(argc, argv);
}

#include "testeditorappearanceui.moc"
