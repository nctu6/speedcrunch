// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#include "gui/diagnosticsdialog.h"
#include "core/settings.h"
#include "core/startupdiagnostics.h"

#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QFile>
#include <QFontDatabase>
#include <QLocale>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QProxyStyle>
#include <QScreen>
#include <QStyle>
#include <QStyleHints>
#include <QSysInfo>
#include <QTextStream>
#include <QVBoxLayout>

#ifdef Q_OS_UNIX
#include <sys/resource.h>
#endif
#ifdef Q_OS_MACOS
#include <mach/mach.h>
#endif
#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#endif

namespace {
QString milliseconds(qint64 ns) { return QString::number(ns / 1000000.0, 'f', 3); }
QString mib(quint64 bytes) { return QString::number(bytes / (1024.0 * 1024.0), 'f', 1); }

QString processStatistics()
{
    QString result;
    QTextStream out(&result);
#ifdef Q_OS_UNIX
    struct rusage usage = {};
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        const auto seconds = [](const timeval& time) {
            return time.tv_sec + time.tv_usec / 1000000.0;
        };
        out << "Process CPU time since launch (user / system): "
            << QString::number(seconds(usage.ru_utime), 'f', 3) << " / "
            << QString::number(seconds(usage.ru_stime), 'f', 3) << " s\n";
#if defined(Q_OS_MACOS)
        out << "Peak resident memory: " << mib(usage.ru_maxrss) << " MiB\n";
#elif defined(Q_OS_LINUX)
        out << "Peak resident memory: " << mib(quint64(usage.ru_maxrss) * 1024) << " MiB\n";
#endif
    }
#endif
#ifdef Q_OS_MACOS
    mach_task_basic_info_data_t info = {};
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  reinterpret_cast<task_info_t>(&info), &count) == KERN_SUCCESS)
        out << "Current resident memory: " << mib(info.resident_size) << " MiB\n";
#elif defined(Q_OS_LINUX)
    QFile status(QStringLiteral("/proc/self/status"));
    if (status.open(QIODevice::ReadOnly)) {
        const QList<QByteArray> lines = status.readAll().split('\n');
        for (const QByteArray& line : lines) {
            if (line.startsWith("VmRSS:")) {
                bool ok = false;
                const quint64 kib = line.mid(6).simplified().split(' ').value(0).toULongLong(&ok);
                if (ok)
                    out << "Current resident memory: " << mib(kib * 1024) << " MiB\n";
            }
        }
    }
#elif defined(Q_OS_WIN)
    PROCESS_MEMORY_COUNTERS info = {};
    if (K32GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info))) {
        out << "Current resident memory: " << mib(info.WorkingSetSize) << " MiB\n"
            << "Peak resident memory: " << mib(info.PeakWorkingSetSize) << " MiB\n";
    }
    FILETIME created, exited, kernel, user;
    if (GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user)) {
        const auto seconds = [](const FILETIME& time) {
            ULARGE_INTEGER value;
            value.LowPart = time.dwLowDateTime;
            value.HighPart = time.dwHighDateTime;
            return value.QuadPart / 10000000.0;
        };
        out << "Process CPU time since launch (user / system): "
            << QString::number(seconds(user), 'f', 3) << " / "
            << QString::number(seconds(kernel), 'f', 3) << " s\n";
    }
#endif
    if (result.isEmpty())
        out << "Process memory and CPU time: unavailable on this platform\n";
    return result;
}
}

QString appDiagnosticsReport(const QString& workload)
{
    const Settings* settings = Settings::instance();
    QString report;
    QTextStream out(&report);
    out << "SpeedCrunch " SPEEDCRUNCH_VERSION;
#ifdef SPEEDCRUNCH_PORTABLE
    out << " Portable\nPortable: yes\n";
#else
    out << "\nPortable: no\n";
#endif
    out << "OS: " << QSysInfo::prettyProductName() << '\n'
        << "OS version: " << QSysInfo::productVersion() << '\n'
        << "Kernel: " << QSysInfo::kernelType() << ' ' << QSysInfo::kernelVersion() << '\n'
        << "Architecture (build / CPU): " << QSysInfo::buildCpuArchitecture()
        << " / " << QSysInfo::currentCpuArchitecture() << '\n'
        << "Build ABI: " << QSysInfo::buildAbi() << '\n'
        << "Qt (runtime / build): " << qVersion() << " / " QT_VERSION_STR << '\n';
#ifdef QT_DEBUG
    out << "Build: Debug\n";
#else
    out << "Build: Release\n";
#endif
    out << "App language: ";
    if (settings->language == QLatin1String("C"))
        out << "System (" << QLocale().name() << ")\n";
    else
        out << settings->language << '\n';
    QString translation = qApp->property("speedcrunchTranslationLanguage").toString();
    if (translation.isEmpty())
        translation = QStringLiteral("unavailable");
    const QStyle* platformStyle = qApp->style();
    while (const auto* proxy = qobject_cast<const QProxyStyle*>(platformStyle))
        platformStyle = proxy->baseStyle();
    out << "System language: " << QLocale::system().name() << '\n'
        << "System UI languages: " << QLocale::system().uiLanguages().join(", ") << '\n'
        << "Loaded translation: "
        << translation << '\n'
        << "Qt platform: " << QGuiApplication::platformName() << '\n'
        << "Widget style: " << (platformStyle->objectName().isEmpty()
                                   ? QString::fromLatin1(platformStyle->metaObject()->className())
                                   : platformStyle->objectName()) << '\n'
        << "CPU threads available: " << QThread::idealThreadCount() << '\n';
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    const Qt::ColorScheme scheme = QGuiApplication::styleHints()->colorScheme();
    out << "Effective Qt color scheme: " << (scheme == Qt::ColorScheme::Light ? "Light"
        : scheme == Qt::ColorScheme::Dark ? "Dark" : "Unknown") << '\n';
#endif
    const QPalette appPalette = QApplication::palette();
    const QPalette menuPalette = QApplication::palette("QMenu");
    out << "Application colors (background / text): " << appPalette.color(QPalette::Window).name()
        << " / " << appPalette.color(QPalette::WindowText).name() << '\n'
        << "Menu colors (background / text): " << menuPalette.color(QPalette::Window).name()
        << " / " << menuPalette.color(QPalette::WindowText).name() << '\n';
    int screenIndex = 0;
    for (const QScreen* screen : QGuiApplication::screens()) {
        const QSize size = screen->size();
        out << "Screen " << ++screenIndex << ": " << size.width() << 'x' << size.height()
            << ", scale " << screen->devicePixelRatio()
            << ", logical DPI " << screen->logicalDotsPerInch()
            << ", refresh " << screen->refreshRate() << " Hz\n";
    }
    out << '\n' << processStatistics() << '\n' << workload;
    int definitionLines = 0;
    for (const QString& line : settings->startupUserDefinitions.split('\n')) {
        if (!line.trimmed().isEmpty())
            ++definitionLines;
    }
    out << "Theme: " << (settings->colorScheme == QLatin1String("Custom")
                            ? QStringLiteral("Custom") : settings->colorScheme) << '\n'
        << "System color mode: " << (settings->systemColorMode == Settings::SystemColorModeLight
                                         ? "Light" : settings->systemColorMode == Settings::SystemColorModeDark
                                             ? "Dark" : "Follow System") << '\n'
        << "Editor appearance: " << (settings->editorDisplayStyle == Settings::EditorDisplayClassic
                                        ? "Classic" : "Modern") << '\n'
        << "Dock appearance: " << (settings->dockAppearance == Settings::DockAppearanceSystem
                                      ? "System" : "SpeedCrunch Theme") << '\n'
        << "Menu appearance: " << (settings->menuAppearance == Settings::MenuAppearanceSystem
                                      ? "System" : "SpeedCrunch Theme") << '\n'
        << "Status bar appearance: " << (settings->statusBarAppearance == Settings::StatusBarAppearanceSystem
                                            ? "System" : "SpeedCrunch Theme") << '\n'
        << "Automatic calculation: " << (settings->autoCalc ? "on" : "off") << '\n'
        << "Syntax highlighting: " << (settings->syntaxHighlighting ? "on" : "off") << '\n'
        << "Automatic completion: " << (settings->autoCompletion ? "on" : "off") << '\n'
        << "Complex numbers: " << (settings->complexNumbers ? "on" : "off") << '\n'
        << "Result precision setting: " << settings->resultPrecision << '\n'
        << "Extra result lines: " << (settings->multipleResultLinesEnabled ? "on" : "off") << '\n'
        << "Global definition lines: " << definitionLines << '\n'
        << "Saved window layout size: " << settings->sessionLayoutJson.toUtf8().size() << " bytes\n"
        << "Keypad mode / zoom: " << int(settings->keypadMode)
        << " / " << settings->keypadZoomPercent << "%\n";
#ifdef SPEEDCRUNCH_ENABLE_UPDATE_CHECKS
    out << "Update checks: enabled\n";
#else
    out << "Update checks: disabled\n";
#endif
    const auto startup = StartupDiagnostics::snapshot();
    out << "\nStartup timings\n";
    if (!StartupDiagnostics::isEnabled()) {
        out << "Startup recording disabled in this build.\n";
        return report;
    }
    if (!startup.started) {
        out << "Unavailable (startup recording did not run).\n";
        return report;
    }
    out << "Status: " << (startup.complete ? "complete" : "in progress") << '\n'
        << "Elapsed from main(): " << milliseconds(startup.elapsedNs) << " ms\n";
    if (startup.firstPaintNs >= 0)
        out << "First window paint and event processing: "
            << milliseconds(startup.firstPaintNs) << " ms from main()\n";
    out << "Durations are wall-clock times. Nested phases and worker tasks overlap.\n"
        << "Totals include repeated calls. Do not add them to estimate startup time.\n"
        << "Async totals include time waiting for the UI thread.\n"
        << "Start ms | Total ms | Max ms | Calls | Thread | Phase\n";
    for (const auto& phase : startup.phases) {
        out << milliseconds(phase.firstStartNs) << " | " << milliseconds(phase.totalNs)
            << " | " << milliseconds(phase.longestNs) << " | " << phase.calls
            << " | " << (phase.worker ? "worker" : "main") << " | " << phase.name;
        if (phase.running)
            out << " (" << phase.running << " running)";
        out << '\n';
    }
    if (!startup.appearanceCalls.isEmpty()) {
        out << "\nStartup appearance calls\n"
            << "Widget IDs are temporary numeric IDs, not names or addresses.\n"
            << "Style installation uses the application object as its target.\n"
            << "Widget 0 means no target. Widget -1 means the identity limit is reached.\n"
            << "Input comparisons refer to the preceding call of the same kind and target.\n"
            << "Document comparisons use revision and size, without reading calculation text.\n"
            << "Document revisions can also change when formatting changes.\n"
            << "Settings comparisons cover highlighting, grouping, radix, editor style and font.\n"
            << "Evaluator comparisons check identity, not changes to its symbols.\n"
            << "Unchanged inputs alone do not establish that a call is safe to skip.\n"
            << "Call | Parent | Start ms | Duration ms | Widget | Context | Operation | Inputs\n";
        int callNumber = 0;
        for (const auto& call : startup.appearanceCalls) {
            out << ++callNumber << " | " << call.parentCall << " | "
                << milliseconds(call.startNs) << " | " << milliseconds(call.durationNs)
                << " | " << call.widgetId << " | " << call.context << " | " << call.name;
            if (!call.finished)
                out << " (running)";
            out << " | ";
            if (call.inputs >= 0) {
                out << "inputs=" << (call.inputs == 0 ? "first"
                    : call.inputs == 1 ? "changed" : "unchanged");
            }
            if (!call.inputChanges.isEmpty()) {
                if (call.inputs >= 0)
                    out << ' ';
                out << call.inputChanges;
            }
            out << '\n';
        }
        out << "\nStartup appearance assignments\n"
            << "Counts cover instrumented setters inside recorded appearance calls.\n"
            << "Assignments belong to the innermost call.\n"
            << "Changed and unchanged compare requested values before invoking the setter.\n"
            << "Skipped counts guards that avoid identical palette or stylesheet assignments.\n"
            << "Native palettes can be skipped when no style polish is needed.\n"
            << "Call | Widget | Assignment | Changed | Unchanged | Skipped\n";
        for (const auto& assignment : startup.appearanceAssignments) {
            out << assignment.call << " | " << assignment.widgetId << " | "
                << (assignment.palette ? "palette" : "stylesheet") << " | "
                << assignment.changed << " | " << assignment.unchanged << " | "
                << assignment.skipped << '\n';
        }
        out << "Appearance recording limits: 512 calls, 2048 identities, 2048 assignment rows.\n"
            << "Omitted appearance records: " << startup.omittedAppearanceRecords << '\n';
    }
    out << "\nTimings start at main() and exclude process loading before main().\n"
        << "Measurements stay in memory and are discarded when the app exits.\n";
    return report;
}

DiagnosticsDialog::DiagnosticsDialog(const QString& report, QWidget* parent)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("AppDiagnosticsDialog"));
    setWindowTitle(tr("App Diagnostics"));
    resize(760, 560);
    auto* text = new QPlainTextEdit(this);
    text->setObjectName(QStringLiteral("DiagnosticsText"));
    text->setReadOnly(true);
    text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    text->setLineWrapMode(QPlainTextEdit::NoWrap);
    text->setPlainText(report);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    auto* copy = buttons->addButton(tr("Copy to Clipboard"), QDialogButtonBox::ActionRole);
    copy->setObjectName(QStringLiteral("CopyDiagnostics"));
    connect(copy, &QPushButton::clicked, this, [text]() {
        QApplication::clipboard()->setText(text->toPlainText());
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(text);
    layout->addWidget(buttons);
}
