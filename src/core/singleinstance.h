// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CORE_SINGLEINSTANCE_H
#define CORE_SINGLEINSTANCE_H

#include <QByteArray>
#include <QLocalServer>
#include <QLockFile>
#include <QObject>
#include <QQueue>
#include <QSet>
#include <QUuid>
#include <memory>

struct LaunchRequest {
    QByteArray activationToken;
    QByteArray startupId;
    QString requestId = QUuid::createUuid().toString(QUuid::Id128);

    static LaunchRequest fromEnvironment();
    QByteArray encode() const;
    static bool decode(const QByteArray& message, LaunchRequest* request);
};

class SingleInstance : public QObject {
    Q_OBJECT
public:
    enum StartResult { PrimaryInstance, SecondaryInstance, ServerUnavailable };
    explicit SingleInstance(QObject* parent = nullptr);
    ~SingleInstance() override;
    StartResult start(const QString& name, const LaunchRequest& request);
    void close();
    static bool notifyRunningInstance(const QString& name, const LaunchRequest& request);

signals:
    void launchRequested(const LaunchRequest& request);

private:
    QLocalServer m_server;
#ifdef Q_OS_WIN
    void* m_nativeLock = nullptr;
#endif
    std::unique_ptr<QLockFile> m_lock;
    QSet<QString> m_acceptedRequests;
    QQueue<QString> m_requestOrder;
};

#endif
