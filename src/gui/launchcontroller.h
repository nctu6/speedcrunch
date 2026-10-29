// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef GUI_LAUNCHCONTROLLER_H
#define GUI_LAUNCHCONTROLLER_H

#include "core/singleinstance.h"
#include <QPointer>
#include <QQueue>
#include <QTimer>

class MainWindow;
class QWidget;

class LaunchController : public QObject {
    Q_OBJECT
public:
    explicit LaunchController(QObject* parent = nullptr);
    ~LaunchController() override;
    void setInitialWindow(MainWindow* window);
    void request(const LaunchRequest& request = {});

signals:
    void requestHandled(MainWindow* window);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void dispatchNext();
    bool m_initialized = false;
    QPointer<MainWindow> m_lastWindow;
    QPointer<QWidget> m_modalWindow;
    QQueue<LaunchRequest> m_requests;
    QTimer m_dispatchTimer;
#ifdef Q_OS_MACOS
    void* m_macHandler = nullptr;
#endif
};

#endif
