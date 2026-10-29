// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mainwindowtesthelpers.h"

using namespace MainWindowTestHelpers;

class TestPrivateSessionsUi : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init() { UiTestFixture::resetSettings(); }
    void private_sessions_never_save_or_restore();
    void private_session_keep_action_is_discoverable();
    void private_session_conversion_preserves_contents_and_removes_json();
    void private_session_conversion_handles_storage_failures();
    void private_session_rename_duplicate_and_close_keep_privacy();
    void private_sessions_use_available_names_across_windows();
    void private_session_menu_targets_active_window_and_keeps_private_window_title();
    void private_tab_shortcut_opens_in_active_pane_data();
    void private_tab_shortcut_opens_in_active_pane();
    void private_only_workspace_restores_with_a_fresh_session();
    void startup_private_tab_setting_is_saved_and_reset();
    void startup_private_tab_reuses_empty_untitled_session_data();
    void startup_private_tab_reuses_empty_untitled_session();
    void startup_private_tab_without_saved_sessions_data();
    void startup_private_tab_without_saved_sessions();
    void startup_private_tab_restores_saved_sessions_data();
    void startup_private_tab_restores_saved_sessions();
    void startup_private_tab_is_opened_once_across_restored_windows();
};

void TestPrivateSessionsUi::initTestCase()
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

void TestPrivateSessionsUi::private_sessions_never_save_or_restore()
{
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    QString normalName;
    {
        MainWindow window(false);
        window.show();
        ResultDisplay* display = window.findChild<ResultDisplay*>();
        Editor* editor = window.findChild<Editor*>();
        normalName = display->session()->name();
        editor->setText(QStringLiteral("ordinaryvalue = 41"));
        QVERIFY(QMetaObject::invokeMethod(&window, "evaluateEditorExpression"));
        QVERIFY(QMetaObject::invokeMethod(&window, "showNewPrivateSession"));
        const Session* session = display->session();
        QVERIFY(session->isPrivate());
        QCOMPARE(session->name(), QStringLiteral("Private-1"));
        editor->setText(QStringLiteral("secretvalue = 987654321"));
        QVERIFY(QMetaObject::invokeMethod(&window, "evaluateEditorExpression"));
        editor->setText(QStringLiteral("unfinished_secret +"));
        QTest::qWait(300);
        QVERIFY(!QFileInfo::exists(storage.path(session->name())));
        QTabBar* tabBar = tabBarForDisplay(display);
        QVERIFY(!tabBar->tabIcon(tabBar->currentIndex()).isNull());
        QVERIFY(window.windowTitle().contains(QStringLiteral("[Private]")));

        QVERIFY(QMetaObject::invokeMethod(&window, "splitActivePaneRight"));
        QVERIFY(QMetaObject::invokeMethod(&window, "showNewPrivateSession"));
        window.persistSessionAndSettingsForShutdown();
        const QString layout = Settings::instance()->sessionLayoutJson;
        QVERIFY(!layout.contains(QStringLiteral("Private-")));
        QVERIFY(!layout.contains(QStringLiteral("unfinished_secret")));
        const QStringList files = QDir(QFileInfo(storage.path(normalName)).path()).entryList({QStringLiteral("*.json")});
        QVERIFY(files.contains(normalName + QStringLiteral(".json")));
        for (const QString& fileName : files) {
            QFile file(QDir(QFileInfo(storage.path(normalName)).path()).filePath(fileName));
            QVERIFY(file.open(QIODevice::ReadOnly));
            QVERIFY(!file.readAll().contains("secretvalue"));
        }
    }
    MainWindow restored;
    restored.show();
    QTest::qWait(400);
    for (ResultDisplay* display : restored.findChildren<ResultDisplay*>()) {
        QVERIFY(!display->session()->isPrivate());
        QVERIFY(!display->session()->hasVariable(QStringLiteral("secretvalue")));
    }
}

void TestPrivateSessionsUi::private_session_keep_action_is_discoverable()
{
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    MainWindow window(false);
    window.show();
    QVERIFY(QMetaObject::invokeMethod(&window, "showNewPrivateSession"));
    ResultDisplay* display = window.findChild<ResultDisplay*>();
    QVERIFY(display);
    QTabBar* tabs = tabBarForDisplay(display);
    QVERIFY(tabs);
    QCOMPARE(tabs->tabToolTip(tabs->currentIndex()),
             QStringLiteral("%1\nPrivate session: contents are discarded when closed.\n"
                            "Choose Session > Keep to turn it into a regular session.").arg(display->session()->name()));

    QMenu* menu = menuWithTitle(window.menuBar(), QStringLiteral("&Session"));
    QVERIFY(menu);
    QAction* keep = directMenuActionWithText(menu, QStringLiteral("Keep"));
    QVERIFY(keep);
    window.activateWindow();
    window.findChild<Editor*>()->setFocus();
    QTRY_COMPARE(QApplication::activeWindow(), static_cast<QWidget*>(&window));
    keep->trigger();
    QVERIFY(!display->session()->isPrivate());
    QVERIFY(QFileInfo::exists(storage.path(display->session()->name())));
    QVERIFY(tabs->tabToolTip(tabs->currentIndex()).isEmpty());
}

void TestPrivateSessionsUi::private_session_conversion_preserves_contents_and_removes_json()
{
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    MainWindow window(false);
    window.show();
    ResultDisplay* display = window.findChild<ResultDisplay*>();
    Editor* editor = window.findChild<Editor*>();
    const QString occupiedName = display->session()->name();
    QVERIFY(QMetaObject::invokeMethod(&window, "showNewPrivateSession"));
    const Session* session = display->session();
    editor->setText(QStringLiteral("privatevalue = 123"));
    QVERIFY(QMetaObject::invokeMethod(&window, "evaluateEditorExpression"));
    editor->setText(QStringLiteral("privatevalue +"));
    QVERIFY(QMetaObject::invokeMethod(&window, "convertSessionPrivacy"));
    QVERIFY(!session->isPrivate());
    QCOMPARE(session->name(), QStringLiteral("Untitled-2"));
    QVERIFY(session->name() != occupiedName);
    QCOMPARE(session->historySize(), 1);
    QCOMPARE(editor->text(), QStringLiteral("privatevalue +"));
    QVERIFY(Settings::instance()->sessionLayoutJson.contains(QStringLiteral("privatevalue +")));
    const QString savedName = session->name();
    QFile saved(storage.path(savedName));
    QVERIFY(saved.open(QIODevice::ReadOnly));
    QVERIFY(saved.readAll().contains("privatevalue"));
    saved.close();

    bool confirmed = false;
    QTimer::singleShot(0, &window, [&]() {
        auto* dialog = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        QVERIFY(dialog);
        confirmed = dialog->text().contains(savedName) && dialog->text().contains(QStringLiteral("deleted"));
        QCOMPARE(dialog->defaultButton(), dialog->button(QMessageBox::Cancel));
        dialog->done(QMessageBox::Cancel);
    });
    QVERIFY(QMetaObject::invokeMethod(&window, "convertSessionPrivacy"));
    QVERIFY(confirmed);
    QVERIFY(!session->isPrivate());
    QVERIFY(QFileInfo::exists(storage.path(savedName)));

    // Leave a delayed save queued when confirming conversion.
    editor->setText(QStringLiteral("456"));
    QVERIFY(QMetaObject::invokeMethod(&window, "evaluateEditorExpression"));
    QTimer::singleShot(0, &window, []() {
        auto* dialog = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        QVERIFY(dialog);
        dialog->done(QMessageBox::Yes);
    });
    QVERIFY(QMetaObject::invokeMethod(&window, "convertSessionPrivacy"));
    QVERIFY(session->isPrivate());
    QCOMPARE(session->name(), QStringLiteral("Private-1"));
    QCOMPARE(session->historySize(), 2);
    QTest::qWait(300);
    QVERIFY(!QFileInfo::exists(storage.path(savedName)));
    QVERIFY(!Settings::instance()->sessionLayoutJson.contains(savedName));
    QVERIFY(!Settings::instance()->sessionLayoutJson.contains(session->name()));
    window.persistSessionAndSettingsForShutdown();
    QVERIFY(!QFileInfo::exists(storage.path(savedName)));
}

void TestPrivateSessionsUi::private_session_conversion_handles_storage_failures()
{
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    MainWindow window(false);
    window.show();
    ResultDisplay* display = window.findChild<ResultDisplay*>();
    const Session* session = display->session();
    const QString name = session->name();
    QVERIFY(QDir().mkpath(storage.path(name)));
    QTimer::singleShot(0, &window, [&]() {
        auto* confirmation = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        QVERIFY(confirmation);
        QTimer::singleShot(0, &window, []() {
            auto* error = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            QVERIFY(error);
            QVERIFY(error->text().contains(QStringLiteral("Could not delete")));
            error->accept();
        });
        confirmation->done(QMessageBox::Yes);
    });
    QVERIFY(QMetaObject::invokeMethod(&window, "convertSessionPrivacy"));
    QVERIFY(!session->isPrivate());
    QCOMPARE(session->name(), name);
    QVERIFY(QMetaObject::invokeMethod(&window, "showNewPrivateSession"));
    session = display->session();
    const QString privateName = session->name();
    const QString sessionsDir = QFileInfo(storage.path(name)).path();
    QVERIFY(QDir(sessionsDir).removeRecursively());
    QFile blocker(sessionsDir);
    QVERIFY(blocker.open(QIODevice::WriteOnly));
    blocker.close();
    QTimer::singleShot(0, &window, []() {
        auto* error = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        QVERIFY(error);
        QVERIFY(error->text().contains(QStringLiteral("remains private")));
        error->accept();
    });
    QVERIFY(QMetaObject::invokeMethod(&window, "convertSessionPrivacy"));
    QVERIFY(session->isPrivate());
    QCOMPARE(session->name(), privateName);
}

void TestPrivateSessionsUi::private_session_rename_duplicate_and_close_keep_privacy()
{
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    MainWindow window(false);
    window.show();
    QVERIFY(QMetaObject::invokeMethod(&window, "showNewPrivateSession"));
    ResultDisplay* display = window.findChild<ResultDisplay*>();
    Editor* editor = window.findChild<Editor*>();
    editor->setText(QStringLiteral("789"));
    QVERIFY(QMetaObject::invokeMethod(&window, "evaluateEditorExpression"));
    const auto enterName = [&window](const QString& name) {
        QTimer::singleShot(0, &window, [name]() {
            auto* dialog = qobject_cast<QInputDialog*>(QApplication::activeModalWidget());
            QVERIFY(dialog);
            dialog->setTextValue(name);
            dialog->accept();
        });
    };
    enterName(QStringLiteral("Temporary work"));
    QVERIFY(QMetaObject::invokeMethod(&window, "showRenameSessionDialog"));
    QVERIFY(display->session()->isPrivate());
    QCOMPARE(display->session()->name(), QStringLiteral("Temporary work"));
    QVERIFY(!QFileInfo::exists(storage.path(QStringLiteral("Temporary work"))));
    enterName(QStringLiteral("Temporary copy"));
    QVERIFY(QMetaObject::invokeMethod(&window, "showDuplicateSessionDialog"));
    QVERIFY(display->session()->isPrivate());
    QCOMPARE(display->session()->historySize(), 1);
    QVERIFY(QMetaObject::invokeMethod(&window, "closeCurrentSession"));
    QVERIFY(QMetaObject::invokeMethod(&window, "restoreClosedSessionTab"));
    for (QTabBar* tabBar : window.findChildren<QTabBar*>()) {
        for (int i = 0; i < tabBar->count(); ++i)
            QVERIFY(tabBar->tabText(i) != QStringLiteral("Temporary copy"));
    }
    window.persistSessionAndSettingsForShutdown();
    QVERIFY(!QFileInfo::exists(storage.path(QStringLiteral("Temporary copy"))));
    QVERIFY(!Settings::instance()->sessionLayoutJson.contains(QStringLiteral("Temporary")));
}

void TestPrivateSessionsUi::private_sessions_use_available_names_across_windows()
{
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    MainWindow first(false);
    MainWindow second(false);
    QVERIFY(QMetaObject::invokeMethod(&first, "showNewPrivateSession"));
    QVERIFY(QMetaObject::invokeMethod(&second, "showNewPrivateSession"));
    QCOMPARE(first.findChild<ResultDisplay*>()->session()->name(), QStringLiteral("Private-1"));
    QCOMPARE(second.findChild<ResultDisplay*>()->session()->name(), QStringLiteral("Private-2"));
    QVERIFY(QMetaObject::invokeMethod(&first, "closeCurrentSession"));
    QVERIFY(QMetaObject::invokeMethod(&second, "showNewPrivateSession"));
    QCOMPARE(second.findChild<ResultDisplay*>()->session()->name(), QStringLiteral("Private-1"));
}

void TestPrivateSessionsUi::private_session_menu_targets_active_window_and_keeps_private_window_title()
{
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    MainWindow first(false);
    MainWindow second(false);
    first.show();
    second.show();
    QVERIFY(tabBarForDisplay(first.findChild<ResultDisplay*>())->isHidden());
    QMenu* menu = menuWithTitle(second.menuBar(), QStringLiteral("&Session"));
    QVERIFY(menu);
    QAction* create = directMenuActionWithText(menu, QStringLiteral("New &Private Tab"));
    QVERIFY(create);
    first.activateWindow();
    first.findChild<Editor*>()->setFocus();
    QTRY_COMPARE(QApplication::activeWindow(), static_cast<QWidget*>(&first));
    create->trigger();
    ResultDisplay* display = first.findChild<ResultDisplay*>();
    QVERIFY(display->session()->isPrivate());
    QVERIFY(!second.findChild<ResultDisplay*>()->session()->isPrivate());
    QTabBar* tabs = tabBarForDisplay(display);
    QCOMPARE(tabs->count(), 2);
    QVERIFY(tabs->isVisible());
    tabs->setCurrentIndex(0);
    QVERIFY(QMetaObject::invokeMethod(&first, "closeCurrentSession"));
    QCOMPARE(tabs->count(), 1);
    QVERIFY(tabs->isHidden());
    QVERIFY(!tabs->tabIcon(0).isNull());
    QVERIFY(display->session()->isPrivate());
    QVERIFY(first.windowTitle().contains(QStringLiteral("[Private]")));

    QAction* convert = directMenuActionWithText(menu, QStringLiteral("Keep"));
    QVERIFY(convert);
    convert->trigger();
    QVERIFY(!display->session()->isPrivate());
    QVERIFY(tabs->isHidden());
    QVERIFY(!second.findChild<ResultDisplay*>()->session()->isPrivate());
    QVERIFY(QFileInfo::exists(storage.path(display->session()->name())));
}

void TestPrivateSessionsUi::private_tab_shortcut_opens_in_active_pane_data()
{
    QTest::addColumn<QString>("focusTarget");
    QTest::newRow("window") << QStringLiteral("window");
    QTest::newRow("editor") << QStringLiteral("editor");
    QTest::newRow("display") << QStringLiteral("display");
}

void TestPrivateSessionsUi::private_tab_shortcut_opens_in_active_pane()
{
    QFETCH(QString, focusTarget);
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    MainWindow window(false);
    MainWindow other(false);
    window.show();
    other.show();
    QVERIFY(QMetaObject::invokeMethod(&window, "splitActivePaneRight"));
    const QList<ResultDisplay*> displays = window.findChildren<ResultDisplay*>();
    QCOMPARE(displays.size(), 2);
    ResultDisplay* target = displays.last();
    Editor* editor = paneWidgetForDisplay(target)->findChild<Editor*>();
    QVERIFY(editor);
    window.activateWindow();
    QTest::mouseClick(target->viewport(), Qt::LeftButton, Qt::NoModifier, target->viewport()->rect().center());
    QTRY_COMPARE(QApplication::activeWindow(), static_cast<QWidget*>(&window));
    editor->setText(QStringLiteral("ordinary draft"));
    const Session* ordinary = target->session();

    QMenu* menu = menuWithTitle(window.menuBar(), QStringLiteral("&Session"));
    QVERIFY(menu);
    QAction* action = directMenuActionWithText(menu, QStringLiteral("New &Private Tab"));
    QVERIFY(action);
    const QKeySequence shortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_N);
    QCOMPARE(action->shortcut(), shortcut);
    QSignalSpy triggered(action, &QAction::triggered);
    QWidget* focus = &window;
    if (focusTarget == QLatin1String("editor"))
        focus = editor;
    else if (focusTarget == QLatin1String("display"))
        focus = target;
    focus->setFocus();
    QCoreApplication::processEvents();
    QTest::keyClick(focus, shortcut[0].key(), shortcut[0].keyboardModifiers());
    QTRY_COMPARE(triggered.count(), 1);
    QVERIFY(target->session()->isPrivate());
    QCOMPARE(tabBarForDisplay(target)->count(), 2);
    QCOMPARE(tabBarForDisplay(displays.first())->count(), 1);
    QVERIFY(!other.findChild<ResultDisplay*>()->session()->isPrivate());
    QCOMPARE(ordinary->editorText(), QStringLiteral("ordinary draft"));
    QVERIFY(editor->text().isEmpty());
}

void TestPrivateSessionsUi::private_only_workspace_restores_with_a_fresh_session()
{
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    {
        MainWindow window(false);
        QVERIFY(QMetaObject::invokeMethod(&window, "showNewPrivateSession"));
        ResultDisplay* display = window.findChild<ResultDisplay*>();
        QTabBar* tabs = tabBarForDisplay(display);
        tabs->setCurrentIndex(0);
        QVERIFY(QMetaObject::invokeMethod(&window, "closeCurrentSession"));
        QVERIFY(display->session()->isPrivate());
        window.findChild<Editor*>()->setText(QStringLiteral("sole_private_secret"));
        window.persistSessionAndSettingsForShutdown();
        const QJsonObject layout = QJsonDocument::fromJson(Settings::instance()->sessionLayoutJson.toUtf8()).object();
        QVERIFY(layout.value(QStringLiteral("windows")).toArray().isEmpty());
        QVERIFY(!Settings::instance()->sessionLayoutJson.contains(QStringLiteral("sole_private_secret")));
    }
    MainWindow restored;
    QTest::qWait(300);
    for (ResultDisplay* display : restored.findChildren<ResultDisplay*>()) {
        QVERIFY(!display->session()->isPrivate());
        QVERIFY(display->session()->historyIsEmpty());
    }
    QVERIFY(restored.findChild<Editor*>()->text().isEmpty());
}

void TestPrivateSessionsUi::startup_private_tab_setting_is_saved_and_reset()
{
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    const auto resetSettings = qScopeGuard([]() { UiTestFixture::resetSettings(); });
    Settings* settings = Settings::instance();
    QVERIFY(!settings->openPrivateTabOnStartup);
    MainWindow first(false);
    MainWindow second(false);
    const auto startupAction = [](MainWindow& window) -> QAction* {
        QMenu* menu = menuWithTitle(window.menuBar(), QStringLiteral("Se&ttings"));
        QMenu* windowMenu = menu ? directSubmenuWithTitle(menu, QStringLiteral("&Window")) : nullptr;
        return windowMenu ? directMenuActionWithText(windowMenu, QStringLiteral("Open a Private Tab on Startup")) : nullptr;
    };
    QAction* firstAction = startupAction(first);
    QAction* secondAction = startupAction(second);
    QVERIFY(firstAction);
    QVERIFY(secondAction);
    QVERIFY(firstAction->isCheckable());
    QVERIFY(!firstAction->isChecked());
    QCOMPARE(firstAction->toolTip(), QStringLiteral("Open and focus a new private tab whenever SpeedCrunch starts."));
    firstAction->trigger();
    QVERIFY(settings->openPrivateTabOnStartup);
    QVERIFY(secondAction->isChecked());
    QVERIFY(!first.findChild<ResultDisplay*>()->session()->isPrivate());
    QVERIFY(!second.findChild<ResultDisplay*>()->session()->isPrivate());
    settings->openPrivateTabOnStartup = false;
    settings->load();
    QVERIFY(settings->openPrivateTabOnStartup);
    MainWindow another(false);
    QVERIFY(startupAction(another)->isChecked());
    QVERIFY(!another.findChild<ResultDisplay*>()->session()->isPrivate());

    secondAction->trigger();
    QVERIFY(!firstAction->isChecked());
    QVERIFY(!startupAction(another)->isChecked());
    settings->load();
    QVERIFY(!settings->openPrivateTabOnStartup);
    firstAction->trigger();
    QTimer::singleShot(0, &first, []() {
        auto* dialog = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        QVERIFY(dialog);
        dialog->done(QMessageBox::Yes);
    });
    QVERIFY(QMetaObject::invokeMethod(&first, "resetAllSettings"));
    QVERIFY(!settings->openPrivateTabOnStartup);
    QVERIFY(!firstAction->isChecked());
    QVERIFY(!secondAction->isChecked());
    QVERIFY(!startupAction(another)->isChecked());
    settings->load();
    QVERIFY(!settings->openPrivateTabOnStartup);
}

void TestPrivateSessionsUi::startup_private_tab_reuses_empty_untitled_session_data()
{
    QTest::addColumn<QString>("content");
    QTest::addColumn<bool>("enabled");
    QTest::newRow("empty-enabled") << QStringLiteral("empty") << true;
    QTest::newRow("empty-disabled") << QStringLiteral("empty") << false;
    QTest::newRow("empty-legacy") << QStringLiteral("legacy") << true;
    QTest::newRow("global-definitions") << QStringLiteral("global") << true;
    QTest::newRow("history") << QStringLiteral("history") << true;
    QTest::newRow("draft") << QStringLiteral("draft") << true;
    QTest::newRow("variable") << QStringLiteral("variable") << true;
    QTest::newRow("function") << QStringLiteral("function") << true;
    QTest::newRow("unit") << QStringLiteral("unit") << true;
    QTest::newRow("named-empty-session") << QStringLiteral("named") << true;
}

void TestPrivateSessionsUi::startup_private_tab_reuses_empty_untitled_session()
{
    QFETCH(QString, content);
    QFETCH(bool, enabled);
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    const auto resetSettings = qScopeGuard([]() { UiTestFixture::resetSettings(); });
    Settings* settings = Settings::instance();
    settings->openPrivateTabOnStartup = enabled;
    const bool legacy = content == QLatin1String("legacy");
    const QString name = content == QLatin1String("named")
        ? QStringLiteral("Saved empty session")
        : (legacy ? QStringLiteral("Untitled-1") : QStringLiteral("Untitled-9"));
    Session savedSession;
    savedSession.setName(name);
    if (content == QLatin1String("history"))
        savedSession.addHistoryEntry(HistoryEntry(QStringLiteral("6*7"), Quantity(42)));
    else if (content == QLatin1String("variable"))
        savedSession.addVariable(Variable(QStringLiteral("startupvalue"), Quantity(42)));
    else if (content == QLatin1String("function"))
        savedSession.addUserFunction(UserFunction(QStringLiteral("startupfunction"), {QStringLiteral("x")}, QStringLiteral("x+1")));
    else if (content == QLatin1String("unit"))
        savedSession.addUserUnit(UserUnit(QStringLiteral("startupunit"), Quantity(42)));
    else if (content == QLatin1String("global")) {
        settings->startupUserDefinitions = QStringLiteral("startupglobal=42");
        UserDefinitions::saveFrom(settings);
    }
    QJsonObject json;
    savedSession.serialize(json);
    const QString path = legacy
        ? QDir(Settings::getDataPath()).filePath(QStringLiteral("history.json")) : storage.path(name);
    QDir().mkpath(QFileInfo(path).path());
    writeFile(path, QJsonDocument(json).toJson());
    if (!legacy) {
        QJsonObject window = startupSessionWindow(name, QStringLiteral("saved"));
        QJsonObject root = window.value(QStringLiteral("root")).toObject();
        QJsonObject tab = root.value(QStringLiteral("tabs")).toArray().first().toObject();
        tab.insert(QStringLiteral("editor"), QJsonObject {
            { QStringLiteral("text"), content == QLatin1String("draft") ? QStringLiteral("unfinished input") : QString() }
        });
        root.insert(QStringLiteral("tabs"), QJsonArray { tab });
        window.insert(QStringLiteral("root"), root);
        settings->sessionLayoutJson = QString::fromUtf8(QJsonDocument(QJsonObject {
            { QStringLiteral("scheme"), 1 },
            { QStringLiteral("kind"), QStringLiteral("session-layout") },
            { QStringLiteral("windows"), QJsonArray { window } }
        }).toJson(QJsonDocument::Compact));
    }
    MainWindow window;
    window.show();
    if (enabled)
        QTRY_VERIFY(window.findChild<ResultDisplay*>()->session()->isPrivate());
    else
        QTRY_COMPARE(window.findChild<ResultDisplay*>()->session()->name(), name);
    ResultDisplay* display = window.findChild<ResultDisplay*>();
    QTabBar* tabs = tabBarForDisplay(display);
    const bool keepRegular = content != QLatin1String("empty") && content != QLatin1String("legacy")
        && content != QLatin1String("global");
    QCOMPARE(tabs->count(), enabled && keepRegular ? 2 : 1);
    QTRY_VERIFY(window.findChild<Editor*>()->hasFocus());
    if (enabled) {
        QVERIFY(display->session()->historyIsEmpty());
        if (content == QLatin1String("global"))
            QVERIFY(display->session()->hasVariable(QStringLiteral("startupglobal")));
    }
    if (keepRegular || !enabled) {
        tabs->setCurrentIndex(0);
        QCOMPARE(display->session()->name(), name);
        QVERIFY(!display->session()->isPrivate());
        if (content == QLatin1String("history"))
            QCOMPARE(display->session()->historySize(), 1);
        else if (content == QLatin1String("draft"))
            QCOMPARE(window.findChild<Editor*>()->text(), QStringLiteral("unfinished input"));
        else if (content == QLatin1String("variable"))
            QVERIFY(display->session()->hasVariable(QStringLiteral("startupvalue")));
        else if (content == QLatin1String("function"))
            QVERIFY(display->session()->hasUserFunction(QStringLiteral("startupfunction")));
        else if (content == QLatin1String("unit"))
            QVERIFY(display->session()->hasUserUnit(QStringLiteral("startupunit")));
    } else {
        QVERIFY(tabs->isHidden());
    }
    QVERIFY(QFileInfo::exists(storage.path(name)));
    QFile saved(storage.path(name));
    QVERIFY(saved.open(QIODevice::ReadOnly));
    QCOMPARE(QJsonDocument::fromJson(saved.readAll()).object(), json);
}

void TestPrivateSessionsUi::startup_private_tab_without_saved_sessions_data()
{
    QTest::addColumn<QString>("savedLayout");
    QTest::newRow("fresh") << QString();
    QTest::newRow("invalid-layout") << QStringLiteral("invalid JSON");
    QTest::newRow("private-only-workspace") << QStringLiteral(
        "{\"scheme\":1,\"kind\":\"session-layout\",\"windows\":[]}");
    const QJsonObject window = startupSessionWindow(QStringLiteral("Missing"), QStringLiteral("saved"));
    QTest::newRow("missing-session-file") << QString::fromUtf8(QJsonDocument(QJsonObject {
        { QStringLiteral("scheme"), 1 },
        { QStringLiteral("kind"), QStringLiteral("session-layout") },
        { QStringLiteral("windows"), QJsonArray { window } }
    }).toJson(QJsonDocument::Compact));
}

void TestPrivateSessionsUi::startup_private_tab_without_saved_sessions()
{
    QFETCH(QString, savedLayout);
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    const auto resetSettings = qScopeGuard([]() { UiTestFixture::resetSettings(); });
    Settings* settings = Settings::instance();
    settings->openPrivateTabOnStartup = true;
    settings->sessionLayoutJson = savedLayout;
    {
        MainWindow window;
        window.show();
        ResultDisplay* display = window.findChild<ResultDisplay*>();
        QTRY_VERIFY(display->session()->isPrivate());
        QTabBar* tabs = tabBarForDisplay(display);
        QCOMPARE(tabs->count(), 1);
        QVERIFY(tabs->isHidden());
        Editor* editor = window.findChild<Editor*>();
        QTRY_VERIFY(editor->hasFocus());
        QVERIFY(display->session()->historyIsEmpty());
        QVERIFY(editor->text().isEmpty());
        editor->setText(QStringLiteral("startupsecret = 7654321"));
        QVERIFY(QMetaObject::invokeMethod(&window, "evaluateEditorExpression"));
        editor->setText(QStringLiteral("unfinished startup secret"));
        window.persistSessionAndSettingsForShutdown();
        QVERIFY(!QFileInfo::exists(storage.path(display->session()->name())));
        QVERIFY(!QFileInfo::exists(storage.path(QStringLiteral("Untitled-1"))));
        QVERIFY(!settings->sessionLayoutJson.contains(QStringLiteral("startupsecret")));
        QVERIFY(!settings->sessionLayoutJson.contains(QStringLiteral("unfinished startup secret")));
    }
    MainWindow restarted;
    restarted.show();
    ResultDisplay* display = restarted.findChild<ResultDisplay*>();
    QTRY_VERIFY(display->session()->isPrivate());
    QCOMPARE(tabBarForDisplay(display)->count(), 1);
    QVERIFY(display->session()->historyIsEmpty());
    QVERIFY(restarted.findChild<Editor*>()->text().isEmpty());
}

void TestPrivateSessionsUi::startup_private_tab_restores_saved_sessions_data()
{
    QTest::addColumn<QString>("layoutType");
    QTest::addColumn<bool>("enabled");
    QTest::newRow("legacy-enabled") << QStringLiteral("legacy") << true;
    QTest::newRow("legacy-disabled") << QStringLiteral("legacy") << false;
    QTest::newRow("tabs-enabled") << QStringLiteral("tabs") << true;
    QTest::newRow("tabs-disabled") << QStringLiteral("tabs") << false;
    QTest::newRow("split-enabled") << QStringLiteral("split") << true;
    QTest::newRow("split-disabled") << QStringLiteral("split") << false;
}

void TestPrivateSessionsUi::startup_private_tab_restores_saved_sessions()
{
    QFETCH(QString, layoutType);
    QFETCH(bool, enabled);
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    const auto resetSettings = qScopeGuard([]() { UiTestFixture::resetSettings(); });
    Settings* settings = Settings::instance();
    settings->openPrivateTabOnStartup = enabled;
    const bool legacy = layoutType == QLatin1String("legacy");
    const QString activeName = legacy ? QStringLiteral("Untitled-1") : QStringLiteral("Saved-2");
    const QString firstPath = legacy
        ? QDir(Settings::getDataPath()).filePath(QStringLiteral("history.json"))
        : storage.path(QStringLiteral("Untitled-1"));
    QDir().mkpath(QFileInfo(firstPath).path());
    writeFile(firstPath, QJsonDocument(sessionJson(QStringLiteral("Untitled-1"))).toJson());
    if (!legacy) {
        writeFile(storage.path(activeName), QJsonDocument(sessionJson(activeName)).toJson());
        QJsonObject window = startupSessionWindow(activeName, QStringLiteral("saved"));
        QJsonObject root = window.value(QStringLiteral("root")).toObject();
        QJsonObject firstPane = startupSessionWindow(QStringLiteral("Untitled-1"), QString()).value(QStringLiteral("root")).toObject();
        if (layoutType == QLatin1String("split")) {
            firstPane.insert(QStringLiteral("type"), QStringLiteral("pane"));
            QJsonObject secondPane = root;
            secondPane.insert(QStringLiteral("type"), QStringLiteral("pane"));
            root.insert(QStringLiteral("type"), QStringLiteral("split"));
            root.insert(QStringLiteral("orientation"), QStringLiteral("horizontal"));
            root.insert(QStringLiteral("children"), QJsonArray { firstPane, secondPane });
        } else {
            root.insert(QStringLiteral("tabs"), QJsonArray {
                firstPane.value(QStringLiteral("tabs")).toArray().first(),
                root.value(QStringLiteral("tabs")).toArray().first()
            });
        }
        window.insert(QStringLiteral("root"), root);
        settings->sessionLayoutJson = QString::fromUtf8(QJsonDocument(QJsonObject {
            { QStringLiteral("scheme"), 1 },
            { QStringLiteral("kind"), QStringLiteral("session-layout") },
            { QStringLiteral("activeWindow"), QStringLiteral("saved") },
            { QStringLiteral("windows"), QJsonArray { window } }
        }).toJson(QJsonDocument::Compact));
    }
    MainWindow window;
    window.show();
    const auto activeDisplay = [&window]() -> ResultDisplay* {
        for (ResultDisplay* display : window.findChildren<ResultDisplay*>()) {
            Editor* editor = paneWidgetForDisplay(display)->findChild<Editor*>();
            if (editor && editor->hasFocus())
                return display;
        }
        return nullptr;
    };
    QTRY_VERIFY(activeDisplay());
    QTRY_COMPARE(activeDisplay()->session()->isPrivate(), enabled);
    if (!enabled) {
        QTRY_COMPARE(activeDisplay()->session()->name(), activeName);
        QTRY_COMPARE(activeDisplay()->session()->historySize(), 1);
    }
    ResultDisplay* display = activeDisplay();
    QTabBar* tabs = tabBarForDisplay(display);
    QCOMPARE(tabs->count(), (legacy || layoutType == QLatin1String("split") ? 1 : 2) + (enabled ? 1 : 0));
    if (enabled) {
        QVERIFY(display->session()->historyIsEmpty());
        QCOMPARE(window.findChildren<ResultDisplay*>().size(), layoutType == QLatin1String("split") ? 2 : 1);
        int ordinaryIndex = -1;
        for (int i = 0; i < tabs->count(); ++i) {
            if (tabs->tabText(i) == activeName)
                ordinaryIndex = i;
        }
        QVERIFY(ordinaryIndex >= 0);
        tabs->setCurrentIndex(ordinaryIndex);
    }
    QCOMPARE(display->session()->name(), activeName);
    QCOMPARE(display->session()->historySize(), 1);
    Editor* editor = paneWidgetForDisplay(display)->findChild<Editor*>();
    QCOMPARE(editor->text(), legacy ? QString() : QStringLiteral("saved draft"));
    window.persistSessionAndSettingsForShutdown();
    QVERIFY(!settings->sessionLayoutJson.contains(QStringLiteral("Private-")));
    QVERIFY(settings->sessionLayoutJson.contains(activeName));
}

void TestPrivateSessionsUi::startup_private_tab_is_opened_once_across_restored_windows()
{
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    const auto resetSettings = qScopeGuard([]() { UiTestFixture::resetSettings(); });
    Settings* settings = Settings::instance();
    settings->openPrivateTabOnStartup = true;
    QDir().mkpath(QFileInfo(storage.path(QStringLiteral("Saved-First"))).path());
    for (const QString& name : {QStringLiteral("Saved-First"), QStringLiteral("Saved-Active")})
        writeFile(storage.path(name), QJsonDocument(sessionJson(name)).toJson());
    settings->sessionLayoutJson = QString::fromUtf8(QJsonDocument(QJsonObject {
        { QStringLiteral("scheme"), 1 },
        { QStringLiteral("kind"), QStringLiteral("session-layout") },
        { QStringLiteral("activeWindow"), QStringLiteral("active") },
        { QStringLiteral("windows"), QJsonArray {
            startupSessionWindow(QStringLiteral("Saved-First"), QStringLiteral("first")),
            startupSessionWindow(QStringLiteral("Saved-Active"), QStringLiteral("active"))
        } }
    }).toJson(QJsonDocument::Compact));
    MainWindow window;
    const auto cleanupWindows = qScopeGuard([&window]() {
        for (MainWindow* other : topLevelMainWindows()) {
            if (other != &window)
                delete other;
        }
    });
    window.show();
    QTRY_COMPARE(topLevelMainWindows().size(), 2);
    MainWindow* other = nullptr;
    for (MainWindow* candidate : topLevelMainWindows()) {
        if (candidate != &window)
            other = candidate;
    }
    QVERIFY(other);
    QTRY_COMPARE(other->findChild<ResultDisplay*>()->session()->name(), QStringLiteral("Saved-First"));
    QTRY_VERIFY(other->isVisible());
    QTRY_VERIFY(window.findChild<ResultDisplay*>()->session()->isPrivate());
    QCOMPARE(tabBarForDisplay(window.findChild<ResultDisplay*>())->count(), 2);
    QCOMPARE(tabBarForDisplay(other->findChild<ResultDisplay*>())->count(), 1);
    QVERIFY(!other->findChild<ResultDisplay*>()->session()->isPrivate());
    QCOMPARE(other->findChild<Editor*>()->text(), QStringLiteral("saved draft"));
    QTRY_COMPARE(QApplication::activeWindow(), static_cast<QWidget*>(&window));
    QTRY_VERIFY(window.findChild<Editor*>()->hasFocus());
    QVERIFY(settings->sessionLayoutJson.contains(QStringLiteral("Saved-Active")));
    QVERIFY(settings->sessionLayoutJson.contains(QStringLiteral("Saved-First")));
    QVERIFY(settings->sessionLayoutJson.contains(QStringLiteral("saved draft")));
    window.persistSessionAndSettingsForShutdown();
    const QJsonArray windows = QJsonDocument::fromJson(settings->sessionLayoutJson.toUtf8()).object().value(QStringLiteral("windows")).toArray();
    QCOMPARE(windows.size(), 2);
    QVERIFY(!settings->sessionLayoutJson.contains(QStringLiteral("Private-")));
    QVERIFY(settings->sessionLayoutJson.contains(QStringLiteral("Saved-Active")));
    QVERIFY(settings->sessionLayoutJson.contains(QStringLiteral("Saved-First")));
}

int main(int argc, char** argv)
{
    return UiTestFixture::run<TestPrivateSessionsUi>(argc, argv);
}

#include "testprivatesessionsui.moc"
