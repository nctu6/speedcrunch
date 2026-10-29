// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef GUI_DIAGNOSTICSDIALOG_H
#define GUI_DIAGNOSTICSDIALOG_H

#include <QDialog>

QString appDiagnosticsReport(const QString& workload);

class DiagnosticsDialog : public QDialog {
    Q_OBJECT
public:
    explicit DiagnosticsDialog(const QString& report, QWidget* parent = nullptr);
};

#endif
