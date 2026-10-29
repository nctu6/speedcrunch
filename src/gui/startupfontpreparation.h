// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef GUI_STARTUPFONTPREPARATION_H
#define GUI_STARTUPFONTPREPARATION_H

#include "core/startupdiagnostics.h"

#include <QFontDatabase>
#include <QGuiApplication>
#include <QThread>
#include <memory>

class StartupFontPreparation {
public:
    StartupFontPreparation()
    {
        if (QGuiApplication::platformName() != QLatin1String("cocoa"))
            return;

        // The first text widget otherwise loads the font database on the UI
        // thread. QFontDatabase is thread-safe, so overlap that work with native
        // menu and window construction after QApplication has initialized.
        const auto task = StartupDiagnostics::task("Prepare font database (async total)");
        m_thread.reset(QThread::create([task]() {
            {
                StartupDiagnostics::Scope timing("Prepare font database");
                QFontDatabase::families();
            }
            if (task)
                task->finish();
        }));
        m_thread->start();
    }

    ~StartupFontPreparation() { wait(); }

    void wait()
    {
        if (!m_thread)
            return;
        StartupDiagnostics::Scope timing("Wait for font preparation");
        m_thread->wait();
        m_thread.reset();
    }

private:
    std::unique_ptr<QThread> m_thread;
};

#endif
