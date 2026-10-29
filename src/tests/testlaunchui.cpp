// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mainwindowtesthelpers.h"
#include "core/singleinstance.h"
#include "gui/launchcontroller.h"
#include "gui/platformactivation.h"

#include <QLocalSocket>
#include <QCryptographicHash>
#include <QProcess>
#include <QProcessEnvironment>
#include <QThread>
#include <QUuid>

using namespace MainWindowTestHelpers;

#ifdef Q_OS_MACOS
bool dispatchMacReopenEvent();
#endif

namespace {
QString serverName()
{
    return QStringLiteral("speedcrunch-test-%1").arg(QUuid::createUuid().toString(QUuid::Id128));
}

QAction* launchAction(MainWindow& window, int choice)
{
    const QStringList names = {QStringLiteral("SubsequentLaunchActivateAction"),
                              QStringLiteral("SubsequentLaunchNewWindowAction"),
                              QStringLiteral("SubsequentLaunchNewTabAction")};
    return window.findChild<QAction*>(names.at(choice));
}

int mainWindowCount()
{
    int count = 0;
    for (QWidget* widget : QApplication::topLevelWidgets()) {
        if (qobject_cast<MainWindow*>(widget))
            ++count;
    }
    return count;
}
}

class TestLaunchUi : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() { qRegisterMetaType<MainWindow*>("MainWindow*"); }
    void init() { UiTestFixture::resetSettings(); }
    void cleanup();
    void preference_persistence_data();
    void preference_persistence();
    void menu_is_exclusive_shared_and_resettable();
    void initial_launch_ignores_preference_data();
    void initial_launch_ignores_preference();
    void launch_actions_data();
    void launch_actions();
    void launches_target_last_active_window_and_pane();
    void new_tabs_use_unique_names_across_windows_data();
    void new_tabs_use_unique_names_across_windows();
    void launches_wait_for_initial_window_and_session_restore();
    void launches_wait_for_modal_dialog();
    void activate_choice_handles_modal_dialog_immediately();
    void shutdown_discards_queued_launches();
    void normal_activation_does_not_create_sessions();
    void request_encoding_data();
    void request_encoding();
    void invalid_requests_data();
    void invalid_requests();
    void ipc_preserves_every_request_and_metadata();
    void ipc_waits_for_complete_request_and_ignores_extra_lines();
    void ipc_rejects_shutdown();
    void secondary_instance_exits_after_acceptance();
    void retries_do_not_duplicate_launches();
    void busy_startup_keeps_single_instance();
    void startup_lock_is_released_on_close();
    void startup_lock_recovers_after_process_death();
    void ipc_lock_process_helper();
#ifdef Q_OS_WIN
    void startup_lock_excludes_legacy_lock_files();
#endif
    void activation_restores_minimized_window();
#ifdef Q_OS_MACOS
    void native_reopen_follows_preference_data();
    void native_reopen_follows_preference();
#endif
};

void TestLaunchUi::cleanup()
{
    // Locally owned windows have already left scope. Release windows created
    // by launch requests without invoking the application's Quit action.
    QList<QPointer<MainWindow>> windows;
    for (QWidget* widget : QApplication::topLevelWidgets()) {
        if (auto* window = qobject_cast<MainWindow*>(widget))
            windows.append(window);
    }
    for (const QPointer<MainWindow>& window : windows)
        delete window.data();
    qApp->setProperty("speedcrunchShutdownInProgress", false);
}

void TestLaunchUi::preference_persistence_data()
{
    QTest::addColumn<QVariant>("stored");
    QTest::addColumn<int>("expected");
    QTest::newRow("missing") << QVariant() << 0;
    QTest::newRow("activate") << QVariant(0) << 0;
    QTest::newRow("window") << QVariant(1) << 1;
    QTest::newRow("tab") << QVariant(2) << 2;
    QTest::newRow("negative") << QVariant(-1) << 0;
    QTest::newRow("unknown") << QVariant(3) << 0;
    QTest::newRow("invalid") << QVariant(QStringLiteral("invalid")) << 0;
}

void TestLaunchUi::preference_persistence()
{
    QFETCH(QVariant, stored);
    QFETCH(int, expected);
    QSettings persisted(Settings::getConfigPath() + QStringLiteral("/SpeedCrunch.ini"), QSettings::IniFormat);
    const QString key = QStringLiteral("SpeedCrunch/General/SubsequentLaunchAction");
    if (stored.isValid())
        persisted.setValue(key, stored);
    else
        persisted.remove(key);
    persisted.sync();
    Settings* settings = Settings::instance();
    settings->load();
    QCOMPARE(int(settings->subsequentLaunchAction), expected);
    settings->save();
    persisted.sync();
    QCOMPARE(persisted.value(key).toInt(), expected);
    QVERIFY(settings->resetToDefaults());
    QCOMPARE(settings->subsequentLaunchAction, Settings::ActivateExistingWindow);
}

void TestLaunchUi::menu_is_exclusive_shared_and_resettable()
{
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    MainWindow first(false);
    MainWindow second(false);
    QMenu* menu = first.findChild<QMenu*>(QStringLiteral("SubsequentLaunchMenu"));
    QVERIFY(menu);
    QCOMPARE(menu->title(), QStringLiteral("On Subsequent Launch"));
    QCOMPARE(menu->actions().size(), 3);
    QVERIFY(launchAction(first, 0)->isChecked());
    for (int choice : {1, 2, 0, 2}) {
        launchAction(first, choice)->trigger();
        QCOMPARE(int(guard.settings->subsequentLaunchAction), choice);
        guard.settings->load();
        QCOMPARE(int(guard.settings->subsequentLaunchAction), choice);
        for (int index = 0; index != 3; ++index) {
            QCOMPARE(launchAction(first, index)->isChecked(), index == choice);
            QCOMPARE(launchAction(second, index)->isChecked(), index == choice);
        }
    }
    MainWindow third(false);
    QVERIFY(launchAction(third, 2)->isChecked());
    QTimer::singleShot(0, &first, []() {
        auto* dialog = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        QVERIFY(dialog);
        dialog->done(QMessageBox::Yes);
    });
    QVERIFY(QMetaObject::invokeMethod(&first, "resetAllSettings"));
    for (MainWindow* window : {&first, &second, &third}) {
        QVERIFY(launchAction(*window, 0)->isChecked());
        QVERIFY(!launchAction(*window, 2)->isChecked());
    }
}

void TestLaunchUi::initial_launch_ignores_preference_data()
{
    QTest::addColumn<int>("choice");
    for (int choice = 0; choice != 3; ++choice)
        QTest::addRow("%d", choice) << choice;
}

void TestLaunchUi::initial_launch_ignores_preference()
{
    QFETCH(int, choice);
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    guard.settings->subsequentLaunchAction = Settings::SubsequentLaunchAction(choice);
    LaunchController controller;
    MainWindow window;
    window.show();
    controller.setInitialWindow(&window);
    QTRY_VERIFY(!MainWindow::sessionRestoreInProgress());
    QTest::qWait(30);
    QCOMPARE(mainWindowCount(), 1);
    QCOMPARE(tabBarForDisplay(window.findChild<ResultDisplay*>())->count(), 1);
}

void TestLaunchUi::launch_actions_data()
{
    QTest::addColumn<int>("choice");
    QTest::addColumn<int>("requests");
    for (int choice = 0; choice != 3; ++choice) {
        for (int requests : {1, 3})
            QTest::addRow("%d-%d", choice, requests) << choice << requests;
    }
}

void TestLaunchUi::launch_actions()
{
    QFETCH(int, choice);
    QFETCH(int, requests);
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    guard.settings->subsequentLaunchAction = Settings::SubsequentLaunchAction(choice);
    LaunchController controller;
    MainWindow window(false);
    window.show();
    controller.setInitialWindow(&window);
    Editor* editor = window.findChild<Editor*>();
    ResultDisplay* display = window.findChild<ResultDisplay*>();
    const Session* original = display->session();
    editor->setText(QStringLiteral("123 +"));
    QSignalSpy handled(&controller, &LaunchController::requestHandled);
    for (int index = 0; index != requests; ++index)
        controller.request();
    QTRY_COMPARE(handled.size(), requests);
    QCOMPARE(mainWindowCount(), choice == 1 ? requests + 1 : 1);
    QCOMPARE(tabBarForDisplay(display)->count(), choice == 2 ? requests + 1 : 1);
    QCOMPARE(original->editorText(), QStringLiteral("123 +"));
    if (choice == 0) {
        QCOMPARE(display->session(), original);
        QCOMPARE(editor->text(), QStringLiteral("123 +"));
    } else if (choice == 2) {
        QVERIFY(display->session() != original);
        QVERIFY(editor->text().isEmpty());
    }
    if (choice == 1) {
        for (QWidget* widget : QApplication::topLevelWidgets()) {
            auto* created = qobject_cast<MainWindow*>(widget);
            if (created && created != &window) {
                QCOMPARE(tabBarForDisplay(created->findChild<ResultDisplay*>())->count(), 1);
                QVERIFY(created->findChild<Editor*>()->text().isEmpty());
            }
        }
    }
}

void TestLaunchUi::launches_target_last_active_window_and_pane()
{
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    guard.settings->subsequentLaunchAction = Settings::OpenNewTab;
    LaunchController controller;
    MainWindow first(false);
    MainWindow second(false);
    first.show();
    second.show();
    controller.setInitialWindow(&first);
    QVERIFY(QMetaObject::invokeMethod(&second, "splitActivePaneRight"));
    const QList<Editor*> editors = second.findChildren<Editor*>();
    QCOMPARE(editors.size(), 2);
    second.activateWindow();
    QTRY_VERIFY(second.isActiveWindow());
    QTest::mouseClick(editors.last(), Qt::LeftButton);
    QTRY_VERIFY(editors.last()->hasFocus());
    ResultDisplay* target = editors.last()->parentWidget()->findChild<ResultDisplay*>();
    QVERIFY(target);
    QSignalSpy handled(&controller, &LaunchController::requestHandled);
    controller.request();
    QTRY_COMPARE(handled.size(), 1);
    QCOMPARE(qvariant_cast<MainWindow*>(handled.first().first()), &second);
    QCOMPARE(tabBarForDisplay(target)->count(), 2);
    QCOMPARE(tabBarForDisplay(first.findChild<ResultDisplay*>())->count(), 1);
}

void TestLaunchUi::launches_wait_for_initial_window_and_session_restore()
{
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    {
        MainWindow saved(false);
        saved.findChild<Editor*>()->setText(QStringLiteral("saved +"));
        saved.persistSessionAndSettingsForShutdown();
    }
    guard.settings->subsequentLaunchAction = Settings::OpenNewTab;
    LaunchController controller;
    QSignalSpy handled(&controller, &LaunchController::requestHandled);
    controller.request();
    controller.request();
    QTest::qWait(20);
    QCOMPARE(handled.size(), 0);
    MainWindow restored;
    restored.show();
    controller.setInitialWindow(&restored);
    QVERIFY(MainWindow::sessionRestoreInProgress());
    QCOMPARE(handled.size(), 0);
    QTRY_COMPARE(handled.size(), 2);
    QVERIFY(!MainWindow::sessionRestoreInProgress());
    ResultDisplay* display = restored.findChild<ResultDisplay*>();
    QTabBar* tabs = tabBarForDisplay(display);
    QCOMPARE(tabs->count(), 3);
    tabs->setCurrentIndex(0);
    QCOMPARE(restored.findChild<Editor*>()->text(), QStringLiteral("saved +"));
}

void TestLaunchUi::launches_wait_for_modal_dialog()
{
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    guard.settings->subsequentLaunchAction = Settings::OpenNewTab;
    LaunchController controller;
    MainWindow window(false);
    window.show();
    controller.setInitialWindow(&window);
    QDialog dialog(&window);
    dialog.setWindowModality(Qt::ApplicationModal);
    dialog.show();
    QTRY_COMPARE(QApplication::activeModalWidget(), &dialog);
    QSignalSpy handled(&controller, &LaunchController::requestHandled);
    controller.request();
    QTest::qWait(80);
    QCOMPARE(handled.size(), 0);
    dialog.accept();
    QTRY_COMPARE(handled.size(), 1);
    QCOMPARE(tabBarForDisplay(window.findChild<ResultDisplay*>())->count(), 2);
}

void TestLaunchUi::shutdown_discards_queued_launches()
{
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    guard.settings->subsequentLaunchAction = Settings::OpenNewWindow;
    LaunchController controller;
    MainWindow window(false);
    window.show();
    controller.setInitialWindow(&window);
    QSignalSpy handled(&controller, &LaunchController::requestHandled);
    controller.request();
    qApp->setProperty("speedcrunchShutdownInProgress", true);
    controller.request();
    QTest::qWait(30);
    QCOMPARE(handled.size(), 0);
    QCOMPARE(mainWindowCount(), 1);
}

void TestLaunchUi::activate_choice_handles_modal_dialog_immediately()
{
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    LaunchController controller;
    MainWindow window(false);
    window.show();
    controller.setInitialWindow(&window);
    QDialog dialog(&window);
    dialog.setWindowModality(Qt::ApplicationModal);
    dialog.show();
    QTRY_COMPARE(QApplication::activeModalWidget(), &dialog);
    QSignalSpy handled(&controller, &LaunchController::requestHandled);
    controller.request();
    QTRY_COMPARE(handled.size(), 1);
    QVERIFY(dialog.isVisible());
    QCOMPARE(tabBarForDisplay(window.findChild<ResultDisplay*>())->count(), 1);
}

void TestLaunchUi::normal_activation_does_not_create_sessions()
{
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    guard.settings->subsequentLaunchAction = Settings::OpenNewWindow;
    LaunchController controller;
    MainWindow window(false);
    window.show();
    controller.setInitialWindow(&window);
    QEvent activate(QEvent::ApplicationActivate);
    QCoreApplication::sendEvent(qApp, &activate);
    window.activateWindow();
    QTest::qWait(30);
    QCOMPARE(mainWindowCount(), 1);
}

void TestLaunchUi::request_encoding_data()
{
    QTest::addColumn<QByteArray>("token");
    QTest::addColumn<QByteArray>("startupId");
    QTest::newRow("legacy") << QByteArray() << QByteArray();
    QTest::newRow("wayland") << QByteArray("token-with-spaces\"and\nnewlines") << QByteArray();
    QTest::newRow("x11") << QByteArray() << QByteArray("launcher_TIME1234");
    QTest::newRow("both") << QByteArray("token") << QByteArray("launcher_TIME4294967295");
}

void TestLaunchUi::request_encoding()
{
    QFETCH(QByteArray, token);
    QFETCH(QByteArray, startupId);
    const LaunchRequest request{token, startupId};
    const QByteArray message = request.encode();
    QVERIFY(message.endsWith('\n'));
    QCOMPARE(message.count('\n'), 1);
    LaunchRequest decoded;
    QVERIFY(LaunchRequest::decode(message, &decoded));
    QCOMPARE(decoded.activationToken, token);
    QCOMPARE(decoded.startupId, startupId);
    QCOMPARE(decoded.requestId, request.requestId);
    QVERIFY(LaunchRequest::decode(QByteArray("activate\n"), &decoded));
    QVERIFY(decoded.requestId.isEmpty());
}

void TestLaunchUi::invalid_requests_data()
{
    QTest::addColumn<QByteArray>("message");
    QTest::newRow("unknown") << QByteArray("new-window\n");
    QTest::newRow("malformed-json") << QByteArray("activate {\n");
    QTest::newRow("array") << QByteArray("activate []\n");
    QTest::newRow("wrong-token-type") << QByteArray("activate {\"token\":42}\n");
    QTest::newRow("oversized") << QByteArray(16385, 'x');
}

void TestLaunchUi::invalid_requests()
{
    QFETCH(QByteArray, message);
    LaunchRequest request;
    QVERIFY(!LaunchRequest::decode(message, &request));
}

void TestLaunchUi::ipc_preserves_every_request_and_metadata()
{
    SingleInstance server;
    const QString name = serverName();
    QCOMPARE(server.start(name, {}), SingleInstance::PrimaryInstance);
    QList<LaunchRequest> received;
    connect(&server, &SingleInstance::launchRequested, this,
            [&received](const LaunchRequest& request) { received.append(request); });
    QList<QLocalSocket*> clients;
    const auto cleanup = qScopeGuard([&clients]() { qDeleteAll(clients); });
    for (int index = 0; index != 3; ++index) {
        auto* client = new QLocalSocket;
        clients.append(client);
        client->connectToServer(name);
        QVERIFY(client->waitForConnected(1000));
        client->write(LaunchRequest{QByteArray::number(index), QByteArray("launcher_TIME123")}.encode());
        client->flush();
    }
    QTRY_COMPARE(received.size(), 3);
    for (int index = 0; index != 3; ++index) {
        QCOMPARE(received.at(index).activationToken, QByteArray::number(index));
        QCOMPARE(received.at(index).startupId, QByteArray("launcher_TIME123"));
        QTRY_VERIFY(clients.at(index)->canReadLine());
        QCOMPARE(clients.at(index)->readLine(), QByteArray("accepted\n"));
    }
}

void TestLaunchUi::ipc_waits_for_complete_request_and_ignores_extra_lines()
{
    SingleInstance server;
    const QString name = serverName();
    QCOMPARE(server.start(name, {}), SingleInstance::PrimaryInstance);
    int received = 0;
    connect(&server, &SingleInstance::launchRequested, this, [&received]() { ++received; });
    QLocalSocket client;
    client.connectToServer(name);
    QVERIFY(client.waitForConnected(1000));
    client.write("acti");
    client.flush();
    QTest::qWait(20);
    QCOMPARE(received, 0);
    client.write("vate\nactivate\n");
    client.flush();
    QTRY_COMPARE(received, 1);
    QTRY_VERIFY(client.canReadLine());
    QCOMPARE(client.readLine(), QByteArray("accepted\n"));
}

void TestLaunchUi::ipc_rejects_shutdown()
{
    SingleInstance server;
    const QString name = serverName();
    QCOMPARE(server.start(name, {}), SingleInstance::PrimaryInstance);
    int received = 0;
    connect(&server, &SingleInstance::launchRequested, this, [&received]() { ++received; });
    qApp->setProperty("speedcrunchShutdownInProgress", true);
    QLocalSocket client;
    client.connectToServer(name);
    QVERIFY(client.waitForConnected(1000));
    client.write("activate\n");
    client.flush();
    QTRY_VERIFY(client.canReadLine());
    QCOMPARE(client.readLine(), QByteArray("shutting-down\n"));
    QCOMPARE(received, 0);
}

void TestLaunchUi::secondary_instance_exits_after_acceptance()
{
    SingleInstance server;
    const QString name = serverName();
    QCOMPARE(server.start(name, {}), SingleInstance::PrimaryInstance);
    int received = 0;
    connect(&server, &SingleInstance::launchRequested, this, [&received]() { ++received; });
    SingleInstance::StartResult result = SingleInstance::ServerUnavailable;
    QThread* thread = QThread::create([&result, name]() {
        SingleInstance secondary;
        result = secondary.start(name, {QByteArray("forwarded-token"), {}});
    });
    const auto join = qScopeGuard([thread]() { thread->wait(); delete thread; });
    thread->start();
    QTRY_VERIFY(thread->isFinished());
    thread->wait();
    QCOMPARE(result, SingleInstance::SecondaryInstance);
    QCOMPARE(received, 1);
}

void TestLaunchUi::activation_restores_minimized_window()
{
    QWidget window;
    window.showMinimized();
    QVERIFY(window.isMinimized());
    activateLaunchWindow(&window, {});
    QVERIFY(!window.isMinimized());
    QVERIFY(window.isVisible());
}

void TestLaunchUi::retries_do_not_duplicate_launches()
{
    SingleInstance server;
    const QString name = serverName();
    QCOMPARE(server.start(name, {}), SingleInstance::PrimaryInstance);
    int received = 0;
    connect(&server, &SingleInstance::launchRequested, this, [&received]() { ++received; });
    const LaunchRequest request;
    for (int attempt = 0; attempt != 2; ++attempt) {
        QLocalSocket client;
        client.connectToServer(name);
        QVERIFY(client.waitForConnected(1000));
        client.write(request.encode());
        client.flush();
        QTRY_VERIFY(client.canReadLine());
        QCOMPARE(client.readLine(), QByteArray("accepted\n"));
    }
    QCOMPARE(received, 1);
}

void TestLaunchUi::busy_startup_keeps_single_instance()
{
    SingleInstance server;
    const QString name = serverName();
    QCOMPARE(server.start(name, {}), SingleInstance::PrimaryInstance);
    int received = 0;
    connect(&server, &SingleInstance::launchRequested, this, [&received]() { ++received; });
    SingleInstance::StartResult result = SingleInstance::ServerUnavailable;
    QThread* thread = QThread::create([&result, name]() {
        SingleInstance secondary;
        result = secondary.start(name, {});
    });
    const auto join = qScopeGuard([thread]() { thread->wait(); delete thread; });
    thread->start();
    // Hold the GUI thread past the first acknowledgment timeout, as a slow
    // initial window construction would. The retry must reach the same server.
    QTest::qSleep(1100);
    QTRY_VERIFY(thread->isFinished());
    thread->wait();
    QCOMPARE(result, SingleInstance::SecondaryInstance);
    QCOMPARE(received, 1);
}

void TestLaunchUi::startup_lock_is_released_on_close()
{
    const QString name = serverName();
    SingleInstance first;
    QCOMPARE(first.start(name, {}), SingleInstance::PrimaryInstance);
    first.close();
    first.close();
    SingleInstance replacement;
    QCOMPARE(replacement.start(name, {}), SingleInstance::PrimaryInstance);
}

void TestLaunchUi::ipc_lock_process_helper()
{
    const QString name = qEnvironmentVariable("SPEEDCRUNCH_TEST_LOCK_SERVER");
    if (name.isEmpty()) {
        QTest::qSkip("Only run by the process recovery test", __FILE__, __LINE__);
        return;
    }
    SingleInstance server;
    QCOMPARE(server.start(name, {}), SingleInstance::PrimaryInstance);
    qInfo("singleton-helper-ready");
    QEventLoop loop;
    QTimer::singleShot(10000, &loop, &QEventLoop::quit);
    loop.exec();
}

void TestLaunchUi::startup_lock_recovers_after_process_death()
{
    const QString name = serverName();
    QProcess child;
    child.setProcessChannelMode(QProcess::MergedChannels);
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("SPEEDCRUNCH_TEST_LOCK_SERVER"), name);
    child.setProcessEnvironment(environment);
    const auto stopChild = qScopeGuard([&child] {
        if (child.state() != QProcess::NotRunning) {
            child.kill();
            child.waitForFinished(3000);
        }
    });
    child.start(QCoreApplication::applicationFilePath(), {QStringLiteral("ipc_lock_process_helper")});
    QVERIFY(child.waitForStarted(3000));
    QByteArray output;
    const auto ready = [&] {
        output += child.readAll();
        return output.contains("singleton-helper-ready");
    };
    QTRY_VERIFY_WITH_TIMEOUT(ready(), 5000);
    child.kill();
    QVERIFY(child.waitForFinished(3000));
    SingleInstance replacement;
    QCOMPARE(replacement.start(name, {}), SingleInstance::PrimaryInstance);
}

#ifdef Q_OS_WIN
void TestLaunchUi::startup_lock_excludes_legacy_lock_files()
{
    const QString name = serverName();
    const QByteArray hash = QCryptographicHash::hash(name.toUtf8(), QCryptographicHash::Sha256).toHex();
    QLockFile legacy(QDir::temp().filePath(
        QStringLiteral("speedcrunch-instance-%1.lock").arg(QString::fromLatin1(hash))));
    legacy.setStaleLockTime(0);
    SingleInstance server;
    QCOMPARE(server.start(name, {}), SingleInstance::PrimaryInstance);
    QVERIFY(!legacy.tryLock());
    QCOMPARE(legacy.error(), QLockFile::LockFailedError);
    server.close();
    QVERIFY(legacy.tryLock());
    SingleInstance::StartResult result = SingleInstance::ServerUnavailable;
    QThread* thread = QThread::create([&result, name] {
        SingleInstance contender;
        result = contender.start(name, {});
    });
    const auto join = qScopeGuard([thread] { thread->wait(); delete thread; });
    thread->start();
    // With no server to contact, a live older lock must block a new primary.
    QTRY_VERIFY_WITH_TIMEOUT(thread->isFinished(), 8000);
    QCOMPARE(result, SingleInstance::ServerUnavailable);
    legacy.unlock();
    QCOMPARE(server.start(name, {}), SingleInstance::PrimaryInstance);
}
#endif

#ifdef Q_OS_MACOS
void TestLaunchUi::native_reopen_follows_preference_data()
{
    initial_launch_ignores_preference_data();
}

void TestLaunchUi::native_reopen_follows_preference()
{
    QFETCH(int, choice);
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    guard.settings->subsequentLaunchAction = Settings::SubsequentLaunchAction(choice);
    LaunchController controller;
    MainWindow window(false);
    window.show();
    controller.setInitialWindow(&window);
    QSignalSpy handled(&controller, &LaunchController::requestHandled);
    QVERIFY(dispatchMacReopenEvent());
    QVERIFY(dispatchMacReopenEvent());
    QTRY_COMPARE(handled.size(), 2);
    QCOMPARE(mainWindowCount(), choice == 1 ? 3 : 1);
    QCOMPARE(tabBarForDisplay(window.findChild<ResultDisplay*>())->count(), choice == 2 ? 3 : 1);
}
#endif

void TestLaunchUi::new_tabs_use_unique_names_across_windows_data()
{
    QTest::addColumn<bool>("subsequentLaunch");
    QTest::newRow("subsequent-launch") << true;
    QTest::newRow("new-tab-action") << false;
}

void TestLaunchUi::new_tabs_use_unique_names_across_windows()
{
    QFETCH(bool, subsequentLaunch);
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    guard.settings->subsequentLaunchAction = Settings::OpenNewTab;
    LaunchController controller;
    MainWindow first(false);
    first.show();
    controller.setInitialWindow(&first);
    MainWindow* second = first.createNewSessionWindow();
    QVERIFY(second);
    const auto deleteSecond = qScopeGuard([second]() { delete second; });
    QVERIFY(QTest::qWaitForWindowExposed(&first));
    QVERIFY(QTest::qWaitForWindowActive(second));
    QSignalSpy handled(&controller, &LaunchController::requestHandled);
    int expectedTabs = 2;
    int expectedRequests = 0;
    for (int round = 0; round != 3; ++round) {
        for (MainWindow* window : {&first, second}) {
            window->activateWindow();
            QTRY_VERIFY(window->isActiveWindow());
            if (subsequentLaunch) {
                controller.request();
                ++expectedRequests;
                QTRY_COMPARE(handled.size(), expectedRequests);
                QCOMPARE(qvariant_cast<MainWindow*>(handled.last().first()), window);
            } else {
                QVERIFY(QMetaObject::invokeMethod(window, "showNewSessionDialog"));
            }
            ++expectedTabs;
            QSet<QString> names;
            int tabs = 0;
            for (MainWindow* owner : {&first, second}) {
                QTabBar* bar = tabBarForDisplay(owner->findChild<ResultDisplay*>());
                QVERIFY(bar);
                for (int index = 0; index != bar->count(); ++index) {
                    const QString name = bar->tabText(index);
                    QVERIFY2(!names.contains(name.toCaseFolded()), qPrintable(name));
                    names.insert(name.toCaseFolded());
                    // Empty tabs have no session file to reserve their name.
                    QVERIFY(!QFileInfo::exists(storage.path(name)));
                    ++tabs;
                }
            }
            QCOMPARE(tabs, expectedTabs);
        }
    }
}

int main(int argc, char** argv)
{
    return UiTestFixture::run<TestLaunchUi>(argc, argv);
}

#include "testlaunchui.moc"
