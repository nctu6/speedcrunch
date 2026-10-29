// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#include "launchcontroller.h"
#include "mainwindow.h"
#include "platformactivation.h"
#include "core/settings.h"

#include <QApplication>
#include <QEvent>
#include <QVariant>

LaunchController::LaunchController(QObject* parent) : QObject(parent)
{
    m_dispatchTimer.setSingleShot(true);
    connect(&m_dispatchTimer, &QTimer::timeout, this, &LaunchController::dispatchNext);
    qApp->installEventFilter(this);
#ifdef Q_OS_MACOS
    m_macHandler = installMacReopenHandler(this);
#endif
}

LaunchController::~LaunchController()
{
#ifdef Q_OS_MACOS
    removeMacReopenHandler(m_macHandler);
#endif
}

void LaunchController::setInitialWindow(MainWindow* window)
{
    m_initialized = window != nullptr;
    if (!m_lastWindow)
        m_lastWindow = window;
    if (!m_requests.isEmpty())
        m_dispatchTimer.start(0);
}

void LaunchController::request(const LaunchRequest& request)
{
    if (qApp->property("speedcrunchShutdownInProgress").toBool())
        return;
    m_requests.enqueue(request);
    m_modalWindow = nullptr;
    if (!m_dispatchTimer.isActive())
        m_dispatchTimer.start(0);
}

bool LaunchController::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::WindowActivate) {
        for (QWidget* widget = qobject_cast<QWidget*>(watched); widget; widget = widget->parentWidget()) {
            if (MainWindow* window = qobject_cast<MainWindow*>(widget)) {
                m_lastWindow = window;
                break;
            }
        }
    }
    return false;
}

void LaunchController::dispatchNext()
{
    if (qApp->property("speedcrunchShutdownInProgress").toBool()) {
        m_requests.clear();
        return;
    }
    if (m_requests.isEmpty())
        return;
    if (!m_initialized || MainWindow::sessionRestoreInProgress()) {
        m_dispatchTimer.start(10);
        return;
    }

    // A modal dialog owns input until it closes. Keep the launch queued so a
    // new tab cannot replace the session that the dialog is working on.
    if (QWidget* modal = QApplication::activeModalWidget()) {
        if (m_modalWindow != modal) {
            activateLaunchWindow(modal, m_requests.head());
            m_requests.head() = {};
            m_modalWindow = modal;
        }
        if (Settings::instance()->subsequentLaunchAction == Settings::ActivateExistingWindow) {
            m_requests.dequeue();
            emit requestHandled(MainWindow::subsequentLaunchWindow());
            if (!m_requests.isEmpty())
                m_dispatchTimer.start(0);
            return;
        }
        m_dispatchTimer.start(50);
        return;
    }
    m_modalWindow = nullptr;

    MainWindow* target = m_lastWindow && m_lastWindow->isVisible()
        ? m_lastWindow.data() : MainWindow::subsequentLaunchWindow();
    if (!target) {
        m_requests.clear();
        return;
    }
    const LaunchRequest request = m_requests.dequeue();
    switch (Settings::instance()->subsequentLaunchAction) {
    case Settings::OpenNewWindow:
        target = target->createNewSessionWindow(false);
        break;
    case Settings::OpenNewTab:
        target->createNewSessionTab();
        break;
    case Settings::ActivateExistingWindow:
    default:
        break;
    }
    if (target) {
        activateLaunchWindow(target, request);
        m_lastWindow = target;
        emit requestHandled(target);
    }
    if (!m_requests.isEmpty())
        m_dispatchTimer.start(0);
}
