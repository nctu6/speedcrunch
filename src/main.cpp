// SPDX-FileCopyrightText: 2007-2010, 2013-2014, 2016, 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later


#include "gui/mainwindow.h"
#include "gui/diagnosticapplication.h"
#include "gui/startupfontpreparation.h"
#include "core/settings.h"
#include "core/startupdiagnostics.h"
#include "core/singleinstance.h"
#include "gui/launchcontroller.h"

#include <QCoreApplication>
#include <QApplication>
#include <QColorSpace>
#include <QGuiApplication>
#include <QCryptographicHash>
#include <QAbstractSocket>
#include <QDebug>
#include <QMetaObject>
#include <QMessageBox>
#include <QSurfaceFormat>
#include <QEvent>
#include <QThread>
#include <QTimer>
#include <QVariant>

#include <atomic>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#ifdef Q_OS_UNIX
#include <QSocketNotifier>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace {

// Dynamic property name mirrored by MainWindow. It lets the GUI layer observe
// shutdown state even when quit starts from the application/singleton layer.
constexpr const char* kShutdownInProgressProperty = "speedcrunchShutdownInProgress";

QString singletonServerName()
{
    const QString scope = QCoreApplication::applicationFilePath()
        + QLatin1Char('|')
        + Settings::getConfigPath();
    const QByteArray hash = QCryptographicHash::hash(scope.toUtf8(), QCryptographicHash::Sha1).toHex();
    return QStringLiteral("speedcrunch-single-instance-%1").arg(QString::fromLatin1(hash.left(24)));
}

MainWindow* g_mainWindow = 0;
std::atomic_bool g_eventLoopRunning(false);
std::atomic_bool g_shutdownInProgress(false);

bool shutdownInProgress()
{
    const QCoreApplication* application = QCoreApplication::instance();
    // Keep both an atomic flag and the QApplication property: signal handlers
    // and singleton callbacks use the atomic, while MainWindow close handling
    // reads the property.
    return g_shutdownInProgress.load()
        || (application && application->property(kShutdownInProgressProperty).toBool());
}

class ShutdownEventFilter : public QObject {
public:
    using QObject::QObject;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        Q_UNUSED(watched);
        if (event != nullptr && event->type() == QEvent::Quit) {
            // Mark shutdown as early as possible. aboutToQuit can be too late
            // for nested close events that decide whether to save child-window
            // layout changes.
            g_shutdownInProgress.store(true);
            if (QCoreApplication::instance())
                QCoreApplication::instance()->setProperty(kShutdownInProgressProperty, true);
        }
        return false;
    }
};

void persistAndQuit()
{
    // Console/signal shutdown paths bypass the normal window-close entry point,
    // so they must publish shutdown state before asking MainWindow to persist.
    g_shutdownInProgress.store(true);
    if (QCoreApplication::instance())
        QCoreApplication::instance()->setProperty(kShutdownInProgressProperty, true);
    if (g_mainWindow)
        g_mainWindow->persistSessionAndSettingsForShutdown();
    if (QCoreApplication::instance())
        QCoreApplication::quit();
}

void requestGracefulShutdown(Qt::ConnectionType connectionType = Qt::QueuedConnection)
{
    QCoreApplication* application = QCoreApplication::instance();
    if (!application)
        return;

    if (QThread::currentThread() == application->thread()) {
        persistAndQuit();
        return;
    }

    QMetaObject::invokeMethod(application, []() {
        persistAndQuit();
    }, connectionType);
}

#ifdef Q_OS_UNIX
int g_unixSignalFds[2] = { -1, -1 };

void handleUnixSignal(int signalNumber)
{
    const char signalCode = static_cast<char>(signalNumber);
    const ssize_t written = ::write(g_unixSignalFds[0], &signalCode, sizeof(signalCode));
    Q_UNUSED(written);
}

bool installUnixSignalHandler(int signalNumber)
{
    struct sigaction action = {};
    action.sa_handler = handleUnixSignal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    return ::sigaction(signalNumber, &action, 0) == 0;
}

bool setupUnixTerminationSignalHandlers()
{
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, g_unixSignalFds) != 0)
        return false;

    const bool ok = installUnixSignalHandler(SIGTERM)
        && installUnixSignalHandler(SIGINT)
        && installUnixSignalHandler(SIGHUP);
    if (!ok) {
        ::close(g_unixSignalFds[0]);
        ::close(g_unixSignalFds[1]);
        g_unixSignalFds[0] = -1;
        g_unixSignalFds[1] = -1;
    }

    return ok;
}
#endif

#ifdef Q_OS_WIN
BOOL WINAPI handleWindowsConsoleControl(DWORD controlType)
{
    switch (controlType) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        requestGracefulShutdown(g_eventLoopRunning.load()
            ? Qt::BlockingQueuedConnection
            : Qt::QueuedConnection);
        return TRUE;
    default:
        return FALSE;
    }
}
#endif

}

int main(int argc, char* argv[])
{
    // Capture launcher metadata before Qt initializes the platform plugin.
    const LaunchRequest launchRequest = LaunchRequest::fromEnvironment();
    StartupDiagnostics::begin();
    StartupDiagnostics::Scope timing("Configure surface format");
    QSurfaceFormat surfaceFormat = QSurfaceFormat::defaultFormat();
    surfaceFormat.setColorSpace(QColorSpace::SRgb);
    QSurfaceFormat::setDefaultFormat(surfaceFormat);

    timing.next("Initialize QApplication and platform plugin");
    DiagnosticApplication application(argc, argv);
    timing.next("Configure application identity");
    ShutdownEventFilter shutdownEventFilter(&application);
    application.installEventFilter(&shutdownEventFilter);

    QCoreApplication::setApplicationName("SpeedCrunch");
    QCoreApplication::setOrganizationDomain("speedcrunch.org");
    QGuiApplication::setDesktopFileName("org.speedcrunch.SpeedCrunch");

    QString storageError;
    if (!Settings::preparePortableStorage(&storageError)) {
        // Let macOS finish launching before opening a native modal alert.
        QTimer::singleShot(0, &application, [&]() {
            QMessageBox message(QMessageBox::Critical, QObject::tr("Portable Data Folder Unavailable"),
                QObject::tr("SpeedCrunch cannot prepare its portable data folder:\n%1\n\n%2\n\n"
                            "Use Finder to move SpeedCrunch.app and any SpeedCrunch-data folder "
                            "to a writable location, then open the app again.")
                    .arg(Settings::getConfigPath(), storageError));
            message.setTextFormat(Qt::PlainText);
            message.exec();
            application.exit(1);
        });
        return application.exec();
    }

    timing.next("Load application settings (total)");
    Settings::instance();
    timing.next("Install shutdown handlers");
    SingleInstance singletonServer;
    LaunchController launchController;
    QObject::connect(&singletonServer, &SingleInstance::launchRequested,
                     &launchController, &LaunchController::request);

    QObject::connect(&application, &QCoreApplication::aboutToQuit, &application, [&]() {
        // Stop accepting single-instance activations during teardown. A second
        // launch that races with quit should create the replacement instance,
        // not send activation to a process that is already saving and exiting.
        g_shutdownInProgress.store(true);
        application.setProperty(kShutdownInProgressProperty, true);
        singletonServer.close();
        if (g_mainWindow)
            g_mainWindow->persistSessionAndSettingsForShutdown();
#ifdef Q_OS_UNIX
        if (g_unixSignalFds[0] != -1) {
            ::close(g_unixSignalFds[0]);
            g_unixSignalFds[0] = -1;
        }
        if (g_unixSignalFds[1] != -1) {
            ::close(g_unixSignalFds[1]);
            g_unixSignalFds[1] = -1;
        }
#endif
    });

#ifdef Q_OS_UNIX
    QSocketNotifier* unixSignalNotifier = 0;
    if (setupUnixTerminationSignalHandlers()) {
        unixSignalNotifier = new QSocketNotifier(g_unixSignalFds[1], QSocketNotifier::Read, &application);
        QObject::connect(unixSignalNotifier, &QSocketNotifier::activated, &application, [&]() {
            char signalCode = 0;
            const ssize_t bytesRead = ::read(g_unixSignalFds[1], &signalCode, sizeof(signalCode));
            if (bytesRead > 0)
                requestGracefulShutdown(Qt::QueuedConnection);
        });
    } else {
        qWarning() << "Could not install Unix termination signal handlers.";
    }
#endif

#ifdef Q_OS_WIN
    if (!SetConsoleCtrlHandler(handleWindowsConsoleControl, TRUE))
        qWarning() << "Could not install Windows console termination handler.";
#endif

    timing.next("Check and register single instance");
    const QString serverName = singletonServerName();
    const SingleInstance::StartResult instanceResult = singletonServer.start(serverName, launchRequest);
    if (instanceResult == SingleInstance::SecondaryInstance)
        return 0;
    if (instanceResult == SingleInstance::ServerUnavailable) {
        QTimer::singleShot(0, &application, [&application]() {
            QMessageBox::warning(nullptr, QObject::tr("Cannot Start SpeedCrunch"),
                QObject::tr("SpeedCrunch cannot contact the running instance or start its local server. "
                            "Wait a moment and try again."));
            application.exit(1);
        });
        return application.exec();
    }

    timing.next("Initialize application widget style");
    MainWindow::initializeApplicationStyle();
    StartupFontPreparation fontPreparation;
    timing.next("Initialize main window (total)");
    MainWindow window;
    fontPreparation.wait();
    g_mainWindow = &window;
    timing.next("Schedule main window display");
    window.showWhenReady();

    launchController.setInitialWindow(&window);

    QObject::connect(&application, &QGuiApplication::lastWindowClosed, &application, [&]() {
        if (!shutdownInProgress())
            application.quit();
    });

    timing.finish();
    g_eventLoopRunning.store(true);
    const int result = application.exec();
    g_eventLoopRunning.store(false);
    g_mainWindow = 0;
    return result;
}
