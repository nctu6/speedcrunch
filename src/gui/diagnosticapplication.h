// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef GUI_DIAGNOSTICAPPLICATION_H
#define GUI_DIAGNOSTICAPPLICATION_H

#include "core/startupdiagnostics.h"
#include "gui/mainwindow.h"
#include <QAbstractEventDispatcher>
#include <QApplication>
#include <QEvent>
#include <QTimer>
#include <type_traits>

class RecordingDiagnosticApplication : public QApplication {
public:
    RecordingDiagnosticApplication(int& argc, char** argv) : QApplication(argc, argv)
    {
        connect(QAbstractEventDispatcher::instance(), &QAbstractEventDispatcher::aboutToBlock,
                this, []() { StartupDiagnostics::finishWhenIdle(); });
    }

    bool notify(QObject* receiver, QEvent* event) override
    {
        if (!StartupDiagnostics::isRecording())
            return QApplication::notify(receiver, event);
        if (event->type() == QEvent::LayoutRequest) {
            StartupDiagnostics::Scope timing("Process startup layout requests");
            return QApplication::notify(receiver, event);
        }
        if (event->type() != QEvent::Paint)
            return QApplication::notify(receiver, event);
        StartupDiagnostics::Scope timing("Paint startup widgets");
        const bool result = QApplication::notify(receiver, event);
        QWidget* widget = qobject_cast<QWidget*>(receiver);
        if (!m_firstPaintQueued && widget && widget->isVisible()
            && qobject_cast<MainWindow*>(widget->window())) {
            m_firstPaintQueued = true;
            QTimer::singleShot(0, this, []() { StartupDiagnostics::firstPaintFinished(); });
        }
        return result;
    }

private:
    bool m_firstPaintQueued = false;
};

using DiagnosticApplication = std::conditional_t<StartupDiagnostics::isEnabled(),
    RecordingDiagnosticApplication, QApplication>;

#endif
