// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later


#include "gui/versioncheck.h"
#include "core/startupdiagnostics.h"
#include "core/settings.h"

#include <QApplication>
#include <QDateTime>
#include <QDeadlineTimer>
#include <QDebug>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QEvent>
#include <QLabel>
#include <QLoggingCategory>
#include <QMutex>
#include <QMutexLocker>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QVector>

#include <memory>

namespace {

static const char WEBSITE_URL[] = "https://speedcrunch.org";
static const char VERSION_URL[] = "https://speedcrunch.org/version";
static const char LAST_CHECK_KEY[] = "updates/lastVersionCheckUtcMsecs";
static const char LAST_FETCHED_VERSION_KEY[] = "updates/lastFetchedVersion";
static const char LAST_NOTIFIED_VERSION_KEY[] = "updates/lastNotifiedVersion";
const qint64 CHECK_INTERVAL_MSECS = 24LL * 60LL * 60LL * 1000LL;
constexpr qint64 MAX_VERSION_RESPONSE_BYTES = 1024;
constexpr int VERSION_REQUEST_TIMEOUT_MSECS = 5000;
Q_LOGGING_CATEGORY(lcVersionCheck, "speedcrunch.updatecheck")

struct VersionResponse {
    QByteArray payload;
    QDeadlineTimer deadline{VERSION_REQUEST_TIMEOUT_MSECS, Qt::PreciseTimer};
    bool tooLarge = false;
    bool timedOut = false;

    void readAvailable(QNetworkReply* reply)
    {
        if (tooLarge || reply->bytesAvailable() == 0)
            return;
        const qint64 remaining = MAX_VERSION_RESPONSE_BYTES - payload.size();
        // Read one extra byte to detect overflow even without Content-Length.
        const QByteArray chunk = reply->read(remaining + 1);
        if (chunk.size() > remaining) {
            tooLarge = true;
            payload.clear();
            return;
        }
        payload.append(chunk);
    }
};

bool compareVersions(const QString& left, const QString& right, int* result);
bool tryParseVersion(const QString& version, QVector<qulonglong>* components);

QMutex& updateScheduleMutex()
{
    static QMutex mutex;
    return mutex;
}

QSettings updateCheckSettings()
{
#ifdef SPEEDCRUNCH_PORTABLE
    return QSettings(QDir(Settings::getConfigPath()).filePath(QStringLiteral("updates.ini")),
                     QSettings::IniFormat);
#else
    return QSettings();
#endif
}

static QDebug versionCheckDebug()
{
    return QMessageLogger(QT_MESSAGELOG_FILE,
                          QT_MESSAGELOG_LINE,
                          QT_MESSAGELOG_FUNC,
                          lcVersionCheck().categoryName()).debug();
}

void deleteStoppedThread(QThread* thread)
{
    // finished() precedes deferred deletion and thread-local cleanup. Deleting
    // a thread still in that stage can make its destructor wait indefinitely.
    if (thread->wait(0)) {
        delete thread;
        return;
    }
    QTimer::singleShot(10, thread, [thread]() { deleteStoppedThread(thread); });
}

}

class VersionCheckWorker : public QObject {
    Q_OBJECT
public:
    void startRequest(VersionCheck::CheckTrigger trigger)
    {
        if (QThread::currentThread()->isInterruptionRequested())
            return;
        {
            // Claim the automatic attempt before another window checks the schedule.
            QMutexLocker lock(&updateScheduleMutex());
            if (trigger == VersionCheck::CheckTriggerAutomatic && !isCheckDue()) {
                versionCheckDebug() << "Update check skipped: next check not due yet.";
                emit requestSkipped();
                return;
            }
            markCheckAttempted();
        }
        if (QThread::currentThread()->isInterruptionRequested())
            return;
        versionCheckDebug() << "Starting update check request to" << QString::fromLatin1(VERSION_URL)
                            << "; trigger =" << (trigger == VersionCheck::CheckTriggerManual ? "manual" : "automatic");
        if (!m_manager) {
            StartupDiagnostics::Scope timing("Initialize update network manager");
            m_manager = new QNetworkAccessManager(this);
        }
        if (QThread::currentThread()->isInterruptionRequested())
            return;
        QNetworkRequest request(QUrl(QString::fromLatin1(VERSION_URL)));
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        request.setTransferTimeout(VERSION_REQUEST_TIMEOUT_MSECS);
        const auto response = std::make_shared<VersionResponse>();
        QNetworkReply* reply;
        {
            StartupDiagnostics::Scope timing("Dispatch update network request");
            reply = m_manager->get(request);
        }
        reply->setReadBufferSize(MAX_VERSION_RESPONSE_BYTES + 1);
        auto* timeout = new QTimer(reply);
        timeout->setSingleShot(true);
        timeout->setTimerType(Qt::PreciseTimer);
        connect(reply, &QNetworkReply::finished, this, [this, reply, trigger, response, timeout]() {
            timeout->stop();
            response->timedOut = response->timedOut || response->deadline.hasExpired();
            response->readAvailable(reply);
            const auto error = response->tooLarge ? QNetworkReply::UnknownContentError
                : response->timedOut ? QNetworkReply::TimeoutError : reply->error();
            const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            const QString errorString = response->tooLarge
                ? QStringLiteral("Update response exceeds the 1024-byte limit.")
                : response->timedOut ? QStringLiteral("Update request exceeds the five-second deadline.")
                                    : reply->errorString();
            reply->deleteLater();
            QString latestVersion;
            int comparison = 0;
            bool comparable = false;
            bool notify = false;
            if (error == QNetworkReply::NoError) {
                latestVersion = QString::fromUtf8(response->payload).trimmed().section('\n', 0, 0).trimmed();
                comparable = compareVersions(latestVersion, QString::fromLatin1(SPEEDCRUNCH_VERSION), &comparison);
                storeFetchedVersion(latestVersion);
                if (comparable && comparison > 0)
                    notify = trigger == VersionCheck::CheckTriggerManual || shouldNotifyForVersion(latestVersion);
            }
            emit requestFinished(error, latestVersion, httpStatus, errorString, comparable, comparison, notify);
        });
        connect(timeout, &QTimer::timeout, this, [reply, response]() {
            response->timedOut = true;
            response->payload.clear();
            reply->abort();
        });
        connect(reply, &QNetworkReply::readyRead, this, [reply, response]() {
            response->readAvailable(reply);
            if (response->tooLarge)
                reply->abort();
        });
        connect(reply, &QNetworkReply::metaDataChanged, this, [reply, response]() {
            if (!response->tooLarge
                && reply->header(QNetworkRequest::ContentLengthHeader).toLongLong() > MAX_VERSION_RESPONSE_BYTES) {
                response->tooLarge = true;
                response->payload.clear();
                reply->abort();
            }
        });
        // Keep one deadline across redirects and incoming data.
        timeout->start(response->deadline.remainingTime());
    }

    void markVersionNotified(const QString& version);

signals:
    void requestDispatched();
    void requestSkipped();
    void requestFinished(QNetworkReply::NetworkError error, const QString& latestVersion,
                         int httpStatus, const QString& errorString,
                         bool comparable, int comparison, bool shouldNotify);

private:
    void markCheckAttempted();
    bool isCheckDue() const;
    void storeFetchedVersion(const QString& version);
    bool shouldNotifyForVersion(const QString& version) const;
    QNetworkAccessManager* m_manager = nullptr;
};

VersionCheck::VersionCheck(QWidget* parentWindow, QObject* parent)
    : QObject(parent),
      m_parentWindow(parentWindow)
{
}

VersionCheck::~VersionCheck()
{
    if (m_networkThread) {
        QThread* thread = m_networkThread;
        thread->requestInterruption();
        // Finish notification writes already queued before stopping the worker.
        QMetaObject::invokeMethod(m_networkWorker, [thread]() { thread->quit(); }, Qt::QueuedConnection);
    }
}

void VersionCheck::checkForUpdateAfterFirstPaint()
{
    if (qEnvironmentVariableIsSet("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK")
        || m_automaticCheckScheduled || !m_parentWindow)
        return;
    m_automaticCheckScheduled = true;
    qApp->installEventFilter(this);
}

bool VersionCheck::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::Paint) {
        QWidget* widget = qobject_cast<QWidget*>(watched);
        if (widget && widget->isVisible() && widget->window() == m_parentWindow) {
            qApp->removeEventFilter(this);
            const auto startupTask = StartupDiagnostics::task("Deferred automatic update check (async total)");
            // Let the paint finish and its diagnostics callback run before dispatch.
            QTimer::singleShot(0, this, [this, startupTask]() {
                QTimer::singleShot(0, this, [this, startupTask]() {
                    StartupDiagnostics::Scope timing("Queue automatic update check");
                    checkForUpdateIfDue();
                });
            });
        }
    }
    return QObject::eventFilter(watched, event);
}

void VersionCheck::checkForUpdateIfDue()
{
    if (qEnvironmentVariableIsSet("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK"))
        return;

    if (m_requestPending) {
        versionCheckDebug() << "Update check skipped: request already in progress.";
        return;
    }
    startRequest(CheckTriggerAutomatic);
}

void VersionCheck::checkForUpdateNow()
{
    if (m_requestPending) {
        versionCheckDebug() << "Manual update check requested while another request is running.";
        showVerificationFailedDialog(m_parentWindow);
        return;
    }

    startRequest(CheckTriggerManual);
}

void VersionCheck::handleReplyFinished(QNetworkReply::NetworkError error, const QString& latestVersion,
                                      int httpStatus, const QString& errorString,
                                      bool comparable, int comparison, bool shouldNotify)
{
    m_requestPending = false;

    versionCheckDebug() << "Update check response received."
                            << "HTTP status:" << httpStatus;

    const bool ok = (error == QNetworkReply::NoError);

    if (!ok) {
        versionCheckDebug() << "Update check failed:" << errorString;
        if (m_trigger == CheckTriggerManual) {
            if (isConnectivityError(error))
                showNoConnectivityDialog(m_parentWindow);
            else
                showVerificationFailedDialog(m_parentWindow);
        }
        return;
    }

    const QString currentVersion = QString::fromLatin1(SPEEDCRUNCH_VERSION);

    versionCheckDebug() << "Parsed versions:"
                            << "remote =" << latestVersion
                            << ", local =" << currentVersion
                            << ", comparable =" << comparable
                            << ", compare result =" << comparison;

    if (!comparable) {
        versionCheckDebug() << "Could not compare versions.";
        if (m_trigger == CheckTriggerManual)
            showVerificationFailedDialog(m_parentWindow);
    } else if (comparison > 0) {
        if (shouldNotify) {
            versionCheckDebug() << "Newer version found; showing update dialog.";
            showUpdateDialog(m_parentWindow, latestVersion,
                             m_trigger == CheckTriggerAutomatic ? m_networkWorker : nullptr);
        } else {
            versionCheckDebug() << "Newer version found, but user was already notified for this version.";
        }
    } else {
        versionCheckDebug() << "No newer version available.";
        if (m_trigger == CheckTriggerManual)
            showUpToDateDialog(m_parentWindow, currentVersion);
    }
}

namespace {
bool compareVersions(const QString& left, const QString& right, int* result)
{
    if (!result)
        return false;
    *result = 0;

    QVector<qulonglong> leftComponents;
    QVector<qulonglong> rightComponents;
    if (!tryParseVersion(left, &leftComponents) || !tryParseVersion(right, &rightComponents))
        return false;

    const int count = qMax(leftComponents.size(), rightComponents.size());
    for (int i = 0; i < count; ++i) {
        const qulonglong leftPart = (i < leftComponents.size()) ? leftComponents.at(i) : 0;
        const qulonglong rightPart = (i < rightComponents.size()) ? rightComponents.at(i) : 0;
        if (leftPart < rightPart)
            *result = -1;
        if (leftPart > rightPart)
            *result = 1;
        if (*result != 0)
            return true;
    }

    return true;
}

bool tryParseVersion(const QString& version, QVector<qulonglong>* components)
{
    if (!components)
        return false;
    components->clear();

    const QString trimmed = version.trimmed();
    if (trimmed.isEmpty())
        return false;

    const QStringList parts = trimmed.split('.', Qt::KeepEmptyParts);
    for (const QString& part : parts) {
        if (part.isEmpty())
            return false;

        for (const QChar ch : part) {
            if (!ch.isDigit())
                return false;
        }

        bool ok = false;
        const qulonglong value = part.toULongLong(&ok);
        if (!ok)
            return false;
        components->append(value);
    }

    return !components->isEmpty();
}
}

void VersionCheck::showUpdateDialog(QWidget* parent, const QString& latestVersion,
                                   VersionCheckWorker* notificationWorker)
{
    versionCheckDebug() << "Displaying update dialog for version" << latestVersion;

    QDialog dialog(parent);
    dialog.setWindowTitle(QObject::tr("Update Available"));

    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    QLabel* text = new QLabel(
        QObject::tr("A newer version (%1) is available.<br/><a href=\"%2\">Visit website to download.</a>")
            .arg(latestVersion, QString::fromLatin1(WEBSITE_URL)),
        &dialog);
    text->setTextFormat(Qt::RichText);
    text->setTextInteractionFlags(Qt::TextBrowserInteraction);
    text->setOpenExternalLinks(false);
    QObject::connect(text, &QLabel::linkActivated, &dialog, [](const QString& url) {
        QDesktopServices::openUrl(QUrl(url));
    });
    layout->addWidget(text);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, &dialog);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    layout->addWidget(buttons);

    if (notificationWorker) {
        QObject::connect(&dialog, &QDialog::finished, notificationWorker,
                         [notificationWorker, latestVersion]() {
            notificationWorker->markVersionNotified(latestVersion);
        }, Qt::QueuedConnection);
    }
    dialog.exec();
}

void VersionCheck::showUpToDateDialog(QWidget* parent, const QString& currentVersion)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(QObject::tr("Check for Updates"));

    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    QLabel* text = new QLabel(
        QObject::tr("SpeedCrunch %1 is up to date.").arg(currentVersion),
        &dialog);
    text->setWordWrap(true);
    layout->addWidget(text);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, &dialog);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    layout->addWidget(buttons);

    dialog.exec();
}

void VersionCheck::showNoConnectivityDialog(QWidget* parent)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(QObject::tr("Check for Updates"));

    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    QLabel* text = new QLabel(
        QObject::tr("No connectivity. Please check your internet connection and try again."),
        &dialog);
    text->setWordWrap(true);
    layout->addWidget(text);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, &dialog);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    layout->addWidget(buttons);

    dialog.exec();
}

void VersionCheck::showVerificationFailedDialog(QWidget* parent)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(QObject::tr("Check for Updates"));

    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    QLabel* text = new QLabel(
        QObject::tr("Could not verify the latest available version."),
        &dialog);
    text->setWordWrap(true);
    layout->addWidget(text);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, &dialog);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    layout->addWidget(buttons);

    dialog.exec();
}

bool VersionCheck::isConnectivityError(QNetworkReply::NetworkError error)
{
    return error == QNetworkReply::ConnectionRefusedError
        || error == QNetworkReply::RemoteHostClosedError
        || error == QNetworkReply::HostNotFoundError
        || error == QNetworkReply::TimeoutError
        || error == QNetworkReply::OperationCanceledError
        || error == QNetworkReply::SslHandshakeFailedError
        || error == QNetworkReply::TemporaryNetworkFailureError
        || error == QNetworkReply::NetworkSessionFailedError
        || error == QNetworkReply::BackgroundRequestNotAllowedError
        || error == QNetworkReply::TooManyRedirectsError
        || error == QNetworkReply::InsecureRedirectError
        || error == QNetworkReply::ProxyConnectionRefusedError
        || error == QNetworkReply::ProxyConnectionClosedError
        || error == QNetworkReply::ProxyNotFoundError
        || error == QNetworkReply::ProxyTimeoutError
        || error == QNetworkReply::ProxyAuthenticationRequiredError
        || error == QNetworkReply::UnknownNetworkError
        || error == QNetworkReply::UnknownProxyError;
}

void VersionCheckWorker::markCheckAttempted()
{
    StartupDiagnostics::Scope timing("Save update check timestamp");
    QSettings settings = updateCheckSettings();
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    settings.setValue(QString::fromLatin1(LAST_CHECK_KEY), now);
    versionCheckDebug() << "Recorded update check timestamp (UTC msecs):" << now;
}

bool VersionCheckWorker::isCheckDue() const
{
    StartupDiagnostics::Scope timing("Check update schedule");
    QSettings settings = updateCheckSettings();
    const qint64 lastCheck = settings.value(QString::fromLatin1(LAST_CHECK_KEY), 0).toLongLong();
    if (lastCheck <= 0) {
        versionCheckDebug() << "No previous update check timestamp found; check is due.";
        return true;
    }

    const qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - lastCheck;
    const bool due = elapsed >= CHECK_INTERVAL_MSECS;
    versionCheckDebug() << "Last update check elapsed msecs:" << elapsed
                            << "; interval msecs:" << CHECK_INTERVAL_MSECS
                            << "; due =" << due;
    return due;
}

void VersionCheck::startRequest(CheckTrigger trigger)
{
    m_trigger = trigger;
    m_requestPending = true;

    versionCheckDebug() << "Queuing update check; trigger ="
                       << (trigger == CheckTriggerManual ? "manual" : "automatic");
    const auto startupTask = StartupDiagnostics::task("Update schedule and request setup (async total)");
    if (!m_networkWorker) {
        // Neither object belongs to the window. They survive until cleanup ends.
        QThread* thread = new QThread;
        m_networkThread = thread;
        m_networkWorker = new VersionCheckWorker;
        m_networkWorker->moveToThread(thread);
        connect(thread, &QThread::finished, m_networkWorker, &QObject::deleteLater);
        connect(thread, &QThread::finished, thread, [thread]() { deleteStoppedThread(thread); },
                Qt::QueuedConnection);
        connect(m_networkWorker, &VersionCheckWorker::requestFinished,
                this, &VersionCheck::handleReplyFinished, Qt::QueuedConnection);
        connect(m_networkWorker, &VersionCheckWorker::requestSkipped,
                this, [this]() { m_requestPending = false; }, Qt::QueuedConnection);
        // Wake diagnostics after setup without retaining a pointer to the window.
        connect(m_networkWorker, &VersionCheckWorker::requestDispatched,
                qApp, []() {}, Qt::QueuedConnection);
        thread->start();
    }
    VersionCheckWorker* worker = m_networkWorker;
    QMetaObject::invokeMethod(worker, [worker, startupTask, trigger]() {
        worker->startRequest(trigger);
        if (startupTask)
            startupTask->finish();
        emit worker->requestDispatched();
    }, Qt::QueuedConnection);
}

void VersionCheckWorker::storeFetchedVersion(const QString& version)
{
    QSettings settings = updateCheckSettings();
    settings.setValue(QString::fromLatin1(LAST_FETCHED_VERSION_KEY), version);
    versionCheckDebug() << "Recorded latest fetched version:" << version;
}

bool VersionCheckWorker::shouldNotifyForVersion(const QString& version) const
{
    QSettings settings = updateCheckSettings();
    const QString lastNotified = settings.value(QString::fromLatin1(LAST_NOTIFIED_VERSION_KEY)).toString().trimmed();
    if (lastNotified.isEmpty())
        return true;

    int comparison = 0;
    const bool comparable = compareVersions(version, lastNotified, &comparison);
    if (!comparable) {
        versionCheckDebug() << "Could not compare against last notified version; notifying by default."
                               << "remote =" << version << ", lastNotified =" << lastNotified;
        return true;
    }

    return comparison > 0;
}

void VersionCheckWorker::markVersionNotified(const QString& version)
{
    QSettings settings = updateCheckSettings();
    settings.setValue(QString::fromLatin1(LAST_NOTIFIED_VERSION_KEY), version);
    versionCheckDebug() << "Recorded notified version:" << version;
}

#include "versioncheck.moc"
