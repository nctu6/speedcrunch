// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mainwindowtesthelpers.h"
#include "core/startupdiagnostics.h"
#include "core/units.h"

#include <QElapsedTimer>

#include <functional>

using namespace MainWindowTestHelpers;

namespace {

void chooseTabContextAction(QTabBar* tabs, int index, const QStringList& path,
                            const std::function<void(QAction*)>& beforeTrigger = {})
{
    QTimer::singleShot(0, tabs, [path, beforeTrigger]() {
        QMenu* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
        QVERIFY(menu);
        QPointer<QMenu> root = menu;
        const auto dismissMenu = qScopeGuard([root]() {
            if (root && root->isVisible())
                root->close();
        });
        for (int i = 0; i + 1 < path.size(); ++i) {
            QMenu* submenu = directSubmenuWithTitle(menu, path.at(i));
            QVERIFY(submenu);
            menu->setActiveAction(submenu->menuAction());
            QTest::keyClick(menu, Qt::Key_Right);
            QTRY_VERIFY(submenu->isVisible());
            menu = submenu;
        }
        QAction* action = directMenuActionWithText(menu, path.last());
        QVERIFY(action);
        if (beforeTrigger)
            beforeTrigger(action);
        if (action->isEnabled()) {
            menu->setActiveAction(action);
            QTest::keyClick(menu, Qt::Key_Return);
        }
    });
    const QPoint position = tabs->tabRect(index).center();
    QContextMenuEvent event(QContextMenuEvent::Mouse, position, tabs->mapToGlobal(position));
    QCoreApplication::sendEvent(tabs, &event);
}

} // namespace

class TestSessionsUi : public QObject {
    Q_OBJECT

public slots:
    void captureOpenedUrl(const QUrl& url) { m_capturedUrl = url; }

private slots:
    void initTestCase();
    void init() { UiTestFixture::resetSettings(); }
    void startup_numeric_history_keeps_unit_registry_unloaded();
    void startup_applies_window_flags_before_polishing_panes_data();
    void startup_applies_window_flags_before_polishing_panes();
    void startup_initializes_only_requested_units();
    void startup_keeps_reused_pane_parented_data();
    void startup_keeps_reused_pane_parented();
    void startup_geometry_fallback_data();
    void startup_geometry_fallback();
    void startup_reuses_initial_pane_data();
    void startup_reuses_initial_pane();
    void startup_shows_restored_session_on_first_show_data();
    void startup_shows_restored_session_on_first_show();
    void startup_hide_cancels_pending_show();
    void saved_window_ui_state_overrides_defaults_before_show();
    void visible_window_applies_restored_dock_and_keypad_layout();
    void always_on_top_toggles_preserve_window_geometry();
    void session_tab_context_menu_opens_on_right_click();
    void tab_context_reopen_recovers_only_regular_tabs_data();
    void tab_context_reopen_recovers_only_regular_tabs();
    void tab_context_privacy_targets_clicked_pane_data();
    void tab_context_privacy_targets_clicked_pane();
    void tab_context_split_targets_clicked_pane_data();
    void tab_context_split_targets_clicked_pane();
    void tab_context_clear_preserves_other_sessions_data();
    void tab_context_clear_preserves_other_sessions();
    void persisting_layout_captures_visible_scroll_positions_for_all_panes();
    void switching_session_tabs_preserves_each_editor_text();
    void session_tabs_show_full_name_tooltip_on_hover();
    void session_tab_navigation_shortcuts_switch_tabs();
    void new_tab_menu_action_and_shortcut_create_session_in_active_pane();
    void new_tab_menu_action_targets_focused_window_when_native_menu_uses_last_window_action();
    void new_session_window_menu_action_copies_layout_with_single_fresh_session();
    void session_open_menu_action_uses_open_dialog();
    void session_clone_menu_action_follows_active_pane_data();
    void session_clone_menu_action_follows_active_pane();
    void session_commands_follow_floating_dock_owner_data();
    void session_commands_follow_floating_dock_owner();
    void session_open_sessions_folder_menu_action_opens_session_storage();
    void session_import_dialog_opens_valid_json_as_new_tab();
    void session_import_rejects_invalid_json_without_new_tab();
    void session_export_json_action_opens_dialog();
    void session_export_dialog_prefills_session_name_data();
    void session_export_dialog_prefills_session_name();
    void restore_closed_tab_shortcut_restores_last_closed_session_tab();
    void close_and_reopen_commands_preserve_regular_tabs_data();
    void close_and_reopen_commands_preserve_regular_tabs();
    void view_commands_split_and_close_active_pane_data();
    void view_commands_split_and_close_active_pane();
    void session_tabs_reorder_with_horizontal_drag();

private:
    QUrl m_capturedUrl;
};

void TestSessionsUi::session_commands_follow_floating_dock_owner_data()
{
    QTest::addColumn<QString>("command");
    QTest::addColumn<bool>("shortcut");
    QTest::addColumn<bool>("searchFocus");
    for (bool searchFocus : {false, true}) {
        const QByteArray focus = searchFocus ? "search-" : "list-";
        QTest::addRow("%sclose-shortcut", focus.constData()) << QStringLiteral("close") << true << searchFocus;
        QTest::addRow("%sclose-menu", focus.constData()) << QStringLiteral("close") << false << searchFocus;
        QTest::addRow("%sreopen-shortcut", focus.constData()) << QStringLiteral("reopen") << true << searchFocus;
        QTest::addRow("%sreopen-menu", focus.constData()) << QStringLiteral("reopen") << false << searchFocus;
        QTest::addRow("%sclone-menu", focus.constData()) << QStringLiteral("clone") << false << searchFocus;
    }
}

void TestSessionsUi::session_commands_follow_floating_dock_owner()
{
    QFETCH(QString, command);
    QFETCH(bool, shortcut);
    QFETCH(bool, searchFocus);
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    guard.settings->constantsDockVisible = true;
    guard.settings->autoCalc = false;
    MainWindow owner(false);
    MainWindow other(false);
    owner.show();
    other.show();
    owner.activateWindow();
    QVERIFY(QTest::qWaitForWindowExposed(&owner));
    Editor* editor = owner.findChild<Editor*>();
    QVERIFY(editor);
    editor->setFocus();
    QTRY_VERIFY(editor->hasFocus());
    QVERIFY(QMetaObject::invokeMethod(&owner, "showNewSessionDialog"));
    QVERIFY(QMetaObject::invokeMethod(&owner, "showNewSessionDialog"));
    ResultDisplay* display = owner.findChild<ResultDisplay*>();
    QTabBar* tabs = tabBarForDisplay(display);
    QVERIFY(tabs);
    editor->setText(QStringLiteral("111"));
    const QString closedName = display->session()->name();
    QVERIFY(QMetaObject::invokeMethod(&owner, "closeCurrentSession"));
    QCOMPARE(tabs->count(), 2);
    other.activateWindow();
    Editor* otherEditor = other.findChild<Editor*>();
    QVERIFY(otherEditor);
    otherEditor->setFocus();
    QTRY_VERIFY(otherEditor->hasFocus());
    QVERIFY(QMetaObject::invokeMethod(&other, "showNewPrivateSession"));
    otherEditor->setText(QStringLiteral("private draft"));
    ResultDisplay* otherDisplay = other.findChild<ResultDisplay*>();
    const Session* otherSession = otherDisplay->session();
    const QString otherName = otherSession->name();
    QTabBar* otherTabs = tabBarForDisplay(otherDisplay);
    QVERIFY(otherTabs);
    QCOMPARE(otherTabs->count(), 2);

    QDockWidget* dock = owner.findChild<QDockWidget*>(QStringLiteral("ConstantsDock"));
    QVERIFY(dock);
    dock->setFloating(true);
    dock->show();
    dock->activateWindow();
    QWidget* control = searchFocus
        ? static_cast<QWidget*>(dock->findChild<QLineEdit*>())
        : static_cast<QWidget*>(dock->findChild<QTreeWidget*>());
    QVERIFY(control);
    control->setFocus();
    QTRY_VERIFY(control->hasFocus());
    QMenu* sessionMenu = menuWithTitle(other.menuBar(), QStringLiteral("&Session"));
    QVERIFY(sessionMenu);
    QAction* reopen = directMenuActionWithText(sessionMenu, QStringLiteral("Reopen Clo&sed Tab"));
    QVERIFY(reopen);
    QTRY_VERIFY(reopen->isEnabled());
    QAction* action = command == QLatin1String("reopen") ? reopen
        : directMenuActionWithText(sessionMenu, command == QLatin1String("close")
            ? QStringLiteral("&Close Tab") : QStringLiteral("Clo&ne..."));
    QVERIFY(action);
    if (command == QLatin1String("clone")) {
        QTimer::singleShot(0, &owner, []() {
            auto* dialog = qobject_cast<QInputDialog*>(QApplication::activeModalWidget());
            QVERIFY(dialog);
            dialog->setTextValue(QStringLiteral("Dock clone"));
            dialog->accept();
        });
    }
    if (shortcut) {
        const QKeyCombination key = command == QLatin1String("reopen")
            ? restoreClosedTabShortcut() : QKeySequence(QKeySequence::Close)[0];
        QTest::keyClick(control, key.key(), key.keyboardModifiers());
    } else {
        QMenu menu;
        menu.addAction(action);
        menu.popup(control->mapToGlobal(QPoint(10, 10)));
        menu.setActiveAction(action);
        QTest::keyClick(&menu, Qt::Key_Return);
    }
    QTRY_COMPARE(tabs->count(), command == QLatin1String("close") ? 1 : 3);
    if (command == QLatin1String("reopen")) {
        QCOMPARE(display->session()->name(), closedName);
        QCOMPARE(editor->text(), QStringLiteral("111"));
    } else if (command == QLatin1String("clone")) {
        QCOMPARE(display->session()->name(), QStringLiteral("Dock clone"));
        QVERIFY(!display->session()->isPrivate());
    }
    QCOMPARE(otherTabs->count(), 2);
    QCOMPARE(otherDisplay->session(), otherSession);
    QCOMPARE(otherSession->name(), otherName);
    QVERIFY(otherSession->isPrivate());
    QCOMPARE(otherEditor->text(), QStringLiteral("private draft"));
}

void TestSessionsUi::session_clone_menu_action_follows_active_pane_data()
{
    QTest::addColumn<bool>("privateTab");
    QTest::addColumn<bool>("foreignMenu");
    QTest::addColumn<bool>("accept");
    QTest::newRow("regular") << false << false << true;
    QTest::newRow("private") << true << false << true;
    QTest::newRow("regular-other-windows-menu") << false << true << true;
    QTest::newRow("private-other-windows-menu") << true << true << true;
    QTest::newRow("cancel") << false << false << false;
}

void TestSessionsUi::session_clone_menu_action_follows_active_pane()
{
    QFETCH(bool, privateTab);
    QFETCH(bool, foreignMenu);
    QFETCH(bool, accept);
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    guard.settings->autoCalc = false;
    MainWindow window(false);
    MainWindow other(false);
    window.show();
    other.show();
    window.activateWindow();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    Editor* inactiveEditor = window.findChild<Editor*>();
    QVERIFY(inactiveEditor);
    inactiveEditor->setText(QStringLiteral("456"));
    QVERIFY(QMetaObject::invokeMethod(&window, "splitActivePaneRight"));
    if (privateTab)
        QVERIFY(QMetaObject::invokeMethod(&window, "showNewPrivateSession"));
    const QList<Editor*> editors = window.findChildren<Editor*>();
    QCOMPARE(editors.size(), 2);
    Editor* editor = editors.first() == inactiveEditor ? editors.last() : editors.first();
    QVERIFY(editor);
    ResultDisplay* display = editor->parentWidget()->findChild<ResultDisplay*>();
    QVERIFY(display);
    QTest::mouseClick(editor->viewport(), Qt::LeftButton);
    QTRY_VERIFY(editor->hasFocus());
    QCOMPARE(display->session()->isPrivate(), privateTab);
    editor->setText(QStringLiteral("clonevalue=123"));
    QVERIFY(QMetaObject::invokeMethod(&window, "evaluateEditorExpression"));
    editor->setText(QStringLiteral("clonevalue +"));
    const Session* source = display->session();
    const QString sourceName = source->name();
    QTabBar* tabs = tabBarForDisplay(display);
    QVERIFY(tabs);
    const int initialTabs = tabs->count();
    QMenu* menu = menuWithTitle((foreignMenu ? other : window).menuBar(), QStringLiteral("&Session"));
    QVERIFY(menu);
    QAction* clone = directMenuActionWithText(menu, QStringLiteral("Clo&ne..."));
    QVERIFY(clone);
    bool visitedDialog = false;
    const QString cloneName = QStringLiteral("Menu clone");
    QTimer::singleShot(0, &window, [&]() {
        auto* dialog = qobject_cast<QInputDialog*>(QApplication::activeModalWidget());
        QVERIFY(dialog);
        visitedDialog = true;
        dialog->setTextValue(cloneName);
        if (accept)
            dialog->accept();
        else
            dialog->reject();
    });
    clone->trigger();
    QVERIFY(visitedDialog);
    QCOMPARE(tabs->count(), initialTabs + (accept ? 1 : 0));
    QCOMPARE(display->session()->name(), accept ? cloneName : sourceName);
    QCOMPARE(display->session()->isPrivate(), privateTab);
    QCOMPARE(display->session()->historySize(), 1);
    QVERIFY(display->session()->hasVariable(QStringLiteral("clonevalue")));
    QCOMPARE(editor->text(), QStringLiteral("clonevalue +"));
    QCOMPARE(source->name(), sourceName);
    QCOMPARE(source->historySize(), 1);
    QCOMPARE(source->editorText(), QStringLiteral("clonevalue +"));
    if (accept) {
        QVERIFY(display->session() != source);
        QCOMPARE(QFileInfo::exists(storage.path(cloneName)), !privateTab);
    }
    QCOMPARE(inactiveEditor->text(), QStringLiteral("456"));
    QVERIFY(other.findChild<Editor*>()->text().isEmpty());
}

void TestSessionsUi::initTestCase()
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

void TestSessionsUi::startup_numeric_history_keeps_unit_registry_unloaded()
{
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    const QString name = QStringLiteral("NumericHistory");
    QVERIFY(QDir().mkpath(QFileInfo(storage.path(name)).path()));
    writeFile(storage.path(name), QJsonDocument(sessionJson(name)).toJson());
    guard.settings->autoCalc = false;
    guard.settings->hasNumberFormatStyleSetting = true;
    guard.settings->sessionLayoutJson = QString::fromUtf8(QJsonDocument(QJsonObject {
        { QStringLiteral("scheme"), 1 },
        { QStringLiteral("kind"), QStringLiteral("session-layout") },
        { QStringLiteral("windows"), QJsonArray {startupSessionWindow(name, QStringLiteral("saved"))} }
    }).toJson());

    StartupDiagnostics::begin();
    const auto stopDiagnostics = qScopeGuard([] {
        StartupDiagnostics::firstPaintFinished();
        StartupDiagnostics::finishWhenIdle();
    });
    for (const QString& emptyName : {QString(), QStringLiteral(" \t\n")}) {
        HNumber output(123);
        QVERIFY(!Units::isAffineUnitName(emptyName));
        QVERIFY(!Units::tryConvertAffineToBase(emptyName, HNumber(42), &output));
        QCOMPARE(output, HNumber(123));
        QVERIFY(!Units::tryConvertAffineFromBase(emptyName, HNumber(42), &output));
        QCOMPARE(output, HNumber(123));
    }
    MainWindow window;
    window.showWhenReady();
    QTRY_VERIFY(window.isVisible());
    ResultDisplay* display = window.findChild<ResultDisplay*>();
    QVERIFY(display);
    QCOMPARE(display->session()->name(), name);
    QCOMPARE(display->session()->historySize(), 1);
    QVERIFY(display->toPlainText().contains(QStringLiteral("42")));
    for (const auto& phase : StartupDiagnostics::snapshot().phases)
        QVERIFY(phase.name != QLatin1String("Build built-in unit registry"));

    // The first temperature conversion must still initialize and use the units.
    HNumber temperature;
    QVERIFY(Units::tryConvertAffineToBase(QStringLiteral(" degC "), HNumber(0), &temperature));
    QCOMPARE(temperature, HNumber("273.15"));
    QVERIFY(Units::tryConvertAffineFromBase(QStringLiteral("degF"), temperature, &temperature));
    QCOMPARE(temperature, HNumber(32));
}

void TestSessionsUi::startup_applies_window_flags_before_polishing_panes_data()
{
    QTest::addColumn<bool>("alwaysOnTop");
    QTest::newRow("normal") << false;
    QTest::newRow("always-on-top") << true;
}

void TestSessionsUi::startup_applies_window_flags_before_polishing_panes()
{
    QFETCH(bool, alwaysOnTop);
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland")))
        QSKIP("Wayland does not support the always-on-top window setting", );
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    guard.settings->windowAlwaysOnTop = alwaysOnTop;
    class PanePolishObserver : public QObject {
    public:
        bool observed = false;
        bool flagAtFirstPolish = false;
        bool eventFilter(QObject* object, QEvent* event) override
        {
            if (!observed && event->type() == QEvent::Polish) {
                if (auto* display = qobject_cast<ResultDisplay*>(object)) {
                    observed = true;
                    flagAtFirstPolish = display->window()->windowFlags().testFlag(Qt::WindowStaysOnTopHint);
                }
            }
            return false;
        }
    } observer;
    qApp->installEventFilter(&observer);
    MainWindow window(false);
    QVERIFY(observer.observed);
    QCOMPARE(observer.flagAtFirstPolish, alwaysOnTop);
    QCOMPARE(window.windowFlags().testFlag(Qt::WindowStaysOnTopHint), alwaysOnTop);
    QVERIFY(!window.isVisible());
}

void TestSessionsUi::startup_initializes_only_requested_units()
{
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    StartupDiagnostics::begin();
    const auto stopDiagnostics = qScopeGuard([] {
        StartupDiagnostics::firstPaintFinished();
        StartupDiagnostics::finishWhenIdle();
    });
    const QStringList identifiers = Evaluator::builtInUnitIdentifiers();
    QVERIFY(identifiers.contains(QStringLiteral("km")));
    QVERIFY(identifiers.contains(QStringLiteral("KiB")));
    QVERIFY(Units::isBuiltInUnitIdentifier(QStringLiteral("mrad")));
    QVERIFY(Units::tryGetBuiltInUnitValue(QStringLiteral("km"), nullptr));
    QVERIFY(Units::tryGetBuiltInUnitQuantity(QStringLiteral("mrad"), 'd', nullptr));
    for (const auto& phase : StartupDiagnostics::snapshot().phases)
        QVERIFY(phase.name != QLatin1String("Construct built-in unit value"));

    MainWindow window(false);
    Editor* editor = window.findChild<Editor*>();
    QVERIFY(editor);
    Session session;
    Evaluator* evaluator = session.evaluator();
    evaluator->setExpression(QStringLiteral("6*7"));
    QCOMPARE(evaluator->evalUpdateAns(), Quantity(42));
    QVERIFY(evaluator->error().isEmpty());
    editor->setText(QStringLiteral("kilo"));
    EditorCompletion* completion = editor->findChild<EditorCompletion*>();
    QVERIFY(completion);
    completion->showCompletion({QStringLiteral("kilometre:Kilometre")});
    QVERIFY(completion->isVisible());
    for (const auto& phase : StartupDiagnostics::snapshot().phases)
        QVERIFY(phase.name != QLatin1String("Initialize requested built-in unit"));

    evaluator->setExpression(QStringLiteral("2[km] in [m]"));
    const Quantity converted = evaluator->evalUpdateAns();
    QVERIFY2(evaluator->error().isEmpty(), qPrintable(evaluator->error()));
    QCOMPARE(converted.numericValue(), CNumber(2000));
    QVERIFY(converted.sameDimension(Units::metre()));
    for (const auto& phase : StartupDiagnostics::snapshot().phases) {
        QVERIFY(phase.name != QLatin1String("Materialize all built-in unit values"));
        QVERIFY(phase.name != QLatin1String("Prepare built-in unit display lookup"));
        QVERIFY(phase.name != QLatin1String("Apply angle mode to unit lookup"));
    }
}

void TestSessionsUi::startup_keeps_reused_pane_parented_data()
{
    QTest::addColumn<int>("layoutKind");
    QTest::newRow("tabs") << 0;
    QTest::newRow("nested-split") << 1;
    QTest::newRow("empty-split-before-pane") << 2;
}

void TestSessionsUi::startup_keeps_reused_pane_parented()
{
    QFETCH(int, layoutKind);
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    const QString name = QStringLiteral("Retained");
    QVERIFY(QDir().mkpath(QFileInfo(storage.path(name)).path()));
    writeFile(storage.path(name), QJsonDocument(sessionJson(name)).toJson());
    QJsonObject windowState = startupSessionWindow(name, QStringLiteral("saved"));
    if (layoutKind != 0) {
        QJsonObject pane = windowState.value(QStringLiteral("root")).toObject();
        pane.insert(QStringLiteral("type"), QStringLiteral("pane"));
        QJsonObject nested {
            { QStringLiteral("type"), QStringLiteral("split") },
            { QStringLiteral("children"), QJsonArray {pane} }
        };
        if (layoutKind == 2)
            nested.insert(QStringLiteral("children"), QJsonArray());
        windowState.insert(QStringLiteral("root"), QJsonObject {
            { QStringLiteral("type"), QStringLiteral("split") },
            { QStringLiteral("children"), layoutKind == 2 ? QJsonArray {nested, pane} : QJsonArray {nested} }
        });
    }
    guard.settings->sessionLayoutJson = QString::fromUtf8(QJsonDocument(QJsonObject {
        { QStringLiteral("scheme"), 1 },
        { QStringLiteral("kind"), QStringLiteral("session-layout") },
        { QStringLiteral("windows"), QJsonArray {windowState} }
    }).toJson());
    class ParentObserver : public QObject {
    public:
        int detached = 0;
        int changes = 0;
        bool eventFilter(QObject* object, QEvent* event) override
        {
            if (event->type() == QEvent::ParentChange) {
                ++changes;
                if (!object->parent())
                    ++detached;
            }
            return false;
        }
    } observer;
    MainWindow window;
    QPointer<ResultDisplay> display = window.findChild<ResultDisplay*>();
    QPointer<QWidget> pane = paneWidgetForDisplay(display);
    QVERIFY(pane);
    pane->installEventFilter(&observer);
    window.showWhenReady();
    QTRY_VERIFY(display && display->session()->name() == name);
    QTRY_VERIFY(window.isVisible());
    QCOMPARE(observer.detached, 0);
    QCOMPARE(observer.changes, layoutKind == 1 ? 1 : 0);
    if (layoutKind == 2) {
        auto* splitter = qobject_cast<QSplitter*>(pane->parentWidget());
        QVERIFY(splitter);
        QCOMPARE(splitter->indexOf(pane), 1);
    }
    QCOMPARE(editorForDisplay(display)->text(), QStringLiteral("saved draft"));
    QCOMPARE(display->session()->historySize(), 1);
}

void TestSessionsUi::startup_geometry_fallback_data()
{
    QTest::addColumn<int>("geometryKind");
    QTest::newRow("saved") << 0;
    QTest::newRow("missing") << 1;
    QTest::newRow("corrupt") << 2;
    QTest::newRow("off-screen") << 3;
    QTest::newRow("maximized") << 4;
}

void TestSessionsUi::startup_geometry_fallback()
{
    QFETCH(int, geometryKind);
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    QWidget source;
    source.setGeometry(100, 100, 700, 500);
    if (geometryKind == 3)
        source.move(-20000, -20000);
    if (geometryKind == 4)
        source.setWindowState(Qt::WindowMaximized);
    QByteArray geometry = source.saveGeometry();
    if (geometryKind == 1)
        geometry.clear();
    if (geometryKind == 2)
        geometry = QByteArrayLiteral("invalid geometry");
    const QString name = QStringLiteral("Geometry");
    QVERIFY(QDir().mkpath(QFileInfo(storage.path(name)).path()));
    writeFile(storage.path(name), QJsonDocument(sessionJson(name)).toJson());
    QJsonObject saved = startupSessionWindow(name, QStringLiteral("saved"));
    saved.insert(QStringLiteral("geometry"), QString::fromLatin1(geometry.toBase64()));
    guard.settings->windowGeometry.clear();
    guard.settings->keypadVisible = false;
    guard.settings->keypadMode = Settings::KeypadModeDisabled;
    guard.settings->sessionLayoutJson = QString::fromUtf8(QJsonDocument(QJsonObject {
        { QStringLiteral("scheme"), 1 },
        { QStringLiteral("kind"), QStringLiteral("session-layout") },
        { QStringLiteral("windows"), QJsonArray {saved} }
    }).toJson());
    MainWindow window;
    if (geometryKind == 0)
        QCOMPARE(window.size(), QSize(700, 500));
    if (geometryKind == 1 || geometryKind == 2)
        QCOMPARE(window.size(), QSize(640, 480));
    window.showWhenReady();
    QTRY_VERIFY(window.isVisible());
    QTRY_COMPARE(window.findChild<ResultDisplay*>()->session()->name(), name);
    if (geometryKind == 4) {
        QTRY_VERIFY(window.isMaximized());
    } else {
        const QSize expected = geometryKind == 1 || geometryKind == 2
            ? QSize(640, 480) : QSize(700, 500);
        QTRY_COMPARE(window.size(), expected);
        QVERIFY(QGuiApplication::primaryScreen()->availableGeometry().intersects(window.frameGeometry()));
    }
}

void TestSessionsUi::startup_reuses_initial_pane_data()
{
    QTest::addColumn<int>("layoutKind");
    QTest::addColumn<bool>("classic");
    const QStringList layouts { QStringLiteral("single"), QStringLiteral("tabs"),
        QStringLiteral("split"), QStringLiteral("nested"),
        QStringLiteral("missing-first-session") };
    for (int kind = 0; kind < layouts.size(); ++kind) {
        QTest::newRow(qPrintable(layouts.at(kind) + QStringLiteral("-modern"))) << kind << false;
        QTest::newRow(qPrintable(layouts.at(kind) + QStringLiteral("-classic"))) << kind << true;
    }
}

void TestSessionsUi::startup_shows_restored_session_on_first_show_data()
{
    QTest::addColumn<bool>("savedLayout");
    QTest::addColumn<bool>("savedSession");
    QTest::addColumn<bool>("alwaysOnTop");
    for (bool layout : {false, true}) {
        for (bool session : {false, true}) {
            if (!layout && session)
                continue;
            for (bool onTop : {false, true}) {
                QTest::addRow("layout-%d-session-%d-on-top-%d", layout, session, onTop)
                    << layout << session << onTop;
            }
        }
    }
}

void TestSessionsUi::startup_shows_restored_session_on_first_show()
{
    QFETCH(bool, savedLayout);
    QFETCH(bool, savedSession);
    QFETCH(bool, alwaysOnTop);
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    Settings* settings = guard.settings;
    settings->hasNumberFormatStyleSetting = true;
    settings->openPrivateTabOnStartup = false;
    settings->windowAlwaysOnTop = alwaysOnTop;
    settings->windowOnfullScreen = false;
    settings->windowState.clear();
    settings->windowGeometry.clear();
    settings->sessionLayoutJson.clear();
    const QString name = savedLayout ? QStringLiteral("Saved-Startup") : QStringLiteral("Untitled-1");
    if (savedSession) {
        QVERIFY(QDir().mkpath(QFileInfo(storage.path(name)).path()));
        writeFile(storage.path(name), QJsonDocument(sessionJson(name)).toJson());
    }
    if (savedLayout) {
        settings->sessionLayoutJson = QString::fromUtf8(QJsonDocument(QJsonObject {
            { QStringLiteral("scheme"), 1 },
            { QStringLiteral("kind"), QStringLiteral("session-layout") },
            { QStringLiteral("windows"), QJsonArray { QJsonObject {
                { QStringLiteral("id"), QStringLiteral("saved") },
                { QStringLiteral("root"), QJsonObject {
                    { QStringLiteral("type"), QStringLiteral("tabs") },
                    { QStringLiteral("active"), name },
                    { QStringLiteral("tabs"), QJsonArray { QJsonObject {
                        { QStringLiteral("name"), name }
                    } } }
                } }
            } } }
        }).toJson(QJsonDocument::Compact));
    }
    class StartupWindow : public MainWindow {
    public:
        int showCount = 0;
        int firstHistorySize = -1;
        QString firstName;
        QString firstText;
    protected:
        void showEvent(QShowEvent* event) override
        {
            if (showCount++ == 0) {
                const auto* display = findChild<ResultDisplay*>();
                firstHistorySize = display->session()->historySize();
                firstName = display->session()->name();
                firstText = display->toPlainText();
            }
            MainWindow::showEvent(event);
        }
    };
    StartupDiagnostics::begin();
    const auto finishRecording = qScopeGuard([]() {
        StartupDiagnostics::firstPaintFinished();
        StartupDiagnostics::finishWhenIdle();
    });
    StartupWindow window;
    QVERIFY(!window.isVisible());
    window.showWhenReady();
    QVERIFY(!window.isVisible());
    QCOMPARE(window.showCount, 0);
    QTRY_VERIFY(window.isVisible());
    QCOMPARE(window.showCount, 1);
    QCOMPARE(window.firstHistorySize, savedSession ? 1 : 0);
    QCOMPARE(window.firstName, savedSession ? name : QStringLiteral("Untitled-1"));
    if (savedSession)
        QVERIFY(window.firstText.contains(QStringLiteral("42")));
    QCOMPARE(window.windowFlags().testFlag(Qt::WindowStaysOnTopHint), alwaysOnTop);
    int themeCalls = 0;
    for (const auto& phase : StartupDiagnostics::snapshot().phases) {
        if (phase.name == QLatin1String("Apply pane themes"))
            themeCalls += phase.calls;
    }
    QCOMPARE(themeCalls, 1);
}

void TestSessionsUi::startup_hide_cancels_pending_show()
{
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    guard.settings->hasNumberFormatStyleSetting = true;
    guard.settings->sessionLayoutJson.clear();
    MainWindow window;
    window.showWhenReady();
    window.hide();
    QTRY_VERIFY(!MainWindow::sessionRestoreInProgress());
    QVERIFY(!window.isVisible());
    window.show();
    QVERIFY(window.isVisible());
}

void TestSessionsUi::startup_reuses_initial_pane()
{
    QFETCH(int, layoutKind);
    QFETCH(bool, classic);
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    Settings* settings = guard.settings;
    settings->editorDisplayStyle = classic ? Settings::EditorDisplayClassic : Settings::EditorDisplayModern;
    settings->openPrivateTabOnStartup = false;
    settings->hasNumberFormatStyleSetting = true;
    settings->windowState.clear();
    settings->windowGeometry.clear();
    const QString activeName = QStringLiteral("Saved-Active");
    const QString firstName = QStringLiteral("Saved-First");
    const QString otherName = QStringLiteral("Saved-Other");
    QVERIFY(QDir().mkpath(QFileInfo(storage.path(activeName)).path()));
    for (const QString& name : {activeName, firstName, otherName})
        writeFile(storage.path(name), QJsonDocument(sessionJson(name)).toJson());

    const auto savedPane = [](const QString& name) {
        return QJsonObject {
            { QStringLiteral("type"), QStringLiteral("pane") },
            { QStringLiteral("active"), name },
            { QStringLiteral("tabs"), QJsonArray { QJsonObject {
                { QStringLiteral("name"), name },
                { QStringLiteral("editor"), QJsonObject {
                    { QStringLiteral("text"), QStringLiteral("12+34") },
                    { QStringLiteral("cursor"), 3 }
                } }
            } } }
        };
    };
    QJsonObject root = savedPane(activeName);
    root.insert(QStringLiteral("type"), QStringLiteral("tabs"));
    if (layoutKind == 1) {
        QJsonArray tabs = root.value(QStringLiteral("tabs")).toArray();
        tabs.append(QJsonObject { { QStringLiteral("name"), otherName } });
        root.insert(QStringLiteral("tabs"), tabs);
    } else if (layoutKind >= 2) {
        QJsonObject firstPane = savedPane(layoutKind == 4
            ? QStringLiteral("Missing") : firstName);
        QJsonObject activePane = savedPane(activeName);
        if (layoutKind == 3) {
            firstPane = QJsonObject {
                { QStringLiteral("type"), QStringLiteral("split") },
                { QStringLiteral("orientation"), QStringLiteral("vertical") },
                { QStringLiteral("children"), QJsonArray { firstPane, savedPane(otherName) } },
                { QStringLiteral("sizes"), QJsonArray { 200, 100 } }
            };
        }
        root = QJsonObject {
            { QStringLiteral("type"), QStringLiteral("split") },
            { QStringLiteral("active"), activeName },
            { QStringLiteral("orientation"), QStringLiteral("horizontal") },
            { QStringLiteral("children"), QJsonArray { firstPane, activePane } },
            { QStringLiteral("sizes"), QJsonArray { 320, 480 } }
        };
    }
    settings->sessionLayoutJson = QString::fromUtf8(QJsonDocument(QJsonObject {
        { QStringLiteral("scheme"), 1 },
        { QStringLiteral("kind"), QStringLiteral("session-layout") },
        { QStringLiteral("windows"), QJsonArray { QJsonObject {
            { QStringLiteral("id"), QStringLiteral("saved") },
            { QStringLiteral("root"), root }
        } } }
    }).toJson(QJsonDocument::Compact));

    MainWindow window;
    QPointer<ResultDisplay> initialDisplay = window.findChild<ResultDisplay*>();
    QPointer<Editor> initialEditor = editorForDisplay(initialDisplay);
    QPointer<QWidget> initialPane = paneWidgetForDisplay(initialDisplay);
    QPointer<QTabBar> initialTabBar = tabBarForDisplay(initialDisplay);
    QVERIFY(initialDisplay && initialEditor && initialPane && initialTabBar);
    window.resize(960, 640);
    window.show();
    window.activateWindow();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    const int expectedPanes = layoutKind == 2 ? 2 : layoutKind == 3 ? 3 : 1;
    QTRY_COMPARE(window.findChildren<Editor*>().size(), expectedPanes);
    const QString reusedSession = layoutKind == 2 || layoutKind == 3 ? firstName : activeName;
    QTRY_VERIFY(initialDisplay && initialDisplay->session()->name() == reusedSession);
    QVERIFY(initialEditor && initialPane && initialTabBar);
    QCOMPARE(editorForDisplay(initialDisplay), initialEditor.data());
    QCOMPARE(paneWidgetForDisplay(initialDisplay), initialPane.data());
    QCOMPARE(tabBarForDisplay(initialDisplay), initialTabBar.data());
    QVERIFY(initialEditor->isVisible());
    QCOMPARE(initialEditor->text(), QStringLiteral("12+34"));
    QCOMPARE(initialEditor->textCursor().position(), 3);
    QCOMPARE(initialTabBar->count(), layoutKind == 1 ? 2 : 1);

    const QList<ResultDisplay*> displays = window.findChildren<ResultDisplay*>();
    for (ResultDisplay* display : displays) {
        Editor* editor = editorForDisplay(display);
        QVERIFY(editor != nullptr);
        QCOMPARE(display->session()->historySize(), 1);
        QVERIFY(display->toPlainText().contains(QStringLiteral("42")));
        QTest::mouseClick(editor->viewport(), Qt::LeftButton);
        QTRY_VERIFY(editor->hasFocus());
        editor->setText(QStringLiteral("10+1"));
        QTest::keyClick(editor, Qt::Key_Return);
        QCOMPARE(display->session()->historySize(), 2);
        QVERIFY(display->toPlainText().contains(QStringLiteral("11")));
    }
    if (layoutKind == 1) {
        initialTabBar->setCurrentIndex(1);
        QCOMPARE(initialDisplay->session()->name(), otherName);
        initialTabBar->setCurrentIndex(0);
        QCOMPARE(initialDisplay->session()->name(), activeName);
        QCOMPARE(initialDisplay->session()->historySize(), 2);
    }
}

void TestSessionsUi::always_on_top_toggles_preserve_window_geometry()
{
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;
    const bool oldAlwaysOnTop = settings->windowAlwaysOnTop;
    const bool oldFullScreen = settings->windowOnfullScreen;
    const auto restoreWindowSettings = qScopeGuard([settings, oldAlwaysOnTop, oldFullScreen]() {
        settings->windowAlwaysOnTop = oldAlwaysOnTop;
        settings->windowOnfullScreen = oldFullScreen;
    });
    settings->windowAlwaysOnTop = false;
    settings->windowOnfullScreen = false;
    settings->hasNumberFormatStyleSetting = true;
    settings->sessionLayoutJson.clear();
    settings->windowState.clear();
    settings->windowGeometry.clear();

    MainWindow window(false);
    window.resize(640, 480);
    window.move(100, 100);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QCoreApplication::processEvents();

    QAction* alwaysOnTop = nullptr;
    for (QAction* action : window.findChildren<QAction*>()) {
        if (action->text() == QStringLiteral("Always on &Top")) {
            alwaysOnTop = action;
            break;
        }
    }
    QVERIFY(alwaysOnTop != nullptr);
    QVERIFY(!alwaysOnTop->isChecked());
    const QRect originalFrame = window.frameGeometry();
    const QRect originalGeometry = window.geometry();
    const Qt::WindowFlags originalFlags = window.windowFlags();

    for (int toggle = 0; toggle < 10; ++toggle) {
        const bool enabled = toggle % 2 == 0;
        alwaysOnTop->trigger();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QCoreApplication::processEvents();

        QCOMPARE(alwaysOnTop->isChecked(), enabled);
        QCOMPARE(settings->windowAlwaysOnTop, enabled);
        QCOMPARE(window.windowFlags().testFlag(Qt::WindowStaysOnTopHint), enabled);
        QCOMPARE(window.windowFlags() & ~Qt::WindowStaysOnTopHint, originalFlags);
        QCOMPARE(window.frameGeometry(), originalFrame);
        QCOMPARE(window.geometry(), originalGeometry);
    }
}

void TestSessionsUi::saved_window_ui_state_overrides_defaults_before_show()
{
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;

    settings->sessionLayoutJson.clear();
    settings->windowState.clear();
    settings->windowGeometry.clear();
    settings->constantsDockVisible = false;
    settings->functionsDockVisible = false;
    settings->historyDockVisible = false;
    settings->keypadMode = Settings::KeypadModeDisabled;
    settings->keypadVisible = false;
    settings->formulaBookDockVisible = false;
    settings->variablesDockVisible = false;
    settings->userFunctionsDockVisible = false;
    settings->userUnitsDockVisible = false;
    settings->bitfieldVisible = false;
    settings->statusBarVisible = false;
    settings->hasNumberFormatStyleSetting = true;

    QByteArray docklessWindowState;
    {
        MainWindow sourceWindow(false);
        docklessWindowState = sourceWindow.saveState(1);
    }
    QVERIFY(!docklessWindowState.isEmpty());

    QMainWindow compactGeometrySource;
    compactGeometrySource.resize(560, 380);
    const QSize compactSize = compactGeometrySource.size();
    const QByteArray compactGeometry = compactGeometrySource.saveGeometry();
    QVERIFY(!compactGeometry.isEmpty());

    QJsonObject tab {
        { QStringLiteral("name"), QStringLiteral("Missing session") },
        { QStringLiteral("file"), QStringLiteral("missing-window-ui-state.json") }
    };
    QJsonObject root {
        { QStringLiteral("type"), QStringLiteral("tabs") },
        { QStringLiteral("active"), QStringLiteral("Missing session") },
        { QStringLiteral("tabs"), QJsonArray({ tab }) }
    };
    QJsonObject savedWindow {
        { QStringLiteral("id"), QStringLiteral("window-0") },
        { QStringLiteral("active"), true },
        { QStringLiteral("root"), root },
        { QStringLiteral("statusBarVisible"), false },
        { QStringLiteral("keypadVisible"), false },
        { QStringLiteral("bitfieldVisible"), false },
        { QStringLiteral("windowState"),
          QString::fromLatin1(docklessWindowState.toBase64()) },
        { QStringLiteral("geometry"),
          QString::fromLatin1(compactGeometry.toBase64()) }
    };
    QJsonObject layout {
        { QStringLiteral("scheme"), 1 },
        { QStringLiteral("kind"), QStringLiteral("session-layout") },
        { QStringLiteral("activeWindow"), QStringLiteral("window-0") },
        { QStringLiteral("windows"), QJsonArray({ savedWindow }) }
    };
    settings->sessionLayoutJson = QString::fromUtf8(
        QJsonDocument(layout).toJson(QJsonDocument::Compact));

    // Deliberately conflicting defaults must never become the visible state of
    // a window that has its own saved UI record.
    settings->constantsDockVisible = true;
    settings->keypadMode = Settings::KeypadModeBasicWide;
    settings->keypadVisible = true;
    settings->statusBarVisible = true;

    MainWindow restoredWindow;

    QStatusBar* statusBar =
        restoredWindow.findChild<QStatusBar*>(QString(), Qt::FindDirectChildrenOnly);
    QDockWidget* constantsDock =
        restoredWindow.findChild<QDockWidget*>(QStringLiteral("ConstantsDock"));
    QVERIFY(statusBar == nullptr || statusBar->isHidden());
    QVERIFY(restoredWindow.findChild<Keypad*>() == nullptr);
    QVERIFY(constantsDock == nullptr);
    QCOMPARE(restoredWindow.size(), compactSize);
}

void TestSessionsUi::visible_window_applies_restored_dock_and_keypad_layout()
{
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;

    settings->sessionLayoutJson.clear();
    settings->windowState.clear();
    settings->constantsDockVisible = false;
    settings->functionsDockVisible = false;
    settings->historyDockVisible = false;
    settings->keypadMode = Settings::KeypadModeBasicWide;
    settings->keypadVisible = true;
    settings->keypadZoomPercent = 100;
    settings->formulaBookDockVisible = false;
    settings->variablesDockVisible = false;
    settings->userFunctionsDockVisible = false;
    settings->userUnitsDockVisible = false;
    settings->bitfieldVisible = false;
    settings->hasNumberFormatStyleSetting = true;

    QByteArray docklessWindowState;
    {
        MainWindow sourceWindow(false);
        sourceWindow.show();
        QVERIFY(QTest::qWaitForWindowExposed(&sourceWindow));
        docklessWindowState = sourceWindow.saveState(1);
    }
    QVERIFY(!docklessWindowState.isEmpty());

    // Global settings describe the primary window. A secondary window can
    // restore after it is shown, and must replace that inherited dock state.
    settings->constantsDockVisible = true;

    MainWindow restoredWindow(false);
    restoredWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&restoredWindow));

    QDockWidget* constantsDock =
        restoredWindow.findChild<QDockWidget*>(QStringLiteral("ConstantsDock"));
    QVERIFY(constantsDock != nullptr);
    QVERIFY(constantsDock->isVisible());
    QVERIFY(QMetaObject::invokeMethod(&restoredWindow,
                                      "restoreWindowLayoutState",
                                      Qt::DirectConnection,
                                      Q_ARG(QByteArray, docklessWindowState)));
    QVERIFY(!constantsDock->isVisible());

    QAction* restoredBasicAction =
        keypadModeAction(&restoredWindow, Settings::KeypadModeBasicWide);
    QVERIFY(restoredBasicAction != nullptr);
    QVERIFY(restoredBasicAction->isChecked());
    QVERIFY(restoredWindow.findChild<Keypad*>() != nullptr);
    QVERIFY(QMetaObject::invokeMethod(&restoredWindow,
                                      "restoreWindowKeypadZoom",
                                      Qt::DirectConnection,
                                      Q_ARG(int, 150)));

    MainWindow secondaryWindow(false);
    secondaryWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&secondaryWindow));
    QVERIFY(QMetaObject::invokeMethod(
        &secondaryWindow,
        "restoreWindowKeypadLayout",
        Qt::DirectConnection,
        Q_ARG(bool, true),
        Q_ARG(int, static_cast<int>(Settings::KeypadModeScientificNarrow))));
    QVERIFY(QMetaObject::invokeMethod(&secondaryWindow,
                                      "restoreWindowKeypadZoom",
                                      Qt::DirectConnection,
                                      Q_ARG(int, 200)));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    QAction* secondaryScientificNarrowAction =
        keypadModeAction(&secondaryWindow, Settings::KeypadModeScientificNarrow);
    QVERIFY(secondaryScientificNarrowAction != nullptr);
    QVERIFY(secondaryScientificNarrowAction->isChecked());
    QVERIFY(secondaryWindow.findChild<Keypad*>() != nullptr);
    QVERIFY(restoredBasicAction->isChecked());
    QVERIFY(restoredWindow.findChild<Keypad*>() != nullptr);

    restoredWindow.persistSessionAndSettingsForShutdown();
    const QJsonDocument savedLayout =
        QJsonDocument::fromJson(settings->sessionLayoutJson.toUtf8());
    QVERIFY(savedLayout.isObject());
    const QJsonArray savedWindows =
        savedLayout.object().value(QStringLiteral("windows")).toArray();
    QCOMPARE(savedWindows.size(), 2);
    bool savedBasicKeypad = false;
    bool savedScientificNarrowKeypad = false;
    for (const QJsonValue& value : savedWindows) {
        const QJsonObject window = value.toObject();
        QVERIFY(window.value(QStringLiteral("keypadVisible")).toBool(false));
        const int mode = window.value(QStringLiteral("keypadMode")).toInt(-1);
        const int zoomPercent =
            window.value(QStringLiteral("keypadZoomPercent")).toInt(-1);
        savedBasicKeypad = savedBasicKeypad
            || (mode == static_cast<int>(Settings::KeypadModeBasicWide)
                && zoomPercent == 150);
        savedScientificNarrowKeypad = savedScientificNarrowKeypad
            || (mode == static_cast<int>(Settings::KeypadModeScientificNarrow)
                && zoomPercent == 200);
    }
    QVERIFY(savedBasicKeypad);
    QVERIFY(savedScientificNarrowKeypad);

    QVERIFY(QMetaObject::invokeMethod(
        &secondaryWindow,
        "restoreWindowKeypadLayout",
        Qt::DirectConnection,
        Q_ARG(bool, false),
        Q_ARG(int, static_cast<int>(Settings::KeypadModeScientificNarrow))));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
    QVERIFY(secondaryWindow.findChild<Keypad*>() == nullptr);
    QAction* secondaryDisabledAction =
        keypadModeAction(&secondaryWindow, Settings::KeypadModeDisabled);
    QVERIFY(secondaryDisabledAction != nullptr);
    QVERIFY(secondaryDisabledAction->isChecked());
    QVERIFY(restoredBasicAction->isChecked());
    QVERIFY(restoredWindow.findChild<Keypad*>() != nullptr);

    QMainWindow compactGeometrySource;
    compactGeometrySource.resize(560, 380);
    compactGeometrySource.show();
    QVERIFY(QTest::qWaitForWindowExposed(&compactGeometrySource));
    const QSize compactSize = compactGeometrySource.size();
    const QByteArray compactGeometry = compactGeometrySource.saveGeometry();
    QVERIFY(!compactGeometry.isEmpty());

    settings->constantsDockVisible = true;
    settings->keypadMode = Settings::KeypadModeBasicWide;
    settings->windowState.clear();
    MainWindow compactWindow(false);
    compactWindow.resize(1000, 700);
    QDockWidget* compactConstantsDock =
        compactWindow.findChild<QDockWidget*>(QStringLiteral("ConstantsDock"));
    QVERIFY(compactConstantsDock != nullptr);
    QVERIFY(!compactConstantsDock->isHidden());
    QVERIFY(compactWindow.findChild<Keypad*>() != nullptr);

    QVERIFY(QMetaObject::invokeMethod(&compactWindow,
                                      "restoreWindowLayoutState",
                                      Qt::DirectConnection,
                                      Q_ARG(QByteArray, docklessWindowState)));
    QVERIFY(QMetaObject::invokeMethod(
        &compactWindow,
        "restoreWindowKeypadLayout",
        Qt::DirectConnection,
        Q_ARG(bool, false),
        Q_ARG(int, static_cast<int>(Settings::KeypadModeBasicWide))));
    QVERIFY(QMetaObject::invokeMethod(&compactWindow,
                                      "showRestoredWindow",
                                      Qt::DirectConnection,
                                      Q_ARG(QByteArray, compactGeometry)));

    QVERIFY(!compactWindow.isVisible());
    QTRY_VERIFY(compactWindow.isVisible());
    QVERIFY(QTest::qWaitForWindowExposed(&compactWindow));
    QTRY_VERIFY(!compactConstantsDock->isVisible());
    QTRY_VERIFY(compactWindow.findChild<Keypad*>() == nullptr);
    QTRY_COMPARE(compactWindow.size(), compactSize);
}

void TestSessionsUi::persisting_layout_captures_visible_scroll_positions_for_all_panes()
{
    Settings* appSettings = Settings::instance();
    struct SettingsGuard {
        Settings* settings;
        QByteArray oldSkipUpdateCheck;
        bool hadSkipUpdateCheck;
        QString oldSessionLayoutJson;
        bool oldConstantsDockVisible;
        bool oldFunctionsDockVisible;
        bool oldHistoryDockVisible;
        bool oldKeypadVisible;
        bool oldFormulaBookDockVisible;
        bool oldVariablesDockVisible;
        bool oldUserFunctionsDockVisible;
        bool oldUserUnitsDockVisible;
        bool oldBitfieldVisible;
        bool oldWindowPositionSave;
        bool oldHasNumberFormatStyleSetting;

        ~SettingsGuard()
        {
            settings->sessionLayoutJson = oldSessionLayoutJson;
            settings->constantsDockVisible = oldConstantsDockVisible;
            settings->functionsDockVisible = oldFunctionsDockVisible;
            settings->historyDockVisible = oldHistoryDockVisible;
            settings->keypadVisible = oldKeypadVisible;
            settings->formulaBookDockVisible = oldFormulaBookDockVisible;
            settings->variablesDockVisible = oldVariablesDockVisible;
            settings->userFunctionsDockVisible = oldUserFunctionsDockVisible;
            settings->userUnitsDockVisible = oldUserUnitsDockVisible;
            settings->bitfieldVisible = oldBitfieldVisible;
            settings->windowPositionSave = oldWindowPositionSave;
            settings->hasNumberFormatStyleSetting = oldHasNumberFormatStyleSetting;
            if (hadSkipUpdateCheck)
                qputenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK", oldSkipUpdateCheck);
            else
                qunsetenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK");
        }
    } guard {
        appSettings,
        qgetenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK"),
        qEnvironmentVariableIsSet("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK"),
        appSettings->sessionLayoutJson,
        appSettings->constantsDockVisible,
        appSettings->functionsDockVisible,
        appSettings->historyDockVisible,
        appSettings->keypadVisible,
        appSettings->formulaBookDockVisible,
        appSettings->variablesDockVisible,
        appSettings->userFunctionsDockVisible,
        appSettings->userUnitsDockVisible,
        appSettings->bitfieldVisible,
        appSettings->windowPositionSave,
        appSettings->hasNumberFormatStyleSetting
    };

    qputenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK", "1");

    appSettings->sessionLayoutJson.clear();
    appSettings->constantsDockVisible = false;
    appSettings->functionsDockVisible = false;
    appSettings->historyDockVisible = false;
    appSettings->keypadVisible = false;
    appSettings->formulaBookDockVisible = false;
    appSettings->variablesDockVisible = false;
    appSettings->userFunctionsDockVisible = false;
    appSettings->userUnitsDockVisible = false;
    appSettings->bitfieldVisible = false;
    appSettings->windowPositionSave = false;
    appSettings->hasNumberFormatStyleSetting = true;

    MainWindow window;
    window.resize(900, 500);
    window.show();
    QCoreApplication::processEvents();

    ResultDisplay* firstDisplay = window.findChild<ResultDisplay*>();
    Editor* firstEditor = window.findChild<Editor*>();
    QVERIFY(firstDisplay != nullptr);
    QVERIFY(firstEditor != nullptr);

    for (int i = 0; i < 80; ++i) {
        firstEditor->setText(QString::number(i));
        QVERIFY(QMetaObject::invokeMethod(&window, "evaluateEditorExpression", Qt::DirectConnection));
    }
    QCoreApplication::processEvents();

    QScrollBar* firstScrollBar = firstDisplay->verticalScrollBar();
    QVERIFY(firstScrollBar->maximum() > 30);
    firstScrollBar->setValue(10);
    QCOMPARE(firstScrollBar->value(), 10);

    QVERIFY(QMetaObject::invokeMethod(&window, "splitActivePaneRight", Qt::DirectConnection));
    QCoreApplication::processEvents();

    const QList<ResultDisplay*> displays = window.findChildren<ResultDisplay*>();
    QCOMPARE(displays.size(), 2);
    ResultDisplay* secondDisplay = displays.first() == firstDisplay ? displays.last() : displays.first();
    QWidget* secondPage = secondDisplay->parentWidget();
    Editor* secondEditor = secondPage ? secondPage->findChild<Editor*>(QString(), Qt::FindDirectChildrenOnly) : nullptr;
    QVERIFY(secondEditor != nullptr);

    for (int i = 0; i < 80; ++i) {
        secondEditor->setText(QString::number(i + 100));
        QVERIFY(QMetaObject::invokeMethod(&window, "evaluateEditorExpression", Qt::DirectConnection));
    }
    QCoreApplication::processEvents();

    QScrollBar* secondScrollBar = secondDisplay->verticalScrollBar();
    QVERIFY(secondScrollBar->maximum() > 30);
    secondScrollBar->setValue(15);
    QCOMPARE(secondScrollBar->value(), 15);

    firstEditor->setText(QStringLiteral("first pane draft"));
    firstEditor->setCursorPosition(5);
    secondEditor->setText(QStringLiteral("second pane draft"));
    secondEditor->setCursorPosition(6);

    firstScrollBar->setValue(22);
    QCOMPARE(firstScrollBar->value(), 22);

    window.persistSessionAndSettingsForShutdown();

    const QJsonDocument layoutDoc = QJsonDocument::fromJson(appSettings->sessionLayoutJson.toUtf8());
    QVERIFY(layoutDoc.isObject());
    const QJsonArray windows = layoutDoc.object().value(QStringLiteral("windows")).toArray();
    QCOMPARE(windows.size(), 1);
    const QJsonObject root = windows.first().toObject().value(QStringLiteral("root")).toObject();
    QList<int> scrollValues;
    appendPaneScrollValues(root, &scrollValues);

    QVERIFY(scrollValues.contains(22));
    QVERIFY(scrollValues.contains(15));

    QStringList editorTexts;
    appendPaneEditorTexts(root, &editorTexts);
    QVERIFY(editorTexts.contains(QStringLiteral("first pane draft")));
    QVERIFY(editorTexts.contains(QStringLiteral("second pane draft")));
}

void TestSessionsUi::switching_session_tabs_preserves_each_editor_text()
{
    Settings* appSettings = Settings::instance();
    struct SettingsGuard {
        Settings* settings;
        QByteArray oldSkipUpdateCheck;
        bool hadSkipUpdateCheck;
        QString oldSessionLayoutJson;
        bool oldConstantsDockVisible;
        bool oldFunctionsDockVisible;
        bool oldHistoryDockVisible;
        bool oldKeypadVisible;
        bool oldFormulaBookDockVisible;
        bool oldVariablesDockVisible;
        bool oldUserFunctionsDockVisible;
        bool oldUserUnitsDockVisible;
        bool oldBitfieldVisible;
        bool oldWindowPositionSave;
        bool oldHasNumberFormatStyleSetting;

        ~SettingsGuard()
        {
            if (hadSkipUpdateCheck)
                qputenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK", oldSkipUpdateCheck);
            else
                qunsetenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK");
            settings->sessionLayoutJson = oldSessionLayoutJson;
            settings->constantsDockVisible = oldConstantsDockVisible;
            settings->functionsDockVisible = oldFunctionsDockVisible;
            settings->historyDockVisible = oldHistoryDockVisible;
            settings->keypadVisible = oldKeypadVisible;
            settings->formulaBookDockVisible = oldFormulaBookDockVisible;
            settings->variablesDockVisible = oldVariablesDockVisible;
            settings->userFunctionsDockVisible = oldUserFunctionsDockVisible;
            settings->userUnitsDockVisible = oldUserUnitsDockVisible;
            settings->bitfieldVisible = oldBitfieldVisible;
            settings->windowPositionSave = oldWindowPositionSave;
            settings->hasNumberFormatStyleSetting = oldHasNumberFormatStyleSetting;
        }
    } guard {
        appSettings,
        qgetenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK"),
        qEnvironmentVariableIsSet("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK"),
        appSettings->sessionLayoutJson,
        appSettings->constantsDockVisible,
        appSettings->functionsDockVisible,
        appSettings->historyDockVisible,
        appSettings->keypadVisible,
        appSettings->formulaBookDockVisible,
        appSettings->variablesDockVisible,
        appSettings->userFunctionsDockVisible,
        appSettings->userUnitsDockVisible,
        appSettings->bitfieldVisible,
        appSettings->windowPositionSave,
        appSettings->hasNumberFormatStyleSetting
    };

    qputenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK", "1");
    appSettings->sessionLayoutJson.clear();
    appSettings->constantsDockVisible = false;
    appSettings->functionsDockVisible = false;
    appSettings->historyDockVisible = false;
    appSettings->keypadVisible = false;
    appSettings->formulaBookDockVisible = false;
    appSettings->variablesDockVisible = false;
    appSettings->userFunctionsDockVisible = false;
    appSettings->userUnitsDockVisible = false;
    appSettings->bitfieldVisible = false;
    appSettings->windowPositionSave = false;
    appSettings->hasNumberFormatStyleSetting = true;

    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    ResultDisplay* display = window.findChild<ResultDisplay*>();
    QVERIFY(display != nullptr);
    QWidget* pane = paneWidgetForDisplay(display);
    QTabBar* tabBar = pane ? pane->findChild<QTabBar*>() : nullptr;
    QVERIFY(tabBar != nullptr);

    Editor* editor = window.findChild<Editor*>();
    QVERIFY(editor != nullptr);
    editor->setText(QStringLiteral("first tab draft"));
    editor->setCursorPosition(5);
    QCoreApplication::processEvents();

    QVERIFY(QMetaObject::invokeMethod(&window, "showNewSessionDialog", Qt::DirectConnection));
    QCoreApplication::processEvents();
    QTRY_COMPARE(tabBar->count(), 2);
    QCOMPARE(tabBar->currentIndex(), 1);

    editor->setText(QStringLiteral("second tab draft"));
    editor->setCursorPosition(6);
    QCoreApplication::processEvents();

    QTest::mouseClick(tabBar, Qt::LeftButton, Qt::NoModifier, tabBar->tabRect(0).center());
    QCoreApplication::processEvents();
    QCOMPARE(editor->text(), QStringLiteral("first tab draft"));

    QTest::mouseClick(tabBar, Qt::LeftButton, Qt::NoModifier, tabBar->tabRect(1).center());
    QCoreApplication::processEvents();
    QCOMPARE(editor->text(), QStringLiteral("second tab draft"));
}

void TestSessionsUi::session_tabs_show_full_name_tooltip_on_hover()
{
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;

    settings->sessionLayoutJson.clear();
    settings->windowState.clear();
    settings->windowGeometry.clear();
    settings->constantsDockVisible = false;
    settings->functionsDockVisible = false;
    settings->historyDockVisible = false;
    settings->keypadMode = Settings::KeypadModeDisabled;
    settings->keypadVisible = false;
    settings->formulaBookDockVisible = false;
    settings->variablesDockVisible = false;
    settings->userFunctionsDockVisible = false;
    settings->userUnitsDockVisible = false;
    settings->bitfieldVisible = false;

    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    ResultDisplay* display = window.findChild<ResultDisplay*>();
    QTabBar* tabBar = tabBarForDisplay(display);
    QVERIFY(tabBar != nullptr);

    QVERIFY(QMetaObject::invokeMethod(&window, "showNewSessionDialog", Qt::DirectConnection));
    QTRY_COMPARE(tabBar->count(), 2);
    QVERIFY(tabBar->isVisible());

    const QString fullName =
        QStringLiteral("A session name that is deliberately much too long to fit "
                       "inside the available tab label area at hover time");
    tabBar->setTabText(0, fullName);
    const QPoint hoverPos = tabBar->tabRect(0).center();
    QMouseEvent moveEvent(QEvent::MouseMove,
                          QPointF(hoverPos),
                          QPointF(tabBar->mapToGlobal(hoverPos)),
                          Qt::NoButton,
                          Qt::NoButton,
                          Qt::NoModifier);
    QCoreApplication::sendEvent(tabBar, &moveEvent);

    QFrame* toolTipPopup =
        tabBar->findChild<QFrame*>(QStringLiteral("sessionTabToolTipPopup"));
    QTRY_VERIFY(toolTipPopup != nullptr && toolTipPopup->isVisible());
    QLabel* toolTipLabel =
        toolTipPopup->findChild<QLabel*>(QStringLiteral("sessionTabToolTipPopupLabel"));
    QVERIFY(toolTipLabel != nullptr);
    QCOMPARE(toolTipLabel->text(), fullName);

    QVERIFY(toolTipPopup->palette().color(QPalette::Window).isValid());
    QVERIFY(toolTipPopup->palette().color(QPalette::WindowText).isValid());
    QVERIFY(toolTipPopup->styleSheet().contains(
        QStringLiteral("border-radius: %1px")
            .arg(UiConfig::CompletionPopupCornerRadius)));

    tabBar->setTabText(0, QStringLiteral("Short"));
    QCoreApplication::sendEvent(tabBar, &moveEvent);
    QTRY_VERIFY(!toolTipPopup->isVisible());
}

void TestSessionsUi::tab_context_reopen_recovers_only_regular_tabs_data()
{
    QTest::addColumn<bool>("privateTab");
    QTest::newRow("regular") << false;
    QTest::newRow("private") << true;
}

void TestSessionsUi::tab_context_reopen_recovers_only_regular_tabs()
{
    QFETCH(bool, privateTab);
    MainWindowStateGuard guard;
    MainWindow window(false);
    window.show();
    window.activateWindow();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    ResultDisplay* display = window.findChild<ResultDisplay*>();
    Editor* editor = editorForDisplay(display);
    QTabBar* tabs = tabBarForDisplay(display);
    QVERIFY(editor && tabs);
    QVERIFY(QMetaObject::invokeMethod(&window, "showNewSessionDialog"));
    QVERIFY(QMetaObject::invokeMethod(&window,
        privateTab ? "showNewPrivateSession" : "showNewSessionDialog"));
    QCOMPARE(tabs->count(), 3);
    editor->setText(QStringLiteral("321"));
    const QString closedName = display->session()->name();
    chooseTabContextAction(tabs, 2, {QStringLiteral("Close Tab")});
    QTRY_COMPARE(tabs->count(), 2);
    bool canReopen = false;
    chooseTabContextAction(tabs, 0, {QStringLiteral("Reopen Closed Tab")},
        [&canReopen](QAction* action) { canReopen = action->isEnabled(); });
    QCOMPARE(canReopen, !privateTab);
    QCOMPARE(tabs->count(), privateTab ? 2 : 3);
    if (!privateTab) {
        QCOMPARE(display->session()->name(), closedName);
        QCOMPARE(editor->text(), QStringLiteral("321"));
    }
    chooseTabContextAction(tabs, 0, {QStringLiteral("Reopen Closed Tab")},
        [](QAction* action) { QVERIFY(!action->isEnabled()); });
}

void TestSessionsUi::tab_context_privacy_targets_clicked_pane_data()
{
    QTest::addColumn<bool>("privateTab");
    QTest::newRow("make-private") << false;
    QTest::newRow("keep") << true;
}

void TestSessionsUi::tab_context_privacy_targets_clicked_pane()
{
    QFETCH(bool, privateTab);
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    MainWindow window(false);
    window.show();
    window.activateWindow();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    ResultDisplay* display = window.findChild<ResultDisplay*>();
    QVERIFY(QMetaObject::invokeMethod(&window,
        privateTab ? "showNewPrivateSession" : "showNewSessionDialog"));
    const Session* target = display->session();
    QTabBar* tabs = tabBarForDisplay(display);
    QVERIFY(tabs);
    Editor* editor = editorForDisplay(display);
    editor->setText(QStringLiteral("123"));
    QVERIFY(QMetaObject::invokeMethod(&window, "evaluateEditorExpression"));
    QVERIFY(QMetaObject::invokeMethod(&window, "splitActivePaneRight"));
    const QList<ResultDisplay*> displays = window.findChildren<ResultDisplay*>();
    QCOMPARE(displays.size(), 2);
    ResultDisplay* other = displays.first() == display ? displays.last() : displays.first();
    const Session* otherSession = other->session();
    editorForDisplay(other)->setText(QStringLiteral("456"));
    QTRY_VERIFY(editorForDisplay(other)->hasFocus());
    chooseTabContextAction(tabs, 1,
        {privateTab ? QStringLiteral("Keep") : QStringLiteral("Make Private...")},
        [&window, privateTab](QAction*) {
            if (!privateTab) {
                QTimer::singleShot(0, &window, []() {
                    auto* confirmation = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
                    QVERIFY(confirmation);
                    confirmation->done(QMessageBox::Yes);
                });
            }
        });
    QCOMPARE(display->session(), target);
    QCOMPARE(target->isPrivate(), !privateTab);
    QCOMPARE(target->historySize(), 1);
    QCOMPARE(QFileInfo::exists(storage.path(target->name())), privateTab);
    QCOMPARE(other->session(), otherSession);
    QVERIFY(!otherSession->isPrivate());
    QCOMPARE(editorForDisplay(other)->text(), QStringLiteral("456"));
}

void TestSessionsUi::tab_context_split_targets_clicked_pane_data()
{
    QTest::addColumn<QString>("direction");
    for (const QString& direction : {QStringLiteral("Left"), QStringLiteral("Right"),
                                    QStringLiteral("Up"), QStringLiteral("Down")})
        QTest::newRow(qPrintable(direction)) << direction;
}

void TestSessionsUi::tab_context_split_targets_clicked_pane()
{
    QFETCH(QString, direction);
    MainWindowStateGuard guard;
    MainWindow window(false);
    window.resize(1000, 700);
    window.show();
    window.activateWindow();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    ResultDisplay* display = window.findChild<ResultDisplay*>();
    Editor* first = editorForDisplay(display);
    QVERIFY(first);
    QVERIFY(QMetaObject::invokeMethod(&window, "showNewSessionDialog"));
    first->setText(QStringLiteral("11"));
    QVERIFY(QMetaObject::invokeMethod(&window, "splitActivePaneRight"));
    const QList<Editor*> originalEditors = window.findChildren<Editor*>();
    QCOMPARE(originalEditors.size(), 2);
    Editor* other = originalEditors.first() == first ? originalEditors.last() : originalEditors.first();
    other->setText(QStringLiteral("22"));
    QTRY_VERIFY(other->hasFocus());
    chooseTabContextAction(tabBarForDisplay(display), 1, {QStringLiteral("Split"), direction});
    QTRY_COMPARE(window.findChildren<Editor*>().size(), 3);
    Editor* added = nullptr;
    for (Editor* editor : window.findChildren<Editor*>()) {
        if (!originalEditors.contains(editor))
            added = editor;
    }
    QVERIFY(added);
    // The offscreen plugin leaves no active window after dismissing a submenu.
    window.activateWindow();
    QTRY_VERIFY(added->hasFocus());
    const QPoint firstPosition = first->mapTo(&window, first->rect().center());
    const QPoint addedPosition = added->mapTo(&window, added->rect().center());
    if (direction == QLatin1String("Left"))
        QVERIFY(addedPosition.x() < firstPosition.x());
    else if (direction == QLatin1String("Right"))
        QVERIFY(addedPosition.x() > firstPosition.x());
    else if (direction == QLatin1String("Up"))
        QVERIFY(addedPosition.y() < firstPosition.y());
    else
        QVERIFY(addedPosition.y() > firstPosition.y());
    QCOMPARE(first->text(), QStringLiteral("11"));
    QCOMPARE(other->text(), QStringLiteral("22"));
}

void TestSessionsUi::tab_context_clear_preserves_other_sessions_data()
{
    QTest::addColumn<bool>("confirm");
    QTest::addColumn<bool>("draftOnly");
    QTest::newRow("cancel") << false << false;
    QTest::newRow("confirm") << true << false;
    QTest::newRow("unfinished-expression") << true << true;
}

void TestSessionsUi::tab_context_clear_preserves_other_sessions()
{
    QFETCH(bool, confirm);
    QFETCH(bool, draftOnly);
    MainWindowStateGuard guard;
    guard.settings->autoCalc = false;
    const QString oldDefinitions = guard.settings->startupUserDefinitions;
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
        guard.settings->startupUserDefinitions = oldDefinitions;
        if (hadDefinitionsFile)
            writeFile(definitionsPath, oldFileContents);
        else
            QFile::remove(definitionsPath);
    });
    guard.settings->startupUserDefinitions = draftOnly ? QString() : QStringLiteral("sharedvalue=99");
    UserDefinitions::saveFrom(guard.settings);
    MainWindow window(false);
    window.show();
    window.activateWindow();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    ResultDisplay* display = window.findChild<ResultDisplay*>();
    Editor* editor = editorForDisplay(display);
    QVERIFY(editor);
    const Session* target = display->session();
    if (!draftOnly)
        QVERIFY(target->hasVariable(QStringLiteral("sharedvalue")));
    if (!draftOnly) {
        for (const QString& expression : {QStringLiteral("localvalue=12"),
                                         QStringLiteral("localfunc(x)=x+1"),
                                         QStringLiteral("[localunit]=[metre]")}) {
            editor->setText(expression);
            QVERIFY(QMetaObject::invokeMethod(&window, "evaluateEditorExpression"));
        }
        QVERIFY(target->hasVariable(QStringLiteral("localvalue")));
        QCOMPARE(target->UserFunctionsToList().size(), 1);
        QCOMPARE(target->userUnitsToList().size(), 1);
    }
    const int historySize = target->historySize();
    editor->setText(QStringLiteral("unfinished +"));
    QVERIFY(QMetaObject::invokeMethod(&window, "showNewSessionDialog"));
    QVERIFY(QMetaObject::invokeMethod(&window, "splitActivePaneRight"));
    const QList<ResultDisplay*> displays = window.findChildren<ResultDisplay*>();
    QCOMPARE(displays.size(), 2);
    ResultDisplay* other = displays.first() == display ? displays.last() : displays.first();
    editorForDisplay(other)->setText(QStringLiteral("456"));
    QTRY_VERIFY(editorForDisplay(other)->hasFocus());
    bool confirmationShown = false;
    chooseTabContextAction(tabBarForDisplay(display), 0, {QStringLiteral("Clear")},
        [&window, confirm, &confirmationShown](QAction*) {
            QTimer::singleShot(0, &window, [confirm, &confirmationShown]() {
                auto* dialog = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
                QVERIFY(dialog);
                confirmationShown = true;
                dialog->done(confirm ? QMessageBox::Yes : QMessageBox::No);
            });
        });
    QVERIFY(confirmationShown);
    QCOMPARE(display->session(), target);
    QCOMPARE(target->historySize(), confirm ? 0 : historySize);
    QCOMPARE(editor->text(), confirm ? QString() : QStringLiteral("unfinished +"));
    if (!draftOnly) {
        QCOMPARE(target->hasVariable(QStringLiteral("localvalue")), !confirm);
        QCOMPARE(target->UserFunctionsToList().size(), confirm ? 0 : 1);
        QCOMPARE(target->userUnitsToList().size(), confirm ? 0 : 1);
        QVERIFY(target->hasVariable(QStringLiteral("sharedvalue")));
    }
    QCOMPARE(editorForDisplay(other)->text(), QStringLiteral("456"));
}

void TestSessionsUi::session_tab_context_menu_opens_on_right_click()
{
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;
    settings->sessionLayoutJson.clear();
    settings->windowState.clear();
    settings->windowGeometry.clear();
    settings->constantsDockVisible = false;
    settings->functionsDockVisible = false;
    settings->historyDockVisible = false;
    settings->keypadMode = Settings::KeypadModeDisabled;
    settings->keypadVisible = false;
    settings->formulaBookDockVisible = false;
    settings->variablesDockVisible = false;
    settings->userFunctionsDockVisible = false;
    settings->userUnitsDockVisible = false;
    settings->bitfieldVisible = false;
    settings->statusBarVisible = false;
    settings->hasNumberFormatStyleSetting = true;

    MainWindow window;
    window.resize(900, 500);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    ResultDisplay* display = window.findChild<ResultDisplay*>();
    QVERIFY(display != nullptr);
    QTabBar* tabBar = tabBarForDisplay(display);
    QVERIFY(tabBar != nullptr);
    QVERIFY(QMetaObject::invokeMethod(&window, "showNewSessionDialog", Qt::DirectConnection));
    QVERIFY(QMetaObject::invokeMethod(&window, "showNewSessionDialog", Qt::DirectConnection));
    QTRY_COMPARE(tabBar->count(), 3);
    tabBar->show();
    QCoreApplication::processEvents();

    bool menuOpened = false;
    QTimer::singleShot(0, &window, [&]() {
        QMenu* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
        if (menu == nullptr)
            return;
        menuOpened = true;
        menu->close();
    });

    const QPoint tabPosition = tabBar->tabRect(0).center();
    QContextMenuEvent contextMenuEvent(QContextMenuEvent::Mouse,
                                       tabPosition,
                                       tabBar->mapToGlobal(tabPosition));
    QCoreApplication::sendEvent(tabBar, &contextMenuEvent);
    QTRY_VERIFY(menuOpened);
}

void TestSessionsUi::session_tab_navigation_shortcuts_switch_tabs()
{
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;

    settings->sessionLayoutJson.clear();
    settings->windowState.clear();
    settings->windowGeometry.clear();
    settings->constantsDockVisible = false;
    settings->functionsDockVisible = false;
    settings->historyDockVisible = false;
    settings->keypadMode = Settings::KeypadModeDisabled;
    settings->keypadVisible = false;
    settings->formulaBookDockVisible = false;
    settings->variablesDockVisible = false;
    settings->userFunctionsDockVisible = false;
    settings->userUnitsDockVisible = false;
    settings->bitfieldVisible = false;
    settings->statusBarVisible = false;
    settings->hasNumberFormatStyleSetting = true;

    MainWindow window;
    window.resize(900, 500);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    ResultDisplay* display = window.findChild<ResultDisplay*>();
    QVERIFY(display != nullptr);
    QTabBar* tabBar = tabBarForDisplay(display);
    QVERIFY(tabBar != nullptr);
    Editor* editor = editorForDisplay(display);
    QVERIFY(editor != nullptr);

    QVERIFY(QMetaObject::invokeMethod(&window, "showNewSessionDialog", Qt::DirectConnection));
    QVERIFY(QMetaObject::invokeMethod(&window, "showNewSessionDialog", Qt::DirectConnection));
    QCoreApplication::processEvents();
    QTRY_COMPARE(tabBar->count(), 3);

    tabBar->setCurrentIndex(0);
    editor->setFocus();
    QTRY_VERIFY(editor->hasFocus());
    QCOMPARE(tabBar->currentIndex(), 0);

    const QKeyCombination next = nextSessionTabShortcut();
    QTest::keyClick(editor, next.key(), next.keyboardModifiers());
    QTRY_COMPARE(tabBar->currentIndex(), 1);

    QTest::keyClick(editor, next.key(), next.keyboardModifiers());
    QTRY_COMPARE(tabBar->currentIndex(), 2);

    const QKeyCombination previous = previousSessionTabShortcut();
    QTest::keyClick(editor, previous.key(), previous.keyboardModifiers());
    QTRY_COMPARE(tabBar->currentIndex(), 1);
}

void TestSessionsUi::new_tab_menu_action_and_shortcut_create_session_in_active_pane()
{
    Settings* appSettings = Settings::instance();
    struct SettingsGuard {
        Settings* settings;
        QByteArray oldSkipUpdateCheck;
        bool hadSkipUpdateCheck;
        QString oldSessionLayoutJson;
        bool oldConstantsDockVisible;
        bool oldFunctionsDockVisible;
        bool oldHistoryDockVisible;
        bool oldKeypadVisible;
        bool oldFormulaBookDockVisible;
        bool oldVariablesDockVisible;
        bool oldUserFunctionsDockVisible;
        bool oldUserUnitsDockVisible;
        bool oldBitfieldVisible;
        bool oldWindowPositionSave;
        bool oldHasNumberFormatStyleSetting;

        ~SettingsGuard()
        {
            if (hadSkipUpdateCheck)
                qputenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK", oldSkipUpdateCheck);
            else
                qunsetenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK");
            settings->sessionLayoutJson = oldSessionLayoutJson;
            settings->constantsDockVisible = oldConstantsDockVisible;
            settings->functionsDockVisible = oldFunctionsDockVisible;
            settings->historyDockVisible = oldHistoryDockVisible;
            settings->keypadVisible = oldKeypadVisible;
            settings->formulaBookDockVisible = oldFormulaBookDockVisible;
            settings->variablesDockVisible = oldVariablesDockVisible;
            settings->userFunctionsDockVisible = oldUserFunctionsDockVisible;
            settings->userUnitsDockVisible = oldUserUnitsDockVisible;
            settings->bitfieldVisible = oldBitfieldVisible;
            settings->windowPositionSave = oldWindowPositionSave;
            settings->hasNumberFormatStyleSetting = oldHasNumberFormatStyleSetting;
        }
    } guard {
        appSettings,
        qgetenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK"),
        qEnvironmentVariableIsSet("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK"),
        appSettings->sessionLayoutJson,
        appSettings->constantsDockVisible,
        appSettings->functionsDockVisible,
        appSettings->historyDockVisible,
        appSettings->keypadVisible,
        appSettings->formulaBookDockVisible,
        appSettings->variablesDockVisible,
        appSettings->userFunctionsDockVisible,
        appSettings->userUnitsDockVisible,
        appSettings->bitfieldVisible,
        appSettings->windowPositionSave,
        appSettings->hasNumberFormatStyleSetting
    };

    qputenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK", "1");
    appSettings->sessionLayoutJson.clear();
    appSettings->constantsDockVisible = false;
    appSettings->functionsDockVisible = false;
    appSettings->historyDockVisible = false;
    appSettings->keypadVisible = false;
    appSettings->formulaBookDockVisible = false;
    appSettings->variablesDockVisible = false;
    appSettings->userFunctionsDockVisible = false;
    appSettings->userUnitsDockVisible = false;
    appSettings->bitfieldVisible = false;
    appSettings->windowPositionSave = false;
    appSettings->hasNumberFormatStyleSetting = true;

    MainWindow window;
    window.resize(900, 500);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QVERIFY(QMetaObject::invokeMethod(&window, "splitActivePaneRight", Qt::DirectConnection));
    QCoreApplication::processEvents();

    const QList<ResultDisplay*> displays = window.findChildren<ResultDisplay*>();
    QCOMPARE(displays.size(), 2);
    ResultDisplay* firstDisplay = displays.at(0);
    ResultDisplay* secondDisplay = displays.at(1);
    QWidget* firstPane = paneWidgetForDisplay(firstDisplay);
    QWidget* secondPane = paneWidgetForDisplay(secondDisplay);
    QTabBar* firstTabBar = firstPane ? firstPane->findChild<QTabBar*>() : nullptr;
    QTabBar* secondTabBar = secondPane ? secondPane->findChild<QTabBar*>() : nullptr;
    QVERIFY(firstTabBar != nullptr);
    QVERIFY(secondTabBar != nullptr);
    QCOMPARE(firstTabBar->count(), 1);
    QCOMPARE(secondTabBar->count(), 1);

    QTest::mouseClick(secondDisplay->viewport(), Qt::LeftButton, Qt::NoModifier,
                      secondDisplay->viewport()->rect().center());
    QCoreApplication::processEvents();

    QMenu* sessionMenu = menuWithTitle(window.menuBar(), QStringLiteral("&Session"));
    QVERIFY(sessionMenu != nullptr);
    QAction* newTabAction = directMenuActionWithText(sessionMenu, QStringLiteral("New &Tab"));
    QVERIFY(newTabAction != nullptr);
    QVERIFY(directMenuActionWithText(sessionMenu, QStringLiteral("New Session")) == nullptr);

    newTabAction->trigger();
    QCoreApplication::processEvents();

    QCOMPARE(firstTabBar->count(), 1);
    QTRY_COMPARE(secondTabBar->count(), 2);
    QCOMPARE(secondTabBar->currentIndex(), 1);

    const QList<QKeySequence> bindings = QKeySequence::keyBindings(QKeySequence::AddTab);
    QVERIFY(!bindings.isEmpty());
    QVERIFY(newTabAction->shortcuts().contains(bindings.first()));
    const QKeyCombination shortcut = bindings.first()[0];
    QTest::keyClick(&window, shortcut.key(), shortcut.keyboardModifiers());
    QCoreApplication::processEvents();

    QCOMPARE(firstTabBar->count(), 1);
    QTRY_COMPARE(secondTabBar->count(), 3);
    QCOMPARE(secondTabBar->currentIndex(), 2);
}

void TestSessionsUi::new_tab_menu_action_targets_focused_window_when_native_menu_uses_last_window_action()
{
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;

    settings->sessionLayoutJson.clear();
    settings->windowState.clear();
    settings->windowGeometry.clear();
    settings->constantsDockVisible = false;
    settings->functionsDockVisible = false;
    settings->historyDockVisible = false;
    settings->keypadVisible = false;
    settings->formulaBookDockVisible = false;
    settings->variablesDockVisible = false;
    settings->userFunctionsDockVisible = false;
    settings->userUnitsDockVisible = false;
    settings->bitfieldVisible = false;
    settings->statusBarVisible = true;
    settings->windowPositionSave = false;
    settings->hasNumberFormatStyleSetting = true;

    MainWindow firstWindow;
    firstWindow.resize(800, 500);
    firstWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&firstWindow));

    MainWindow secondWindow;
    secondWindow.resize(800, 500);
    secondWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&secondWindow));

    ResultDisplay* firstDisplay = firstWindow.findChild<ResultDisplay*>();
    ResultDisplay* secondDisplay = secondWindow.findChild<ResultDisplay*>();
    QVERIFY(firstDisplay != nullptr);
    QVERIFY(secondDisplay != nullptr);
    Editor* firstEditor = editorForDisplay(firstDisplay);
    Editor* secondEditor = editorForDisplay(secondDisplay);
    QVERIFY(firstEditor != nullptr);
    QVERIFY(secondEditor != nullptr);
    QTabBar* firstTabBar = tabBarForDisplay(firstDisplay);
    QTabBar* secondTabBar = tabBarForDisplay(secondDisplay);
    QVERIFY(firstTabBar != nullptr);
    QVERIFY(secondTabBar != nullptr);
    QCOMPARE(firstTabBar->count(), 1);
    QCOMPARE(secondTabBar->count(), 1);

    QTest::mouseClick(secondDisplay->viewport(), Qt::LeftButton, Qt::NoModifier,
                      secondDisplay->viewport()->rect().center());
    QCoreApplication::processEvents();

    QEvent secondWindowDeactivate(QEvent::WindowDeactivate);
    QCoreApplication::sendEvent(&secondWindow, &secondWindowDeactivate);
    QEvent firstWindowActivate(QEvent::WindowActivate);
    QCoreApplication::sendEvent(&firstWindow, &firstWindowActivate);
    QCoreApplication::processEvents();

    QVERIFY(QMetaObject::invokeMethod(&firstWindow, "showNewSessionDialog", Qt::DirectConnection));
    QCoreApplication::processEvents();

    QTRY_COMPARE(firstTabBar->count(), 2);
    QCOMPARE(secondTabBar->count(), 1);

    firstWindow.raise();
    firstWindow.activateWindow();
    firstEditor->setFocus(Qt::OtherFocusReason);
    QTRY_VERIFY(([firstEditor]() {
        QWidget* focused = QApplication::focusWidget();
        return focused == firstEditor
            || (focused != nullptr && focused->parentWidget() == firstEditor);
    }()));

    QMenu* secondSessionMenu = menuWithTitle(secondWindow.menuBar(), QStringLiteral("&Session"));
    QVERIFY(secondSessionMenu != nullptr);
    QAction* secondWindowNewTabAction =
        directMenuActionWithText(secondSessionMenu, QStringLiteral("New &Tab"));
    QVERIFY(secondWindowNewTabAction != nullptr);
    secondWindowNewTabAction->trigger();
    QCoreApplication::processEvents();

    QTRY_COMPARE(firstTabBar->count(), 3);
    QCOMPARE(secondTabBar->count(), 1);
}

void TestSessionsUi::new_session_window_menu_action_copies_layout_with_single_fresh_session()
{
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;

    settings->sessionLayoutJson.clear();
    settings->windowState.clear();
    settings->windowGeometry.clear();
    settings->constantsDockVisible = true;
    settings->functionsDockVisible = false;
    settings->historyDockVisible = false;
    settings->keypadMode = Settings::KeypadModeBasicWide;
    settings->keypadVisible = true;
    settings->formulaBookDockVisible = false;
    settings->variablesDockVisible = false;
    settings->userFunctionsDockVisible = false;
    settings->userUnitsDockVisible = false;
    settings->bitfieldVisible = true;
    settings->statusBarVisible = true;
    settings->windowPositionSave = false;
    settings->hasNumberFormatStyleSetting = true;

    MainWindow window;
    window.resize(1000, 700);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    ResultDisplay* sourceDisplay = window.findChild<ResultDisplay*>();
    QVERIFY(sourceDisplay != nullptr);
    QVERIFY(sourceDisplay->session() != nullptr);
    const QString sourceSessionName = sourceDisplay->session()->name();
    Editor* sourceEditor = editorForDisplay(sourceDisplay);
    QVERIFY(sourceEditor != nullptr);
    sourceEditor->setText(QStringLiteral("2+2"));
    QVERIFY(QMetaObject::invokeMethod(&window, "evaluateEditorExpression", Qt::DirectConnection));
    QCOMPARE(sourceDisplay->session()->historySize(), 1);

    QVERIFY(QMetaObject::invokeMethod(&window, "splitActivePaneRight", Qt::DirectConnection));
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.findChildren<ResultDisplay*>().size(), 2);

    QMenu* sessionMenu = menuWithTitle(window.menuBar(), QStringLiteral("&Session"));
    QVERIFY(sessionMenu != nullptr);
    QAction* newWindowAction =
        directMenuActionWithText(sessionMenu, QStringLiteral("New &Window"));
    QVERIFY(newWindowAction != nullptr);
    QVERIFY(directMenuActionWithText(sessionMenu, QStringLiteral("New &Window with New Session")) == nullptr);

    const QList<MainWindow*> windowsBefore = topLevelMainWindows();
    QPointer<MainWindow> createdWindow;
    newWindowAction->trigger();
    QTRY_VERIFY(([&]() {
        for (MainWindow* candidate : topLevelMainWindows()) {
            if (!windowsBefore.contains(candidate) && candidate->isVisible()) {
                createdWindow = candidate;
                return true;
            }
        }
        return false;
    }()));

    QTest::qWait(300);
    QCoreApplication::processEvents();

    const QList<ResultDisplay*> createdDisplays = createdWindow->findChildren<ResultDisplay*>();
    QCOMPARE(createdDisplays.size(), 1);
    QDockWidget* constantsDock =
        createdWindow->findChild<QDockWidget*>(QStringLiteral("ConstantsDock"));
    QVERIFY(constantsDock != nullptr);
    QVERIFY(constantsDock->isVisible());
    QDockWidget* bitfieldDock =
        createdWindow->findChild<QDockWidget*>(QStringLiteral("BitfieldDock"));
    QVERIFY(bitfieldDock != nullptr);
    QVERIFY(bitfieldDock->isVisible());
    QStatusBar* statusBar =
        createdWindow->findChild<QStatusBar*>(QString(), Qt::FindDirectChildrenOnly);
    QVERIFY(statusBar != nullptr);
    QVERIFY(statusBar->isVisible());
    QVERIFY(createdWindow->findChild<Keypad*>() != nullptr);

    ResultDisplay* createdDisplay = createdDisplays.constFirst();
    const Session* session = createdDisplay->session();
    QVERIFY(session != nullptr);
    QCOMPARE(session->historySize(), 0);
    QVERIFY(session->name() != sourceSessionName);

    QTabBar* createdTabBar = tabBarForDisplay(createdDisplay);
    QVERIFY(createdTabBar != nullptr);
    QCOMPARE(createdTabBar->count(), 1);
    QCOMPARE(createdTabBar->tabText(0), session->name());

    createdWindow->close();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QTRY_VERIFY(createdWindow == nullptr);
}

void TestSessionsUi::session_open_menu_action_uses_open_dialog()
{
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;

    settings->sessionLayoutJson.clear();
    settings->windowState.clear();
    settings->windowGeometry.clear();
    settings->constantsDockVisible = false;
    settings->functionsDockVisible = false;
    settings->historyDockVisible = false;
    settings->keypadVisible = false;
    settings->formulaBookDockVisible = false;
    settings->variablesDockVisible = false;
    settings->userFunctionsDockVisible = false;
    settings->userUnitsDockVisible = false;
    settings->bitfieldVisible = false;
    settings->hasNumberFormatStyleSetting = true;

    MainWindow window;
    window.resize(900, 500);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QMenu* sessionMenu = menuWithTitle(window.menuBar(), QStringLiteral("&Session"));
    QVERIFY(sessionMenu != nullptr);
    QAction* openAction = directMenuActionWithText(sessionMenu, QStringLiteral("&Open..."));
    QVERIFY(openAction != nullptr);
    const QList<QKeySequence> openKeyBindings = QKeySequence::keyBindings(QKeySequence::Open);
    QVERIFY(!openKeyBindings.isEmpty());
    QVERIFY(directMenuActionWithText(sessionMenu, QStringLiteral("&Load...")) == nullptr);

    bool shortcutOpenedDialog = false;
    QTimer::singleShot(0, &window, [&shortcutOpenedDialog]() {
        shortcutOpenedDialog = rejectActiveDialogWithTitle(QStringLiteral("Open Session"));
    });
    const QKeyCombination shortcut = openKeyBindings.constFirst()[0];
    QTest::keyClick(&window, shortcut.key(), shortcut.keyboardModifiers());
    QVERIFY(shortcutOpenedDialog);

    bool menuActionOpenedDialog = false;
    QTimer::singleShot(0, &window, [&menuActionOpenedDialog]() {
        menuActionOpenedDialog = rejectActiveDialogWithTitle(QStringLiteral("Open Session"));
    });
    openAction->trigger();
    QVERIFY(menuActionOpenedDialog);
}

void TestSessionsUi::session_open_sessions_folder_menu_action_opens_session_storage()
{
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;

    settings->sessionLayoutJson.clear();
    settings->windowState.clear();
    settings->windowGeometry.clear();
    settings->constantsDockVisible = false;
    settings->functionsDockVisible = false;
    settings->historyDockVisible = false;
    settings->keypadVisible = false;
    settings->formulaBookDockVisible = false;
    settings->variablesDockVisible = false;
    settings->userFunctionsDockVisible = false;
    settings->userUnitsDockVisible = false;
    settings->bitfieldVisible = false;
    settings->hasNumberFormatStyleSetting = true;

    MainWindow window;
    window.resize(900, 500);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QMenu* sessionMenu = menuWithTitle(window.menuBar(), QStringLiteral("&Session"));
    QVERIFY(sessionMenu != nullptr);
    QAction* openSessionsFolderAction =
        directMenuActionWithText(sessionMenu, QStringLiteral("Open Sessions &Folder"));
    QVERIFY(openSessionsFolderAction != nullptr);

    m_capturedUrl = QUrl();
    QDesktopServices::setUrlHandler(QStringLiteral("file"), this, "captureOpenedUrl");
    auto urlHandlerCleanup = qScopeGuard([]() {
        QDesktopServices::unsetUrlHandler(QStringLiteral("file"));
    });

    openSessionsFolderAction->trigger();

    QTRY_VERIFY(m_capturedUrl.isValid());
    QCOMPARE(m_capturedUrl.scheme(), QStringLiteral("file"));

    const QString expectedPath =
        QDir(Settings::getDataPath()).filePath(QStringLiteral("sessions"));
    QCOMPARE(QDir::cleanPath(m_capturedUrl.toLocalFile()), QDir::cleanPath(expectedPath));
    QVERIFY(QDir(expectedPath).exists());
}

void TestSessionsUi::session_import_dialog_opens_valid_json_as_new_tab()
{
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;

    settings->sessionLayoutJson.clear();
    settings->windowState.clear();
    settings->windowGeometry.clear();
    settings->constantsDockVisible = false;
    settings->functionsDockVisible = false;
    settings->historyDockVisible = false;
    settings->keypadMode = Settings::KeypadModeDisabled;
    settings->keypadVisible = false;
    settings->formulaBookDockVisible = false;
    settings->variablesDockVisible = false;
    settings->userFunctionsDockVisible = false;
    settings->userUnitsDockVisible = false;
    settings->bitfieldVisible = false;
    settings->statusBarVisible = false;
    settings->hasNumberFormatStyleSetting = true;

    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString importedName =
        QStringLiteral("Imported Session %1").arg(QUuid::createUuid().toString(QUuid::Id128));
    const QString importFilePath = QDir(temporaryDirectory.path()).filePath(QStringLiteral("imported.json"));
    writeFile(importFilePath, QJsonDocument(sessionJson(importedName)).toJson(QJsonDocument::Compact));
    const QString savedImportPath = QDir(QDir(Settings::getDataPath()).filePath(QStringLiteral("sessions")))
        .filePath(importedName + QStringLiteral(".json"));
    auto savedImportCleanup = qScopeGuard([savedImportPath]() {
        QFile::remove(savedImportPath);
    });

    MainWindow window;
    window.resize(900, 500);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    ResultDisplay* display = window.findChild<ResultDisplay*>();
    QVERIFY(display != nullptr);
    QTabBar* tabBar = tabBarForDisplay(display);
    QVERIFY(tabBar != nullptr);
    QCOMPARE(tabBar->count(), 1);

    QMenu* sessionMenu = menuWithTitle(window.menuBar(), QStringLiteral("&Session"));
    QVERIFY(sessionMenu != nullptr);
    QAction* importAction = directMenuActionWithText(sessionMenu, QStringLiteral("&Import..."));
    QVERIFY(importAction != nullptr);

    bool sawImportDialog = false;
    QTimer::singleShot(0, &window, [&sawImportDialog, &importFilePath]() {
        QFileDialog* dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget());
        if (dialog == nullptr)
            return;

        sawImportDialog = dialog->windowTitle() == QStringLiteral("Import Session")
            && dialog->acceptMode() == QFileDialog::AcceptOpen
            && dialog->fileMode() == QFileDialog::ExistingFile
            && dialog->defaultSuffix() == QStringLiteral("json");
        dialog->selectFile(importFilePath);
        static_cast<QDialog*>(dialog)->accept();
    });
    importAction->trigger();
    QVERIFY(sawImportDialog);

    QTRY_COMPARE(tabBar->count(), 2);
    QCOMPARE(tabBar->tabText(tabBar->currentIndex()), importedName);
    QVERIFY(display->session() != nullptr);
    QCOMPARE(display->session()->name(), importedName);
    QCOMPARE(display->session()->historySize(), 1);
    QCOMPARE(display->session()->historyEntryAtRef(0).expr(), QStringLiteral("6*7"));
}

void TestSessionsUi::session_import_rejects_invalid_json_without_new_tab()
{
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;

    settings->sessionLayoutJson.clear();
    settings->windowState.clear();
    settings->windowGeometry.clear();
    settings->constantsDockVisible = false;
    settings->functionsDockVisible = false;
    settings->historyDockVisible = false;
    settings->keypadMode = Settings::KeypadModeDisabled;
    settings->keypadVisible = false;
    settings->formulaBookDockVisible = false;
    settings->variablesDockVisible = false;
    settings->userFunctionsDockVisible = false;
    settings->userUnitsDockVisible = false;
    settings->bitfieldVisible = false;
    settings->statusBarVisible = false;
    settings->hasNumberFormatStyleSetting = true;

    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString importFilePath = QDir(temporaryDirectory.path()).filePath(QStringLiteral("not-session.json"));
    writeFile(importFilePath, QByteArray("1+1\n"));

    MainWindow window;
    window.resize(900, 500);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    ResultDisplay* display = window.findChild<ResultDisplay*>();
    QVERIFY(display != nullptr);
    QTabBar* tabBar = tabBarForDisplay(display);
    QVERIFY(tabBar != nullptr);
    QCOMPARE(tabBar->count(), 1);

    QMenu* sessionMenu = menuWithTitle(window.menuBar(), QStringLiteral("&Session"));
    QVERIFY(sessionMenu != nullptr);
    QAction* importAction = directMenuActionWithText(sessionMenu, QStringLiteral("&Import..."));
    QVERIFY(importAction != nullptr);

    const bool nativeDialogsDisabled = QApplication::testAttribute(Qt::AA_DontUseNativeDialogs);
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    const auto restoreDialogMode = qScopeGuard([nativeDialogsDisabled]() {
        QApplication::setAttribute(Qt::AA_DontUseNativeDialogs, nativeDialogsDisabled);
    });

    bool sawImportDialog = false;
    bool sawImportError = false;
    QElapsedTimer dialogWait;
    dialogWait.start();
    QTimer dialogDriver;
    connect(&dialogDriver, &QTimer::timeout, &window, [&]() {
        QDialog* modal = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        if (dialogWait.elapsed() > 5000) {
            if (modal != nullptr)
                modal->reject();
            return;
        }
        if (auto* dialog = qobject_cast<QFileDialog*>(modal)) {
            if (!sawImportDialog) {
                sawImportDialog = dialog->windowTitle() == QStringLiteral("Import Session")
                    && dialog->defaultSuffix() == QStringLiteral("json");
                if (QLineEdit* fileNameEdit = dialog->findChild<QLineEdit*>(QStringLiteral("fileNameEdit")))
                    fileNameEdit->setText(importFilePath);
            }
            static_cast<QDialog*>(dialog)->accept();
        } else if (auto* messageBox = qobject_cast<QMessageBox*>(modal)) {
            sawImportError = messageBox->text().contains(QStringLiteral("not valid JSON"));
            messageBox->accept();
        }
    });
    dialogDriver.start(20);
    importAction->trigger();
    dialogDriver.stop();
    QVERIFY(sawImportDialog);
    QVERIFY(sawImportError);
    QCOMPARE(tabBar->count(), 1);
}

void TestSessionsUi::session_export_json_action_opens_dialog()
{
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;

    settings->sessionLayoutJson.clear();
    settings->windowState.clear();
    settings->windowGeometry.clear();
    settings->constantsDockVisible = false;
    settings->functionsDockVisible = false;
    settings->historyDockVisible = false;
    settings->keypadVisible = false;
    settings->formulaBookDockVisible = false;
    settings->variablesDockVisible = false;
    settings->userFunctionsDockVisible = false;
    settings->userUnitsDockVisible = false;
    settings->bitfieldVisible = false;
    settings->hasNumberFormatStyleSetting = true;

    MainWindow window;
    window.resize(900, 500);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QMenu* sessionMenu = menuWithTitle(window.menuBar(), QStringLiteral("&Session"));
    QVERIFY(sessionMenu != nullptr);

    QMenu* exportMenu = directSubmenuWithTitle(sessionMenu, QStringLiteral("&Export"));
    QVERIFY(exportMenu != nullptr);
    QAction* jsonAction = directMenuActionWithText(exportMenu, QStringLiteral("JSON"));
    QVERIFY(jsonAction != nullptr);

    bool sawJsonDialog = false;
    QTimer::singleShot(0, &window, [&sawJsonDialog]() {
        QFileDialog* dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget());
        if (dialog == nullptr)
            return;

        const QStringList files = dialog->selectedFiles();
        const QString selectedFile = files.isEmpty() ? QString() : files.constFirst();
        const QFileInfo fileInfo(selectedFile);
        sawJsonDialog = dialog->windowTitle() == QStringLiteral("Export session as JSON")
            && dialog->defaultSuffix() == QStringLiteral("json")
            && fileInfo.suffix() == QStringLiteral("json")
            && QDir::cleanPath(fileInfo.absolutePath()) == QDir::cleanPath(QDir::homePath());
        dialog->reject();
    });
    jsonAction->trigger();
    QVERIFY(sawJsonDialog);
}

void TestSessionsUi::session_export_dialog_prefills_session_name_data()
{
    QTest::addColumn<QString>("exportSlot");
    QTest::addColumn<QString>("extension");
    QTest::addColumn<QString>("sessionName");
    QTest::addColumn<QString>("baseName");

    for (const auto& format : {qMakePair(QStringLiteral("exportJson"), QStringLiteral("json")),
                               qMakePair(QStringLiteral("exportHtml"), QStringLiteral("html")),
                               qMakePair(QStringLiteral("exportPlainText"), QStringLiteral("txt"))}) {
        QTest::newRow(qPrintable(format.second + QStringLiteral("-named")))
            << format.first << format.second << QStringLiteral("Project notes") << QStringLiteral("Project notes");
        QTest::newRow(qPrintable(format.second + QStringLiteral("-sanitized")))
            << format.first << format.second << QStringLiteral("Budget/2026: Q4") << QStringLiteral("Budget_2026_ Q4");
    }
}

void TestSessionsUi::session_export_dialog_prefills_session_name()
{
    QFETCH(QString, exportSlot);
    QFETCH(QString, extension);
    QFETCH(QString, sessionName);
    QFETCH(QString, baseName);

    MainWindowStateGuard guard;
    MainWindow window(false);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    ResultDisplay* display = window.findChild<ResultDisplay*>();
    QVERIFY(display != nullptr);
    Session* session = const_cast<Session*>(display->session());
    QVERIFY(session != nullptr);
    const QString originalName = session->name();
    const auto restoreName = qScopeGuard([session, originalName]() { session->setName(originalName); });
    session->setName(sessionName);

    bool sawDialog = false;
    QString suggestedFileName;
    QString defaultSuffix;
    QString selectedName;
    QFileDialog::AcceptMode acceptMode = QFileDialog::AcceptOpen;
    QTimer::singleShot(0, &window, [&]() {
        QFileDialog* dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget());
        if (dialog == nullptr)
            return;

        // Let the export dialog's filename-selection timer run first.
        QTimer::singleShot(0, dialog, [&, dialog]() {
            sawDialog = true;
            const QStringList files = dialog->selectedFiles();
            if (!files.isEmpty())
                suggestedFileName = QFileInfo(files.constFirst()).fileName();
            defaultSuffix = dialog->defaultSuffix();
            acceptMode = dialog->acceptMode();
            if (QLineEdit* fileNameEdit = dialog->findChild<QLineEdit*>(QStringLiteral("fileNameEdit")))
                selectedName = fileNameEdit->selectedText();
            dialog->reject();
        });
    });
    QVERIFY(QMetaObject::invokeMethod(&window, qPrintable(exportSlot), Qt::DirectConnection));
    QVERIFY(sawDialog);
    QCOMPARE(suggestedFileName, baseName + QLatin1Char('.') + extension);
    QCOMPARE(defaultSuffix, extension);
    QCOMPARE(acceptMode, QFileDialog::AcceptSave);
    QCOMPARE(selectedName, baseName);
}

void TestSessionsUi::close_and_reopen_commands_preserve_regular_tabs_data()
{
    QTest::addColumn<bool>("privateTab");
    QTest::addColumn<bool>("shortcut");
    QTest::addColumn<bool>("foreignMenu");
    QTest::newRow("regular-menu") << false << false << false;
    QTest::newRow("regular-shortcut") << false << true << false;
    QTest::newRow("private-menu") << true << false << false;
    QTest::newRow("private-shortcut") << true << true << false;
    QTest::newRow("regular-other-window") << false << false << true;
    QTest::newRow("private-other-window") << true << false << true;
}

void TestSessionsUi::close_and_reopen_commands_preserve_regular_tabs()
{
    QFETCH(bool, privateTab);
    QFETCH(bool, shortcut);
    QFETCH(bool, foreignMenu);
    MainWindowStateGuard guard;
    MainWindow window(false);
    MainWindow other(false);
    other.show();
    window.show();
    window.activateWindow();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    ResultDisplay* display = window.findChild<ResultDisplay*>();
    Editor* editor = editorForDisplay(display);
    QTabBar* tabs = tabBarForDisplay(display);
    QVERIFY(editor && tabs);
    QMenu* menu = menuWithTitle((foreignMenu ? other : window).menuBar(), QStringLiteral("&Session"));
    QVERIFY(menu);
    QAction* close = directMenuActionWithText(menu, QStringLiteral("&Close Tab"));
    QAction* reopen = directMenuActionWithText(menu, QStringLiteral("Reopen Clo&sed Tab"));
    QVERIFY(close && reopen);
    QVERIFY(!reopen->isEnabled());
    QVERIFY(QMetaObject::invokeMethod(&window,
        privateTab ? "showNewPrivateSession" : "showNewSessionDialog", Qt::DirectConnection));
    QTRY_COMPARE(tabs->count(), 2);
    editor->setFocus();
    QTRY_VERIFY(editor->hasFocus());
    editor->setText(QStringLiteral("321"));
    const QString name = display->session()->name();
    if (shortcut) {
        const QKeyCombination key = QKeySequence(QKeySequence::Close)[0];
        QTest::keyClick(&window, key.key(), key.keyboardModifiers());
    } else {
        close->trigger();
    }
    QTRY_COMPARE(tabs->count(), 1);
    QCOMPARE(reopen->isEnabled(), !privateTab);
    other.activateWindow();
    other.findChild<Editor*>()->setFocus();
    QTRY_VERIFY(other.findChild<Editor*>()->hasFocus());
    QVERIFY(!reopen->isEnabled());
    window.activateWindow();
    editor->setFocus();
    QTRY_VERIFY(editor->hasFocus());
    QCOMPARE(reopen->isEnabled(), !privateTab);
    if (shortcut) {
        const QKeyCombination key = restoreClosedTabShortcut();
        QTest::keyClick(&window, key.key(), key.keyboardModifiers());
    } else if (foreignMenu) {
        reopen->trigger();
    } else if (!privateTab) {
        // Exercise a real popup dismissal before the queued restore runs.
        menu->popup(window.mapToGlobal(QPoint(20, 20)));
        menu->setActiveAction(reopen);
        QTest::keyClick(menu, Qt::Key_Return);
    }
    QTRY_COMPARE(tabs->count(), privateTab ? 1 : 2);
    if (!privateTab) {
        QCOMPARE(display->session()->name(), name);
        QCOMPARE(editor->text(), QStringLiteral("321"));
    }
    QVERIFY(!reopen->isEnabled());
}

void TestSessionsUi::view_commands_split_and_close_active_pane_data()
{
    QTest::addColumn<QString>("direction");
    for (const QString& direction : {QStringLiteral("&Left"), QStringLiteral("&Right"),
                                    QStringLiteral("&Up"), QStringLiteral("&Down")})
        QTest::newRow(qPrintable(direction)) << direction;
}

void TestSessionsUi::view_commands_split_and_close_active_pane()
{
    QFETCH(QString, direction);
    MainWindowStateGuard guard;
    MainWindow window(false);
    window.resize(900, 600);
    window.show();
    window.activateWindow();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    Editor* first = window.findChild<Editor*>();
    QVERIFY(first);
    first->setFocus();
    QTRY_VERIFY(first->hasFocus());
    first->setText(QStringLiteral("11"));
    QMenu* view = menuWithTitle(window.menuBar(), QStringLiteral("&View"));
    QVERIFY(view);
    QMenu* split = directSubmenuWithTitle(view, QStringLiteral("S&plit"));
    QVERIFY(split);
    QAction* action = directMenuActionWithText(split, direction);
    QAction* close = directMenuActionWithText(view, QStringLiteral("Close Pa&ne"));
    QVERIFY(action && close);
    action->trigger();
    QTRY_COMPARE(window.findChildren<Editor*>().size(), 2);
    const QList<Editor*> editors = window.findChildren<Editor*>();
    Editor* added = editors.first() == first ? editors.last() : editors.first();
    QTRY_VERIFY(added->hasFocus());
    const QPoint firstPosition = first->mapTo(&window, first->rect().center());
    const QPoint addedPosition = added->mapTo(&window, added->rect().center());
    if (direction == QLatin1String("&Left"))
        QVERIFY(addedPosition.x() < firstPosition.x());
    else if (direction == QLatin1String("&Right"))
        QVERIFY(addedPosition.x() > firstPosition.x());
    else if (direction == QLatin1String("&Up"))
        QVERIFY(addedPosition.y() < firstPosition.y());
    else
        QVERIFY(addedPosition.y() > firstPosition.y());
    added->setText(QStringLiteral("22"));
    close->trigger();
    QTRY_COMPARE(window.findChildren<Editor*>().size(), 1);
    QCOMPARE(window.findChild<Editor*>(), first);
    QCOMPARE(first->text(), QStringLiteral("11"));
}

void TestSessionsUi::restore_closed_tab_shortcut_restores_last_closed_session_tab()
{
    Settings* appSettings = Settings::instance();
    struct SettingsGuard {
        Settings* settings;
        QByteArray oldSkipUpdateCheck;
        bool hadSkipUpdateCheck;
        QString oldSessionLayoutJson;
        bool oldConstantsDockVisible;
        bool oldFunctionsDockVisible;
        bool oldHistoryDockVisible;
        bool oldKeypadVisible;
        bool oldFormulaBookDockVisible;
        bool oldVariablesDockVisible;
        bool oldUserFunctionsDockVisible;
        bool oldUserUnitsDockVisible;
        bool oldBitfieldVisible;
        bool oldWindowPositionSave;
        bool oldHasNumberFormatStyleSetting;

        ~SettingsGuard()
        {
            if (hadSkipUpdateCheck)
                qputenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK", oldSkipUpdateCheck);
            else
                qunsetenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK");
            settings->sessionLayoutJson = oldSessionLayoutJson;
            settings->constantsDockVisible = oldConstantsDockVisible;
            settings->functionsDockVisible = oldFunctionsDockVisible;
            settings->historyDockVisible = oldHistoryDockVisible;
            settings->keypadVisible = oldKeypadVisible;
            settings->formulaBookDockVisible = oldFormulaBookDockVisible;
            settings->variablesDockVisible = oldVariablesDockVisible;
            settings->userFunctionsDockVisible = oldUserFunctionsDockVisible;
            settings->userUnitsDockVisible = oldUserUnitsDockVisible;
            settings->bitfieldVisible = oldBitfieldVisible;
            settings->windowPositionSave = oldWindowPositionSave;
            settings->hasNumberFormatStyleSetting = oldHasNumberFormatStyleSetting;
        }
    } guard {
        appSettings,
        qgetenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK"),
        qEnvironmentVariableIsSet("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK"),
        appSettings->sessionLayoutJson,
        appSettings->constantsDockVisible,
        appSettings->functionsDockVisible,
        appSettings->historyDockVisible,
        appSettings->keypadVisible,
        appSettings->formulaBookDockVisible,
        appSettings->variablesDockVisible,
        appSettings->userFunctionsDockVisible,
        appSettings->userUnitsDockVisible,
        appSettings->bitfieldVisible,
        appSettings->windowPositionSave,
        appSettings->hasNumberFormatStyleSetting
    };

    qputenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK", "1");
    appSettings->sessionLayoutJson.clear();
    appSettings->constantsDockVisible = false;
    appSettings->functionsDockVisible = false;
    appSettings->historyDockVisible = false;
    appSettings->keypadVisible = false;
    appSettings->formulaBookDockVisible = false;
    appSettings->variablesDockVisible = false;
    appSettings->userFunctionsDockVisible = false;
    appSettings->userUnitsDockVisible = false;
    appSettings->bitfieldVisible = false;
    appSettings->windowPositionSave = false;
    appSettings->hasNumberFormatStyleSetting = true;

    MainWindow window;
    window.resize(900, 500);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    ResultDisplay* display = window.findChild<ResultDisplay*>();
    QVERIFY(display != nullptr);
    QWidget* pane = paneWidgetForDisplay(display);
    QTabBar* tabBar = pane ? pane->findChild<QTabBar*>() : nullptr;
    QVERIFY(tabBar != nullptr);
    Editor* editor = window.findChild<Editor*>();
    QVERIFY(editor != nullptr);

    const QKeyCombination shortcut = restoreClosedTabShortcut();
    QTest::keyClick(&window, shortcut.key(), shortcut.keyboardModifiers());
    QCoreApplication::processEvents();
    QCOMPARE(tabBar->count(), 1);

    editor->setText(QStringLiteral("first tab draft"));
    editor->setCursorPosition(editor->text().size());
    QCoreApplication::processEvents();

    QVERIFY(QMetaObject::invokeMethod(&window, "showNewSessionDialog", Qt::DirectConnection));
    QCoreApplication::processEvents();
    QTRY_COMPARE(tabBar->count(), 2);
    QCOMPARE(tabBar->currentIndex(), 1);

    editor->setText(QStringLiteral("second tab draft"));
    editor->setCursorPosition(editor->text().size());
    QCoreApplication::processEvents();

    QVERIFY(QMetaObject::invokeMethod(&window, "closeCurrentSession", Qt::DirectConnection));
    QCoreApplication::processEvents();
    QTRY_COMPARE(tabBar->count(), 1);
    QCOMPARE(editor->text(), QStringLiteral("first tab draft"));

    QTest::keyClick(&window, shortcut.key(), shortcut.keyboardModifiers());
    QCoreApplication::processEvents();
    QTRY_COMPARE(tabBar->count(), 2);
    QCOMPARE(tabBar->currentIndex(), 1);
    QCOMPARE(editor->text(), QStringLiteral("second tab draft"));

    QTest::keyClick(&window, shortcut.key(), shortcut.keyboardModifiers());
    QCoreApplication::processEvents();
    QCOMPARE(tabBar->count(), 2);
    QCOMPARE(tabBar->currentIndex(), 1);
    QCOMPARE(editor->text(), QStringLiteral("second tab draft"));
}

void TestSessionsUi::session_tabs_reorder_with_horizontal_drag()
{
    Settings* appSettings = Settings::instance();
    struct SettingsGuard {
        Settings* settings;
        QByteArray oldSkipUpdateCheck;
        bool hadSkipUpdateCheck;
        QString oldSessionLayoutJson;
        bool oldConstantsDockVisible;
        bool oldFunctionsDockVisible;
        bool oldHistoryDockVisible;
        bool oldKeypadVisible;
        bool oldFormulaBookDockVisible;
        bool oldVariablesDockVisible;
        bool oldUserFunctionsDockVisible;
        bool oldUserUnitsDockVisible;
        bool oldBitfieldVisible;
        bool oldWindowPositionSave;
        bool oldHasNumberFormatStyleSetting;

        ~SettingsGuard()
        {
            settings->sessionLayoutJson = oldSessionLayoutJson;
            settings->constantsDockVisible = oldConstantsDockVisible;
            settings->functionsDockVisible = oldFunctionsDockVisible;
            settings->historyDockVisible = oldHistoryDockVisible;
            settings->keypadVisible = oldKeypadVisible;
            settings->formulaBookDockVisible = oldFormulaBookDockVisible;
            settings->variablesDockVisible = oldVariablesDockVisible;
            settings->userFunctionsDockVisible = oldUserFunctionsDockVisible;
            settings->userUnitsDockVisible = oldUserUnitsDockVisible;
            settings->bitfieldVisible = oldBitfieldVisible;
            settings->windowPositionSave = oldWindowPositionSave;
            settings->hasNumberFormatStyleSetting = oldHasNumberFormatStyleSetting;
            if (hadSkipUpdateCheck)
                qputenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK", oldSkipUpdateCheck);
            else
                qunsetenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK");
        }
    } guard {
        appSettings,
        qgetenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK"),
        qEnvironmentVariableIsSet("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK"),
        appSettings->sessionLayoutJson,
        appSettings->constantsDockVisible,
        appSettings->functionsDockVisible,
        appSettings->historyDockVisible,
        appSettings->keypadVisible,
        appSettings->formulaBookDockVisible,
        appSettings->variablesDockVisible,
        appSettings->userFunctionsDockVisible,
        appSettings->userUnitsDockVisible,
        appSettings->bitfieldVisible,
        appSettings->windowPositionSave,
        appSettings->hasNumberFormatStyleSetting
    };

    qputenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK", "1");

    appSettings->sessionLayoutJson.clear();
    appSettings->constantsDockVisible = false;
    appSettings->functionsDockVisible = false;
    appSettings->historyDockVisible = false;
    appSettings->keypadVisible = false;
    appSettings->formulaBookDockVisible = false;
    appSettings->variablesDockVisible = false;
    appSettings->userFunctionsDockVisible = false;
    appSettings->userUnitsDockVisible = false;
    appSettings->bitfieldVisible = false;
    appSettings->windowPositionSave = false;
    appSettings->hasNumberFormatStyleSetting = true;

    MainWindow window;
    window.resize(900, 500);
    window.show();
    QCoreApplication::processEvents();

    QVERIFY(QMetaObject::invokeMethod(&window, "showNewSessionDialog", Qt::DirectConnection));
    QVERIFY(QMetaObject::invokeMethod(&window, "showNewSessionDialog", Qt::DirectConnection));
    QCoreApplication::processEvents();

    ResultDisplay* display = window.findChild<ResultDisplay*>();
    QVERIFY(display != nullptr);
    QWidget* pane = paneWidgetForDisplay(display);
    QTabBar* tabBar = pane ? pane->findChild<QTabBar*>() : nullptr;
    QVERIFY(tabBar != nullptr);
    QVERIFY(tabBar->isVisible());
    QCOMPARE(tabBar->count(), 3);

    const auto dragTab = [tabBar](int from, int to) {
        const QPoint start = tabBar->tabRect(from).center();
        const QPoint end = to == 0
            ? QPoint(tabBar->tabRect(0).left() + 1, tabBar->tabRect(0).center().y())
            : QPoint(tabBar->tabRect(to).right() - 2, tabBar->tabRect(to).center().y());

        sendTabDragMouseEvent(tabBar, QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
        const int step = qMax(1, qAbs(end.x() - start.x()) / 6);
        if (end.x() >= start.x()) {
            for (int x = start.x(); x <= end.x(); x += step)
                sendTabDragMouseEvent(tabBar, QEvent::MouseMove, QPoint(x, start.y()), Qt::NoButton, Qt::LeftButton);
        } else {
            for (int x = start.x(); x >= end.x(); x -= step)
                sendTabDragMouseEvent(tabBar, QEvent::MouseMove, QPoint(x, start.y()), Qt::NoButton, Qt::LeftButton);
        }
        sendTabDragMouseEvent(tabBar, QEvent::MouseMove, end, Qt::NoButton, Qt::LeftButton);
        sendTabDragMouseEvent(tabBar, QEvent::MouseButtonRelease, end, Qt::LeftButton, Qt::NoButton);
    };

    const QString firstTab = tabBar->tabText(0);
    dragTab(0, 2);
    QCoreApplication::processEvents();
    QCOMPARE(tabBar->tabText(tabBar->count() - 1), firstTab);

    dragTab(tabBar->count() - 1, 0);
    QCoreApplication::processEvents();
    QCOMPARE(tabBar->tabText(0), firstTab);
}

int main(int argc, char** argv)
{
    return UiTestFixture::run<TestSessionsUi>(argc, argv);
}

#include "testsessionsui.moc"
