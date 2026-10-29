// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef GUI_PLATFORMACTIVATION_H
#define GUI_PLATFORMACTIVATION_H

#include "core/singleinstance.h"

class QWidget;
class LaunchController;

void activateLaunchWindow(QWidget* window, const LaunchRequest& request);
#ifdef Q_OS_MACOS
void* installMacReopenHandler(LaunchController* controller);
void removeMacReopenHandler(void* handler);
void activateMacApplication();
#endif

#endif
