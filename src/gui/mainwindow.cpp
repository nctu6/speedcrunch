// SPDX-FileCopyrightText: 2007-2020, 2022, 2024, 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later


#include "gui/mainwindow.h"
#include "gui/diagnosticsdialog.h"
#include "core/startupdiagnostics.h"
#include "gui/startupappearance.h"
#include "gui/menustyleutils.h"

#include "core/complexform.h"
#include "core/constants.h"
#include "core/evaluator.h"
#include "core/functions.h"
#include "core/numberformatter.h"
#include "core/regexpatterns.h"
#include "core/settings.h"
#include "core/session.h"
#include "core/sessionjsonkeys.h"
#include "core/userdefinitions.h"
#include "core/unicodechars.h"
#include "core/variable.h"
#include "core/sessionhistory.h"
#include "core/userfunction.h"
#include "gui/aboutbox.h"
#include "gui/bitfieldwidget.h"
#include "gui/bookdock.h"
#include "gui/genericdock.h"
#include "gui/constantswidget.h"
#include "gui/customkeypaddialog.h"
#include "gui/dockcomboboxchevron.h"
#include "gui/editorutils.h"
#include "gui/functionswidget.h"
#include "gui/historywidget.h"
#include "gui/userfunctionlistwidget.h"
#include "gui/userunitlistwidget.h"
#include "gui/variablelistwidget.h"
#ifdef SPEEDCRUNCH_ENABLE_UPDATE_CHECKS
#include "gui/versioncheck.h"
#endif
#include "gui/editor.h"
#include "gui/historywidget.h"
#include "gui/manualwindow.h"
#include "gui/numberformatdialog.h"
#include "gui/notationandprecisiondialog.h"
#include "gui/oklchutils.h"
#include "gui/splittertreeutils.h"
#include "core/manualserver.h"
#include "gui/resultdisplay.h"
#include "gui/resultlineformatutils.h"
#include "gui/syntaxhighlighter.h"
#include "gui/themedlineedit.h"
#include "gui/lineeditstyleutils.h"
#include "gui/tooltipstyleutils.h"
#include "gui/uiconfig.h"
#include "math/cmath.h"
#include "math/floatnum/floatconfig.h"
#include "core/mathdsl.h"
#include "core/units.h"

#include <QLatin1String>
#include <QLocale>
#include <QTextStream>
#include <QTimer>
#include <QTranslator>
#include <QUrl>
#include <QAction>
#include <QActionGroup>
#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QApplication>
#include <QStyleHints>
#include <QClipboard>
#include <QCloseEvent>
#include <QCheckBox>
#include <QContextMenuEvent>
#include <QCoreApplication>
#include <QCursor>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QComboBox>
#include <QColorDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFont>
#include <QFontDialog>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHelpEvent>
#include <QHeaderView>
#include <QImage>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMetaObject>
#include <QMessageBox>
#include <QMouseEvent>
#include <QMimeData>
#include <QMutex>
#include <QMutexLocker>
#include <QOperatingSystemVersion>
#include <QPainter>
#include <QPainterPath>
#include <QPlainTextEdit>
#include <QPixmap>
#include <QPointer>
#include <QProxyStyle>
#include <QDropEvent>
#include <QPushButton>
#include <QScreen>
#include <QGuiApplication>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSplitter>
#include <QSplitterHandle>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QScrollBar>
#include <QSaveFile>
#include <QSet>
#include <QStatusBar>
#include <QStyle>
#include <QStyleOptionComboBox>
#include <QStyleOption>
#include <QTabBar>
#include <QToolButton>
#include <QToolTip>
#include <QTreeWidget>
#include <QThread>
#include <QVBoxLayout>
#include <QWidgetAction>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QUuid>

#include <algorithm>
#include <functional>
#include <limits>
#include <memory>
#include <vector>
#ifdef Q_OS_WIN32
#include "windows.h"
#include <shlobj.h>
#endif // Q_OS_WIN32

namespace {
constexpr const char* kFeedbackUrl = "https://www.speedcrunch.org/issues.html";
constexpr const char* kCommunityUrl = "https://speedcrunch.org/community.html";
constexpr const char* kSourceUrl = "https://www.speedcrunch.org/source.html";
constexpr const char* kDonateUrl = "https://www.speedcrunch.org/donate.html";

QString sessionsPath()
{
    return QDir(Settings::getDataPath()).filePath(QStringLiteral("sessions"));
}

bool ensureSessionsPath()
{
    QDir dir;
    return dir.mkpath(sessionsPath());
}

bool directoryIsEmpty(const QString& path)
{
    const QDir dir(path);
    return !dir.exists()
        || dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty();
}

QString normalizedSessionName(QString name)
{
    name = name.trimmed();
    return name.isEmpty()
        ? QLatin1String(SessionJsonKeys::SessionValueMain)
        : name;
}

bool hasCurrentSessionSchema(const QByteArray& data)
{
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject())
        return false;

    const QJsonObject object = doc.object();
    if (object.contains(QLatin1String("scheme")))
        return false;

    const QJsonValue schema = object.value(QLatin1String(SessionJsonKeys::Schema));
    const QJsonValue id = object.value(QLatin1String(SessionJsonKeys::Id));
    return schema.isString()
        && schema.toString() == QLatin1String(SessionJsonKeys::SchemaDialect)
        && id.isString()
        && id.toString() == QLatin1String(SessionJsonKeys::SchemaId);
}

bool readValidSessionJson(const QString& filePath, QJsonObject* json)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
        return false;

    const QJsonObject object = doc.object();
    if (object.contains(QLatin1String("scheme")))
        return false;

    const QJsonValue schema = object.value(QLatin1String(SessionJsonKeys::Schema));
    if (!schema.isString() || schema.toString() != QLatin1String(SessionJsonKeys::SchemaDialect))
        return false;

    const QJsonValue id = object.value(QLatin1String(SessionJsonKeys::Id));
    if (!id.isString() || id.toString() != QLatin1String(SessionJsonKeys::SchemaId))
        return false;

    const QString name = object.value(QLatin1String(SessionJsonKeys::Session)).toString().trimmed();
    if (name.isEmpty())
        return false;

    if (json != nullptr)
        *json = object;
    return true;
}

QString sessionFilePath(const QString& sessionName);
QString firstAvailableUntitledSessionName(const QHash<QString, Session*>& sessions);

void migrateLegacyHistoryIfNeeded()
{
    const QDir dataDir(Settings::getDataPath());
    const QString legacyPath = dataDir.filePath(QStringLiteral("history.json"));
    QFile legacyFile(legacyPath);
    if (!legacyFile.exists())
        return;

    const QString sessionDirPath = sessionsPath();
    const QFileInfo sessionDirInfo(sessionDirPath);
    const bool shouldMigrate = !sessionDirInfo.exists()
        || (sessionDirInfo.isDir() && directoryIsEmpty(sessionDirPath));
    if (!shouldMigrate)
        return;

    if (!legacyFile.open(QIODevice::ReadOnly))
        return;

    const QByteArray data = legacyFile.readAll();
    legacyFile.close();
    if (!hasCurrentSessionSchema(data))
        return;

    if (!ensureSessionsPath())
        return;

    QJsonObject sessionJson = QJsonDocument::fromJson(data).object();
    const QHash<QString, Session*> loadedSessions;
    const QString sessionName = firstAvailableUntitledSessionName(loadedSessions);
    sessionJson[QLatin1String(SessionJsonKeys::Session)] = sessionName;
    const QByteArray migratedData = QJsonDocument(sessionJson).toJson(QJsonDocument::Compact);

    QFile migratedSession(sessionFilePath(sessionName));
    if (!migratedSession.open(QIODevice::WriteOnly))
        return;

    if (migratedSession.write(migratedData) != migratedData.size())
        return;

    migratedSession.close();
    legacyFile.remove();
}

QString sessionFileBaseName(QString sessionName)
{
    sessionName = normalizedSessionName(sessionName);

    QString safeName;
    safeName.reserve(sessionName.size());
    for (const QChar ch : sessionName) {
        if (ch.isLetterOrNumber() || ch == QLatin1Char('-') || ch == QLatin1Char('_') || ch == QLatin1Char(' '))
            safeName.append(ch);
        else
            safeName.append(QLatin1Char('_'));
    }

    const QString trimmedSafeName = safeName.trimmed();
    return trimmedSafeName.isEmpty()
        ? QLatin1String(SessionJsonKeys::SessionValueMain)
        : trimmedSafeName;
}

QString sessionFilePath(const QString& sessionName)
{
    ensureSessionsPath();
    return QDir(sessionsPath()).filePath(sessionFileBaseName(sessionName) + QLatin1String(".json"));
}

QMutex& asyncSessionIoMutex()
{
    static QMutex mutex;
    return mutex;
}

QHash<QString, quint64>& asyncSessionSaveGenerations()
{
    static QHash<QString, quint64> generations;
    return generations;
}

QList<QThread*>& asyncSessionIoThreads()
{
    static QList<QThread*> threads;
    return threads;
}

struct SessionLoadSpec {
    QString name;
    QString filePath;
    QJsonObject tab;
};

bool g_restoringExtraWindows = false;
bool g_multiWindowSpawnDone = false;
constexpr int DockLayoutStateVersion = 1;

QStringList allDockNames()
{
    return {QStringLiteral("BookDock"), QStringLiteral("ConstantsDock"),
        QStringLiteral("FunctionsDock"), QStringLiteral("HistoryDock"),
        QStringLiteral("VariablesDock"), QStringLiteral("UserFunctionsDock"),
        QStringLiteral("UserUnitsDock"), QStringLiteral("BitfieldDock")};
}

QStringList legacyOpenDockNames(const QByteArray& state, bool* valid)
{
    StartupDiagnostics::Scope timing("Read legacy dock layout visibility");
    // Let Qt read its own layout format using empty docks, without building
    // formula pages, lists, search controls, or other application dock contents.
    QMainWindow probe;
    probe.setAttribute(Qt::WA_DontShowOnScreen);
    QList<QDockWidget*> docks;
    for (const QString& name : allDockNames()) {
        auto* dock = new QDockWidget(&probe);
        dock->setObjectName(name);
        dock->setAttribute(Qt::WA_DontShowOnScreen);
        probe.addDockWidget(Qt::RightDockWidgetArea, dock);
        dock->hide();
        docks.append(dock);
    }
    *valid = probe.restoreState(state, DockLayoutStateVersion);
    QStringList names;
    if (*valid) {
        for (QDockWidget* dock : docks) {
            if (!dock->isHidden())
                names.append(dock->objectName());
        }
    }
    return names;
}

bool asyncSessionSaveIsCurrent(const QString& filePath, quint64 generation)
{
    QMutexLocker locker(&asyncSessionIoMutex());
    return asyncSessionSaveGenerations().value(filePath) == generation;
}

void unregisterAsyncSessionIoThread(QThread* thread)
{
    QMutexLocker locker(&asyncSessionIoMutex());
    asyncSessionIoThreads().removeAll(thread);
}

bool writeSessionJsonToFile(const QJsonObject& json, const QString& filePath, quint64 generation = 0)
{
    const QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact);

    if (generation != 0 && !asyncSessionSaveIsCurrent(filePath, generation))
        return false;

    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    if (file.write(data) != data.size())
        return false;
    return file.commit();
}

void saveSessionAsync(const Session& session, const QString& filePath)
{
    if (session.isPrivate())
        return;
    ensureSessionsPath();

    QJsonObject json;
    session.serialize(json);

    quint64 generation = 0;
    {
        QMutexLocker locker(&asyncSessionIoMutex());
        generation = asyncSessionSaveGenerations().value(filePath) + 1;
        asyncSessionSaveGenerations().insert(filePath, generation);
    }

    QThread* thread = QThread::create([json, filePath, generation]() {
        if (!asyncSessionSaveIsCurrent(filePath, generation))
            return;
        writeSessionJsonToFile(json, filePath, generation);
    });

    {
        QMutexLocker locker(&asyncSessionIoMutex());
        asyncSessionIoThreads().append(thread);
    }
    QObject::connect(thread, &QThread::finished, thread, [thread]() {
        unregisterAsyncSessionIoThread(thread);
        thread->deleteLater();
    });
    thread->start();
}

void waitForAsyncSessionIo()
{
    while (true) {
        QList<QThread*> threads;
        {
            QMutexLocker locker(&asyncSessionIoMutex());
            threads = asyncSessionIoThreads();
        }
        if (threads.isEmpty())
            return;

        for (QThread* thread : threads) {
            if (thread != nullptr) {
                thread->wait();
                unregisterAsyncSessionIoThread(thread);
            }
        }
    }
}

bool loadedSessionNameExists(const QHash<QString, Session*>& sessions, const QString& name)
{
    for (const QString& existingName : sessions.keys()) {
        if (existingName.compare(name, Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}

QString firstAvailableUntitledSessionName(const QHash<QString, Session*>& sessions)
{
    for (int number = 1; number < (std::numeric_limits<int>::max)(); ++number) {
        const QString name = QStringLiteral("Untitled-%1").arg(number);
        if (!loadedSessionNameExists(sessions, name) && !QFileInfo::exists(sessionFilePath(name)))
            return name;
    }

    return QStringLiteral("Untitled");
}

QIcon privateSessionIcon(const QColor& color)
{
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(color, 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    QPainterPath shield;
    shield.moveTo(16, 3);
    shield.lineTo(27, 7);
    shield.lineTo(26, 17);
    shield.cubicTo(25, 23, 20, 27, 16, 29);
    shield.cubicTo(12, 27, 7, 23, 6, 17);
    shield.lineTo(5, 7);
    shield.closeSubpath();
    painter.drawPath(shield);
    return QIcon(pixmap);
}

QJsonObject persistentSessionLayout(QJsonObject node, const QSet<QString>& privateNames)
{
    if (node.contains(QStringLiteral("tabs"))) {
        QJsonArray tabs;
        for (const QJsonValue& value : node.value(QStringLiteral("tabs")).toArray()) {
            const QString name = value.toObject().value(QStringLiteral("name")).toString();
            if (!privateNames.contains(name))
                tabs.append(value);
        }
        node.insert(QStringLiteral("tabs"), tabs);
        if (privateNames.contains(node.value(QStringLiteral("active")).toString()))
            node.insert(QStringLiteral("active"), tabs.isEmpty()
                ? QString() : tabs.first().toObject().value(QStringLiteral("name")).toString());
        if (tabs.isEmpty())
            return QJsonObject();
    }
    if (node.contains(QStringLiteral("children"))) {
        QJsonArray children;
        QJsonArray sizes;
        const QJsonArray oldChildren = node.value(QStringLiteral("children")).toArray();
        const QJsonArray oldSizes = node.value(QStringLiteral("sizes")).toArray();
        for (int i = 0; i < oldChildren.size(); ++i) {
            const QJsonObject child = persistentSessionLayout(oldChildren.at(i).toObject(), privateNames);
            if (!child.isEmpty()) {
                children.append(child);
                sizes.append(i < oldSizes.size() ? oldSizes.at(i) : QJsonValue(1));
            }
        }
        if (children.isEmpty())
            return QJsonObject();
        node.insert(QStringLiteral("children"), children);
        node.insert(QStringLiteral("sizes"), sizes);
    }
    return node;
}

QString firstAvailableImportedSessionName(const QString& preferredName, const QHash<QString, Session*>& sessions)
{
    const QString baseName = normalizedSessionName(preferredName);
    const auto isAvailable = [&sessions](const QString& name) {
        return !loadedSessionNameExists(sessions, name) && !QFileInfo::exists(sessionFilePath(name));
    };

    if (isAvailable(baseName))
        return baseName;

    for (int number = 2; number < (std::numeric_limits<int>::max)(); ++number) {
        const QString candidate = QStringLiteral("%1 (%2)").arg(baseName).arg(number);
        if (isAvailable(candidate))
            return candidate;
    }

    return firstAvailableUntitledSessionName(sessions);
}

int untitledSessionNumber(const QString& name)
{
    static const QString prefix = QStringLiteral("Untitled-");
    if (!name.startsWith(prefix, Qt::CaseInsensitive))
        return -1;

    bool ok = false;
    const int number = name.mid(prefix.size()).toInt(&ok);
    return ok && number > 0 ? number : -1;
}

bool isReusableUntitledSession(const Session* session)
{
    if (session == nullptr || session->isPrivate() || !session->historyIsEmpty())
        return false;

    const Evaluator* evaluator = session->evaluator();
    const QList<Variable> variables = session->variablesToList();
    for (const Variable& variable : variables) {
        if (variable.type() != Variable::BuiltIn
            && !(evaluator && evaluator->isGlobalUserVariable(variable.identifier())))
            return false;
    }

    const QList<UserFunction> functions = session->UserFunctionsToList();
    for (const UserFunction& function : functions) {
        if (!(evaluator && evaluator->isGlobalUserFunction(function.name())))
            return false;
    }

    const QList<UserUnit> units = session->userUnitsToList();
    for (const UserUnit& unit : units) {
        if (!(evaluator && evaluator->isGlobalUserUnit(unit.name())))
            return false;
    }

    return true;
}

bool sessionHasPersistableContent(const Session* session)
{
    if (session == nullptr)
        return false;
    if (!session->historyIsEmpty())
        return true;

    const Evaluator* evaluator = session->evaluator();
    const QList<Variable> variables = session->variablesToList();
    for (const Variable& variable : variables) {
        if (variable.type() != Variable::BuiltIn
            && !(evaluator && evaluator->isGlobalUserVariable(variable.identifier()))) {
            return true;
        }
    }

    const QList<UserFunction> functions = session->UserFunctionsToList();
    for (const UserFunction& function : functions) {
        if (!(evaluator && evaluator->isGlobalUserFunction(function.name())))
            return true;
    }

    const QList<UserUnit> units = session->userUnitsToList();
    for (const UserUnit& unit : units) {
        if (!(evaluator && evaluator->isGlobalUserUnit(unit.name())))
            return true;
    }

    return false;
}

bool shouldDeleteSessionFileOnClose(const QString& sessionName, const Session* session)
{
    return untitledSessionNumber(sessionName) > 0 && isReusableUntitledSession(session);
}

QJsonObject editorState(Editor* editor)
{
    QJsonObject state;
    if (editor == nullptr)
        return state;

    state.insert(QStringLiteral("text"), editor->text());
    state.insert(QStringLiteral("cursor"), editor->cursorPosition());
    return state;
}

void restoreEditorState(Editor* editor, const QJsonObject& state, const QString& fallbackText)
{
    if (editor == nullptr)
        return;

    const QString text = state.value(QStringLiteral("text")).toString(fallbackText);
    const int cursor = state.contains(QStringLiteral("cursor"))
        ? state.value(QStringLiteral("cursor")).toInt(text.size())
        : text.size();
    {
        const QSignalBlocker blocker(editor);
        editor->setText(text);
        editor->setCursorPosition(qBound(0, cursor, text.size()));
    }
    if (!text.trimmed().isEmpty())
        editor->refreshAutoCalc();
}

QJsonObject sessionLayoutEntry(const QString& name,
                               const QPair<int, int>& viewportAnchor,
                               int scrollValue,
                               const QJsonObject& editor,
                               bool includeEditor)
{
    QJsonObject entry;
    entry.insert(QStringLiteral("name"), name);
    entry.insert(QStringLiteral("file"), QString(sessionFileBaseName(name) + QLatin1String(".json")));
    if (viewportAnchor.first >= 0 || scrollValue >= 0) {
        QJsonObject scroll;
        if (viewportAnchor.first >= 0) {
            scroll.insert(QStringLiteral("block"), viewportAnchor.first);
            scroll.insert(QStringLiteral("offset"), viewportAnchor.second);
        }
        if (scrollValue >= 0)
            scroll.insert(QStringLiteral("value"), scrollValue);
        entry.insert(QStringLiteral("scroll"), scroll);
    }
    if (includeEditor)
        entry.insert(QStringLiteral("editor"), editor);
    return entry;
}

EvaluationContext currentEvaluationContext(const Settings* settings)
{
    EvaluationContext ctx;
    ctx.main.fmt = settings->resultFormat;
    ctx.main.prec = settings->resultPrecision;
    ctx.main.cplx = settings->resultComplexForm;
    if (settings->multipleResultLinesEnabled) {
        if (settings->secondaryResultEnabled)
            ctx.extras.append(ResultLineContext{settings->alternativeResultFormat, settings->secondaryResultPrecision, settings->secondaryResultComplexForm});
        if (settings->tertiaryResultEnabled)
            ctx.extras.append(ResultLineContext{settings->tertiaryResultFormat, settings->tertiaryResultPrecision, settings->tertiaryResultComplexForm});
        if (settings->quaternaryResultEnabled)
            ctx.extras.append(ResultLineContext{settings->quaternaryResultFormat, settings->quaternaryResultPrecision, settings->quaternaryResultComplexForm});
        if (settings->quinaryResultEnabled)
            ctx.extras.append(ResultLineContext{settings->quinaryResultFormat, settings->quinaryResultPrecision, settings->quinaryResultComplexForm});
    }
    ctx.complexOn = settings->complexNumbers;
    ctx.unit = settings->imaginaryUnit;
    ctx.angle = settings->angleUnit;
    ctx.unitExp = settings->unitNegativeExponentStyle;
    ctx.round = settings->resultRoundingMode;
    return ctx;
}

void applyEvaluationContext(Settings* settings, const EvaluationContext& ctx)
{
    settings->resultFormat = ctx.main.fmt;
    settings->resultPrecision = ctx.main.prec;
    settings->resultComplexForm = ctx.main.cplx;

    settings->multipleResultLinesEnabled = !ctx.extras.isEmpty();
    settings->secondaryResultEnabled = false;
    settings->tertiaryResultEnabled = false;
    settings->quaternaryResultEnabled = false;
    settings->quinaryResultEnabled = false;

    auto applyExtra = [settings](int index, const ResultLineContext& line) {
        if (index == 0) {
            settings->secondaryResultEnabled = true;
            settings->alternativeResultFormat = line.fmt;
            settings->secondaryResultPrecision = line.prec;
            settings->secondaryResultComplexForm = line.cplx;
        } else if (index == 1) {
            settings->tertiaryResultEnabled = true;
            settings->tertiaryResultFormat = line.fmt;
            settings->tertiaryResultPrecision = line.prec;
            settings->tertiaryResultComplexForm = line.cplx;
        } else if (index == 2) {
            settings->quaternaryResultEnabled = true;
            settings->quaternaryResultFormat = line.fmt;
            settings->quaternaryResultPrecision = line.prec;
            settings->quaternaryResultComplexForm = line.cplx;
        } else if (index == 3) {
            settings->quinaryResultEnabled = true;
            settings->quinaryResultFormat = line.fmt;
            settings->quinaryResultPrecision = line.prec;
            settings->quinaryResultComplexForm = line.cplx;
        }
    };
    for (int i = 0; i < ctx.extras.size() && i < 4; ++i)
        applyExtra(i, ctx.extras.at(i));

    settings->complexNumbers = ctx.complexOn;
    settings->imaginaryUnit = (ctx.unit == 'j') ? 'j' : 'i';
    settings->angleUnit = ctx.angle;
    settings->unitNegativeExponentStyle = isValidUnitNegativeExponentStyle(ctx.unitExp)
        ? ctx.unitExp
        : Settings::UnitNegativeExponentSuperscript;
    settings->resultRoundingMode = isValidResultRoundingMode(ctx.round)
        ? ctx.round
        : Settings::ResultRoundingHalfAwayFromZero;

    DMath::complexMode = settings->complexNumbers;
    CMath::setImaginaryUnitSymbol(settings->imaginaryUnit);
    setRuntimeUnitNegativeExponentStyle(settings->unitNegativeExponentStyle);
    setRuntimeResultRoundingMode(settings->resultRoundingMode);
}

QStringList renderedLinesForHistoryEntry(const HistoryEntry& entry, Settings* settings, const Evaluator* evaluator)
{
    const EvaluationContext previousContext = currentEvaluationContext(settings);
    applyEvaluationContext(settings, entry.contextRef());

    QStringList lines;
    lines.append(ResultLineFormatUtils::formattedExpressionLineForDisplay(
        entry.expr(),
        entry.interpretedExpr(),
        evaluator));
    if (!entry.result().isNan()) {
        lines.append(ResultLineFormatUtils::formatResultLinesForDisplay(
            entry.expr(),
            entry.interpretedExpr(),
            entry.result(),
            false,
            true,
            evaluator));
    }

    applyEvaluationContext(settings, previousContext);
    return lines;
}
}

QTranslator* MainWindow::createTranslator(const QString& langCode)
{
    QTranslator* translator = new QTranslator;
    QLocale locale(langCode == "C" ? QLocale().name() : langCode);

    if(!translator->load(locale, QString(":/locale/"))) {
        // There are regional Portuguese translations only for Brazil and Portugal.
        // Unsupported Portuguese variants (e.g. pt_AO) should fall back to pt_PT.
        if (locale.language() == QLocale::Portuguese) {
            const QLocale::Territory territory = locale.territory();
            if (territory != QLocale::Brazil && territory != QLocale::Portugal) {
                if (translator->load(QLocale(QLocale::Portuguese, QLocale::Portugal), QString(":/locale/")))
                    return translator;
            }
        }

        // Strip the country and try to find a generic translation for this language
        locale = QLocale(locale.language());
        if (!translator->load(locale, QString(":/locale/"))) {
            // Handle the case where the translation file cannot be loaded
            // For example, log an error, use a default language, etc.
        }
    }

    return translator;
}

static bool isVisibleKeypadMode(Settings::KeypadMode mode)
{
    return mode == Settings::KeypadModeBasicWide
        || mode == Settings::KeypadModeScientificWide
        || mode == Settings::KeypadModeScientificNarrow
        || mode == Settings::KeypadModeCustom;
}

static bool isWaylandPlatform()
{
    const auto platform = QGuiApplication::platformName();
    const bool isWayland = (platform == "wayland");
    return isWayland;
}

QString colorSchemeRoleLabel(ColorScheme::Role role)
{
    switch (role) {
    case ColorScheme::Number: return QStringLiteral("number");
    case ColorScheme::Parens: return QStringLiteral("parens");
    case ColorScheme::List: return QStringLiteral("list");
    case ColorScheme::Unit: return QStringLiteral("unit");
    case ColorScheme::Result: return QStringLiteral("result");
    case ColorScheme::Comment: return QStringLiteral("comment");
    case ColorScheme::Function: return QStringLiteral("function");
    case ColorScheme::Operator: return QStringLiteral("operator");
    case ColorScheme::Variable: return QStringLiteral("variable");
    case ColorScheme::Separator: return QStringLiteral("separator");
    case ColorScheme::Background: return QStringLiteral("background");
    case ColorScheme::Primary: return QStringLiteral("primary");
    }
    return QString();
}

void updateColorButtonStyle(QPushButton* button, const QColor& color)
{
    if (!button || !color.isValid())
        return;

    const QColor textColor = aaForegroundForBackground(color);

    button->setText(color.name());
    button->setMinimumWidth(button->fontMetrics().horizontalAdvance(QStringLiteral("#000000")) + 22);
    button->setStyleSheet(QStringLiteral(R"(
        QPushButton {
            background-color: %1;
            color: %2;
            border: 1px solid %2;
            border-radius: 3px;
            padding: 2px 8px;
        }
    )").arg(color.name(), textColor.name()));
}

enum class ColorSchemeFilter {
    Dark,
    Light
};

bool colorSchemeMatchesFilter(const ColorScheme& scheme, ColorSchemeFilter filter)
{
    const QColor background = scheme.colorForRole(ColorScheme::Background);
    const ThemePolarity polarity = themePolarityForBackground(background);

    return filter == ColorSchemeFilter::Dark
        ? polarity == ThemePolarity::Dark
        : polarity == ThemePolarity::Light;
}

ColorScheme activeColorScheme(const Settings* settings)
{
    if (settings && settings->colorScheme == QLatin1String("Custom")) {
        const QJsonDocument doc = QJsonDocument::fromJson(settings->customColorSchemeJson.toUtf8());
        const ColorScheme custom(doc);
        if (custom.isValid())
            return custom;
    }

    const ColorScheme named = ColorScheme::loadByName(settings ? settings->colorScheme : QString());
    if (named.isValid())
        return named;
    return ColorScheme::loadByName(QStringLiteral("Terminal"));
}

QString colorSchemeDisplayName(const QString& schemeName, const ColorScheme& scheme)
{
    const QString displayName = scheme.displayName();
    return displayName.isEmpty() ? schemeName : displayName;
}

QString colorSchemeDisplayName(const QString& schemeName)
{
    return colorSchemeDisplayName(schemeName, ColorScheme::loadByName(schemeName));
}

struct ThemeSurfaceColors
{
    QColor background;
    QColor foreground;
};

struct ThemeScrollBarColors
{
    QColor track;
    QColor thumb;
    QColor hoverThumb;
    QColor pressedThumb;
};

constexpr int kThemeGeneratedShadeCount = UiConfig::Shade600 + 1;
constexpr int kDockComboPopupHorizontalPadding = 8;
constexpr int kDockComboPopupVerticalPadding = 6;

struct GeneratedThemeSurfaces
{
    QColor base;
    ThemeSurfaceColors primary;
    ThemePolarity polarity;
    QVector<QColor> backgrounds;
    QVector<QColor> foregrounds;
    // 100: outer window chrome, including tab-row empty space and dock padding.
    ThemeSurfaceColors window;
    // 200: result display and active session surface. This is the background
    // role from the selected SpeedCrunch theme.
    ThemeSurfaceColors result;
    // 300: expression editor, dock list/table content, bitfield bit cells,
    // hovered dock header buttons, and normal keypad buttons. The editor no
    // longer has an independent theme role; it is derived from the
    // result-display background by moving one OKLCH shade step away from the
    // base surface.
    ThemeSurfaceColors editorAndLists;
    // 400: dock title bars, dock/list/table headers, combo popup fill,
    // list/table hovered items, active dock tabs, selected session tabs,
    // popup/input borders, normal bitfield buttons, hovered keypad buttons,
    // and hovered bitfield bit cells.
    ThemeSurfaceColors headersAndBorders;
    // 500: input controls, search boxes, combo boxes, hovered session tabs,
    // hovered bitfield buttons, and pressed keypad buttons.
    ThemeSurfaceColors inputs;
    // 600: inactive tabs, session tab close-button hover fills, and pressed
    // bitfield buttons.
    ThemeSurfaceColors hoverAndInactiveTabs;
};

GeneratedThemeSurfaces generatedSurfaceColorsForScheme(const ColorScheme& scheme)
{
    const QColor base = scheme.isValid()
        ? scheme.colorForRole(ColorScheme::Background)
        : QApplication::palette().color(QPalette::Base);
    const QColor configuredPrimary = scheme.isValid() && scheme.hasColorForRole(ColorScheme::Primary)
        ? scheme.colorForRole(ColorScheme::Primary)
        : QColor();
    const QColor fallbackText = QApplication::palette().color(QPalette::Text);
    // Only these colors affect generated surfaces. Keep one entry per thread
    // so repeated widget refreshes share the shades without growing the cache.
    struct SurfaceCache {
        QColor base;
        QColor primary;
        QColor fallbackText;
        GeneratedThemeSurfaces surfaces;
        bool valid = false;
    };
    static thread_local SurfaceCache cache;
    const bool unchanged = cache.valid && cache.base == base
        && cache.primary == configuredPrimary && cache.fallbackText == fallbackText;
    StartupDiagnostics::AppearanceScope timing(unchanged
        ? "Reuse theme surface colors" : "Generate theme surface colors", nullptr,
        [&]() { return StartupDiagnostics::appearanceInputs({base, configuredPrimary, fallbackText}); });
    if (unchanged)
        return cache.surfaces;

    const QColor generatedPrimary = configuredPrimary.isValid()
        ? QColor() : generatePrimaryFromBackground(base);
    const QColor primary = configuredPrimary.isValid()
        ? configuredPrimary
        : (generatedPrimary.isValid()
              ? generatedPrimary
              : fallbackText);
    const ThemePolarity polarity = themePolarityForBackground(base);
    const QVector<QColor> shades = generateOklchShades(base,
                                                       kThemeGeneratedShadeCount,
                                                       polarity);
    const QVector<QColor> foregrounds = aaForegroundsForBackgrounds(shades);
    const QColor fallbackForeground = aaForegroundForBackground(base);
    const auto surface = [&](int index) {
        return ThemeSurfaceColors {
            shades.value(index, base),
            foregrounds.value(index, fallbackForeground)
        };
    };
    const GeneratedThemeSurfaces surfaces {
        base,
        ThemeSurfaceColors { primary, aaForegroundForBackground(primary) },
        polarity,
        shades,
        foregrounds,
        surface(UiConfig::WindowBackgroundShade),
        surface(UiConfig::ResultDisplayShade),
        surface(UiConfig::DockBackgroundShade),
        surface(UiConfig::DockHeaderShade),
        surface(UiConfig::DockUnfocusedSelectedItemShade),
        surface(UiConfig::Shade600)
    };
    cache = {base, configuredPrimary, fallbackText, surfaces, true};
    return surfaces;
}

GeneratedThemeSurfaces generatedSurfaceColors(const Settings* settings)
{
    return generatedSurfaceColorsForScheme(activeColorScheme(settings));
}

QPalette paletteForThemeSurface(const QPalette& inherited, const ThemeSurfaceColors& surface)
{
    QPalette palette = inherited;
    for (const QPalette::ColorGroup group : {QPalette::Active,
                                             QPalette::Inactive,
                                             QPalette::Disabled}) {
        palette.setColor(group, QPalette::Window, surface.background);
        palette.setColor(group, QPalette::WindowText, surface.foreground);
        palette.setColor(group, QPalette::Base, surface.background);
        palette.setColor(group, QPalette::Text, surface.foreground);
        palette.setColor(group, QPalette::Button, surface.background);
        palette.setColor(group, QPalette::ButtonText, surface.foreground);
    }
    return palette;
}

ThemeSurfaceColors themeSurfaceForShadeIndex(const GeneratedThemeSurfaces& surfaces, int shadeIndex)
{
    const QColor fallbackBackground = surfaces.base.isValid()
        ? surfaces.base
        : QApplication::palette().color(QPalette::Base);
    const QColor fallbackForeground = aaForegroundForBackground(fallbackBackground);
    return {
        surfaces.backgrounds.value(shadeIndex, fallbackBackground),
        surfaces.foregrounds.value(shadeIndex, fallbackForeground)
    };
}

bool setStyleSheetIfChanged(QWidget* widget, const QString& styleSheet)
{
    // Even an identical stylesheet makes Qt repolish the widget's children.
    if (widget->styleSheet() == styleSheet) {
        StartupDiagnostics::appearanceAssignment(widget, false, false, true);
        return false;
    }
    StartupDiagnostics::setAppearanceStyleSheet(widget, styleSheet);
    return true;
}

void setDockPaletteIfChanged(QWidget* widget, const QPalette& palette)
{
    const QPalette current = widget->palette();
    // Matching colors must also keep the same explicit palette roles.
    if (widget->testAttribute(Qt::WA_SetPalette) && current == palette
        && current.resolveMask() == palette.resolveMask()) {
        StartupDiagnostics::appearanceAssignment(widget, true, false, true);
        return;
    }
    StartupDiagnostics::setAppearancePalette(widget, palette);
}

void applyStatusBarAppearance(QStatusBar* bar, const GeneratedThemeSurfaces& surfaces)
{
    if (bar == nullptr)
        return;

    if (!bar->property("speedcrunchNativeStatusBarStyledBackground").isValid()) {
        bar->setProperty("speedcrunchNativeStatusBarStyledBackground",
                         bar->testAttribute(Qt::WA_StyledBackground));
    }
    if (Settings::instance()->statusBarAppearance == Settings::StatusBarAppearanceSystem) {
        setStyleSheetIfChanged(bar, QString());
        QList<QWidget*> widgets = bar->findChildren<QWidget*>();
        widgets.prepend(bar);
        for (QWidget* widget : widgets) {
            widget->style()->unpolish(widget);
            StartupDiagnostics::setAppearancePalette(widget, MenuStyle::systemPalette(widget));
            widget->style()->polish(widget);
            widget->updateGeometry();
            widget->update();
        }
        // Fill styles that leave the status bar transparent so the window's
        // SpeedCrunch theme cannot show through the system surface.
        bar->setAutoFillBackground(true);
        bar->setAttribute(Qt::WA_StyledBackground,
                          bar->property("speedcrunchNativeStatusBarStyledBackground").toBool());
        return;
    }

    const ThemeSurfaceColors surface =
        themeSurfaceForShadeIndex(surfaces, UiConfig::StatusBarBackgroundShade);
    StartupDiagnostics::setAppearancePalette(bar, paletteForThemeSurface(bar->palette(), surface));
    bar->setAutoFillBackground(true);
    bar->setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheetIfChanged(bar, QStringLiteral(
        "QStatusBar { background-color: %1; color: %2; }"
        "QStatusBar::item { border: none; }"
        "QStatusBar QLabel { background: transparent; color: %2; }"
        "QStatusBar QPushButton {"
        " background: transparent; color: %2; border: none;"
        " margin: 0px; padding: 0px 4px;"
        "}"
        "QStatusBar QPushButton:hover {"
        " background: transparent; color: %2;"
        "}"
        "QStatusBar QPushButton:pressed {"
        " background: transparent; color: %2;"
        "}")
                           .arg(surface.background.name(),
                                surface.foreground.name()));

    for (QWidget* child : bar->findChildren<QWidget*>())
        StartupDiagnostics::setAppearancePalette(child, paletteForThemeSurface(child->palette(), surface));
}

ThemeScrollBarColors scrollBarColorsForSurfaceIndex(const GeneratedThemeSurfaces& surfaces, int surfaceIndex)
{
    const auto colorAt = [&surfaces](int index) {
        return surfaces.backgrounds.value(qBound(0, index, surfaces.backgrounds.size() - 1));
    };
    return {
        colorAt(surfaceIndex),
        colorAt(surfaceIndex + 1),
        colorAt(surfaceIndex + 2),
        colorAt(surfaceIndex + 3)
    };
}

constexpr auto DockSeparatorNormalColorProperty = "speedcrunchDockSeparatorNormalColor";
constexpr auto DockSeparatorActiveColorProperty = "speedcrunchDockSeparatorActiveColor";
constexpr auto DockSeparatorBackgroundColorProperty = "speedcrunchDockSeparatorBackgroundColor";
constexpr auto DockSeparatorStrokeWidthProperty = "speedcrunchDockSeparatorStrokeWidth";
constexpr auto DockChromeStyleInstalledProperty = "speedcrunchDockChromeStyleInstalled";

QVariant dockSeparatorPropertyForWidget(const QWidget* widget, const char* propertyName)
{
    for (const QWidget* current = widget; current != nullptr; current = current->parentWidget()) {
        const QVariant value = current->property(propertyName);
        if (value.isValid())
            return value;
    }
    return QVariant();
}

QRect dockSeparatorStrokeRect(const QRect& separatorRect, int strokeWidth)
{
    QRect strokeRect = separatorRect;
    if (strokeRect.width() > strokeRect.height()) {
        const int height = qMax(1, strokeWidth);
        strokeRect.setTop(strokeRect.center().y() - (height - 1) / 2);
        strokeRect.setHeight(height);
    } else {
        const int width = qMax(1, strokeWidth);
        strokeRect.setLeft(strokeRect.center().x() - (width - 1) / 2);
        strokeRect.setWidth(width);
    }
    return strokeRect;
}

class DockChromeStyle : public QProxyStyle {
public:
    using QProxyStyle::QProxyStyle;

    QRect subElementRect(SubElement element, const QStyleOption* option,
                         const QWidget* widget = nullptr) const override
    {
        QRect rect = QProxyStyle::subElementRect(element, option, widget);
        if (element != SE_DockWidgetCloseButton && element != SE_DockWidgetFloatButton)
            return rect;

        const auto* dock = qobject_cast<const QDockWidget*>(widget);
        const auto* dockOption = qstyleoption_cast<const QStyleOptionDockWidget*>(option);
        if (dock == nullptr || dockOption == nullptr || rect.isEmpty())
            return rect;
        const auto* button = dock->findChild<QAbstractButton*>(
            element == SE_DockWidgetCloseButton ? QStringLiteral("qt_dockwidget_closebutton")
                                               : QStringLiteral("qt_dockwidget_floatbutton"),
            Qt::FindDirectChildrenOnly);
        if (button == nullptr || !button->property("speedcrunchDockHeaderButton").toBool())
            return rect;

        // Native styles center their own button extent. QWidget's fixed size
        // can then change that extent without adjusting the origin. Center the
        // actual themed button in the title area before Qt applies its geometry.
        const QRect titleRect = dockOption->rect;
        if (dockOption->verticalTitleBar) {
            rect.setLeft(titleRect.left() + (titleRect.width() - button->width()) / 2);
            rect.setWidth(button->width());
        } else {
            rect.setTop(titleRect.top() + (titleRect.height() - button->height()) / 2);
            rect.setHeight(button->height());
        }
        return rect;
    }

    void drawPrimitive(PrimitiveElement element,
                       const QStyleOption* option,
                       QPainter* painter,
                       const QWidget* widget = nullptr) const override
    {
        if (element != PE_IndicatorDockWidgetResizeHandle || option == nullptr || widget == nullptr) {
            QProxyStyle::drawPrimitive(element, option, painter, widget);
            return;
        }

        const QColor normalColor = dockSeparatorPropertyForWidget(widget, DockSeparatorNormalColorProperty).value<QColor>();
        const QColor activeColor = dockSeparatorPropertyForWidget(widget, DockSeparatorActiveColorProperty).value<QColor>();
        if (!normalColor.isValid() || !activeColor.isValid()) {
            QProxyStyle::drawPrimitive(element, option, painter, widget);
            return;
        }

        const QColor backgroundColor = dockSeparatorPropertyForWidget(widget, DockSeparatorBackgroundColorProperty).value<QColor>();
        if (backgroundColor.isValid())
            painter->fillRect(option->rect, backgroundColor);

        const QVariant strokeWidth = dockSeparatorPropertyForWidget(widget, DockSeparatorStrokeWidthProperty);
        const QRect strokeRect = dockSeparatorStrokeRect(option->rect,
            strokeWidth.isValid() ? strokeWidth.toInt() : UiConfig::DockSplitterStrokeWidth);
        const QPoint cursorPosition = widget->mapFromGlobal(QCursor::pos());
        const bool cursorOverStroke = strokeRect.contains(cursorPosition);
        const bool active = option->state.testFlag(State_MouseOver)
            || option->state.testFlag(State_Sunken)
            || cursorOverStroke;
        painter->fillRect(strokeRect, active ? activeColor : normalColor);
    }
};

void ensureDockChromeStyleInstalled()
{
    if (QApplication::style()->property(DockChromeStyleInstalledProperty).toBool())
        return;

    StartupDiagnostics::AppearanceScope timing("Install dock chrome style", qApp);
    timing.observeInputs("installed", []() {
        return StartupDiagnostics::appearanceInputs({
            QApplication::style()->property(DockChromeStyleInstalledProperty).toBool()});
    });

    QStyle* style = new DockChromeStyle(QApplication::style());
    style->setProperty(DockChromeStyleInstalledProperty, true);
    QApplication::setStyle(style);
}

QString splitterStyleSheet(const QColor& normal, const QColor& active)
{
    return QStringLiteral(
        "QSplitter::handle { background-color: %1; }"
        "QSplitter::handle:hover, QSplitter::handle:pressed { background-color: %2; }")
        .arg(normal.name(), active.name());
}

QString splitterHandleStyleSheet(const QColor& normal, const QColor& active)
{
    return QStringLiteral(
        "QSplitterHandle { background-color: %1; }"
        "QSplitterHandle:hover, QSplitterHandle:pressed { background-color: %2; }")
        .arg(normal.name(), active.name());
}

QString scrollBarStyleSheet(const ThemeScrollBarColors& colors)
{
    return QStringLiteral(
        "QScrollBar:vertical {"
        " background: %1; border: 0; margin: 0; width: 10px;"
        "}"
        "QScrollBar:horizontal {"
        " background: %1; border: 0; margin: 0; height: 10px;"
        "}"
        "QScrollBar::handle:vertical, QScrollBar::handle:horizontal {"
        " background: %2; border: 0; border-radius: 4px; min-height: 20px; min-width: 20px;"
        "}"
        "QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover {"
        " background: %3;"
        "}"
        "QScrollBar::handle:vertical:pressed, QScrollBar::handle:horizontal:pressed {"
        " background: %4;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
        " background: %1; border: 0; width: 0; height: 0;"
        "}"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical,"
        "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {"
        " background: %1;"
        "}")
        .arg(colors.track.name(),
             colors.thumb.name(),
             colors.hoverThumb.name(),
             colors.pressedThumb.name());
}

void saveNativeDockSurface(QWidget* widget);
void restoreNativeDockSurface(QWidget* widget, bool forceNativePolish = true);

void applyScrollCornerColorToScrollArea(QAbstractScrollArea* area, const QColor& color)
{
    if (area == nullptr || !color.isValid())
        return;

    QWidget* corner = area->cornerWidget();
    if (corner == nullptr) {
        corner = new QWidget(area);
        area->setCornerWidget(corner);
    }
    saveNativeDockSurface(corner);

    QPalette palette = corner->palette();
    for (const QPalette::ColorGroup group : {QPalette::Active,
                                             QPalette::Inactive,
                                             QPalette::Disabled}) {
        palette.setColor(group, QPalette::Window, color);
        palette.setColor(group, QPalette::Base, color);
        palette.setColor(group, QPalette::Button, color);
    }
    StartupDiagnostics::setAppearancePalette(corner, palette);
    corner->setAutoFillBackground(true);
    corner->setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheetIfChanged(corner, QStringLiteral("QWidget { background-color: %1; border: 0; }")
                              .arg(color.name()));
}

class MenuPrecisionSpinBox : public QSpinBox {
public:
    using QSpinBox::QSpinBox;

    void setSystemAppearance()
    {
        m_arrowColor = QColor();
        setStyleSheet(QString());
        StartupDiagnostics::setAppearancePalette(this, MenuStyle::systemPalette(this));
        if (QLineEdit* editor = lineEdit()) {
            setStyleSheetIfChanged(editor, QString());
            StartupDiagnostics::setAppearancePalette(editor, MenuStyle::systemPalette(editor));
        }
        update();
    }

    void setThemeSurface(const ThemeSurfaceColors& surface)
    {
        m_arrowColor = surface.foreground;
        StartupDiagnostics::setAppearancePalette(this, paletteForThemeSurface(palette(), surface));
        setStyleSheet(QStringLiteral(
            "QSpinBox {"
            " background-color: %1; color: %2; border: none; border-radius: 7px;"
            " padding: 1px 18px 1px 6px;"
            " selection-background-color: %2; selection-color: %1;"
            "}"
            "QSpinBox::up-button, QSpinBox::down-button {"
            " subcontrol-origin: border; width: 18px;"
            " background: transparent; border: none;"
            "}"
            "QSpinBox::up-button { subcontrol-position: top right; }"
            "QSpinBox::down-button { subcontrol-position: bottom right; }"
            "QSpinBox::up-arrow, QSpinBox::down-arrow {"
            " image: none; width: 0; height: 0;"
            "}")
                              .arg(surface.background.name(),
                                   surface.foreground.name()));

        if (QLineEdit* editor = lineEdit()) {
            StartupDiagnostics::setAppearancePalette(editor, paletteForThemeSurface(editor->palette(), surface));
            setStyleSheetIfChanged(editor, QStringLiteral(
                "QLineEdit { background: transparent; color: %1; border: none; }")
                                      .arg(surface.foreground.name()));
        }
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        QSpinBox::paintEvent(event);
        if (!m_arrowColor.isValid())
            return;

        QStyleOptionSpinBox option;
        initStyleOption(&option);

        QColor arrowColor = m_arrowColor;
        if (!isEnabled())
            arrowColor.setAlphaF(0.55f);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(arrowColor, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);

        const auto drawArrow = [this, &option, &painter](QStyle::SubControl control, bool up) {
            const QRect rect = style()->subControlRect(QStyle::CC_SpinBox,
                                                       &option,
                                                       control,
                                                       this);
            const QPointF center = QRectF(rect).center();
            QPainterPath path;
            path.moveTo(center.x() - 3.5, center.y() + (up ? 1.5 : -1.5));
            path.lineTo(center.x(), center.y() + (up ? -1.5 : 1.5));
            path.lineTo(center.x() + 3.5, center.y() + (up ? 1.5 : -1.5));
            painter.drawPath(path);
        };

        drawArrow(QStyle::SC_SpinBoxUp, true);
        drawArrow(QStyle::SC_SpinBoxDown, false);
    }

private:
    QColor m_arrowColor;
};

void applyScrollBarColorsToScrollArea(QAbstractScrollArea* area, const ThemeScrollBarColors& colors)
{
    if (area == nullptr)
        return;

    const QString styleSheet = scrollBarStyleSheet(colors);
    if (QScrollBar* bar = area->verticalScrollBar())
        setStyleSheetIfChanged(bar, styleSheet);
    if (QScrollBar* bar = area->horizontalScrollBar())
        setStyleSheetIfChanged(bar, styleSheet);
    applyScrollCornerColorToScrollArea(area, colors.track);
}

QIcon dockTitleButtonIcon(bool isCloseButton, const QColor& fill, const QColor& foreground)
{
    const auto pixmapForScale = [isCloseButton, &fill, &foreground](qreal scale) {
        constexpr int logicalSize = 18;
        const int physicalSize = qRound(logicalSize * scale);
        QPixmap pixmap(physicalSize, physicalSize);
        pixmap.setDevicePixelRatio(scale);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Qt::NoPen);
        painter.setBrush(fill);
        painter.drawEllipse(QRectF(0.5, 0.5, 17.0, 17.0));

        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(foreground, 1.35, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        if (isCloseButton) {
            painter.drawLine(QPointF(6.5, 6.5), QPointF(11.5, 11.5));
            painter.drawLine(QPointF(11.5, 6.5), QPointF(6.5, 11.5));
        } else {
            painter.drawRect(QRectF(6.5, 6.5, 5.0, 5.0));
        }
        return pixmap;
    };

    QIcon icon;
    icon.addPixmap(pixmapForScale(1.0));
    icon.addPixmap(pixmapForScale(2.0));
    icon.addPixmap(pixmapForScale(3.0));
    return icon;
}

void applyDockTitleButtonIcon(QAbstractButton* button, bool hovered)
{
    if (button == nullptr)
        return;

    const bool closeButton =
        button->property("speedcrunchDockHeaderCloseButton").toBool();
    const QColor fill =
        button->property(hovered
                             ? "speedcrunchDockHeaderButtonHoverFill"
                             : "speedcrunchDockHeaderButtonFill").value<QColor>();
    const QColor foreground =
        button->property(hovered
                             ? "speedcrunchDockHeaderButtonHoverForeground"
                             : "speedcrunchDockHeaderButtonForeground").value<QColor>();
    button->setIcon(dockTitleButtonIcon(closeButton, fill, foreground));
}

void applyDockSearchClearButtonIcon(QLineEdit* searchBox,
                                    const GeneratedThemeSurfaces& surfaces,
                                    bool focused)
{
    if (searchBox == nullptr)
        return;

    const ThemeSurfaceColors clearButtonSurface = focused
        ? surfaces.primary
        : themeSurfaceForShadeIndex(surfaces, UiConfig::DockTextInputOutlineShade);
    LineEditStyleUtils::applyClearButtonColors(searchBox, clearButtonSurface.background,
                                              clearButtonSurface.foreground);
}

void applyDockSearchClearButtonIcon(QLineEdit* searchBox,
                                    const GeneratedThemeSurfaces& surfaces)
{
    applyDockSearchClearButtonIcon(searchBox, surfaces, searchBox != nullptr && searchBox->hasFocus());
}

void applySurfaceToLabel(QLabel* label, const ThemeSurfaceColors& surface)
{
    if (label == nullptr)
        return;

    setDockPaletteIfChanged(label, paletteForThemeSurface(label->palette(), surface));
    setStyleSheetIfChanged(label, QStringLiteral(
        "QLabel { background-color: %1; color: %2; }")
                             .arg(surface.background.name(),
                                  surface.foreground.name()));
}

void applyNoMatchLabelSurface(QAbstractItemView* view, const ThemeSurfaceColors& surface)
{
    if (view == nullptr || view->viewport() == nullptr)
        return;

    for (QLabel* label : view->viewport()->findChildren<QLabel*>(QString(), Qt::FindDirectChildrenOnly)) {
        if (!label->property("dockListNoMatchLabel").toBool())
            continue;
        setDockPaletteIfChanged(label, paletteForThemeSurface(label->palette(), surface));
        setStyleSheetIfChanged(label, QStringLiteral("QLabel { background: transparent; color: %1; }")
                                 .arg(surface.foreground.name()));
    }
}

bool isStructuralDockBackgroundWidget(QWidget* widget)
{
    if (widget == nullptr || widget->window()->windowType() == Qt::Popup)
        return false;
    if (qobject_cast<QAbstractButton*>(widget)
        || qobject_cast<QAbstractScrollArea*>(widget)
        || qobject_cast<QComboBox*>(widget)
        || qobject_cast<QHeaderView*>(widget)
        || qobject_cast<QLabel*>(widget)
        || qobject_cast<QLineEdit*>(widget)
        || qobject_cast<QMenu*>(widget)
        || qobject_cast<QScrollBar*>(widget)
        || qobject_cast<BitFieldWidget*>(widget)) {
        return false;
    }
    if (qobject_cast<QAbstractScrollArea*>(widget->parentWidget()))
        return false;
    return true;
}

void applySurfaceToStructuralDockWidget(QWidget* widget, const ThemeSurfaceColors& surface)
{
    if (!isStructuralDockBackgroundWidget(widget))
        return;

    setDockPaletteIfChanged(widget, paletteForThemeSurface(widget->palette(), surface));
    widget->setAutoFillBackground(true);
    widget->setAttribute(Qt::WA_StyledBackground, true);
    // Dock containers share their stylesheet with child menus. Limit these
    // colors to the structural surfaces so the menu setting controls popups.
    widget->setProperty("speedcrunchDockSurface", true);
    setStyleSheetIfChanged(widget, QStringLiteral(
        "QWidget[speedcrunchDockSurface=\"true\"] { background-color: %1; color: %2; }")
                              .arg(surface.background.name(),
                                   surface.foreground.name()));
}

void applySurfaceToContainingRow(QWidget* root, QWidget* item, const ThemeSurfaceColors& surface)
{
    if (root == nullptr || item == nullptr)
        return;

    QList<QWidget*> candidates = root->findChildren<QWidget*>();
    candidates.prepend(root);
    for (QWidget* candidate : candidates) {
        QLayout* layout = candidate->layout();
        if (layout == nullptr || layout->indexOf(item) < 0)
            continue;

        applySurfaceToStructuralDockWidget(candidate, surface);
        for (int index = 0; index < layout->count(); ++index) {
            applySurfaceToLabel(qobject_cast<QLabel*>(layout->itemAt(index)->widget()), surface);
        }
        return;
    }
}

bool isComboBoxPopupView(const QAbstractItemView* view, const QList<QComboBox*>& comboBoxes)
{
    for (const QComboBox* comboBox : comboBoxes) {
        if (comboBox->view() == view || comboBox->isAncestorOf(view))
            return true;
    }
    return false;
}

void updateDockSystemTabCursor(QTabBar* tabBar, const QPoint& pos)
{
    if (tabBar == nullptr)
        return;

    tabBar->setCursor(tabBar->tabAt(pos) >= 0
                          ? Qt::PointingHandCursor
                          : Qt::ArrowCursor);
}

void applyDockChromeAppearance(MainWindow* owner,
                               QDockWidget* dock,
                               const GeneratedThemeSurfaces& surfaces,
                               bool forceNativePolish)
{
    if (dock == nullptr)
        return;

    StartupDiagnostics::AppearanceScope timing("Apply dock chrome appearance", dock);
    StartupDiagnostics::observeAppearanceSettings(timing);

    saveNativeDockSurface(dock);
    QList<QAbstractButton*> titleButtons;
    for (QAbstractButton* button : dock->findChildren<QAbstractButton*>(
             QString(), Qt::FindDirectChildrenOnly)) {
        if (button->objectName() != QLatin1String("qt_dockwidget_closebutton")
            && button->objectName() != QLatin1String("qt_dockwidget_floatbutton"))
            continue;
        saveNativeDockSurface(button);
        if (!button->property("speedcrunchNativeDockHeaderButton").isValid()) {
            const QVariantList state {button->minimumSize(), button->maximumSize(), button->iconSize(),
                                      QVariant::fromValue(button->cursor()), button->hasMouseTracking(),
                                      button->testAttribute(Qt::WA_Hover)};
            button->setProperty("speedcrunchNativeDockHeaderButton", state);
        }
        titleButtons.append(button);
    }
    if (Settings::instance()->dockAppearance == Settings::DockAppearanceSystem) {
        for (QAbstractButton* button : titleButtons) {
            button->setProperty("speedcrunchDockHeaderButton", false);
            if (owner != nullptr)
                button->removeEventFilter(owner);
            restoreNativeDockSurface(button, forceNativePolish);
            const QVariantList state = button->property("speedcrunchNativeDockHeaderButton").toList();
            button->setMinimumSize(state.at(0).toSize());
            button->setMaximumSize(state.at(1).toSize());
            button->setIconSize(state.at(2).toSize());
            button->setCursor(state.at(3).value<QCursor>());
            button->setMouseTracking(state.at(4).toBool());
            button->setAttribute(Qt::WA_Hover, state.at(5).toBool());
        }
        restoreNativeDockSurface(dock, forceNativePolish);
        // Some system styles leave the title background transparent. Fill the
        // dock with its system palette so the main window theme cannot show through.
        dock->setAutoFillBackground(true);
        // Let Qt refresh native title icons and layout for the current dock state.
        QEvent styleChange(QEvent::StyleChange);
        QCoreApplication::sendEvent(dock, &styleChange);
        dock->updateGeometry();
        dock->update();
        return;
    }

    const ThemeSurfaceColors titleButton =
        themeSurfaceForShadeIndex(surfaces, UiConfig::DockHeaderButtonFillShade);
    const ThemeSurfaceColors titleButtonHover =
        themeSurfaceForShadeIndex(surfaces, UiConfig::DockHeaderButtonHoverFillShade);
    setDockPaletteIfChanged(dock, paletteForThemeSurface(dock->palette(), surfaces.headersAndBorders));
    const bool styleChanged = setStyleSheetIfChanged(dock, QStringLiteral(
        "QDockWidget {"
        " color: %2;"
        "}"
        "QDockWidget::title {"
        " background-color: %1; color: %2; padding: 5px 4px;"
        "}"
        "QDockWidget::close-button, QDockWidget::float-button {"
        " background-color: transparent; border: none; padding: 0px;"
        " width: 18px; height: 18px;"
        "}"
        "QAbstractButton#qt_dockwidget_closebutton,"
        "QAbstractButton#qt_dockwidget_floatbutton {"
        " background-color: %3; color: %4;"
        " border: none; border-radius: 9px; padding: 0px;"
        " min-width: 18px; max-width: 18px;"
        " min-height: 18px; max-height: 18px;"
        "}"
        "QAbstractButton#qt_dockwidget_closebutton:hover,"
        "QAbstractButton#qt_dockwidget_floatbutton:hover {"
        " background-color: %5; color: %6;"
        "}")
                            .arg(surfaces.headersAndBorders.background.name(),
                                 surfaces.headersAndBorders.foreground.name(),
                                 titleButton.background.name(),
                                 titleButton.foreground.name(),
                                 titleButtonHover.background.name(),
                                 titleButtonHover.foreground.name()));
    if (styleChanged) {
        dock->style()->unpolish(dock);
        dock->style()->polish(dock);
        dock->setWindowTitle(dock->windowTitle());
        dock->update();
    }
    for (QAbstractButton* button : titleButtons) {
        const bool closeButton = button->objectName() == QLatin1String("qt_dockwidget_closebutton");
        button->setProperty("speedcrunchDockHeaderButton", true);
        button->setProperty("speedcrunchDockHeaderCloseButton", closeButton);
        button->setProperty("speedcrunchDockHeaderButtonFill", titleButton.background);
        button->setProperty("speedcrunchDockHeaderButtonForeground", titleButton.foreground);
        button->setProperty("speedcrunchDockHeaderButtonHoverFill", titleButtonHover.background);
        button->setProperty("speedcrunchDockHeaderButtonHoverForeground",
                            titleButtonHover.foreground);
        if (owner != nullptr)
            button->installEventFilter(owner);
        button->setCursor(Qt::PointingHandCursor);
        button->setMouseTracking(true);
        button->setAttribute(Qt::WA_Hover, true);
        button->setFixedSize(QSize(18, 18));
        button->setIconSize(QSize(18, 18));
        setStyleSheetIfChanged(button, QStringLiteral(
            "QAbstractButton {"
            " background-color: %1; color: %2;"
            " border: none; border-radius: 9px; padding: 0px;"
            " min-width: 18px; max-width: 18px;"
            " min-height: 18px; max-height: 18px;"
            "}"
            "QAbstractButton:hover {"
            " background-color: %3; color: %4;"
            "}").arg(titleButton.background.name(),
                     titleButton.foreground.name(),
                     titleButtonHover.background.name(),
                     titleButtonHover.foreground.name()));
        setDockPaletteIfChanged(button, paletteForThemeSurface(button->palette(), titleButton));
        applyDockTitleButtonIcon(button, button->underMouse());
    }
}

void saveNativeDockSurface(QWidget* widget)
{
    // Keep frame and background settings before the first theme application.
    if (widget->property("speedcrunchNativeDockSurface").isValid())
        return;
    QVariantList state {widget->styleSheet(), widget->autoFillBackground(),
                        widget->testAttribute(Qt::WA_StyledBackground)};
    if (QFrame* frame = qobject_cast<QFrame*>(widget)) {
        state << int(frame->frameShape()) << int(frame->frameShadow())
              << frame->lineWidth() << frame->midLineWidth();
    }
    widget->setProperty("speedcrunchNativeDockSurface", state);
    if (QAbstractButton* button = qobject_cast<QAbstractButton*>(widget))
        button->setProperty("speedcrunchNativeDockIcon", button->icon());
    if (QLineEdit* edit = qobject_cast<QLineEdit*>(widget)) {
        for (QAction* action : edit->actions())
            action->setProperty("speedcrunchNativeDockIcon", action->icon());
    }
}

void restoreNativeDockSurface(QWidget* widget, bool forceNativePolish)
{
    const QVariantList state = widget->property("speedcrunchNativeDockSurface").toList();
    if (state.size() < 3)
        return;
    const QString styleSheet = widget->property("dockListNoMatchLabel").toBool()
        ? QStringLiteral("QLabel { background: transparent; }") : state.at(0).toString();
    const bool styleSheetChanged = setStyleSheetIfChanged(widget, styleSheet);
    bool needsPolish = forceNativePolish || styleSheetChanged;
    if (!needsPolish) {
        const QPointer<QStyle> lastPolishedStyle =
            widget->property("speedcrunchNativeDockPolishedStyle").value<QPointer<QStyle>>();
        const QVariant sourcePalette = widget->property("speedcrunchNativeDockSourcePalette");
        const QVariant restoredPalette = widget->property("speedcrunchNativeDockRestoredPalette");
        // Stylesheets can alter native palette roles during polish.
        needsPolish = lastPolishedStyle != widget->style()
            || !sourcePalette.isValid() || !restoredPalette.isValid()
            || sourcePalette.value<QPalette>() != MenuStyle::systemPalette(widget)
            || widget->palette() != restoredPalette.value<QPalette>();
    }
    if (needsPolish) {
        StartupDiagnostics::Scope timing("Restore native dock style");
        widget->style()->unpolish(widget);
        const QPalette nativePalette = MenuStyle::systemPalette(widget);
        StartupDiagnostics::setAppearancePalette(widget, nativePalette);
        widget->style()->polish(widget);
        widget->setProperty("speedcrunchNativeDockSourcePalette", nativePalette);
    } else {
        StartupDiagnostics::appearanceAssignment(widget, true, false, true);
    }
    widget->setAutoFillBackground(state.at(1).toBool());
    widget->setAttribute(Qt::WA_StyledBackground, state.at(2).toBool());
    if (QFrame* frame = qobject_cast<QFrame*>(widget)) {
        if (state.size() == 7) {
            frame->setFrameShape(static_cast<QFrame::Shape>(state.at(3).toInt()));
            frame->setFrameShadow(static_cast<QFrame::Shadow>(state.at(4).toInt()));
            frame->setLineWidth(state.at(5).toInt());
            frame->setMidLineWidth(state.at(6).toInt());
        }
    }
    if (QAbstractButton* button = qobject_cast<QAbstractButton*>(widget))
        button->setIcon(button->property("speedcrunchNativeDockIcon").value<QIcon>());
    for (const char* property : {"dockListHoverBackground", "dockListHoverForeground",
                                 "dockListActiveSelectionBackground", "dockListActiveSelectionForeground",
                                 "dockListInactiveSelectionBackground", "dockListInactiveSelectionForeground"})
        widget->setProperty(property, QVariant());
    if (ThemedLineEdit* edit = dynamic_cast<ThemedLineEdit*>(widget))
        edit->setCursorColor(QColor());
    if (QLineEdit* edit = qobject_cast<QLineEdit*>(widget)) {
        for (QAction* action : edit->actions())
            action->setIcon(action->property("speedcrunchNativeDockIcon").value<QIcon>());
    }
    if (needsPolish) {
        widget->setProperty("speedcrunchNativeDockPolishedStyle",
                            QVariant::fromValue(QPointer<QStyle>(widget->style())));
        widget->setProperty("speedcrunchNativeDockRestoredPalette", widget->palette());
    }
    widget->update();
}

void alignDockSearchRow(QWidget* content)
{
    QLineEdit* search = content->findChild<QLineEdit*>();
    if (!search || !search->parentWidget()->layout())
        return;

    const auto combos = content->findChildren<QComboBox*>();
    if (combos.isEmpty())
        return;
    int bottomInset = 0;
    for (QComboBox* combo : combos) {
        QStyleOptionComboBox option;
        option.initFrom(combo);
        option.rect = QRect(QPoint(), combo->sizeHint());
        const QRect itemRect = combo->style()->subElementRect(QStyle::SE_ComboBoxLayoutItem, &option, combo);
        if (!itemRect.isEmpty())
            bottomInset = qMax(bottomInset, option.rect.bottom() - itemRect.bottom());
    }
    // Native dropdowns reserve space below their painted bounds in the row above.
    QLayout* layout = search->parentWidget()->layout();
    QMargins margins = layout->contentsMargins();
    margins.setTop(qMax(0, UiConfig::DockSearchRowVerticalPadding - bottomInset));
    layout->setContentsMargins(margins);
}

void applyDockContentAppearance(MainWindow* owner, QDockWidget* dock,
                                const GeneratedThemeSurfaces& surfaces,
                                const QList<QWidget*>& widgets, bool forceNativePolish)
{
    StartupDiagnostics::AppearanceScope timing("Apply dock content appearance", dock);
    StartupDiagnostics::observeAppearanceSettings(timing);
    QWidget* dockContent = dock->widget();
    if (dockContent == nullptr)
        return;

    if (Settings::instance()->dockAppearance == Settings::DockAppearanceSystem) {
        for (QWidget* widget : widgets) {
            if (!qobject_cast<QMenu*>(widget))
                restoreNativeDockSurface(widget, forceNativePolish);
        }
        dockContent->setAutoFillBackground(true);
        if (BookDock* book = qobject_cast<BookDock*>(dock)) {
            const QPalette palette = MenuStyle::systemPalette(book->findChild<QTextBrowser*>());
            book->setContentSurfaceColors(palette.color(QPalette::Base), palette.color(QPalette::Text));
        }
        if (BitFieldWidget* bitField = qobject_cast<BitFieldWidget*>(dockContent)) {
            bitField->setThemeColors(QColor(), QColor());
            const QPalette palette = MenuStyle::systemPalette(bitField);
            bitField->setToolTipThemeColors(palette.color(QPalette::ToolTipBase),
                                           palette.color(QPalette::ToolTipText),
                                           palette.color(QPalette::Mid), 0);
        }
        if (ConstantsWidget* constants = qobject_cast<ConstantsWidget*>(dockContent)) {
            const QPalette palette = MenuStyle::systemPalette(constants);
            constants->setSummaryPopupThemeColors(palette.color(QPalette::ToolTipBase),
                                                  palette.color(QPalette::ToolTipText),
                                                  palette.color(QPalette::Mid), 0);
        }
        for (QComboBox* combo : dockContent->findChildren<QComboBox*>())
            DockComboBoxChevron::applySystemAppearance(combo);
        alignDockSearchRow(dockContent);
        return;
    }

    const ThemeSurfaceColors dockBackground =
        themeSurfaceForShadeIndex(surfaces, UiConfig::DockBackgroundShade);
    const ThemeSurfaceColors dockHeader =
        themeSurfaceForShadeIndex(surfaces, UiConfig::DockHeaderShade);
    const ThemeSurfaceColors dockHoveredItem =
        themeSurfaceForShadeIndex(surfaces, UiConfig::DockHoveredItemShade);
    const int dockComboPopupShade =
        qMin(UiConfig::DockBackgroundShade + 1, UiConfig::Shade600);
    const ThemeSurfaceColors dockComboPopup =
        themeSurfaceForShadeIndex(surfaces, dockComboPopupShade);
    const ThemeSurfaceColors dockTextInput =
        themeSurfaceForShadeIndex(surfaces, UiConfig::DockTextInputShade);
    const ThemeSurfaceColors dockTextInputOutline =
        themeSurfaceForShadeIndex(surfaces, UiConfig::DockTextInputOutlineShade);
    const ThemeSurfaceColors dockUnfocusedSelection =
        themeSurfaceForShadeIndex(surfaces, UiConfig::DockUnfocusedSelectedItemShade);
    BitFieldWidget* bitField = qobject_cast<BitFieldWidget*>(dockContent);
    const QList<QComboBox*> comboBoxes = dockContent->findChildren<QComboBox*>();
    const ThemeSurfaceColors& dockSurface = dockBackground;

    setDockPaletteIfChanged(dockContent, paletteForThemeSurface(dockContent->palette(), dockSurface));
    dockContent->setAutoFillBackground(true);
    applySurfaceToStructuralDockWidget(dockContent, dockSurface);
    for (QWidget* child : dockContent->findChildren<QWidget*>())
        applySurfaceToStructuralDockWidget(child, dockSurface);
    if (bitField != nullptr) {
        const ThemeSurfaceColors bitfieldHover =
            themeSurfaceForShadeIndex(surfaces, UiConfig::BitfieldBitHoverShade);
        const ThemeSurfaceColors bitfieldButton =
            themeSurfaceForShadeIndex(surfaces, UiConfig::BitfieldButtonFillShade);
        const ThemeSurfaceColors bitfieldButtonHover =
            themeSurfaceForShadeIndex(surfaces, UiConfig::BitfieldButtonHoverFillShade);
        const ThemeSurfaceColors bitfieldButtonPressed =
            themeSurfaceForShadeIndex(surfaces, UiConfig::BitfieldButtonPressedFillShade);
        const ThemeSurfaceColors bitfieldToolTip =
            themeSurfaceForShadeIndex(surfaces, UiConfig::CompletionPopupBackgroundShade);
        const ThemeSurfaceColors bitfieldToolTipOutline =
            themeSurfaceForShadeIndex(surfaces, UiConfig::CompletionPopupOutlineShade);
        bitField->setToolTipThemeColors(bitfieldToolTip.background,
                                        bitfieldToolTip.foreground,
                                        bitfieldToolTipOutline.background,
                                        UiConfig::CompletionPopupCornerRadius);
        bitField->setThemeColors(dockBackground.background,
                                 dockBackground.foreground,
                                 bitfieldHover.background,
                                 bitfieldHover.foreground,
                                 dockUnfocusedSelection.background,
                                 dockUnfocusedSelection.foreground,
                                 surfaces.primary.background,
                                 surfaces.primary.foreground,
                                 bitfieldButton.background,
                                 bitfieldButton.foreground,
                                 bitfieldButtonHover.background,
                                 bitfieldButtonHover.foreground,
                                 bitfieldButtonPressed.background,
                                 bitfieldButtonPressed.foreground);
    }
    const ThemeScrollBarColors listScrollBars =
        scrollBarColorsForSurfaceIndex(surfaces, UiConfig::DockBackgroundShade);
    const ThemeScrollBarColors comboPopupScrollBars =
        scrollBarColorsForSurfaceIndex(surfaces, dockComboPopupShade);

    if (BookDock* bookDock = qobject_cast<BookDock*>(dock)) {
        bookDock->setContentSurfaceColors(dockBackground.background,
                                          dockBackground.foreground);
    }

    if (ConstantsWidget* constantsWidget = qobject_cast<ConstantsWidget*>(dockContent)) {
        const ThemeSurfaceColors completionPopup =
            themeSurfaceForShadeIndex(surfaces, UiConfig::CompletionPopupBackgroundShade);
        const ThemeSurfaceColors completionOutline =
            themeSurfaceForShadeIndex(surfaces, UiConfig::CompletionPopupOutlineShade);
        constantsWidget->setSummaryPopupThemeColors(completionPopup.background,
                                                    completionPopup.foreground,
                                                    completionOutline.background,
                                                    UiConfig::CompletionPopupCornerRadius);
    }

    for (QAbstractScrollArea* scrollArea : dockContent->findChildren<QAbstractScrollArea*>()) {
        if (qobject_cast<QAbstractItemView*>(scrollArea))
            continue;
        applyScrollBarColorsToScrollArea(scrollArea, listScrollBars);
    }

    for (QComboBox* comboBox : comboBoxes) {
        applySurfaceToContainingRow(dockContent, comboBox, dockSurface);
        setDockPaletteIfChanged(comboBox, paletteForThemeSurface(comboBox->palette(), dockTextInput));
        DockComboBoxChevron::PopupTheme popupTheme;
        popupTheme.buttonStyle = QStringLiteral(
            "QComboBox {"
            " background-color: %1; color: %2;"
            " border: 1px solid %3; border-radius: %6px; padding: 4px %4px 4px 8px;"
            "}"
            "QComboBox::drop-down {"
            " subcontrol-origin: border; subcontrol-position: top right;"
            " width: %5px; border: none; background: transparent;"
            "}"
            "QComboBox::down-arrow {"
            " image: none; width: 0px; height: 0px;"
            "}")
                                    .arg(dockTextInput.background.name(),
                                         dockTextInput.foreground.name(),
                                         dockTextInputOutline.background.name())
                                    .arg(DockComboBoxChevron::IndicatorWidth + 4)
                                    .arg(DockComboBoxChevron::IndicatorWidth)
                                    .arg(UiConfig::ControlCornerRadius);
        if (QAbstractItemView* popupView = comboBox->view()) {
            QPalette popupPalette = paletteForThemeSurface(MenuStyle::systemPalette(popupView), dockComboPopup);
            popupPalette.setColor(QPalette::Highlight, dockHoveredItem.background);
            popupPalette.setColor(QPalette::HighlightedText, dockHoveredItem.foreground);
            popupTheme.viewStyle = QStringLiteral(
                "QAbstractItemView {"
                " background-color: %1; color: %2;"
                " border: 0; border-radius: %7px; outline: 0;"
                " padding: %3px %4px;"
                "}"
                "QAbstractItemView::item {"
                " border: 0; border-radius: %8px;"
                "}"
                "QAbstractItemView::item:hover {"
                " background-color: %5; color: %6;"
                " border-radius: %8px;"
                "}")
                                         .arg(dockComboPopup.background.name(),
                                              dockComboPopup.foreground.name())
                                         .arg(kDockComboPopupVerticalPadding)
                                         .arg(kDockComboPopupHorizontalPadding)
                                         .arg(dockHoveredItem.background.name(),
                                              dockHoveredItem.foreground.name())
                                         .arg(UiConfig::CompletionPopupCornerRadius)
                                         .arg(UiConfig::DockHoveredItemCornerRadius);
            popupTheme.palette = popupPalette;
            popupTheme.scrollBarStyle = scrollBarStyleSheet(comboPopupScrollBars);
            DockComboBoxChevron::apply(comboBox, dockTextInput.foreground,
                                       dockTextInputOutline.background, popupTheme);
        }
    }

    for (QLineEdit* searchBox : dockContent->findChildren<QLineEdit*>()) {
        applySurfaceToContainingRow(dockContent, searchBox, dockSurface);
        searchBox->setProperty("speedcrunchDockTextInput", true);
        if (owner != nullptr)
            searchBox->installEventFilter(owner);
        setDockPaletteIfChanged(searchBox, paletteForThemeSurface(searchBox->palette(), dockTextInput));
        if (ThemedLineEdit* themedSearchBox = dynamic_cast<ThemedLineEdit*>(searchBox))
            themedSearchBox->setCursorColor(surfaces.primary.background);
        setStyleSheetIfChanged(searchBox, QStringLiteral(
            "QLineEdit {"
            " background-color: %1; color: %2;"
            " border: %5px solid %3; border-radius: %7px; padding: 4px 8px;"
            "}"
            "QLineEdit:focus {"
            " border: %6px solid %4;"
            "}")
                                     .arg(dockTextInput.background.name(),
                                          dockTextInput.foreground.name(),
                                          dockTextInputOutline.background.name(),
                                          surfaces.primary.background.name())
                                     .arg(UiConfig::DockTextInputUnfocusedOutlineStrokeWidth)
                                     .arg(UiConfig::OutlineStrokeWidth)
                                     .arg(UiConfig::ControlCornerRadius));
        applyDockSearchClearButtonIcon(searchBox, surfaces);
    }

    for (QAbstractItemView* view : dockContent->findChildren<QAbstractItemView*>()) {
        if (qobject_cast<QHeaderView*>(view) || isComboBoxPopupView(view, comboBoxes))
            continue;
        view->setFrameShape(QFrame::NoFrame);
        QPalette palette = paletteForThemeSurface(view->palette(), dockBackground);
        palette.setColor(QPalette::Active, QPalette::Highlight, surfaces.primary.background);
        palette.setColor(QPalette::Active, QPalette::HighlightedText, surfaces.primary.foreground);
        palette.setColor(QPalette::Inactive, QPalette::Highlight, dockUnfocusedSelection.background);
        palette.setColor(QPalette::Inactive, QPalette::HighlightedText, dockUnfocusedSelection.foreground);
        palette.setColor(QPalette::Disabled, QPalette::Highlight, dockUnfocusedSelection.background);
        palette.setColor(QPalette::Disabled, QPalette::HighlightedText, dockUnfocusedSelection.foreground);
        view->setProperty("dockListHoverBackground", dockHoveredItem.background);
        view->setProperty("dockListHoverForeground", dockHoveredItem.foreground);
        view->setProperty("dockListActiveSelectionBackground", surfaces.primary.background);
        view->setProperty("dockListActiveSelectionForeground", surfaces.primary.foreground);
        view->setProperty("dockListInactiveSelectionBackground", dockUnfocusedSelection.background);
        view->setProperty("dockListInactiveSelectionForeground", dockUnfocusedSelection.foreground);
        setStyleSheetIfChanged(view, QStringLiteral(
            "QAbstractItemView {"
            " background-color: %1; color: %2;"
            " border: 0;"
            " padding: %3px;"
            "}")
                                .arg(dockBackground.background.name(),
                                     dockBackground.foreground.name())
                                .arg(UiConfig::DockListPadding)
                            + scrollBarStyleSheet(listScrollBars));
        setDockPaletteIfChanged(view, palette);
        setDockPaletteIfChanged(view->viewport(), palette);
        applyScrollBarColorsToScrollArea(view, listScrollBars);
        applyNoMatchLabelSurface(view, dockBackground);
    }

    for (QHeaderView* header : dockContent->findChildren<QHeaderView*>()) {
        const QPointer<QAbstractItemModel> measuredModel =
            header->property("speedcrunchNativeHeaderModel").value<QPointer<QAbstractItemModel>>();
        if (measuredModel != header->model()) {
            header->setProperty("speedcrunchNativeHeaderMetrics", QVariant());
            QAbstractItemModel* model = header->model();
            if (model != nullptr) {
                const auto invalidateMetrics = [header, model]() {
                    if (header->model() == model)
                        header->setProperty("speedcrunchNativeHeaderMetrics", QVariant());
                };
                QObject::connect(model, &QAbstractItemModel::headerDataChanged, header,
                    [header, invalidateMetrics](Qt::Orientation orientation, int, int) {
                        if (orientation == header->orientation())
                            invalidateMetrics();
                    });
                QObject::connect(model, &QAbstractItemModel::modelReset, header, invalidateMetrics);
            }
            header->setProperty("speedcrunchNativeHeaderModel",
                                QVariant::fromValue(QPointer<QAbstractItemModel>(model)));
        }
        const auto metricInputs = [header]() {
            return QVariantList {header->font(), MenuStyle::systemPalette(header),
                header->logicalDpiX(), header->logicalDpiY(), int(header->layoutDirection()), header->count()};
        };
        const QVariantList metrics = header->property("speedcrunchNativeHeaderMetrics").toList();
        const QPointer<QStyle> measuredStyle =
            header->property("speedcrunchNativeHeaderStyle").value<QPointer<QStyle>>();
        const bool reuseMetrics = !forceNativePolish && metrics.size() == 2
            && measuredStyle == header->style()
            && header->property("speedcrunchNativeHeaderInputs").toList() == metricInputs()
            && header->property("speedcrunchThemedHeaderStyleSheet").toString() == header->styleSheet();
        int headerMargin;
        int headerContentHeight;
        if (reuseMetrics) {
            StartupDiagnostics::Scope headerMetricsTiming("Reuse native dock header metrics");
            headerMargin = metrics.at(0).toInt();
            headerContentHeight = metrics.at(1).toInt();
        } else {
            StartupDiagnostics::Scope headerMetricsTiming("Measure native dock header metrics");
            restoreNativeDockSurface(header);
            QStyleOptionHeader option;
            option.initFrom(header);
            headerMargin = qMax(0, header->style()->pixelMetric(QStyle::PM_HeaderMargin, &option, header));
            // Keep the native height, including the two themed border strokes.
            headerContentHeight = qMax(0, header->sizeHint().height() - 2);
        }
        setDockPaletteIfChanged(header, paletteForThemeSurface(header->palette(), dockBackground));
        setStyleSheetIfChanged(header, QStringLiteral(
            "QHeaderView { background-color: %1; color: %2; border: 0; padding: 0; }"
            "QHeaderView::section {"
            " background-color: %1; color: %2;"
            " border: 0;"
            " border-top: 1px solid %3;"
            " border-right: 1px solid %3;"
            " border-bottom: 1px solid %3;"
            " padding: 0px %4px;"
            " height: %5px;"
            "}"
            "QHeaderView::section:first {"
            " border-left: 0;"
            "}"
            "QHeaderView::section:last {"
            " border-right: 0;"
            "}")
                                  .arg(dockBackground.background.name(),
                                       dockBackground.foreground.name(),
                                       dockHeader.background.name())
                                  .arg(headerMargin)
                                  .arg(headerContentHeight));
        if (!reuseMetrics) {
            header->setProperty("speedcrunchNativeHeaderMetrics", QVariantList {headerMargin, headerContentHeight});
            header->setProperty("speedcrunchNativeHeaderStyle", QVariant::fromValue(QPointer<QStyle>(header->style())));
            header->setProperty("speedcrunchNativeHeaderInputs", metricInputs());
            header->setProperty("speedcrunchThemedHeaderStyleSheet", header->styleSheet());
        }
    }
    alignDockSearchRow(dockContent);
}

void applyDockAppearance(MainWindow* owner, QDockWidget* dock,
                         const GeneratedThemeSurfaces& surfaces, bool forceNativePolish = true)
{
    if (dock == nullptr)
        return;
    StartupDiagnostics::AppearanceScope timing("Apply dock appearance", dock);
    QList<QWidget*> widgets;
    if (QWidget* content = dock->widget()) {
        widgets = content->findChildren<QWidget*>();
        widgets.prepend(content);
        // Capture native state before the parent's stylesheet can polish children.
        for (QWidget* widget : widgets) {
            if (!qobject_cast<QMenu*>(widget))
                saveNativeDockSurface(widget);
        }
    }
    // Set the parent style first so it does not repolish freshly styled contents.
    applyDockChromeAppearance(owner, dock, surfaces, forceNativePolish);
    applyDockContentAppearance(owner, dock, surfaces, widgets, forceNativePolish);
}

void applyThemeBackgroundRoleToWidget(QWidget* widget, const QColor& background)
{
    if (widget == nullptr)
        return;
    StartupDiagnostics::AppearanceScope timing("Apply themed widget background", widget);

    QPalette pal = widget->palette();
    for (const QPalette::ColorGroup group : {QPalette::Active,
                                             QPalette::Inactive,
                                             QPalette::Disabled}) {
        pal.setColor(group, QPalette::Window, background);
        pal.setColor(group, QPalette::Base, background);
        pal.setColor(group, QPalette::Button, background);
    }
    StartupDiagnostics::setAppearancePalette(widget, pal);
    widget->setAutoFillBackground(true);
    widget->setAttribute(Qt::WA_StyledBackground, true);
    if (!qobject_cast<QSplitter*>(widget)) {
        // Keep the surface color local. An unqualified stylesheet also paints
        // child menus, even when they have a complete system palette.
        widget->setProperty("speedcrunchThemeSurfaceBackground", true);
        setStyleSheetIfChanged(widget, QStringLiteral(
            "QWidget[speedcrunchThemeSurfaceBackground=\"true\"] { background-color: %1; }")
                                 .arg(background.name()));
    }
}

void applyWindowThemeBackground(QWidget* window, const ThemeSurfaceColors& surface)
{
    StartupDiagnostics::AppearanceScope timing("Apply window background", window);
    StartupDiagnostics::setAppearancePalette(window, paletteForThemeSurface(window->palette(), surface));
    window->setAutoFillBackground(true);
    window->setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheetIfChanged(window, QStringLiteral("QMainWindow { background-color: %1; }")
                      .arg(surface.background.name()));
}

void applyDockTabBarBackgroundToWidget(QWidget* widget, const ThemeSurfaceColors& surface)
{
    if (widget == nullptr)
        return;

    widget->setProperty("speedcrunchDockTabBackground", true);
    StartupDiagnostics::setAppearancePalette(widget, paletteForThemeSurface(widget->palette(), surface));
    widget->setAutoFillBackground(true);
    widget->setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheetIfChanged(widget, QStringLiteral(
        "QWidget[speedcrunchDockTabBackground=\"true\"] {"
        " background-color: %1; border: 0;"
        "}")
                              .arg(surface.background.name()));
}

QString oklchThemeReportPath()
{
    return QDir(QDir::tempPath()).absoluteFilePath(
        QStringLiteral("speedcrunch-oklch-theme-report.html"));
}

// Theme application can run multiple times while startup restores widgets, but
// later user theme changes still need a fresh diagnostics report. Keying by the
// generated colors suppresses duplicate writes without turning the report into a
// process-wide one-shot.
QString oklchThemeReportKey(const GeneratedThemeSurfaces& surfaces)
{
    QStringList colors;
    colors.reserve(surfaces.backgrounds.size() + surfaces.foregrounds.size() + 2);
    colors.append(surfaces.base.name(QColor::HexArgb));
    colors.append(QString::number(static_cast<int>(surfaces.polarity)));
    for (const QColor& color : surfaces.backgrounds)
        colors.append(color.name(QColor::HexArgb));
    for (const QColor& color : surfaces.foregrounds)
        colors.append(color.name(QColor::HexArgb));
    return colors.join(QLatin1Char('|'));
}

QString& lastOklchThemeReportKey()
{
    static QString key;
    return key;
}

QString debugColorName(const QColor& color)
{
    return color.isValid()
        ? color.name(QColor::HexRgb).toUpper()
        : QStringLiteral("(invalid)");
}

QString paletteColorName(const QWidget* widget, QPalette::ColorRole role)
{
    return widget ? debugColorName(widget->palette().color(role)) : QStringLiteral("(missing)");
}

QString grabbedCenterColorName(QWidget* widget)
{
    if (widget == nullptr || widget->size().isEmpty() || !widget->isVisible())
        return QStringLiteral("(unavailable)");

    const QPixmap pixmap = widget->grab();
    const QImage image = pixmap.toImage();
    if (image.isNull() || image.width() <= 0 || image.height() <= 0)
        return QStringLiteral("(unavailable)");

    return debugColorName(image.pixelColor(image.width() / 2, image.height() / 2));
}

QString htmlTableCell(const QString& value)
{
    return QStringLiteral("<td><code>%1</code></td>").arg(value.toHtmlEscaped());
}

void appendDiagnosticRow(QTextStream& out,
                         const QString& widget,
                         const QString& expected,
                         const QString& palette,
                         const QString& viewportPalette,
                         const QString& grab,
                         const QString& styleSheet)
{
    out << "<tr><th scope=\"row\">" << widget.toHtmlEscaped() << "</th>"
        << htmlTableCell(expected)
        << htmlTableCell(palette)
        << htmlTableCell(viewportPalette)
        << htmlTableCell(grab)
        << htmlTableCell(styleSheet)
        << "</tr>\n";
}

QString shortStyleSheet(const QWidget* widget)
{
    if (widget == nullptr)
        return QStringLiteral("(missing)");

    QString styleSheet = widget->styleSheet().simplified();
    constexpr int kMaximumStyleSheetLength = 120;
    if (styleSheet.size() > kMaximumStyleSheetLength)
        styleSheet = styleSheet.left(kMaximumStyleSheetLength) + QStringLiteral("...");
    return styleSheet.isEmpty() ? QStringLiteral("(empty)") : styleSheet;
}

static void typeTextThroughEditorInputRules(Editor* editor, const QString& text)
{
    if (!editor || text.isEmpty())
        return;

    const auto keyForChar = [](QChar ch) -> int {
        switch (ch.unicode()) {
        case MathDsl::DotSep.unicode(): return Qt::Key_Period;
        case MathDsl::CommaSep.unicode(): return Qt::Key_Comma;
        case MathDsl::AddOp.unicode(): return Qt::Key_Plus;
        case MathDsl::SubOpAl1.unicode(): return Qt::Key_Minus;
        case MathDsl::DivOp.unicode(): return Qt::Key_Slash;
        case MathDsl::MulOpAl1.unicode(): return Qt::Key_Asterisk;
        case MathDsl::PowOp.unicode(): return Qt::Key_AsciiCircum;
        case MathDsl::GroupStart.unicode(): return Qt::Key_ParenLeft;
        case MathDsl::GroupEnd.unicode(): return Qt::Key_ParenRight;
        case MathDsl::PercentOp.unicode(): return Qt::Key_Percent;
        case MathDsl::FactorOp.unicode(): return Qt::Key_Exclam;
        default: break;
        }
        if (ch.isDigit())
            return Qt::Key_0 + (ch.unicode() - MathDsl::Dig0.unicode());
        return Qt::Key_unknown;
    };

    for (const QChar ch : text) {
        const int key = keyForChar(ch);
        QKeyEvent keyEvent(QEvent::KeyPress, key, Qt::NoModifier, QString(ch));
        QApplication::sendEvent(editor, &keyEvent);
    }
}

namespace {

struct AssignmentTarget {
    QString identifier;
    bool isFunction = false;
    bool isUnit = false;
    bool valid = false;
};

int& activeSessionTabDragCount()
{
    static int count = 0;
    return count;
}

QPointer<MainWindow>& primaryMainWindow()
{
    static QPointer<MainWindow> window;
    return window;
}

QList<QPointer<MainWindow>>& allMainWindows()
{
    static QList<QPointer<MainWindow>> windows;
    return windows;
}

QHash<QString, QPointer<MainWindow>>& windowIds()
{
    static QHash<QString, QPointer<MainWindow>> ids;
    return ids;
}

QPointer<ResultDisplay>& globallyActiveDisplay()
{
    static QPointer<ResultDisplay> display;
    return display;
}

QPointer<Editor>& globallyActiveEditor()
{
    static QPointer<Editor> editor;
    return editor;
}

bool& activeEditorDisplayPaneActivationInProgress()
{
    static bool inProgress = false;
    return inProgress;
}

int& dockTextInputFocusTransferDepth()
{
    static int depth = 0;
    return depth;
}

bool dockTextInputFocusTransferInProgress()
{
    return dockTextInputFocusTransferDepth() > 0;
}

QPointer<QWidget>& pendingDockTextInputFocusTarget()
{
    static QPointer<QWidget> target;
    return target;
}

int& pendingDockTextInputFocusTargetGeneration()
{
    static int generation = 0;
    return generation;
}

int setPendingDockTextInputFocusTarget(QWidget* target)
{
    pendingDockTextInputFocusTarget() = target;
    return ++pendingDockTextInputFocusTargetGeneration();
}

QPointer<QWidget>& pendingDockFocusTarget()
{
    static QPointer<QWidget> target;
    return target;
}

int& pendingDockFocusTargetGeneration()
{
    static int generation = 0;
    return generation;
}

int setPendingDockFocusTarget(QWidget* target)
{
    pendingDockFocusTarget() = target;
    return ++pendingDockFocusTargetGeneration();
}

QPointer<ResultDisplay>& pendingSessionTabActivationDisplay()
{
    static QPointer<ResultDisplay> display;
    return display;
}

QPointer<Editor>& pendingSessionTabActivationEditor()
{
    static QPointer<Editor> editor;
    return editor;
}

QPointer<Editor>& pendingWindowActivationEditor()
{
    static QPointer<Editor> editor;
    return editor;
}

QPointer<Editor>& editorBeforeWindowDeactivate()
{
    static QPointer<Editor> editor;
    return editor;
}

QPointer<QWidget>& focusWidgetBeforeWindowDeactivate()
{
    static QPointer<QWidget> widget;
    return widget;
}

QPointer<QWidget>& lastFocusWidgetInActiveWindow()
{
    static QPointer<QWidget> widget;
    return widget;
}

QPointer<QWidget>& pendingWindowActivationFocusWidget()
{
    static QPointer<QWidget> widget;
    return widget;
}

int& windowActivationRestoreGeneration()
{
    static int generation = 0;
    return generation;
}

void cancelWindowActivationRestore()
{
    pendingWindowActivationEditor() = nullptr;
    pendingWindowActivationFocusWidget() = nullptr;
    ++windowActivationRestoreGeneration();
}

MainWindow* activeMainWindowForMenuAction(MainWindow* fallback)
{
    const auto windowForWidget = [](QWidget* widget) -> MainWindow* {
        if (widget == nullptr)
            return nullptr;
        if (auto* window = qobject_cast<MainWindow*>(widget->window()))
            return window;
        // Floating docks are windows too, but retain their MainWindow parent.
        for (QWidget* ancestor = widget; ancestor != nullptr; ancestor = ancestor->parentWidget()) {
            if (auto* dock = qobject_cast<QDockWidget*>(ancestor)) {
                for (QWidget* owner = dock->parentWidget(); owner != nullptr; owner = owner->parentWidget()) {
                    if (auto* window = qobject_cast<MainWindow*>(owner))
                        return window;
                }
            }
        }
        return nullptr;
    };
    MainWindow* targetWindow = windowForWidget(QApplication::activeWindow());
    if (targetWindow == nullptr)
        targetWindow = windowForWidget(QApplication::focusWidget());
    if (targetWindow == nullptr)
        targetWindow = windowForWidget(lastFocusWidgetInActiveWindow());
    return targetWindow != nullptr ? targetWindow : fallback;
}

class DockTextInputFocusTransferGuard {
public:
    DockTextInputFocusTransferGuard()
    {
        ++dockTextInputFocusTransferDepth();
    }

    ~DockTextInputFocusTransferGuard()
    {
        --dockTextInputFocusTransferDepth();
    }
};

bool& appShutdownInProgress()
{
    static bool shuttingDown = false;
    return shuttingDown;
}

bool applicationShutdownInProgress()
{
    // MainWindow close events can be triggered by more than one shutdown path:
    // the primary window, QApplication::quit(), and the single-instance startup
    // handshake in main.cpp. Keep this helper in sync with main.cpp's dynamic
    // property so child windows can tell a real app shutdown from an ordinary
    // child-window close.
    return appShutdownInProgress()
        || (qApp && qApp->property("speedcrunchShutdownInProgress").toBool());
}


QList<QPointer<QWidget>>& panesPendingSessionTabDragDeletion()
{
    static QList<QPointer<QWidget>> panes;
    return panes;
}

void flushPanesPendingSessionTabDragDeletion()
{
    if (activeSessionTabDragCount() > 0)
        return;

    QList<QPointer<QWidget>> panes = panesPendingSessionTabDragDeletion();
    panesPendingSessionTabDragDeletion().clear();
    for (const QPointer<QWidget>& pane : panes) {
        if (pane != nullptr)
            pane->deleteLater();
    }
}

void deletePaneAfterSessionTabDrag(QWidget* pane)
{
    if (pane == nullptr)
        return;

    pane->hide();
    pane->setParent(nullptr);
    if (activeSessionTabDragCount() > 0) {
        panesPendingSessionTabDragDeletion().append(QPointer<QWidget>(pane));
        return;
    }

    pane->deleteLater();
}

class SessionTabDragGuard {
public:
    SessionTabDragGuard()
    {
        ++activeSessionTabDragCount();
    }

    ~SessionTabDragGuard()
    {
        --activeSessionTabDragCount();
        flushPanesPendingSessionTabDragDeletion();
    }
};

class SessionTabBar : public QTabBar {
public:
    explicit SessionTabBar(QWidget* parent = nullptr)
        : QTabBar(parent)
    {
        setAcceptDrops(true);
        setDrawBase(false);
        setElideMode(Qt::ElideRight);
        setExpanding(true);
        setMouseTracking(true);
        setMovable(true);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setUsesScrollButtons(false);
        setContextMenuPolicy(Qt::DefaultContextMenu);
        applyStyle(QColor());
    }

    static QColor secondarySurfaceColor(const QColor& surface)
    {
        return surface.lightnessF() < 0.5 ? surface.lighter(118) : surface.darker(108);
    }

    void applyStyle(const QColor& selectedText,
                    const QColor& selectedSurface = QColor(),
                    const QColor& hoverText = QColor(),
                    const QColor& hoverSurface = QColor(),
                    const QColor& stripSurface = QColor(),
                    const QColor& inactiveText = QColor(),
                    const QColor& closeButtonHoverText = QColor(),
                    const QColor& closeButtonHoverSurface = QColor(),
                    const QColor& toolTipBackground = QColor(),
                    const QColor& toolTipForeground = QColor(),
                    const QColor& toolTipOutline = QColor(),
                    int toolTipCornerRadius = 0)
    {
        StartupDiagnostics::AppearanceScope timing("Style session tab bar", this);
        const QColor fg = selectedText.isValid()
            ? selectedText
            : palette().color(QPalette::WindowText);
        const QPalette pal = palette();
        const QColor selected = selectedSurface.isValid() ? selectedSurface : pal.color(QPalette::Window);
        const QColor hovered = hoverSurface.isValid()
            ? hoverSurface
            : secondarySurfaceColor(selected);
        const QColor strip = stripSurface.isValid() ? stripSurface : selected;
        const QColor text = inactiveText.isValid()
            ? inactiveText
            : pal.color(QPalette::WindowText);
        const QColor hoveredText = hoverText.isValid()
            ? hoverText
            : text;
        const QColor closeButtonHovered = closeButtonHoverSurface.isValid()
            ? closeButtonHoverSurface
            : hovered;
        const QColor closeButtonHoveredText = closeButtonHoverText.isValid()
            ? closeButtonHoverText
            : hoveredText;
        m_tabStripColor = strip;
        m_inactiveTabColor = strip;
        m_hoveredTabColor = hovered;
        m_selectedTabColor = selected;
        m_tabTextColor = text;
        m_hoveredTextColor = hoveredText;
        m_selectedTextColor = fg;
        m_closeButtonHoverColor = closeButtonHovered;
        m_closeButtonHoverTextColor = closeButtonHoveredText;
        m_toolTipBackgroundColor = toolTipBackground;
        m_toolTipForegroundColor = toolTipForeground;
        m_toolTipOutlineColor = toolTipOutline;
        m_toolTipCornerRadius = qMax(0, toolTipCornerRadius);
        applyToolTipTheme();

        setStyleSheetIfChanged(this, QStringLiteral(R"(
                QToolButton {
                    background: transparent;
                    border: none;
                    padding: 0px;
                    margin: 0px;
                }

                QToolButton:hover,
                QToolButton:pressed {
                    background: transparent;
                    border: none;
                }
            )"));
        if (QWidget* row = parentWidget()) {
            QPalette rowPalette = row->palette();
            for (const QPalette::ColorGroup group : {QPalette::Active,
                                                     QPalette::Inactive,
                                                     QPalette::Disabled}) {
                rowPalette.setColor(group, QPalette::Window, m_tabStripColor);
                rowPalette.setColor(group, QPalette::Base, m_tabStripColor);
            }
            StartupDiagnostics::setAppearancePalette(row, rowPalette);
            row->setAutoFillBackground(true);
            row->setAttribute(Qt::WA_StyledBackground, true);
            setStyleSheetIfChanged(row, QStringLiteral("background-color: %1;")
                                            .arg(m_tabStripColor.name()));
        }
        QPalette tabPalette = palette();
        for (const QPalette::ColorGroup group : {QPalette::Active,
                                                 QPalette::Inactive,
                                                 QPalette::Disabled}) {
            tabPalette.setColor(group, QPalette::Window, m_tabStripColor);
            tabPalette.setColor(group, QPalette::Base, m_tabStripColor);
        }
        StartupDiagnostics::setAppearancePalette(this, tabPalette);
        setAutoFillBackground(true);
        updateGeometry();
        update();
    }

    std::function<void(const QString&, const QPoint&)> tabContextMenuRequested;
    std::function<void(SessionTabBar*, const QString&, int)> sessionTabDropped;
    std::function<MainWindow*()> sourceMainWindow;
    std::function<void(const QString&, const QPoint&)> sessionTabDetached;
    std::function<void(const QString&)> tabCloseRequested;
    std::function<void(const QString&)> tabActivated;

    void setActivePaneSelectedTabIndicatorColor(const QColor& color)
    {
        const QColor activeColor = color.isValid() ? color : QColor();
        if (m_activePaneSelectedTabIndicatorColor == activeColor)
            return;
        m_activePaneSelectedTabIndicatorColor = activeColor;
        refreshCloseButtons();
        update();
    }

    void refreshCloseButtons()
    {
        bool visibilityChanged = false;
        for (int i = 0; i < count(); ++i) {
            const bool showButton = !m_visualDragActive
                && (i == currentIndex() || i == m_hoveredTabIndex);
            QWidget* existingButton = tabButton(i, QTabBar::RightSide);
            QToolButton* closeButton = qobject_cast<QToolButton*>(existingButton);
            if (closeButton == nullptr) {
                closeButton = new QToolButton(this);
                closeButton->setAutoRaise(false);
                closeButton->setCursor(Qt::PointingHandCursor);
                closeButton->setFocusPolicy(Qt::NoFocus);
                closeButton->setObjectName(QStringLiteral("SessionTabCloseButton"));
                closeButton->setMouseTracking(true);
                closeButton->setAttribute(Qt::WA_Hover, true);
                closeButton->setText(QStringLiteral("×"));
                closeButton->setStyleSheet(QStringLiteral(R"(
                    QToolButton {
                        background: transparent;
                        border: none;
                        margin: 3px 3px 3px 0px;
                        padding: 0px;
                        min-width: 18px;
                        max-width: 18px;
                        min-height: 18px;
                        max-height: 18px;
                        border-radius: 9px;
                        font-weight: 400;
                        text-align: center;
                    }

                    QToolButton:hover {
                        background: rgba(127, 127, 127, 80);
                    }

                    QToolButton:pressed {
                        background: rgba(127, 127, 127, 192);
                    }
                )"));
                closeButton->setToolTip(QString());
                closeButton->installEventFilter(this);
                QFont closeFont = closeButton->font();
                closeFont.setBold(false);
                closeFont.setPixelSize(qMax(11, fontMetrics().height() - 5));
                closeButton->setFont(closeFont);
                const int buttonExtent = qMax(16, fontMetrics().height() + 1);
                closeButton->setFixedSize(buttonExtent, buttonExtent);
                setTabButton(i, QTabBar::RightSide, closeButton);
                connect(closeButton, &QToolButton::clicked, this, [this, closeButton]() {
                    int tabIndex = -1;
                    for (int i = 0; i < count(); ++i) {
                        if (tabButton(i, QTabBar::RightSide) == closeButton) {
                            tabIndex = i;
                            break;
                        }
                    }
                    if (tabIndex >= 0 && tabCloseRequested)
                        tabCloseRequested(tabText(tabIndex));
                });
            }
            const bool selected = i == currentIndex();
            const bool hovered = i == m_hoveredTabIndex;
            const QColor normalText = selected ? m_selectedTextColor : (hovered ? m_hoveredTextColor : m_tabTextColor);
            const QColor hoveredText = m_closeButtonHoverTextColor.isValid()
                ? m_closeButtonHoverTextColor
                : normalText;
            closeButton->setStyleSheet(QStringLiteral(R"(
                    QToolButton {
                        background: transparent;
                        color: %1;
                        border: none;
                        margin: 3px 3px 3px 0px;
                        padding: 0px;
                        min-width: 18px;
                        max-width: 18px;
                        min-height: 18px;
                        max-height: 18px;
                        border-radius: 9px;
                        font-weight: 400;
                        text-align: center;
                    }

                    QToolButton:hover {
                        background: %2;
                        color: %3;
                    }

                    QToolButton:pressed {
                        background: %2;
                        color: %3;
                    }
                )")
                                           .arg(normalText.name(),
                                                m_closeButtonHoverColor.name(),
                                                hoveredText.name()));
            visibilityChanged = visibilityChanged || closeButton->isVisible() != showButton;
            closeButton->setVisible(showButton);
        }
        if (visibilityChanged)
            update();
    }

protected:
    void contextMenuEvent(QContextMenuEvent* event) override
    {
        const int index = tabAt(event->pos());
        if (index >= 0) {
            setCurrentIndex(index);
            if (tabContextMenuRequested)
                tabContextMenuRequested(tabText(index), event->globalPos());
            event->accept();
            return;
        }

        QTabBar::contextMenuEvent(event);
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        hideTabToolTip();
        if (event->button() == Qt::LeftButton) {
            m_dragTabIndex = tabAt(event->pos());
            m_dragSessionName = m_dragTabIndex >= 0 ? tabText(m_dragTabIndex) : QString();
            m_dragTabPressOffsetX = m_dragTabIndex >= 0
                ? event->pos().x() - tabRect(m_dragTabIndex).left()
                : 0;
        }
        const QString pressedSession = m_dragSessionName;

        if (event->button() == Qt::LeftButton && !pressedSession.isEmpty()) {
            if (currentIndex() != m_dragTabIndex)
                setCurrentIndex(m_dragTabIndex);
            else if (tabActivated)
                tabActivated(pressedSession);
            refreshCloseButtons();
            event->accept();
            return;
        }
        QTabBar::mousePressEvent(event);
        refreshCloseButtons();
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        updateHoveredTab(event->pos());
        showTabToolTip(event->pos(), event->globalPosition().toPoint());

        const bool hasPressedTab = (event->buttons() & Qt::LeftButton)
            && m_dragTabIndex >= 0;
        if (!hasPressedTab) {
            QTabBar::mouseMoveEvent(event);
            refreshCloseButtons();
            return;
        }

        if (!shouldStartCrossBarDrag(event->pos(), event->globalPosition().toPoint())) {
            updateDraggedTabVisual(event->pos());
            event->accept();
            refreshCloseButtons();
            return;
        }

        m_visualDragActive = false;
        m_visualTargetIndex = -1;
        refreshCloseButtons();
        repaint();

        QMimeData* mime = new QMimeData();
        QJsonObject payload;
        payload.insert(QStringLiteral("session"), m_dragSessionName);
        if (sourceMainWindow) {
            if (MainWindow* window = sourceMainWindow())
                payload.insert(QStringLiteral("windowId"), window->objectName());
        }
        mime->setData(QStringLiteral("application/x-speedcrunch-session-tab"),
                      QJsonDocument(payload).toJson(QJsonDocument::Compact));

        QDrag* drag = new QDrag(this);
        drag->setMimeData(mime);
        const int currentDragIndex = qMax(0, indexOfDragSession());
        drag->setPixmap(grab(tabRect(currentDragIndex)));
        const QPoint clampedPos = clampTabDragPos(event->pos());
        drag->setHotSpot(clampedPos - tabRect(currentDragIndex).topLeft());
        QPointer<SessionTabBar> self(this);
        SessionTabDragGuard dragGuard;
        const Qt::DropAction dropAction = drag->exec(Qt::MoveAction);
        if (self == nullptr)
            return;
        if (dropAction != Qt::MoveAction && sessionTabDetached && !m_dragSessionName.isEmpty())
            sessionTabDetached(m_dragSessionName, QCursor::pos());
        m_dragTabIndex = -1;
        m_dragSessionName.clear();
        m_dropIndicatorIndex = -1;
        update();
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && m_visualDragActive) {
            commitVisualTabDrag();
            m_dragTabIndex = -1;
            m_dragSessionName.clear();
            m_visualDragActive = false;
            m_visualTargetIndex = -1;
            refreshCloseButtons();
            update();
            event->accept();
            return;
        }

        QTabBar::mouseReleaseEvent(event);
        if (event->button() == Qt::LeftButton) {
            m_dragTabIndex = -1;
            m_dragSessionName.clear();
            m_visualDragActive = false;
            m_visualTargetIndex = -1;
            refreshCloseButtons();
            update();
        }
    }

    void dragEnterEvent(QDragEnterEvent* event) override
    {
        if (event->mimeData()->hasFormat(QStringLiteral("application/x-speedcrunch-session-tab"))) {
            updateDropIndicator(event->position().toPoint());
            event->acceptProposedAction();
        }
    }

    void dragMoveEvent(QDragMoveEvent* event) override
    {
        if (event->mimeData()->hasFormat(QStringLiteral("application/x-speedcrunch-session-tab"))) {
            updateDropIndicator(event->position().toPoint());
            event->acceptProposedAction();
        }
    }

    void dragLeaveEvent(QDragLeaveEvent* event) override
    {
        m_dropIndicatorIndex = -1;
        update();
        QTabBar::dragLeaveEvent(event);
    }

    void dropEvent(QDropEvent* event) override
    {
        const QMimeData* mime = event->mimeData();
        if (!mime->hasFormat(QStringLiteral("application/x-speedcrunch-session-tab")))
            return;

        QString sessionName;
        const QByteArray payload = mime->data(QStringLiteral("application/x-speedcrunch-session-tab"));
        const QJsonDocument payloadDoc = QJsonDocument::fromJson(payload);
        if (payloadDoc.isObject())
            sessionName = payloadDoc.object().value(QStringLiteral("session")).toString();
        if (sessionName.isEmpty())
            sessionName = QString::fromUtf8(payload);
        SessionTabBar* sourceTabBar = dynamic_cast<SessionTabBar*>(event->source());
        int targetIndex = tabAt(event->position().toPoint());
        if (targetIndex < 0)
            targetIndex = count();

        if (sessionTabDropped)
            sessionTabDropped(sourceTabBar, sessionName, targetIndex);
        m_dropIndicatorIndex = -1;
        update();
        event->acceptProposedAction();
    }

    void leaveEvent(QEvent* event) override
    {
        m_hoveredTabIndex = -1;
        hideTabToolTip();
        setCursor(Qt::ArrowCursor);
        refreshCloseButtons();
        update();
        QTabBar::leaveEvent(event);
    }

    bool event(QEvent* event) override
    {
        if (event->type() == QEvent::HoverMove) {
            QHoverEvent* hoverEvent = static_cast<QHoverEvent*>(event);
            const QPoint pos = hoverEvent->position().toPoint();
            updateHoveredTab(pos);
            showTabToolTip(pos, mapToGlobal(pos));
        } else if (event->type() == QEvent::ToolTip) {
            QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);
            showTabToolTip(helpEvent->pos(), helpEvent->globalPos());
            return true;
        }
        return QTabBar::event(event);
    }

    void paintEvent(QPaintEvent* event) override
    {
        QPainter tabPainter(this);
        tabPainter.setRenderHint(QPainter::Antialiasing, true);
        tabPainter.fillRect(event->rect(), m_tabStripColor);

        const int visualDragIndex = m_visualDragActive ? indexOfDragSession() : -1;
        const int draggedWidth = visualDragIndex >= 0 ? tabRect(visualDragIndex).width() : 0;
        QVector<QRect> paintedRects(count());
        for (int i = 0; i < count(); ++i) {
            if (i == visualDragIndex)
                continue;

            QRect rect = tabRect(i);
            if (m_visualDragActive && m_visualTargetIndex >= 0) {
                if (m_visualTargetIndex > visualDragIndex && i > visualDragIndex && i <= m_visualTargetIndex)
                    rect.translate(-draggedWidth, 0);
                else if (m_visualTargetIndex < visualDragIndex && i >= m_visualTargetIndex && i < visualDragIndex)
                    rect.translate(draggedWidth, 0);
            }
            paintedRects[i] = rect;
            paintTab(&tabPainter, i, rect, false);
        }

        for (int i = 0; i + 1 < count(); ++i) {
            if (i == visualDragIndex || i + 1 == visualDragIndex)
                continue;
            if (i == currentIndex() || i + 1 == currentIndex())
                continue;
            if (i == m_hoveredTabIndex || i + 1 == m_hoveredTabIndex)
                continue;
            if (paintedRects[i].isEmpty() || paintedRects[i + 1].isEmpty())
                continue;

            const int x = (paintedRects[i].right() + paintedRects[i + 1].left()) / 2;
            const int top = pillRect(paintedRects[i]).top() + 5;
            const int bottom = pillRect(paintedRects[i]).bottom() - 5;
            tabPainter.setPen(QPen(m_hoveredTabColor, 1));
            tabPainter.drawLine(QPoint(x, top), QPoint(x, bottom));
        }

        if (visualDragIndex >= 0) {
            QRect draggedRect = tabRect(visualDragIndex);
            draggedRect.moveLeft(qRound(m_visualDragLeft));
            paintTab(&tabPainter, visualDragIndex, draggedRect, true);
        }

        if (m_dropIndicatorIndex < 0)
            return;

        int x = width() - 1;
        if (m_dropIndicatorIndex < count())
            x = tabRect(m_dropIndicatorIndex).left();
        else if (count() > 0)
            x = tabRect(count() - 1).right() + 1;

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(palette().highlight().color(), 2));
        painter.drawLine(QPoint(x, 4), QPoint(x, height() - 4));
    }

    QSize tabSizeHint(int index) const override
    {
        const int closeExtent = qMax(16, fontMetrics().height() + 1);
        const int closeWidth = closeExtent + 8;
        const int width = fontMetrics().horizontalAdvance(tabText(index))
            + UiConfig::SessionTabHorizontalPadding + closeWidth + (tabIcon(index).isNull() ? 0 : 22);
        const int height = qMax(30, fontMetrics().height() + 12);
        return QSize(width, height);
    }

private:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        QWidget* watchedWidget = qobject_cast<QWidget*>(watched);
        if (watchedWidget != nullptr
            && watchedWidget->objectName() == QLatin1String("SessionTabCloseButton")) {
            switch (event->type()) {
            case QEvent::Enter:
            case QEvent::HoverEnter:
            case QEvent::MouseMove:
            case QEvent::HoverMove:
                showCloseButtonToolTip(QCursor::pos());
                break;
            case QEvent::Leave:
            case QEvent::HoverLeave:
                hideTabToolTip();
                break;
            case QEvent::ToolTip: {
                const QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);
                showCloseButtonToolTip(helpEvent->globalPos());
                return true;
            }
            default:
                break;
            }
        }
        return QTabBar::eventFilter(watched, event);
    }

    void applyToolTipTheme()
    {
        ToolTipStyleUtils::applyPopupTheme(
            m_toolTipPopup,
            m_toolTipPopupLabel,
            this,
            {m_toolTipBackgroundColor,
             m_toolTipForegroundColor,
             m_toolTipOutlineColor,
             m_toolTipCornerRadius});
    }

    void ensureToolTipPopup()
    {
        if (m_toolTipPopup != nullptr)
            return;

        m_toolTipPopup = ToolTipStyleUtils::createPopup(
            this,
            QStringLiteral("sessionTabToolTipPopup"),
            QStringLiteral("sessionTabToolTipPopupLabel"),
            Qt::PlainText,
            &m_toolTipPopupLabel,
            true);
        applyToolTipTheme();
    }

    void hideTabToolTip()
    {
        if (m_toolTipPopup != nullptr)
            m_toolTipPopup->hide();
    }

    void showTabToolTip(const QPoint& pos, const QPoint& globalPos)
    {
        const int index = tabAt(pos);
        if (index < 0) {
            hideTabToolTip();
            return;
        }

        QWidget* closeButton = tabButton(index, QTabBar::RightSide);
        if (closeButton != nullptr
            && closeButton->rect().contains(closeButton->mapFromGlobal(globalPos))) {
            hideTabToolTip();
            return;
        }

        const QString text = tabToolTip(index).isEmpty() ? tabText(index) : tabToolTip(index);
        if (tabIcon(index).isNull() && fontMetrics().horizontalAdvance(text)
            <= tabTextRect(index, tabRect(index)).width()) {
            hideTabToolTip();
            return;
        }

        ensureToolTipPopup();
        ToolTipStyleUtils::showPopup(m_toolTipPopup,
                                     m_toolTipPopupLabel,
                                     text,
                                     this,
                                     globalPos,
                                     m_toolTipCornerRadius);
    }

    void showCloseButtonToolTip(const QPoint& globalPos)
    {
        ensureToolTipPopup();
        ToolTipStyleUtils::showPopup(m_toolTipPopup,
                                     m_toolTipPopupLabel,
                                     MainWindow::tr("Close Session"),
                                     this,
                                     globalPos,
                                     m_toolTipCornerRadius);
    }

    QRect tabTextRect(int index, const QRect& rect) const
    {
        const int rightButtonWidth = tabButton(index, QTabBar::RightSide) != nullptr
            && tabButton(index, QTabBar::RightSide)->isVisible()
            ? tabButton(index, QTabBar::RightSide)->width() + 6
            : 0;
        return pillRect(rect).adjusted(12 + (tabIcon(index).isNull() ? 0 : 22), 0, -12 - rightButtonWidth, 0);
    }

    void updateHoveredTab(const QPoint& pos)
    {
        const int hoveredTabIndex = tabAt(pos);
        if (m_hoveredTabIndex != hoveredTabIndex) {
            m_hoveredTabIndex = hoveredTabIndex;
            refreshCloseButtons();
            update();
        }
        setCursor(hoveredTabIndex >= 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
    }

    QRect pillRect(const QRect& tabRect) const
    {
        return tabRect.adjusted(2, 3, -2, 0);
    }

    QPainterPath topRoundedTabPath(const QRect& rect) const
    {
        const QRectF tab(rect);
        const qreal radius = qMin(qreal(UiConfig::ControlCornerRadius),
                                 qMin(tab.width() / 2.0, tab.height() / 2.0));
        QPainterPath path;
        path.moveTo(tab.left(), tab.bottom());
        path.lineTo(tab.left(), tab.top() + radius);
        path.quadTo(tab.left(), tab.top(), tab.left() + radius, tab.top());
        path.lineTo(tab.right() - radius, tab.top());
        path.quadTo(tab.right(), tab.top(), tab.right(), tab.top() + radius);
        path.lineTo(tab.right(), tab.bottom());
        path.closeSubpath();
        return path;
    }

    void paintTab(QPainter* painter, int index, const QRect& rect, bool dragged) const
    {
        if (index < 0 || index >= count() || rect.isEmpty())
            return;

        const bool selected = index == currentIndex();
        const bool hovered = index == m_hoveredTabIndex && !m_visualDragActive;
        const QRect pill = pillRect(rect);
        if (selected || hovered || dragged) {
            const QColor fill = selected
                ? m_selectedTabColor
                : (hovered || dragged ? m_hoveredTabColor : m_inactiveTabColor);
            painter->setPen(Qt::NoPen);
            painter->setBrush(fill);
            painter->drawPath(topRoundedTabPath(pill));
        }
        if (selected && m_activePaneSelectedTabIndicatorColor.isValid()) {
            const int strokeWidth = UiConfig::ActiveSessionTabIndicatorStrokeWidth;
            const qreal y = pill.bottom() - (strokeWidth - 1) / 2.0;
            painter->setPen(QPen(m_activePaneSelectedTabIndicatorColor,
                                 strokeWidth,
                                 Qt::SolidLine,
                                 Qt::SquareCap));
            painter->setBrush(Qt::NoBrush);
            painter->drawLine(QPointF(pill.left(), y), QPointF(pill.right(), y));
        }

        const QRect textRect = tabTextRect(index, rect);
        if (!tabIcon(index).isNull()) {
            const QRect iconRect(textRect.left() - 22, textRect.center().y() - 8, 16, 16);
            tabIcon(index).paint(painter, iconRect);
        }
        painter->setPen(selected ? m_selectedTextColor : (hovered ? m_hoveredTextColor : m_tabTextColor));
        painter->setFont(font());
        painter->drawText(textRect,
                          Qt::AlignVCenter | Qt::AlignLeft,
                          fontMetrics().elidedText(tabText(index), elideMode(), textRect.width()));
    }

    void updateDropIndicator(const QPoint& pos)
    {
        int index = tabAt(clampDropPos(pos));
        if (index < 0) {
            if (count() > 0 && pos.x() < tabRect(0).left())
                index = 0;
            else
                index = count();
        }
        if (index < 0)
            index = count();
        else if (clampDropPos(pos).x() > tabRect(index).center().x())
            ++index;

        if (m_dropIndicatorIndex != index) {
            m_dropIndicatorIndex = index;
            update();
        }
    }

    bool shouldStartCrossBarDrag(const QPoint& pos, const QPoint& globalPos) const
    {
        if (count() == 0)
            return true;

        const int detachDistance = QApplication::startDragDistance();
        if (pos.y() < -detachDistance || pos.y() >= height() + detachDistance)
            return true;

        QWidget* topLevel = window();
        return topLevel != nullptr && !topLevel->frameGeometry().contains(globalPos);
    }

    QPoint clampTabDragPos(const QPoint& pos) const
    {
        if (count() == 0)
            return pos;

        return QPoint(qBound(tabRect(0).left(), pos.x(), tabRect(count() - 1).right()),
                      qBound(0, pos.y(), height() - 1));
    }

    QPoint clampDropPos(const QPoint& pos) const
    {
        if (count() == 0)
            return pos;

        return QPoint(qBound(tabRect(0).left(), pos.x(), tabRect(count() - 1).right()),
                      qBound(0, pos.y(), height() - 1));
    }

    int visualTargetIndexForDrag(qreal visualLeft, int draggedWidth) const
    {
        const int sourceIndex = indexOfDragSession();
        if (sourceIndex < 0 || sourceIndex >= count())
            return -1;

        int targetIndex = sourceIndex;
        if (visualLeft < tabRect(sourceIndex).left()) {
            while (targetIndex > 0 && visualLeft < tabRect(targetIndex - 1).center().x())
                --targetIndex;
        } else {
            const qreal visualRight = visualLeft + draggedWidth;
            while (targetIndex + 1 < count() && visualRight > tabRect(targetIndex + 1).center().x())
                ++targetIndex;
        }
        return qBound(0, targetIndex, count() - 1);
    }

    void updateDraggedTabVisual(const QPoint& pos)
    {
        const int index = indexOfDragSession();
        if (index < 0 || index >= count())
            return;

        const int leftLimit = count() > 0 ? tabRect(0).left() : 0;
        const int rightLimit = count() > 0 ? tabRect(count() - 1).right() - tabRect(index).width() + 1 : width();
        const int clampedRightLimit = qMax(leftLimit, rightLimit);
        m_visualDragActive = true;
        m_visualDragLeft = qBound(leftLimit, pos.x() - m_dragTabPressOffsetX, clampedRightLimit);
        m_visualTargetIndex = visualTargetIndexForDrag(m_visualDragLeft, tabRect(index).width());
        update();
    }

    void commitVisualTabDrag()
    {
        int index = indexOfDragSession();
        if (index < 0 || index >= count())
            return;
        if (m_visualTargetIndex < 0 || m_visualTargetIndex == index)
            return;

        moveTab(index, m_visualTargetIndex);
        m_dragTabIndex = m_visualTargetIndex;
        setCurrentIndex(m_visualTargetIndex);
    }

    int m_dragTabPressOffsetX = 0;
    int m_dragTabIndex = -1;
    int m_hoveredTabIndex = -1;
    int m_dropIndicatorIndex = -1;
    bool m_visualDragActive = false;
    qreal m_visualDragLeft = 0.0;
    int m_visualTargetIndex = -1;
    QColor m_tabStripColor;
    QColor m_inactiveTabColor;
    QColor m_hoveredTabColor;
    QColor m_selectedTabColor;
    QColor m_activePaneSelectedTabIndicatorColor;
    QColor m_tabTextColor;
    QColor m_hoveredTextColor;
    QColor m_selectedTextColor;
    QColor m_closeButtonHoverColor;
    QColor m_closeButtonHoverTextColor;
    QColor m_toolTipBackgroundColor;
    QColor m_toolTipForegroundColor;
    QColor m_toolTipOutlineColor;
    int m_toolTipCornerRadius = 0;
    QFrame* m_toolTipPopup = nullptr;
    QLabel* m_toolTipPopupLabel = nullptr;
    QString m_dragSessionName;

    int indexOfDragSession() const
    {
        for (int i = 0; i < count(); ++i) {
            if (tabText(i) == m_dragSessionName)
                return i;
        }
        return m_dragTabIndex;
    }
};

enum class PaneDropZone {
    Center,
    Top,
    Bottom,
    Left,
    Right
};

class SessionPane : public QWidget {
public:
    explicit SessionPane(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setObjectName(QStringLiteral("SessionPane"));
        setAcceptDrops(true);
        m_overlay = new QWidget(this);
        m_overlay->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_overlay->setStyleSheet(QStringLiteral("background: rgba(0, 0, 0, 80);"));
        m_overlay->hide();
    }

    std::function<void(SessionTabBar*, const QString&, const QPoint&)> sessionTabDroppedOnPane;
    std::function<bool(SessionTabBar*)> shouldShowOverlayForDrag;
    std::function<void()> paneActivated;
    void setOverlayAreaWidget(QWidget* widget)
    {
        m_overlayAreaWidget = widget;
    }

    PaneDropZone dropZoneForPanePosition(const QPoint& panePos) const
    {
        return dropZoneForPosition(panePos);
    }

    void watchDropTarget(QWidget* widget)
    {
        if (widget == nullptr)
            return;
        widget->installEventFilter(this);
        widget->setAcceptDrops(true);
    }

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (paneActivated)
            paneActivated();
        QWidget::mousePressEvent(event);
    }

    void dragEnterEvent(QDragEnterEvent* event) override
    {
        if (acceptSessionTabDrag(event))
            return;
        QWidget::dragEnterEvent(event);
    }

    void dragMoveEvent(QDragMoveEvent* event) override
    {
        if (acceptSessionTabDrag(event, event->position().toPoint()))
            return;
        QWidget::dragMoveEvent(event);
    }

    void dropEvent(QDropEvent* event) override
    {
        if (handleSessionTabDrop(event, event->position().toPoint()))
            return;
        QWidget::dropEvent(event);
    }

    void dragLeaveEvent(QDragLeaveEvent* event) override
    {
        hideOverlay();
        QWidget::dragLeaveEvent(event);
    }

    void resizeEvent(QResizeEvent* event) override
    {
        if (m_overlay != nullptr)
            m_overlay->setGeometry(overlayRect(PaneDropZone::Center));
        QWidget::resizeEvent(event);
    }

    bool eventFilter(QObject* watched, QEvent* event) override
    {
        QWidget* watchedWidget = qobject_cast<QWidget*>(watched);
        if (watchedWidget == nullptr)
            return QWidget::eventFilter(watched, event);

        if (event->type() == QEvent::DragEnter) {
            QDragEnterEvent* dragEvent = static_cast<QDragEnterEvent*>(event);
            return acceptSessionTabDrag(dragEvent);
        }
        if (event->type() == QEvent::DragMove) {
            QDragMoveEvent* dragEvent = static_cast<QDragMoveEvent*>(event);
            return acceptSessionTabDrag(dragEvent, watchedWidget->mapTo(this, dragEvent->position().toPoint()));
        }
        if (event->type() == QEvent::Drop) {
            QDropEvent* dropEvent = static_cast<QDropEvent*>(event);
            return handleSessionTabDrop(dropEvent, watchedWidget->mapTo(this, dropEvent->position().toPoint()));
        }
        if (event->type() == QEvent::DragLeave) {
            hideOverlay();
            return false;
        }
        if (event->type() == QEvent::MouseButtonPress) {
            if (paneActivated)
                paneActivated();
            return false;
        }

        return QWidget::eventFilter(watched, event);
    }

private:
    bool acceptSessionTabDrag(QDragMoveEvent* event, const QPoint& panePos = QPoint())
    {
        if (!event->mimeData()->hasFormat(QStringLiteral("application/x-speedcrunch-session-tab")))
            return false;
        SessionTabBar* sourceTabBar = dynamic_cast<SessionTabBar*>(event->source());
        if (shouldShowOverlayForDrag && !shouldShowOverlayForDrag(sourceTabBar)) {
            hideOverlay();
            event->acceptProposedAction();
            return true;
        }
        updateOverlay(panePos.isNull() ? event->position().toPoint() : panePos);
        event->acceptProposedAction();
        return true;
    }

    bool handleSessionTabDrop(QDropEvent* event, const QPoint& panePos)
    {
        const QMimeData* mime = event->mimeData();
        if (!mime->hasFormat(QStringLiteral("application/x-speedcrunch-session-tab")))
            return false;

        QString sessionName;
        const QByteArray payload = mime->data(QStringLiteral("application/x-speedcrunch-session-tab"));
        const QJsonDocument payloadDoc = QJsonDocument::fromJson(payload);
        if (payloadDoc.isObject())
            sessionName = payloadDoc.object().value(QStringLiteral("session")).toString();
        if (sessionName.isEmpty())
            sessionName = QString::fromUtf8(payload);
        SessionTabBar* sourceTabBar = dynamic_cast<SessionTabBar*>(event->source());
        if (sourceTabBar == nullptr || sessionName.isEmpty())
            return false;

        hideOverlay();
        if (sessionTabDroppedOnPane)
            sessionTabDroppedOnPane(sourceTabBar, sessionName, panePos);
        event->acceptProposedAction();
        return true;
    }

    void updateOverlay(const QPoint& panePos)
    {
        if (m_overlay == nullptr)
            return;

        m_overlay->setGeometry(overlayRect(dropZoneForPosition(panePos)));
        m_overlay->raise();
        m_overlay->show();
    }

    void hideOverlay()
    {
        if (m_overlay != nullptr)
            m_overlay->hide();
    }

    PaneDropZone dropZoneForPosition(const QPoint& panePos) const
    {
        const QPointF pos(panePos);
        const QRect rect = overlayBounds();
        const QPointF center(rect.center());
        const QPointF top(rect.left() + rect.width() / 2.0, rect.top());
        const QPointF bottom(rect.left() + rect.width() / 2.0, rect.bottom());
        const QPointF left(rect.left(), rect.top() + rect.height() / 2.0);
        const QPointF right(rect.right(), rect.top() + rect.height() / 2.0);

        const auto distanceSquared = [&pos](const QPointF& point) {
            const QPointF delta = pos - point;
            return delta.x() * delta.x() + delta.y() * delta.y();
        };

        PaneDropZone zone = PaneDropZone::Center;
        qreal bestDistance = distanceSquared(center);
        const auto consider = [&bestDistance, &zone, &distanceSquared](PaneDropZone candidate, const QPointF& point) {
            const qreal distance = distanceSquared(point);
            if (distance < bestDistance) {
                bestDistance = distance;
                zone = candidate;
            }
        };
        consider(PaneDropZone::Top, top);
        consider(PaneDropZone::Bottom, bottom);
        consider(PaneDropZone::Left, left);
        consider(PaneDropZone::Right, right);
        return zone;
    }

    QRect overlayRect(PaneDropZone zone) const
    {
        const QRect rect = overlayBounds();
        switch (zone) {
        case PaneDropZone::Center:
            return rect;
        case PaneDropZone::Top:
            return QRect(rect.left(), rect.top(), rect.width(), rect.height() / 2);
        case PaneDropZone::Bottom:
            return QRect(rect.left(), rect.top() + rect.height() / 2, rect.width(), rect.height() - rect.height() / 2);
        case PaneDropZone::Left:
            return QRect(rect.left(), rect.top(), rect.width() / 2, rect.height());
        case PaneDropZone::Right:
            return QRect(rect.left() + rect.width() / 2, rect.top(), rect.width() - rect.width() / 2, rect.height());
        }
        return rect;
    }

    QRect overlayBounds() const
    {
        if (m_overlayAreaWidget == nullptr)
            return this->rect();

        const QPoint topLeft = m_overlayAreaWidget->mapTo(this, QPoint(0, 0));
        return QRect(topLeft, m_overlayAreaWidget->size());
    }

    QWidget* m_overlay = nullptr;
    QWidget* m_overlayAreaWidget = nullptr;
};

QWidget* paneWidgetForDisplay(ResultDisplay* display)
{
    QWidget* widget = display;
    while (widget != nullptr && !qobject_cast<QSplitter*>(widget->parentWidget()))
        widget = widget->parentWidget();
    return widget;
}

bool splitAssignmentDescriptionForImport(const QString& expression,
                                         QString* expressionWithoutDescription,
                                         QString* description = nullptr)
{
    const int equalsPos = expression.indexOf(MathDsl::Equals);
    if (equalsPos < 0)
        return false;

    int depth = 0;
    const int n = expression.size();
    for (int i = equalsPos + 1; i < n; ++i) {
        const QChar ch = expression.at(i);
        if (ch == MathDsl::GroupStart) {
            ++depth;
        } else if (ch == MathDsl::GroupEnd && depth > 0) {
            --depth;
        } else if (ch == MathDsl::CommentSep && depth == 0) {
            *expressionWithoutDescription = expression.left(i).trimmed();
            if (description)
                *description = expression.mid(i + 1).trimmed();
            return true;
        }
    }

    return false;
}

AssignmentTarget assignmentTargetFromExpression(Evaluator* evaluator, const QString& expression)
{
    AssignmentTarget target;
    if (!evaluator)
        return target;

    QString expressionToParse = expression;
    splitAssignmentDescriptionForImport(expression, &expressionToParse);
    Tokens tokens = evaluator->scan(expressionToParse);

    if (!tokens.valid())
        return target;

    if (tokens.count() > 2
        && tokens.at(0).isIdentifier()
        && tokens.at(1).asOperator() == Token::Assignment)
    {
        target.identifier = tokens.at(0).text();
        target.valid = true;
        return target;
    }

    if (tokens.count() > 4
        && tokens.at(0).asOperator() == Token::AssociationStart
        && tokens.at(0).text() == QString(MathDsl::UnitStart)
        && tokens.at(1).isUnitIdentifier()
        && tokens.at(2).asOperator() == Token::AssociationEnd
        && tokens.at(2).text() == QString(MathDsl::UnitEnd)
        && tokens.at(3).asOperator() == Token::Assignment)
    {
        target.identifier = tokens.at(1).text();
        target.isUnit = true;
        target.valid = true;
        return target;
    }

    if (tokens.count() > 2
        && tokens.at(0).isIdentifier()
        && tokens.at(1).asOperator() == Token::AssociationStart
        && tokens.at(1).text() == QLatin1String("("))
    {
        bool assignFunc = false;
        int t = 0;

        if (tokens.count() > 4
            && tokens.at(2).asOperator() == Token::AssociationEnd
            && tokens.at(2).text() == QLatin1String(")"))
        {
            t = 3;
            if (tokens.at(3).asOperator() == Token::Assignment)
                assignFunc = true;
        } else {
            for (t = 2; t + 1 < tokens.count(); t += 2)  {
                if (!tokens.at(t).isIdentifier())
                    break;

                if (tokens.at(t + 1).asOperator() == Token::AssociationEnd
                    && tokens.at(t + 1).text() == QLatin1String(")")) {
                    t += 2;
                    if (t < tokens.count()
                        && tokens.at(t).asOperator() == Token::Assignment)
                    {
                        assignFunc = true;
                    }
                    break;
                } else if (tokens.at(t + 1).asOperator() != Token::ListSeparator) {
                    break;
                }
            }
        }

        if (assignFunc) {
            target.identifier = tokens.at(0).text();
            target.isFunction = true;
            target.valid = true;
        }
    }

    return target;
}

bool findUserFunctionByName(const QList<UserFunction>& functions, const QString& name, UserFunction* function)
{
    for (const UserFunction& candidate : functions) {
        if (candidate.name() == name) {
            if (function)
                *function = candidate;
            return true;
        }
    }

    return false;
}

QString normalizedDisplaySelectionForEvaluation(QString selected)
{
    selected.replace(QChar::ParagraphSeparator, QLatin1Char('\n'));

    const QStringList rawLines = selected.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    QStringList candidateLines;
    candidateLines.reserve(rawLines.size());
    for (const QString& rawLine : rawLines) {
        const QString line = rawLine.trimmed();
        if (line.isEmpty())
            continue;
        if (line.startsWith(QLatin1String("=")))
            continue;
        candidateLines.append(line);
    }

    if (!candidateLines.isEmpty())
        return candidateLines.join(QLatin1Char(' '));

    for (const QString& rawLine : rawLines) {
        QString line = rawLine.trimmed();
        if (line.startsWith(QLatin1String("=")))
            line = line.mid(1).trimmed();
        if (!line.isEmpty())
            return line;
    }

    return selected.trimmed();
}

} // namespace

void MainWindow::createUi()
{
    createActions();
    createActionGroups();
    createActionShortcuts();
    createMenus();
    createFixedWidgets();
    createFixedConnections();

    setWindowTitle("SpeedCrunch");
    setWindowIcon(QPixmap(":/speedcrunch.png"));

    m_copyWidget = m_widgets.editor;
    syncEditMenuActionState();
    syncSessionMenuActionState();
}

void MainWindow::createActions()
{
    StartupDiagnostics::Scope timing("Create actions");
    m_actions.sessionExportHtml = new QAction(this);
    m_actions.sessionExportJson = new QAction(this);
    m_actions.sessionExportPlainText = new QAction(this);
    m_actions.sessionImport = new QAction(this);
    m_actions.sessionImportUserDefinitions = new QAction(this);
    m_actions.sessionNewTab = new QAction(this);
    m_actions.sessionNewPrivate = new QAction(this);
    m_actions.sessionConvertPrivacy = new QAction(this);
    m_actions.sessionNewWindow = new QAction(this);
    m_actions.sessionOpen = new QAction(this);
    m_actions.sessionRename = new QAction(this);
    m_actions.sessionClone = new QAction(this);
    m_actions.sessionClose = new QAction(this);
    m_actions.sessionReopen = new QAction(this);
    m_actions.sessionOpenSessionsFolder = new QAction(this);
    m_actions.sessionQuit = new QAction(this);
    m_actions.editUndo = new QAction(this);
    m_actions.editRedo = new QAction(this);
    m_actions.editCut = new QAction(this);
    m_actions.editClearExpression = new QAction(this);
    m_actions.editClearHistory = new QAction(this);
    m_actions.editCopyLastResult = new QAction(this);
    m_actions.editCopy = new QAction(this);
    m_actions.editPaste = new QAction(this);
    m_actions.editSelectExpression = new QAction(this);
    m_actions.editWrapSelection = new QAction(this);
    m_actions.viewConstants = new QAction(this);
    m_actions.viewFullScreenMode = new QAction(this);
    m_actions.viewFunctions = new QAction(this);
    m_actions.viewHistory = new QAction(this);
    m_actions.viewKeypadDisabled = new QAction(this);
    m_actions.viewKeypadBasicWide = new QAction(this);
    m_actions.viewKeypadScientificWide = new QAction(this);
    m_actions.viewKeypadScientificNarrow = new QAction(this);
    m_actions.viewKeypadCustom = new QAction(this);
    m_actions.viewKeypadZoom50 = new QAction(this);
    m_actions.viewKeypadZoom100 = new QAction(this);
    m_actions.viewKeypadZoom150 = new QAction(this);
    m_actions.viewKeypadZoom200 = new QAction(this);
    m_actions.viewFormulaBook = new QAction(this);
    m_actions.viewStatusBar = new QAction(this);
    m_actions.viewMenuBar = new QAction(this);
    m_actions.viewVariables = new QAction(this);
    m_actions.viewBitfield = new QAction(this);
    m_actions.viewSplitLeft = new QAction(this);
    m_actions.viewSplitRight = new QAction(this);
    m_actions.viewSplitUp = new QAction(this);
    m_actions.viewSplitDown = new QAction(this);
    m_actions.viewClosePane = new QAction(this);
    m_actions.viewUserFunctions = new QAction(this);
    m_actions.viewUserUnits = new QAction(this);
    m_actions.settingsAngleUnitDegree = new QAction(this);
    m_actions.settingsAngleUnitRadian = new QAction(this);
    m_actions.settingsAngleUnitGradian = new QAction(this);
    m_actions.settingsAngleUnitTurn = new QAction(this);
    m_actions.settingsAngleUnitRevolution = new QAction(this);
    m_actions.settingsBehaviorAlwaysOnTop = new QAction(this);
    m_actions.settingsOpenPrivateTabOnStartup = new QAction(this);
    m_actions.settingsLaunchActivate = new QAction(this);
    m_actions.settingsLaunchNewWindow = new QAction(this);
    m_actions.settingsLaunchNewTab = new QAction(this);
    m_actions.settingsLaunchActivate->setObjectName(QStringLiteral("SubsequentLaunchActivateAction"));
    m_actions.settingsLaunchNewWindow->setObjectName(QStringLiteral("SubsequentLaunchNewWindowAction"));
    m_actions.settingsLaunchNewTab->setObjectName(QStringLiteral("SubsequentLaunchNewTabAction"));
    for (QAction* action : {m_actions.settingsLaunchActivate, m_actions.settingsLaunchNewWindow,
                            m_actions.settingsLaunchNewTab})
        action->setCheckable(true);
    m_actions.settingsLaunchActivate->setData(Settings::ActivateExistingWindow);
    m_actions.settingsLaunchNewWindow->setData(Settings::OpenNewWindow);
    m_actions.settingsLaunchNewTab->setData(Settings::OpenNewTab);
    m_actions.settingsBehaviorAutoAns = new QAction(this);
    m_actions.settingsBehaviorAutoCompletion = new QAction(this);
    m_actions.settingsBehaviorAutoCompletionAcceptWithEnter = new QAction(this);
    m_actions.settingsBehaviorAutoCompletionAcceptWithEnter->setObjectName(QStringLiteral("AutoCompletionAcceptWithEnterAction"));
    m_actions.settingsBehaviorAutoCompletionBuiltInFunctions = new QAction(this);
    m_actions.settingsBehaviorAutoCompletionBuiltInVariables = new QAction(this);
    m_actions.settingsBehaviorAutoCompletionLongFormUnits = new QAction(this);
    m_actions.settingsBehaviorAutoCompletionUserFunctions = new QAction(this);
    m_actions.settingsBehaviorAutoCompletionUserVariables = new QAction(this);
    m_actions.settingsBehaviorLeaveLastExpression = new QAction(this);
    m_actions.settingsBehaviorNumberFormat = new QAction(this);
    m_actions.settingsResetAll = new QAction(this);
    m_actions.settingsResetAll->setObjectName(QStringLiteral("ResetAllSettingsAction"));
    m_actions.settingsBehaviorResultSlots = new QAction(this);
    m_actions.settingsBehaviorUpDownArrowNever = new QAction(this);
    m_actions.settingsBehaviorUpDownArrowAlways = new QAction(this);
    m_actions.settingsBehaviorUpDownArrowSingleLineOnly = new QAction(this);
    m_actions.settingsBehaviorPartialResults = new QAction(this);
    m_actions.settingsBehaviorSaveWindowPositionOnExit = new QAction(this);
    m_actions.settingsBehaviorSyntaxHighlighting = new QAction(this);
    m_actions.settingsEditingInputDigitGrouping = new QAction(this);
    m_actions.settingsEditingAutomaticInputAdjustments = new QAction(this);
    m_actions.settingsEditingAutomaticInputAdjustments->setObjectName(QStringLiteral("AutomaticInputAdjustmentsAction"));
    m_actions.settingsEditingInputDigitGrouping->setObjectName(QStringLiteral("InputDigitGroupingAction"));
    m_actions.settingsEditingCursorBlinking = new QAction(this);
    m_actions.settingsEditingCursorBlinking->setObjectName(QStringLiteral("CursorBlinkingAction"));
    m_actions.settingsEditingEscapeClearsEditor = new QAction(this);
    m_actions.settingsEditingEscapeClearsEditor->setObjectName(QStringLiteral("EscapeClearsEditorAction"));
    m_actions.settingsBehaviorHoverHighlightResults = new QAction(this);
    m_actions.settingsBehaviorDigitGroupingNone = new QAction(this);
    m_actions.settingsBehaviorDigitGroupingOneSpace = new QAction(this);
    m_actions.settingsBehaviorDigitGroupingTwoSpaces = new QAction(this);
    m_actions.settingsBehaviorDigitGroupingThreeSpaces = new QAction(this);
    m_actions.settingsBehaviorDigitGroupingIntegerPartOnly = new QAction(this);
    m_actions.settingsBehaviorAutoResultToClipboard = new QAction(this);
    m_actions.settingsBehaviorSimplifyResultExpressions = new QAction(this);
    m_actions.settingsBehaviorHistorySizeLimit = new QAction(this);
    m_actions.settingsDisplayFont = new QAction(this);
    m_actions.settingsDisplayColorSchemeCustom = new QAction(this);
    m_actions.settingsSystemColorModeFollowSystem = new QAction(this);
    m_actions.settingsSystemColorModeFollowSystem->setCheckable(true);
    m_actions.settingsSystemColorModeFollowSystem->setData(Settings::SystemColorModeFollowSystem);
    m_actions.settingsSystemColorModeLight = new QAction(this);
    m_actions.settingsSystemColorModeLight->setCheckable(true);
    m_actions.settingsSystemColorModeLight->setData(Settings::SystemColorModeLight);
    m_actions.settingsSystemColorModeDark = new QAction(this);
    m_actions.settingsSystemColorModeDark->setCheckable(true);
    m_actions.settingsSystemColorModeDark->setData(Settings::SystemColorModeDark);
    m_actions.settingsMenuAppearanceSystem = new QAction(this);
    m_actions.settingsMenuAppearanceSpeedCrunch = new QAction(this);
    m_actions.settingsMenuAppearanceSystem->setCheckable(true);
    m_actions.settingsMenuAppearanceSystem->setData(Settings::MenuAppearanceSystem);
    m_actions.settingsMenuAppearanceSpeedCrunch->setCheckable(true);
    m_actions.settingsMenuAppearanceSpeedCrunch->setData(Settings::MenuAppearanceSpeedCrunch);
    m_actions.settingsDockAppearanceSystem = new QAction(this);
    m_actions.settingsDockAppearanceSpeedCrunch = new QAction(this);
    m_actions.settingsDockAppearanceSystem->setCheckable(true);
    m_actions.settingsDockAppearanceSystem->setData(Settings::DockAppearanceSystem);
    m_actions.settingsDockAppearanceSpeedCrunch->setCheckable(true);
    m_actions.settingsDockAppearanceSpeedCrunch->setData(Settings::DockAppearanceSpeedCrunch);
    m_actions.settingsStatusBarAppearanceSystem = new QAction(this);
    m_actions.settingsStatusBarAppearanceSpeedCrunch = new QAction(this);
    m_actions.settingsStatusBarAppearanceSystem->setCheckable(true);
    m_actions.settingsStatusBarAppearanceSystem->setData(Settings::StatusBarAppearanceSystem);
    m_actions.settingsStatusBarAppearanceSpeedCrunch->setCheckable(true);
    m_actions.settingsStatusBarAppearanceSpeedCrunch->setData(Settings::StatusBarAppearanceSpeedCrunch);
    m_actions.settingsKeypadAppearanceClassic = new QAction(this);
    m_actions.settingsKeypadAppearanceClassic->setCheckable(true);
    m_actions.settingsKeypadAppearanceClassic->setData(Settings::KeypadAppearanceClassic);
    m_actions.settingsKeypadAppearanceModern = new QAction(this);
    m_actions.settingsKeypadAppearanceModern->setCheckable(true);
    m_actions.settingsKeypadAppearanceModern->setData(Settings::KeypadAppearanceModern);
    m_actions.settingsEditorDisplayClassic = new QAction(this);
    m_actions.settingsEditorDisplayClassic->setCheckable(true);
    m_actions.settingsEditorDisplayClassic->setData(Settings::EditorDisplayClassic);
    m_actions.settingsEditorDisplayModern = new QAction(this);
    m_actions.settingsEditorDisplayModern->setCheckable(true);
    m_actions.settingsEditorDisplayModern->setData(Settings::EditorDisplayModern);
    m_actions.settingsLanguage = new QAction(this);
    m_actions.settingsRadixCharComma = new QAction(this);
    m_actions.settingsRadixCharDefault = new QAction(this);
    m_actions.settingsRadixCharDot = new QAction(this);
    m_actions.settingsRadixCharBoth = new QAction(this);
    m_actions.settingsResultFormat0Digits = new QAction(this);
    m_actions.settingsResultFormat15Digits = new QAction(this);
    m_actions.settingsResultFormat2Digits = new QAction(this);
    m_actions.settingsResultFormat3Digits = new QAction(this);
    m_actions.settingsResultFormat50Digits = new QAction(this);
    m_actions.settingsResultFormat8Digits = new QAction(this);
    m_actions.settingsResultFormatCustomDigits = new QAction(this);
    m_actions.settingsResultRoundingHalfAwayFromZero = new QAction(this);
    m_actions.settingsResultRoundingHalfEven = new QAction(this);
    m_actions.settingsResultRoundingTowardZero = new QAction(this);
    m_actions.settingsResultRoundingTowardPositiveInfinity = new QAction(this);
    m_actions.settingsResultRoundingTowardNegativeInfinity = new QAction(this);
    m_actions.settingsResultFormatAutoPrecision = new QAction(this);
    m_actions.settingsResultFormatBinary = new QAction(this);
    m_actions.settingsResultFormatEngineering = new QAction(this);
    m_actions.settingsRemoveTrailingZeroes = new QAction(this);
    m_actions.settingsRemoveTrailingZeroes->setObjectName(QStringLiteral("RemoveTrailingZeroesAction"));
    m_actions.settingsRemoveTrailingZeroes->setCheckable(true);
    m_actions.settingsResultFormatFixed = new QAction(this);
    m_actions.settingsResultFormatGeneral = new QAction(this);
    m_actions.settingsResultFormatHexadecimal = new QAction(this);
    m_actions.settingsResultFormatOctal = new QAction(this);
    m_actions.settingsResultFormatRational = new QAction(this);
    m_actions.settingsResultFormatScientific = new QAction(this);
    m_actions.settingsResultFormatCartesian= new QAction(this);
    m_actions.settingsResultFormatPolar = new QAction(this);
    m_actions.settingsResultFormatTrigonometric = new QAction(this);
    m_actions.settingsResultFormatCis = new QAction(this);
    m_actions.settingsResultFormatPolarAngle = new QAction(this);
    m_actions.settingsImaginaryUnitI = new QAction(this);
    m_actions.settingsImaginaryUnitJ = new QAction(this);
    m_actions.settingsResultFormatSexagesimal = new QAction(this);
    m_actions.settingsUnitNegativeExponentSuperscript = new QAction(this);
    m_actions.settingsUnitNegativeExponentFraction = new QAction(this);
    m_actions.helpManual = new QAction(this);
#ifdef SPEEDCRUNCH_ENABLE_UPDATE_CHECKS
    m_actions.helpUpdates = new QAction(this);
#endif
    m_actions.helpFeedback = new QAction(this);
    m_actions.helpCommunity = new QAction(this);
    m_actions.helpSource = new QAction(this);
    m_actions.helpDonate = new QAction(this);
    m_actions.helpAbout = new QAction(this);
    m_actions.helpDiagnostics = new QAction(this);
    m_actions.helpDiagnostics->setObjectName(QStringLiteral("AppDiagnosticsAction"));
    m_actions.helpDiagnostics->setMenuRole(QAction::NoRole);
    m_actions.contextHelp = new QAction(this);

    m_actions.settingsAngleUnitDegree->setCheckable(true);
    m_actions.settingsAngleUnitRadian->setCheckable(true);
    m_actions.settingsAngleUnitGradian->setCheckable(true);
    m_actions.settingsAngleUnitTurn->setCheckable(true);
    m_actions.settingsAngleUnitRevolution->setCheckable(true);
    m_actions.settingsBehaviorAlwaysOnTop->setCheckable(true);
    m_actions.settingsOpenPrivateTabOnStartup->setCheckable(true);
    m_actions.settingsBehaviorAutoAns->setCheckable(true);
    m_actions.settingsBehaviorAutoCompletion->setCheckable(true);
    m_actions.settingsBehaviorAutoCompletionAcceptWithEnter->setCheckable(true);
    m_actions.settingsBehaviorAutoCompletionBuiltInFunctions->setCheckable(true);
    m_actions.settingsBehaviorAutoCompletionBuiltInVariables->setCheckable(true);
    m_actions.settingsBehaviorAutoCompletionLongFormUnits->setCheckable(true);
    m_actions.settingsBehaviorAutoCompletionUserFunctions->setCheckable(true);
    m_actions.settingsBehaviorAutoCompletionUserVariables->setCheckable(true);
    m_actions.settingsBehaviorLeaveLastExpression->setCheckable(true);
    m_actions.settingsBehaviorUpDownArrowNever->setCheckable(true);
    m_actions.settingsBehaviorUpDownArrowNever->setData(Settings::UpDownArrowBehaviorNever);
    m_actions.settingsBehaviorUpDownArrowAlways->setCheckable(true);
    m_actions.settingsBehaviorUpDownArrowAlways->setData(Settings::UpDownArrowBehaviorAlways);
    m_actions.settingsBehaviorUpDownArrowSingleLineOnly->setCheckable(true);
    m_actions.settingsBehaviorUpDownArrowSingleLineOnly->setData(Settings::UpDownArrowBehaviorSingleLineOnly);
    m_actions.settingsBehaviorPartialResults->setCheckable(true);
    m_actions.settingsBehaviorSaveWindowPositionOnExit->setCheckable(true);
    m_actions.settingsBehaviorSyntaxHighlighting->setCheckable(true);
    m_actions.settingsEditingInputDigitGrouping->setCheckable(true);
    m_actions.settingsEditingAutomaticInputAdjustments->setCheckable(true);
    m_actions.settingsEditingCursorBlinking->setCheckable(true);
    m_actions.settingsEditingEscapeClearsEditor->setCheckable(true);
    m_actions.settingsBehaviorHoverHighlightResults->setCheckable(true);
    m_actions.settingsBehaviorDigitGroupingNone->setCheckable(true);
    m_actions.settingsBehaviorDigitGroupingNone->setData(0);
    m_actions.settingsBehaviorDigitGroupingOneSpace->setCheckable(true);
    m_actions.settingsBehaviorDigitGroupingOneSpace->setData(1);
    m_actions.settingsBehaviorDigitGroupingTwoSpaces->setCheckable(true);
    m_actions.settingsBehaviorDigitGroupingTwoSpaces->setData(2);
    m_actions.settingsBehaviorDigitGroupingThreeSpaces->setCheckable(true);
    m_actions.settingsBehaviorDigitGroupingThreeSpaces->setData(3);
    m_actions.settingsBehaviorDigitGroupingIntegerPartOnly->setCheckable(true);
    m_actions.settingsBehaviorAutoResultToClipboard->setCheckable(true);
    m_actions.settingsBehaviorSimplifyResultExpressions->setCheckable(true);
    m_actions.settingsRadixCharComma->setCheckable(true);
    m_actions.settingsRadixCharDefault->setCheckable(true);
    m_actions.settingsRadixCharDot->setCheckable(true);
    m_actions.settingsRadixCharBoth->setCheckable(true);
    m_actions.settingsResultFormat0Digits->setCheckable(true);
    m_actions.settingsResultFormat15Digits->setCheckable(true);
    m_actions.settingsResultFormat2Digits->setCheckable(true);
    m_actions.settingsResultFormat3Digits->setCheckable(true);
    m_actions.settingsResultFormat50Digits->setCheckable(true);
    m_actions.settingsResultFormat8Digits->setCheckable(true);
    m_actions.settingsResultFormatCustomDigits->setCheckable(true);
    m_actions.settingsResultRoundingHalfAwayFromZero->setCheckable(true);
    m_actions.settingsResultRoundingHalfAwayFromZero->setData(Settings::ResultRoundingHalfAwayFromZero);
    m_actions.settingsResultRoundingHalfEven->setCheckable(true);
    m_actions.settingsResultRoundingHalfEven->setData(Settings::ResultRoundingHalfEven);
    m_actions.settingsResultRoundingTowardZero->setCheckable(true);
    m_actions.settingsResultRoundingTowardZero->setData(Settings::ResultRoundingTowardZero);
    m_actions.settingsResultRoundingTowardPositiveInfinity->setCheckable(true);
    m_actions.settingsResultRoundingTowardPositiveInfinity->setData(Settings::ResultRoundingTowardPositiveInfinity);
    m_actions.settingsResultRoundingTowardNegativeInfinity->setCheckable(true);
    m_actions.settingsResultRoundingTowardNegativeInfinity->setData(Settings::ResultRoundingTowardNegativeInfinity);
    m_actions.settingsResultFormatAutoPrecision->setCheckable(true);
    m_actions.settingsResultFormatBinary->setCheckable(true);
    m_actions.settingsResultFormatCartesian->setCheckable(true);
    m_actions.settingsResultFormatEngineering->setCheckable(true);
    m_actions.settingsResultFormatFixed->setCheckable(true);
    m_actions.settingsResultFormatGeneral->setCheckable(true);
    m_actions.settingsResultFormatHexadecimal->setCheckable(true);
    m_actions.settingsResultFormatOctal->setCheckable(true);
    m_actions.settingsResultFormatPolar->setCheckable(true);
    m_actions.settingsResultFormatTrigonometric->setCheckable(true);
    m_actions.settingsResultFormatCis->setCheckable(true);
    m_actions.settingsResultFormatPolarAngle->setCheckable(true);
    m_actions.settingsImaginaryUnitI->setCheckable(true);
    m_actions.settingsImaginaryUnitJ->setCheckable(true);
    m_actions.settingsResultFormatRational->setCheckable(true);
    m_actions.settingsResultFormatScientific->setCheckable(true);
    m_actions.settingsResultFormatSexagesimal->setCheckable(true);
    m_actions.settingsUnitNegativeExponentSuperscript->setCheckable(true);
    m_actions.settingsUnitNegativeExponentSuperscript->setData(
        static_cast<int>(Settings::UnitNegativeExponentSuperscript));
    m_actions.settingsUnitNegativeExponentFraction->setCheckable(true);
    m_actions.settingsUnitNegativeExponentFraction->setData(
        static_cast<int>(Settings::UnitNegativeExponentFraction));
    m_actions.viewConstants->setCheckable(true);
    m_actions.viewFullScreenMode->setCheckable(true);
    m_actions.viewFunctions->setCheckable(true);
    m_actions.viewHistory->setCheckable(true);
    m_actions.viewKeypadDisabled->setCheckable(true);
    m_actions.viewKeypadDisabled->setData(Settings::KeypadModeDisabled);
    m_actions.viewKeypadBasicWide->setCheckable(true);
    m_actions.viewKeypadBasicWide->setData(Settings::KeypadModeBasicWide);
    m_actions.viewKeypadScientificWide->setCheckable(true);
    m_actions.viewKeypadScientificWide->setData(Settings::KeypadModeScientificWide);
    m_actions.viewKeypadScientificNarrow->setCheckable(true);
    m_actions.viewKeypadScientificNarrow->setData(Settings::KeypadModeScientificNarrow);
    m_actions.viewKeypadCustom->setCheckable(true);
    m_actions.viewKeypadCustom->setData(Settings::KeypadModeCustom);
    m_actions.viewKeypadZoom50->setCheckable(true);
    m_actions.viewKeypadZoom50->setData(50);
    m_actions.viewKeypadZoom100->setCheckable(true);
    m_actions.viewKeypadZoom100->setData(100);
    m_actions.viewKeypadZoom150->setCheckable(true);
    m_actions.viewKeypadZoom150->setData(150);
    m_actions.viewKeypadZoom200->setCheckable(true);
    m_actions.viewKeypadZoom200->setData(200);
    m_actions.viewFormulaBook->setCheckable(true);
    m_actions.viewStatusBar->setCheckable(true);
    m_actions.viewMenuBar->setCheckable(true);
    m_actions.viewVariables->setCheckable(true);
    m_actions.viewBitfield->setCheckable(true);
    m_actions.viewUserFunctions->setCheckable(true);
    m_actions.viewUserUnits->setCheckable(true);

    const auto schemes = ColorScheme::enumerate(); // TODO: use qAsConst().
    for (auto& colorScheme : schemes) {
        auto action = new QAction(this);
        action->setCheckable(true);
        action->setText(colorSchemeDisplayName(colorScheme));
        action->setData(colorScheme);
        m_actions.settingsDisplayColorSchemes.append(action);
    }
}

void MainWindow::retranslateText()
{
    StartupDiagnostics::Scope timing("Load translations and translate UI");
    QTranslator* tr = 0;
    tr = createTranslator(m_settings->language);
    if (tr) {
        if (m_translator) {
            qApp->removeTranslator(m_translator);
            m_translator->deleteLater();
        }

        qApp->installTranslator(tr);
        m_translator = tr;
        const QString translationLanguage = tr->language().isEmpty()
            ? QFileInfo(tr->filePath()).completeBaseName() : tr->language();
        qApp->setProperty("speedcrunchTranslationLanguage", tr->isEmpty()
                          ? QStringLiteral("English (fallback)") : translationLanguage);
    } else {
        qApp->removeTranslator(m_translator);
        m_translator = 0;
    }

    setMenusText();
    setActionsText();
    for (const QPointer<MainWindow>& ptr : allMainWindows()) {
        if (MainWindow* window = ptr.data())
            window->setStatusBarText();
    }
    setWidgetsDirection();
}

void MainWindow::setStatusBarText()
{
    if (m_status.angleUnit) {
        m_status.angleUnitLabel->setText(MainWindow::tr("Angle Mode:"));
        m_status.resultFormatLabel->setText(MainWindow::tr("Notation:"));
        m_status.resultPrecisionLabel->setText(MainWindow::tr("Precision:"));
        m_status.angleUnit->setText(statusBarAngleUnitValue());
        m_status.resultFormat->setText(statusBarResultFormatValue());
        m_status.resultPrecision->setText(statusBarResultPrecisionValue());
        updateStatusBarSectionVisibility();
    }
}

void MainWindow::updateStatusBarSectionVisibility()
{
    if (!m_status.angleUnit)
        return;

    QStatusBar* bar = statusBar();
    const int availableWidth = bar->contentsRect().width();
    const int spacing = qMax(0, bar->style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing, nullptr, bar));

    struct Section {
        QWidget* widget;
        QWidget* separatorBefore;
        bool enabled;
    };

    const Section sections[] = {
        { m_status.resultFormatSection, nullptr, true },
        { m_status.resultPrecisionSection, m_status.resultPrecisionSeparator, true },
        { m_status.angleUnitSection, m_status.angleUnitSeparator, true }
    };

    int usedWidth = 0;
    bool hasVisibleSection = false;
    for (const Section& section : sections) {
        if (!section.enabled) {
            section.widget->setVisible(false);
            if (section.separatorBefore != nullptr)
                section.separatorBefore->setVisible(false);
            continue;
        }

        const int sectionWidth = section.widget->sizeHint().width();
        const int separatorWidth =
            hasVisibleSection && section.separatorBefore != nullptr
                ? section.separatorBefore->sizeHint().width()
                : 0;
        const int gap = hasVisibleSection
            ? spacing + (section.separatorBefore != nullptr ? separatorWidth + spacing : 0)
            : 0;
        if (usedWidth + gap + sectionWidth <= availableWidth) {
            if (section.separatorBefore != nullptr)
                section.separatorBefore->setVisible(hasVisibleSection);
            section.widget->setVisible(true);
            usedWidth += gap + sectionWidth;
            hasVisibleSection = true;
        } else {
            if (section.separatorBefore != nullptr)
                section.separatorBefore->setVisible(false);
            section.widget->setVisible(false);
        }
    }
}

void MainWindow::updateColorSchemeActionState()
{
    const auto schemes = m_actions.settingsDisplayColorSchemes;
    bool colorSchemeMatched = false;
    for (auto& action : schemes) {
        const bool matches = m_settings->colorScheme == action->data().toString();
        action->setChecked(matches);
        colorSchemeMatched = colorSchemeMatched || matches;
    }
    m_actions.settingsDisplayColorSchemeCustom->setChecked(!colorSchemeMatched
                                                           && m_settings->colorScheme == QLatin1String("Custom"));
}

namespace {
void applySystemColorMode(Settings::SystemColorMode mode)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    Qt::ColorScheme scheme = Qt::ColorScheme::Unknown;
    if (mode == Settings::SystemColorModeLight)
        scheme = Qt::ColorScheme::Light;
    else if (mode == Settings::SystemColorModeDark)
        scheme = Qt::ColorScheme::Dark;
    // Unknown removes the override, even when the effective colors are unchanged.
    QGuiApplication::styleHints()->setColorScheme(scheme);
#else
    Q_UNUSED(mode);
#endif
}
}

void MainWindow::setSystemColorMode(QAction* action)
{
    if (action == nullptr)
        return;
    const int mode = action->data().toInt();
    if (mode < Settings::SystemColorModeFollowSystem || mode > Settings::SystemColorModeDark)
        return;
    m_settings->systemColorMode = static_cast<Settings::SystemColorMode>(mode);
    for (const QPointer<MainWindow>& window : allMainWindows()) {
        if (!window)
            continue;
        for (QAction* choice : window->m_actionGroups.systemColorMode->actions())
            choice->setChecked(choice->data().toInt() == mode);
    }
    applySystemColorMode(m_settings->systemColorMode);
    MenuStyle::refresh();
    for (const QPointer<MainWindow>& window : allMainWindows()) {
        if (!window)
            continue;
        // Some platform themes change their control palettes without notifying
        // every window. Refresh after Qt finishes applying the requested mode.
        QTimer::singleShot(0, window, [window]() {
            window->applyThemeSurfacePalette();
            window->updateSplitterStyleSheet();
        });
    }
}

void MainWindow::setMenuAppearance(QAction* action)
{
    if (action == nullptr)
        return;
    m_settings->menuAppearance = action->data().toInt() == Settings::MenuAppearanceSpeedCrunch
        ? Settings::MenuAppearanceSpeedCrunch : Settings::MenuAppearanceSystem;
    MenuStyle::refresh();
    for (const QPointer<MainWindow>& window : allMainWindows()) {
        if (!window)
            continue;
        window->m_actions.settingsMenuAppearanceSystem->setChecked(
            m_settings->menuAppearance == Settings::MenuAppearanceSystem);
        window->m_actions.settingsMenuAppearanceSpeedCrunch->setChecked(
            m_settings->menuAppearance == Settings::MenuAppearanceSpeedCrunch);
        emit window->menuAppearanceChanged();
    }
}

void MainWindow::setDockAppearance(QAction* action)
{
    if (action == nullptr)
        return;
    m_settings->dockAppearance = action->data().toInt() == Settings::DockAppearanceSystem
        ? Settings::DockAppearanceSystem : Settings::DockAppearanceSpeedCrunch;
    const GeneratedThemeSurfaces surfaces = generatedSurfaceColors(m_settings);
    for (const QPointer<MainWindow>& window : allMainWindows()) {
        if (!window)
            continue;
        window->m_actions.settingsDockAppearanceSystem->setChecked(
            m_settings->dockAppearance == Settings::DockAppearanceSystem);
        window->m_actions.settingsDockAppearanceSpeedCrunch->setChecked(
            m_settings->dockAppearance == Settings::DockAppearanceSpeedCrunch);
        for (QDockWidget* dock : window->m_allDocks) {
            applyDockAppearance(window, dock, surfaces);
        }
        window->applyDockTabBarAppearance();
        window->updateSplitterStyleSheet();
        window->update();
    }
}

void MainWindow::setStatusBarAppearance(QAction* action)
{
    if (action == nullptr)
        return;
    m_settings->statusBarAppearance = action->data().toInt() == Settings::StatusBarAppearanceSystem
        ? Settings::StatusBarAppearanceSystem : Settings::StatusBarAppearanceSpeedCrunch;
    const GeneratedThemeSurfaces surfaces = generatedSurfaceColors(m_settings);
    for (const QPointer<MainWindow>& window : allMainWindows()) {
        if (!window)
            continue;
        window->m_actions.settingsStatusBarAppearanceSystem->setChecked(
            m_settings->statusBarAppearance == Settings::StatusBarAppearanceSystem);
        window->m_actions.settingsStatusBarAppearanceSpeedCrunch->setChecked(
            m_settings->statusBarAppearance == Settings::StatusBarAppearanceSpeedCrunch);
        applyStatusBarAppearance(window->findChild<QStatusBar*>(QString(), Qt::FindDirectChildrenOnly),
                                 surfaces);
        window->updateStatusBarSectionVisibility();
    }
}

void MainWindow::setKeypadAppearance(QAction* action)
{
    if (action == nullptr)
        return;
    m_settings->keypadAppearance = action->data().toInt() == Settings::KeypadAppearanceClassic
        ? Settings::KeypadAppearanceClassic : Settings::KeypadAppearanceModern;
    for (const QPointer<MainWindow>& window : allMainWindows()) {
        if (window)
            window->applyKeypadAppearance();
    }
}

void MainWindow::applyKeypadAppearance()
{
    StartupDiagnostics::Scope timing("Apply keypad appearance");
    const bool classic = m_settings->keypadAppearance == Settings::KeypadAppearanceClassic;
    m_actions.settingsKeypadAppearanceClassic->setChecked(classic);
    m_actions.settingsKeypadAppearanceModern->setChecked(!classic);
    if (m_widgets.keypad)
        m_widgets.keypad->setAppearance(classic ? Keypad::Classic : Keypad::Modern);
}

void MainWindow::setEditorDisplayStyle(QAction* action)
{
    if (action == nullptr)
        return;
    m_settings->editorDisplayStyle = action->data().toInt() == Settings::EditorDisplayClassic
        ? Settings::EditorDisplayClassic : Settings::EditorDisplayModern;
    for (const QPointer<MainWindow>& window : allMainWindows()) {
        if (window)
            window->applyEditorDisplayStyle();
    }
    updatePaneEditorCursorVisibility();
}

void MainWindow::applyEditorDisplayStyle()
{
    StartupDiagnostics::Scope timing("Apply editor and display appearance");
    m_actions.settingsEditorDisplayClassic->setChecked(
        m_settings->editorDisplayStyle == Settings::EditorDisplayClassic);
    m_actions.settingsEditorDisplayModern->setChecked(
        m_settings->editorDisplayStyle == Settings::EditorDisplayModern);
    for (ResultDisplay* display : splitPaneDisplays())
        display->applyEditorDisplayStyle();
    for (Editor* editor : splitPaneEditors())
        editor->rehighlight();
    updateSplitterStyleSheet();
    if (m_widgets.state != nullptr && m_widgets.state->isVisible()) {
        // The new editor height takes effect when Qt lays out the pane.
        QTimer::singleShot(0, this, [this]() {
            if (m_widgets.state->isVisible())
                showStateLabel(m_widgets.state->text());
        });
    }
}

QString MainWindow::statusBarAngleUnitValue() const
{
    return (m_status.selectedAngleUnit == 'r' ? MainWindow::tr("Radian")
        : (m_status.selectedAngleUnit == 'g') ? MainWindow::tr("Gradian")
        : (m_status.selectedAngleUnit == 't') ? MainWindow::tr("Turn")
        : (m_status.selectedAngleUnit == 'v') ? MainWindow::tr("Revolution")
        : MainWindow::tr("Degree"));
}

QString MainWindow::statusBarResultFormatValue() const
{
    switch (m_status.selectedResultFormat) {
        case 'b': return MainWindow::tr("Binary");
        case 'o': return MainWindow::tr("Octal");
        case 'h': return MainWindow::tr("Hexadecimal");
        case 's': return MainWindow::tr("Sexagesimal");
        case 'f': return MainWindow::tr("Fixed-point decimal");
        case 'n': return MainWindow::tr("Engineering decimal");
        case 'e': return MainWindow::tr("Scientific decimal");
        case 'r': return MainWindow::tr("Rational");
        case 'g': return MainWindow::tr("Automatic decimal");
        default : return QString();
    }
}

QString MainWindow::statusBarResultPrecisionValue() const
{
    if (m_status.selectedResultPrecision < 0)
        return MainWindow::tr("Automatic");
    return QString::number(m_status.selectedResultPrecision);
}

void MainWindow::setActionsText()
{
    m_actions.sessionExportHtml->setText(QStringLiteral("&HTML"));
    m_actions.sessionExportJson->setText(QStringLiteral("JSON"));
    m_actions.sessionExportPlainText->setText(MainWindow::tr("Plain &text"));
    m_actions.sessionImport->setText(MainWindow::tr("&Import..."));
    m_actions.sessionImportUserDefinitions->setText(MainWindow::tr("User &Definitions..."));
    m_actions.sessionNewTab->setText(MainWindow::tr("New &Tab"));
    m_actions.sessionNewPrivate->setText(MainWindow::tr("New &Private Tab"));
    m_actions.sessionConvertPrivacy->setText(m_session && m_session->isPrivate()
        ? tr("Keep") : tr("Make Private..."));
    m_actions.sessionNewWindow->setText(MainWindow::tr("New &Window"));
    m_actions.sessionOpen->setText(MainWindow::tr("&Open..."));
    m_actions.sessionRename->setText(MainWindow::tr("&Rename..."));
    m_actions.sessionClone->setText(tr("Clo&ne..."));
    m_actions.sessionClose->setText(tr("&Close Tab"));
    m_actions.sessionReopen->setText(tr("Reopen Clo&sed Tab"));
    m_actions.sessionOpenSessionsFolder->setText(MainWindow::tr("Open Sessions &Folder"));
    m_actions.sessionQuit->setText(MainWindow::tr("&Quit"));

    m_actions.editClearExpression->setText(MainWindow::tr("Clear E&xpression"));
    m_actions.editUndo->setText(tr("&Undo"));
    m_actions.editRedo->setText(tr("&Redo"));
    m_actions.editCut->setText(tr("Cu&t"));
    m_actions.editClearHistory->setText(MainWindow::tr("Clear &History"));
    m_actions.editCopyLastResult->setText(MainWindow::tr("Copy &Last Result"));
    m_actions.editCopy->setText(MainWindow::tr("&Copy"));
    m_actions.editPaste->setText(MainWindow::tr("&Paste"));
    m_actions.editSelectExpression->setText(MainWindow::tr("&Select Expression"));
    m_actions.editWrapSelection->setText(MainWindow::tr("&Wrap in Parentheses"));

    m_actions.viewConstants->setText(MainWindow::tr("&Constants"));
    m_actions.viewFullScreenMode->setText(MainWindow::tr("Full Scr&een Mode"));
    m_actions.viewFunctions->setText(MainWindow::tr("&Functions"));
    m_actions.viewHistory->setText(MainWindow::tr("&History"));
    updateKeypadDisabledActionText();
    m_actions.viewKeypadBasicWide->setText(MainWindow::tr("&Basic"));
    m_actions.viewKeypadScientificWide->setText(MainWindow::tr("&Scientific (wide)"));
    m_actions.viewKeypadScientificNarrow->setText(MainWindow::tr("Scientific (narrow)"));
    m_actions.viewKeypadCustom->setText(MainWindow::tr("&Custom..."));
    m_actions.viewKeypadZoom50->setText(QStringLiteral("50%"));
    m_actions.viewKeypadZoom100->setText(QStringLiteral("100%"));
    m_actions.viewKeypadZoom150->setText(QStringLiteral("150%"));
    m_actions.viewKeypadZoom200->setText(QStringLiteral("200%"));
    m_actions.viewFormulaBook->setText(MainWindow::tr("Formula &Book"));
    m_actions.viewStatusBar->setText(MainWindow::tr("&Status Bar"));
    m_actions.viewMenuBar->setText(MainWindow::tr("Main &Menu"));
    m_actions.viewVariables->setText(MainWindow::tr("User &Variables"));
    m_actions.viewBitfield->setText(MainWindow::tr("Bitfield"));
    m_actions.viewSplitLeft->setText(tr("&Left"));
    m_actions.viewSplitRight->setText(tr("&Right"));
    m_actions.viewSplitUp->setText(tr("&Up"));
    m_actions.viewSplitDown->setText(tr("&Down"));
    m_actions.viewClosePane->setText(tr("Close Pa&ne"));
    m_actions.viewUserFunctions->setText(MainWindow::tr("Use&r Functions"));
    m_actions.viewUserUnits->setText(MainWindow::tr("User &Units"));

    m_actions.settingsAngleUnitDegree->setText(MainWindow::tr("&Degree"));
    m_actions.settingsAngleUnitRadian->setText(MainWindow::tr("&Radian"));
    m_actions.settingsAngleUnitGradian->setText(MainWindow::tr("&Gradian"));
    m_actions.settingsAngleUnitTurn->setText(MainWindow::tr("&Turn"));
    m_actions.settingsAngleUnitRevolution->setText(MainWindow::tr("Re&volution"));
    m_actions.settingsBehaviorAlwaysOnTop->setText(MainWindow::tr("Always on &Top"));
    m_actions.settingsBehaviorAutoAns->setText(MainWindow::tr("Auto-Insert a&ns"));
    m_actions.settingsBehaviorAutoAns->setToolTip(MainWindow::tr("If a new expression starts with +, -, *, or /, SpeedCrunch inserts \"ans\" first."));
    m_actions.settingsBehaviorAutoAns->setStatusTip(MainWindow::tr("If a new expression starts with +, -, *, or /, SpeedCrunch inserts \"ans\" first."));
    m_actions.settingsBehaviorAutoCompletion->setText(MainWindow::tr("Automatic &Completion"));
    m_actions.settingsBehaviorAutoCompletionAcceptWithEnter->setText(tr("&Accept with Enter"));
    m_actions.settingsBehaviorAutoCompletionAcceptWithEnter->setToolTip(tr("Accept the current autocomplete suggestion with Enter without first selecting it with the arrow keys."));
    m_actions.settingsBehaviorAutoCompletionAcceptWithEnter->setStatusTip(m_actions.settingsBehaviorAutoCompletionAcceptWithEnter->toolTip());
    m_actions.settingsBehaviorAutoCompletionBuiltInFunctions->setText(MainWindow::tr("&Built-in Functions"));
    m_actions.settingsBehaviorAutoCompletionBuiltInVariables->setText(MainWindow::tr("Built-in &Variables"));
    m_actions.settingsBehaviorAutoCompletionLongFormUnits->setText(MainWindow::tr("&Units"));
    m_actions.settingsBehaviorAutoCompletionUserFunctions->setText(MainWindow::tr("User &Functions"));
    m_actions.settingsBehaviorAutoCompletionUserVariables->setText(MainWindow::tr("User Va&riables"));
    m_actions.settingsBehaviorPartialResults->setText(MainWindow::tr("Show Live Result &Preview"));
    m_actions.settingsBehaviorSaveWindowPositionOnExit->setText(MainWindow::tr("Save &Window Position on Exit"));
    m_actions.settingsOpenPrivateTabOnStartup->setText(tr("Open a Private Tab on Startup"));
    m_actions.settingsLaunchActivate->setText(tr("Activate Existing Window"));
    m_actions.settingsLaunchNewWindow->setText(tr("Open New Window"));
    m_actions.settingsLaunchNewTab->setText(tr("Open New Tab"));
    m_actions.settingsOpenPrivateTabOnStartup->setToolTip(tr("Open and focus a new private tab whenever SpeedCrunch starts."));
    m_actions.settingsOpenPrivateTabOnStartup->setStatusTip(m_actions.settingsOpenPrivateTabOnStartup->toolTip());
    m_actions.settingsBehaviorSyntaxHighlighting->setText(MainWindow::tr("Syntax &Highlighting"));
    m_actions.settingsEditingInputDigitGrouping->setText(MainWindow::tr("Group Digits in &Input"));
    m_actions.settingsEditingAutomaticInputAdjustments->setText(MainWindow::tr("Automatic Input &Adjustments"));
    m_actions.settingsEditingAutomaticInputAdjustments->setToolTip(MainWindow::tr("Automatically adjust operators, spacing, brackets, and powers, and restrict input to valid characters."));
    m_actions.settingsEditingInputDigitGrouping->setToolTip(MainWindow::tr("Group input digits using the selected number format."));
    m_actions.settingsEditingCursorBlinking->setText(MainWindow::tr("&Blinking Cursor"));
    m_actions.settingsEditingCursorBlinking->setToolTip(MainWindow::tr("Blink the cursor in the expression editor."));
    m_actions.settingsEditingEscapeClearsEditor->setText(MainWindow::tr("&Escape Clears Editor"));
    m_actions.settingsEditingEscapeClearsEditor->setToolTip(MainWindow::tr("Clear the focused expression editor with Escape after dismissing any popup or preview."));
    m_actions.settingsBehaviorHoverHighlightResults->setText(MainWindow::tr("Hover Highlighting"));
    m_actions.settingsBehaviorDigitGroupingNone->setText(MainWindow::tr("Disabled"));
    m_actions.settingsBehaviorDigitGroupingOneSpace->setText(MainWindow::tr("Small Space"));
    m_actions.settingsBehaviorDigitGroupingTwoSpaces->setText(MainWindow::tr("Medium Space"));
    m_actions.settingsBehaviorDigitGroupingThreeSpaces->setText(MainWindow::tr("Large Space"));
    m_actions.settingsBehaviorDigitGroupingIntegerPartOnly->setText(MainWindow::tr("Group Integer Part Only"));
    m_actions.settingsBehaviorLeaveLastExpression->setText(MainWindow::tr("Keep Expression After Evaluation"));
    m_actions.settingsBehaviorNumberFormat->setText(MainWindow::tr("Number Format..."));
    m_actions.settingsBehaviorResultSlots->setText(MainWindow::tr("Notation && Precision..."));
    m_actions.settingsBehaviorLeaveLastExpression->setToolTip(MainWindow::tr("After pressing Enter, keep the entered expression selected in the editor."));
    m_actions.settingsBehaviorLeaveLastExpression->setStatusTip(MainWindow::tr("After pressing Enter, keep the entered expression selected in the editor."));
    m_actions.settingsBehaviorUpDownArrowNever->setText(MainWindow::tr("Never"));
    m_actions.settingsBehaviorUpDownArrowAlways->setText(MainWindow::tr("Always"));
    m_actions.settingsBehaviorUpDownArrowSingleLineOnly->setText(MainWindow::tr("Only for Single-Line Expressions"));
    m_actions.settingsBehaviorAutoResultToClipboard->setText(MainWindow::tr("Copy New Results Automatically"));
    m_actions.settingsBehaviorSimplifyResultExpressions->setText(MainWindow::tr("Simplify Displayed Expressions"));
    m_actions.settingsBehaviorHistorySizeLimit->setText(MainWindow::tr("History Size &Limit..."));
    m_actions.settingsRadixCharComma->setText(MainWindow::tr("&Comma"));
    m_actions.settingsRadixCharDefault->setText(MainWindow::tr("&System Default"));
    m_actions.settingsRadixCharDot->setText(MainWindow::tr("&Dot"));
    m_actions.settingsRadixCharBoth->setText(MainWindow::tr("Dot &And Comma"));
    m_actions.settingsResultFormat0Digits->setText(MainWindow::tr("&0 Digits"));
    m_actions.settingsResultFormat15Digits->setText(MainWindow::tr("&15 Digits"));
    m_actions.settingsResultFormat2Digits->setText(MainWindow::tr("&2 Digits"));
    m_actions.settingsResultFormat3Digits->setText(MainWindow::tr("&3 Digits"));
    m_actions.settingsResultFormat50Digits->setText(MainWindow::tr("&50 Digits"));
    m_actions.settingsResultFormat8Digits->setText(MainWindow::tr("&8 Digits"));
    m_actions.settingsResultFormatCustomDigits->setText(MainWindow::tr("&Custom..."));
    m_actions.settingsRemoveTrailingZeroes->setText(MainWindow::tr("Remove Trailing &Zeroes"));
    m_actions.settingsResultRoundingHalfAwayFromZero->setText(
        MainWindow::tr("Nearest, Half &Away (round)"));
    m_actions.settingsResultRoundingHalfEven->setText(
        MainWindow::tr("Nearest, Half &Even (roundeven)"));
    m_actions.settingsResultRoundingTowardZero->setText(MainWindow::tr("Toward &Zero (trunc)"));
    m_actions.settingsResultRoundingTowardPositiveInfinity->setText(
        MainWindow::tr("Toward +∞ (&ceil)"));
    m_actions.settingsResultRoundingTowardNegativeInfinity->setText(
        MainWindow::tr("Toward −∞ (&floor)"));
    m_actions.settingsResultFormatAutoPrecision->setText(MainWindow::tr("&Automatic"));
    m_actions.settingsResultFormatGeneral->setText(MainWindow::tr("&Automatic"));
    m_actions.settingsResultFormatFixed->setText(MainWindow::tr("&Fixed-Point"));
    m_actions.settingsResultFormatEngineering->setText(MainWindow::tr("&Engineering"));
    m_actions.settingsResultFormatScientific->setText(MainWindow::tr("&Scientific"));
    m_actions.settingsResultFormatRational->setText(MainWindow::tr("&Rational"));
    m_actions.settingsResultFormatBinary->setText(MainWindow::tr("&Binary"));
    m_actions.settingsResultFormatOctal->setText(MainWindow::tr("&Octal"));
    m_actions.settingsResultFormatHexadecimal->setText(MainWindow::tr("&Hexadecimal"));
    m_actions.settingsResultFormatSexagesimal->setText(MainWindow::tr("&Sexagesimal"));
    m_actions.settingsUnitNegativeExponentSuperscript->setText(
        MainWindow::tr("&Exponential (m·s⁻¹)"));
    m_actions.settingsUnitNegativeExponentFraction->setText(
        MainWindow::tr("&Fractional (m/s)"));
    m_actions.settingsResultFormatCartesian->setText(MainWindow::tr("&Rectangular (a + bi)"));
    m_actions.settingsResultFormatPolar->setText(MainWindow::tr("Exponential (reⁱᶿ)"));
    m_actions.settingsResultFormatTrigonometric->setText(
        MainWindow::tr("Trigonometric (r(cos θ + i·sin θ))"));
    m_actions.settingsResultFormatCis->setText(QStringLiteral("Cis (r·cis(θ))"));
    m_actions.settingsResultFormatPolarAngle->setText(MainWindow::tr("Phasor (r∠θ)"));
    m_actions.settingsImaginaryUnitI->setText(QStringLiteral("&i"));
    m_actions.settingsImaginaryUnitJ->setText(QStringLiteral("&j"));
    m_actions.settingsDisplayFont->setText(MainWindow::tr("&Font..."));
    m_actions.settingsResetAll->setText(MainWindow::tr("Reset All Settings..."));
    m_actions.settingsDisplayColorSchemeCustom->setText(MainWindow::tr("&Theme..."));
    m_actions.settingsSystemColorModeFollowSystem->setText(MainWindow::tr("&Follow System"));
    m_actions.settingsSystemColorModeLight->setText(MainWindow::tr("&Light"));
    m_actions.settingsSystemColorModeDark->setText(MainWindow::tr("&Dark"));
    m_actions.settingsMenuAppearanceSystem->setText(MainWindow::tr("&System"));
    m_actions.settingsMenuAppearanceSpeedCrunch->setText(MainWindow::tr("SpeedCrunch &Theme"));
    m_actions.settingsDockAppearanceSystem->setText(MainWindow::tr("&System"));
    m_actions.settingsDockAppearanceSpeedCrunch->setText(MainWindow::tr("SpeedCrunch &Theme"));
    m_actions.settingsStatusBarAppearanceSystem->setText(MainWindow::tr("&System"));
    m_actions.settingsStatusBarAppearanceSpeedCrunch->setText(MainWindow::tr("SpeedCrunch &Theme"));
    m_actions.settingsKeypadAppearanceClassic->setText(MainWindow::tr("&Classic"));
    m_actions.settingsKeypadAppearanceModern->setText(MainWindow::tr("&Modern"));
    m_actions.settingsEditorDisplayClassic->setText(MainWindow::tr("&Classic"));
    m_actions.settingsEditorDisplayModern->setText(MainWindow::tr("&Modern"));
    m_actions.settingsLanguage->setText(MainWindow::tr("&Language..."));

    m_actions.helpManual->setText(MainWindow::tr("User &Manual"));
    m_actions.contextHelp->setText(MainWindow::tr("Context Help"));
#ifdef SPEEDCRUNCH_ENABLE_UPDATE_CHECKS
    m_actions.helpUpdates->setText(MainWindow::tr("Check for &Updates"));
#endif
    m_actions.helpFeedback->setText(MainWindow::tr("Issues and &Feedback"));
    m_actions.helpCommunity->setText(MainWindow::tr("Community"));
    m_actions.helpSource->setText(MainWindow::tr("Source Code"));
    m_actions.helpDonate->setText(MainWindow::tr("&Donate"));
    m_actions.helpAbout->setText(MainWindow::tr("&About SpeedCrunch"));
    m_actions.helpDiagnostics->setText(MainWindow::tr("App Diag&nostics"));
}

void MainWindow::createActionGroups()
{
    m_actionGroups.subsequentLaunch = new QActionGroup(this);
    m_actionGroups.subsequentLaunch->addAction(m_actions.settingsLaunchActivate);
    m_actionGroups.subsequentLaunch->addAction(m_actions.settingsLaunchNewWindow);
    m_actionGroups.subsequentLaunch->addAction(m_actions.settingsLaunchNewTab);
    StartupDiagnostics::Scope timing("Create action groups");
    m_actionGroups.resultFormat = new QActionGroup(this);
    m_actionGroups.resultFormat->addAction(m_actions.settingsResultFormatBinary);
    m_actionGroups.resultFormat->addAction(m_actions.settingsResultFormatGeneral);
    m_actionGroups.resultFormat->addAction(m_actions.settingsResultFormatFixed);
    m_actionGroups.resultFormat->addAction(m_actions.settingsResultFormatEngineering);
    m_actionGroups.resultFormat->addAction(m_actions.settingsResultFormatScientific);
    m_actionGroups.resultFormat->addAction(m_actions.settingsResultFormatRational);
    m_actionGroups.resultFormat->addAction(m_actions.settingsResultFormatOctal);
    m_actionGroups.resultFormat->addAction(m_actions.settingsResultFormatHexadecimal);
    m_actionGroups.resultFormat->addAction(m_actions.settingsResultFormatSexagesimal);


    m_actionGroups.complexFormat = new QActionGroup(this);
    m_actionGroups.complexFormat->addAction(m_actions.settingsResultFormatCartesian);
    m_actionGroups.complexFormat->addAction(m_actions.settingsResultFormatPolar);
    m_actionGroups.complexFormat->addAction(m_actions.settingsResultFormatTrigonometric);
    m_actionGroups.complexFormat->addAction(m_actions.settingsResultFormatCis);
    m_actionGroups.complexFormat->addAction(m_actions.settingsResultFormatPolarAngle);

    m_actionGroups.imaginaryUnit = new QActionGroup(this);
    m_actionGroups.imaginaryUnit->addAction(m_actions.settingsImaginaryUnitI);
    m_actionGroups.imaginaryUnit->addAction(m_actions.settingsImaginaryUnitJ);

    m_actionGroups.radixChar = new QActionGroup(this);
    m_actionGroups.radixChar->addAction(m_actions.settingsRadixCharDefault);
    m_actionGroups.radixChar->addAction(m_actions.settingsRadixCharDot);
    m_actionGroups.radixChar->addAction(m_actions.settingsRadixCharComma);
    m_actionGroups.radixChar->addAction(m_actions.settingsRadixCharBoth);

    m_actionGroups.digits = new QActionGroup(this);
    m_actionGroups.digits->addAction(m_actions.settingsResultFormatAutoPrecision);
    m_actionGroups.digits->addAction(m_actions.settingsResultFormat0Digits);
    m_actionGroups.digits->addAction(m_actions.settingsResultFormat2Digits);
    m_actionGroups.digits->addAction(m_actions.settingsResultFormat3Digits);
    m_actionGroups.digits->addAction(m_actions.settingsResultFormat8Digits);
    m_actionGroups.digits->addAction(m_actions.settingsResultFormat15Digits);
    m_actionGroups.digits->addAction(m_actions.settingsResultFormat50Digits);
    m_actionGroups.digits->addAction(m_actions.settingsResultFormatCustomDigits);

    m_actionGroups.resultRoundingMode = new QActionGroup(this);
    m_actionGroups.resultRoundingMode->addAction(m_actions.settingsResultRoundingHalfAwayFromZero);
    m_actionGroups.resultRoundingMode->addAction(m_actions.settingsResultRoundingHalfEven);
    m_actionGroups.resultRoundingMode->addAction(m_actions.settingsResultRoundingTowardZero);
    m_actionGroups.resultRoundingMode->addAction(m_actions.settingsResultRoundingTowardPositiveInfinity);
    m_actionGroups.resultRoundingMode->addAction(m_actions.settingsResultRoundingTowardNegativeInfinity);

    m_actionGroups.angle = new QActionGroup(this);
    m_actionGroups.angle->addAction(m_actions.settingsAngleUnitDegree);
    m_actionGroups.angle->addAction(m_actions.settingsAngleUnitRadian);
    m_actionGroups.angle->addAction(m_actions.settingsAngleUnitGradian);
    m_actionGroups.angle->addAction(m_actions.settingsAngleUnitTurn);
    m_actionGroups.angle->addAction(m_actions.settingsAngleUnitRevolution);

    m_actionGroups.colorScheme = new QActionGroup(this);
    m_actionGroups.systemColorMode = new QActionGroup(this);
    m_actionGroups.systemColorMode->addAction(m_actions.settingsSystemColorModeFollowSystem);
    m_actionGroups.systemColorMode->addAction(m_actions.settingsSystemColorModeLight);
    m_actionGroups.systemColorMode->addAction(m_actions.settingsSystemColorModeDark);
    m_actionGroups.menuAppearance = new QActionGroup(this);
    m_actionGroups.menuAppearance->addAction(m_actions.settingsMenuAppearanceSystem);
    m_actionGroups.menuAppearance->addAction(m_actions.settingsMenuAppearanceSpeedCrunch);
    m_actionGroups.dockAppearance = new QActionGroup(this);
    m_actionGroups.dockAppearance->addAction(m_actions.settingsDockAppearanceSystem);
    m_actionGroups.dockAppearance->addAction(m_actions.settingsDockAppearanceSpeedCrunch);
    m_actionGroups.statusBarAppearance = new QActionGroup(this);
    m_actionGroups.statusBarAppearance->addAction(m_actions.settingsStatusBarAppearanceSystem);
    m_actionGroups.statusBarAppearance->addAction(m_actions.settingsStatusBarAppearanceSpeedCrunch);
    m_actionGroups.keypadAppearance = new QActionGroup(this);
    m_actionGroups.keypadAppearance->addAction(m_actions.settingsKeypadAppearanceClassic);
    m_actionGroups.keypadAppearance->addAction(m_actions.settingsKeypadAppearanceModern);
    m_actionGroups.editorDisplayStyle = new QActionGroup(this);
    m_actionGroups.editorDisplayStyle->addAction(m_actions.settingsEditorDisplayClassic);
    m_actionGroups.editorDisplayStyle->addAction(m_actions.settingsEditorDisplayModern);
    const auto schemes = m_actions.settingsDisplayColorSchemes;
    for (auto& action : schemes)
        m_actionGroups.colorScheme->addAction(action);

    m_actionGroups.digitGrouping = new QActionGroup(this);
    m_actionGroups.digitGrouping->addAction(m_actions.settingsBehaviorDigitGroupingNone);
    m_actionGroups.digitGrouping->addAction(m_actions.settingsBehaviorDigitGroupingOneSpace);
    m_actionGroups.digitGrouping->addAction(m_actions.settingsBehaviorDigitGroupingTwoSpaces);
    m_actionGroups.digitGrouping->addAction(m_actions.settingsBehaviorDigitGroupingThreeSpaces);

    m_actionGroups.upDownArrowBehavior = new QActionGroup(this);
    m_actionGroups.upDownArrowBehavior->addAction(m_actions.settingsBehaviorUpDownArrowNever);
    m_actionGroups.upDownArrowBehavior->addAction(m_actions.settingsBehaviorUpDownArrowAlways);
    m_actionGroups.upDownArrowBehavior->addAction(m_actions.settingsBehaviorUpDownArrowSingleLineOnly);

    m_actionGroups.keypad = new QActionGroup(this);
    m_actionGroups.keypad->addAction(m_actions.viewKeypadDisabled);
    m_actionGroups.keypad->addAction(m_actions.viewKeypadBasicWide);
    m_actionGroups.keypad->addAction(m_actions.viewKeypadScientificWide);
    m_actionGroups.keypad->addAction(m_actions.viewKeypadScientificNarrow);
    m_actionGroups.keypad->addAction(m_actions.viewKeypadCustom);

    m_actionGroups.keypadZoom = new QActionGroup(this);
    m_actionGroups.keypadZoom->addAction(m_actions.viewKeypadZoom50);
    m_actionGroups.keypadZoom->addAction(m_actions.viewKeypadZoom100);
    m_actionGroups.keypadZoom->addAction(m_actions.viewKeypadZoom150);
    m_actionGroups.keypadZoom->addAction(m_actions.viewKeypadZoom200);

    m_actionGroups.unitNegativeExponentStyle = new QActionGroup(this);
    m_actionGroups.unitNegativeExponentStyle->addAction(
        m_actions.settingsUnitNegativeExponentSuperscript);
    m_actionGroups.unitNegativeExponentStyle->addAction(
        m_actions.settingsUnitNegativeExponentFraction);
}

void MainWindow::createActionShortcuts()
{
    StartupDiagnostics::Scope timing("Create shortcuts");
    QList<QKeySequence> newTabBindings = QKeySequence::keyBindings(QKeySequence::AddTab);
    for (const QKeySequence& binding : QKeySequence::keyBindings(QKeySequence::New)) {
        if (!newTabBindings.contains(binding))
            newTabBindings.append(binding);
    }
    m_actions.sessionNewTab->setShortcuts(newTabBindings);
    m_actions.sessionOpen->setShortcuts(QKeySequence::Open);
    m_actions.sessionClose->setShortcuts(QKeySequence::Close);
    m_actions.sessionReopen->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_T);
    m_actions.editUndo->setShortcuts(QKeySequence::Undo);
    m_actions.editRedo->setShortcuts(QKeySequence::Redo);
    m_actions.editCut->setShortcuts(QKeySequence::Cut);
    m_actions.sessionNewPrivate->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_N);
    // Let this QAction own Quit; a separate standard-key QShortcut would
    // duplicate Ctrl+Q on Linux and make the shortcut ambiguous.
    m_actions.sessionQuit->setShortcut(Qt::CTRL | Qt::Key_Q);
    m_actions.editCopyLastResult->setShortcut(Qt::CTRL | Qt::Key_R);
    m_actions.editCopy->setShortcut(Qt::CTRL | Qt::Key_C);
    m_actions.editPaste->setShortcut(Qt::CTRL | Qt::Key_V);
    m_actions.editSelectExpression->setShortcut(Qt::CTRL | Qt::Key_A);
    m_actions.editWrapSelection->setShortcuts({
        QKeySequence(Qt::CTRL | Qt::Key_ParenLeft),
        QKeySequence(Qt::CTRL | Qt::Key_ParenRight)
    });
    m_actions.viewBitfield->setShortcut(Qt::CTRL | Qt::Key_8);
    m_actions.viewConstants->setShortcut(Qt::CTRL | Qt::Key_2);
    m_actions.viewFullScreenMode->setShortcut(Qt::Key_F11);
    m_actions.viewFunctions->setShortcut(Qt::CTRL | Qt::Key_3);
    m_actions.viewHistory->setShortcut(Qt::CTRL | Qt::Key_7);
    m_actions.viewFormulaBook->setShortcut(Qt::CTRL | Qt::Key_1);
    m_actions.viewStatusBar->setShortcut(Qt::CTRL | Qt::Key_B);
    m_actions.viewVariables->setShortcut(Qt::CTRL | Qt::Key_4);
    m_actions.viewUserFunctions->setShortcut(Qt::CTRL | Qt::Key_5);
    m_actions.viewUserUnits->setShortcut(Qt::CTRL | Qt::Key_6);
    m_actions.settingsResultFormatGeneral->setShortcut(Qt::Key_F2);
    m_actions.settingsResultFormatFixed->setShortcut(Qt::Key_F3);
    m_actions.settingsResultFormatEngineering->setShortcut(Qt::Key_F4);
    m_actions.settingsResultFormatScientific->setShortcut(Qt::Key_F5);
    m_actions.settingsResultFormatOctal->setShortcut(Qt::Key_F7);
    m_actions.settingsResultFormatHexadecimal->setShortcut(Qt::Key_F8);
    m_actions.settingsResultFormatSexagesimal->setShortcut(Qt::Key_F9);
    m_actions.settingsResultFormatBinary->setShortcut(Qt::Key_F10);
    m_actions.contextHelp->setShortcut(Qt::Key_F1);

    // The notation menus are standalone popups, so register their shortcuts
    // with the window even when those menus and the status bar are hidden.
    addActions({m_actions.settingsResultFormatGeneral,
                m_actions.settingsResultFormatFixed,
                m_actions.settingsResultFormatEngineering,
                m_actions.settingsResultFormatScientific,
                m_actions.settingsResultFormatOctal,
                m_actions.settingsResultFormatHexadecimal,
                m_actions.settingsResultFormatSexagesimal,
                m_actions.settingsResultFormatBinary});
}

void MainWindow::createMenus()
{
    StartupDiagnostics::Scope timing("Create menus");
    QMenuBar* bar;
    {
        StartupDiagnostics::Scope menuBarTiming("Create menu bar");
        bar = menuBar();
    }
    StartupDiagnostics::Scope contents("Populate application menus");
    m_menus.session = new QMenu("", this);
    bar->addMenu(m_menus.session);
    m_menus.session->addAction(m_actions.sessionNewTab);
    m_menus.session->addAction(m_actions.sessionNewPrivate);
    m_menus.session->addAction(m_actions.sessionNewWindow);
    m_menus.session->addSeparator();
    m_menus.session->addAction(m_actions.sessionOpen);
    m_menus.session->addAction(m_actions.sessionRename);
    m_menus.session->addAction(m_actions.sessionClone);
    m_menus.session->addAction(m_actions.sessionConvertPrivacy);
    m_menus.session->addSeparator();
    m_menus.session->addAction(m_actions.sessionImport);
    m_menus.sessionExport = m_menus.session->addMenu("");
    m_menus.sessionExport->addAction(m_actions.sessionExportJson);
    m_menus.sessionExport->addAction(m_actions.sessionExportPlainText);
    m_menus.sessionExport->addAction(m_actions.sessionExportHtml);
    m_menus.session->addAction(m_actions.sessionOpenSessionsFolder);
    m_menus.session->addSeparator();
    m_menus.session->addAction(m_actions.settingsBehaviorHistorySizeLimit);
    m_menus.session->addSeparator();
    m_menus.session->addAction(m_actions.sessionClose);
    m_menus.session->addAction(m_actions.sessionReopen);
    m_menus.session->addSeparator();
    m_menus.session->addAction(m_actions.sessionQuit);
    connect(m_menus.session, &QMenu::aboutToShow, this, [this]() {
        syncSessionMenuActionState();
    });

    m_menus.edit = new QMenu("", this);
    bar->addMenu(m_menus.edit);
    m_menus.edit->addAction(m_actions.editUndo);
    m_menus.edit->addAction(m_actions.editRedo);
    m_menus.edit->addSeparator();
    m_menus.edit->addAction(m_actions.editCut);
    m_menus.edit->addAction(m_actions.editCopy);
    m_menus.edit->addAction(m_actions.editPaste);
    m_menus.edit->addAction(m_actions.editCopyLastResult);
    m_menus.edit->addSeparator();
    m_menus.edit->addAction(m_actions.editSelectExpression);
    m_menus.edit->addAction(m_actions.editWrapSelection);
    m_menus.edit->addAction(m_actions.editClearExpression);
    m_menus.edit->addSeparator();
    m_menus.edit->addAction(m_actions.editClearHistory);
    connect(m_menus.edit, &QMenu::aboutToShow, this, &MainWindow::syncEditMenuActionState);

    m_menus.view = new QMenu("", this);
    bar->addMenu(m_menus.view);
    m_menus.keypad = m_menus.view->addMenu("");
    m_menus.keypad->addAction(m_actions.viewKeypadDisabled);
    m_menus.keypad->addAction(m_actions.viewKeypadBasicWide);
    m_menus.keypad->addAction(m_actions.viewKeypadScientificWide);
    m_menus.keypad->addAction(m_actions.viewKeypadScientificNarrow);
    m_menus.keypad->addAction(m_actions.viewKeypadCustom);
    m_menus.view->addAction(m_actions.viewStatusBar);
    m_menus.view->addSeparator();
    m_menus.view->addAction(m_actions.viewFormulaBook);
    m_menus.view->addAction(m_actions.viewConstants);
    m_menus.view->addAction(m_actions.viewFunctions);
    m_menus.view->addSeparator();
    m_menus.view->addAction(m_actions.viewVariables);
    m_menus.view->addAction(m_actions.viewUserFunctions);
    m_menus.view->addAction(m_actions.viewUserUnits);
    m_menus.view->addSeparator();
    m_menus.view->addAction(m_actions.viewHistory);
    m_menus.view->addAction(m_actions.viewBitfield);
    m_menus.view->addSeparator();
    m_menus.split = m_menus.view->addMenu("");
    m_menus.split->addAction(m_actions.viewSplitLeft);
    m_menus.split->addAction(m_actions.viewSplitRight);
    m_menus.split->addAction(m_actions.viewSplitUp);
    m_menus.split->addAction(m_actions.viewSplitDown);
    m_menus.view->addAction(m_actions.viewClosePane);
#if !defined(Q_OS_MACOS)
    m_menus.view->addSeparator();
    m_menus.view->addAction(m_actions.viewMenuBar);
    m_menus.view->addAction(m_actions.viewFullScreenMode);
#endif
    m_menus.settings = new QMenu("", this);
    bar->addMenu(m_menus.settings);
    m_menus.settings->addAction(m_actions.settingsBehaviorNumberFormat);
    m_menus.settings->addAction(m_actions.settingsLanguage);
    m_menus.settings->addSeparator();

    m_menus.display = m_menus.settings->addMenu("");
    m_menus.display->addAction(m_actions.settingsDisplayColorSchemeCustom);
    m_menus.systemColorMode = m_menus.display->addMenu("");
    m_menus.systemColorMode->setObjectName(QStringLiteral("SystemColorModeMenu"));
    m_menus.systemColorMode->addActions(m_actionGroups.systemColorMode->actions());
#if QT_VERSION < QT_VERSION_CHECK(6, 8, 0)
    m_menus.systemColorMode->menuAction()->setVisible(false);
#endif
    m_menus.display->addSeparator();
    m_menus.menuAppearance = m_menus.display->addMenu("");
    m_menus.menuAppearance->setObjectName(QStringLiteral("MenuAppearanceMenu"));
    m_menus.menuAppearance->addAction(m_actions.settingsMenuAppearanceSystem);
    m_menus.menuAppearance->addAction(m_actions.settingsMenuAppearanceSpeedCrunch);
    m_menus.dockAppearance = m_menus.display->addMenu("");
    m_menus.dockAppearance->setObjectName(QStringLiteral("DockAppearanceMenu"));
    m_menus.dockAppearance->addAction(m_actions.settingsDockAppearanceSystem);
    m_menus.dockAppearance->addAction(m_actions.settingsDockAppearanceSpeedCrunch);
    m_menus.editorDisplayStyle = m_menus.display->addMenu("");
    m_menus.editorDisplayStyle->setObjectName(QStringLiteral("EditorDisplayStyleMenu"));
    m_menus.editorDisplayStyle->addAction(m_actions.settingsEditorDisplayClassic);
    m_menus.editorDisplayStyle->addAction(m_actions.settingsEditorDisplayModern);
    m_menus.editorDisplayStyle->addSeparator();
    m_menus.editorDisplayStyle->addAction(m_actions.settingsDisplayFont);
    m_menus.keypadAppearance = m_menus.display->addMenu("");
    m_menus.keypadAppearance->setObjectName(QStringLiteral("KeypadAppearanceMenu"));
    m_menus.keypadAppearance->addAction(m_actions.settingsKeypadAppearanceClassic);
    m_menus.keypadAppearance->addAction(m_actions.settingsKeypadAppearanceModern);
    m_menus.keypadAppearance->addSeparator();
    m_menus.keypadZoom = m_menus.keypadAppearance->addMenu("");
    m_menus.keypadZoom->addAction(m_actions.viewKeypadZoom50);
    m_menus.keypadZoom->addAction(m_actions.viewKeypadZoom100);
    m_menus.keypadZoom->addAction(m_actions.viewKeypadZoom150);
    m_menus.keypadZoom->addAction(m_actions.viewKeypadZoom200);
    m_menus.statusBarAppearance = m_menus.display->addMenu("");
    m_menus.statusBarAppearance->setObjectName(QStringLiteral("StatusBarAppearanceMenu"));
    m_menus.statusBarAppearance->addAction(m_actions.settingsStatusBarAppearanceSystem);
    m_menus.statusBarAppearance->addAction(m_actions.settingsStatusBarAppearanceSpeedCrunch);
    m_menus.display->addSeparator();
    m_menus.display->addAction(m_actions.settingsBehaviorSyntaxHighlighting);
    m_menus.display->addAction(m_actions.settingsBehaviorHoverHighlightResults);
    m_menus.editing = m_menus.settings->addMenu("");
    m_menus.editing->addAction(m_actions.settingsEditingAutomaticInputAdjustments);
    m_menus.editing->addAction(m_actions.settingsBehaviorAutoAns);
    m_menus.editing->addAction(m_actions.settingsEditingInputDigitGrouping);
    m_menus.editing->addAction(m_actions.settingsEditingCursorBlinking);
    m_menus.editing->addAction(m_actions.settingsEditingEscapeClearsEditor);
    m_menus.editing->addAction(m_actions.settingsBehaviorLeaveLastExpression);
    m_menus.editing->setToolTipsVisible(true);
    m_menus.editing->addSeparator();
    m_menus.autoCompletion = m_menus.editing->addMenu("");
    m_menus.autoCompletion->setToolTipsVisible(true);
    m_menus.autoCompletion->addAction(m_actions.settingsBehaviorAutoCompletionAcceptWithEnter);
    m_menus.autoCompletion->addSeparator();
    m_menus.autoCompletion->addAction(m_actions.settingsBehaviorAutoCompletionBuiltInFunctions);
    m_menus.autoCompletion->addAction(m_actions.settingsBehaviorAutoCompletionBuiltInVariables);
    m_menus.autoCompletion->addAction(m_actions.settingsBehaviorAutoCompletionLongFormUnits);
    m_menus.autoCompletion->addAction(m_actions.settingsBehaviorAutoCompletionUserFunctions);
    m_menus.autoCompletion->addAction(m_actions.settingsBehaviorAutoCompletionUserVariables);
    m_menus.upDownArrowBehavior = m_menus.editing->addMenu("");
    m_menus.upDownArrowBehavior->addAction(m_actions.settingsBehaviorUpDownArrowNever);
    m_menus.upDownArrowBehavior->addAction(m_actions.settingsBehaviorUpDownArrowAlways);
    m_menus.upDownArrowBehavior->addAction(m_actions.settingsBehaviorUpDownArrowSingleLineOnly);

    m_menus.results = m_menus.settings->addMenu("");
    m_menus.results->addAction(m_actions.settingsBehaviorResultSlots);
    m_menus.results->addAction(m_actions.settingsRemoveTrailingZeroes);
    m_menus.results->addSeparator();
    m_menus.resultRoundingMode = m_menus.results->addMenu("");
    m_menus.resultRoundingMode->addAction(m_actions.settingsResultRoundingHalfAwayFromZero);
    m_menus.resultRoundingMode->addAction(m_actions.settingsResultRoundingHalfEven);
    m_menus.resultRoundingMode->addAction(m_actions.settingsResultRoundingTowardZero);
    m_menus.resultRoundingMode->addAction(m_actions.settingsResultRoundingTowardPositiveInfinity);
    m_menus.resultRoundingMode->addAction(m_actions.settingsResultRoundingTowardNegativeInfinity);
    m_menus.complexForm = m_menus.results->addMenu("");
    m_menus.complexForm->addAction(m_actions.settingsResultFormatCartesian);
    m_menus.complexForm->addAction(m_actions.settingsResultFormatPolar);
    m_menus.complexForm->addAction(m_actions.settingsResultFormatTrigonometric);
    m_menus.complexForm->addAction(m_actions.settingsResultFormatCis);
    m_menus.complexForm->addAction(m_actions.settingsResultFormatPolarAngle);
    m_menus.imaginaryUnit = m_menus.results->addMenu("");
    m_menus.imaginaryUnit->addAction(m_actions.settingsImaginaryUnitI);
    m_menus.imaginaryUnit->addAction(m_actions.settingsImaginaryUnitJ);
    m_menus.unitNegativeExponentStyle = m_menus.results->addMenu("");
    m_menus.unitNegativeExponentStyle->addAction(
        m_actions.settingsUnitNegativeExponentSuperscript);
    m_menus.unitNegativeExponentStyle->addAction(
        m_actions.settingsUnitNegativeExponentFraction);
    m_menus.results->addSeparator();

    // Deprecated direct menus kept as internal context menus only; users should
    // configure these via "Notation & Precision...".
    m_menus.resultFormat = new QMenu("", this);
    m_menus.decimal = m_menus.resultFormat->addMenu("");
    m_menus.decimal->addAction(m_actions.settingsResultFormatGeneral);
    m_menus.decimal->addAction(m_actions.settingsResultFormatFixed);
    m_menus.decimal->addAction(m_actions.settingsResultFormatEngineering);
    m_menus.decimal->addAction(m_actions.settingsResultFormatScientific);
    m_menus.resultFormat->addAction(m_actions.settingsResultFormatRational);
    m_menus.resultFormat->addAction(m_actions.settingsResultFormatBinary);
    m_menus.resultFormat->addAction(m_actions.settingsResultFormatOctal);
    m_menus.resultFormat->addAction(m_actions.settingsResultFormatHexadecimal);
    m_menus.resultFormat->addAction(m_actions.settingsResultFormatSexagesimal);

    m_menus.precision = new QMenu("", this);
    m_menus.precision->addAction(m_actions.settingsResultFormatAutoPrecision);
    m_menus.precision->addAction(m_actions.settingsResultFormat0Digits);
    m_menus.precision->addAction(m_actions.settingsResultFormat2Digits);
    m_menus.precision->addAction(m_actions.settingsResultFormat3Digits);
    m_menus.precision->addAction(m_actions.settingsResultFormat8Digits);
    m_menus.precision->addAction(m_actions.settingsResultFormat15Digits);
    m_menus.precision->addAction(m_actions.settingsResultFormat50Digits);
    m_menus.precision->addSeparator();
    m_menus.precision->addAction(m_actions.settingsResultFormatCustomDigits);

    m_menus.results->addAction(m_actions.settingsBehaviorPartialResults);
    m_menus.results->addAction(m_actions.settingsBehaviorSimplifyResultExpressions);
    m_menus.results->addAction(m_actions.settingsBehaviorAutoResultToClipboard);

    m_menus.angleUnit = m_menus.settings->addMenu("");
    m_menus.angleUnit->addAction(m_actions.settingsAngleUnitDegree);
    m_menus.angleUnit->addAction(m_actions.settingsAngleUnitRadian);
    m_menus.angleUnit->addAction(m_actions.settingsAngleUnitGradian);
    m_menus.angleUnit->addAction(m_actions.settingsAngleUnitTurn);
    m_menus.angleUnit->addAction(m_actions.settingsAngleUnitRevolution);

    m_menus.window = m_menus.settings->addMenu("");
    m_menus.subsequentLaunch = m_menus.window->addMenu("");
    m_menus.subsequentLaunch->setObjectName(QStringLiteral("SubsequentLaunchMenu"));
    m_menus.subsequentLaunch->addActions(m_actionGroups.subsequentLaunch->actions());
    m_menus.window->addAction(m_actions.settingsOpenPrivateTabOnStartup);
    m_menus.window->addAction(m_actions.settingsBehaviorSaveWindowPositionOnExit);
    if (!isWaylandPlatform())
        m_menus.window->addAction(m_actions.settingsBehaviorAlwaysOnTop);

    m_menus.settings->addSeparator();
    m_menus.settings->addAction(m_actions.sessionImportUserDefinitions);
    m_menus.settings->addSeparator();
    m_menus.settings->addAction(m_actions.settingsResetAll);

    m_menus.help = new QMenu("", this);
    bar->addMenu(m_menus.help);
    m_menus.help->addAction(m_actions.helpManual);
    m_menus.help->addAction(m_actions.contextHelp);
    m_menus.help->addSeparator();
    m_menus.help->addAction(m_actions.helpCommunity);
    m_menus.help->addAction(m_actions.helpFeedback);
    m_menus.help->addAction(m_actions.helpSource);
    m_menus.help->addAction(m_actions.helpDonate);
    m_menus.help->addSeparator();
#ifdef SPEEDCRUNCH_ENABLE_UPDATE_CHECKS
    m_menus.help->addAction(m_actions.helpUpdates);
#endif
    m_menus.help->addAction(m_actions.helpDiagnostics);
    m_menus.help->addAction(m_actions.helpAbout);

    addActions(bar->actions());
}

void MainWindow::setMenusText()
{
    m_menus.subsequentLaunch->setTitle(tr("On Subsequent Launch"));
    m_menus.session->setTitle(MainWindow::tr("&Session"));
    m_menus.sessionExport->setTitle(MainWindow::tr("&Export"));
    m_menus.edit->setTitle(MainWindow::tr("&Edit"));
    m_menus.view->setTitle(MainWindow::tr("&View"));
    m_menus.keypad->setTitle(MainWindow::tr("&Keypad"));
    m_menus.split->setTitle(tr("S&plit"));
    m_menus.keypadZoom->setTitle(MainWindow::tr("&Zoom"));
    m_menus.settings->setTitle(MainWindow::tr("Se&ttings"));
    m_menus.results->setTitle(MainWindow::tr("&Results"));
    m_menus.unitNegativeExponentStyle->setTitle(MainWindow::tr("Unit Notation"));
    m_menus.resultRoundingMode->setTitle(MainWindow::tr("Rounding Mode"));
    m_menus.resultFormat->setTitle(MainWindow::tr("&Notation"));
    m_menus.decimal->setTitle(MainWindow::tr("&Decimal"));
    m_menus.precision->setTitle(MainWindow::tr("&Precision"));
    m_menus.angleUnit->setTitle(MainWindow::tr("Angle &Mode"));
    m_menus.complexForm->setTitle(MainWindow::tr("&Complex Form"));
    m_menus.imaginaryUnit->setTitle(MainWindow::tr("&Imaginary Unit"));
    m_menus.window->setTitle(MainWindow::tr("&Window"));
    m_menus.editing->setTitle(MainWindow::tr("&Editing"));
    m_menus.autoCompletion->setTitle(MainWindow::tr("A&utocomplete"));
    m_menus.upDownArrowBehavior->setTitle(MainWindow::tr("Up/Down Arrow History"));
    m_menus.display->setTitle(MainWindow::tr("&Appearance"));
    m_menus.systemColorMode->setTitle(MainWindow::tr("System &Color Mode"));
    m_menus.menuAppearance->setTitle(MainWindow::tr("&Menus"));
    m_menus.dockAppearance->setTitle(MainWindow::tr("&Docks"));
    m_menus.statusBarAppearance->setTitle(MainWindow::tr("&Status Bar"));
    m_menus.keypadAppearance->setTitle(MainWindow::tr("&Keypad"));
    m_menus.editorDisplayStyle->setTitle(MainWindow::tr("&Input and Results"));
    m_menus.help->setTitle(MainWindow::tr("&Help"));
}

void MainWindow::updateKeypadDisabledActionText()
{
    if (m_actions.viewKeypadDisabled->isChecked())
        m_actions.viewKeypadDisabled->setText(MainWindow::tr("&Disabled"));
    else
        m_actions.viewKeypadDisabled->setText(MainWindow::tr("&Disable"));
}

void MainWindow::updateKeypadModeActionState()
{
    const QSignalBlocker keypadBlocker(m_actionGroups.keypad);
    switch (m_keypadMode) {
    case Settings::KeypadModeBasicWide:
        m_actions.viewKeypadBasicWide->setChecked(true);
        break;
    case Settings::KeypadModeScientificWide:
        m_actions.viewKeypadScientificWide->setChecked(true);
        break;
    case Settings::KeypadModeScientificNarrow:
        m_actions.viewKeypadScientificNarrow->setChecked(true);
        break;
    case Settings::KeypadModeCustom:
        m_actions.viewKeypadCustom->setChecked(true);
        break;
    case Settings::KeypadModeDisabled:
    default:
        m_actions.viewKeypadDisabled->setChecked(true);
        break;
    }
}

void MainWindow::createStatusBar()
{
    StartupDiagnostics::Scope timing("Create status bar");
    QStatusBar* bar = statusBar();
    if (m_status.angleUnitSection != nullptr) {
        applyStatusBarAppearance(bar, generatedSurfaceColors(m_settings));
        bar->show();
        setStatusBarText();
        updateStatusBarSectionVisibility();
        return;
    }

    m_status.angleUnitSection = new QWidget(bar);
    m_status.resultFormatSection = new QWidget(bar);
    m_status.resultPrecisionSection = new QWidget(bar);

    m_status.angleUnitLabel = new QLabel(m_status.angleUnitSection);
    m_status.resultFormatLabel = new QLabel(m_status.resultFormatSection);
    m_status.resultPrecisionLabel = new QLabel(m_status.resultPrecisionSection);
    m_status.resultPrecisionSeparator = new QLabel(QStringLiteral("|"), bar);
    m_status.angleUnitSeparator = new QLabel(QStringLiteral("|"), bar);

    m_status.angleUnit = new QPushButton(bar);
    m_status.resultFormat = new QPushButton(bar);
    m_status.resultPrecision = new QPushButton(bar);

    m_status.angleUnit->setParent(m_status.angleUnitSection);
    m_status.resultFormat->setParent(m_status.resultFormatSection);
    m_status.resultPrecision->setParent(m_status.resultPrecisionSection);

    QHBoxLayout* angleLayout = new QHBoxLayout(m_status.angleUnitSection);
    QHBoxLayout* formatLayout = new QHBoxLayout(m_status.resultFormatSection);
    QHBoxLayout* precisionLayout = new QHBoxLayout(m_status.resultPrecisionSection);
    angleLayout->setContentsMargins(0, 0, 0, 0);
    formatLayout->setContentsMargins(0, 0, 0, 0);
    precisionLayout->setContentsMargins(0, 0, 0, 0);
    angleLayout->setSpacing(2);
    formatLayout->setSpacing(2);
    precisionLayout->setSpacing(2);
    angleLayout->addWidget(m_status.angleUnitLabel);
    angleLayout->addWidget(m_status.angleUnit);
    formatLayout->addWidget(m_status.resultFormatLabel);
    formatLayout->addWidget(m_status.resultFormat);
    precisionLayout->addWidget(m_status.resultPrecisionLabel);
    precisionLayout->addWidget(m_status.resultPrecision);

    QFont boldFont = m_status.angleUnitLabel->font();
    boldFont.setBold(true);
    m_status.angleUnitLabel->setFont(boldFont);
    m_status.resultFormatLabel->setFont(boldFont);
    m_status.resultPrecisionLabel->setFont(boldFont);
    m_status.resultPrecisionSeparator->setContentsMargins(2, 0, 2, 0);
    m_status.angleUnitSeparator->setContentsMargins(2, 0, 2, 0);
    m_status.angleUnitLabel->setCursor(Qt::PointingHandCursor);
    m_status.resultFormatLabel->setCursor(Qt::PointingHandCursor);
    m_status.resultPrecisionLabel->setCursor(Qt::PointingHandCursor);
    m_status.angleUnitLabel->installEventFilter(this);
    m_status.resultFormatLabel->installEventFilter(this);
    m_status.resultPrecisionLabel->installEventFilter(this);

    m_status.angleUnit->setCursor(Qt::PointingHandCursor);
    m_status.resultFormat->setCursor(Qt::PointingHandCursor);
    m_status.resultPrecision->setCursor(Qt::PointingHandCursor);

    m_status.angleUnit->setFocusPolicy(Qt::NoFocus);
    m_status.resultFormat->setFocusPolicy(Qt::NoFocus);
    m_status.resultPrecision->setFocusPolicy(Qt::NoFocus);

    m_status.angleUnit->setFlat(true);
    m_status.resultFormat->setFlat(true);
    m_status.resultPrecision->setFlat(true);

    m_status.angleUnit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_status.angleUnit, SIGNAL(customContextMenuRequested(const QPoint&)),
        SLOT(showAngleModeContextMenu(const QPoint&)));

    m_status.resultFormat->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_status.resultFormat, SIGNAL(customContextMenuRequested(const QPoint&)),
        SLOT(showResultFormatContextMenu(const QPoint&)));
    m_status.resultPrecision->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_status.resultPrecision, SIGNAL(customContextMenuRequested(const QPoint&)),
        SLOT(showPrecisionContextMenu(const QPoint&)));

    connect(m_status.angleUnit, &QPushButton::clicked, this, [this]() {
        showAngleModeContextMenu(QPoint(0, m_status.angleUnit->height()));
    });
    connect(m_status.resultFormat, &QPushButton::clicked, this, [this]() {
        showResultFormatContextMenu(QPoint(0, m_status.resultFormat->height()));
    });
    connect(m_status.resultPrecision, &QPushButton::clicked, this, [this]() {
        showPrecisionContextMenu(QPoint(0, m_status.resultPrecision->height()));
    });

    bar->addWidget(m_status.resultFormatSection);
    bar->addWidget(m_status.resultPrecisionSeparator);
    bar->addWidget(m_status.resultPrecisionSection);
    bar->addWidget(m_status.angleUnitSeparator);
    bar->addWidget(m_status.angleUnitSection);
    applyStatusBarAppearance(bar, generatedSurfaceColors(m_settings));
    bar->show();

    setStatusBarText();
    // When the status bar is recreated via View > Status Bar, geometry might not
    // be updated yet and width can be 0, which hides all sections. Recompute
    // once the event loop lays out the status bar.
    QTimer::singleShot(0, this, [this]() {
        updateStatusBarSectionVisibility();
    });
}

void MainWindow::createFixedWidgets()
{
    StartupDiagnostics::Scope timing("Create editor and result display");
    m_widgets.root = new QWidget(this);
    // Install parent backgrounds before controls inherit their styles.
    applyThemeBackgroundRoleToWidget(m_widgets.root,
                                     generatedSurfaceColors(m_settings).window.background);
    setCentralWidget(m_widgets.root);

    m_layouts.root = new QVBoxLayout(m_widgets.root);
    m_layouts.root->setSpacing(0);
    m_layouts.root->setContentsMargins(0, 0, 0, 0);

    m_widgets.splitContainer = new QSplitter(Qt::Horizontal, m_widgets.root);
    m_widgets.splitContainer->setObjectName(QStringLiteral("MainSplitContainer"));
    m_widgets.splitContainer->setChildrenCollapsible(false);
    m_widgets.splitContainer->setHandleWidth(UiConfig::SessionPaneSplitterWidth);
    applyThemeBackgroundRoleToWidget(m_widgets.splitContainer,
                                     generatedSurfaceColors(m_settings).window.background);
    updateSplitterStyleSheet();
    m_layouts.root->addWidget(m_widgets.splitContainer, 1);

    {
        StartupDiagnostics::Scope displayTiming("Construct initial result display");
        m_widgets.display = new ResultDisplay();
        m_widgets.display->setFrameStyle(QFrame::NoFrame);
    }
    {
        StartupDiagnostics::Scope editorTiming("Construct initial expression editor");
        m_widgets.editor = new Editor();
        m_widgets.editor->setFrameStyle(QFrame::NoFrame);
        m_widgets.editor->installEventFilter(this);
        m_widgets.editor->viewport()->installEventFilter(this);
    }
    {
        StartupDiagnostics::Scope paneTiming("Create and bind initial session pane");
        m_widgets.splitContainer->addWidget(createEditorDisplayPane(m_widgets.display, m_widgets.editor));
        m_paneSessionNames.insert(m_widgets.display, m_session ? m_session->name() : QString());
        m_paneSessionTabs.insert(m_widgets.display, QStringList(m_session ? m_session->name() : QString()));
        m_widgets.display->setSession(m_session);
        m_widgets.editor->setSession(m_session);
    }

    m_widgets.state = new QLabel(this);
    m_widgets.state->setObjectName(QStringLiteral("EditorStatePopup"));
    m_widgets.state->setPalette(QToolTip::palette());
    m_widgets.state->setAutoFillBackground(true);
    m_widgets.state->setFrameShape(QFrame::NoFrame);
    m_widgets.state->installEventFilter(this);
    m_widgets.stateCloseButton = new QPushButton(QStringLiteral("×"), m_widgets.state);
    m_widgets.stateCloseButton->setFocusPolicy(Qt::NoFocus);
    m_widgets.stateCloseButton->setFlat(true);
    m_widgets.stateCloseButton->setToolTip(tr("Close preview"));
    m_widgets.stateCloseButton->setStyleSheet(QStringLiteral(R"(
        QPushButton {
            border: none;
            background: transparent;
            padding: 0;
            margin: 0;
            outline: none;
        }

        QPushButton:hover {
            background: transparent;
        }

        QPushButton:pressed {
            background: transparent;
        }
    )"));
    connect(m_widgets.stateCloseButton, &QPushButton::clicked, this, &MainWindow::hideStateLabel);
    m_widgets.state->hide();
}

QWidget* MainWindow::createEditorDisplayPane(ResultDisplay* display, Editor* editor)
{
    connect(editor, &Editor::constantSearchVisibilityChanged,
            this, &MainWindow::updatePaneEditorCursorVisibility);

    const GeneratedThemeSurfaces surfaces = generatedSurfaceColors(m_settings);
    SessionPane* pane = new SessionPane(m_widgets.splitContainer);
    applyThemeBackgroundRoleToWidget(pane, surfaces.result.background);
    QVBoxLayout* layout = new QVBoxLayout(pane);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    QWidget* tabBarRow = new QWidget(pane);
    // The tab strip styles its parent. Give it the final row immediately so
    // construction does not install a temporary stylesheet on the whole pane.
    SessionTabBar* tabBar = new SessionTabBar(tabBarRow);
    tabBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    QHBoxLayout* tabBarRowLayout = new QHBoxLayout(tabBarRow);
    tabBarRowLayout->setSpacing(0);
    tabBarRowLayout->setContentsMargins(0, 0, 0, 0);
    tabBarRowLayout->addWidget(tabBar);
    tabBarRowLayout->addStretch(1);

    QStackedWidget* stack = new QStackedWidget(pane);
    applyThemeBackgroundRoleToWidget(stack, surfaces.result.background);
    QWidget* page = new QWidget(stack);
    applyThemeBackgroundRoleToWidget(page, surfaces.result.background);
    display->setThemeSurfaceColor(surfaces.result.background);
    const ThemeSurfaceColors resultToolTip =
        themeSurfaceForShadeIndex(surfaces, UiConfig::ResultTooltipBackgroundShade);
    const ThemeSurfaceColors resultToolTipOutline =
        themeSurfaceForShadeIndex(surfaces, UiConfig::ResultTooltipOutlineShade);
    display->setThemeToolTipColors(resultToolTip.background,
                                   resultToolTip.foreground,
                                   resultToolTipOutline.background);
    display->setThemeInteractionColors(surfaces.editorAndLists.background,
                                       surfaces.primary.background,
                                       surfaces.headersAndBorders.background,
                                       surfaces.headersAndBorders.foreground,
                                       surfaces.inputs.background,
                                       surfaces.inputs.foreground);
    display->rehighlight();
    editor->setThemeSurfaceColor(surfaces.editorAndLists.background,
                                 surfaces.result.background);
    const ThemeSurfaceColors completionPopup =
        themeSurfaceForShadeIndex(surfaces, UiConfig::CompletionPopupBackgroundShade);
    const ThemeSurfaceColors completionScrollbarThumb =
        themeSurfaceForShadeIndex(surfaces, UiConfig::CompletionPopupScrollbarThumbShade);
    const ThemeSurfaceColors completionSelectedRow =
        themeSurfaceForShadeIndex(surfaces, UiConfig::CompletionPopupSelectedRowShade);
    const ThemeSurfaceColors completionOutline =
        themeSurfaceForShadeIndex(surfaces, UiConfig::CompletionPopupOutlineShade);
    editor->setThemeCompletionColors(completionPopup.background,
                                     completionPopup.foreground,
                                     completionScrollbarThumb.background,
                                     completionScrollbarThumb.foreground,
                                     completionSelectedRow.background,
                                     completionSelectedRow.foreground,
                                     completionOutline.background,
                                     UiConfig::CompletionPopupCornerRadius,
                                     surfaces.primary.background,
                                     surfaces.primary.foreground);
    editor->rehighlight();
    QVBoxLayout* pageLayout = new QVBoxLayout(page);
    pageLayout->setSpacing(0);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->addWidget(display);
    pageLayout->addWidget(editor);
    stack->addWidget(page);

    layout->addWidget(tabBarRow);
    layout->addWidget(stack, 1);

    m_paneTabBars.insert(display, tabBar);
    m_tabBarDisplays.insert(tabBar, display);

    connect(tabBar, &QTabBar::currentChanged, this, [this, tabBar, display, editor](int index) {
        if (index < 0)
            return;
        pendingSessionTabActivationDisplay() = display;
        pendingSessionTabActivationEditor() = editor;
        setActiveEditorDisplayPane(display, editor, true);
        switchPaneToSession(display, tabBar->tabText(index));
        QPointer<ResultDisplay> pendingDisplay(display);
        QPointer<Editor> pendingEditor(editor);
        QTimer::singleShot(0, this, [pendingDisplay, pendingEditor]() {
            if (pendingSessionTabActivationDisplay() == pendingDisplay
                && pendingSessionTabActivationEditor() == pendingEditor) {
                pendingSessionTabActivationDisplay() = nullptr;
                pendingSessionTabActivationEditor() = nullptr;
            }
        });
        tabBar->refreshCloseButtons();
    });
    tabBar->tabActivated = [this, display, editor](const QString& sessionName) {
        pendingSessionTabActivationDisplay() = display;
        pendingSessionTabActivationEditor() = editor;
        setActiveEditorDisplayPane(display, editor, true);
        switchPaneToSession(display, sessionName);
        QPointer<ResultDisplay> pendingDisplay(display);
        QPointer<Editor> pendingEditor(editor);
        QTimer::singleShot(0, this, [pendingDisplay, pendingEditor]() {
            if (pendingSessionTabActivationDisplay() == pendingDisplay
                && pendingSessionTabActivationEditor() == pendingEditor) {
                pendingSessionTabActivationDisplay() = nullptr;
                pendingSessionTabActivationEditor() = nullptr;
            }
        });
    };
    connect(tabBar, &QTabBar::tabMoved, this, [this, tabBar, display](int, int) {
        QStringList names;
        for (int i = 0; i < tabBar->count(); ++i)
            names.append(tabBar->tabText(i));
        m_paneSessionTabs.insert(display, names);
        saveSessionLayout(false);
    });
    connect(tabBar, &QTabBar::tabBarDoubleClicked, this, [this, tabBar, display, editor](int index) {
        if (index < 0 || index >= tabBar->count())
            return;
        setActiveEditorDisplayPane(display, editor);
        switchPaneToSession(display, tabBar->tabText(index));
        showRenameSessionDialog();
    });
    tabBar->tabContextMenuRequested = [this, display, editor](const QString& sessionName, const QPoint& globalPos) {
        switchPaneToSession(display, sessionName);

        QMenu menu(this);
        QAction* renameSessionAction = menu.addAction(tr("Rename..."));
        QAction* duplicateSessionAction = menu.addAction(tr("Clone..."));
        QAction* convertPrivacyAction = menu.addAction(m_session->isPrivate()
            ? tr("Keep") : tr("Make Private..."));
        QAction* closeSessionAction = menu.addAction(tr("Close Tab"));
        menu.addSeparator();
        QAction* newSessionAction = menu.addAction(tr("New Tab"));
        QAction* newPrivateAction = menu.addAction(tr("New Private Tab"));
        QAction* openSessionAction = menu.addAction(tr("Open..."));
        QAction* reopenSessionAction = menu.addAction(tr("Reopen Closed Tab"));
        reopenSessionAction->setEnabled(!m_closedSessionTabs.isEmpty());
        menu.addSeparator();
        menu.addAction(m_actions.sessionImport);
        QMenu* exportMenu = menu.addMenu(m_menus.sessionExport->title());
        exportMenu->addAction(m_actions.sessionExportJson);
        exportMenu->addAction(m_actions.sessionExportPlainText);
        exportMenu->addAction(m_actions.sessionExportHtml);
        menu.addSeparator();
        QMenu* splitMenu = menu.addMenu(tr("Split"));
        QAction* splitLeftAction = splitMenu->addAction(tr("Left"));
        QAction* splitRightAction = splitMenu->addAction(tr("Right"));
        QAction* splitUpAction = splitMenu->addAction(tr("Up"));
        QAction* splitDownAction = splitMenu->addAction(tr("Down"));
        QAction* closePaneAction = menu.addAction(tr("Close Pane"));
        menu.addSeparator();
        QAction* clearSessionAction = menu.addAction(tr("Clear"));
        QAction* deleteSessionAction = menu.addAction(tr("Delete"));

        MenuStyle::apply(&menu);
        QAction* selectedAction = menu.exec(globalPos);
        if (selectedAction == nullptr)
            return;

        setActiveEditorDisplayPane(display, editor);
        if (selectedAction == newSessionAction)
            showNewSessionDialog();
        else if (selectedAction == newPrivateAction)
            showNewPrivateSession();
        else if (selectedAction == convertPrivacyAction)
            convertSessionPrivacy();
        else if (selectedAction == openSessionAction)
            showOpenSessionDialog();
        else if (selectedAction == reopenSessionAction)
            restoreClosedSessionTab();
        else if (selectedAction == duplicateSessionAction)
            showDuplicateSessionDialog();
        else if (selectedAction == splitLeftAction)
            splitActivePaneLeft();
        else if (selectedAction == splitRightAction)
            splitActivePaneRight();
        else if (selectedAction == splitUpAction)
            splitActivePaneUp();
        else if (selectedAction == splitDownAction)
            splitActivePaneDown();
        else if (selectedAction == renameSessionAction)
            showRenameSessionDialog();
        else if (selectedAction == clearSessionAction)
            clearSession();
        else if (selectedAction == closeSessionAction)
            closeCurrentSession();
        else if (selectedAction == closePaneAction)
            closeCurrentPane();
        else if (selectedAction == deleteSessionAction)
            deleteCurrentSession();
    };
    tabBar->sessionTabDropped = [this, tabBar](SessionTabBar* sourceTabBar, const QString& sessionName, int targetIndex) {
        moveSessionTab(sourceTabBar, tabBar, sessionName, targetIndex);
    };
    tabBar->sourceMainWindow = [this]() { return this; };
    tabBar->sessionTabDetached = [this, tabBar](const QString& sessionName, const QPoint& globalPos) {
        if (sessionName.isEmpty())
            return;
        ResultDisplay* sourceDisplay = tabBarDisplay(tabBar);
        if (sourceDisplay == nullptr)
            return;
        if (!paneSessionNames(sourceDisplay).contains(sessionName, Qt::CaseInsensitive))
            return;

        const QString previousLayoutJson = m_settings->sessionLayoutJson;
        m_settings->sessionLayoutJson.clear();
        MainWindow* detachedWindow = new MainWindow(false);
        m_settings->sessionLayoutJson = previousLayoutJson;
        detachedWindow->show();
        detachedWindow->move(globalPos - QPoint(detachedWindow->width() / 4, 18));
        Session* sourceSession = m_loadedSessions.value(sessionName, nullptr);
        if (sourceSession == nullptr)
            return;

        const QStringList detachedNames = detachedWindow->m_loadedSessions.keys();
        for (const QString& name : detachedNames) {
            Session* sessionToDelete = detachedWindow->m_loadedSessions.take(name);
            detachedWindow->m_sessionViewportAnchors.remove(name);
            detachedWindow->m_sessionScrollValues.remove(name);
            if (sessionToDelete == detachedWindow->m_session)
                detachedWindow->m_session = nullptr;
            if (sessionToDelete != nullptr)
                delete sessionToDelete;
        }
        Session* movedSession = new Session(*sourceSession);
        movedSession->setName(sessionName);
        detachedWindow->m_loadedSessions.insert(sessionName, movedSession);
        detachedWindow->applyUserDefinitions();
        detachedWindow->m_paneSessionTabs.insert(detachedWindow->m_widgets.display, QStringList(sessionName));
        detachedWindow->m_paneSessionNames.insert(detachedWindow->m_widgets.display, sessionName);
        detachedWindow->m_widgets.display->setSession(movedSession);
        detachedWindow->activateSession(movedSession);
        detachedWindow->updatePaneLoadedSessionCounts();
        detachedWindow->updatePaneTabBars();

        removeSessionTabFromPane(sourceDisplay, sessionName, true);
        const auto referencedByRemainingPanes = [this](const QString& name) {
            for (ResultDisplay* display : splitPaneDisplays()) {
                if (paneSessionNames(display).contains(name, Qt::CaseInsensitive))
                    return true;
            }
            return false;
        };
        if (!referencedByRemainingPanes(sessionName)) {
            Session* sessionToDelete = m_loadedSessions.take(sessionName);
            m_sessionViewportAnchors.remove(sessionName);
            m_sessionScrollValues.remove(sessionName);
            if (sessionToDelete != nullptr && sessionToDelete != m_session)
                delete sessionToDelete;
        }
        updatePaneLoadedSessionCounts();
        updatePaneTabBars();
        detachedWindow->saveSessionLayout(false);
        saveSessionLayout(false);
    };
    tabBar->tabCloseRequested = [this, display](const QString& sessionName) {
        switchPaneToSession(display, sessionName);
        closeCurrentSession();
    };
    pane->sessionTabDroppedOnPane = [this, display](SessionTabBar* sourceTabBar, const QString& sessionName, const QPoint& panePos) {
        moveSessionTabToPane(sourceTabBar, display, sessionName, panePos);
    };
    pane->shouldShowOverlayForDrag = [this, display](SessionTabBar* sourceTabBar) {
        ResultDisplay* sourceDisplay = tabBarDisplay(sourceTabBar);
        if (sourceDisplay == nullptr || sourceDisplay != display)
            return true;
        return paneSessionNames(sourceDisplay).size() > 1;
    };
    pane->paneActivated = [this, display, editor]() {
        setActiveEditorDisplayPane(display, editor);
    };
    pane->watchDropTarget(stack);
    pane->setOverlayAreaWidget(stack);
    pane->watchDropTarget(page);
    pane->watchDropTarget(display);
    pane->watchDropTarget(display->viewport());
    pane->watchDropTarget(editor);
    pane->watchDropTarget(editor->viewport());

    return pane;
}

void MainWindow::setActiveEditorDisplayPane(ResultDisplay* display, Editor* editor, bool forceEditorFocus)
{
    if (display == nullptr || editor == nullptr)
        return;

    if (forceEditorFocus) {
        cancelWindowActivationRestore();
        setPendingDockTextInputFocusTarget(nullptr);
    }

    const bool dockWidgetHasFocus = !forceEditorFocus
        && pendingDockFocusTarget() != nullptr;
    const bool dockTextInputHasFocus = !forceEditorFocus
        && (isDockTextInput(QApplication::focusWidget())
            || pendingDockTextInputFocusTarget() != nullptr);
    if (dockWidgetHasFocus || dockTextInputHasFocus) {
        globallyActiveDisplay() = nullptr;
        globallyActiveEditor() = nullptr;
        return;
    } else {
        globallyActiveDisplay() = display;
        globallyActiveEditor() = editor;
    }
    if (activeEditorDisplayPaneActivationInProgress())
        return;

    activeEditorDisplayPaneActivationInProgress() = true;
    struct ActivationGuard {
        ~ActivationGuard()
        {
            activeEditorDisplayPaneActivationInProgress() = false;
        }
    } activationGuard;

    if (m_widgets.display != display || m_widgets.editor != editor) {
        const QString sessionName = m_paneSessionNames.value(display);
        Session* paneSession = m_loadedSessions.value(sessionName, nullptr);
        const bool switchingSession = paneSession != nullptr && paneSession != m_session;
        const bool previousSessionIsLoaded =
            m_session != nullptr && m_loadedSessions.values().contains(m_session);
        if (previousSessionIsLoaded) {
            if (m_sessionSavePending)
                flushPendingSessionSave();
            captureEditorTextInCurrentSession();
            if (m_widgets.display != nullptr
                    && m_paneSessionNames.value(m_widgets.display) == m_session->name()) {
                m_sessionViewportAnchors.insert(m_session->name(), m_widgets.display->viewportTopAnchor());
                QScrollBar* bar = m_widgets.display->verticalScrollBar();
                const int scrollValue = bar->value() == bar->maximum()
                    ? (std::numeric_limits<int>::max)()
                    : bar->value();
                m_sessionScrollValues.insert(m_session->name(), scrollValue);
            }
        } else {
            captureEditorTextInCurrentSession();
        }
        if (switchingSession)
            m_session = nullptr;

        m_widgets.display = display;
        m_widgets.editor = editor;
        // Selection signals can set the copy source before activating a pane.
        // Keep that source when it belongs to the pane being activated.
        if (m_copyWidget != display && m_copyWidget != editor)
            m_copyWidget = editor;

        if (paneSession != nullptr && paneSession != m_session)
            activateSession(paneSession);
    }
    if (!dockWidgetHasFocus && !dockTextInputHasFocus
        && QApplication::activePopupWidget() == nullptr) {
        editor->setFocus(Qt::OtherFocusReason);
        QPointer<Editor> editorGuard(editor);
        QTimer::singleShot(0, editor, [editorGuard]() {
            if (editorGuard != nullptr
                && globallyActiveEditor() == editorGuard
                && pendingDockFocusTarget() == nullptr
                && pendingDockTextInputFocusTarget() == nullptr
                && QApplication::activePopupWidget() == nullptr) {
                editorGuard->setFocus(Qt::OtherFocusReason);
            }
        });
    }
    updatePaneEditorCursorVisibility();
    updateActiveSessionPaneTabColor();
    syncEditMenuActionState();
    if (!dockWidgetHasFocus && !dockTextInputHasFocus && m_widgets.state != nullptr && m_widgets.state->isVisible())
        showStateLabel(m_widgets.state->text());
}

bool MainWindow::isDockTextInput(QWidget* widget) const
{
    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(widget);
    if (lineEdit == nullptr)
        return false;
    if (lineEdit->property("speedcrunchDockTextInput").toBool())
        return true;

    for (QObject* ancestor = widget; ancestor != nullptr; ancestor = ancestor->parent()) {
        QDockWidget* dock = qobject_cast<QDockWidget*>(ancestor);
        if (dock != nullptr && m_allDocks.contains(dock))
            return true;
    }

    for (QDockWidget* dock : m_allDocks) {
        if (dock != nullptr && dock->findChildren<QLineEdit*>().contains(lineEdit))
            return true;
    }

    return false;
}

bool MainWindow::isDockWidgetDescendant(QWidget* widget) const
{
    return dockWidgetForDescendant(widget) != nullptr;
}

QDockWidget* MainWindow::dockWidgetForDescendant(QWidget* widget) const
{
    if (widget == nullptr)
        return nullptr;
    for (QObject* ancestor = widget; ancestor != nullptr; ancestor = ancestor->parent()) {
        QDockWidget* dock = qobject_cast<QDockWidget*>(ancestor);
        if (dock != nullptr && m_allDocks.contains(dock))
            return dock;
    }

    return nullptr;
}

QAbstractItemView* MainWindow::dockItemViewFocusTarget(QWidget* widget) const
{
    if (widget == nullptr)
        return nullptr;

    const auto usableDockView = [this](QAbstractItemView* view) -> QAbstractItemView* {
        if (view == nullptr)
            return nullptr;
        if (!isDockWidgetDescendant(view))
            return nullptr;
        if (view->model() == nullptr || view->model()->rowCount() <= 0)
            return nullptr;
        return view;
    };

    for (QWidget* candidate = widget; candidate != nullptr; candidate = candidate->parentWidget()) {
        if (QAbstractItemView* view = usableDockView(qobject_cast<QAbstractItemView*>(candidate)))
            return view;
    }

    const QPoint globalPos = QCursor::pos();
    for (QDockWidget* dock : m_allDocks) {
        if (dock == nullptr || !dock->isVisible())
            continue;
        if (!dock->rect().contains(dock->mapFromGlobal(globalPos)))
            continue;
        for (QAbstractItemView* view : dock->findChildren<QAbstractItemView*>()) {
            if (!view->isVisible())
                continue;
            if (!view->rect().contains(view->mapFromGlobal(globalPos)))
                continue;
            if (QAbstractItemView* usableView = usableDockView(view))
                return usableView;
        }
    }

    return nullptr;
}

QList<QWidget*> MainWindow::focusCycleTargets() const
{
    QList<QWidget*> targets;
    QSet<QWidget*> seen;

    const auto addTarget = [&targets, &seen, this](QWidget* widget) {
        if (widget == nullptr || seen.contains(widget))
            return;
        if (!widget->isEnabled() || !widget->isVisibleTo(const_cast<MainWindow*>(this)))
            return;
        if (widget->focusPolicy() == Qt::NoFocus)
            return;
        targets.append(widget);
        seen.insert(widget);
    };

    for (Editor* editor : splitPaneEditors())
        addTarget(editor);

    for (QDockWidget* dock : m_allDocks) {
        if (dock == nullptr || !dock->isEnabled() || !dock->isVisibleTo(const_cast<MainWindow*>(this)))
            continue;
        QWidget* dockWidget = dock->widget();
        if (dockWidget == nullptr || !dockWidget->isEnabled() || !dockWidget->isVisibleTo(const_cast<MainWindow*>(this)))
            continue;

        const QList<QLineEdit*> textInputs = dockWidget->findChildren<QLineEdit*>();
        for (QLineEdit* textInput : textInputs)
            addTarget(textInput);

        const QList<QAbstractItemView*> itemViews = dockWidget->findChildren<QAbstractItemView*>();
        for (QAbstractItemView* itemView : itemViews)
            addTarget(itemView);
    }

    return targets;
}

bool MainWindow::focusWidgetMatchesCycleTarget(QWidget* focusWidget, QWidget* target) const
{
    if (focusWidget == nullptr || target == nullptr)
        return false;
    if (focusWidget == target)
        return true;
    if (target->isAncestorOf(focusWidget))
        return true;

    if (QAbstractItemView* itemView = qobject_cast<QAbstractItemView*>(target)) {
        QWidget* viewport = itemView->viewport();
        return focusWidget == viewport
            || (viewport != nullptr && viewport->isAncestorOf(focusWidget));
    }

    return false;
}

void MainWindow::cycleFocusRegion(int direction)
{
    const QList<QWidget*> targets = focusCycleTargets();
    if (targets.size() <= 1)
        return;

    // F6 expresses a newer focus choice than the delayed replay scheduled by
    // window activation. Do not let that replay pull focus back to the editor.
    cancelWindowActivationRestore();

    QWidget* focused = QApplication::focusWidget();
    if (focused == nullptr
        || (focused->window() != this && !isDockWidgetDescendant(focused)))
        focused = focusWidget();

    int currentIndex = -1;
    for (int i = 0; i < targets.size(); ++i) {
        if (focusWidgetMatchesCycleTarget(focused, targets.at(i))) {
            currentIndex = i;
            break;
        }
    }

    const int targetIndex = currentIndex < 0
        ? (direction >= 0 ? 0 : targets.size() - 1)
        : (currentIndex + direction + targets.size()) % targets.size();
    QWidget* target = targets.at(targetIndex);

    setPendingDockFocusTarget(nullptr);
    setPendingDockTextInputFocusTarget(nullptr);
    if (isDockWidgetDescendant(target)) {
        const int focusGeneration = setPendingDockFocusTarget(target);
        int textInputGeneration = pendingDockTextInputFocusTargetGeneration();
        if (isDockTextInput(target))
            textInputGeneration = setPendingDockTextInputFocusTarget(target);
        deactivateActiveEditorForTextInputFocus();
        hideStateLabel();
        QPointer<QWidget> focusTarget(target);
        QTimer::singleShot(100, target, [focusTarget, focusGeneration, textInputGeneration]() {
            if (pendingDockFocusTargetGeneration() == focusGeneration
                && pendingDockFocusTarget() == focusTarget) {
                setPendingDockFocusTarget(nullptr);
            }
            if (pendingDockTextInputFocusTargetGeneration() == textInputGeneration
                && pendingDockTextInputFocusTarget() == focusTarget) {
                setPendingDockTextInputFocusTarget(nullptr);
            }
        });
    }
    // Floating docks are separate top-level windows; activate the destination
    // before asking one of its children to take focus.
    if (!target->window()->isActiveWindow())
        target->window()->activateWindow();
    target->setFocus(Qt::ShortcutFocusReason);
}

void MainWindow::cycleFocusForward()
{
    cycleFocusRegion(1);
}

void MainWindow::cycleFocusBackward()
{
    cycleFocusRegion(-1);
}

void MainWindow::deactivateActiveEditorForTextInputFocus()
{
    // Dock search fields and other dock text inputs are peers of the expression
    // editor. The focus event is already moving to the dock input here, so this
    // clears only the app-level active-editor state and repaint flags. Calling
    // clearFocus() on the previous editor during the transfer can make Qt fall
    // back to that editor, which would leave two text inputs competing for the
    // primary outline.
    QPointer<Editor> previousActiveEditor = globallyActiveEditor();
    if (previousActiveEditor == nullptr)
        previousActiveEditor = m_widgets.editor;
    if (previousActiveEditor == nullptr && globallyActiveDisplay() == nullptr)
        return;

    DockTextInputFocusTransferGuard focusTransferGuard;

    globallyActiveEditor() = nullptr;
    globallyActiveDisplay() = nullptr;

    if (previousActiveEditor != nullptr) {
        previousActiveEditor->setCustomCursorVisible(false);
        previousActiveEditor->setThemePrimaryColor(
            generatedSurfaceColors(m_settings).primary.background,
            false);
    }
}

void MainWindow::handleApplicationFocusChanged(QWidget* previous, QWidget* focused)
{
    if (!allMainWindows().contains(QPointer<MainWindow>(this)))
        return;
    if (focused != nullptr) {
        if (focused->window() == this || isDockWidgetDescendant(focused))
            lastFocusWidgetInActiveWindow() = focused;
    } else if (previous != nullptr
               && (previous->window() == this || isDockWidgetDescendant(previous))) {
        lastFocusWidgetInActiveWindow() = previous;
    }

    syncEditMenuActionState();
    syncSessionMenuActionState();
    if (!isDockTextInput(focused))
        return;

    const int textInputGeneration = setPendingDockTextInputFocusTarget(focused);
    QPointer<QWidget> textInput(focused);
    QTimer::singleShot(100, focused, [textInput, textInputGeneration]() {
        if (pendingDockTextInputFocusTargetGeneration() == textInputGeneration
            && pendingDockTextInputFocusTarget() == textInput) {
            setPendingDockTextInputFocusTarget(nullptr);
        }
    });
    deactivateActiveEditorForTextInputFocus();
}

void MainWindow::configureEditorDisplayPane(ResultDisplay* display, Editor* editor)
{
    if (display == nullptr || editor == nullptr)
        return;

    editor->installEventFilter(this);
    editor->viewport()->installEventFilter(this);

    connect(editor, &Editor::textChanged, this, [this, display, editor]() {
        // Programmatic text changes in an inactive pane must not make that pane
        // active. Real editor input activates the pane through the mouse/key/input
        // event path before the text changes.
        if (editor == m_widgets.editor
            || editor->hasFocus()
            || editor->viewport()->hasFocus()) {
            setActiveEditorDisplayPane(display, editor);
        }
    });
    connect(editor, &Editor::returnPressed, this, [this, display, editor]() {
        setActiveEditorDisplayPane(display, editor);
        evaluateEditorExpression();
    });
    connect(editor, &Editor::bulkEvaluationStarted, this, &MainWindow::handleBulkEvaluationStarted);
    connect(editor, &Editor::bulkEvaluationFinished, this, &MainWindow::handleBulkEvaluationFinished);
    connect(editor, &Editor::escapePressed, this, [this, display, editor]() {
        setActiveEditorDisplayPane(display, editor);
        handleEditorEscapePressed();
    });
    connect(editor, &Editor::selectionChanged, this, [this, display, editor]() {
        setActiveEditorDisplayPane(display, editor);
        handleEditorSelectionChange();
    });
    connect(editor, &Editor::autoCalcDisabled, this, [this, editor]() {
        if (editor == m_widgets.editor)
            hideStateLabel();
    });
    connect(editor, &Editor::autoCalcMessageAvailable, this, &MainWindow::handleAutoCalcMessageAvailable);
    connect(editor, &Editor::autoCalcQuantityAvailable, this, &MainWindow::handleAutoCalcQuantityAvailable);
    connect(editor, &Editor::shiftDownPressed, this, &MainWindow::decreaseDisplayFontPointSize);
    connect(editor, &Editor::shiftUpPressed, this, &MainWindow::increaseDisplayFontPointSize);
    connect(editor, &Editor::controlPageUpPressed, display, &ResultDisplay::scrollToTop);
    connect(editor, &Editor::controlPageDownPressed, display, &ResultDisplay::scrollToBottom);
    connect(editor, &Editor::shiftPageUpPressed, display, &ResultDisplay::scrollLineUp);
    connect(editor, &Editor::shiftPageDownPressed, display, &ResultDisplay::scrollLineDown);
    connect(editor, &Editor::pageUpPressed, display, &ResultDisplay::scrollPageUp);
    connect(editor, &Editor::pageDownPressed, display, &ResultDisplay::scrollPageDown);
    connect(editor, &Editor::textChanged, this, [this, editor]() {
        if (editor == m_widgets.editor
            || editor->hasFocus()
            || editor->viewport()->hasFocus()) {
            handleEditorTextChange();
        }
    });
    connect(editor, &Editor::copyAvailable, this, &MainWindow::handleCopyAvailable);
    connect(editor, &Editor::undoAvailable, this, &MainWindow::syncEditMenuActionState);
    connect(editor, &Editor::redoAvailable, this, &MainWindow::syncEditMenuActionState);
    connect(editor, &Editor::copySequencePressed, this, &MainWindow::copy);
    connect(this, &MainWindow::historyChanged, editor, &Editor::updateHistory);

    connect(display, &ResultDisplay::clicked, this, [this, display, editor]() {
        setActiveEditorDisplayPane(display, editor, true);
        hideStateLabel();
    });
    connect(display, &ResultDisplay::copyAvailable, this, &MainWindow::handleCopyAvailable);
    connect(display, &ResultDisplay::expressionSelected, this, [this, display, editor](const QString& text) {
        setActiveEditorDisplayPane(display, editor, true);
        insertHistoryExpressionIntoEditor(text);
    });
    connect(display, &ResultDisplay::editHistoryEntryRequested, this, &MainWindow::startHistoryEntryEdit);
    connect(display, &ResultDisplay::editHistoryEntryContextRequested, this, &MainWindow::editHistoryEntryContext);
    connect(display, &ResultDisplay::cancelHistoryEditRequested, this, &MainWindow::cancelHistoryEntryEdit);
    connect(display, &ResultDisplay::removeHistoryEntryRequested, this, &MainWindow::removeHistoryEntryAt);
    connect(display, &ResultDisplay::removeHistoryEntriesAboveRequested, this, &MainWindow::removeHistoryEntriesAbove);
    connect(display, &ResultDisplay::removeHistoryEntriesBelowRequested, this, &MainWindow::removeHistoryEntriesBelow);
    connect(display, &ResultDisplay::newSessionRequested, this, &MainWindow::showNewSessionDialog);
    connect(display, &ResultDisplay::newPrivateSessionRequested, this, &MainWindow::showNewPrivateSession);
    connect(display, &ResultDisplay::convertSessionPrivacyRequested, this, &MainWindow::convertSessionPrivacy);
    connect(display, &ResultDisplay::openSessionRequested, this, &MainWindow::showOpenSessionDialog);
    connect(display, &ResultDisplay::importSessionRequested, this, &MainWindow::showSessionImportDialog);
    connect(display, &ResultDisplay::exportSessionJsonRequested, this, &MainWindow::exportJson);
    connect(display, &ResultDisplay::exportSessionPlainTextRequested, this, &MainWindow::exportPlainText);
    connect(display, &ResultDisplay::exportSessionHtmlRequested, this, &MainWindow::exportHtml);
    connect(display, &ResultDisplay::duplicateSessionRequested, this, &MainWindow::showDuplicateSessionDialog);
    connect(display, &ResultDisplay::splitLeftRequested, this, &MainWindow::splitActivePaneLeft);
    connect(display, &ResultDisplay::splitRightRequested, this, &MainWindow::splitActivePaneRight);
    connect(display, &ResultDisplay::splitUpRequested, this, &MainWindow::splitActivePaneUp);
    connect(display, &ResultDisplay::splitDownRequested, this, &MainWindow::splitActivePaneDown);
    connect(display, &ResultDisplay::renameSessionRequested, this, &MainWindow::showRenameSessionDialog);
    connect(display, &ResultDisplay::clearSessionRequested, this, &MainWindow::clearSession);
    connect(display, &ResultDisplay::closeSessionRequested, this, &MainWindow::closeCurrentSession);
    connect(display, &ResultDisplay::closePaneRequested, this, &MainWindow::closeCurrentPane);
    connect(display, &ResultDisplay::deleteSessionRequested, this, &MainWindow::deleteCurrentSession);
    connect(display, &ResultDisplay::loadedSessionsMenuRequested, this, &MainWindow::showLoadedSessionsMenu);
    connect(display, &ResultDisplay::selectionChanged, this, [this, display, editor]() {
        setActiveEditorDisplayPane(display, editor, true);
        handleDisplaySelectionChange();
    });
    connect(display, &ResultDisplay::shiftWheelUp, this, &MainWindow::increaseDisplayFontPointSize);
    connect(display, &ResultDisplay::shiftWheelDown, this, &MainWindow::decreaseDisplayFontPointSize);
    connect(display, &ResultDisplay::controlWheelUp, this, &MainWindow::increaseDisplayFontPointSize);
    connect(display, &ResultDisplay::controlWheelDown, this, &MainWindow::decreaseDisplayFontPointSize);
    connect(display, &ResultDisplay::shiftControlWheelDown, this, &MainWindow::decreaseOpacity);
    connect(display, &ResultDisplay::shiftControlWheelUp, this, &MainWindow::increaseOpacity);
    connect(this, &MainWindow::historyChanged, display, &ResultDisplay::refresh);
    connect(this, &MainWindow::radixCharacterChanged, display, &ResultDisplay::refresh);
    connect(this, &MainWindow::radixCharacterChanged, editor, &Editor::refreshAutoCalc);
    connect(this, &MainWindow::angleUnitChanged, editor, &Editor::refreshAutoCalc);
    connect(this, &MainWindow::complexNumbersChanged, display, &ResultDisplay::refreshLastHistoryEntry);
    connect(this, &MainWindow::complexNumbersChanged, editor, &Editor::refreshAutoCalc);
    connect(this, &MainWindow::resultFormatChanged, display, &ResultDisplay::refreshLastHistoryEntry);
    connect(this, &MainWindow::resultFormatChanged, editor, &Editor::refreshAutoCalc);
    connect(this, &MainWindow::resultPrecisionChanged, display, &ResultDisplay::refreshLastHistoryEntry);
    connect(this, &MainWindow::resultPrecisionChanged, editor, &Editor::refreshAutoCalc);
    connect(this, &MainWindow::resultRoundingModeChanged, display, &ResultDisplay::refreshLastHistoryEntry);
    connect(this, &MainWindow::resultRoundingModeChanged, editor, &Editor::refreshAutoCalc);
    connect(this, &MainWindow::colorSchemeChanged, display, &ResultDisplay::rehighlight);
    connect(this, &MainWindow::colorSchemeChanged, editor, &Editor::rehighlight);
    connect(this, &MainWindow::syntaxHighlightingChanged, display, &ResultDisplay::rehighlight);
    connect(this, &MainWindow::syntaxHighlightingChanged, editor, &Editor::rehighlight);
}

void MainWindow::splitActivePane(Qt::Orientation orientation, bool insertAfter)
{
    if (m_widgets.splitContainer == nullptr || m_widgets.display == nullptr || m_widgets.editor == nullptr)
        return;

    QWidget* activePane = paneWidgetForDisplay(m_widgets.display);
    QSplitter* parentSplitter = qobject_cast<QSplitter*>(activePane ? activePane->parentWidget() : nullptr);
    if (parentSplitter == nullptr)
        return;

    const int activeIndex = parentSplitter->indexOf(activePane);
    if (activeIndex < 0)
        return;
    const QList<int> parentSizesBefore = parentSplitter->sizes();
    const int activeSize = activeIndex < parentSizesBefore.size()
        ? parentSizesBefore.at(activeIndex)
        : qMax(1, orientation == Qt::Horizontal ? activePane->width() : activePane->height());

    ResultDisplay* display = new ResultDisplay();
    display->setFrameStyle(QFrame::NoFrame);
    display->setFont(m_widgets.display->font());
    display->setHoverHighlightEnabled(m_settings->hoverHighlightResults);
    display->setLoadedSessionCount(1);
    display->rehighlight();

    Editor* editor = new Editor();
    editor->setFrameStyle(QFrame::NoFrame);
    editor->setFont(m_widgets.editor->font());
    editor->setAutoCalcEnabled(m_settings->autoCalc);
    editor->setAutoCompletionEnabled(m_settings->autoCompletion);
    editor->rehighlight();

    Session* newSession = createUntitledSession(false);
    editor->setText(QString());
    editor->setCursorPosition(editor->text().size());

    QWidget* pane = createEditorDisplayPane(display, editor);
    configureEditorDisplayPane(display, editor);

    QSplitter* targetSplitter = parentSplitter;
    if (parentSplitter->orientation() != orientation) {
        QSplitter* nestedSplitter = new QSplitter(orientation);
        nestedSplitter->setChildrenCollapsible(false);
        nestedSplitter->setHandleWidth(UiConfig::SessionPaneSplitterWidth);
        nestedSplitter->setStyleSheet(m_widgets.splitContainer->styleSheet());
        activePane->setParent(nullptr);
        parentSplitter->insertWidget(activeIndex, nestedSplitter);
        if (insertAfter) {
            nestedSplitter->addWidget(activePane);
            nestedSplitter->addWidget(pane);
        } else {
            nestedSplitter->addWidget(pane);
            nestedSplitter->addWidget(activePane);
        }
        targetSplitter = nestedSplitter;
        if (parentSizesBefore.size() == parentSplitter->count())
            parentSplitter->setSizes(parentSizesBefore);
    } else {
        const int insertIndex = insertAfter ? activeIndex + 1 : activeIndex;
        parentSplitter->insertWidget(insertIndex, pane);
    }
    if (newSession != nullptr)
        m_paneSessionNames.insert(display, newSession->name());
    if (newSession != nullptr)
        m_paneSessionTabs.insert(display, QStringList(newSession->name()));
    display->setSession(newSession);
    if (newSession != nullptr)
        editor->setSession(newSession);

    const int firstHalf = qMax(1, activeSize / 2);
    const int secondHalf = qMax(1, activeSize - firstHalf);
    if (targetSplitter == parentSplitter) {
        QList<int> sizes = parentSizesBefore;
        if (activeIndex < sizes.size()) {
            sizes[activeIndex] = insertAfter ? firstHalf : secondHalf;
            sizes.insert(insertAfter ? activeIndex + 1 : activeIndex,
                         insertAfter ? secondHalf : firstHalf);
            if (sizes.size() == targetSplitter->count())
                targetSplitter->setSizes(sizes);
        }
    } else {
        targetSplitter->setSizes(insertAfter
            ? QList<int>({ firstHalf, secondHalf })
            : QList<int>({ firstHalf, secondHalf }));
    }

    display->refresh();
    editor->updateHistory();
    editor->refreshAutoCalc();
    updateSplitterStyleSheet();
    updatePaneLoadedSessionCounts();
    cancelWindowActivationRestore();
    setActiveEditorDisplayPane(display, editor);
    saveSessionLayout();
}

void MainWindow::splitActivePaneLeft()
{
    splitActivePane(Qt::Horizontal, false);
}

void MainWindow::splitActivePaneRight()
{
    splitActivePane(Qt::Horizontal, true);
}

void MainWindow::splitActivePaneUp()
{
    splitActivePane(Qt::Vertical, false);
}

void MainWindow::splitActivePaneDown()
{
    splitActivePane(Qt::Vertical, true);
}

void MainWindow::activateNextChild()
{
    if (m_widgets.display == nullptr)
        return;

    QTabBar* currentTabBar = displayTabBar(m_widgets.display);
    if (currentTabBar != nullptr && currentTabBar->count() > 0) {
        const int currentIndex = currentTabBar->currentIndex();
        if (currentIndex >= 0 && currentIndex + 1 < currentTabBar->count()) {
            currentTabBar->setCurrentIndex(currentIndex + 1);
            return;
        }
    }

    const QList<ResultDisplay*> displays = splitPaneDisplays();
    const int currentPaneIndex = displays.indexOf(m_widgets.display);
    if (currentPaneIndex < 0 || currentPaneIndex + 1 >= displays.size())
        return;

    ResultDisplay* nextDisplay = displays.at(currentPaneIndex + 1);
    QWidget* page = nextDisplay->parentWidget();
    Editor* nextEditor = page ? page->findChild<Editor*>(QString(), Qt::FindDirectChildrenOnly) : nullptr;
    if (nextEditor == nullptr)
        return;

    setActiveEditorDisplayPane(nextDisplay, nextEditor);
    const QString sessionName = m_paneSessionNames.value(nextDisplay);
    if (!sessionName.isEmpty())
        switchPaneToSession(nextDisplay, sessionName);
}

void MainWindow::activatePreviousChild()
{
    if (m_widgets.display == nullptr)
        return;

    QTabBar* currentTabBar = displayTabBar(m_widgets.display);
    if (currentTabBar != nullptr && currentTabBar->count() > 0) {
        const int currentIndex = currentTabBar->currentIndex();
        if (currentIndex > 0) {
            currentTabBar->setCurrentIndex(currentIndex - 1);
            return;
        }
    }

    const QList<ResultDisplay*> displays = splitPaneDisplays();
    const int currentPaneIndex = displays.indexOf(m_widgets.display);
    if (currentPaneIndex <= 0)
        return;

    ResultDisplay* previousDisplay = displays.at(currentPaneIndex - 1);
    QWidget* page = previousDisplay->parentWidget();
    Editor* previousEditor = page ? page->findChild<Editor*>(QString(), Qt::FindDirectChildrenOnly) : nullptr;
    if (previousEditor == nullptr)
        return;

    setActiveEditorDisplayPane(previousDisplay, previousEditor);
    const QString sessionName = m_paneSessionNames.value(previousDisplay);
    if (!sessionName.isEmpty())
        switchPaneToSession(previousDisplay, sessionName);
}

QString MainWindow::firstAvailableUntitledSessionNameAcrossWindows(const MainWindow* ignoredWindow) const
{
    QHash<QString, Session*> sessions;
    for (const QPointer<MainWindow>& ptr : allMainWindows()) {
        MainWindow* window = ptr.data();
        if (window == nullptr || window == ignoredWindow)
            continue;
        for (auto it = window->m_loadedSessions.constBegin(); it != window->m_loadedSessions.constEnd(); ++it)
            sessions.insert(it.key(), it.value());
    }
    return firstAvailableUntitledSessionName(sessions);
}

void MainWindow::copyWindowLayoutFrom(const MainWindow* source)
{
    if (source == nullptr)
        return;

    restoreDockColumnOrders(source->dockColumnOrders());
    restoreDockLayout(source->saveState(DockLayoutStateVersion), source->openDockNames());

    const auto dockIsVisible = [](QDockWidget* dock) {
        return dock != nullptr && !dock->isHidden();
    };
    const auto statusBarIsVisible = [](const MainWindow* window) {
        const QStatusBar* statusBar = window != nullptr
            ? window->findChild<QStatusBar*>(QString(), Qt::FindDirectChildrenOnly)
            : nullptr;
        return statusBar != nullptr && statusBar->isVisible();
    };

    const bool statusBarVisible = statusBarIsVisible(source);
    if (statusBarIsVisible(this) != statusBarVisible)
        setStatusBarVisible(statusBarVisible);
    m_actions.viewStatusBar->setChecked(statusBarVisible);
    m_status.selectedAngleUnit = source->m_status.selectedAngleUnit;
    m_status.selectedResultFormat = source->m_status.selectedResultFormat;
    m_status.selectedResultPrecision = source->m_status.selectedResultPrecision;
    setStatusBarText();

    const bool formulaBookVisible = dockIsVisible(source->m_docks.book);
    if (dockIsVisible(m_docks.book) != formulaBookVisible)
        setFormulaBookDockVisible(formulaBookVisible, false);
    m_actions.viewFormulaBook->setChecked(formulaBookVisible);

    const bool constantsVisible = dockIsVisible(source->m_docks.constants);
    if (dockIsVisible(m_docks.constants) != constantsVisible)
        setConstantsDockVisible(constantsVisible, false);
    m_actions.viewConstants->setChecked(constantsVisible);

    const bool functionsVisible = dockIsVisible(source->m_docks.functions);
    if (dockIsVisible(m_docks.functions) != functionsVisible)
        setFunctionsDockVisible(functionsVisible, false);
    m_actions.viewFunctions->setChecked(functionsVisible);

    const bool historyVisible = dockIsVisible(source->m_docks.history);
    if (dockIsVisible(m_docks.history) != historyVisible)
        setHistoryDockVisible(historyVisible, false);
    m_actions.viewHistory->setChecked(historyVisible);

    const bool variablesVisible = dockIsVisible(source->m_docks.variables);
    if (dockIsVisible(m_docks.variables) != variablesVisible)
        setVariablesDockVisible(variablesVisible, false);
    m_actions.viewVariables->setChecked(variablesVisible);

    const bool userFunctionsVisible = dockIsVisible(source->m_docks.userFunctions);
    if (dockIsVisible(m_docks.userFunctions) != userFunctionsVisible)
        setUserFunctionsDockVisible(userFunctionsVisible, false);
    m_actions.viewUserFunctions->setChecked(userFunctionsVisible);

    const bool userUnitsVisible = dockIsVisible(source->m_docks.userUnits);
    if (dockIsVisible(m_docks.userUnits) != userUnitsVisible)
        setUserUnitsDockVisible(userUnitsVisible, false);
    m_actions.viewUserUnits->setChecked(userUnitsVisible);

    const bool bitfieldVisible = dockIsVisible(source->m_docks.bitField);
    if (dockIsVisible(m_docks.bitField) != bitfieldVisible)
        setBitfieldVisible(bitfieldVisible);
    m_actions.viewBitfield->setChecked(bitfieldVisible);

    const bool keypadVisible = source->m_widgets.keypad != nullptr;
    restoreWindowKeypadLayout(keypadVisible, static_cast<int>(source->m_keypadMode));
    restoreWindowKeypadZoom(source->m_keypadZoomPercent);

    applyThemeSurfacePalette();
    refreshPaneThemes();
}

Session* MainWindow::createUntitledSession(bool activateCreatedSession)
{
    Session* recycledSession = nullptr;
    int recycledNumber = (std::numeric_limits<int>::max)();
    for (auto it = m_loadedSessions.constBegin(); it != m_loadedSessions.constEnd(); ++it) {
        const QString name = it.key();
        const int number = untitledSessionNumber(name);
        if (number <= 0 || number >= recycledNumber)
            continue;
        if (!isReusableUntitledSession(it.value()))
            continue;

        bool alreadyAttachedToPane = false;
        for (auto paneIt = m_paneSessionTabs.constBegin(); paneIt != m_paneSessionTabs.constEnd(); ++paneIt) {
            if (paneIt.value().contains(name, Qt::CaseInsensitive)) {
                alreadyAttachedToPane = true;
                break;
            }
        }
        if (alreadyAttachedToPane)
            continue;

        recycledSession = it.value();
        recycledNumber = number;
    }

    if (recycledSession != nullptr) {
        applyUserDefinitions();
        if (activateCreatedSession)
            activateSession(recycledSession);
        return recycledSession;
    }

    const QString name = firstAvailableUntitledSessionNameAcrossWindows();
    Session* session = new Session();
    session->setName(name);
    m_loadedSessions.insert(name, session);
    updatePaneLoadedSessionCounts();

    applyUserDefinitions();
    if (activateCreatedSession)
        activateSession(session);
    return session;
}

QList<ResultDisplay*> MainWindow::splitPaneDisplays() const
{
    QList<ResultDisplay*> displays;
    if (m_widgets.splitContainer == nullptr)
        return displays;

    const auto collectDisplays = [&displays](QWidget* widget, const auto& collectDisplaysRef) -> void {
        if (widget == nullptr)
            return;
        if (ResultDisplay* display = qobject_cast<ResultDisplay*>(widget)) {
            displays.append(display);
            return;
        }
        if (QSplitter* splitter = qobject_cast<QSplitter*>(widget)) {
            for (int i = 0; i < splitter->count(); ++i)
                collectDisplaysRef(splitter->widget(i), collectDisplaysRef);
            return;
        }
        const QList<ResultDisplay*> childDisplays = widget->findChildren<ResultDisplay*>();
        for (ResultDisplay* display : childDisplays)
            displays.append(display);
    };
    collectDisplays(m_widgets.splitContainer, collectDisplays);
    return displays;
}

QList<Editor*> MainWindow::splitPaneEditors() const
{
    QList<Editor*> editors;
    for (ResultDisplay* display : splitPaneDisplays()) {
        QWidget* pane = display->parentWidget();
        if (Editor* editor = pane ? pane->findChild<Editor*>() : nullptr)
            editors.append(editor);
    }
    return editors;
}

QStringList MainWindow::paneSessionNames(ResultDisplay* display) const
{
    QStringList names = m_paneSessionTabs.value(display);
    if (names.isEmpty()) {
        const QString activeName = m_paneSessionNames.value(display);
        if (!activeName.isEmpty())
            names.append(activeName);
    }
    return names;
}

void MainWindow::addSessionToActivePane(const QString& name)
{
    if (m_widgets.display == nullptr || name.isEmpty())
        return;

    QStringList names = paneSessionNames(m_widgets.display);
    const bool tabAdded = !names.contains(name, Qt::CaseInsensitive);
    const bool activeChanged = m_paneSessionNames.value(m_widgets.display).compare(name, Qt::CaseInsensitive) != 0;
    if (tabAdded)
        names.append(name);
    if (!tabAdded && !activeChanged)
        return;

    m_paneSessionTabs.insert(m_widgets.display, names);
    m_paneSessionNames.insert(m_widgets.display, name);
    updatePaneLoadedSessionCounts();
}

ResultDisplay* MainWindow::tabBarDisplay(QTabBar* tabBar) const
{
    return m_tabBarDisplays.value(tabBar, nullptr);
}

QTabBar* MainWindow::displayTabBar(ResultDisplay* display) const
{
    return m_paneTabBars.value(display, nullptr);
}

void MainWindow::switchPaneToSession(ResultDisplay* display, const QString& name)
{
    if (m_shutdownStateSaved)
        return;
    if (display == nullptr || name.isEmpty())
        return;
    if (!m_paneTabBars.contains(display))
        return;

    Session* session = m_loadedSessions.value(name, nullptr);
    if (session == nullptr)
        return;

    QWidget* page = display->parentWidget();
    Editor* editor = page ? page->findChild<Editor*>(QString(), Qt::FindDirectChildrenOnly) : nullptr;
    if (editor == nullptr)
        return;

    const QString currentName = m_paneSessionNames.value(display);
    if (!currentName.isEmpty()
        && currentName.compare(name, Qt::CaseInsensitive) != 0) {
        if (Session* currentSession = m_loadedSessions.value(currentName, nullptr))
            currentSession->setEditorText(editor->text());
    }

    const bool paneAlreadyShowsSession =
        display->session() == session
        && m_paneSessionNames.value(display).compare(name, Qt::CaseInsensitive) == 0;

    setActiveEditorDisplayPane(display, editor);
    activateSession(session);
    if (!paneAlreadyShowsSession) {
        editor->setText(session->editorText());
        editor->setCursorPosition(editor->text().size());
    }
    if (!paneAlreadyShowsSession)
        updatePaneTabBars();
}

bool MainWindow::focusOpenSession(const QString& name)
{
    if (name.isEmpty())
        return false;

    QList<QPointer<MainWindow>> candidateWindows;
    candidateWindows.append(QPointer<MainWindow>(this));
    for (const QPointer<MainWindow>& window : allMainWindows()) {
        if (window != nullptr && window != this)
            candidateWindows.append(window);
    }

    for (const QPointer<MainWindow>& window : candidateWindows) {
        if (window == nullptr)
            continue;

        for (ResultDisplay* display : window->splitPaneDisplays()) {
            if (!window->paneSessionNames(display).contains(name, Qt::CaseInsensitive))
                continue;

            QWidget* page = display->parentWidget();
            Editor* editor = page ? page->findChild<Editor*>(QString(), Qt::FindDirectChildrenOnly) : nullptr;
            if (editor == nullptr)
                continue;

            window->setActiveEditorDisplayPane(display, editor);
            window->switchPaneToSession(display, name);
            if (window->isMinimized())
                window->showNormal();
            window->raise();
            window->activateWindow();
            return true;
        }
    }

    return false;
}

void MainWindow::moveSessionTab(QTabBar* sourceTabBar, QTabBar* targetTabBar, const QString& name, int targetIndex)
{
    if (m_shutdownStateSaved)
        return;
    ResultDisplay* sourceDisplay = tabBarDisplay(sourceTabBar);
    ResultDisplay* targetDisplay = tabBarDisplay(targetTabBar);
    if (targetDisplay == nullptr || name.isEmpty())
        return;
    MainWindow* sourceWindow = this;
    if (sourceDisplay == nullptr && sourceTabBar != nullptr)
        sourceWindow = qobject_cast<MainWindow*>(sourceTabBar->window());
    if (sourceDisplay == nullptr && sourceWindow != nullptr)
        sourceDisplay = sourceWindow->tabBarDisplay(sourceTabBar);
    if (sourceDisplay == nullptr || sourceWindow == nullptr)
        return;

    const bool crossWindowMove = sourceWindow != this;
    if (!crossWindowMove && !m_loadedSessions.contains(name))
        return;
    if (crossWindowMove && !sourceWindow->m_loadedSessions.contains(name))
        return;

    QStringList sourceNames = crossWindowMove
        ? sourceWindow->paneSessionNames(sourceDisplay)
        : paneSessionNames(sourceDisplay);
    if (!sourceNames.contains(name, Qt::CaseInsensitive))
        return;
    QStringList targetNames = paneSessionNames(targetDisplay);
    if (crossWindowMove) {
        Session* sourceSession = sourceWindow->m_loadedSessions.value(name, nullptr);
        if (sourceSession == nullptr)
            return;

        Session* movedSession = new Session(*sourceSession);
        movedSession->setName(name);
        m_loadedSessions.insert(name, movedSession);
        applyUserDefinitions();

        if (!targetNames.contains(name, Qt::CaseInsensitive))
            targetNames.insert(qBound(0, targetIndex, targetNames.size()), name);
        m_paneSessionTabs.insert(targetDisplay, targetNames);
        m_paneSessionNames.insert(targetDisplay, name);
        switchPaneToSession(targetDisplay, name);

        sourceWindow->removeSessionTabFromPane(sourceDisplay, name, true);
        const auto referencedByRemainingPanes = [sourceWindow](const QString& sessionName) {
            for (ResultDisplay* display : sourceWindow->splitPaneDisplays()) {
                if (sourceWindow->paneSessionNames(display).contains(sessionName, Qt::CaseInsensitive))
                    return true;
            }
            return false;
        };
        if (!referencedByRemainingPanes(name)) {
            Session* removedSession = sourceWindow->m_loadedSessions.take(name);
            sourceWindow->m_sessionViewportAnchors.remove(name);
            sourceWindow->m_sessionScrollValues.remove(name);
            if (removedSession != nullptr && removedSession != sourceWindow->m_session)
                delete removedSession;
        }

        sourceWindow->updatePaneLoadedSessionCounts();
        sourceWindow->updatePaneTabBars();
        sourceWindow->saveSessionLayout(false);
        updatePaneLoadedSessionCounts();
        updatePaneTabBars();
        saveSessionLayout(false);
        return;
    }

    if (sourceDisplay == targetDisplay) {
        const int sourceIndex = sourceNames.indexOf(name);
        sourceNames.removeAll(name);
        if (sourceIndex >= 0 && sourceIndex < targetIndex)
            --targetIndex;
        sourceNames.insert(qBound(0, targetIndex, sourceNames.size()), name);
        m_paneSessionTabs.insert(sourceDisplay, sourceNames);
        updatePaneTabBars();
        saveSessionLayout(false);
        return;
    }

    sourceNames.removeAll(name);
    if (!targetNames.contains(name, Qt::CaseInsensitive))
        targetNames.insert(qBound(0, targetIndex, targetNames.size()), name);

    if (sourceNames.isEmpty()) {
        if (splitPaneDisplays().size() > 1) {
            m_paneSessionTabs.insert(targetDisplay, targetNames);
            m_paneSessionNames.insert(targetDisplay, name);
            removePaneForDisplay(sourceDisplay);
            switchPaneToSession(targetDisplay, name);
            updatePaneLoadedSessionCounts();
            updatePaneTabBars();
            saveSessionLayout(false);
            return;
        }

        Session* replacement = createUntitledSession(false);
        if (replacement != nullptr)
            sourceNames.append(replacement->name());
    }

    m_paneSessionTabs.insert(sourceDisplay, sourceNames);
    if (m_paneSessionNames.value(sourceDisplay).compare(name, Qt::CaseInsensitive) == 0) {
        m_paneSessionNames.insert(sourceDisplay, sourceNames.first());
        if (Session* sourceSession = m_loadedSessions.value(sourceNames.first(), nullptr)) {
            sourceDisplay->setSession(sourceSession);
            sourceDisplay->refresh();
            QWidget* page = sourceDisplay->parentWidget();
            if (Editor* sourceEditor = page ? page->findChild<Editor*>(QString(), Qt::FindDirectChildrenOnly) : nullptr) {
                sourceEditor->setSession(sourceSession);
                sourceEditor->setText(sourceSession->editorText());
                sourceEditor->setCursorPosition(sourceEditor->text().size());
                sourceEditor->updateHistory();
                sourceEditor->refreshAutoCalc();
            }
        }
    }
    m_paneSessionTabs.insert(targetDisplay, targetNames);
    m_paneSessionNames.insert(targetDisplay, name);

    switchPaneToSession(targetDisplay, name);
    updatePaneLoadedSessionCounts();
    updatePaneTabBars();
    saveSessionLayout(false);
}

void MainWindow::rememberClosedSessionTab(ResultDisplay* display, const QString& name)
{
    if (display == nullptr || name.isEmpty())
        return;

    Session* session = m_loadedSessions.value(name, nullptr);
    if (session == nullptr || session->isPrivate())
        return;

    if (display == m_widgets.display && session == m_session)
        captureEditorTextInCurrentSession();

    ClosedSessionTab closedTab;
    closedTab.name = name;
    session->serialize(closedTab.sessionJson);
    closedTab.editorText = session->editorText();
    closedTab.display = display;
    closedTab.tabIndex = paneSessionNames(display).indexOf(name);
    m_closedSessionTabs.append(closedTab);
    syncSessionMenuActionState();
}

void MainWindow::restoreClosedSessionTab()
{
    if (qApp->activeModalWidget() != nullptr || qApp->activePopupWidget() != nullptr)
        return;
    if (m_closedSessionTabs.isEmpty())
        return;

    ClosedSessionTab closedTab = m_closedSessionTabs.last();
    ResultDisplay* targetDisplay = nullptr;
    const QList<ResultDisplay*> displays = splitPaneDisplays();
    if (closedTab.display != nullptr && displays.contains(closedTab.display)
        && displayTabBar(closedTab.display) != nullptr) {
        targetDisplay = closedTab.display;
    } else if (m_widgets.display != nullptr && displays.contains(m_widgets.display)
               && displayTabBar(m_widgets.display) != nullptr) {
        targetDisplay = m_widgets.display;
    }
    if (targetDisplay == nullptr)
        return;

    m_closedSessionTabs.removeLast();
    syncSessionMenuActionState();

    Session* session = m_loadedSessions.value(closedTab.name, nullptr);
    if (session == nullptr) {
        session = new Session();
        if (!session->deSerialize(closedTab.sessionJson, false)) {
            delete session;
            return;
        }
        session->setName(closedTab.name);
        session->setEditorText(closedTab.editorText);
        m_loadedSessions.insert(closedTab.name, session);
        applyUserDefinitions();
    } else {
        session->setEditorText(closedTab.editorText);
    }

    QStringList targetNames = paneSessionNames(targetDisplay);
    if (targetNames.contains(closedTab.name, Qt::CaseInsensitive)) {
        switchPaneToSession(targetDisplay, closedTab.name);
        return;
    }

    const int insertIndex = qBound(0, closedTab.tabIndex, targetNames.size());
    targetNames.insert(insertIndex, closedTab.name);
    m_paneSessionTabs.insert(targetDisplay, targetNames);
    switchPaneToSession(targetDisplay, closedTab.name);
    updatePaneLoadedSessionCounts();
    updatePaneTabBars();
    saveSessionLayout(false);
}

void MainWindow::removeSessionTabFromPane(ResultDisplay* display, const QString& name, bool closePaneIfEmpty)
{
    if (display == nullptr || name.isEmpty())
        return;

    QStringList names = paneSessionNames(display);
    if (!names.contains(name, Qt::CaseInsensitive))
        return;

    names.removeAll(name);
    if (names.isEmpty()) {
        if (closePaneIfEmpty) {
            removePaneForDisplay(display);
            return;
        }

        Session* replacement = createUntitledSession(false);
        if (replacement != nullptr)
            names.append(replacement->name());
    }

    m_paneSessionTabs.insert(display, names);
    if (m_paneSessionNames.value(display).compare(name, Qt::CaseInsensitive) == 0) {
        m_paneSessionNames.insert(display, names.first());
        if (Session* session = m_loadedSessions.value(names.first(), nullptr)) {
            QWidget* page = display->parentWidget();
            if (Editor* editor = page ? page->findChild<Editor*>(QString(), Qt::FindDirectChildrenOnly) : nullptr) {
                // Removing a tab can switch the visible pane to another
                // session without changing keyboard focus. If this is the
                // active pane, update MainWindow::m_session too; otherwise the
                // window-level symbol docks keep reading the removed session
                // until the pane receives a click/focus event.
                if (display == m_widgets.display && editor == m_widgets.editor) {
                    activateSession(session);
                } else {
                    display->setSession(session);
                    display->refresh();
                    editor->setText(session->editorText());
                    editor->setCursorPosition(editor->text().size());
                    editor->updateHistory();
                    editor->refreshAutoCalc();
                }
            }
        }
    }
}

void MainWindow::removePaneForDisplay(ResultDisplay* display)
{
    QWidget* pane = paneWidgetForDisplay(display);
    if (display == nullptr || pane == nullptr)
        return;

    const QList<ResultDisplay*> displays = splitPaneDisplays();
    if (displays.size() <= 1)
        return;

    ResultDisplay* nextDisplay = nullptr;
    const int displayIndex = displays.indexOf(display);
    if (displayIndex >= 0 && displayIndex + 1 < displays.size())
        nextDisplay = displays.at(displayIndex + 1);
    else if (displayIndex > 0)
        nextDisplay = displays.at(displayIndex - 1);
    else {
        for (ResultDisplay* candidate : displays) {
            if (candidate != display) {
                nextDisplay = candidate;
                break;
            }
        }
    }
    if (nextDisplay == nullptr)
        return;

    Editor* nextEditor = nextDisplay->parentWidget()
        ? nextDisplay->parentWidget()->findChild<Editor*>(QString(), Qt::FindDirectChildrenOnly)
        : nullptr;
    if (nextEditor == nullptr)
        return;

    QTabBar* tabBar = displayTabBar(display);
    m_paneSessionNames.remove(display);
    m_paneSessionTabs.remove(display);
    m_paneTabBars.remove(display);
    if (tabBar != nullptr)
        m_tabBarDisplays.remove(tabBar);

    if (m_widgets.display == display) {
        m_widgets.display = nextDisplay;
        m_widgets.editor = nextEditor;
        m_copyWidget = nextEditor;
        if (Session* nextSession = m_loadedSessions.value(m_paneSessionNames.value(nextDisplay), nullptr))
            activateSession(nextSession);
    }

    // Tab drags run a nested event loop. Defer physical pane destruction until
    // the drag unwinds so source/target widgets cannot disappear mid-event.
    deletePaneAfterSessionTabDrag(pane);
    normalizeSplitContainerTree();
}

void MainWindow::moveSessionTabToPane(QTabBar* sourceTabBar, ResultDisplay* targetDisplay, const QString& name, const QPoint& panePos)
{
    if (sourceTabBar == nullptr || targetDisplay == nullptr || name.isEmpty())
        return;
    MainWindow* sourceWindow = qobject_cast<MainWindow*>(sourceTabBar->window());
    ResultDisplay* sourceDisplay = tabBarDisplay(sourceTabBar);
    if (sourceDisplay == nullptr && sourceWindow != nullptr)
        sourceDisplay = sourceWindow->tabBarDisplay(sourceTabBar);
    if (sourceDisplay == nullptr || sourceWindow == nullptr)
        return;

    QWidget* targetPane = paneWidgetForDisplay(targetDisplay);
    if (targetPane == nullptr)
        return;
    PaneDropZone zone = PaneDropZone::Center;
    if (SessionPane* sessionPane = dynamic_cast<SessionPane*>(targetPane))
        zone = sessionPane->dropZoneForPanePosition(panePos);

    const bool crossWindowMove = (sourceWindow != this);
    if (crossWindowMove) {
        QTabBar* targetTabBar = displayTabBar(targetDisplay);
        if (targetTabBar == nullptr)
            return;
        const int insertIndex = paneSessionNames(targetDisplay).size();
        moveSessionTab(sourceTabBar, targetTabBar, name, insertIndex);
        if (zone == PaneDropZone::Center)
            return;
        QTabBar* localTargetTabBar = displayTabBar(targetDisplay);
        ResultDisplay* localSourceDisplay = localTargetTabBar ? tabBarDisplay(localTargetTabBar) : nullptr;
        if (localTargetTabBar == nullptr || localSourceDisplay == nullptr)
            return;
        switch (zone) {
        case PaneDropZone::Top:
            splitPaneWithSession(localTargetTabBar, targetDisplay, name, Qt::Vertical, false);
            return;
        case PaneDropZone::Bottom:
            splitPaneWithSession(localTargetTabBar, targetDisplay, name, Qt::Vertical, true);
            return;
        case PaneDropZone::Left:
            splitPaneWithSession(localTargetTabBar, targetDisplay, name, Qt::Horizontal, false);
            return;
        case PaneDropZone::Right:
            splitPaneWithSession(localTargetTabBar, targetDisplay, name, Qt::Horizontal, true);
            return;
        case PaneDropZone::Center:
            return;
        }
    }

    switch (zone) {
    case PaneDropZone::Center:
        if (sourceDisplay == targetDisplay)
            return;
        if (sourceWindow != this) {
            if (QTabBar* targetTabBar = displayTabBar(targetDisplay))
                moveSessionTab(sourceTabBar, targetTabBar, name, paneSessionNames(targetDisplay).size());
            return;
        }
        {
            QStringList targetNames = paneSessionNames(targetDisplay);
            if (!targetNames.contains(name, Qt::CaseInsensitive))
                targetNames.append(name);
            m_paneSessionTabs.insert(targetDisplay, targetNames);
            m_paneSessionNames.insert(targetDisplay, name);
            removeSessionTabFromPane(sourceDisplay, name, true);
            switchPaneToSession(targetDisplay, name);
            updatePaneLoadedSessionCounts();
            saveSessionLayout(false);
        }
        break;
    case PaneDropZone::Top:
        splitPaneWithSession(sourceTabBar, targetDisplay, name, Qt::Vertical, false);
        break;
    case PaneDropZone::Bottom:
        splitPaneWithSession(sourceTabBar, targetDisplay, name, Qt::Vertical, true);
        break;
    case PaneDropZone::Left:
        splitPaneWithSession(sourceTabBar, targetDisplay, name, Qt::Horizontal, false);
        break;
    case PaneDropZone::Right:
        splitPaneWithSession(sourceTabBar, targetDisplay, name, Qt::Horizontal, true);
        break;
    }
}

void MainWindow::splitPaneWithSession(QTabBar* sourceTabBar, ResultDisplay* targetDisplay, const QString& name, Qt::Orientation orientation, bool insertAfter)
{
    ResultDisplay* sourceDisplay = tabBarDisplay(sourceTabBar);
    Session* session = m_loadedSessions.value(name, nullptr);
    if (sourceDisplay == nullptr || targetDisplay == nullptr || session == nullptr)
        return;

    QStringList sourceNames = paneSessionNames(sourceDisplay);
    if (!sourceNames.contains(name, Qt::CaseInsensitive))
        return;

    QWidget* activePane = paneWidgetForDisplay(targetDisplay);
    QSplitter* parentSplitter = qobject_cast<QSplitter*>(activePane ? activePane->parentWidget() : nullptr);
    if (parentSplitter == nullptr)
        return;

    Editor* targetEditor = targetDisplay->parentWidget()
        ? targetDisplay->parentWidget()->findChild<Editor*>(QString(), Qt::FindDirectChildrenOnly)
        : nullptr;
    if (targetEditor == nullptr)
        return;

    const int activeIndex = parentSplitter->indexOf(activePane);
    if (activeIndex < 0)
        return;
    const QList<int> parentSizesBefore = parentSplitter->sizes();
    const int activeSize = activeIndex < parentSizesBefore.size()
        ? parentSizesBefore.at(activeIndex)
        : qMax(1, orientation == Qt::Horizontal ? activePane->width() : activePane->height());

    ResultDisplay* display = new ResultDisplay();
    display->setFrameStyle(QFrame::NoFrame);
    display->setFont(targetDisplay->font());
    display->setHoverHighlightEnabled(m_settings->hoverHighlightResults);
    display->setLoadedSessionCount(1);
    display->rehighlight();

    Editor* editor = new Editor();
    editor->setFrameStyle(QFrame::NoFrame);
    editor->setFont(targetEditor->font());
    editor->setAutoCalcEnabled(m_settings->autoCalc);
    editor->setAutoCompletionEnabled(m_settings->autoCompletion);
    editor->setText(session->editorText());
    editor->setCursorPosition(editor->text().size());
    editor->rehighlight();

    QWidget* pane = createEditorDisplayPane(display, editor);
    configureEditorDisplayPane(display, editor);

    QSplitter* targetSplitter = parentSplitter;
    if (parentSplitter->orientation() != orientation) {
        QSplitter* nestedSplitter = new QSplitter(orientation);
        nestedSplitter->setChildrenCollapsible(false);
        nestedSplitter->setHandleWidth(UiConfig::SessionPaneSplitterWidth);
        nestedSplitter->setStyleSheet(m_widgets.splitContainer->styleSheet());
        activePane->setParent(nullptr);
        parentSplitter->insertWidget(activeIndex, nestedSplitter);
        if (insertAfter) {
            nestedSplitter->addWidget(activePane);
            nestedSplitter->addWidget(pane);
        } else {
            nestedSplitter->addWidget(pane);
            nestedSplitter->addWidget(activePane);
        }
        targetSplitter = nestedSplitter;
        if (parentSizesBefore.size() == parentSplitter->count())
            parentSplitter->setSizes(parentSizesBefore);
    } else {
        const int insertIndex = insertAfter ? activeIndex + 1 : activeIndex;
        parentSplitter->insertWidget(insertIndex, pane);
    }

    m_paneSessionNames.insert(display, name);
    m_paneSessionTabs.insert(display, QStringList(name));
    display->setSession(session);
    editor->setSession(session);

    const int firstHalf = qMax(1, activeSize / 2);
    const int secondHalf = qMax(1, activeSize - firstHalf);
    if (targetSplitter == parentSplitter) {
        QList<int> sizes = parentSizesBefore;
        if (activeIndex < sizes.size()) {
            sizes[activeIndex] = insertAfter ? firstHalf : secondHalf;
            sizes.insert(insertAfter ? activeIndex + 1 : activeIndex,
                         insertAfter ? secondHalf : firstHalf);
            if (sizes.size() == targetSplitter->count())
                targetSplitter->setSizes(sizes);
        }
    } else {
        targetSplitter->setSizes(QList<int>({ firstHalf, secondHalf }));
    }

    display->refresh();
    editor->updateHistory();
    editor->refreshAutoCalc();
    removeSessionTabFromPane(sourceDisplay, name, true);
    updateSplitterStyleSheet();
    updatePaneLoadedSessionCounts();
    setActiveEditorDisplayPane(display, editor);
    updatePaneTabBars();
    saveSessionLayout(false);
}

void MainWindow::updatePaneLoadedSessionCounts()
{
    const bool multiplePanes = splitPaneDisplays().size() > 1;
    for (ResultDisplay* display : splitPaneDisplays()) {
        display->setLoadedSessionCount(paneSessionNames(display).size());
        display->setCloseSessionEnabled(multiplePanes || paneSessionNames(display).size() > 1);
    }
    updatePaneTabBars();
    updatePaneEditorCursorVisibility();
}

void MainWindow::updatePaneEditorCursorVisibility()
{
    const QPointer<ResultDisplay> activeDisplay = globallyActiveDisplay();
    const QPointer<Editor> activeEditor = globallyActiveEditor();
    const GeneratedThemeSurfaces surfaces = generatedSurfaceColors(m_settings);
    const int windowCount = std::count_if(allMainWindows().cbegin(), allMainWindows().cend(),
        [](const QPointer<MainWindow>& window) { return !window.isNull(); });
    QSet<MainWindow*> updatedWindows;
    for (const QPointer<MainWindow>& ptr : allMainWindows()) {
        MainWindow* window = ptr.data();
        if (window == nullptr || updatedWindows.contains(window))
            continue;

        updatedWindows.insert(window);
        const QList<ResultDisplay*> displays = window->splitPaneDisplays();
        const bool showActiveOutline = m_settings->editorDisplayStyle != Settings::EditorDisplayClassic
            || windowCount != 1 || displays.size() != 1;
        QSet<Editor*> updatedEditors;
        for (ResultDisplay* display : displays) {
            QWidget* page = display->parentWidget();
            Editor* editor = page
                ? page->findChild<Editor*>(QString(), Qt::FindDirectChildrenOnly)
                : nullptr;
            const bool active = activeDisplay != nullptr
                && activeEditor != nullptr
                && display == activeDisplay
                && editor == activeEditor;
            if (editor != nullptr) {
                updatedEditors.insert(editor);
                const bool editorFocused = active && !editor->isConstantSearchVisible();
                editor->setCustomCursorVisible(editorFocused);
                editor->setThemePrimaryColor(surfaces.primary.background, editorFocused && showActiveOutline);
            }
        }
        for (Editor* editor : window->splitPaneEditors()) {
            if (editor == nullptr || updatedEditors.contains(editor))
                continue;
            editor->setCustomCursorVisible(false);
            editor->setThemePrimaryColor(surfaces.primary.background, false);
        }
        window->updateActiveSessionPaneTabColor();
    }
}

void MainWindow::updateActiveSessionPaneTabColor()
{
    const GeneratedThemeSurfaces surfaces = generatedSurfaceColors(m_settings);
    const QList<ResultDisplay*> displays = splitPaneDisplays();
    for (ResultDisplay* display : displays) {
        QTabBar* tabBar = displayTabBar(display);
        SessionTabBar* sessionTabBar = dynamic_cast<SessionTabBar*>(tabBar);
        if (sessionTabBar == nullptr)
            continue;
        sessionTabBar->setActivePaneSelectedTabIndicatorColor(QColor());
    }

    ResultDisplay* activeDisplay = globallyActiveDisplay();
    if (activeDisplay == nullptr || activeDisplay->window() != this) {
        if (QApplication::activeWindow() != this)
            return;
        activeDisplay = m_widgets.display;
    }
    SessionTabBar* activeTabBar =
        dynamic_cast<SessionTabBar*>(displayTabBar(activeDisplay));
    if (activeTabBar != nullptr)
        activeTabBar->setActivePaneSelectedTabIndicatorColor(surfaces.primary.background);
}

void MainWindow::updatePaneTabBars()
{
    const QList<ResultDisplay*> displays = splitPaneDisplays();
    const bool singlePaneSingleTab = displays.size() == 1 && paneSessionNames(displays.first()).size() == 1;
    const GeneratedThemeSurfaces surfaces = generatedSurfaceColors(m_settings);
    const ThemeSurfaceColors selectedSessionTab =
        themeSurfaceForShadeIndex(surfaces, UiConfig::SelectedSessionTabFillShade);
    const ThemeSurfaceColors hoveredSessionTab =
        themeSurfaceForShadeIndex(surfaces, UiConfig::HoveredSessionTabFillShade);
    const ThemeSurfaceColors closeButtonHover =
        themeSurfaceForShadeIndex(surfaces, UiConfig::SessionTabCloseButtonHoverFillShade);
    const ThemeSurfaceColors toolTip =
        themeSurfaceForShadeIndex(surfaces, UiConfig::CompletionPopupBackgroundShade);
    const ThemeSurfaceColors toolTipOutline =
        themeSurfaceForShadeIndex(surfaces, UiConfig::CompletionPopupOutlineShade);
    for (ResultDisplay* display : displays) {
        QTabBar* tabBar = displayTabBar(display);
        if (tabBar == nullptr)
            continue;
        static_cast<SessionTabBar*>(tabBar)->applyStyle(
            selectedSessionTab.foreground,
            selectedSessionTab.background,
            hoveredSessionTab.foreground,
            hoveredSessionTab.background,
            surfaces.window.background,
            surfaces.window.foreground,
            closeButtonHover.foreground,
            closeButtonHover.background,
            toolTip.background,
            toolTip.foreground,
            toolTipOutline.background,
            UiConfig::CompletionPopupCornerRadius);

        const QSignalBlocker blocker(tabBar);
        const QStringList names = paneSessionNames(display);
        bool tabsAlreadyMatch = tabBar->count() == names.size();
        for (int i = 0; tabsAlreadyMatch && i < names.size(); ++i)
            tabsAlreadyMatch = tabBar->tabText(i) == names.at(i);
        if (!tabsAlreadyMatch) {
            while (tabBar->count() > 0)
                tabBar->removeTab(0);
            for (const QString& name : names)
                tabBar->addTab(name);
        }
        for (int i = 0; i < names.size(); ++i) {
            const Session* session = m_loadedSessions.value(names.at(i));
            const bool isPrivate = session && session->isPrivate();
            tabBar->setTabIcon(i, isPrivate ? privateSessionIcon(selectedSessionTab.foreground) : QIcon());
            tabBar->setTabToolTip(i, isPrivate
                ? tr("%1\nPrivate session: contents are discarded when closed.\n"
                     "Choose Session > Keep to turn it into a regular session."
                    ).arg(names.at(i)) : QString());
        }

        const int activeIndex = names.indexOf(m_paneSessionNames.value(display));
        tabBar->setCurrentIndex(activeIndex >= 0 ? activeIndex : 0);
        // tabBar->setUsesScrollButtons(names.size() > 4);
        tabBar->setVisible(!singlePaneSingleTab);
        static_cast<SessionTabBar*>(tabBar)->refreshCloseButtons();
    }
    updateSessionWindowTitle();
    updateActiveSessionPaneTabColor();
}

void MainWindow::updateSessionWindowTitle()
{
    syncSessionMenuActionState();
    if (m_session && m_session->isPrivate()) {
        setWindowTitle(tr("SpeedCrunch - %1 [Private]").arg(m_session->name()));
        return;
    }
    const QList<ResultDisplay*> displays = splitPaneDisplays();
    if (displays.size() == 1) {
        const QStringList names = paneSessionNames(displays.first());
        if (names.size() == 1) {
            setWindowTitle(QStringLiteral("SpeedCrunch - %1").arg(names.first()));
            return;
        }
    }

    setWindowTitle(QStringLiteral("SpeedCrunch"));
}

void MainWindow::normalizeSplitContainerTree()
{
    normalizeSplitterTree(m_widgets.splitContainer);
}

void MainWindow::updateSplitterStyleSheet()
{
    StartupDiagnostics::AppearanceScope timing("Style session splitters", this);
    if (m_widgets.splitContainer == nullptr)
        return;

    const GeneratedThemeSurfaces surfaces = generatedSurfaceColors(m_settings);
    const bool classic = m_settings->editorDisplayStyle == Settings::EditorDisplayClassic;
    const QColor handle = themeSurfaceForShadeIndex(
        surfaces, classic ? UiConfig::ClassicVerticalPaneDividerShade : UiConfig::SplitterShade).background;
    const QColor hoveredHandle = UiConfig::SplitterHoverUsesPrimary
        ? surfaces.primary.background
        : themeSurfaceForShadeIndex(surfaces, UiConfig::SplitterHoverShade).background;
    const bool systemDocks = m_settings->dockAppearance == Settings::DockAppearanceSystem;
    const QPalette systemPalette = MenuStyle::systemPalette(this);
    setProperty(DockSeparatorNormalColorProperty, systemDocks ? systemPalette.color(QPalette::Mid) : handle);
    setProperty(DockSeparatorActiveColorProperty, systemDocks ? systemPalette.color(QPalette::Highlight) : hoveredHandle);
    setProperty(DockSeparatorBackgroundColorProperty, systemDocks ? systemPalette.color(QPalette::Window) : QColor());
    setProperty(DockSeparatorStrokeWidthProperty,
                systemDocks ? UiConfig::SystemDockSplitterStrokeWidth : UiConfig::DockSplitterStrokeWidth);

    const QColor classicPaneDivider = themeSurfaceForShadeIndex(
        surfaces, UiConfig::ClassicVerticalPaneDividerShade).background;
    const QString styleSheet = splitterStyleSheet(handle, hoveredHandle);
    const QString classicVerticalStyleSheet = splitterStyleSheet(classicPaneDivider, hoveredHandle);
    const auto applyStyle = [classic, &styleSheet, &classicVerticalStyleSheet](
        QSplitter* splitter, const auto& applyStyleRef) -> void {
        if (splitter == nullptr)
            return;
        setStyleSheetIfChanged(splitter, classic && splitter->orientation() == Qt::Horizontal
            ? classicVerticalStyleSheet : styleSheet);
        for (int i = 0; i < splitter->count(); ++i) {
            if (QSplitter* childSplitter = qobject_cast<QSplitter*>(splitter->widget(i)))
                applyStyleRef(childSplitter, applyStyleRef);
        }
    };
    applyStyle(m_widgets.splitContainer, applyStyle);

    for (QSplitterHandle* splitterHandle : findChildren<QSplitterHandle*>()) {
        splitterHandle->installEventFilter(this);
        const bool classicVerticalDivider = classic
            && splitterHandle->orientation() == Qt::Horizontal
            && m_widgets.splitContainer->isAncestorOf(splitterHandle);
        setStyleSheetIfChanged(splitterHandle, splitterHandleStyleSheet(
            classicVerticalDivider ? classicPaneDivider : handle, hoveredHandle));
        splitterHandle->update();
    }
}

void MainWindow::refreshPaneThemes()
{
    // Settings actions can emit several appearance signals during startup.
    // Apply the final pane theme once settings and session restoration settle.
    if (m_initializingUi)
        return;
    StartupDiagnostics::AppearanceScope timing("Apply pane themes", this);
    timing.observeInputs("theme", StartupDiagnostics::configuredThemeInputs());
    StartupDiagnostics::observeAppearanceSettings(timing);
    if (StartupDiagnostics::isRecording()) {
        QVariantList panes;
        for (ResultDisplay* display : splitPaneDisplays())
            panes.append(StartupDiagnostics::appearanceObjectId(display));
        for (Editor* editor : splitPaneEditors())
            panes.append(StartupDiagnostics::appearanceObjectId(editor));
        timing.observeInputs("panes", StartupDiagnostics::appearanceInputs(panes));
    }
    const GeneratedThemeSurfaces surfaces = generatedSurfaceColors(m_settings);
    applyThemeBackgroundRoleToWidget(m_widgets.splitContainer, surfaces.window.background);
    const ThemeSurfaceColors resultToolTip =
        themeSurfaceForShadeIndex(surfaces, UiConfig::ResultTooltipBackgroundShade);
    const ThemeSurfaceColors resultToolTipOutline =
        themeSurfaceForShadeIndex(surfaces, UiConfig::ResultTooltipOutlineShade);
    for (ResultDisplay* display : splitPaneDisplays()) {
        QWidget* page = display->parentWidget();
        applyThemeBackgroundRoleToWidget(paneWidgetForDisplay(display),
                                         surfaces.result.background);
        applyThemeBackgroundRoleToWidget(page ? page->parentWidget() : nullptr,
                                         surfaces.result.background);
        applyThemeBackgroundRoleToWidget(page, surfaces.result.background);
        display->setThemeSurfaceColor(surfaces.result.background);
        display->setThemeToolTipColors(resultToolTip.background,
                                       resultToolTip.foreground,
                                       resultToolTipOutline.background);
        display->setThemeInteractionColors(surfaces.editorAndLists.background,
                                           surfaces.primary.background,
                                           surfaces.headersAndBorders.background,
                                           surfaces.headersAndBorders.foreground,
                                           surfaces.inputs.background,
                                           surfaces.inputs.foreground);
        display->rehighlight();
    }
    for (Editor* editor : splitPaneEditors()) {
        editor->setThemeSurfaceColor(surfaces.editorAndLists.background,
                                     surfaces.result.background);
        const ThemeSurfaceColors completionPopup =
            themeSurfaceForShadeIndex(surfaces, UiConfig::CompletionPopupBackgroundShade);
        const ThemeSurfaceColors completionScrollbarThumb =
            themeSurfaceForShadeIndex(surfaces, UiConfig::CompletionPopupScrollbarThumbShade);
        const ThemeSurfaceColors completionSelectedRow =
            themeSurfaceForShadeIndex(surfaces, UiConfig::CompletionPopupSelectedRowShade);
        const ThemeSurfaceColors completionOutline =
            themeSurfaceForShadeIndex(surfaces, UiConfig::CompletionPopupOutlineShade);
        editor->setThemeCompletionColors(completionPopup.background,
                                         completionPopup.foreground,
                                         completionScrollbarThumb.background,
                                         completionScrollbarThumb.foreground,
                                         completionSelectedRow.background,
                                         completionSelectedRow.foreground,
                                         completionOutline.background,
                                         UiConfig::CompletionPopupCornerRadius,
                                         surfaces.primary.background,
                                         surfaces.primary.foreground);
        editor->rehighlight();
    }
    updatePaneEditorCursorVisibility();
    updatePaneTabBars();
    updateSplitterStyleSheet();
}

void MainWindow::applyDockTabBarAppearance()
{
    StartupDiagnostics::AppearanceScope timing("Apply dock tab bar appearance", this);
    const GeneratedThemeSurfaces tabSurfaces = generatedSurfaceColors(m_settings);
    for (QTabBar* tabBar : findChildren<QTabBar*>()) {
        if (m_tabBarDisplays.contains(tabBar))
            continue;
        saveNativeDockSurface(tabBar);
        if (!tabBar->property("speedcrunchNativeDockTabBar").isValid()) {
            const QVariantList state {tabBar->drawBase(), QVariant::fromValue(tabBar->cursor()),
                                      tabBar->hasMouseTracking(), tabBar->testAttribute(Qt::WA_Hover)};
            tabBar->setProperty("speedcrunchNativeDockTabBar", state);
        }
        QWidget* tabBarParent = tabBar->parentWidget();
        const bool separateParent = tabBarParent != nullptr && tabBarParent != this
            && !qobject_cast<QDockWidget*>(tabBarParent);
        if (separateParent)
            saveNativeDockSurface(tabBarParent);
        if (m_settings->dockAppearance == Settings::DockAppearanceSystem) {
            tabBar->setProperty("speedcrunchDockSystemTabBar", false);
            tabBar->removeEventFilter(this);
            restoreNativeDockSurface(tabBar);
            const QVariantList state = tabBar->property("speedcrunchNativeDockTabBar").toList();
            tabBar->setDrawBase(state.at(0).toBool());
            tabBar->setCursor(state.at(1).value<QCursor>());
            tabBar->setMouseTracking(state.at(2).toBool());
            tabBar->setAttribute(Qt::WA_Hover, state.at(3).toBool());
            // Fill unused space with system colors instead of the main window theme.
            tabBar->setAutoFillBackground(true);
            if (separateParent) {
                restoreNativeDockSurface(tabBarParent);
                tabBarParent->setAutoFillBackground(true);
            }
            continue;
        }
        if (separateParent) {
            applyDockTabBarBackgroundToWidget(tabBarParent, tabSurfaces.window);
        }
        const QPalette tabBarPalette =
            paletteForThemeSurface(tabBar->palette(), tabSurfaces.headersAndBorders);
        if (!tabBar->property("speedcrunchDockSystemTabBar").toBool()) {
            tabBar->setProperty("speedcrunchDockSystemTabBar", true);
            tabBar->setMouseTracking(true);
            tabBar->setAttribute(Qt::WA_Hover, true);
            tabBar->installEventFilter(this);
        }
        updateDockSystemTabCursor(tabBar, tabBar->mapFromGlobal(QCursor::pos()));
        tabBar->setDrawBase(false);
        StartupDiagnostics::setAppearancePalette(tabBar, tabBarPalette);
        setStyleSheetIfChanged(tabBar, QStringLiteral(
            "QTabBar { background-color: %1; }"
            "QTabBar::tab {"
            " background-color: transparent; color: %2;"
            " padding: 5px 14px; margin: 2px 1px;"
            "}"
            "QTabBar::tab:!selected:hover {"
            " background-color: %3; color: %4;"
            " border-radius: 10px;"
            "}"
            "QTabBar::tab:selected {"
            " background-color: %5; color: %6;"
            " border-radius: 10px;"
            "}")
                                  .arg(tabSurfaces.window.background.name(),
                                       tabSurfaces.window.foreground.name(),
                                       tabSurfaces.result.background.name(),
                                       tabSurfaces.result.foreground.name(),
                                       tabSurfaces.headersAndBorders.background.name(),
                                       tabSurfaces.headersAndBorders.foreground.name()));
        StartupDiagnostics::setAppearancePalette(tabBar, tabBarPalette);
    }
}

void MainWindow::applyThemeSurfacePalette()
{
    StartupDiagnostics::AppearanceScope timing("Apply window theme palette", this);
    timing.observeInputs("theme", StartupDiagnostics::configuredThemeInputs());
    StartupDiagnostics::observeAppearanceSettings(timing);
    const GeneratedThemeSurfaces surfaces = generatedSurfaceColors(m_settings);
    if (UiConfig::OklchThemeDebugReportEnabled) {
        const QString reportKey = oklchThemeReportKey(surfaces);
        if (reportKey != lastOklchThemeReportKey()) {
            const QString reportPath = writeOklchGenerationHtmlReport(surfaces.base,
                                                                      1,
                                                                      surfaces.backgrounds.size() - 2,
                                                                      surfaces.polarity,
                                                                      defaultOklchShadeDistanceFactor(),
                                                                      false,
                                                                      surfaces.backgrounds,
                                                                      surfaces.foregrounds,
                                                                      UiConfig::GeneratedTextMinimumContrast);
            if (!reportPath.isEmpty()) {
                lastOklchThemeReportKey() = reportKey;
                QTextStream stream(stderr);
                stream << "OKLCH HTML report: " << reportPath << Qt::endl;
                scheduleThemeRuntimeDiagnosticsReport();
            }
        }
    }
    const ThemeSurfaceColors& surface = surfaces.window;
    applyWindowThemeBackground(this, surface);
    MenuStyle::refresh();

    if (m_widgets.root)
        applyThemeBackgroundRoleToWidget(m_widgets.root, surface.background);
    applyKeypadThemeSurfacePalette();
    applyStatusBarAppearance(findChild<QStatusBar*>(QString(), Qt::FindDirectChildrenOnly),
                             surfaces);
    for (QDockWidget* dock : m_allDocks) {
        if (dock == nullptr)
            continue;
        applyDockAppearance(this, dock, surfaces);
    }
    applyDockTabBarAppearance();
    // The direct pass has already polished the controls for the current style.
    m_dockNativePolishPending = false;
    scheduleDockAppearanceRefresh();
}

void MainWindow::restoreWindowThemeBackgrounds()
{
    StartupDiagnostics::AppearanceScope timing("Restore window theme backgrounds", this);
    timing.observeInputs("theme", StartupDiagnostics::configuredThemeInputs());
    StartupDiagnostics::observeAppearanceSettings(timing);
    const GeneratedThemeSurfaces surfaces = generatedSurfaceColors(m_settings);
    // Native polishing can change inherited palettes after the window is shown.
    applyWindowThemeBackground(this, surfaces.window);
    MenuStyle::refresh();
    applyThemeBackgroundRoleToWidget(m_widgets.root, surfaces.window.background);
    // Keep the deferred dock pass after native layout and palette updates settle.
    scheduleDockAppearanceRefresh();
}

void MainWindow::scheduleDockAppearanceRefresh(bool forceNativePolish)
{
    if (m_initializingUi)
        return;
    m_dockNativePolishPending = m_dockNativePolishPending || forceNativePolish;
    if (m_dockAppearanceRefreshPending)
        return;

    m_dockAppearanceRefreshPending = true;
    QTimer::singleShot(0, this, [this]() {
        StartupDiagnostics::AppearanceContext context("Deferred dock refresh");
        StartupDiagnostics::AppearanceScope timing("Refresh deferred dock appearance", this);
        timing.observeInputs("theme", StartupDiagnostics::configuredThemeInputs());
        StartupDiagnostics::observeAppearanceSettings(timing);
        const bool forceNativePolish = m_dockNativePolishPending;
        m_dockNativePolishPending = false;
        const GeneratedThemeSurfaces surfaces = generatedSurfaceColors(m_settings);
        MenuStyle::refresh();
        const QList<QDockWidget*> docks = m_allDocks;
        for (QDockWidget* dock : docks) {
            applyDockAppearance(this, dock, surfaces, forceNativePolish);
        }
        applyDockTabBarAppearance();
        updateSplitterStyleSheet();
        m_dockAppearanceRefreshPending = false;
        if (m_dockNativePolishPending)
            scheduleDockAppearanceRefresh(true);
    });
}

void MainWindow::applyKeypadThemeSurfacePalette()
{
    StartupDiagnostics::AppearanceScope timing("Apply keypad theme palette", m_widgets.keypad);
    if (!m_widgets.keypad)
        return;

    const GeneratedThemeSurfaces surfaces = generatedSurfaceColors(m_settings);
    const ThemeSurfaceColors keypadBackground =
        themeSurfaceForShadeIndex(surfaces, UiConfig::KeypadBackgroundShade);
    const QPalette keypadPalette = paletteForThemeSurface(palette(), keypadBackground);
    const ThemeSurfaceColors keypadButton =
        themeSurfaceForShadeIndex(surfaces, UiConfig::KeypadButtonShade);
    const ThemeSurfaceColors keypadButtonHover =
        themeSurfaceForShadeIndex(surfaces, UiConfig::KeypadButtonHoverShade);
    const ThemeSurfaceColors keypadButtonPressed =
        themeSurfaceForShadeIndex(surfaces, UiConfig::KeypadButtonPressedShade);
    const ThemeSurfaceColors keypadToolTip =
        themeSurfaceForShadeIndex(surfaces, UiConfig::CompletionPopupBackgroundShade);
    const ThemeSurfaceColors keypadToolTipOutline =
        themeSurfaceForShadeIndex(surfaces, UiConfig::CompletionPopupOutlineShade);

    applyThemeBackgroundRoleToWidget(m_widgets.keypadContainer, keypadBackground.background);
    StartupDiagnostics::setAppearancePalette(m_widgets.keypad, keypadPalette);
    m_widgets.keypad->setAutoFillBackground(true);
    const QPointer<Keypad> keypad = m_widgets.keypad;
    QTimer::singleShot(0, this, [keypad, keypadPalette]() {
        if (!keypad)
            return;
        StartupDiagnostics::AppearanceContext context("Deferred keypad refresh");
        StartupDiagnostics::AppearanceScope timing("Refresh deferred keypad palette", keypad);
        StartupDiagnostics::setAppearancePalette(keypad, keypadPalette);
        keypad->setAutoFillBackground(true);
        QEvent paletteChange(QEvent::PaletteChange);
        QApplication::sendEvent(keypad, &paletteChange);
    });
    m_widgets.keypad->setToolTipThemeColors(keypadToolTip.background,
                                            keypadToolTip.foreground,
                                            keypadToolTipOutline.background,
                                            UiConfig::CompletionPopupCornerRadius);
    m_widgets.keypad->setThemeButtonColors(keypadButton.background,
                                           keypadButton.foreground,
                                           keypadButtonHover.background,
                                           keypadButtonHover.foreground,
                                           keypadButtonPressed.background,
                                           keypadButtonPressed.foreground,
                                           surfaces.primary.background);
    QEvent paletteChange(QEvent::PaletteChange);
    QApplication::sendEvent(m_widgets.keypad, &paletteChange);
}

void MainWindow::scheduleThemeRuntimeDiagnosticsReport()
{
    writeThemeRuntimeDiagnosticsReport();

    QPointer<MainWindow> guard(this);
    QTimer::singleShot(150, this, [guard]() {
        if (guard)
            guard->writeThemeRuntimeDiagnosticsReport();
    });
}

void MainWindow::writeThemeRuntimeDiagnosticsReport()
{
    const QString path = oklchThemeReportPath();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QString html = QString::fromUtf8(file.readAll());
    file.close();
    if (html.isEmpty())
        return;

    const GeneratedThemeSurfaces surfaces = generatedSurfaceColors(m_settings);
    QString section;
    QTextStream out(&section);
    out << "\n<!-- speedcrunch-oklch-runtime-start -->\n"
        << "<section class=\"runtime-diagnostics\" "
        << "style=\"margin-top:2rem;padding:1rem;border:1px solid #c6d0c4;"
        << "border-radius:14px;background:#fcfdfb;\">\n"
        << "<h2 style=\"margin:0 0 0.75rem;\">Runtime widget samples</h2>\n"
        << "<p style=\"margin:0 0 1rem;color:#586659;\">Generated after the "
        << "live window has restored its saved layout and the Qt event loop has "
        << "repainted. These values are read from widget palettes, style sheets, "
        << "and widget grabs inside the running application.</p>\n"
        << "<p><strong>Executable:</strong> <code>"
        << QCoreApplication::applicationFilePath().toHtmlEscaped()
        << "</code></p>\n"
        << "<p><strong>Expected shades:</strong> ";
    for (int i = 0; i < surfaces.backgrounds.size(); ++i) {
        if (i > 0)
            out << " ";
        out << "<code>" << i << "=" << debugColorName(surfaces.backgrounds.at(i)) << "</code>";
    }
    out << "</p>\n"
        << "<div style=\"overflow-x:auto;\">"
        << "<table style=\"width:100%;border-collapse:collapse;font-size:0.92rem;\">"
        << "<thead><tr>"
        << "<th style=\"text-align:left;border-bottom:1px solid #c6d0c4;padding:0.4rem;\">Widget</th>"
        << "<th style=\"text-align:left;border-bottom:1px solid #c6d0c4;padding:0.4rem;\">Expected</th>"
        << "<th style=\"text-align:left;border-bottom:1px solid #c6d0c4;padding:0.4rem;\">Palette</th>"
        << "<th style=\"text-align:left;border-bottom:1px solid #c6d0c4;padding:0.4rem;\">Viewport palette</th>"
        << "<th style=\"text-align:left;border-bottom:1px solid #c6d0c4;padding:0.4rem;\">Grab center</th>"
        << "<th style=\"text-align:left;border-bottom:1px solid #c6d0c4;padding:0.4rem;\">Style sheet</th>"
        << "</tr></thead><tbody>\n";

    appendDiagnosticRow(out,
                        QStringLiteral("MainWindow"),
                        debugColorName(surfaces.window.background),
                        paletteColorName(this, QPalette::Window),
                        QStringLiteral("(n/a)"),
                        grabbedCenterColorName(this),
                        shortStyleSheet(this));

    appendDiagnosticRow(out,
                        QStringLiteral("MainSplitContainer"),
                        debugColorName(surfaces.window.background),
                        paletteColorName(m_widgets.splitContainer, QPalette::Window),
                        QStringLiteral("(n/a)"),
                        grabbedCenterColorName(m_widgets.splitContainer),
                        shortStyleSheet(m_widgets.splitContainer));

    int displayIndex = 0;
    for (ResultDisplay* display : splitPaneDisplays()) {
        ++displayIndex;
        QWidget* page = display ? display->parentWidget() : nullptr;
        QWidget* stack = page ? page->parentWidget() : nullptr;
        QWidget* pane = display ? paneWidgetForDisplay(display) : nullptr;
        appendDiagnosticRow(out,
                            QStringLiteral("ResultDisplay %1").arg(displayIndex),
                            debugColorName(surfaces.result.background),
                            paletteColorName(display, QPalette::Base),
                            paletteColorName(display ? display->viewport() : nullptr,
                                             QPalette::Base),
                            grabbedCenterColorName(display ? display->viewport() : nullptr),
                            shortStyleSheet(display));
        appendDiagnosticRow(out,
                            QStringLiteral("ResultDisplay %1 viewport").arg(displayIndex),
                            debugColorName(surfaces.result.background),
                            paletteColorName(display ? display->viewport() : nullptr,
                                             QPalette::Window),
                            QStringLiteral("(n/a)"),
                            grabbedCenterColorName(display ? display->viewport() : nullptr),
                            shortStyleSheet(display ? display->viewport() : nullptr));
        appendDiagnosticRow(out,
                            QStringLiteral("ResultDisplay %1 page").arg(displayIndex),
                            debugColorName(surfaces.result.background),
                            paletteColorName(page, QPalette::Window),
                            QStringLiteral("(n/a)"),
                            grabbedCenterColorName(page),
                            shortStyleSheet(page));
        appendDiagnosticRow(out,
                            QStringLiteral("ResultDisplay %1 stack").arg(displayIndex),
                            debugColorName(surfaces.result.background),
                            paletteColorName(stack, QPalette::Window),
                            QStringLiteral("(n/a)"),
                            grabbedCenterColorName(stack),
                            shortStyleSheet(stack));
        appendDiagnosticRow(out,
                            QStringLiteral("ResultDisplay %1 pane").arg(displayIndex),
                            debugColorName(surfaces.result.background),
                            paletteColorName(pane, QPalette::Window),
                            QStringLiteral("(n/a)"),
                            grabbedCenterColorName(pane),
                            shortStyleSheet(pane));
    }

    int editorIndex = 0;
    for (Editor* editor : splitPaneEditors()) {
        ++editorIndex;
        appendDiagnosticRow(out,
                            QStringLiteral("Editor %1").arg(editorIndex),
                            debugColorName(surfaces.editorAndLists.background),
                            paletteColorName(editor, QPalette::Base),
                            paletteColorName(editor ? editor->viewport() : nullptr,
                                             QPalette::Base),
                            grabbedCenterColorName(editor),
                            shortStyleSheet(editor));
    }

    const ThemeSurfaceColors keypadBackground =
        themeSurfaceForShadeIndex(surfaces, UiConfig::KeypadBackgroundShade);
    appendDiagnosticRow(out,
                        QStringLiteral("Keypad"),
                        debugColorName(keypadBackground.background),
                        paletteColorName(m_widgets.keypad, QPalette::Window),
                        QStringLiteral("(n/a)"),
                        grabbedCenterColorName(m_widgets.keypad),
                        shortStyleSheet(m_widgets.keypad));

    appendDiagnosticRow(out,
                        QStringLiteral("Bitfield"),
                        debugColorName(surfaces.editorAndLists.background),
                        paletteColorName(m_widgets.bitField, QPalette::Window),
                        QStringLiteral("(n/a)"),
                        grabbedCenterColorName(m_widgets.bitField),
                        shortStyleSheet(m_widgets.bitField));

    if (QStatusBar* bar = findChild<QStatusBar*>(QString(), Qt::FindDirectChildrenOnly)) {
        const ThemeSurfaceColors statusBarSurface =
            themeSurfaceForShadeIndex(surfaces, UiConfig::StatusBarBackgroundShade);
        appendDiagnosticRow(out,
                            QStringLiteral("StatusBar"),
                            debugColorName(statusBarSurface.background),
                            paletteColorName(bar, QPalette::Window),
                            QStringLiteral("(n/a)"),
                            grabbedCenterColorName(bar),
                            shortStyleSheet(bar));
    }

    out << "</tbody></table></div>\n"
        << "</section>\n"
        << "<!-- speedcrunch-oklch-runtime-end -->\n";

    const QString startMarker = QStringLiteral("<!-- speedcrunch-oklch-runtime-start -->");
    const QString endMarker = QStringLiteral("<!-- speedcrunch-oklch-runtime-end -->");
    const int start = html.indexOf(startMarker);
    const int end = start >= 0 ? html.indexOf(endMarker, start) : -1;
    if (start >= 0 && end >= 0)
        html.remove(start, end + endMarker.size() - start);

    const int insertAt = html.lastIndexOf(QStringLiteral("</main>"));
    if (insertAt >= 0)
        html.insert(insertAt, section);
    else
        html.append(section);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return;
    file.write(html.toUtf8());
    file.close();
}

void MainWindow::captureVisibleSessionViewports()
{
    for (ResultDisplay* display : splitPaneDisplays()) {
        if (display == nullptr)
            continue;

        QString sessionName = m_paneSessionNames.value(display);
        if (sessionName.isEmpty() && display->session() != nullptr)
            sessionName = display->session()->name();
        if (sessionName.isEmpty())
            continue;

        m_sessionViewportAnchors.insert(sessionName, display->viewportTopAnchor());
        QScrollBar* bar = display->verticalScrollBar();
        const int scrollValue = bar->value() == bar->maximum()
            ? (std::numeric_limits<int>::max)()
            : bar->value();
        m_sessionScrollValues.insert(sessionName, scrollValue);
    }
}

void MainWindow::restoreVisibleSessionViewports()
{
    StartupDiagnostics::Scope timing("Restore visible history viewports");
    for (ResultDisplay* display : splitPaneDisplays()) {
        if (display == nullptr)
            continue;

        QString sessionName = m_paneSessionNames.value(display);
        if (sessionName.isEmpty() && display->session() != nullptr)
            sessionName = display->session()->name();
        if (sessionName.isEmpty())
            continue;

        const QPair<int, int> anchor = m_sessionViewportAnchors.value(sessionName, qMakePair(-1, 0));
        const int scrollValue = m_sessionScrollValues.value(sessionName, -1);
        if (anchor.first >= 0)
            display->restoreViewportTopAnchor(anchor);
        if (scrollValue >= 0)
            display->restoreScrollValue(scrollValue);
    }
}

void MainWindow::createBitField() {
    StartupDiagnostics::Scope timing("Create bitfield dock");
    if (m_docks.bitField) {
        m_docks.bitField->show();
        m_docks.bitField->raise();
        m_settings->bitfieldVisible = true;
        return;
    }

    m_docks.bitField = new GenericDock<BitFieldWidget>("MainWindow", QT_TR_NOOP("Bitfield"), this);
    m_docks.bitField->setObjectName("BitfieldDock");
    m_docks.bitField->installEventFilter(this);
    m_docks.bitField->setAllowedAreas(Qt::AllDockWidgetAreas);
    m_widgets.bitField = m_docks.bitField->widget();

    addTabifiedDock(m_docks.bitField, false, Qt::BottomDockWidgetArea);
    m_widgets.display->verticalScrollBar()->setValue(m_widgets.display->verticalScrollBar()->maximum());
    connect(m_widgets.bitField, SIGNAL(bitsChanged(const QString&)), SLOT(handleBitsChanged(const QString&)));
    m_settings->bitfieldVisible = true;
}

void MainWindow::createKeypad()
{
    StartupDiagnostics::Scope timing("Create keypad");
    if (m_widgets.keypad)
        return;

    m_widgets.keypadContainer = new QWidget(m_widgets.root);

    if (m_keypadMode == Settings::KeypadModeCustom) {
        QList<Keypad::CustomButtonDescription> customButtons;
        for (const auto& button : m_settings->customKeypad.buttons) {
            if (button.row < 0 || button.row >= m_settings->customKeypad.rows
                    || button.column < 0 || button.column >= m_settings->customKeypad.columns) {
                continue;
            }
            Keypad::CustomButtonDescription description;
            description.label = button.label;
            description.text = button.text;
            description.action = static_cast<int>(button.action);
            description.row = button.row;
            description.column = button.column;
            customButtons.append(description);
        }
        m_widgets.keypad = new Keypad(customButtons,
                                      m_widgets.keypadContainer,
                                      m_keypadZoomPercent);
        connect(m_widgets.keypad, &Keypad::customButtonPressed,
                this, &MainWindow::handleCustomKeypadButtonPress);
    } else {
        Keypad::LayoutMode layoutMode = Keypad::LayoutModeScientificWide;
        if (m_keypadMode == Settings::KeypadModeBasicWide)
            layoutMode = Keypad::LayoutModeBasicWide;
        else if (m_keypadMode == Settings::KeypadModeScientificNarrow)
            layoutMode = Keypad::LayoutModeScientificNarrow;
        m_widgets.keypad = new Keypad(layoutMode,
                                      m_widgets.keypadContainer,
                                      m_keypadZoomPercent);
        connect(m_widgets.keypad, SIGNAL(buttonPressed(Keypad::Button)), SLOT(handleKeypadButtonPress(Keypad::Button)));
        connect(this, SIGNAL(radixCharacterChanged()), m_widgets.keypad, SLOT(handleRadixCharacterChange()));
    }
    m_widgets.keypad->setFocusPolicy(Qt::NoFocus);
    applyKeypadAppearance();
    m_widgets.keypad->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_widgets.keypad, SIGNAL(customContextMenuRequested(const QPoint&)),
            SLOT(showKeypadContextMenu(const QPoint&)));

    m_layouts.keypad = new QHBoxLayout(m_widgets.keypadContainer);
    m_layouts.keypad->setContentsMargins(0, 0, 0, 0);
    m_layouts.keypad->addStretch();
    m_layouts.keypad->addWidget(m_widgets.keypad);
    m_layouts.keypad->addStretch();
    m_widgets.keypad->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_layouts.root->addWidget(m_widgets.keypadContainer, 0);

    m_widgets.keypadContainer->show();
    m_widgets.keypad->show();
    applyKeypadThemeSurfacePalette();
}

void MainWindow::createBookDock(bool)
{
    StartupDiagnostics::Scope timing("Create formula book dock");
    if (m_docks.book) {
        m_docks.book->show();
        m_docks.book->raise();
        m_settings->formulaBookDockVisible = true;
        return;
    }

    m_docks.book = new BookDock(this);
    m_docks.book->setObjectName("BookDock");
    m_docks.book->installEventFilter(this);
    m_docks.book->setAllowedAreas(Qt::AllDockWidgetAreas);

    connect(m_docks.book,
            SIGNAL(expressionSelected(const QString&)),
            SLOT(insertTextIntoEditor(const QString&)));

    // No focus for this dock.
    addTabifiedDock(m_docks.book, false);
    if (!m_settings->formulaBookActivePage.isEmpty())
        m_docks.book->openPage(QUrl(m_settings->formulaBookActivePage));
    m_settings->formulaBookDockVisible = true;
}

void MainWindow::createConstantsDock(bool takeFocus)
{
    StartupDiagnostics::Scope timing("Create constants dock");
    if (m_docks.constants) {
        m_docks.constants->show();
        m_docks.constants->raise();
        if (takeFocus)
            m_docks.constants->setFocus();
        m_settings->constantsDockVisible = true;
        return;
    }

    const ConstantsWidget::InitialState state {m_settings->constantsDockDomain,
        m_settings->constantsDockSubdomain, m_settings->constantsDockSearchText};
    m_docks.constants = new GenericDock<ConstantsWidget>("MainWindow", QT_TR_NOOP("Constants"), this, state);
    m_docks.constants->setObjectName("ConstantsDock");
    m_docks.constants->setMinimumWidth(UiConfig::ConstantsDockMinimumWidth);
    m_docks.constants->installEventFilter(this);
    m_docks.constants->setAllowedAreas(Qt::AllDockWidgetAreas);

    connect(m_docks.constants->widget(), &ConstantsWidget::constantSelected,
            this, &MainWindow::insertConstantIntoEditor);
    connect(this, &MainWindow::radixCharacterChanged,
            m_docks.constants->widget(), &ConstantsWidget::handleRadixCharacterChange);

    addTabifiedDock(m_docks.constants, takeFocus);
    m_settings->constantsDockVisible = true;
}

void MainWindow::createFunctionsDock(bool takeFocus)
{
    StartupDiagnostics::Scope timing("Create functions dock");
    if (m_docks.functions) {
        m_docks.functions->show();
        m_docks.functions->raise();
        if (takeFocus)
            m_docks.functions->setFocus();
        m_settings->functionsDockVisible = true;
        return;
    }

    const FunctionsWidget::InitialState state {m_settings->functionsDockDomain,
        m_settings->functionsDockSearchText};
    m_docks.functions = new GenericDock<FunctionsWidget>("MainWindow", QT_TR_NOOP("Functions"), this, state);
    m_docks.functions->setObjectName("FunctionsDock");
    m_docks.functions->installEventFilter(this);
    m_docks.functions->setAllowedAreas(Qt::AllDockWidgetAreas);

    connect(m_docks.functions->widget(), &FunctionsWidget::functionSelected,
            this, &MainWindow::insertFunctionIntoEditor);

    addTabifiedDock(m_docks.functions, takeFocus);
    m_settings->functionsDockVisible = true;
}

void MainWindow::createHistoryDock(bool)
{
    StartupDiagnostics::Scope timing("Create history dock");
    if (m_docks.history) {
        m_docks.history->show();
        m_docks.history->raise();
        m_settings->historyDockVisible = true;
        return;
    }

    m_docks.history = new GenericDock<HistoryWidget>("MainWindow", QT_TR_NOOP("History"), this);
    m_docks.history->setObjectName("HistoryDock");
    m_docks.history->installEventFilter(this);
    m_docks.history->setAllowedAreas(Qt::AllDockWidgetAreas);

    connect(m_docks.history->widget(), &HistoryWidget::expressionSelected,
            this, &MainWindow::insertHistoryExpressionIntoEditor);
    connect(m_docks.history->widget(), &HistoryWidget::removeHistoryEntryRequested,
            this, &MainWindow::removeHistoryEntryAt);
    connect(m_docks.history->widget(), &HistoryWidget::removeHistoryEntriesAboveRequested,
            this, &MainWindow::removeHistoryEntriesAbove);
    connect(m_docks.history->widget(), &HistoryWidget::removeHistoryEntriesBelowRequested,
            this, &MainWindow::removeHistoryEntriesBelow);
    connect(this, &MainWindow::historyChanged,
            m_docks.history->widget(), &HistoryWidget::updateHistory);
    m_docks.history->widget()->setSession(m_session);

    // No focus for this dock.
    addTabifiedDock(m_docks.history, false);
    m_settings->historyDockVisible = true;
}

void MainWindow::createVariablesDock(bool takeFocus)
{
    StartupDiagnostics::Scope timing("Create variables dock");
    if (m_docks.variables) {
        m_docks.variables->show();
        m_docks.variables->raise();
        if (takeFocus)
            m_docks.variables->setFocus();
        m_settings->variablesDockVisible = true;
        return;
    }

    m_docks.variables = new GenericDock<VariableListWidget>("MainWindow", QT_TR_NOOP("User Variables"), this);
    m_docks.variables->setObjectName("VariablesDock");
    m_docks.variables->installEventFilter(this);
    m_docks.variables->setAllowedAreas(Qt::AllDockWidgetAreas);

    connect(m_docks.variables->widget(), &VariableListWidget::variableSelected,
            this, &MainWindow::insertVariableIntoEditor);
    connect(m_docks.variables->widget(), &VariableListWidget::variableEdited,
            this, &MainWindow::insertTextIntoEditor);
    m_docks.variables->widget()->setEvaluator(m_evaluator);
    const auto updateVariables = [this]() {
        if (m_docks.variables)
            m_docks.variables->widget()->updateList();
    };
    connect(this, &MainWindow::radixCharacterChanged, this, updateVariables);
    connect(this, &MainWindow::variablesChanged, this, updateVariables);
    connect(m_docks.variables, &QDockWidget::visibilityChanged, this, [updateVariables](bool visible) {
        if (visible)
            updateVariables();
    });

    addTabifiedDock(m_docks.variables, takeFocus);
    m_docks.variables->widget()->setSearchText(m_settings->variablesDockSearchText);
    m_settings->variablesDockVisible = true;
}

void MainWindow::createUserFunctionsDock(bool takeFocus)
{
    StartupDiagnostics::Scope timing("Create user functions dock");
    if (m_docks.userFunctions) {
        m_docks.userFunctions->show();
        m_docks.userFunctions->raise();
        if (takeFocus)
            m_docks.userFunctions->setFocus();
        m_settings->userFunctionsDockVisible = true;
        return;
    }

    m_docks.userFunctions = new GenericDock<UserFunctionListWidget>("MainWindow", QT_TR_NOOP("User Functions"), this);
    m_docks.userFunctions->setObjectName("UserFunctionsDock");
    m_docks.userFunctions->installEventFilter(this);
    m_docks.userFunctions->setAllowedAreas(Qt::AllDockWidgetAreas);

    connect(m_docks.userFunctions->widget(), &UserFunctionListWidget::userFunctionSelected,
            this, &MainWindow::insertUserFunctionIntoEditor);
    connect(m_docks.userFunctions->widget(), &UserFunctionListWidget::userFunctionEdited,
            this, &MainWindow::insertUserFunctionIntoEditor);
    m_docks.userFunctions->widget()->setEvaluator(m_evaluator);
    const auto updateFunctions = [this]() {
        if (m_docks.userFunctions)
            m_docks.userFunctions->widget()->updateList();
    };
    connect(this, &MainWindow::radixCharacterChanged, this, updateFunctions);
    connect(this, &MainWindow::functionsChanged, this, updateFunctions);
    connect(m_docks.userFunctions, &QDockWidget::visibilityChanged, this, [updateFunctions](bool visible) {
        if (visible)
            updateFunctions();
    });

    addTabifiedDock(m_docks.userFunctions, takeFocus);
    m_docks.userFunctions->widget()->setSearchText(m_settings->userFunctionsDockSearchText);
    m_settings->userFunctionsDockVisible = true;
}

void MainWindow::createUserUnitsDock(bool takeFocus)
{
    StartupDiagnostics::Scope timing("Create user units dock");
    if (m_docks.userUnits) {
        m_docks.userUnits->show();
        m_docks.userUnits->raise();
        if (takeFocus)
            m_docks.userUnits->setFocus();
        m_settings->userUnitsDockVisible = true;
        return;
    }

    m_docks.userUnits = new GenericDock<UserUnitListWidget>("MainWindow", QT_TR_NOOP("User Units"), this);
    m_docks.userUnits->setObjectName("UserUnitsDock");
    m_docks.userUnits->installEventFilter(this);
    m_docks.userUnits->setAllowedAreas(Qt::AllDockWidgetAreas);

    connect(m_docks.userUnits->widget(), &UserUnitListWidget::userUnitSelected,
            this, &MainWindow::insertUserUnitIntoEditor);
    connect(m_docks.userUnits->widget(), &UserUnitListWidget::userUnitEdited,
            this, &MainWindow::insertTextIntoEditor);
    m_docks.userUnits->widget()->setEvaluator(m_evaluator);
    const auto updateUnits = [this]() {
        if (m_docks.userUnits)
            m_docks.userUnits->widget()->updateList();
    };
    connect(this, &MainWindow::radixCharacterChanged, this, updateUnits);
    connect(this, &MainWindow::unitsChanged, this, updateUnits);
    connect(m_docks.userUnits, &QDockWidget::visibilityChanged, this, [updateUnits](bool visible) {
        if (visible)
            updateUnits();
    });

    addTabifiedDock(m_docks.userUnits, takeFocus);
    m_docks.userUnits->widget()->setSearchText(m_settings->userUnitsDockSearchText);
    m_settings->userUnitsDockVisible = true;
}

void MainWindow::addTabifiedDock(QDockWidget* newDock, bool takeFocus, Qt::DockWidgetArea area)
{
    connect(newDock, &QDockWidget::visibilityChanged, this, &MainWindow::handleDockWidgetVisibilityChanged);
    connect(newDock, &QDockWidget::topLevelChanged, newDock, [newDock]() {
        // Qt replaces title button icons when changing between floating and
        // docked. Restore our themed icons after that reset, before any hover.
        for (QAbstractButton* button : newDock->findChildren<QAbstractButton*>(
                 QString(), Qt::FindDirectChildrenOnly)) {
            if (button->property("speedcrunchDockHeaderButton").toBool())
                applyDockTitleButtonIcon(button, button->underMouse());
        }
    });
    const bool restored = m_hasRestoredDockLayout && restoreDockWidget(newDock);
    if (!restored) {
        addDockWidget(area, newDock);
        for (QDockWidget* dock : m_allDocks) {
            // Keep floating panels independent when opening another dock.
            if (!dock->isFloating() && dockWidgetArea(dock) == area)
                tabifyDockWidget(dock, newDock);
        }
    }
    m_allDocks.append(newDock);
    restoreDockColumnOrder(newDock);
    newDock->show();
    newDock->raise();
    // Startup styles the restored docks together once their layout is ready.
    if (!m_initializingUi) {
        const GeneratedThemeSurfaces surfaces = generatedSurfaceColors(m_settings);
        applyDockAppearance(this, newDock, surfaces);
        applyDockTabBarAppearance();
        updateSplitterStyleSheet();
        scheduleDockAppearanceRefresh();
    }
    if (takeFocus)
        newDock->setFocus();
}

void MainWindow::deleteDock(QDockWidget* dock)
{
    // QMainWindow retains internal layout items for dock restoration and tab
    // groups. Keep an attached dock alive and hidden instead of destroying it
    // from its Close event while Qt may still be traversing that layout.
    dock->hide();
}

void MainWindow::createFixedConnections()
{
    StartupDiagnostics::Scope timing("Connect UI signals");
    ResultDisplay* initialDisplay = m_widgets.display;
    Editor* initialEditor = m_widgets.editor;
    connect(initialEditor, &Editor::textChanged, this, [this, initialDisplay, initialEditor]() {
        if (initialEditor == m_widgets.editor
            || initialEditor->hasFocus()
            || initialEditor->viewport()->hasFocus()) {
            setActiveEditorDisplayPane(initialDisplay, initialEditor);
        }
    });
    connect(initialEditor, &Editor::selectionChanged, this, [this, initialDisplay, initialEditor]() {
        setActiveEditorDisplayPane(initialDisplay, initialEditor);
    });
    connect(initialDisplay, &ResultDisplay::clicked, this, [this, initialDisplay, initialEditor]() {
        setActiveEditorDisplayPane(initialDisplay, initialEditor, true);
    });
    connect(initialDisplay, &ResultDisplay::selectionChanged, this, [this, initialDisplay, initialEditor]() {
        setActiveEditorDisplayPane(initialDisplay, initialEditor, true);
    });
    connect(this, &MainWindow::colorSchemeChanged, this, &MainWindow::updateSplitterStyleSheet);
    connect(this, &MainWindow::colorSchemeChanged, this, &MainWindow::applyThemeSurfacePalette);
    connect(this, &MainWindow::colorSchemeChanged, this, &MainWindow::refreshPaneThemes);
    connect(m_actionGroups.menuAppearance, &QActionGroup::triggered,
            this, &MainWindow::setMenuAppearance);
    connect(m_actionGroups.systemColorMode, &QActionGroup::triggered,
            this, &MainWindow::setSystemColorMode);
    connect(m_actionGroups.dockAppearance, &QActionGroup::triggered,
            this, &MainWindow::setDockAppearance);
    connect(m_actionGroups.statusBarAppearance, &QActionGroup::triggered,
            this, &MainWindow::setStatusBarAppearance);
    connect(m_actionGroups.keypadAppearance, &QActionGroup::triggered,
            this, &MainWindow::setKeypadAppearance);
    connect(m_actionGroups.editorDisplayStyle, &QActionGroup::triggered,
            this, &MainWindow::setEditorDisplayStyle);
    connect(this, &MainWindow::syntaxHighlightingChanged, this, &MainWindow::refreshPaneThemes);

    connect(m_actions.sessionExportJson, SIGNAL(triggered()), SLOT(exportJson()));
    connect(m_actions.sessionExportHtml, SIGNAL(triggered()), SLOT(exportHtml()));
    connect(m_actions.sessionExportPlainText, SIGNAL(triggered()), SLOT(exportPlainText()));
    connect(m_actions.sessionImport, SIGNAL(triggered()), SLOT(showSessionImportDialog()));
    connect(m_actions.sessionImportUserDefinitions, &QAction::triggered,
            this, &MainWindow::showUserDefinitionsImportDialog);
    connect(m_actions.sessionNewTab, &QAction::triggered, this, [this]() {
        // On macOS the native menu bar can dispatch a menu action owned by a
        // window that is no longer frontmost. The target has to come from the
        // current Qt active window first, not from this QAction's owner.
        MainWindow* targetWindow = qobject_cast<MainWindow*>(QApplication::activeWindow());
        if (targetWindow == nullptr) {
            if (QWidget* focusWidget = QApplication::focusWidget())
                targetWindow = qobject_cast<MainWindow*>(focusWidget->window());
        }
        // Focus bookkeeping can lag behind title-bar activation. These fallbacks
        // keep keyboard/menu activation working while avoiding stale global
        // editor state until every foreground-window source has been tried.
        if (targetWindow == nullptr) {
            if (QWidget* focusWidget = lastFocusWidgetInActiveWindow())
                targetWindow = qobject_cast<MainWindow*>(focusWidget->window());
        }
        if (targetWindow == nullptr) {
            if (QWidget* focusWidget = pendingWindowActivationFocusWidget())
                targetWindow = qobject_cast<MainWindow*>(focusWidget->window());
        }
        if (targetWindow == nullptr) {
            if (Editor* activeEditor = pendingWindowActivationEditor())
                targetWindow = qobject_cast<MainWindow*>(activeEditor->window());
        }
        if (targetWindow == nullptr) {
            if (Editor* activeEditor = globallyActiveEditor())
                targetWindow = qobject_cast<MainWindow*>(activeEditor->window());
        }
        if (targetWindow == nullptr) {
            if (ResultDisplay* activeDisplay = globallyActiveDisplay())
                targetWindow = qobject_cast<MainWindow*>(activeDisplay->window());
        }
        if (targetWindow == nullptr)
            targetWindow = this;

        QPointer<MainWindow> guardedWindow(targetWindow);
        QTimer::singleShot(0, targetWindow, [guardedWindow]() {
            if (guardedWindow != nullptr)
                guardedWindow->showNewSessionDialog();
        });
    });
    connect(m_actions.sessionNewWindow, SIGNAL(triggered()), SLOT(showNewSessionWindow()));
    connect(m_actions.sessionNewPrivate, &QAction::triggered, this, [this]() {
        activeMainWindowForMenuAction(this)->showNewPrivateSession();
    });
    connect(m_actions.sessionConvertPrivacy, &QAction::triggered, this, [this]() {
        activeMainWindowForMenuAction(this)->convertSessionPrivacy();
    });
    const auto connectWindowAction = [this](QAction* action, void (MainWindow::*operation)()) {
        connect(action, &QAction::triggered, this, [this, operation]() {
            (activeMainWindowForMenuAction(this)->*operation)();
        });
    };
    connectWindowAction(m_actions.sessionOpen, &MainWindow::showOpenSessionDialog);
    connectWindowAction(m_actions.sessionRename, &MainWindow::showRenameSessionDialog);
    connectWindowAction(m_actions.sessionClone, &MainWindow::showDuplicateSessionDialog);
    connectWindowAction(m_actions.sessionClose, &MainWindow::closeCurrentSession);
    connect(m_actions.sessionReopen, &QAction::triggered, this, [this]() {
        MainWindow* target = activeMainWindowForMenuAction(this);
        QTimer::singleShot(0, target, &MainWindow::restoreClosedSessionTab);
    });
    connect(m_actions.sessionOpenSessionsFolder, SIGNAL(triggered()), SLOT(openSessionsFolder()));
    connect(m_actions.sessionQuit, &QAction::triggered, qApp, &QCoreApplication::quit);

    connectWindowAction(m_actions.editClearExpression, &MainWindow::clearEditorAndBitfield);
    connectWindowAction(m_actions.editClearHistory, &MainWindow::clearHistory);
    connectWindowAction(m_actions.editCopyLastResult, &MainWindow::copyResultToClipboard);
    connectWindowAction(m_actions.editCopy, &MainWindow::copy);
    connectWindowAction(m_actions.editSelectExpression, &MainWindow::selectEditorExpression);
    connectWindowAction(m_actions.editWrapSelection, &MainWindow::wrapSelection);
    const auto connectEditorAction = [this](QAction* action, void (QPlainTextEdit::*operation)()) {
        connect(action, &QAction::triggered, this, [this, operation]() {
            MainWindow* target = activeMainWindowForMenuAction(this);
            if (target->m_widgets.editor != nullptr)
                (target->m_widgets.editor->*operation)();
            target->syncEditMenuActionState();
        });
    };
    connectEditorAction(m_actions.editUndo, &QPlainTextEdit::undo);
    connectEditorAction(m_actions.editRedo, &QPlainTextEdit::redo);
    connectEditorAction(m_actions.editCut, &QPlainTextEdit::cut);
    connectEditorAction(m_actions.editPaste, &QPlainTextEdit::paste);
    connectWindowAction(m_actions.viewSplitLeft, &MainWindow::splitActivePaneLeft);
    connectWindowAction(m_actions.viewSplitRight, &MainWindow::splitActivePaneRight);
    connectWindowAction(m_actions.viewSplitUp, &MainWindow::splitActivePaneUp);
    connectWindowAction(m_actions.viewSplitDown, &MainWindow::splitActivePaneDown);
    connectWindowAction(m_actions.viewClosePane, &MainWindow::closeCurrentPane);
    connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &MainWindow::syncEditMenuActionState);

    const auto connectViewToggleToActiveWindow =
        [this](QAction* action, auto apply) {
            connect(action, &QAction::triggered, this, [this, apply](bool checked) {
                MainWindow* targetWindow = activeMainWindowForMenuAction(this);
                apply(targetWindow, checked);
                targetWindow->syncViewMenuActionState();
            });
        };
    connectViewToggleToActiveWindow(
        m_actions.viewFullScreenMode,
        [](MainWindow* window, bool enabled) { window->setFullScreenEnabled(enabled); });
    connect(m_actionGroups.keypad, &QActionGroup::triggered, this,
            [this](QAction* action) {
                MainWindow* targetWindow = activeMainWindowForMenuAction(this);
                targetWindow->setKeypadMode(action);
                targetWindow->syncViewMenuActionState();
            });
    connect(m_actions.viewKeypadDisabled, &QAction::toggled,
            this, [this](bool) {
                updateKeypadDisabledActionText();
            });
    connect(m_actionGroups.keypadZoom, &QActionGroup::triggered, this,
            [this](QAction* action) {
                MainWindow* targetWindow = activeMainWindowForMenuAction(this);
                targetWindow->setKeypadZoom(action);
                targetWindow->syncViewMenuActionState();
            });
    connectViewToggleToActiveWindow(
        m_actions.viewStatusBar,
        [](MainWindow* window, bool visible) { window->setStatusBarVisible(visible); });
#if !defined(Q_OS_MACOS)
    connectViewToggleToActiveWindow(
        m_actions.viewMenuBar,
        [](MainWindow* window, bool visible) { window->setMenuBarVisible(visible); });
#endif
    connectViewToggleToActiveWindow(
        m_actions.viewBitfield,
        [](MainWindow* window, bool visible) { window->setBitfieldVisible(visible); });
    connectViewToggleToActiveWindow(
        m_actions.viewConstants,
        [](MainWindow* window, bool visible) { window->setConstantsDockVisible(visible); });
    connectViewToggleToActiveWindow(
        m_actions.viewFunctions,
        [](MainWindow* window, bool visible) { window->setFunctionsDockVisible(visible); });
    connectViewToggleToActiveWindow(
        m_actions.viewHistory,
        [](MainWindow* window, bool visible) { window->setHistoryDockVisible(visible); });
    connectViewToggleToActiveWindow(
        m_actions.viewFormulaBook,
        [](MainWindow* window, bool visible) { window->setFormulaBookDockVisible(visible); });
    connectViewToggleToActiveWindow(
        m_actions.viewVariables,
        [](MainWindow* window, bool visible) { window->setVariablesDockVisible(visible); });
    connectViewToggleToActiveWindow(
        m_actions.viewUserFunctions,
        [](MainWindow* window, bool visible) { window->setUserFunctionsDockVisible(visible); });
    connectViewToggleToActiveWindow(
        m_actions.viewUserUnits,
        [](MainWindow* window, bool visible) { window->setUserUnitsDockVisible(visible); });
    connect(m_menus.view, &QMenu::aboutToShow, this, [this]() {
        activeMainWindowForMenuAction(this)->syncViewMenuActionState();
    });

    const auto connectToActiveWindow =
        [this](QAction* action, void (MainWindow::*handler)()) {
            connect(action, &QAction::triggered, this, [this, handler]() {
                MainWindow* targetWindow = activeMainWindowForMenuAction(this);
                (targetWindow->*handler)();
            });
        };
    connectToActiveWindow(m_actions.settingsAngleUnitDegree, &MainWindow::setAngleModeDegree);
    connectToActiveWindow(m_actions.settingsAngleUnitRadian, &MainWindow::setAngleModeRadian);
    connectToActiveWindow(m_actions.settingsAngleUnitGradian, &MainWindow::setAngleModeGradian);
    connectToActiveWindow(m_actions.settingsAngleUnitTurn, &MainWindow::setAngleModeTurn);
    connectToActiveWindow(m_actions.settingsAngleUnitRevolution, &MainWindow::setAngleModeRevolution);

    if (!isWaylandPlatform())
        connect(m_actions.settingsBehaviorAlwaysOnTop, SIGNAL(toggled(bool)), SLOT(setAlwaysOnTopEnabled(bool)));
    connect(m_actions.settingsBehaviorAutoCompletion, SIGNAL(toggled(bool)), SLOT(setAutoCompletionEnabled(bool)));
    connect(m_actions.settingsBehaviorAutoCompletionAcceptWithEnter, &QAction::toggled,
            this, &MainWindow::setAutoCompletionAcceptWithEnterEnabled);
    connect(m_actions.settingsBehaviorAutoCompletionBuiltInFunctions, SIGNAL(toggled(bool)), SLOT(setAutoCompletionBuiltInFunctionsEnabled(bool)));
    connect(m_actions.settingsBehaviorAutoCompletionBuiltInVariables, SIGNAL(toggled(bool)), SLOT(setAutoCompletionBuiltInVariablesEnabled(bool)));
    connect(m_actions.settingsBehaviorAutoCompletionLongFormUnits, SIGNAL(toggled(bool)), SLOT(setAutoCompletionLongFormUnitsEnabled(bool)));
    connect(m_actions.settingsBehaviorAutoCompletionUserFunctions, SIGNAL(toggled(bool)), SLOT(setAutoCompletionUserFunctionsEnabled(bool)));
    connect(m_actions.settingsBehaviorAutoCompletionUserVariables, SIGNAL(toggled(bool)), SLOT(setAutoCompletionUserVariablesEnabled(bool)));
    connect(m_actions.settingsBehaviorAutoAns, SIGNAL(toggled(bool)), SLOT(setAutoAnsEnabled(bool)));
    connect(m_actions.settingsBehaviorPartialResults, SIGNAL(toggled(bool)), SLOT(setAutoCalcEnabled(bool)));
    connect(m_actions.settingsBehaviorHistorySizeLimit, SIGNAL(triggered()), SLOT(setHistorySizeLimit()));
    connect(m_actions.settingsBehaviorSaveWindowPositionOnExit, SIGNAL(toggled(bool)), SLOT(setWindowPositionSaveEnabled(bool)));
    connect(m_actions.settingsOpenPrivateTabOnStartup, &QAction::toggled,
            this, &MainWindow::setOpenPrivateTabOnStartupEnabled);
    connect(m_actionGroups.subsequentLaunch, &QActionGroup::triggered,
            this, &MainWindow::setSubsequentLaunchAction);
    connect(m_actions.settingsBehaviorSyntaxHighlighting, SIGNAL(toggled(bool)), SLOT(setSyntaxHighlightingEnabled(bool)));
    connect(m_actions.settingsEditingInputDigitGrouping, &QAction::toggled,
            this, &MainWindow::setInputDigitGroupingEnabled);
    connect(m_actions.settingsEditingAutomaticInputAdjustments, &QAction::toggled,
            this, &MainWindow::setAutomaticInputAdjustmentsEnabled);
    connect(m_actions.settingsEditingCursorBlinking, &QAction::toggled,
            this, &MainWindow::setCursorBlinkingEnabled);
    connect(m_actions.settingsEditingEscapeClearsEditor, &QAction::toggled,
            this, &MainWindow::setEscapeClearsEditorEnabled);
    connect(m_actions.settingsBehaviorHoverHighlightResults, SIGNAL(toggled(bool)), SLOT(setHoverHighlightResultsEnabled(bool)));
    connect(m_actionGroups.digitGrouping, SIGNAL(triggered(QAction*)), SLOT(setDigitGrouping(QAction*)));
    connect(m_actions.settingsBehaviorDigitGroupingIntegerPartOnly, SIGNAL(toggled(bool)), SLOT(setDigitGroupingIntegerPartOnlyEnabled(bool)));
    connect(m_actions.settingsBehaviorLeaveLastExpression, SIGNAL(toggled(bool)), SLOT(setLeaveLastExpressionEnabled(bool)));
    connect(m_actions.settingsBehaviorNumberFormat, SIGNAL(triggered()), SLOT(showNumberFormatDialog()));
    connectToActiveWindow(m_actions.settingsBehaviorResultSlots, &MainWindow::showResultSlotsDialog);
    connect(m_actionGroups.upDownArrowBehavior, SIGNAL(triggered(QAction*)), SLOT(setUpDownArrowBehavior(QAction*)));
    connect(m_actions.settingsBehaviorAutoResultToClipboard, SIGNAL(toggled(bool)), SLOT(setAutoResultToClipboardEnabled(bool)));
    connect(m_actions.settingsBehaviorSimplifyResultExpressions, SIGNAL(toggled(bool)), SLOT(setSimplifyResultExpressionsEnabled(bool)));
    connect(m_actions.settingsRadixCharComma, SIGNAL(triggered()), SLOT(setRadixCharacterComma()));
    connect(m_actions.settingsRadixCharDefault, SIGNAL(triggered()), SLOT(setRadixCharacterAutomatic()));
    connect(m_actions.settingsRadixCharDot, SIGNAL(triggered()), SLOT(setRadixCharacterDot()));
    connect(m_actions.settingsRadixCharBoth, SIGNAL(triggered()), SLOT(setRadixCharacterBoth()));

    connect(m_actions.settingsResultFormat0Digits, &QAction::triggered, this, [this]() {
        activeMainWindowForMenuAction(this)->setResultPrecision(0);
    });
    connectToActiveWindow(m_actions.settingsResultFormat15Digits, &MainWindow::setResultPrecision15Digits);
    connectToActiveWindow(m_actions.settingsResultFormat2Digits, &MainWindow::setResultPrecision2Digits);
    connectToActiveWindow(m_actions.settingsResultFormat3Digits, &MainWindow::setResultPrecision3Digits);
    connectToActiveWindow(m_actions.settingsResultFormat50Digits, &MainWindow::setResultPrecision50Digits);
    connectToActiveWindow(m_actions.settingsResultFormat8Digits, &MainWindow::setResultPrecision8Digits);
    connectToActiveWindow(m_actions.settingsResultFormatCustomDigits, &MainWindow::setResultPrecisionCustom);
    connectToActiveWindow(m_actions.settingsResultFormatAutoPrecision, &MainWindow::setResultPrecisionAutomatic);
    connectToActiveWindow(m_actions.settingsResultFormatBinary, &MainWindow::setResultFormatBinary);
    connect(m_actions.settingsResultFormatCartesian, SIGNAL(triggered()), SLOT(setResultFormatCartesian()));
    connectToActiveWindow(m_actions.settingsResultFormatEngineering, &MainWindow::setResultFormatEngineering);
    connectToActiveWindow(m_actions.settingsResultFormatFixed, &MainWindow::setResultFormatFixed);
    connectToActiveWindow(m_actions.settingsResultFormatGeneral, &MainWindow::setResultFormatGeneral);
    connectToActiveWindow(m_actions.settingsResultFormatHexadecimal, &MainWindow::setResultFormatHexadecimal);
    connect(m_actions.settingsImaginaryUnitI, SIGNAL(triggered()), SLOT(setImaginaryUnitI()));
    connect(m_actions.settingsImaginaryUnitJ, SIGNAL(triggered()), SLOT(setImaginaryUnitJ()));
    connectToActiveWindow(m_actions.settingsResultFormatOctal, &MainWindow::setResultFormatOctal);
    connect(m_actions.settingsResultFormatPolar, SIGNAL(triggered()), SLOT(setResultFormatPolar()));
    connect(m_actions.settingsResultFormatTrigonometric, SIGNAL(triggered()), SLOT(setResultFormatTrigonometric()));
    connect(m_actions.settingsResultFormatCis, SIGNAL(triggered()), SLOT(setResultFormatCis()));
    connect(m_actions.settingsResultFormatPolarAngle, SIGNAL(triggered()), SLOT(setResultFormatPolarAngle()));
    connectToActiveWindow(m_actions.settingsResultFormatRational, &MainWindow::setResultFormatRational);
    connectToActiveWindow(m_actions.settingsResultFormatSexagesimal, &MainWindow::setResultFormatSexagesimal);
    connectToActiveWindow(m_actions.settingsResultFormatScientific, &MainWindow::setResultFormatScientific);
    connect(m_actionGroups.unitNegativeExponentStyle, SIGNAL(triggered(QAction*)),
            SLOT(setUnitNegativeExponentStyle(QAction*)));
    connect(m_actionGroups.resultRoundingMode, SIGNAL(triggered(QAction*)),
            SLOT(setResultRoundingMode(QAction*)));
    connect(m_actions.settingsRemoveTrailingZeroes, &QAction::toggled,
            this, &MainWindow::setRemoveTrailingZeroesEnabled);

    connect(m_actions.settingsLanguage, SIGNAL(triggered()), SLOT(showLanguageChooserDialog()));
    connectToActiveWindow(m_actions.settingsResetAll, &MainWindow::resetAllSettings);

    connect(m_actions.helpManual, SIGNAL(triggered()), SLOT(showManualWindow()));
    connect(m_actions.contextHelp, SIGNAL(triggered()), SLOT(showContextHelp()));
#ifdef SPEEDCRUNCH_ENABLE_UPDATE_CHECKS
    connect(m_actions.helpUpdates, SIGNAL(triggered()), SLOT(checkForUpdates()));
#endif
    connect(m_actions.helpFeedback, SIGNAL(triggered()), SLOT(openFeedbackURL()));
    connect(m_actions.helpCommunity, SIGNAL(triggered()), SLOT(openCommunityURL()));
    connect(m_actions.helpSource, SIGNAL(triggered()), SLOT(openSourceURL()));
    connect(m_actions.helpDonate, SIGNAL(triggered()), SLOT(openDonateURL()));
    connect(m_actions.helpAbout, SIGNAL(triggered()), SLOT(showAboutDialog()));
    connect(m_actions.helpDiagnostics, &QAction::triggered, this, &MainWindow::showDiagnosticsDialog);

    connect(m_widgets.editor, &Editor::autoCalcDisabled, this, [this]() {
        if (sender() == m_widgets.editor)
            hideStateLabel();
    });
    connect(m_widgets.editor, SIGNAL(autoCalcMessageAvailable(const QString&)), SLOT(handleAutoCalcMessageAvailable(const QString&)));
    connect(m_widgets.editor, SIGNAL(autoCalcQuantityAvailable(const Quantity&)), SLOT(handleAutoCalcQuantityAvailable(const Quantity&)));
    connect(m_widgets.editor, SIGNAL(returnPressed()), SLOT(evaluateEditorExpression()));
    connect(m_widgets.editor, SIGNAL(escapePressed()), SLOT(handleEditorEscapePressed()));
    connect(m_widgets.editor, SIGNAL(shiftDownPressed()), SLOT(decreaseDisplayFontPointSize()));
    connect(m_widgets.editor, SIGNAL(shiftUpPressed()), SLOT(increaseDisplayFontPointSize()));
    connect(m_widgets.editor, SIGNAL(controlPageUpPressed()), m_widgets.display, SLOT(scrollToTop()));
    connect(m_widgets.editor, SIGNAL(controlPageDownPressed()), m_widgets.display, SLOT(scrollToBottom()));
    connect(m_widgets.editor, SIGNAL(shiftPageUpPressed()), m_widgets.display, SLOT(scrollLineUp()));
    connect(m_widgets.editor, SIGNAL(shiftPageDownPressed()), m_widgets.display, SLOT(scrollLineDown()));
    connect(m_widgets.editor, SIGNAL(pageUpPressed()), m_widgets.display, SLOT(scrollPageUp()));
    connect(m_widgets.editor, SIGNAL(pageDownPressed()), m_widgets.display, SLOT(scrollPageDown()));
    connect(initialEditor, &Editor::textChanged, this, [this, initialEditor]() {
        if (initialEditor == m_widgets.editor
            || initialEditor->hasFocus()
            || initialEditor->viewport()->hasFocus()) {
            handleEditorTextChange();
        }
    });
    connect(m_widgets.editor, SIGNAL(copyAvailable(bool)), SLOT(handleCopyAvailable(bool)));
    connect(initialEditor, &Editor::undoAvailable, this, &MainWindow::syncEditMenuActionState);
    connect(initialEditor, &Editor::redoAvailable, this, &MainWindow::syncEditMenuActionState);
    connect(m_widgets.editor, SIGNAL(copySequencePressed()), SLOT(copy()));
    connect(m_widgets.editor, SIGNAL(selectionChanged()), SLOT(handleEditorSelectionChange()));
    connect(this, SIGNAL(historyChanged()), m_widgets.editor, SLOT(updateHistory()));

    connect(m_widgets.display, SIGNAL(copyAvailable(bool)), SLOT(handleCopyAvailable(bool)));
    connect(m_widgets.display, SIGNAL(clicked()), SLOT(hideStateLabel()));
    connect(m_widgets.display, SIGNAL(expressionSelected(const QString&)), SLOT(insertHistoryExpressionIntoEditor(const QString&)));
    connect(m_widgets.display, SIGNAL(editHistoryEntryRequested(int)), SLOT(startHistoryEntryEdit(int)));
    connect(m_widgets.display, SIGNAL(editHistoryEntryContextRequested(int)), SLOT(editHistoryEntryContext(int)));
    connect(m_widgets.display, SIGNAL(cancelHistoryEditRequested()), SLOT(cancelHistoryEntryEdit()));
    connect(m_widgets.display, SIGNAL(removeHistoryEntryRequested(int)), SLOT(removeHistoryEntryAt(int)));
    connect(m_widgets.display, SIGNAL(removeHistoryEntriesAboveRequested(int)), SLOT(removeHistoryEntriesAbove(int)));
    connect(m_widgets.display, SIGNAL(removeHistoryEntriesBelowRequested(int)), SLOT(removeHistoryEntriesBelow(int)));
    connect(m_widgets.display, SIGNAL(newSessionRequested()), SLOT(showNewSessionDialog()));
    connect(m_widgets.display, &ResultDisplay::newPrivateSessionRequested, this, &MainWindow::showNewPrivateSession);
    connect(m_widgets.display, &ResultDisplay::convertSessionPrivacyRequested, this, &MainWindow::convertSessionPrivacy);
    connect(m_widgets.display, SIGNAL(openSessionRequested()), SLOT(showOpenSessionDialog()));
    connect(m_widgets.display, SIGNAL(importSessionRequested()), SLOT(showSessionImportDialog()));
    connect(m_widgets.display, SIGNAL(exportSessionJsonRequested()), SLOT(exportJson()));
    connect(m_widgets.display, SIGNAL(exportSessionPlainTextRequested()), SLOT(exportPlainText()));
    connect(m_widgets.display, SIGNAL(exportSessionHtmlRequested()), SLOT(exportHtml()));
    connect(m_widgets.display, SIGNAL(duplicateSessionRequested()), SLOT(showDuplicateSessionDialog()));
    connect(m_widgets.display, SIGNAL(splitLeftRequested()), SLOT(splitActivePaneLeft()));
    connect(m_widgets.display, SIGNAL(splitRightRequested()), SLOT(splitActivePaneRight()));
    connect(m_widgets.display, SIGNAL(splitUpRequested()), SLOT(splitActivePaneUp()));
    connect(m_widgets.display, SIGNAL(splitDownRequested()), SLOT(splitActivePaneDown()));
    connect(m_widgets.display, SIGNAL(renameSessionRequested()), SLOT(showRenameSessionDialog()));
    connect(m_widgets.display, SIGNAL(clearSessionRequested()), SLOT(clearSession()));
    connect(m_widgets.display, SIGNAL(closeSessionRequested()), SLOT(closeCurrentSession()));
    connect(m_widgets.display, SIGNAL(closePaneRequested()), SLOT(closeCurrentPane()));
    connect(m_widgets.display, SIGNAL(deleteSessionRequested()), SLOT(deleteCurrentSession()));
    connect(m_widgets.display, SIGNAL(loadedSessionsMenuRequested(const QPoint&)), SLOT(showLoadedSessionsMenu(const QPoint&)));
    connect(m_widgets.display, SIGNAL(selectionChanged()), SLOT(handleDisplaySelectionChange()));
    connect(m_widgets.display, SIGNAL(shiftWheelUp()), SLOT(increaseDisplayFontPointSize()));
    connect(m_widgets.display, SIGNAL(shiftWheelDown()), SLOT(decreaseDisplayFontPointSize()));
    connect(m_widgets.display, SIGNAL(controlWheelUp()), SLOT(increaseDisplayFontPointSize()));
    connect(m_widgets.display, SIGNAL(controlWheelDown()), SLOT(decreaseDisplayFontPointSize()));
    connect(m_widgets.display, SIGNAL(shiftControlWheelDown()), SLOT(decreaseOpacity()));
    connect(m_widgets.display, SIGNAL(shiftControlWheelUp()), SLOT(increaseOpacity()));
    connect(this, SIGNAL(historyChanged()), m_widgets.display, SLOT(refresh()));

    connect(this, SIGNAL(radixCharacterChanged()), m_widgets.display, SLOT(refresh()));
    connect(this, SIGNAL(radixCharacterChanged()), m_widgets.editor, SLOT(refreshAutoCalc()));
    connect(this, SIGNAL(angleUnitChanged()), m_widgets.editor, SLOT(refreshAutoCalc()));
    connect(this, SIGNAL(complexNumbersChanged()), m_widgets.display, SLOT(refreshLastHistoryEntry()));
    connect(this, SIGNAL(complexNumbersChanged()), m_widgets.editor, SLOT(refreshAutoCalc()));
    connect(this, SIGNAL(resultFormatChanged()), m_widgets.display, SLOT(refreshLastHistoryEntry()));
    connect(this, SIGNAL(resultFormatChanged()), m_widgets.editor, SLOT(refreshAutoCalc()));
    connect(this, SIGNAL(resultPrecisionChanged()), m_widgets.display, SLOT(refreshLastHistoryEntry()));
    connect(this, SIGNAL(resultPrecisionChanged()), m_widgets.editor, SLOT(refreshAutoCalc()));
    connect(this, SIGNAL(resultRoundingModeChanged()), m_widgets.display, SLOT(refreshLastHistoryEntry()));
    connect(this, SIGNAL(resultRoundingModeChanged()), m_widgets.editor, SLOT(refreshAutoCalc()));
    connect(this, SIGNAL(colorSchemeChanged()), m_widgets.display, SLOT(rehighlight()));
    connect(this, SIGNAL(colorSchemeChanged()), m_widgets.editor, SLOT(rehighlight()));
    connect(this, SIGNAL(syntaxHighlightingChanged()), m_widgets.display, SLOT(rehighlight()));
    connect(this, SIGNAL(syntaxHighlightingChanged()), m_widgets.editor, SLOT(rehighlight()));

    connect(m_actions.settingsDisplayFont, SIGNAL(triggered()), SLOT(showFontDialog()));
    connect(m_actions.settingsDisplayColorSchemeCustom, SIGNAL(triggered()), SLOT(showCustomThemeDialog()));

    connect(this, SIGNAL(languageChanged()), SLOT(retranslateText()));

    const auto bindApplicationShortcut = [this](const QKeySequence& sequence,
                                                const std::function<void()>& handler) {
        QShortcut* shortcut = new QShortcut(sequence, this);
        shortcut->setContext(Qt::ApplicationShortcut);
        connect(shortcut, &QShortcut::activated, this, handler);
    };

#if defined(Q_OS_MACOS)
    bindApplicationShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Right),
                            [this]() { activateNextChild(); });
    bindApplicationShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Left),
                            [this]() { activatePreviousChild(); });
#else
    bindApplicationShortcut(QKeySequence(Qt::CTRL | Qt::Key_PageDown),
                            [this]() { activateNextChild(); });
    bindApplicationShortcut(QKeySequence(Qt::CTRL | Qt::Key_PageUp),
                            [this]() { activatePreviousChild(); });
#endif

    QShortcut* cycleFocusForwardShortcut = new QShortcut(QKeySequence(Qt::Key_F6), this);
    // Each window owns its focus cycle. Application-wide bindings conflict
    // when more than one main window is visible.
    cycleFocusForwardShortcut->setContext(Qt::WindowShortcut);
    connect(cycleFocusForwardShortcut, &QShortcut::activated,
            this, &MainWindow::cycleFocusForward);

    QShortcut* cycleFocusBackwardShortcut =
        new QShortcut(QKeySequence(Qt::SHIFT | Qt::Key_F6), this);
    cycleFocusBackwardShortcut->setContext(Qt::WindowShortcut);
    connect(cycleFocusBackwardShortcut, &QShortcut::activated,
            this, &MainWindow::cycleFocusBackward);

    QShortcut* splitRightShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+\\")), this);
    connect(splitRightShortcut, &QShortcut::activated, this, &MainWindow::splitActivePaneRight);
    QShortcut* splitDownShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+\\")), this);
    connect(splitDownShortcut, &QShortcut::activated, this, &MainWindow::splitActivePaneDown);
}

void MainWindow::applySettings(bool initializing)
{
    StartupDiagnostics::AppearanceContext context("Apply startup settings");
    StartupDiagnostics::Scope timing("Apply appearance and language");
    for (QAction* choice : m_actionGroups.systemColorMode->actions())
        choice->setChecked(choice->data().toInt() == m_settings->systemColorMode);
    applySystemColorMode(m_settings->systemColorMode);
    applyKeypadAppearance();
    m_actions.settingsDockAppearanceSystem->setChecked(
        m_settings->dockAppearance == Settings::DockAppearanceSystem);
    m_actions.settingsDockAppearanceSpeedCrunch->setChecked(
        m_settings->dockAppearance == Settings::DockAppearanceSpeedCrunch);
    m_actions.settingsStatusBarAppearanceSystem->setChecked(
        m_settings->statusBarAppearance == Settings::StatusBarAppearanceSystem);
    m_actions.settingsStatusBarAppearanceSpeedCrunch->setChecked(
        m_settings->statusBarAppearance == Settings::StatusBarAppearanceSpeedCrunch);
    applyEditorDisplayStyle();
    m_actions.settingsMenuAppearanceSystem->setChecked(
        m_settings->menuAppearance == Settings::MenuAppearanceSystem);
    m_actions.settingsMenuAppearanceSpeedCrunch->setChecked(
        m_settings->menuAppearance == Settings::MenuAppearanceSpeedCrunch);
    emit languageChanged();
    m_status.selectedAngleUnit = m_settings->angleUnit;
    m_status.selectedResultFormat = m_settings->resultFormat;
    m_status.selectedResultPrecision = m_settings->resultPrecision;

    // Saved workspaces choose their own docks before session files are read.
    // Other windows use the configured defaults without constructing closed panels.
    if (!initializing)
        applyOpenDocks(configuredOpenDockNames());
    else if (!m_restorePreviousSessionOnStartup)
        restoreWindowLayoutState(m_settings->windowState);
    timing.next("Configure keypad, status bar and menu bar");
    updateKeypadModeActionState();
    switch (m_keypadZoomPercent) {
    case 50:
        m_actions.viewKeypadZoom50->setChecked(true);
        break;
    case 150:
        m_actions.viewKeypadZoom150->setChecked(true);
        break;
    case 200:
        m_actions.viewKeypadZoom200->setChecked(true);
        break;
    case 100:
    default:
        m_actions.viewKeypadZoom100->setChecked(true);
        break;
    }
    setKeypadVisible(isVisibleKeypadMode(m_keypadMode));
    m_menus.keypadZoom->setEnabled(isVisibleKeypadMode(m_keypadMode));
    setStatusBarVisible(m_settings->statusBarVisible);
    m_actions.viewStatusBar->setChecked(m_settings->statusBarVisible);
#if !defined(Q_OS_MACOS)
    setMenuBarVisible(m_settings->menuBarVisible);
    m_actions.viewMenuBar->setChecked(m_settings->menuBarVisible);
#endif

    timing.next("Restore window option actions");
    // A saved workspace restores its geometry while scheduling session I/O.
    // Wait for that result before choosing a fallback size and position.
    m_savedWindowGeometryRestored = false;

    StartupDiagnostics::Scope optionTiming("Restore fullscreen action state");
    m_actions.viewFullScreenMode->setChecked(m_settings->windowOnfullScreen);
    optionTiming.next("Restore always-on-top action state");
    if (!isWaylandPlatform())
        m_actions.settingsBehaviorAlwaysOnTop->setChecked(m_settings->windowAlwaysOnTop);

    optionTiming.next("Restore angle action state");
    if (m_settings->angleUnit == 'r')
        m_actions.settingsAngleUnitRadian->setChecked(true);
    else if (m_settings->angleUnit == 'd')
        m_actions.settingsAngleUnitDegree->setChecked(true);
    else if (m_settings->angleUnit == 'g')
        m_actions.settingsAngleUnitGradian->setChecked(true);
    else if (m_settings->angleUnit == 't')
        m_actions.settingsAngleUnitTurn->setChecked(true);
    else if (m_settings->angleUnit == 'v')
        m_actions.settingsAngleUnitRevolution->setChecked(true);
    optionTiming.finish();

    timing.next("Load global definitions and schedule session restore");
    if (initializing) {
        UserDefinitions::loadInto(m_settings);
        if (m_restorePreviousSessionOnStartup)
            restoreSession();
        applyUserDefinitions();
    }

    timing.next("Restore fallback window geometry");
    if (!m_savedWindowGeometryRestored && !restoreGeometry(m_settings->windowGeometry)) {
        int defaultWidth = 640;
        if (m_widgets.keypad)
            defaultWidth = m_widgets.keypad->sizeHint().width();
        resize(defaultWidth, 480);
        if (QScreen* screen = QGuiApplication::primaryScreen())
            move(screen->availableGeometry().center() - rect().center());
    }

    timing.next("Apply calculation, editing and display preferences");
    m_actions.settingsBehaviorLeaveLastExpression->setChecked(m_settings->leaveLastExpression);
    switch (m_settings->upDownArrowBehavior) {
    case Settings::UpDownArrowBehaviorNever:
        m_actions.settingsBehaviorUpDownArrowNever->setChecked(true);
        break;
    case Settings::UpDownArrowBehaviorSingleLineOnly:
        m_actions.settingsBehaviorUpDownArrowSingleLineOnly->setChecked(true);
        break;
    case Settings::UpDownArrowBehaviorAlways:
    default:
        m_actions.settingsBehaviorUpDownArrowAlways->setChecked(true);
        break;
    }
    m_actions.settingsBehaviorSaveWindowPositionOnExit->setChecked(m_settings->windowPositionSave);
    m_actions.settingsOpenPrivateTabOnStartup->setChecked(m_settings->openPrivateTabOnStartup);
    for (QAction* action : m_actionGroups.subsequentLaunch->actions())
        action->setChecked(action->data().toInt() == int(m_settings->subsequentLaunchAction));


    checkInitialResultFormat();
    checkInitialResultPrecision();
    checkInitialComplexFormat();
    checkInitialImaginaryUnit();
    m_actions.settingsRemoveTrailingZeroes->setChecked(m_settings->removeTrailingZeroes);
    switch (m_settings->resultRoundingMode) {
    case Settings::ResultRoundingHalfEven:
        m_actions.settingsResultRoundingHalfEven->setChecked(true);
        break;
    case Settings::ResultRoundingTowardZero:
        m_actions.settingsResultRoundingTowardZero->setChecked(true);
        break;
    case Settings::ResultRoundingTowardPositiveInfinity:
        m_actions.settingsResultRoundingTowardPositiveInfinity->setChecked(true);
        break;
    case Settings::ResultRoundingTowardNegativeInfinity:
        m_actions.settingsResultRoundingTowardNegativeInfinity->setChecked(true);
        break;
    case Settings::ResultRoundingHalfAwayFromZero:
    default:
        m_actions.settingsResultRoundingHalfAwayFromZero->setChecked(true);
        break;
    }
    setRuntimeResultRoundingMode(m_settings->resultRoundingMode);
    if (m_settings->unitNegativeExponentStyle == Settings::UnitNegativeExponentFraction)
        m_actions.settingsUnitNegativeExponentFraction->setChecked(true);
    else
        m_actions.settingsUnitNegativeExponentSuperscript->setChecked(true);
    setRuntimeUnitNegativeExponentStyle(m_settings->unitNegativeExponentStyle);

    if (m_settings->autoAns)
        m_actions.settingsBehaviorAutoAns->setChecked(true);
    else
        setAutoAnsEnabled(false);

    if (m_settings->autoCalc)
        m_actions.settingsBehaviorPartialResults->setChecked(true);
    else
        setAutoCalcEnabled(false);

    if (m_settings->autoCompletion)
        m_actions.settingsBehaviorAutoCompletion->setChecked(true);
    else
        setAutoCompletionEnabled(false);
    setAutoCompletionAcceptWithEnterEnabled(m_settings->autoCompletionAcceptWithEnter);
    m_actions.settingsBehaviorAutoCompletionBuiltInFunctions->setChecked(
        m_settings->autoCompletionBuiltInFunctions);
    m_actions.settingsBehaviorAutoCompletionBuiltInVariables->setChecked(
        m_settings->autoCompletionBuiltInVariables);
    m_actions.settingsBehaviorAutoCompletionLongFormUnits->setChecked(
        m_settings->autoCompletionLongFormUnits);
    m_actions.settingsBehaviorAutoCompletionUserFunctions->setChecked(
        m_settings->autoCompletionUserFunctions);
    m_actions.settingsBehaviorAutoCompletionUserVariables->setChecked(
        m_settings->autoCompletionUserVariables);

    if (initializing) {
        // The editors already read these preferences during construction.
        // Refresh once after the restored sessions and final palette are ready.
        const QSignalBlocker highlighting(m_actions.settingsBehaviorSyntaxHighlighting);
        const QSignalBlocker grouping(m_actions.settingsEditingInputDigitGrouping);
        const QSignalBlocker adjustments(m_actions.settingsEditingAutomaticInputAdjustments);
        m_actions.settingsBehaviorSyntaxHighlighting->setChecked(m_settings->syntaxHighlighting);
        m_actions.settingsEditingInputDigitGrouping->setChecked(m_settings->inputDigitGrouping);
        m_actions.settingsEditingAutomaticInputAdjustments->setChecked(m_settings->automaticInputAdjustments);
    } else {
        if (m_settings->syntaxHighlighting)
            m_actions.settingsBehaviorSyntaxHighlighting->setChecked(true);
        else
            setSyntaxHighlightingEnabled(false);
        m_actions.settingsEditingInputDigitGrouping->setChecked(m_settings->inputDigitGrouping);
        setAutomaticInputAdjustmentsEnabled(m_settings->automaticInputAdjustments);
    }
    setCursorBlinkingEnabled(m_settings->cursorBlinking);
    setEscapeClearsEditorEnabled(m_settings->escapeClearsEditor);

    if (m_settings->hoverHighlightResults)
        m_actions.settingsBehaviorHoverHighlightResults->setChecked(true);
    else
        setHoverHighlightResultsEnabled(false);

    if (m_settings->autoResultToClipboard)
        m_actions.settingsBehaviorAutoResultToClipboard->setChecked(true);
    else
        setAutoResultToClipboardEnabled(false);

    if (m_settings->simplifyResultExpressions)
        m_actions.settingsBehaviorSimplifyResultExpressions->setChecked(true);
    else
        setSimplifyResultExpressionsEnabled(false);

    QFont font;
    font.fromString(m_settings->displayFont);
    for (ResultDisplay* display : splitPaneDisplays())
        display->setFont(font);
    for (Editor* editor : splitPaneEditors())
        editor->setFont(font);

    if (initializing && m_widgets.display != nullptr)
        m_widgets.display->verticalScrollBar()->setValue(m_widgets.display->verticalScrollBar()->maximum());

    updateColorSchemeActionState();
    updateSplitterStyleSheet();
}

void MainWindow::resetAllSettings()
{
    QMessageBox confirmation(this);
    confirmation.setIcon(QMessageBox::Question);
    confirmation.setWindowTitle(tr("Reset All Settings"));
    confirmation.setText(tr("Reset all settings to their default values?"));
    confirmation.setInformativeText(tr("This includes appearance, editing, result formats, and window preferences. "
                                       "Your calculations, open sessions, and user definitions will be kept."));
    confirmation.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmation.setDefaultButton(QMessageBox::No);
    confirmation.setEscapeButton(QMessageBox::No);
    if (confirmation.exec() != QMessageBox::Yes)
        return;

    saveSessionLayout(false);
    if (!m_settings->resetToDefaults()) {
        QMessageBox::warning(this, tr("Reset All Settings"), tr("The settings file could not be written."));
        return;
    }

    DMath::complexMode = m_settings->complexNumbers;
    CMath::setImaginaryUnitSymbol(m_settings->imaginaryUnit);
    MenuStyle::refresh();
    // Keep QActionGroup's exclusive checks working while suppressing window
    // notifications that would reformat existing calculation history.
    std::vector<std::unique_ptr<QSignalBlocker>> windowBlockers;
    for (const QPointer<MainWindow>& window : allMainWindows()) {
        if (window)
            windowBlockers.push_back(std::make_unique<QSignalBlocker>(window.data()));
    }
    for (const QPointer<MainWindow>& window : allMainWindows()) {
        if (!window)
            continue;

        std::vector<std::unique_ptr<QSignalBlocker>> blockers;
        window->restoreDockColumnOrders(QJsonObject());
        for (QDockWidget* dock : window->m_allDocks)
            blockers.push_back(std::make_unique<QSignalBlocker>(dock));
        // Restore a complete layout to avoid removing live tab groups on Qt 6.10.
        QMainWindow defaultLayout;
        QDockWidget* previousDock = nullptr;
        for (QDockWidget* source : window->m_allDocks) {
            auto* dock = new QDockWidget(&defaultLayout);
            dock->setObjectName(source->objectName());
            const bool bitfield = source == window->m_docks.bitField;
            defaultLayout.addDockWidget(bitfield ? Qt::BottomDockWidgetArea : Qt::RightDockWidgetArea, dock);
            if (!bitfield) {
                if (previousDock)
                    defaultLayout.tabifyDockWidget(previousDock, dock);
                previousDock = dock;
            }
        }
        window->restoreState(defaultLayout.saveState(DockLayoutStateVersion), DockLayoutStateVersion);
        window->applyDockCornerLayout();

        window->deleteKeypad();
        window->m_hasRestoredDockLayout = false;
        window->m_keypadMode = m_settings->keypadMode;
        window->m_keypadZoomPercent = m_settings->keypadZoomPercent;
        window->applySettings(false);
        if (!isWaylandPlatform())
            window->setAlwaysOnTopEnabled(m_settings->windowAlwaysOnTop);
        window->setFullScreenEnabled(m_settings->windowOnfullScreen);

        if (window->m_docks.constants) {
            for (QComboBox* combo : window->m_docks.constants->widget()->findChildren<QComboBox*>())
                combo->setCurrentIndex(0);
            window->m_docks.constants->widget()->restoreState(QString(), QString(), QStringLiteral(""));
        }
        if (window->m_docks.functions) {
            for (QComboBox* combo : window->m_docks.functions->widget()->findChildren<QComboBox*>())
                combo->setCurrentIndex(0);
            window->m_docks.functions->widget()->setSearchText(QString());
        }
        if (window->m_docks.variables)
            window->m_docks.variables->widget()->setSearchText(QString());
        if (window->m_docks.userFunctions)
            window->m_docks.userFunctions->widget()->setSearchText(QString());
        if (window->m_docks.userUnits)
            window->m_docks.userUnits->widget()->setSearchText(QString());
        if (window->m_docks.book)
            window->m_docks.book->openPage(QUrl(m_settings->formulaBookActivePage));
        if (window->m_widgets.manual)
            window->m_widgets.manual->resize(640, 480);

        for (ResultDisplay* display : window->splitPaneDisplays())
            display->setHoverHighlightEnabled(m_settings->hoverHighlightResults);
        for (Editor* editor : window->splitPaneEditors()) {
            editor->setAutoCalcEnabled(m_settings->autoCalc);
            editor->setAutoCompletionEnabled(m_settings->autoCompletion);
            editor->refreshAutoCalc();
        }
        window->setStatusBarText();
        window->syncStatusBarSelectionMenuActionState();
    }
    windowBlockers.clear();
    for (const QPointer<MainWindow>& window : allMainWindows()) {
        if (!window)
            continue;
        emit window->languageChanged();
        emit window->colorSchemeChanged();
        emit window->menuAppearanceChanged();
    }
    saveSessionLayout(false);
    m_settings->save();
}

void MainWindow::showManualWindow()
{
    if (m_widgets.manual) {
        m_widgets.manual->raise();
        m_widgets.manual->activateWindow();
        return;
    }

    m_widgets.manual = new ManualWindow();
    if (!m_widgets.manual->restoreGeometry(m_settings->manualWindowGeometry))
        m_widgets.manual->resize(640, 480);
    m_widgets.manual->show();
    connect(m_widgets.manual, SIGNAL(windowClosed()), SLOT(handleManualClosed()));
}

void MainWindow::showContextHelp()
{
    QString kw = "";
    if(m_widgets.editor->hasFocus()) {
        kw = m_widgets.editor->getKeyword();
        if (kw != "") {
            auto url = m_manualServer->urlForKeyword(kw);
            if (url.isValid()) {
                showManualWindow();
                m_widgets.manual->openPage(url);
            }
        }
    }
}

void MainWindow::checkInitialResultFormat()
{
    switch (m_settings->resultFormat) {
        case 'g': m_actions.settingsResultFormatGeneral->setChecked(true); break;
        case 'n': m_actions.settingsResultFormatEngineering->setChecked(true); break;
        case 'e': m_actions.settingsResultFormatScientific->setChecked(true); break;
        case 'r': m_actions.settingsResultFormatRational->setChecked(true); break;
        case 'h': m_actions.settingsResultFormatHexadecimal->setChecked(true); break;
        case 'o': m_actions.settingsResultFormatOctal->setChecked(true); break;
        case 'b': m_actions.settingsResultFormatBinary->setChecked(true); break;
        case 's': m_actions.settingsResultFormatSexagesimal->setChecked(true); break;
        default : m_actions.settingsResultFormatFixed->setChecked(true);
    }
}

void MainWindow::checkInitialComplexFormat()
{
    m_settings->complexNumbers = true;
    m_settings->secondaryComplexNumbers = true;
    m_settings->tertiaryComplexNumbers = true;
    m_settings->quaternaryComplexNumbers = true;
    m_settings->quinaryComplexNumbers = true;
    DMath::complexMode = true;

    if (m_settings->resultComplexForm == ComplexForm::Exponential)
        m_actions.settingsResultFormatPolar->setChecked(true);
    else if (m_settings->resultComplexForm == ComplexForm::Trigonometric)
        m_actions.settingsResultFormatTrigonometric->setChecked(true);
    else if (m_settings->resultComplexForm == ComplexForm::Cis)
        m_actions.settingsResultFormatCis->setChecked(true);
    else if (m_settings->resultComplexForm == ComplexForm::Phasor)
        m_actions.settingsResultFormatPolarAngle->setChecked(true);
    else
        m_actions.settingsResultFormatCartesian->setChecked(true);
}

void MainWindow::checkInitialImaginaryUnit()
{
    if (m_settings->imaginaryUnit == 'j')
        m_actions.settingsImaginaryUnitJ->setChecked(true);
    else
        m_actions.settingsImaginaryUnitI->setChecked(true);
}

void MainWindow::checkInitialResultPrecision()
{
    switch (m_settings->resultPrecision) {
        case 0: m_actions.settingsResultFormat0Digits->setChecked(true); break;
        case 2: m_actions.settingsResultFormat2Digits->setChecked(true); break;
        case 3: m_actions.settingsResultFormat3Digits->setChecked(true); break;
        case 8: m_actions.settingsResultFormat8Digits->setChecked(true); break;
        case 15: m_actions.settingsResultFormat15Digits->setChecked(true); break;
        case 50: m_actions.settingsResultFormat50Digits->setChecked(true); break;
        case -1: m_actions.settingsResultFormatAutoPrecision->setChecked(true); break;
        default: m_actions.settingsResultFormatCustomDigits->setChecked(true); break;
    }
}

void MainWindow::checkInitialDigitGrouping()
{
    switch (m_settings->digitGrouping) {
        case 1: m_actions.settingsBehaviorDigitGroupingOneSpace->setChecked(true); break;
        case 2: m_actions.settingsBehaviorDigitGroupingTwoSpaces->setChecked(true); break;
        case 3: m_actions.settingsBehaviorDigitGroupingThreeSpaces->setChecked(true); break;
        default:
        case 0: m_actions.settingsBehaviorDigitGroupingNone->setChecked(true); break;
    }
}



void MainWindow::saveSettings()
{
    if (m_docks.constants) {
        m_settings->constantsDockDomain = m_docks.constants->widget()->selectedDomain();
        m_settings->constantsDockSubdomain = m_docks.constants->widget()->selectedSubdomain();
        m_settings->constantsDockSearchText = m_docks.constants->widget()->searchText();
    }
    if (m_docks.functions) {
        m_settings->functionsDockDomain = m_docks.functions->widget()->selectedDomain();
        m_settings->functionsDockSearchText = m_docks.functions->widget()->searchText();
    }
    if (m_docks.userFunctions)
        m_settings->userFunctionsDockSearchText = m_docks.userFunctions->widget()->searchText();
    if (m_docks.userUnits)
        m_settings->userUnitsDockSearchText = m_docks.userUnits->widget()->searchText();
    if (m_docks.variables)
        m_settings->variablesDockSearchText = m_docks.variables->widget()->searchText();
    if (m_docks.book)
        m_settings->formulaBookActivePage = m_docks.book->currentPage();

    // These values seed a window only when no session-layout record exists.
    // Use the active window rather than assigning ownership to the first one.
    MainWindow* defaultWindow = activeMainWindowForMenuAction(this);
    m_settings->windowGeometry = m_settings->windowPositionSave
        ? defaultWindow->saveGeometry()
        : QByteArray();
    m_settings->windowState = defaultWindow->saveState(DockLayoutStateVersion);
    m_settings->dockColumnOrdersJson = QString::fromUtf8(
        QJsonDocument(defaultWindow->dockColumnOrders()).toJson(QJsonDocument::Compact));
    const QStringList openDocks = defaultWindow->openDockNames();
    m_settings->formulaBookDockVisible = openDocks.contains(QStringLiteral("BookDock"));
    m_settings->constantsDockVisible = openDocks.contains(QStringLiteral("ConstantsDock"));
    m_settings->functionsDockVisible = openDocks.contains(QStringLiteral("FunctionsDock"));
    m_settings->historyDockVisible = openDocks.contains(QStringLiteral("HistoryDock"));
    m_settings->variablesDockVisible = openDocks.contains(QStringLiteral("VariablesDock"));
    m_settings->userFunctionsDockVisible = openDocks.contains(QStringLiteral("UserFunctionsDock"));
    m_settings->userUnitsDockVisible = openDocks.contains(QStringLiteral("UserUnitsDock"));
    m_settings->bitfieldVisible = openDocks.contains(QStringLiteral("BitfieldDock"));
    m_settings->keypadMode = defaultWindow->m_keypadMode;
    m_settings->keypadVisible = defaultWindow->m_widgets.keypad != nullptr;
    m_settings->keypadZoomPercent = defaultWindow->m_keypadZoomPercent;
    if (m_widgets.manual)
        m_settings->manualWindowGeometry = m_settings->windowPositionSave ? m_widgets.manual->saveGeometry() : QByteArray();
    if (m_widgets.display != nullptr)
        m_settings->displayFont = m_widgets.display->font().toString();

    m_settings->save();
}

void MainWindow::saveSession(QString & fname)
{
    captureEditorTextInCurrentSession();
    QJsonObject json;
    m_session->serialize(json);
    writeSessionJsonToFile(json, fname);
}

void MainWindow::saveSessionLayout(bool captureCurrentViewport, bool captureEditorState)
{
    const bool includeEditorState = captureEditorState || m_shutdownStateSaved || applicationShutdownInProgress();
    const auto editorForDisplay = [](ResultDisplay* display) -> Editor* {
        QWidget* page = display ? display->parentWidget() : nullptr;
        return page ? page->findChild<Editor*>(QString(), Qt::FindDirectChildrenOnly) : nullptr;
    };

    if (captureCurrentViewport) {
        QSet<MainWindow*> capturedWindows;
        for (const QPointer<MainWindow>& ptr : allMainWindows()) {
            MainWindow* window = ptr.data();
            if (window == nullptr || capturedWindows.contains(window))
                continue;

            capturedWindows.insert(window);
            window->captureVisibleSessionViewports();
        }
    }

    QStringList sessionNames;
    for (ResultDisplay* display : splitPaneDisplays()) {
        const QStringList names = paneSessionNames(display);
        for (const QString& name : names) {
            if (!name.isEmpty() && !sessionNames.contains(name, Qt::CaseInsensitive))
                sessionNames.append(name);
        }
    }
    if (sessionNames.isEmpty())
        sessionNames = m_loadedSessions.keys();
    sessionNames.sort(Qt::CaseInsensitive);

    QJsonArray tabs;
    const QString activePaneSessionName = m_widgets.display != nullptr
        ? m_paneSessionNames.value(m_widgets.display)
        : QString();
    const QJsonObject activeEditorState = includeEditorState
        ? editorState(editorForDisplay(m_widgets.display))
        : QJsonObject();
    for (const QString& name : sessionNames)
        tabs.append(sessionLayoutEntry(name,
                                       m_sessionViewportAnchors.value(name, qMakePair(-1, 0)),
                                       m_sessionScrollValues.value(name, -1),
                                       activeEditorState,
                                       includeEditorState && name.compare(activePaneSessionName, Qt::CaseInsensitive) == 0));

    const auto layoutNodeForWidget = [this, includeEditorState, &editorForDisplay](QWidget* widget, const auto& layoutNodeForWidgetRef) -> QJsonObject {
        QJsonObject node;
        if (widget == nullptr)
            return node;

        if (QSplitter* splitter = qobject_cast<QSplitter*>(widget)) {
            QJsonArray children;
            for (int i = 0; i < splitter->count(); ++i) {
                const QJsonObject child = layoutNodeForWidgetRef(splitter->widget(i), layoutNodeForWidgetRef);
                if (!child.isEmpty())
                    children.append(child);
            }

            QJsonArray sizes;
            for (int size : splitter->sizes())
                sizes.append(size);

            node.insert(QStringLiteral("type"), QStringLiteral("split"));
            node.insert(QStringLiteral("orientation"),
                        splitter->orientation() == Qt::Horizontal
                            ? QStringLiteral("horizontal")
                            : QStringLiteral("vertical"));
            node.insert(QStringLiteral("sizes"), sizes);
            node.insert(QStringLiteral("children"), children);
            return node;
        }

        ResultDisplay* display = widget->findChild<ResultDisplay*>();
        if (display == nullptr)
            return node;

        const QString paneSessionName = m_paneSessionNames.value(display);
        QJsonArray paneTabs;
        const QStringList paneNames = paneSessionNames(display);
        const QJsonObject paneEditorState = includeEditorState
            ? editorState(editorForDisplay(display))
            : QJsonObject();
        for (const QString& name : paneNames) {
            paneTabs.append(sessionLayoutEntry(name,
                                               m_sessionViewportAnchors.value(name, qMakePair(-1, 0)),
                                               m_sessionScrollValues.value(name, -1),
                                               paneEditorState,
                                               includeEditorState && name.compare(paneSessionName, Qt::CaseInsensitive) == 0));
        }

        node.insert(QStringLiteral("type"), QStringLiteral("pane"));
        node.insert(QStringLiteral("active"), paneSessionName);
        node.insert(QStringLiteral("tabs"), paneTabs);
        return node;
    };

    QJsonObject root;
    const QList<ResultDisplay*> displays = splitPaneDisplays();
    if (m_widgets.splitContainer != nullptr && displays.size() > 1) {
        root = layoutNodeForWidget(m_widgets.splitContainer, layoutNodeForWidget);
        root.insert(QStringLiteral("active"), m_session ? m_session->name() : QString());
        root.insert(QStringLiteral("tabs"), tabs);
    } else {
        root.insert(QStringLiteral("type"), QStringLiteral("tabs"));
        root.insert(QStringLiteral("active"), m_session ? m_session->name() : QString());
        root.insert(QStringLiteral("tabs"), tabs);
    }

    QJsonArray windows;
    int windowIndex = 0;
    QSet<MainWindow*> uniqueWindows;
    for (const QPointer<MainWindow>& ptr : allMainWindows()) {
        MainWindow* windowObject = ptr.data();
        if (windowObject == nullptr || windowObject->m_session == nullptr)
            continue;
        if (uniqueWindows.contains(windowObject))
            continue;
        uniqueWindows.insert(windowObject);

        QJsonArray windowTabs;
        QStringList names;
        for (ResultDisplay* display : windowObject->splitPaneDisplays()) {
            for (const QString& name : windowObject->paneSessionNames(display)) {
                if (!name.isEmpty() && !names.contains(name, Qt::CaseInsensitive))
                    names.append(name);
            }
        }
        if (names.isEmpty())
            names = windowObject->m_loadedSessions.keys();
        names.sort(Qt::CaseInsensitive);
        const QString windowActivePaneSessionName = windowObject->m_widgets.display != nullptr
            ? windowObject->m_paneSessionNames.value(windowObject->m_widgets.display)
            : QString();
        const QJsonObject windowActiveEditorState = includeEditorState
            ? editorState(editorForDisplay(windowObject->m_widgets.display))
            : QJsonObject();
        for (const QString& name : names) {
            windowTabs.append(sessionLayoutEntry(name,
                windowObject->m_sessionViewportAnchors.value(name, qMakePair(-1, 0)),
                windowObject->m_sessionScrollValues.value(name, -1),
                windowActiveEditorState,
                includeEditorState && name.compare(windowActivePaneSessionName, Qt::CaseInsensitive) == 0));
        }

        const auto nodeForWidget = [windowObject, includeEditorState, &editorForDisplay](QWidget* widget, const auto& selfRef) -> QJsonObject {
            QJsonObject node;
            if (widget == nullptr)
                return node;
            if (QSplitter* splitter = qobject_cast<QSplitter*>(widget)) {
                QJsonArray children;
                for (int i = 0; i < splitter->count(); ++i) {
                    const QJsonObject child = selfRef(splitter->widget(i), selfRef);
                    if (!child.isEmpty())
                        children.append(child);
                }
                QJsonArray sizes;
                for (int size : splitter->sizes())
                    sizes.append(size);
                node.insert(QStringLiteral("type"), QStringLiteral("split"));
                node.insert(QStringLiteral("orientation"),
                            splitter->orientation() == Qt::Horizontal ? QStringLiteral("horizontal") : QStringLiteral("vertical"));
                node.insert(QStringLiteral("sizes"), sizes);
                node.insert(QStringLiteral("children"), children);
                return node;
            }
            ResultDisplay* display = widget->findChild<ResultDisplay*>();
            if (display == nullptr)
                return node;
            node.insert(QStringLiteral("type"), QStringLiteral("pane"));
            const QString paneSessionName = windowObject->m_paneSessionNames.value(display);
            node.insert(QStringLiteral("active"), paneSessionName);
            QJsonArray paneTabs;
            const QJsonObject paneEditorState = includeEditorState
                ? editorState(editorForDisplay(display))
                : QJsonObject();
            for (const QString& name : windowObject->paneSessionNames(display)) {
                paneTabs.append(sessionLayoutEntry(name,
                    windowObject->m_sessionViewportAnchors.value(name, qMakePair(-1, 0)),
                    windowObject->m_sessionScrollValues.value(name, -1),
                    paneEditorState,
                    includeEditorState && name.compare(paneSessionName, Qt::CaseInsensitive) == 0));
            }
            node.insert(QStringLiteral("tabs"), paneTabs);
            return node;
        };

        QJsonObject windowRoot;
        const QList<ResultDisplay*> windowDisplays = windowObject->splitPaneDisplays();
        if (windowObject->m_widgets.splitContainer != nullptr && windowDisplays.size() > 1) {
            windowRoot = nodeForWidget(windowObject->m_widgets.splitContainer, nodeForWidget);
            windowRoot.insert(QStringLiteral("active"), windowObject->m_session ? windowObject->m_session->name() : QString());
            windowRoot.insert(QStringLiteral("tabs"), windowTabs);
        } else {
            windowRoot.insert(QStringLiteral("type"), QStringLiteral("tabs"));
            windowRoot.insert(QStringLiteral("active"), windowObject->m_session ? windowObject->m_session->name() : QString());
            windowRoot.insert(QStringLiteral("tabs"), windowTabs);
        }

        QJsonObject window;
        const QString id = QStringLiteral("window-%1").arg(windowIndex++);
        window.insert(QStringLiteral("id"), id);
        window.insert(QStringLiteral("active"), windowObject == this);
        window.insert(QStringLiteral("root"), windowRoot);
        const QStatusBar* windowStatusBar =
            windowObject->findChild<QStatusBar*>(QString(), Qt::FindDirectChildrenOnly);
        // isVisible() also becomes false when the top-level window is hidden
        // during shutdown. isHidden() records only the status bar's own choice.
        window.insert(QStringLiteral("statusBarVisible"),
                      windowStatusBar != nullptr && !windowStatusBar->isHidden());
        window.insert(QStringLiteral("statusBarAngleUnit"),
                      QString(QChar::fromLatin1(windowObject->m_status.selectedAngleUnit)));
        window.insert(QStringLiteral("statusBarResultFormat"),
                      QString(QChar::fromLatin1(windowObject->m_status.selectedResultFormat)));
        window.insert(QStringLiteral("statusBarResultPrecision"),
                      windowObject->m_status.selectedResultPrecision);
        window.insert(QStringLiteral("bitfieldVisible"),
                      windowObject->m_docks.bitField != nullptr && !windowObject->m_docks.bitField->isHidden());
        window.insert(QStringLiteral("openDocks"), QJsonArray::fromStringList(windowObject->openDockNames()));
        window.insert(QStringLiteral("dockColumnOrders"), windowObject->dockColumnOrders());
        const bool keypadVisible = windowObject->m_widgets.keypad != nullptr;
        window.insert(QStringLiteral("keypadVisible"), keypadVisible);
        if (keypadVisible)
            window.insert(QStringLiteral("keypadMode"), static_cast<int>(windowObject->m_keypadMode));
        window.insert(QStringLiteral("keypadZoomPercent"), windowObject->m_keypadZoomPercent);
        window.insert(QStringLiteral("windowState"),
                      QString::fromLatin1(windowObject->saveState(DockLayoutStateVersion).toBase64()));
        if (windowObject->m_settings->windowPositionSave)
            window.insert(QStringLiteral("geometry"), QString::fromLatin1(windowObject->saveGeometry().toBase64()));
        windows.append(window);
    }

    QJsonObject layout;
    layout.insert(QStringLiteral("scheme"), 1);
    layout.insert(QStringLiteral("kind"), QStringLiteral("session-layout"));
    layout.insert(QStringLiteral("activeWindow"), QStringLiteral("window-0"));
    QSet<QString> privateNames;
    for (const QPointer<MainWindow>& window : allMainWindows()) {
        if (window) {
            for (Session* session : window->m_loadedSessions) {
                if (session->isPrivate())
                    privateNames.insert(session->name());
            }
        }
    }
    QJsonArray persistentWindows;
    for (const QJsonValue& value : windows) {
        QJsonObject window = value.toObject();
        const QJsonObject persistentRoot = persistentSessionLayout(window.value(QStringLiteral("root")).toObject(), privateNames);
        if (!persistentRoot.isEmpty()) {
            window.insert(QStringLiteral("root"), persistentRoot);
            persistentWindows.append(window);
        }
    }
    if (!persistentWindows.isEmpty())
        layout.insert(QStringLiteral("activeWindow"), persistentWindows.first().toObject().value(QStringLiteral("id")));
    layout.insert(QStringLiteral("windows"), persistentWindows);

    m_settings->sessionLayoutJson = QString::fromUtf8(
        QJsonDocument(layout).toJson(QJsonDocument::Compact));
    m_settings->saveSessionLayoutJson();
}

void MainWindow::openImportedSession(Session* session)
{
    if (session == nullptr)
        return;

    if (m_sessionSavePending)
        flushPendingSessionSave();

    const QString name = firstAvailableImportedSessionName(session->name(), m_loadedSessions);
    session->setName(name);
    m_loadedSessions.insert(name, session);
    updatePaneLoadedSessionCounts();
    applyUserDefinitions();
    activateSession(session);

    QString importedSessionPath = sessionFilePath(name);
    saveSession(importedSessionPath);
    saveSessionLayout(false);
}

void MainWindow::activateSession(Session* session)
{
    StartupDiagnostics::AppearanceContext context("Activate session");
    if (session == nullptr)
        return;

    const bool previousSessionIsLoaded =
        m_session != nullptr && m_loadedSessions.values().contains(m_session);
    if (previousSessionIsLoaded && m_session != session) {
        if (m_sessionSavePending)
            flushPendingSessionSave();
        captureEditorTextInCurrentSession();
        if (m_widgets.display != nullptr
                && m_paneSessionNames.value(m_widgets.display) == m_session->name()) {
            m_sessionViewportAnchors.insert(m_session->name(), m_widgets.display->viewportTopAnchor());
            QScrollBar* bar = m_widgets.display->verticalScrollBar();
            const int scrollValue = bar->value() == bar->maximum()
                ? (std::numeric_limits<int>::max)()
                : bar->value();
            m_sessionScrollValues.insert(m_session->name(), scrollValue);
        }
    }

    const bool displayAlreadyShowsSession =
        m_widgets.display != nullptr && m_widgets.display->session() == session;

    m_session = session;
    m_evaluator = m_session->evaluator();
    m_evaluator->initializeBuiltInVariables();
    if (m_widgets.display != nullptr)
        m_widgets.display->setSession(m_session);
    if (m_widgets.editor != nullptr)
        m_widgets.editor->setSession(m_session);
    if (m_docks.history)
        m_docks.history->widget()->setSession(m_session);
    if (m_docks.variables)
        m_docks.variables->widget()->setEvaluator(m_evaluator);
    if (m_docks.userFunctions)
        m_docks.userFunctions->widget()->setEvaluator(m_evaluator);
    if (m_docks.userUnits)
        m_docks.userUnits->widget()->setEvaluator(m_evaluator);
    if (m_widgets.display != nullptr)
        addSessionToActivePane(m_session->name());
    m_pendingHistoryEditIndex = -1;
    if (m_widgets.display != nullptr)
        m_widgets.display->setEditingHistoryIndex(-1);
    if (m_widgets.editor != nullptr)
        m_widgets.editor->setHistoryArrowNavigationEnabled(true);
    if (!displayAlreadyShowsSession)
        restoreEditorTextFromCurrentSession();
    if (m_widgets.editor != nullptr)
        m_widgets.editor->updateHistory();
    if (m_widgets.display != nullptr && !displayAlreadyShowsSession)
        m_widgets.display->refresh();
    if (m_docks.history)
        m_docks.history->widget()->updateHistory();
    emit variablesChanged();
    emit functionsChanged();
    emit unitsChanged();
    QTimer::singleShot(0, this, [this]() {
        emit variablesChanged();
        emit functionsChanged();
        emit unitsChanged();
    });
    if (m_widgets.display == nullptr) {
        m_conditions.autoAns = !m_session->historyIsEmpty();
        updatePaneEditorCursorVisibility();
        return;
    }

    m_widgets.display->viewport()->update();
    if (!displayAlreadyShowsSession) {
        const QPair<int, int> anchor = m_sessionViewportAnchors.value(m_session->name(), qMakePair(-1, 0));
        const int scrollValue = m_sessionScrollValues.value(m_session->name(), -1);
        if (anchor.first >= 0) {
            m_widgets.display->restoreViewportTopAnchor(anchor);
            QTimer::singleShot(0, this, [this, anchor]() {
                m_widgets.display->restoreViewportTopAnchor(anchor);
            });
        }
        if (scrollValue >= 0) {
            m_widgets.display->restoreScrollValue(scrollValue);
            QTimer::singleShot(0, this, [this, scrollValue]() {
                m_widgets.display->restoreScrollValue(scrollValue);
            });
        }
    }
    m_conditions.autoAns = !m_session->historyIsEmpty();
    updatePaneEditorCursorVisibility();
}

void MainWindow::captureEditorTextInCurrentSession()
{
    if (m_widgets.editor == nullptr)
        return;

    Session* session = nullptr;
    if (m_widgets.display != nullptr) {
        const QString paneSessionName = m_paneSessionNames.value(m_widgets.display);
        session = m_loadedSessions.value(paneSessionName, nullptr);
    }
    if (session == nullptr)
        session = m_session;
    if (session == nullptr)
        return;

    session->setEditorText(m_widgets.editor->text());
}

void MainWindow::restoreEditorTextFromCurrentSession()
{
    if (m_session == nullptr || m_widgets.editor == nullptr)
        return;

    m_widgets.editor->setText(m_session->editorText());
    m_widgets.editor->setCursorPosition(m_widgets.editor->text().size());
    if (m_widgets.editor->text().trimmed().isEmpty() && m_widgets.bitField)
        m_widgets.bitField->clear();
    else
        m_widgets.editor->refreshAutoCalc();
    QTimer::singleShot(0, this, [this]() {
        if (m_widgets.editor)
            m_widgets.editor->refreshAutoCalc();
    });
    m_widgets.editor->setFocus();
}

void MainWindow::initializeApplicationStyle()
{
    StartupDiagnostics::AppearanceContext context("Initialize application style");
    // Set the application style before any main-window widgets are created.
    applySystemColorMode(Settings::instance()->systemColorMode);
    ensureDockChromeStyleInstalled();
}

MainWindow::MainWindow(bool restorePreviousSession)
    : QMainWindow()
    , m_restorePreviousSessionOnStartup(restorePreviousSession)
    , m_sessionRestorePending(restorePreviousSession)
{
    StartupDiagnostics::AppearanceContext context("Initial window construction");
    StartupDiagnostics::Scope windowTiming("Construct main window (total)");
    StartupDiagnostics::Scope timing("Register window");
    qApp->setQuitOnLastWindowClosed(false);
    applyDockCornerLayout();
    setAttribute(Qt::WA_Hover, true);
    setMouseTracking(true);
    if (primaryMainWindow().isNull())
        primaryMainWindow() = this;
    allMainWindows().append(QPointer<MainWindow>(this));
    if (objectName().isEmpty())
        setObjectName(QStringLiteral("window-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    windowIds().insert(objectName(), QPointer<MainWindow>(this));

    timing.next("Create initial session and evaluator");
    m_session = new Session();
    m_session->setName(firstAvailableUntitledSessionName(m_loadedSessions));
    m_loadedSessions.insert(m_session->name(), m_session);
    timing.next("Load constants repository");
    m_constants = Constants::instance();
    m_evaluator = m_session->evaluator();
    timing.next("Load functions repository");
    m_functions = FunctionRepo::instance();
    m_evaluator->initializeBuiltInVariables();

    timing.next("Initialize window state and menu style");
    m_translator = 0;
    m_settings = Settings::instance();
    // Changing flags later can recreate the native window and repolish its
    // children. Choose the saved flag before constructing the interface.
    if (!isWaylandPlatform() && m_settings->windowAlwaysOnTop)
        setWindowFlag(Qt::WindowStaysOnTopHint);
    m_dockColumnOrders = QJsonDocument::fromJson(m_settings->dockColumnOrdersJson.toUtf8()).object();
    m_startupPrivateTabPending = restorePreviousSession && this == primaryMainWindow()
        && !g_restoringExtraWindows && m_settings->openPrivateTabOnStartup;
    if (g_restoringExtraWindows && primaryMainWindow()
        && primaryMainWindow()->m_startupPrivateTabPending) {
        setAttribute(Qt::WA_ShowWithoutActivating);
    }
    MenuStyle::install();
    m_keypadMode = m_settings->keypadMode;
    m_keypadZoomPercent = m_settings->keypadZoomPercent;
    DMath::complexMode = m_settings->complexNumbers;
    CMath::setImaginaryUnitSymbol(m_settings->imaginaryUnit);

    m_widgets.manual = 0;
    m_widgets.keypad  = 0;
    m_widgets.bitField = nullptr;
    m_widgets.keypadContainer = 0;

    m_conditions.autoAns = false;

    m_docks.book = 0;
    m_docks.history = 0;
    m_docks.constants = 0;
    m_docks.functions = 0;
    m_docks.variables = 0;
    m_docks.userFunctions = 0;
    m_docks.userUnits = 0;
    m_docks.bitField = 0;

    m_status.angleUnit = 0;
    m_status.angleUnitSection = 0;
    m_status.angleUnitLabel = 0;
    m_status.resultFormat = 0;
    m_status.resultFormatSection = 0;
    m_status.resultFormatLabel = 0;
    m_status.resultPrecisionSeparator = 0;
    m_status.resultPrecision = 0;
    m_status.resultPrecisionSection = 0;
    m_status.resultPrecisionLabel = 0;
    m_status.angleUnitSeparator = 0;

    m_copyWidget = 0;
    m_pendingHistoryEditIndex = -1;
    m_shutdownStateSaved = false;
#ifdef SPEEDCRUNCH_ENABLE_UPDATE_CHECKS
    m_versionCheck = 0;
#endif
    m_deferredSessionSaveTimer = new QTimer(this);
    m_deferredSessionSaveTimer->setSingleShot(true);
    m_deferredSessionSaveTimer->setInterval(200);
    m_sessionSavePending = false;
    m_bulkEvaluationInProgress = false;
    m_bulkHistoryChanged = false;
    m_bulkVariablesChanged = false;
    m_bulkFunctionsChanged = false;
    m_bulkUnitsChanged = false;
    connect(m_deferredSessionSaveTimer, &QTimer::timeout, this, &MainWindow::flushPendingSessionSave);
    connect(qApp, &QApplication::focusChanged, this, &MainWindow::handleApplicationFocusChanged);
    qApp->installEventFilter(this);

    timing.next("Initialize window theme background");
    // A late parent stylesheet repolishes every menu, pane and editor below it.
    // Final theme application still refreshes palettes after settings restore.
    applyWindowThemeBackground(this, generatedSurfaceColors(m_settings).window);
    timing.next("Create main UI (total)");
    createUi();
    timing.next("Apply startup settings (total)");
    applySettings();
    timing.next("Finalize theme and editor focus");
    StartupDiagnostics::AppearanceContext finalThemeContext("Finalize window theme");
    m_initializingUi = false;
    applyThemeSurfacePalette();
    if (!m_sessionRestorePending)
        refreshPaneThemes();
    // Keep direct window construction working without the startup initializer.
    ensureDockChromeStyleInstalled();
    updatePaneLoadedSessionCounts();
    // Set the initial focus after parenting the editor and restoring the docks.
    m_widgets.editor->setFocus(Qt::OtherFocusReason);

    if (!m_settings->hasNumberFormatStyleSetting)
        QTimer::singleShot(0, this, SLOT(showNumberFormatDialog()));

#ifdef SPEEDCRUNCH_ENABLE_UPDATE_CHECKS
    timing.next("Initialize update checker");
    m_versionCheck = new VersionCheck(this, this);
    m_versionCheck->checkForUpdateAfterFirstPaint();
#endif

    timing.next("Initialize manual server");
    m_manualServer = ManualServer::instance();
    connect(this, SIGNAL(languageChanged()), m_manualServer, SLOT(ensureCorrectLanguage()));
}

MainWindow::~MainWindow()
{
    // QWidget's destructor can change focus and hide docks before QObject
    // disconnects us. Stop these callbacks while MainWindow is still alive.
    disconnect(qApp, &QApplication::focusChanged, this, &MainWindow::handleApplicationFocusChanged);
    for (QDockWidget* dock : std::as_const(m_allDocks))
        disconnect(dock, nullptr, this, nullptr);
    qApp->removeEventFilter(this);
    windowIds().remove(objectName());
    allMainWindows().removeAll(QPointer<MainWindow>(this));
    if (!applicationShutdownInProgress())
        updatePaneEditorCursorVisibility();
    if (m_docks.book)
        deleteBookDock();
    if (m_docks.constants)
        deleteConstantsDock();
    if (m_docks.variables)
        deleteVariablesDock();
    if (m_docks.userFunctions)
        deleteUserFunctionsDock();
    if (m_docks.userUnits)
        deleteUserUnitsDock();
    if (m_docks.functions)
        deleteFunctionsDock();
    if (m_docks.history)
        deleteHistoryDock();
    qDeleteAll(m_loadedSessions);
    m_session = nullptr;
}

void MainWindow::showAboutDialog()
{
    AboutBox dialog(this);
    dialog.resize(480, 640);
    dialog.exec();
}

void MainWindow::showDiagnosticsDialog()
{
    int windows = 0;
    int panes = 0;
    int sessions = 0;
    qint64 historyEntries = 0;
    qint64 variables = 0;
    qint64 functions = 0;
    qint64 units = 0;
    for (const auto& pointer : allMainWindows()) {
        if (MainWindow* window = pointer.data()) {
            ++windows;
            panes += window->splitPaneDisplays().size();
            sessions += window->m_loadedSessions.size();
            for (const Session* session : window->m_loadedSessions) {
                historyEntries += session->historySize();
                variables += session->variablesToList().size();
                functions += session->UserFunctionsToList().size();
                units += session->userUnitsToList().size();
            }
        }
    }
    const QString workload = QStringLiteral(
        "Open windows: %1\nVisible session panes: %2\nLoaded sessions: %3\n"
        "Loaded history entries: %4\nVariables: %5\nUser functions: %6\nUser units: %7\n")
        .arg(windows).arg(panes).arg(sessions).arg(historyEntries)
        .arg(variables).arg(functions).arg(units);
    DiagnosticsDialog dialog(appDiagnosticsReport(workload), this);
    dialog.exec();
}

void MainWindow::clearHistory()
{
    if (m_session->historyIsEmpty())
        return;

    QMessageBox confirmation(this);
    confirmation.setIcon(QMessageBox::Question);
    confirmation.setWindowTitle(tr("Clear History"));
    confirmation.setText(tr("Are you sure you want to clear the calculation history?"));
    confirmation.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmation.setDefaultButton(QMessageBox::No);
    confirmation.setEscapeButton(QMessageBox::No);
    QShortcut clearHistoryEscape(QKeySequence(Qt::Key_Escape), &confirmation);
    connect(&clearHistoryEscape, &QShortcut::activated, &confirmation, &QMessageBox::reject);
    if (confirmation.exec() != QMessageBox::Yes)
        return;

    m_session->clearHistory();
    m_pendingHistoryEditIndex = -1;
    m_widgets.display->setEditingHistoryIndex(-1);
    m_widgets.editor->setHistoryArrowNavigationEnabled(true);
    clearEditorAndBitfield();
    emit historyChanged();

    m_conditions.autoAns = false;
}

void MainWindow::clearSession()
{
    if (m_session->historyIsEmpty()
            && m_session->variablesToList().isEmpty()
            && m_session->UserFunctionsToList().isEmpty()
            && m_session->userUnitsToList().isEmpty()
            && m_widgets.editor->text().isEmpty()) {
        return;
    }

    QMessageBox confirmation(this);
    confirmation.setIcon(QMessageBox::Question);
    confirmation.setWindowTitle(tr("Clear Session"));
    confirmation.setText(tr("Clear this session's calculations, unfinished expression, variables, functions, and units?"));
    confirmation.setInformativeText(tr("Global user definitions are kept."));
    confirmation.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmation.setDefaultButton(QMessageBox::No);
    confirmation.setEscapeButton(QMessageBox::No);
    QShortcut clearSessionEscape(QKeySequence(Qt::Key_Escape), &confirmation);
    connect(&clearSessionEscape, &QShortcut::activated, &confirmation, &QMessageBox::reject);
    if (confirmation.exec() != QMessageBox::Yes)
        return;

    m_session->clearHistory();
    m_session->clearVariables();
    m_session->clearUserFunctions();
    m_session->clearUserUnits();
    m_pendingHistoryEditIndex = -1;
    m_widgets.display->setEditingHistoryIndex(-1);
    m_widgets.editor->setHistoryArrowNavigationEnabled(true);
    clearEditorAndBitfield();
    m_evaluator->initializeBuiltInVariables();
    applyUserDefinitions();
    emit historyChanged();
    emit variablesChanged();
    emit functionsChanged();
    emit unitsChanged();

    m_conditions.autoAns = false;
    saveSessionToDefaultPath();
}

void MainWindow::showNewSessionDialog()
{
    createUntitledSession();
    saveSessionLayout(false);
}

QString MainWindow::firstAvailableSessionNameAcrossWindows(bool privateSession) const
{
    QHash<QString, Session*> sessions;
    for (const QPointer<MainWindow>& window : allMainWindows()) {
        if (window) {
            for (auto it = window->m_loadedSessions.constBegin(); it != window->m_loadedSessions.constEnd(); ++it)
                sessions.insert(it.key(), it.value());
        }
    }
    if (!privateSession)
        return firstAvailableUntitledSessionName(sessions);
    for (int number = 1; number < (std::numeric_limits<int>::max)(); ++number) {
        const QString name = QStringLiteral("Private-%1").arg(number);
        if (!loadedSessionNameExists(sessions, name) && !QFileInfo::exists(sessionFilePath(name)))
            return name;
    }
    return firstAvailableImportedSessionName(QStringLiteral("Private"), sessions);
}

void MainWindow::showNewPrivateSession()
{
    Session* session = new Session();
    session->setPrivate(true);
    session->setName(firstAvailableSessionNameAcrossWindows(true));
    m_loadedSessions.insert(session->name(), session);
    applyUserDefinitions();
    activateSession(session);
    // Startup must keep the saved workspace intact while other windows restore.
    if (!m_startupPrivateTabPending)
        saveSessionLayout(false);
}

void MainWindow::finishStartupSessionRestore(bool restoredSession)
{
    StartupDiagnostics::Scope timing("Finalize startup session and focus");
    // A failed load keeps the initial pane, which still needs its final theme.
    if (!restoredSession)
        refreshPaneThemes();
    if (m_startupPrivateTabPending) {
        const bool emptyUntitledSession = untitledSessionNumber(m_session->name()) > 0
            && !sessionHasPersistableContent(m_session)
            && m_widgets.editor->text().trimmed().isEmpty();
        if (restoredSession && !emptyUntitledSession) {
            showNewPrivateSession();
        } else {
            // Reuse a blank tab instead of opening an extra empty ordinary session.
            m_session->setPrivate(true);
            renameLoadedSession(m_session, firstAvailableSessionNameAcrossWindows(true));
            applyUserDefinitions();
        }
        m_startupPrivateTabPending = false;
        if (isVisible()) {
            raise();
            activateWindow();
        }
    }
    // Startup always ends in the active editor, even if a dock input took focus
    // while the saved window layout was being restored.
    {
        StartupDiagnostics::Scope focusTiming("Restore startup editor focus");
        if (isActiveWindow())
            setActiveEditorDisplayPane(m_widgets.display, m_widgets.editor, true);
        else
            m_widgets.editor->setFocus(Qt::OtherFocusReason);
    }
    m_sessionRestorePending = false;
    if (m_showAfterSessionRestore)
        showWhenReady();
}

void MainWindow::showWhenReady()
{
    // File I/O runs in a worker. Showing the placeholder before its reply
    // exposes an empty tab and lets native styles paint an unfinished layout.
    if (m_sessionRestorePending) {
        m_showAfterSessionRestore = true;
        return;
    }
    StartupDiagnostics::Scope timing("Show restored window");
    const bool preserveStartupFocus = testAttribute(Qt::WA_ShowWithoutActivating);
    {
        StartupDiagnostics::Scope showTiming("Show native window and activate layout");
        show();
    }
    timing.next("Restore activation after showing window");
    setAttribute(Qt::WA_ShowWithoutActivating, false);
    if (preserveStartupFocus && primaryMainWindow()) {
        MainWindow* startupWindow = primaryMainWindow();
        QTimer::singleShot(0, startupWindow, [startupWindow]() {
            if (!startupWindow->m_session || !startupWindow->m_session->isPrivate())
                return;
            startupWindow->raise();
            startupWindow->activateWindow();
            startupWindow->m_widgets.editor->setFocus();
        });
    }
}

void MainWindow::setVisible(bool visible)
{
    // An explicit show or hide cancels a display request waiting for disk I/O.
    m_showAfterSessionRestore = false;
    QMainWindow::setVisible(visible);
}

void MainWindow::renameLoadedSession(Session* session, const QString& name)
{
    const QString originalName = session->name();
    session->setName(name);
    for (const QPointer<MainWindow>& window : allMainWindows()) {
        if (!window)
            continue;
        // Old reopen snapshots must not bring back the session's saved identity.
        for (int i = window->m_closedSessionTabs.size() - 1; i >= 0; --i) {
            if (window->m_closedSessionTabs.at(i).name == originalName)
                window->m_closedSessionTabs.removeAt(i);
        }
        if (window->m_loadedSessions.value(originalName) != session)
            continue;
        window->m_loadedSessions.remove(originalName);
        window->m_loadedSessions.insert(name, session);
        if (window->m_sessionViewportAnchors.contains(originalName))
            window->m_sessionViewportAnchors.insert(name, window->m_sessionViewportAnchors.take(originalName));
        if (window->m_sessionScrollValues.contains(originalName))
            window->m_sessionScrollValues.insert(name, window->m_sessionScrollValues.take(originalName));
        for (QString& paneName : window->m_paneSessionNames) {
            if (paneName == originalName)
                paneName = name;
        }
        for (QStringList& names : window->m_paneSessionTabs) {
            for (QString& paneName : names) {
                if (paneName == originalName)
                    paneName = name;
            }
        }
        window->updatePaneTabBars();
    }
}

void MainWindow::convertSessionPrivacy()
{
    if (m_session == nullptr)
        return;
    Session* session = m_session;
    const bool makePrivate = !session->isPrivate();
    if (makePrivate) {
        QMessageBox confirmation(QMessageBox::Warning, tr("Make Session Private"),
            tr("The saved session %1 will be deleted. Its contents will stay open in a private session "
               "and will be discarded when that session is closed or SpeedCrunch exits.")
                .arg(session->name()), QMessageBox::Yes | QMessageBox::Cancel, this);
        confirmation.setDefaultButton(QMessageBox::Cancel);
        confirmation.setEscapeButton(QMessageBox::Cancel);
        confirmation.button(QMessageBox::Yes)->setText(tr("Make Private"));
        if (confirmation.exec() != QMessageBox::Yes)
            return;
    }

    captureEditorTextInCurrentSession();
    // A delayed or running save must not recreate the file after conversion.
    m_deferredSessionSaveTimer->stop();
    m_sessionSavePending = false;
    waitForAsyncSessionIo();
    const QString name = firstAvailableSessionNameAcrossWindows(makePrivate);
    if (makePrivate) {
        const QString path = sessionFilePath(session->name());
        if (QFileInfo::exists(path) && !QFile::remove(path)) {
            QMessageBox::warning(this, tr("Make Session Private"),
                                 tr("Could not delete the saved session %1.").arg(path));
            saveSessionToDefaultPath();
            return;
        }
    } else {
        QJsonObject json;
        session->serialize(json);
        json.insert(QLatin1String(SessionJsonKeys::Session), name);
        if (!writeSessionJsonToFile(json, sessionFilePath(name))) {
            QMessageBox::warning(this, tr("Keep Session"),
                                 tr("Could not save the session. It remains private."));
            return;
        }
    }
    session->setPrivate(makePrivate);
    renameLoadedSession(session, name);
    saveSessionLayout(true, !makePrivate);
}

void MainWindow::discardUnusedPrivateSessions()
{
    for (auto it = m_loadedSessions.begin(); it != m_loadedSessions.end();) {
        Session* session = it.value();
        bool attached = session == m_session;
        for (const QStringList& names : m_paneSessionTabs)
            attached = attached || names.contains(it.key());
        if (session->isPrivate() && !attached) {
            m_sessionViewportAnchors.remove(it.key());
            m_sessionScrollValues.remove(it.key());
            it = m_loadedSessions.erase(it);
            delete session;
        } else {
            ++it;
        }
    }
}

void MainWindow::showNewSessionWindow()
{
    createNewSessionWindow();
}

MainWindow* MainWindow::createNewSessionWindow(bool activate)
{
    if (m_shutdownStateSaved)
        return nullptr;

    MainWindow* window = new MainWindow(false);
    window->setAttribute(Qt::WA_DeleteOnClose, true);

    Session* session = window->m_session;
    if (session != nullptr) {
        const QString name = firstAvailableUntitledSessionNameAcrossWindows(window);
        if (session->name().compare(name, Qt::CaseInsensitive) != 0) {
            window->m_loadedSessions.remove(session->name());
            session->setName(name);
            window->m_loadedSessions.insert(name, session);
        }
        session->setEditorText(QString());
        window->m_sessionViewportAnchors.clear();
        window->m_sessionScrollValues.clear();
        if (window->m_widgets.display != nullptr) {
            window->m_paneSessionNames.clear();
            window->m_paneSessionTabs.clear();
            window->m_paneSessionNames.insert(window->m_widgets.display, name);
            window->m_paneSessionTabs.insert(window->m_widgets.display, QStringList(name));
            window->m_widgets.display->setSession(session);
            window->m_widgets.display->refresh();
        }
        if (window->m_widgets.editor != nullptr) {
            window->m_widgets.editor->setSession(session);
            window->m_widgets.editor->setText(QString());
            window->m_widgets.editor->setCursorPosition(0);
            window->m_widgets.editor->updateHistory();
            window->m_widgets.editor->refreshAutoCalc();
        }
        window->m_conditions.autoAns = false;
        window->updatePaneLoadedSessionCounts();
    }

    window->copyWindowLayoutFrom(this);
    window->resize(size());

    QRect availableGeometry;
    if (QScreen* targetScreen = QGuiApplication::screenAt(frameGeometry().center()))
        availableGeometry = targetScreen->availableGeometry();
    else if (QGuiApplication::primaryScreen() != nullptr)
        availableGeometry = QGuiApplication::primaryScreen()->availableGeometry();

    QPoint targetPos = frameGeometry().topLeft() + QPoint(32, 32);
    if (!availableGeometry.isEmpty()
            && !availableGeometry.contains(QRect(targetPos, window->size()))) {
        targetPos = availableGeometry.center() - QRect(QPoint(0, 0), window->size()).center();
    }
    window->move(targetPos);
    window->setAttribute(Qt::WA_ShowWithoutActivating, !activate);
    window->show();
    window->setAttribute(Qt::WA_ShowWithoutActivating, false);
    if (activate) {
        window->raise();
        window->activateWindow();
    }
    window->saveSessionLayout(false);
    return window;
}

void MainWindow::createNewSessionTab()
{
    if (!m_shutdownStateSaved)
        showNewSessionDialog();
}

MainWindow* MainWindow::subsequentLaunchWindow()
{
    MainWindow* target = activeMainWindowForMenuAction(primaryMainWindow());
    if (target && allMainWindows().contains(QPointer<MainWindow>(target))
        && !target->m_shutdownStateSaved)
        return target;
    for (const QPointer<MainWindow>& window : allMainWindows()) {
        if (window && !window->m_shutdownStateSaved)
            return window;
    }
    return nullptr;
}

bool MainWindow::sessionRestoreInProgress()
{
    for (const QPointer<MainWindow>& window : allMainWindows()) {
        if (window && window->m_sessionRestorePending)
            return true;
    }
    return false;
}

void MainWindow::showOpenSessionDialog()
{
    migrateLegacyHistoryIfNeeded();
    ensureSessionsPath();

    struct SessionFileEntry {
        QString name;
        QString path;
        QJsonObject json;
    };
    QList<SessionFileEntry> entries;

    const QDir dir(sessionsPath());
    const QFileInfoList files = dir.entryInfoList(QStringList(QStringLiteral("*.json")),
                                                  QDir::Files | QDir::Readable,
                                                  QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo& fileInfo : files) {
        QJsonObject json;
        if (!readValidSessionJson(fileInfo.absoluteFilePath(), &json))
            continue;

        SessionFileEntry entry;
        entry.name = normalizedSessionName(json.value(QLatin1String(SessionJsonKeys::Session)).toString());
        entry.path = fileInfo.absoluteFilePath();
        entry.json = json;
        entries.append(entry);
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Open Session"));
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    QListWidget* list = new QListWidget(&dialog);
    for (int i = 0; i < entries.size(); ++i) {
        QListWidgetItem* item = new QListWidgetItem(entries.at(i).name, list);
        item->setData(Qt::UserRole, i);
        if (m_session != nullptr && entries.at(i).name == m_session->name())
            item->setSelected(true);
    }
    layout->addWidget(list);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Open | QDialogButtonBox::Cancel, &dialog);
    QPushButton* openButton = buttons->button(QDialogButtonBox::Open);
    openButton->setEnabled(list->currentItem() != nullptr);
    layout->addWidget(buttons);

    connect(list, &QListWidget::currentItemChanged, &dialog, [openButton](QListWidgetItem* current) {
        openButton->setEnabled(current != nullptr);
    });
    connect(list, &QListWidget::itemDoubleClicked, &dialog, [&dialog](QListWidgetItem*) {
        dialog.accept();
    });
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (list->count() > 0 && list->currentItem() == nullptr)
        list->setCurrentRow(0);
    list->setFocus();

    if (dialog.exec() != QDialog::Accepted || list->currentItem() == nullptr)
        return;

    const int entryIndex = list->currentItem()->data(Qt::UserRole).toInt();
    if (entryIndex < 0 || entryIndex >= entries.size())
        return;

    const SessionFileEntry entry = entries.at(entryIndex);
    if (focusOpenSession(entry.name)) {
        saveSessionLayout(false);
        return;
    }

    Session* selectedSession = m_loadedSessions.value(entry.name, nullptr);
    bool reloadedSession = false;
    if (selectedSession == nullptr) {
        selectedSession = new Session();
        selectedSession->deSerialize(entry.json, false);
        selectedSession->setName(entry.name);
        m_loadedSessions.insert(entry.name, selectedSession);
        updatePaneLoadedSessionCounts();
        reloadedSession = true;
    } else if (selectedSession == m_session) {
        selectedSession->deSerialize(entry.json, false);
        selectedSession->setName(entry.name);
        reloadedSession = true;
    }
    // Deserializing a saved session replaces its symbol tables. Startup global
    // definitions are applied after normal restore, so manual open/reload has
    // to do the same or an empty session appears to have no global symbols.
    if (reloadedSession)
        applyUserDefinitions();

    if (selectedSession != m_session)
        saveSessionToDefaultPath();

    activateSession(selectedSession);
    saveSessionLayout(false);
}

void MainWindow::showDuplicateSessionDialog()
{
    if (m_session == nullptr)
        return;

    captureEditorTextInCurrentSession();

    const QString originalName = normalizedSessionName(m_session->name());
    while (true) {
        bool accepted = false;
        const QString enteredName = QInputDialog::getText(
            this,
            tr("Clone Session"),
            tr("Session name:"),
            QLineEdit::Normal,
            m_session->isPrivate() ? firstAvailableSessionNameAcrossWindows(true)
                                   : originalName + QStringLiteral(" (copy)"),
            &accepted).trimmed();
        if (!accepted)
            return;

        const QString name = normalizedSessionName(enteredName);
        if (loadedSessionNameExists(m_loadedSessions, name) || QFileInfo::exists(sessionFilePath(name))) {
            QMessageBox::warning(this,
                                 tr("Clone Session"),
                                 tr("A session named %1 already exists.").arg(name));
            continue;
        }

        QJsonObject duplicateJson;
        m_session->serialize(duplicateJson);
        duplicateJson.insert(QLatin1String(SessionJsonKeys::Session), name);

        const QString duplicatePath = sessionFilePath(name);
        if (m_session->isPrivate()) {
            Session* duplicate = new Session(*m_session);
            duplicate->setName(name);
            m_loadedSessions.insert(name, duplicate);
            activateSession(duplicate);
            saveSessionLayout(false);
            return;
        }
        QFile duplicateFile(duplicatePath);
        if (!duplicateFile.open(QIODevice::WriteOnly)) {
            QMessageBox::warning(this,
                                 tr("Clone Session"),
                                 tr("Could not create session file %1.").arg(duplicatePath));
            continue;
        }

        const QByteArray sessionData = QJsonDocument(duplicateJson).toJson(QJsonDocument::Compact);
        if (duplicateFile.write(sessionData) != sessionData.size()) {
            duplicateFile.close();
            QFile::remove(duplicatePath);
            QMessageBox::warning(this,
                                 tr("Clone Session"),
                                 tr("Could not write session file %1.").arg(duplicatePath));
            continue;
        }
        duplicateFile.close();

        Session* duplicateSession = new Session();
        duplicateSession->deSerialize(duplicateJson, false);
        duplicateSession->setName(name);
        duplicateSession->setEditorText(m_session->editorText());
        m_loadedSessions.insert(name, duplicateSession);
        updatePaneLoadedSessionCounts();

        activateSession(duplicateSession);
        m_conditions.autoAns = !duplicateSession->historyIsEmpty();
        saveSessionLayout(false);
        return;
    }
}

void MainWindow::showRenameSessionDialog()
{
    if (m_session == nullptr)
        return;

    const QString originalName = m_session->name();
    while (true) {
        bool accepted = false;
        const QString enteredName = QInputDialog::getText(
            this, tr("Rename Session"), tr("Session name:"), QLineEdit::Normal,
            originalName, &accepted).trimmed();
        if (!accepted)
            return;
        const QString name = normalizedSessionName(enteredName);
        if (name == originalName)
            return;

        bool exists = QFileInfo::exists(sessionFilePath(name));
        for (const QPointer<MainWindow>& window : allMainWindows()) {
            if (window)
                exists = exists || loadedSessionNameExists(window->m_loadedSessions, name);
        }
        if (exists) {
            QMessageBox::warning(this, tr("Rename Session"),
                                 tr("A session named %1 already exists.").arg(name));
            continue;
        }
        captureEditorTextInCurrentSession();
        if (!m_session->isPrivate()) {
            m_deferredSessionSaveTimer->stop();
            m_sessionSavePending = false;
            waitForAsyncSessionIo();
            QJsonObject json;
            m_session->serialize(json);
            json.insert(QLatin1String(SessionJsonKeys::Session), name);
            const QString path = sessionFilePath(name);
            if (!writeSessionJsonToFile(json, path)) {
                QMessageBox::warning(this, tr("Rename Session"),
                                     tr("Could not save session file %1.").arg(path));
                continue;
            }
            const QString originalPath = sessionFilePath(originalName);
            if (QFileInfo::exists(originalPath) && !QFile::remove(originalPath)) {
                QFile::remove(path);
                QMessageBox::warning(this, tr("Rename Session"),
                                     tr("Could not delete session file %1.").arg(originalPath));
                continue;
            }
        }
        renameLoadedSession(m_session, name);
        saveSessionLayout();
        return;
    }
}

void MainWindow::closeCurrentSession()
{
    if (m_session == nullptr)
        return;

    QStringList names = paneSessionNames(m_widgets.display);
    if (names.size() <= 1) {
        if (splitPaneDisplays().size() <= 1) {
            const QString closingName = m_session->name();
            rememberClosedSessionTab(m_widgets.display, closingName);
            if (shouldDeleteSessionFileOnClose(closingName, m_session))
                QFile::remove(sessionFilePath(closingName));
            else if (sessionHasPersistableContent(m_session))
                saveSessionToDefaultPath();

            // A child window exists only to host its current session set. When
            // the last tab closes there, close the child window instead of
            // creating a replacement untitled session as the primary window
            // does.
            if (this != primaryMainWindow()) {
                close();
                return;
            }

            Session* replacementSession = createUntitledSession(true);
            QStringList paneNames = paneSessionNames(m_widgets.display);
            paneNames.removeAll(closingName);
            if (replacementSession != nullptr && !paneNames.contains(replacementSession->name(), Qt::CaseInsensitive))
                paneNames.append(replacementSession->name());
            m_paneSessionTabs.insert(m_widgets.display, paneNames);
            m_paneSessionNames.insert(m_widgets.display, replacementSession ? replacementSession->name() : QString());

            Session* closingSession = m_loadedSessions.take(closingName);
            m_sessionViewportAnchors.remove(closingName);
            m_sessionScrollValues.remove(closingName);
            delete closingSession;
            updatePaneLoadedSessionCounts();
            saveSessionLayout(false);
            return;
        }

        QWidget* pane = paneWidgetForDisplay(m_widgets.display);
        QSplitter* parentSplitter = qobject_cast<QSplitter*>(pane ? pane->parentWidget() : nullptr);
        if (parentSplitter == nullptr)
            return;
        const int closingPaneIndex = parentSplitter->indexOf(pane);
        const int nextPaneIndex = closingPaneIndex + 1 < parentSplitter->count()
            ? closingPaneIndex + 1
            : qMax(0, closingPaneIndex - 1);
        QWidget* nextPane = parentSplitter->widget(nextPaneIndex);
        ResultDisplay* nextDisplay = nextPane ? nextPane->findChild<ResultDisplay*>() : nullptr;
        Editor* nextEditor = nextPane ? nextPane->findChild<Editor*>() : nullptr;
        if (nextDisplay == nullptr || nextEditor == nullptr || pane == nextPane)
            return;

        const QString closingName = m_session->name();
        rememberClosedSessionTab(m_widgets.display, closingName);
        captureEditorTextInCurrentSession();
        QTabBar* closingTabBar = displayTabBar(m_widgets.display);
        m_paneSessionNames.remove(m_widgets.display);
        m_paneSessionTabs.remove(m_widgets.display);
        m_paneTabBars.remove(m_widgets.display);
        if (closingTabBar != nullptr)
            m_tabBarDisplays.remove(closingTabBar);
        m_widgets.display = nextDisplay;
        m_widgets.editor = nextEditor;
        m_copyWidget = nextEditor;
        deletePaneAfterSessionTabDrag(pane);
        normalizeSplitContainerTree();
        updatePaneLoadedSessionCounts();
        Session* nextSession = m_loadedSessions.value(m_paneSessionNames.value(nextDisplay), nullptr);
        if (nextSession != nullptr)
            activateSession(nextSession);
        discardUnusedPrivateSessions();
        saveSessionLayout(false);
        return;
    }

    const QString closingName = m_session->name();
    rememberClosedSessionTab(m_widgets.display, closingName);
    if (shouldDeleteSessionFileOnClose(closingName, m_session))
        QFile::remove(sessionFilePath(closingName));
    else if (sessionHasPersistableContent(m_session))
        saveSessionToDefaultPath();

    names.sort(Qt::CaseInsensitive);
    const int closingIndex = names.indexOf(closingName);
    const int nextIndex = closingIndex >= 0 && closingIndex + 1 < names.size()
        ? closingIndex + 1
        : qMax(0, closingIndex - 1);
    const QString nextName = names.value(nextIndex);
    Session* nextSession = m_loadedSessions.value(nextName, nullptr);
    if (nextSession == nullptr || nextSession == m_session)
        return;

    QStringList paneNames = paneSessionNames(m_widgets.display);
    paneNames.removeAll(closingName);
    m_paneSessionTabs.insert(m_widgets.display, paneNames);
    activateSession(nextSession);
    discardUnusedPrivateSessions();
    updatePaneLoadedSessionCounts();
    saveSessionLayout(false);
}

void MainWindow::closeCurrentPane()
{
    if (m_widgets.display == nullptr || m_widgets.editor == nullptr || m_session == nullptr)
        return;

    captureEditorTextInCurrentSession();

    ResultDisplay* closingDisplay = m_widgets.display;
    QWidget* closingPane = paneWidgetForDisplay(closingDisplay);
    if (closingPane == nullptr)
        return;

    QStringList closingNames = paneSessionNames(closingDisplay);
    if (closingNames.isEmpty() && m_session != nullptr)
        closingNames.append(m_session->name());

    const auto saveLoadedSession = [this](const QString& name) {
        Session* session = m_loadedSessions.value(name, nullptr);
        if (session == nullptr || session->isPrivate())
            return;
        if (shouldDeleteSessionFileOnClose(name, session)) {
            QFile::remove(sessionFilePath(name));
            return;
        }
        if (!sessionHasPersistableContent(session))
            return;

        QJsonObject json;
        session->serialize(json);
        QFile file(sessionFilePath(name));
        if (file.open(QIODevice::WriteOnly))
            file.write(QJsonDocument(json).toJson(QJsonDocument::Compact));
    };

    for (const QString& name : closingNames)
        saveLoadedSession(name);

    const QList<ResultDisplay*> displaysBeforeClose = splitPaneDisplays();
    if (displaysBeforeClose.size() <= 1) {
        Session* replacementSession = createUntitledSession(true);
        const QString replacementName = replacementSession ? replacementSession->name() : QString();
        m_paneSessionTabs.insert(closingDisplay, replacementName.isEmpty() ? QStringList() : QStringList(replacementName));
        m_paneSessionNames.insert(closingDisplay, replacementName);

        for (const QString& name : closingNames) {
            if (name == replacementName)
                continue;
            Session* session = m_loadedSessions.take(name);
            m_sessionViewportAnchors.remove(name);
            m_sessionScrollValues.remove(name);
            delete session;
        }

        if (replacementSession != nullptr)
            activateSession(replacementSession);
        updatePaneLoadedSessionCounts();
        saveSessionLayout(false);
        return;
    }

    const int closingDisplayIndex = displaysBeforeClose.indexOf(closingDisplay);
    ResultDisplay* nextDisplay = nullptr;
    if (closingDisplayIndex >= 0 && closingDisplayIndex + 1 < displaysBeforeClose.size())
        nextDisplay = displaysBeforeClose.at(closingDisplayIndex + 1);
    else if (closingDisplayIndex > 0)
        nextDisplay = displaysBeforeClose.at(closingDisplayIndex - 1);
    else {
        for (ResultDisplay* display : displaysBeforeClose) {
            if (display != closingDisplay) {
                nextDisplay = display;
                break;
            }
        }
    }

    if (nextDisplay == nullptr)
        return;

    Editor* nextEditor = nextDisplay->parentWidget()
        ? nextDisplay->parentWidget()->findChild<Editor*>(QString(), Qt::FindDirectChildrenOnly)
        : nullptr;
    Session* nextSession = m_loadedSessions.value(m_paneSessionNames.value(nextDisplay), nullptr);
    if (nextEditor == nullptr || nextSession == nullptr)
        return;

    m_paneSessionNames.remove(closingDisplay);
    m_paneSessionTabs.remove(closingDisplay);
    QTabBar* closingTabBar = displayTabBar(closingDisplay);
    m_paneTabBars.remove(closingDisplay);
    if (closingTabBar != nullptr)
        m_tabBarDisplays.remove(closingTabBar);
    m_widgets.display = nextDisplay;
    m_widgets.editor = nextEditor;
    m_copyWidget = nextEditor;

    deletePaneAfterSessionTabDrag(closingPane);
    normalizeSplitContainerTree();

    const auto referencedByRemainingPanes = [this](const QString& name) {
        for (ResultDisplay* display : splitPaneDisplays()) {
            const QStringList names = paneSessionNames(display);
            if (names.contains(name, Qt::CaseInsensitive))
                return true;
        }
        return false;
    };

    activateSession(nextSession);

    for (const QString& name : closingNames) {
        if (referencedByRemainingPanes(name))
            continue;
        Session* session = m_loadedSessions.take(name);
        m_sessionViewportAnchors.remove(name);
        m_sessionScrollValues.remove(name);
        if (session != m_session)
            delete session;
    }

    updatePaneLoadedSessionCounts();
    updatePaneEditorCursorVisibility();
    saveSessionLayout(false);
}

void MainWindow::deleteCurrentSession()
{
    if (m_session == nullptr)
        return;

    QMessageBox confirmation(this);
    confirmation.setIcon(QMessageBox::Question);
    confirmation.setWindowTitle(tr("Delete Session"));
    confirmation.setText(tr("Are you sure you want to delete this session?"));
    confirmation.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmation.setDefaultButton(QMessageBox::No);
    confirmation.setEscapeButton(QMessageBox::No);
    QShortcut deleteSessionEscape(QKeySequence(Qt::Key_Escape), &confirmation);
    connect(&deleteSessionEscape, &QShortcut::activated, &confirmation, &QMessageBox::reject);
    if (confirmation.exec() != QMessageBox::Yes)
        return;

    const QString deletingName = m_session->name();
    const QString deletingPath = sessionFilePath(deletingName);
    Session* deletingSession = m_session;

    m_loadedSessions.remove(deletingName);
    m_sessionViewportAnchors.remove(deletingName);
    m_sessionScrollValues.remove(deletingName);
    m_session = nullptr;
    m_evaluator = nullptr;

    if (QFileInfo::exists(deletingPath))
        QFile::remove(deletingPath);

    Session* nextSession = nullptr;
    bool createdReplacementSession = false;
    if (m_loadedSessions.isEmpty()) {
        const QString name = firstAvailableUntitledSessionName(m_loadedSessions);
        nextSession = new Session();
        nextSession->setName(name);
        m_loadedSessions.insert(name, nextSession);
        createdReplacementSession = true;
    } else {
        QStringList names = m_loadedSessions.keys();
        names.sort(Qt::CaseInsensitive);
        nextSession = m_loadedSessions.value(names.first(), nullptr);
    }

    updatePaneLoadedSessionCounts();
    activateSession(nextSession);
    for (auto it = m_paneSessionNames.begin(); it != m_paneSessionNames.end(); ++it) {
        if (it.value() == deletingName)
            it.value() = nextSession->name();
    }
    for (auto it = m_paneSessionTabs.begin(); it != m_paneSessionTabs.end(); ++it) {
        QStringList names = it.value();
        names.removeAll(deletingName);
        if (names.isEmpty() && nextSession != nullptr)
            names.append(nextSession->name());
        it.value() = names;
    }
    if (createdReplacementSession)
        applyUserDefinitions();

    delete deletingSession;
    m_conditions.autoAns = false;
    saveSessionLayout(false);
}

void MainWindow::showLoadedSessionsMenu(const QPoint& globalPos)
{
    const QStringList paneNames = paneSessionNames(m_widgets.display);
    if (paneNames.isEmpty())
        return;

    QMenu menu(this);
    QStringList names = paneNames;
    names.sort(Qt::CaseInsensitive);
    for (const QString& name : names) {
        QAction* action = menu.addAction(name);
        action->setCheckable(true);
        action->setChecked(m_session != nullptr && name == m_session->name());
        action->setData(name);
    }

    MenuStyle::apply(&menu);
    QAction* selectedAction = menu.exec(globalPos);
    if (selectedAction == nullptr)
        return;

    const QString name = selectedAction->data().toString();
    Session* selectedSession = m_loadedSessions.value(name, nullptr);
    if (selectedSession == nullptr || selectedSession == m_session)
        return;

    saveSessionToDefaultPath();

    activateSession(selectedSession);
    saveSessionLayout(false);
}

void MainWindow::clearEditor()
{
    m_widgets.editor->clear();
    if (m_widgets.bitField)
        m_widgets.bitField->clear();
    m_widgets.editor->setFocus();
}

void MainWindow::clearEditorAndBitfield()
{
    clearEditor();
}

void MainWindow::copyResultToClipboard()
{
    QClipboard* cb = QApplication::clipboard();
    Quantity q = m_evaluator->getVariable(QLatin1String("ans")).value();
    QString strToCopy(NumberFormatter::format(q));
    strToCopy.replace(UnicodeChars::MinusSign, MathDsl::SubOpAl1);
    cb->setText(strToCopy, QClipboard::Clipboard);
}

void MainWindow::decreaseOpacity()
{
    if (windowOpacity() > 0.4)
        setWindowOpacity(windowOpacity() - 0.1);
}

void MainWindow::increaseOpacity()
{
    if (windowOpacity() < 1.0)
        setWindowOpacity(windowOpacity() + 0.1);
}

void MainWindow::deleteVariables()
{
    m_session->clearVariables();

    if (m_docks.variables)
        m_docks.variables->widget()->updateList();
}

void MainWindow::deleteUserFunctions()
{
    m_session->clearUserFunctions();

    if (m_docks.userFunctions)
        m_docks.userFunctions->widget()->updateList();
}

void MainWindow::setResultPrecision2Digits()
{
    setResultPrecision(2);
}

void MainWindow::setResultPrecision3Digits()
{
    setResultPrecision(3);
}

void MainWindow::setResultPrecision8Digits()
{
    setResultPrecision(8);
}

void MainWindow::setResultPrecision15Digits()
{
    setResultPrecision(15);
}

void MainWindow::setResultPrecision50Digits()
{
    setResultPrecision(50);
}

void MainWindow::setResultPrecisionAutomatic()
{
    setResultPrecision(-1);
}

void MainWindow::setResultPrecisionCustom()
{
    bool ok = false;
    const int current = (m_settings->resultPrecision >= 0) ? m_settings->resultPrecision : 8;
    const int precision = QInputDialog::getInt(this,
        tr("Custom Precision"),
        tr("Fractional digits:"),
        current,
        0,
        50,
        1,
        &ok);

    if (ok)
        setResultPrecision(precision);

    checkInitialResultPrecision();
}

void MainWindow::showCustomThemeDialog()
{
    QDialog dialog(this);
    dialog.setObjectName(QStringLiteral("ThemeDialog"));
    dialog.setWindowTitle(tr("Theme"));

    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    QWidget* themeWidget = new QWidget(&dialog);
    QHBoxLayout* themeLayout = new QHBoxLayout(themeWidget);
    themeLayout->setContentsMargins(0, 0, 0, 0);
    themeLayout->setSpacing(10);

    QGroupBox* lightThemesGroup = new QGroupBox(tr("Light Themes"), themeWidget);
    QVBoxLayout* lightThemesLayout = new QVBoxLayout(lightThemesGroup);
    QListWidget* lightThemeList = new QListWidget(lightThemesGroup);
    lightThemeList->setObjectName(QStringLiteral("LightThemeList"));
    lightThemesLayout->addWidget(lightThemeList);
    themeLayout->addWidget(lightThemesGroup);

    QGroupBox* darkThemesGroup = new QGroupBox(tr("Dark Themes"), themeWidget);
    QVBoxLayout* darkThemesLayout = new QVBoxLayout(darkThemesGroup);
    QListWidget* darkThemeList = new QListWidget(darkThemesGroup);
    darkThemeList->setObjectName(QStringLiteral("DarkThemeList"));
    darkThemesLayout->addWidget(darkThemeList);
    themeLayout->addWidget(darkThemesGroup);
    layout->addWidget(themeWidget);

    QGroupBox* previewGroup = new QGroupBox(tr("Preview"), &dialog);
    QVBoxLayout* previewGroupLayout = new QVBoxLayout(previewGroup);

    QWidget* previewWidget = new QWidget(previewGroup);
    previewWidget->setObjectName(QStringLiteral("ThemePreview"));
    QVBoxLayout* previewLayout = new QVBoxLayout(previewWidget);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    previewLayout->setSpacing(0);

    QWidget* mainPreviewWidget = new QWidget(previewWidget);
    QHBoxLayout* mainPreviewLayout = new QHBoxLayout(mainPreviewWidget);
    mainPreviewLayout->setContentsMargins(0, 0, 0, 0);
    mainPreviewLayout->setSpacing(0);
    QPlainTextEdit* preview = new QPlainTextEdit(mainPreviewWidget);
    preview->setReadOnly(true);
    preview->setFrameShape(QFrame::NoFrame);
    preview->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    preview->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    preview->setPlainText(
        QStringLiteral("cos(2 · π + (3 / 2) · π)\n"
                       "= 0.98512127610111389784\n"
                       "\n"
                       "average({1;2;3;4;5}) ? Calculate the average of the list\n"
                       "= 3\n"
                       "\n"
                       "distance = 42 [km] + 195 [m] → [km] ? Marathon distance\n"
                       "= 42.195 km"
                    )
                );
    auto previewHighlighter = new SyntaxHighlighter(preview);
    QWidget* previewScrollbarTrack = new QWidget(mainPreviewWidget);
    const int previewScrollbarWidth = m_widgets.display
        ? m_widgets.display->verticalScrollBar()->sizeHint().width()
        : previewScrollbarTrack->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    previewScrollbarTrack->setFixedWidth(previewScrollbarWidth);
    QVBoxLayout* previewScrollbarLayout = new QVBoxLayout(previewScrollbarTrack);
    previewScrollbarLayout->setContentsMargins(0, 0, 0, 0);
    previewScrollbarLayout->setSpacing(0);
    QWidget* previewScrollbar = new QWidget(previewScrollbarTrack);
    previewScrollbar->setFixedHeight(28);
    previewScrollbarLayout->addWidget(previewScrollbar);
    previewScrollbarLayout->addStretch();
    mainPreviewLayout->addWidget(preview);
    mainPreviewLayout->addWidget(previewScrollbarTrack);

    previewLayout->addWidget(mainPreviewWidget);

    Editor* editorPreview = new Editor(previewWidget);
    editorPreview->setReadOnly(true);
    editorPreview->setFrameShape(QFrame::NoFrame);
    editorPreview->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    editorPreview->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    editorPreview->setText(QStringLiteral("sqrt(144) + sin(π / 2)"));
    editorPreview->setCustomCursorVisible(true);
    const int editorPreviewHeight = m_widgets.editor
        ? m_widgets.editor->height()
        : editorPreview->fontMetrics().lineSpacing() + 46;
    editorPreview->setFixedHeight(editorPreviewHeight);
    previewLayout->addWidget(editorPreview);
    previewGroupLayout->addWidget(previewWidget);
    layout->addWidget(previewGroup);

    QGroupBox* rolesGroup = new QGroupBox(tr("Colors"), &dialog);
    QGridLayout* roleLayout = new QGridLayout(rolesGroup);
    roleLayout->setHorizontalSpacing(10);
    roleLayout->setVerticalSpacing(6);
    layout->addWidget(rolesGroup);
    QWidget* rolesWidget = rolesGroup;

    QStringList schemeNames = ColorScheme::enumerate();
    QString selectedSchemeName = m_settings->colorScheme == QLatin1String("Custom")
        ? QString()
        : m_settings->colorScheme;
    bool isCustomScheme = selectedSchemeName.isEmpty();
    auto currentScheme = ColorScheme::loadByName(m_settings->colorScheme);
    if (!currentScheme.isValid()) {
        const QJsonDocument customDoc = QJsonDocument::fromJson(m_settings->customColorSchemeJson.toUtf8());
        currentScheme = ColorScheme(customDoc);
    }
    if (!currentScheme.isValid())
        currentScheme = ColorScheme::loadByName(QStringLiteral("Terminal"));
    if (!currentScheme.isValid())
        currentScheme = ColorScheme(QJsonDocument(QJsonObject()));

    QMap<ColorScheme::Role, QColor> colorsByRole;
    const auto roleEntries = ColorScheme::roleNames();
    bool primaryColorExplicit = false;

    QMap<ColorScheme::Role, QPushButton*> roleButtons;
    QPushButton* okButton = nullptr;
    const auto applyColorsToControls = [&colorsByRole, &roleButtons, roleEntries]() {
        for (const auto& roleEntry : roleEntries)
            updateColorButtonStyle(roleButtons.value(roleEntry.second), colorsByRole.value(roleEntry.second));
    };
    const auto setColorsFromScheme =
        [&colorsByRole, &primaryColorExplicit, roleEntries](const ColorScheme& scheme) {
        for (const auto& roleEntry : roleEntries)
            colorsByRole[roleEntry.second] = scheme.colorForRole(roleEntry.second);
        primaryColorExplicit = scheme.hasColorForRole(ColorScheme::Primary);
        if (!primaryColorExplicit) {
            const QColor background = colorsByRole.value(ColorScheme::Background);
            const QColor generatedPrimary = generatePrimaryFromBackground(background);
            if (generatedPrimary.isValid())
                colorsByRole[ColorScheme::Primary] = generatedPrimary;
        }
    };
    setColorsFromScheme(currentScheme);
    const auto colorSchemeJsonObject = [&colorsByRole, &primaryColorExplicit, roleEntries]() {
        QJsonObject object;
        object.insert(QStringLiteral("$schema"), QString::fromLatin1(ColorScheme::SchemaDraft));
        object.insert(QStringLiteral("$id"), QString::fromLatin1(ColorScheme::SchemaId));
        for (const auto& roleEntry : roleEntries) {
            if (roleEntry.second == ColorScheme::Primary && !primaryColorExplicit)
                continue;
            object.insert(roleEntry.first, colorsByRole.value(roleEntry.second).name());
        }
        return object;
    };
    const auto applyPreview = [&colorSchemeJsonObject,
                               preview,
                               previewHighlighter,
                               previewWidget,
                               previewScrollbarTrack,
                               previewScrollbar,
                               editorPreview]() {
        const QJsonObject object = colorSchemeJsonObject();
        const ColorScheme scheme = ColorScheme::fromJsonObject(object);
        const GeneratedThemeSurfaces surfaces = generatedSurfaceColorsForScheme(scheme);
        previewWidget->setAutoFillBackground(true);
        previewWidget->setAttribute(Qt::WA_StyledBackground, true);
        previewWidget->setStyleSheet(QStringLiteral("QWidget#ThemePreview { background-color: %1; }")
                                         .arg(surfaces.result.background.name()));
        previewHighlighter->setColorScheme(ColorScheme::fromJsonObject(scheme.toJsonObject()));
        const QPalette resultPalette = paletteForThemeSurface(preview->palette(), surfaces.result);
        preview->setPalette(resultPalette);
        preview->viewport()->setPalette(resultPalette);
        preview->setStyleSheet(QStringLiteral("QPlainTextEdit { background-color: %1; color: %2; }")
                                   .arg(surfaces.result.background.name(),
                                        surfaces.result.foreground.name()));
        previewHighlighter->rehighlight();
        const ThemeScrollBarColors resultScrollBars = scrollBarColorsForSurfaceIndex(surfaces, 1);
        previewScrollbarTrack->setStyleSheet(QStringLiteral("background-color: %1;")
                                                 .arg(resultScrollBars.track.name()));
        previewScrollbar->setStyleSheet(QStringLiteral("background-color: %1; border-radius: 4px;")
                                            .arg(resultScrollBars.thumb.name()));

        editorPreview->setThemeSurfaceColor(surfaces.editorAndLists.background,
                                            surfaces.result.background);
        editorPreview->setThemePreviewColorScheme(scheme);
        editorPreview->setThemePrimaryColor(surfaces.primary.background, true);
        editorPreview->setCustomCursorVisible(true);
        editorPreview->rehighlight();
    };
    const auto updateThemeListHeight = [](QListWidget* list) {
        const int visibleRows = qMin(list->count(), 7);
        const int rowHeight = list->sizeHintForRow(0) > 0 ? list->sizeHintForRow(0) : list->fontMetrics().height() + 6;
        list->setMaximumHeight(rowHeight * visibleRows + list->frameWidth() * 2);
    };
    const auto restoreThemeListScroll = [&dialog](QListWidget* list, int scrollValue) {
        if (list == nullptr || list->verticalScrollBar() == nullptr)
            return;

        list->verticalScrollBar()->setValue(scrollValue);
        QPointer<QListWidget> guardedList(list);
        QTimer::singleShot(0, &dialog, [guardedList, scrollValue]() {
            if (guardedList != nullptr && guardedList->verticalScrollBar() != nullptr)
                guardedList->verticalScrollBar()->setValue(scrollValue);
        });
    };
    QHash<QListWidget*, int> pressedThemeListScrollValues;
    const auto rememberThemeListScroll = [&](QListWidget* list) {
        if (list != nullptr && list->verticalScrollBar() != nullptr)
            pressedThemeListScrollValues.insert(list, list->verticalScrollBar()->value());
    };
    const auto restorePressedThemeListScroll = [&](QListWidget* list) {
        if (!pressedThemeListScrollValues.contains(list))
            return;

        restoreThemeListScroll(list, pressedThemeListScrollValues.take(list));
    };
    const auto clearThemeListSelection = [&](QListWidget* list) {
        if (list == nullptr)
            return;

        const int scrollValue = list->verticalScrollBar() ? list->verticalScrollBar()->value() : 0;
        const QSignalBlocker blocker(list);
        list->clearSelection();
        restoreThemeListScroll(list, scrollValue);
    };
    connect(lightThemeList, &QListWidget::itemPressed, &dialog, [&, lightThemeList](QListWidgetItem*) {
        rememberThemeListScroll(lightThemeList);
    });
    connect(darkThemeList, &QListWidget::itemPressed, &dialog, [&, darkThemeList](QListWidgetItem*) {
        rememberThemeListScroll(darkThemeList);
    });
    connect(lightThemeList, &QListWidget::itemClicked, &dialog, [&, lightThemeList](QListWidgetItem*) {
        restorePressedThemeListScroll(lightThemeList);
    });
    connect(darkThemeList, &QListWidget::itemClicked, &dialog, [&, darkThemeList](QListWidgetItem*) {
        restorePressedThemeListScroll(darkThemeList);
    });

    const auto populateThemeList = [&](QListWidget* list, ColorSchemeFilter filter) {
        const QSignalBlocker blocker(list);
        list->clear();
        for (const auto& schemeName : schemeNames) {
            const ColorScheme scheme = ColorScheme::loadByName(schemeName);
            if (!scheme.isValid() || !colorSchemeMatchesFilter(scheme, filter))
                continue;
            auto item = new QListWidgetItem(colorSchemeDisplayName(schemeName, scheme), list);
            item->setData(Qt::UserRole, schemeName);
        }
        updateThemeListHeight(list);
    };
    const auto showSelectedTheme = [&](QListWidget* list) {
        if (selectedSchemeName.isEmpty())
            return false;
        for (int row = 0; row < list->count(); ++row) {
            QListWidgetItem* item = list->item(row);
            if (item->data(Qt::UserRole).toString() != selectedSchemeName)
                continue;
            const QSignalBlocker blocker(list);
            list->setCurrentItem(item);
            item->setSelected(true);
            list->setFocus(Qt::OtherFocusReason);
            return true;
        }
        return false;
    };
    const auto populateThemeLists = [&]() {
        schemeNames = ColorScheme::enumerate();
        populateThemeList(lightThemeList, ColorSchemeFilter::Light);
        populateThemeList(darkThemeList, ColorSchemeFilter::Dark);
        if (showSelectedTheme(lightThemeList))
            darkThemeList->clearSelection();
        else if (showSelectedTheme(darkThemeList))
            lightThemeList->clearSelection();
    };

    int roleIndex = 0;
    constexpr int roleRowsPerColumn = 4;
    for (const auto& roleEntry : roleEntries) {
        const ColorScheme::Role role = roleEntry.second;
        QLabel* roleLabel = new QLabel(colorSchemeRoleLabel(role), rolesWidget);
        QPushButton* colorButton = new QPushButton(rolesWidget);
        colorButton->setObjectName(QStringLiteral("ThemeColorButton_%1").arg(roleEntry.first));
        updateColorButtonStyle(colorButton, colorsByRole.value(role));
        roleButtons.insert(role, colorButton);
        const int row = roleIndex % roleRowsPerColumn;
        const int column = (roleIndex / roleRowsPerColumn) * 2;
        roleLayout->addWidget(colorButton, row, column);
        roleLayout->addWidget(roleLabel, row, column + 1);
        connect(colorButton, &QPushButton::clicked, &dialog, [&, role]() {
            const QColor initial = colorsByRole.value(role);
            const QColor chosen = QColorDialog::getColor(initial, &dialog, tr("Select color for %1").arg(colorSchemeRoleLabel(role)));
            if (!chosen.isValid())
                return;
            colorsByRole[role] = chosen;
            if (role == ColorScheme::Primary)
                primaryColorExplicit = true;
            if (role == ColorScheme::Background && !primaryColorExplicit) {
                const QColor generatedPrimary = generatePrimaryFromBackground(chosen);
                if (generatedPrimary.isValid()) {
                    colorsByRole[ColorScheme::Primary] = generatedPrimary;
                    updateColorButtonStyle(roleButtons.value(ColorScheme::Primary), generatedPrimary);
                }
            }
            selectedSchemeName.clear();
            isCustomScheme = true;
            lightThemeList->clearSelection();
            darkThemeList->clearSelection();
            updateColorButtonStyle(roleButtons.value(role), chosen);
            if (okButton != nullptr)
                okButton->setEnabled(false);
            applyPreview();
        });
        ++roleIndex;
    }

    applyPreview();
    populateThemeLists();

    const auto handleThemeSelection = [&](QListWidget* currentList, QListWidget* otherList, QListWidgetItem* current) {
        if (!current)
            return;
        clearThemeListSelection(otherList);
        const QString schemeName = current->data(Qt::UserRole).toString();
        const ColorScheme scheme = ColorScheme::loadByName(schemeName);
        if (!scheme.isValid())
            return;
        selectedSchemeName = schemeName;
        isCustomScheme = false;
        setColorsFromScheme(scheme);
        applyColorsToControls();
        if (okButton != nullptr)
            okButton->setEnabled(true);
        applyPreview();
        restorePressedThemeListScroll(currentList);
    };
    // Clearing a selection leaves the current item in place, so selecting that
    // same theme again must update the preview even when the current item stays.
    connect(lightThemeList, &QListWidget::itemSelectionChanged, &dialog, [&]() {
        handleThemeSelection(lightThemeList, darkThemeList, lightThemeList->selectedItems().value(0));
    });
    connect(darkThemeList, &QListWidget::itemSelectionChanged, &dialog, [&]() {
        handleThemeSelection(darkThemeList, lightThemeList, darkThemeList->selectedItems().value(0));
    });

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    okButton = buttons->button(QDialogButtonBox::Ok);
    QPushButton* importButton = buttons->addButton(tr("Import..."), QDialogButtonBox::ActionRole);
    QPushButton* exportButton = buttons->addButton(tr("Export..."), QDialogButtonBox::ActionRole);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    const auto applyCurrentTheme = [&]() {
        if (isCustomScheme || selectedSchemeName.isEmpty()) {
            const QJsonObject object = colorSchemeJsonObject();
            m_settings->customColorSchemeJson = QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
            m_settings->colorScheme = QStringLiteral("Custom");
        } else {
            m_settings->colorScheme = selectedSchemeName;
        }
        for (const QPointer<MainWindow>& ptr : allMainWindows()) {
            if (MainWindow* window = ptr.data()) {
                window->updateColorSchemeActionState();
                emit window->colorSchemeChanged();
            }
        }
    };
    const auto writableColorSchemesPath = [&]() {
        const auto colorSchemePaths = ColorScheme::fileSystemSearchPaths();
        const QString path = colorSchemePaths.isEmpty() ? QString() : colorSchemePaths.constFirst();
        if (!path.isEmpty())
            QDir().mkpath(path);
        return path;
    };

    connect(importButton, &QPushButton::clicked, this, [&, roleEntries]() {
        const QString filePath = QFileDialog::getOpenFileName(
            this, tr("Import Theme"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            tr("Theme file (*.json);;All files (*)"));
        if (filePath.isEmpty())
            return;
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, tr("Error"), tr("Can't read from file %1").arg(filePath));
            return;
        }
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        const ColorScheme importedScheme(doc);
        if (!importedScheme.isValid()) {
            QMessageBox::critical(this, tr("Error"), tr("Invalid theme file."));
            return;
        }
        const QFileInfo importFileInfo(filePath);
        const QString themeName = importFileInfo.completeBaseName();
        if (ColorScheme::isBuiltInName(themeName)) {
            QMessageBox::critical(
                this,
                tr("Error"),
                tr("Can't import theme \"%1\" because it conflicts with a built-in theme.").arg(themeName));
            return;
        }
        const QString colorSchemesPath = writableColorSchemesPath();
        if (colorSchemesPath.isEmpty()) {
            QMessageBox::critical(this, tr("Error"), tr("Can't find a writable theme folder."));
            return;
        }
        const QString destinationPath = QDir(colorSchemesPath).filePath(themeName + QLatin1String(".json"));
        if (QFileInfo::exists(destinationPath)) {
            const QMessageBox::StandardButton answer = QMessageBox::question(
                this,
                tr("Overwrite Theme"),
                tr("A custom theme named \"%1\" already exists. Do you want to overwrite it?").arg(themeName),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);
            if (answer != QMessageBox::Yes)
                return;
            const QFileInfo destinationFileInfo(destinationPath);
            if (destinationFileInfo.absoluteFilePath() != importFileInfo.absoluteFilePath()
                    && !QFile::remove(destinationPath)) {
                QMessageBox::critical(this, tr("Error"), tr("Can't overwrite theme file %1").arg(destinationPath));
                return;
            }
        }
        if (QFileInfo(destinationPath).absoluteFilePath() != importFileInfo.absoluteFilePath()
                && !QFile::copy(filePath, destinationPath)) {
            QMessageBox::critical(this, tr("Error"), tr("Can't copy theme file to %1").arg(destinationPath));
            return;
        }
        for (const auto& roleEntry : roleEntries) {
            const QColor color = importedScheme.colorForRole(roleEntry.second);
            colorsByRole[roleEntry.second] = color;
        }
        for (const auto& roleEntry : roleEntries)
            updateColorButtonStyle(roleButtons.value(roleEntry.second),
                                   colorsByRole.value(roleEntry.second));
        selectedSchemeName = themeName;
        isCustomScheme = false;
        if (okButton != nullptr)
            okButton->setEnabled(true);
        populateThemeLists();
        applyPreview();
    });
    connect(exportButton, &QPushButton::clicked, this, [&, roleEntries]() {
        QString colorSchemesPath;
        if (!selectedSchemeName.isEmpty()) {
            const QString selectedSchemePath = ColorScheme::filePathForName(selectedSchemeName);
            if (!selectedSchemePath.startsWith(QLatin1Char(':'))) {
                const QFileInfo selectedSchemeInfo(selectedSchemePath);
                if (selectedSchemeInfo.exists())
                    colorSchemesPath = selectedSchemeInfo.absolutePath();
            }
        }
        const auto colorSchemePaths = ColorScheme::fileSystemSearchPaths();
        if (colorSchemesPath.isEmpty()) {
            for (const auto& path : colorSchemePaths) {
                if (QDir(path).exists()) {
                    colorSchemesPath = path;
                    break;
                }
            }
        }
        if (colorSchemesPath.isEmpty() && !colorSchemePaths.isEmpty())
            colorSchemesPath = colorSchemePaths.constFirst();
        if (!colorSchemesPath.isEmpty())
            QDir().mkpath(colorSchemesPath);
        QString filePath = QFileDialog::getSaveFileName(
            this, tr("Export Theme"), colorSchemesPath,
            tr("Theme file (*.json);;All files (*)"));
        if (filePath.isEmpty())
            return;
        if (!filePath.endsWith(QLatin1String(".json"), Qt::CaseInsensitive))
            filePath += QLatin1String(".json");
        const QFileInfo exportFileInfo(filePath);
        if (ColorScheme::isBuiltInName(exportFileInfo.completeBaseName())) {
            QMessageBox::critical(
                this,
                tr("Error"),
                tr("Can't export theme as \"%1\" because it conflicts with a built-in theme.").arg(exportFileInfo.completeBaseName()));
            return;
        }
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QMessageBox::critical(this, tr("Error"), tr("Can't write to file %1").arg(filePath));
            return;
        }
        const QJsonObject object = colorSchemeJsonObject();
        file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
        file.close();
        const ColorScheme exportedScheme = ColorScheme::loadFromFile(filePath);
        if (exportedScheme.isValid()) {
            selectedSchemeName = exportFileInfo.completeBaseName();
            isCustomScheme = false;
            populateThemeLists();
        }
    });

    dialog.setFixedSize(dialog.sizeHint().expandedTo(QSize(720, 560)));

    if (dialog.exec() != QDialog::Accepted) {
        m_actions.settingsDisplayColorSchemeCustom->setChecked(m_settings->colorScheme == QLatin1String("Custom"));
        return;
    }

    applyCurrentTheme();
}

void MainWindow::selectEditorExpression()
{
    activateWindow();
    m_widgets.editor->selectAll();
    m_widgets.editor->setFocus();
}

void MainWindow::hideStateLabel()
{
    if (m_widgets.state->isVisible()
        && m_widgets.state->text().contains(QStringLiteral("Current result:"))) {
        m_currentResultPreviewDismissed = true;
        if (m_widgets.editor != nullptr)
            m_widgets.editor->dismissCurrentAutoCalc();
    }
    m_widgets.state->hide();
}

void MainWindow::hideCurrentResultPreview()
{
    if (m_widgets.state == nullptr
        || !m_widgets.state->isVisible()
        || !m_widgets.state->text().contains(QStringLiteral("Current result:"))) {
        return;
    }

    hideStateLabel();
}

void MainWindow::handleEditorEscapePressed()
{
    if (m_widgets.state->isVisible()) {
        hideStateLabel();
        return;
    }

    cancelHistoryEntryEdit();
    if (m_settings->escapeClearsEditor
        && (m_widgets.editor->hasFocus() || m_widgets.editor->viewport()->hasFocus())) {
        clearEditor();
    }
}

void MainWindow::wrapSelection()
{
    m_widgets.editor->wrapSelection();
}

void MainWindow::exportJson()
{
    if (m_session == nullptr)
        return;

    const QString filters = tr("JSON file (*.json);;Any file (*.*)");
    const QString sessionBaseName = sessionFileBaseName(m_session->name());
    const QString defaultFileName = sessionBaseName + QLatin1String(".json");

    QFileDialog dialog(this, tr("Export session as JSON"), QDir::homePath(), filters);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setDefaultSuffix(QStringLiteral("json"));
    dialog.selectFile(defaultFileName);
    QTimer::singleShot(0, &dialog, [sessionBaseName, &dialog]() {
        if (QLineEdit* fileNameEdit = dialog.findChild<QLineEdit*>())
            fileNameEdit->setSelection(0, sessionBaseName.size());
    });

    if (dialog.exec() != QDialog::Accepted)
        return;

    const QStringList selectedFiles = dialog.selectedFiles();
    if (selectedFiles.isEmpty())
        return;

    QString fname = selectedFiles.constFirst();
    if (fname.isEmpty())
        return;

    QFile file(fname);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, tr("Error"), tr("Can't write to file %1").arg(fname));
        return;
    }

    QJsonObject json;
    captureEditorTextInCurrentSession();
    m_session->serialize(json);
    QJsonDocument doc(json);
    file.write(doc.toJson());

    file.close();
}

void MainWindow::showSessionImportDialog()
{
    const QString filters = tr("JSON file (*.json);;Any file (*.*)");
    QFileDialog dialog(this, tr("Import Session"), QDir::homePath(), filters);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setDefaultSuffix(QStringLiteral("json"));

    if (dialog.exec() != QDialog::Accepted)
        return;

    const QStringList selectedFiles = dialog.selectedFiles();
    if (selectedFiles.isEmpty())
        return;

    const QString fname = selectedFiles.constFirst();
    if (fname.isEmpty())
        return;

    QFile file(fname);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Import Session"), tr("Can't read from file %1").arg(fname));
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        QMessageBox::critical(this,
                              tr("Import Session"),
                              tr("The selected file is not valid JSON: %1").arg(parseError.errorString()));
        return;
    }

    if (!doc.isObject()) {
        QMessageBox::critical(this,
                              tr("Import Session"),
                              tr("The selected file is not a SpeedCrunch session JSON file."));
        return;
    }

    const QJsonObject json = doc.object();
    if (json.contains(QLatin1String("scheme"))) {
        QMessageBox::critical(this,
                              tr("Import Session"),
                              tr("This file uses an obsolete SpeedCrunch session format and cannot be imported."));
        return;
    }

    const QJsonValue schema = json.value(QLatin1String(SessionJsonKeys::Schema));
    if (!schema.isString()) {
        QMessageBox::critical(this,
                              tr("Import Session"),
                              tr("The selected JSON file is missing the required $schema field."));
        return;
    }
    if (schema.toString() != QLatin1String(SessionJsonKeys::SchemaDialect)) {
        QMessageBox::critical(this,
                              tr("Import Session"),
                              tr("The selected JSON file uses an unsupported JSON schema: %1").arg(schema.toString()));
        return;
    }

    const QJsonValue schemaId = json.value(QLatin1String(SessionJsonKeys::Id));
    if (!schemaId.isString()) {
        QMessageBox::critical(this,
                              tr("Import Session"),
                              tr("The selected JSON file is missing the SpeedCrunch session schema identifier ($id)."));
        return;
    }
    if (schemaId.toString() != QLatin1String(SessionJsonKeys::SchemaId)) {
        QMessageBox::critical(this,
                              tr("Import Session"),
                              tr("The selected JSON file uses an unsupported SpeedCrunch session format: %1").arg(schemaId.toString()));
        return;
    }

    const QJsonValue sessionName = json.value(QLatin1String(SessionJsonKeys::Session));
    if (!sessionName.isString()) {
        QMessageBox::critical(this,
                              tr("Import Session"),
                              tr("The selected JSON file is missing the required session name."));
        return;
    }
    if (sessionName.toString().trimmed().isEmpty()) {
        QMessageBox::critical(this,
                              tr("Import Session"),
                              tr("The selected JSON file has an empty session name."));
        return;
    }

    std::unique_ptr<Session> importedSession(new Session());
    if (!importedSession->deSerialize(json, false)) {
        QMessageBox::critical(this,
                              tr("Import Session"),
                              tr("The selected JSON file has invalid or incomplete SpeedCrunch session data."));
        return;
    }

    openImportedSession(importedSession.release());
}

void MainWindow::openSessionsFolder()
{
    if (!ensureSessionsPath()) {
        QMessageBox::critical(this,
                              tr("Open Sessions Folder"),
                              tr("Could not create the sessions folder: %1").arg(sessionsPath()));
        return;
    }

    const QUrl sessionsFolderUrl = QUrl::fromLocalFile(sessionsPath());
    if (!QDesktopServices::openUrl(sessionsFolderUrl)) {
        QMessageBox::critical(this,
                              tr("Open Sessions Folder"),
                              tr("Could not open the sessions folder: %1").arg(sessionsPath()));
    }
}

void MainWindow::importUserDefinitionsFromText(const QString& text, bool overwriteExisting,
                                               int* importedVariables, int* importedFunctions, int* importedUnits,
                                               int* ignoredLines, QList<int>* ignoredLineNumbers,
                                               bool dryRun)
{
    const QString globalVariableTag = tr("Global User Variable");
    const QString globalFunctionTag = tr("Global User Function");
    const QString globalUnitTag = tr("Global User Unit");
    int localImportedVariables = 0;
    int localImportedFunctions = 0;
    int localImportedUnits = 0;
    int localIgnoredLines = 0;
    QList<int> localIgnoredLineNumbers;
    Session sessionBackup;
    bool autoAnsBackup = false;

    if (dryRun && m_session) {
        sessionBackup = *m_session;
        autoAnsBackup = m_conditions.autoAns;
    }
    m_evaluator->setAllowGlobalUserDefinitionsOverride(true);

    const auto hasGlobalTag = [](const QString& description, const QString& tag) {
        return description.contains(tag);
    };

    const QList<Variable> existingVariables = m_evaluator->getUserDefinedVariables();
    for (const Variable& variable : existingVariables) {
        if (hasGlobalTag(variable.description(), globalVariableTag))
            m_evaluator->unsetVariable(variable.identifier());
    }

    const QList<UserFunction> existingFunctions = m_evaluator->getUserFunctions();
    for (const UserFunction& function : existingFunctions) {
        if (hasGlobalTag(function.description(), globalFunctionTag))
            m_evaluator->unsetUserFunction(function.name());
    }

    const QList<UserUnit> existingUnits = m_evaluator->getUserUnits();
    for (const UserUnit& unit : existingUnits) {
        if (hasGlobalTag(unit.description(), globalUnitTag))
            m_evaluator->unsetUserUnit(unit.name());
    }

    m_evaluator->clearGlobalUserDefinitionRegistry();

    QString inputText = text;
    QTextStream stream(&inputText, QIODevice::ReadOnly);
    int lineNumber = 0;
    while (!stream.atEnd()) {
        const QString rawLine = stream.readLine();
        ++lineNumber;
        const QString normalizedExpression =
            EditorUtils::normalizeExpressionOperators(rawLine).trimmed();
        if (normalizedExpression.isEmpty())
            continue;

        const QString expression = m_evaluator->autoFix(normalizedExpression);
        if (expression.isEmpty() || Evaluator::isCommentOnlyExpression(expression))
            continue;
        QString expressionWithoutDescription = expression;
        QString explicitDescription;
        splitAssignmentDescriptionForImport(
            expression,
            &expressionWithoutDescription,
            &explicitDescription);

        const AssignmentTarget target =
            assignmentTargetFromExpression(m_evaluator, expression);
        if (!target.valid) {
            ++localIgnoredLines;
            localIgnoredLineNumbers.append(lineNumber);
            continue;
        }

        const bool hasExistingVariable = m_evaluator->hasVariable(target.identifier);
        const bool hasExistingUserVariable =
            hasExistingVariable && !m_evaluator->isBuiltInVariable(target.identifier);
        const bool hasExistingUserFunction =
            m_evaluator->hasUserFunction(target.identifier);
        bool hasExistingUserUnit =
            m_evaluator->hasUserUnit(target.identifier);
        if (hasExistingUserUnit) {
            const UserUnit* existingUnit = m_evaluator->getUserUnit(target.identifier);
            if (!existingUnit || existingUnit->value().isZero()) {
                m_evaluator->unsetUserUnit(target.identifier);
                hasExistingUserUnit = false;
            }
        }

        if (!overwriteExisting && (hasExistingVariable || hasExistingUserFunction || hasExistingUserUnit)) {
            ++localIgnoredLines;
            localIgnoredLineNumbers.append(lineNumber);
            continue;
        }

        Variable previousVariable;
        UserFunction previousFunction;
        UserUnit previousUnit;
        bool hasPreviousUserFunction = false;
        if (hasExistingUserVariable)
            previousVariable = m_evaluator->getVariable(target.identifier);
        if (hasExistingUserFunction)
            hasPreviousUserFunction = findUserFunctionByName(
                m_evaluator->getUserFunctions(),
                target.identifier,
                &previousFunction);
        if (hasExistingUserUnit && m_evaluator->getUserUnit(target.identifier))
            previousUnit = *m_evaluator->getUserUnit(target.identifier);

        if (overwriteExisting) {
            if (target.isFunction && hasExistingUserVariable) {
                m_evaluator->unsetVariable(target.identifier);
            }
            if (target.isFunction && hasExistingUserFunction) {
                m_evaluator->unsetUserFunction(target.identifier);
            } else if (!target.isFunction && !target.isUnit && hasExistingUserFunction) {
                m_evaluator->unsetUserFunction(target.identifier);
            } else if (!target.isFunction && !target.isUnit && hasExistingUserVariable) {
                m_evaluator->unsetVariable(target.identifier);
            } else if (target.isUnit) {
                if (hasExistingUserVariable)
                    m_evaluator->unsetVariable(target.identifier);
                if (hasExistingUserFunction)
                    m_evaluator->unsetUserFunction(target.identifier);
                if (hasExistingUserUnit)
                    m_evaluator->unsetUserUnit(target.identifier);
            }
        }

        m_evaluator->setExpression(expressionWithoutDescription);
        m_evaluator->eval();

        bool importSucceeded = false;
        if (m_evaluator->error().isEmpty()) {
            if (target.isFunction) {
                importSucceeded = m_evaluator->hasUserFunction(target.identifier);
            } else if (target.isUnit) {
                importSucceeded = m_evaluator->hasUserUnit(target.identifier);
            } else if (m_evaluator->hasVariable(target.identifier)
                       && !m_evaluator->isBuiltInVariable(target.identifier))
            {
                const Variable importedVariable = m_evaluator->getVariable(target.identifier);
                importSucceeded = !importedVariable.value().isNan();
            }
        }

        if (!importSucceeded) {
            if (target.isFunction) {
                if (m_evaluator->hasUserFunction(target.identifier))
                    m_evaluator->unsetUserFunction(target.identifier);
            } else if (target.isUnit) {
                if (m_evaluator->hasUserUnit(target.identifier))
                    m_evaluator->unsetUserUnit(target.identifier);
            } else if (m_evaluator->hasVariable(target.identifier)
                       && !m_evaluator->isBuiltInVariable(target.identifier))
            {
                m_evaluator->unsetVariable(target.identifier);
            }

            if (hasExistingUserVariable) {
                m_evaluator->setVariable(
                    previousVariable.identifier(),
                    previousVariable.value(),
                    previousVariable.type(),
                    previousVariable.description(),
                    previousVariable.formattedValue());
            }
            if (hasPreviousUserFunction)
                m_evaluator->setUserFunction(previousFunction);
            if (hasExistingUserUnit)
                m_evaluator->setUserUnit(previousUnit);

            ++localIgnoredLines;
            localIgnoredLineNumbers.append(lineNumber);
            continue;
        }

        if (target.isFunction) {
            UserFunction taggedFunction;
            if (findUserFunctionByName(m_evaluator->getUserFunctions(), target.identifier, &taggedFunction)) {
                QString description = explicitDescription;
                if (description.isEmpty())
                    description = taggedFunction.description();
                if (!description.contains(globalFunctionTag))
                    description = description.isEmpty()
                        ? globalFunctionTag
                        : description + QStringLiteral(" · ") + globalFunctionTag;
                if (taggedFunction.description() != description)
                    taggedFunction.setDescription(description);
                m_evaluator->setUserFunction(taggedFunction);
                m_evaluator->registerGlobalUserFunction(target.identifier);
            }
        } else if (target.isUnit) {
            const UserUnit* existingUnit = m_evaluator->getUserUnit(target.identifier);
            if (existingUnit) {
                UserUnit taggedUnit = *existingUnit;
                QString description = explicitDescription;
                if (description.isEmpty())
                    description = taggedUnit.description();
                if (!description.contains(globalUnitTag))
                    description = description.isEmpty()
                        ? globalUnitTag
                        : description + QStringLiteral(" · ") + globalUnitTag;
                if (taggedUnit.description() != description)
                    taggedUnit.setDescription(description);
                m_evaluator->setUserUnit(taggedUnit);
                m_evaluator->registerGlobalUserUnit(target.identifier);
            }
        } else if (m_evaluator->hasVariable(target.identifier)
                   && !m_evaluator->isBuiltInVariable(target.identifier)) {
            const Variable importedVariable = m_evaluator->getVariable(target.identifier);
            QString description = explicitDescription;
            if (description.isEmpty())
                description = importedVariable.description();
            if (!description.contains(globalVariableTag)) {
                description = description.isEmpty()
                    ? globalVariableTag
                    : description + QStringLiteral(" · ") + globalVariableTag;
            }
            m_evaluator->setVariable(
                importedVariable.identifier(),
                importedVariable.value(),
                importedVariable.type(),
                description,
                importedVariable.formattedValue());
            m_evaluator->registerGlobalUserVariable(target.identifier);
        }

        if (target.isFunction)
            ++localImportedFunctions;
        else if (target.isUnit)
            ++localImportedUnits;
        else
            ++localImportedVariables;
    }

    if (!dryRun && (localImportedVariables > 0 || localImportedFunctions > 0 || localImportedUnits > 0)) {
        emit variablesChanged();
        emit functionsChanged();
        emit unitsChanged();
    }

    if (dryRun && m_session) {
        *m_session = sessionBackup;
        m_conditions.autoAns = autoAnsBackup;
    }
    m_evaluator->setAllowGlobalUserDefinitionsOverride(false);

    if (importedVariables)
        *importedVariables = localImportedVariables;
    if (importedFunctions)
        *importedFunctions = localImportedFunctions;
    if (importedUnits)
        *importedUnits = localImportedUnits;
    if (ignoredLines)
        *ignoredLines = localIgnoredLines;
    if (ignoredLineNumbers)
        *ignoredLineNumbers = localIgnoredLineNumbers;
}

void MainWindow::applyUserDefinitions(int* importedVariables,
                                      int* importedFunctions,
                                      int* importedUnits,
                                      int* ignoredLines,
                                      QList<int>* ignoredLineNumbers)
{
    StartupDiagnostics::Scope timing("Apply global definitions");
    const QString definitionsText = m_settings->startupUserDefinitions.trimmed();
    const int previousImportedVariables = importedVariables ? *importedVariables : 0;
    const int previousImportedFunctions = importedFunctions ? *importedFunctions : 0;
    const int previousImportedUnits = importedUnits ? *importedUnits : 0;
    const int previousIgnoredLines = ignoredLines ? *ignoredLines : 0;

    int totalImportedVariables = 0;
    int totalImportedFunctions = 0;
    int totalImportedUnits = 0;
    int maxIgnoredLines = 0;
    QSet<int> uniqueIgnoredLineNumbers;

    Session* previousSession = m_session;
    const bool previousAutoAns = m_conditions.autoAns;

    const QList<Session*> sessions = m_loadedSessions.values();
    for (Session* session : sessions) {
        if (session == nullptr)
            continue;

        m_session = session;
        m_evaluator = m_session->evaluator();
        m_evaluator->initializeBuiltInVariables();

        int sessionImportedVariables = 0;
        int sessionImportedFunctions = 0;
        int sessionImportedUnits = 0;
        int sessionIgnoredLines = 0;
        QList<int> sessionIgnoredLineNumbers;
        importUserDefinitionsFromText(
            definitionsText,
            true,
            &sessionImportedVariables,
            &sessionImportedFunctions,
            &sessionImportedUnits,
            &sessionIgnoredLines,
            &sessionIgnoredLineNumbers,
            false);

        totalImportedVariables += sessionImportedVariables;
        totalImportedFunctions += sessionImportedFunctions;
        totalImportedUnits += sessionImportedUnits;
        maxIgnoredLines = qMax(maxIgnoredLines, sessionIgnoredLines);
        for (int lineNumber : sessionIgnoredLineNumbers)
            uniqueIgnoredLineNumbers.insert(lineNumber);
    }

    m_session = previousSession;
    m_evaluator = m_session ? m_session->evaluator() : nullptr;
    if (m_session != nullptr)
        m_evaluator->initializeBuiltInVariables();
    m_conditions.autoAns = previousAutoAns;
    if (m_widgets.display != nullptr && m_session != nullptr)
        m_widgets.display->setSession(m_session);
    if (m_widgets.editor != nullptr && m_evaluator != nullptr)
        m_widgets.editor->setSession(m_session);

    if (importedVariables)
        *importedVariables = previousImportedVariables + totalImportedVariables;
    if (importedFunctions)
        *importedFunctions = previousImportedFunctions + totalImportedFunctions;
    if (importedUnits)
        *importedUnits = previousImportedUnits + totalImportedUnits;
    if (ignoredLines)
        *ignoredLines = previousIgnoredLines + maxIgnoredLines;
    if (ignoredLineNumbers) {
        QList<int> sortedIgnoredLineNumbers = uniqueIgnoredLineNumbers.values();
        std::sort(sortedIgnoredLineNumbers.begin(), sortedIgnoredLineNumbers.end());
        for (int lineNumber : sortedIgnoredLineNumbers) {
            if (!ignoredLineNumbers->contains(lineNumber))
                ignoredLineNumbers->append(lineNumber);
        }
    }

    emit variablesChanged();
    emit functionsChanged();
    emit unitsChanged();
}

void MainWindow::showUserDefinitionsImportDialog()
{
    QDialog dialog(this);
    dialog.setObjectName(QStringLiteral("UserDefinitionsDialog"));
    dialog.setWindowTitle(tr("User Definitions"));
    dialog.setMinimumSize(640, 420);

    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    QLabel* info = new QLabel(
        tr("These definitions are global and are loaded into every session.\n"
           "They are immutable for sessions and override same-name definitions from the session editor.\n"
           "Enter one definition per line."),
        &dialog);
    info->setWordWrap(true);
    layout->addWidget(info);

    QPlainTextEdit* textEdit = new QPlainTextEdit(&dialog);
    textEdit->setObjectName(QStringLiteral("UserDefinitionsEditor"));
    textEdit->setPlainText(m_settings->startupUserDefinitions);
    textEdit->setPlaceholderText(tr("Examples:\n"
                                    "my_rate=1.25\n"
                                    "f(x)=x^2+1\n"
                                    "[cm_s]=[centimetre/second]"));
    SyntaxHighlighter* startupDefinitionsHighlighter = new SyntaxHighlighter(textEdit);
    const auto updateDefinitionsTheme = [textEdit, startupDefinitionsHighlighter]() {
        startupDefinitionsHighlighter->update();
        QPalette palette = textEdit->palette();
        for (const QPalette::ColorGroup group : {QPalette::Active,
                                                QPalette::Inactive,
                                                QPalette::Disabled}) {
            palette.setColor(group, QPalette::PlaceholderText,
                             aaForegroundForBackground(palette.color(group, QPalette::Base)));
        }
        textEdit->setPalette(palette);
        textEdit->viewport()->setPalette(palette);
    };
    updateDefinitionsTheme();
    connect(this, &MainWindow::colorSchemeChanged, &dialog, updateDefinitionsTheme);
    connect(this, &MainWindow::syntaxHighlightingChanged, &dialog, [startupDefinitionsHighlighter]() {
        startupDefinitionsHighlighter->rehighlight();
    });
    QPlainTextEdit* lineNumbers = new QPlainTextEdit(&dialog);
    lineNumbers->setObjectName(QStringLiteral("UserDefinitionsLineNumbers"));
    lineNumbers->setReadOnly(true);
    lineNumbers->setFrameStyle(QFrame::NoFrame);
    lineNumbers->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    lineNumbers->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    lineNumbers->setWordWrapMode(QTextOption::NoWrap);
    lineNumbers->setTextInteractionFlags(Qt::NoTextInteraction);
    lineNumbers->setStyleSheet(QStringLiteral(
        "QPlainTextEdit { background: palette(alternate-base); color: palette(mid); }"));
    lineNumbers->setFixedWidth(36);
    lineNumbers->setFont(textEdit->font());

    QWidget* editorRow = new QWidget(&dialog);
    QHBoxLayout* editorRowLayout = new QHBoxLayout(editorRow);
    editorRowLayout->setContentsMargins(0, 0, 0, 0);
    editorRowLayout->setSpacing(0);
    editorRowLayout->addWidget(lineNumbers);
    editorRowLayout->addWidget(textEdit);
    layout->addWidget(editorRow);

    const auto updateLineNumbers = [textEdit, lineNumbers]() {
        QStringList lines;
        lines.reserve(textEdit->blockCount());
        for (int i = 1; i <= textEdit->blockCount(); ++i)
            lines.append(QString::number(i));
        lineNumbers->setPlainText(lines.join(QLatin1Char('\n')));
        lineNumbers->verticalScrollBar()->setValue(textEdit->verticalScrollBar()->value());
    };
    connect(textEdit, &QPlainTextEdit::textChanged, &dialog, updateLineNumbers);
    connect(textEdit->verticalScrollBar(), &QScrollBar::valueChanged, &dialog,
            [textEdit, lineNumbers](int) {
                lineNumbers->verticalScrollBar()->setValue(textEdit->verticalScrollBar()->value());
            });
    updateLineNumbers();

    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal,
        &dialog);
    QPushButton* applyNowButton = buttons->addButton(tr("Apply"), QDialogButtonBox::ActionRole);
    QPushButton* validateButton = buttons->addButton(tr("Validate"), QDialogButtonBox::ActionRole);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, [this, textEdit, &dialog]() {
        m_settings->startupUserDefinitions = textEdit->toPlainText();
        applyUserDefinitions();
        dialog.accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(validateButton, &QPushButton::clicked, this, [this, textEdit]() {
        int importedVariables = 0;
        int importedFunctions = 0;
        int importedUnits = 0;
        int ignoredLines = 0;
        QList<int> ignoredLineNumbers;
        importUserDefinitionsFromText(
            textEdit->toPlainText(),
            true,
            &importedVariables,
            &importedFunctions,
            &importedUnits,
            &ignoredLines,
            &ignoredLineNumbers,
            true);

        QString ignoredLineNumbersText;
        if (!ignoredLineNumbers.isEmpty()) {
            QStringList values;
            values.reserve(ignoredLineNumbers.size());
            for (int line : ignoredLineNumbers)
                values.append(QString::number(line));
            ignoredLineNumbersText = values.join(QStringLiteral(", "));
        }

        QMessageBox::information(
            this,
            tr("Test Results"),
            tr("Imported variables: %1\nImported functions: %2\nImported units: %3\nLine numbers with errors: %4")
                .arg(importedVariables)
                .arg(importedFunctions)
                .arg(importedUnits)
                .arg(ignoredLineNumbersText.isEmpty() ? tr("none") : ignoredLineNumbersText));
    });
    connect(applyNowButton, &QPushButton::clicked, this, [this, textEdit]() {
        int importedVariables = 0;
        int importedFunctions = 0;
        int importedUnits = 0;
        int ignoredLines = 0;
        QList<int> ignoredLineNumbers;
        m_settings->startupUserDefinitions = textEdit->toPlainText();
        applyUserDefinitions(
            &importedVariables,
            &importedFunctions,
            &importedUnits,
            &ignoredLines,
            &ignoredLineNumbers);

        QString ignoredLineNumbersText;
        if (!ignoredLineNumbers.isEmpty()) {
            QStringList values;
            values.reserve(ignoredLineNumbers.size());
            for (int line : ignoredLineNumbers)
                values.append(QString::number(line));
            ignoredLineNumbersText = values.join(QStringLiteral(", "));
        }

        QMessageBox::information(
            this,
            tr("Apply Results"),
            tr("Imported variables: %1\nImported functions: %2\nImported units: %3\nLine numbers with errors: %4")
                .arg(importedVariables)
                .arg(importedFunctions)
                .arg(importedUnits)
                .arg(ignoredLineNumbersText.isEmpty() ? tr("none") : ignoredLineNumbersText));
    });

    if (dialog.exec() != QDialog::Accepted)
        return;

    m_settings->startupUserDefinitions = textEdit->toPlainText();
    UserDefinitions::saveFrom(m_settings);

    showStateLabel(tr("User definitions saved."));
}

void MainWindow::setAlwaysOnTopEnabled(bool b)
{
    m_settings->windowAlwaysOnTop = b;
    if (windowFlags().testFlag(Qt::WindowStaysOnTopHint) == b)
        return;

    // move() uses the outer frame position, not the global client-area origin.
    const bool wasVisible = isVisible();
    const QPoint cur = pos();
    if (b)
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    else
        setWindowFlags(windowFlags() & (~ Qt::WindowStaysOnTopHint));
    move(cur);
    if (wasVisible)
        show();
}

void MainWindow::setAutoAnsEnabled(bool b)
{
    m_settings->autoAns = b;
}

void MainWindow::setAutoCalcEnabled(bool b)
{
    m_settings->autoCalc = b;
    if (m_widgets.editor != nullptr)
        m_widgets.editor->setAutoCalcEnabled(b);
}

void MainWindow::setHistorySizeLimit()
{
    bool ok = false;
    const int current = m_session->historyLimit();
    const int value = QInputDialog::getInt(
        this,
        tr("History Size Limit"),
        tr("Maximum number of history entries for this session (0 = unlimited):"),
        current,
        0,
        1000000,
        100,
        &ok);

    if (!ok || value == current)
        return;

    m_session->setHistoryLimit(value);
    m_conditions.autoAns = !m_session->historyIsEmpty();
    emit historyChanged();
    saveSessionToDefaultPath();
}

void MainWindow::setLeaveLastExpressionEnabled(bool b)
{
    m_settings->leaveLastExpression = b;
}

void MainWindow::setUpDownArrowBehavior(QAction* action)
{
    if (!action)
        return;
    m_settings->upDownArrowBehavior =
        static_cast<Settings::UpDownArrowBehavior>(action->data().toInt());
}

void MainWindow::setWindowPositionSaveEnabled(bool b)
{
    m_settings->windowPositionSave = b;
}

void MainWindow::setOpenPrivateTabOnStartupEnabled(bool enabled)
{
    if (m_settings->openPrivateTabOnStartup == enabled)
        return;
    m_settings->openPrivateTabOnStartup = enabled;
    for (const QPointer<MainWindow>& window : allMainWindows()) {
        if (!window)
            continue;
        const QSignalBlocker blocker(window->m_actions.settingsOpenPrivateTabOnStartup);
        window->m_actions.settingsOpenPrivateTabOnStartup->setChecked(enabled);
    }
    m_settings->save();
}

void MainWindow::setSubsequentLaunchAction(QAction* action)
{
    if (!action)
        return;
    m_settings->subsequentLaunchAction =
        static_cast<Settings::SubsequentLaunchAction>(action->data().toInt());
    for (const QPointer<MainWindow>& window : allMainWindows()) {
        if (!window)
            continue;
        for (QAction* choice : window->m_actionGroups.subsequentLaunch->actions()) {
            const QSignalBlocker blocker(choice);
            choice->setChecked(choice->data().toInt() == int(m_settings->subsequentLaunchAction));
        }
    }
    m_settings->save();
}

void MainWindow::setAutoCompletionEnabled(bool b)
{
    m_settings->autoCompletion = b;
    if (m_widgets.editor != nullptr)
        m_widgets.editor->setAutoCompletionEnabled(b);
}

void MainWindow::setAutoCompletionAcceptWithEnterEnabled(bool enabled)
{
    m_settings->autoCompletionAcceptWithEnter = enabled;
    QList<QPointer<MainWindow>> windows = allMainWindows();
    if (!windows.contains(QPointer<MainWindow>(this)))
        windows.append(QPointer<MainWindow>(this));
    for (const QPointer<MainWindow>& window : windows) {
        if (!window)
            continue;
        const QSignalBlocker blocker(window->m_actions.settingsBehaviorAutoCompletionAcceptWithEnter);
        window->m_actions.settingsBehaviorAutoCompletionAcceptWithEnter->setChecked(enabled);
    }
}

void MainWindow::setAutoCompletionBuiltInFunctionsEnabled(bool b)
{
    m_settings->autoCompletionBuiltInFunctions = b;
}

void MainWindow::setAutoCompletionBuiltInVariablesEnabled(bool b)
{
    m_settings->autoCompletionBuiltInVariables = b;
}

void MainWindow::setAutoCompletionLongFormUnitsEnabled(bool b)
{
    m_settings->autoCompletionLongFormUnits = b;
}

void MainWindow::setAutoCompletionUserFunctionsEnabled(bool b)
{
    m_settings->autoCompletionUserFunctions = b;
}

void MainWindow::setAutoCompletionUserVariablesEnabled(bool b)
{
    m_settings->autoCompletionUserVariables = b;
}

void MainWindow::setBitfieldVisible(bool b)
{
    if (b)
        createBitField();
    else
        deleteBitField();
}

void MainWindow::setSyntaxHighlightingEnabled(bool b)
{
    m_settings->syntaxHighlighting = b;
    emit syntaxHighlightingChanged();
}

void MainWindow::setAutomaticInputAdjustmentsEnabled(bool enabled)
{
    m_settings->automaticInputAdjustments = enabled;
    QList<QPointer<MainWindow>> windows = allMainWindows();
    if (!windows.contains(QPointer<MainWindow>(this)))
        windows.append(QPointer<MainWindow>(this));
    for (const QPointer<MainWindow>& window : windows) {
        if (!window)
            continue;
        const QSignalBlocker blocker(window->m_actions.settingsEditingAutomaticInputAdjustments);
        window->m_actions.settingsEditingAutomaticInputAdjustments->setChecked(enabled);
        for (Editor* editor : window->findChildren<Editor*>())
            editor->refreshInputMode();
    }
}

void MainWindow::setInputDigitGroupingEnabled(bool enabled)
{
    m_settings->inputDigitGrouping = enabled;
    QList<QPointer<MainWindow>> windows = allMainWindows();
    if (!windows.contains(QPointer<MainWindow>(this)))
        windows.append(QPointer<MainWindow>(this));
    for (const QPointer<MainWindow>& window : windows) {
        if (!window)
            continue;
        const QSignalBlocker blocker(window->m_actions.settingsEditingInputDigitGrouping);
        window->m_actions.settingsEditingInputDigitGrouping->setChecked(enabled);
        emit window->syntaxHighlightingChanged();
    }
}

void MainWindow::setCursorBlinkingEnabled(bool enabled)
{
    m_settings->cursorBlinking = enabled;
    QList<QPointer<MainWindow>> windows = allMainWindows();
    if (!windows.contains(QPointer<MainWindow>(this)))
        windows.append(QPointer<MainWindow>(this));
    for (const QPointer<MainWindow>& window : windows) {
        if (!window)
            continue;
        const QSignalBlocker blocker(window->m_actions.settingsEditingCursorBlinking);
        window->m_actions.settingsEditingCursorBlinking->setChecked(enabled);
        for (Editor* editor : window->findChildren<Editor*>())
            editor->setCursorBlinkingEnabled(enabled);
    }
}

void MainWindow::setEscapeClearsEditorEnabled(bool enabled)
{
    m_settings->escapeClearsEditor = enabled;
    QList<QPointer<MainWindow>> windows = allMainWindows();
    if (!windows.contains(QPointer<MainWindow>(this)))
        windows.append(QPointer<MainWindow>(this));
    for (const QPointer<MainWindow>& window : windows) {
        if (!window)
            continue;
        const QSignalBlocker blocker(window->m_actions.settingsEditingEscapeClearsEditor);
        window->m_actions.settingsEditingEscapeClearsEditor->setChecked(enabled);
    }
}

void MainWindow::setDigitGrouping(QAction *action)
{
    m_settings->digitGrouping = action->data().toInt();
    emit historyChanged();
    if (m_widgets.editor != nullptr)
        m_widgets.editor->refreshAutoCalc();
    emit syntaxHighlightingChanged();
}

void MainWindow::setDigitGroupingIntegerPartOnlyEnabled(bool b)
{
    m_settings->digitGroupingIntegerPartOnly = b;
    emit historyChanged();
    if (m_widgets.editor != nullptr)
        m_widgets.editor->refreshAutoCalc();
    emit syntaxHighlightingChanged();
}

void MainWindow::setAutoResultToClipboardEnabled(bool b)
{
    m_settings->autoResultToClipboard = b;
}

void MainWindow::setSimplifyResultExpressionsEnabled(bool b)
{
    m_settings->simplifyResultExpressions = b;
    emit historyChanged();
    if (m_widgets.editor != nullptr)
        m_widgets.editor->refreshAutoCalc();
}

void MainWindow::setHoverHighlightResultsEnabled(bool b)
{
    m_settings->hoverHighlightResults = b;
    for (ResultDisplay* display : splitPaneDisplays())
        display->setHoverHighlightEnabled(b);
}

void MainWindow::setAngleModeDegree()
{
    if (m_settings->angleUnit == 'd' && m_status.selectedAngleUnit == 'd')
        return;

    m_settings->angleUnit = 'd';
    m_status.selectedAngleUnit = 'd';
    setStatusBarText();
    syncStatusBarSelectionMenuActionState();

    m_evaluator->initializeAngleUnits();
    emit angleUnitChanged();
}

void MainWindow::setAngleModeRadian()
{
    if (m_settings->angleUnit == 'r' && m_status.selectedAngleUnit == 'r')
        return;

    m_settings->angleUnit = 'r';
    m_status.selectedAngleUnit = 'r';
    setStatusBarText();
    syncStatusBarSelectionMenuActionState();

    m_evaluator->initializeAngleUnits();
    emit angleUnitChanged();
}

void MainWindow::setAngleModeGradian()
{
    if (m_settings->angleUnit == 'g' && m_status.selectedAngleUnit == 'g')
        return;

    m_settings->angleUnit = 'g';
    m_status.selectedAngleUnit = 'g';
    setStatusBarText();
    syncStatusBarSelectionMenuActionState();

    m_evaluator->initializeAngleUnits();
    emit angleUnitChanged();
}

void MainWindow::setAngleModeTurn()
{
    if (m_settings->angleUnit == 't' && m_status.selectedAngleUnit == 't')
        return;

    m_settings->angleUnit = 't';
    m_status.selectedAngleUnit = 't';

    setStatusBarText();
    syncStatusBarSelectionMenuActionState();

    m_evaluator->initializeAngleUnits();
    emit angleUnitChanged();
}

void MainWindow::setAngleModeRevolution()
{
    if (m_settings->angleUnit == 'v' && m_status.selectedAngleUnit == 'v')
        return;

    m_settings->angleUnit = 'v';
    m_status.selectedAngleUnit = 'v';

    setStatusBarText();
    syncStatusBarSelectionMenuActionState();

    m_evaluator->initializeAngleUnits();
    emit angleUnitChanged();
}

inline static QString documentsLocation()
{
    return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
}

void MainWindow::exportHtml()
{
    if (m_session == nullptr)
        return;

    const QString sessionBaseName = sessionFileBaseName(m_session->name());
    QFileDialog dialog(this, tr("Export session as HTML"), documentsLocation(), tr("HTML file (*.html)"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setDefaultSuffix(QStringLiteral("html"));
    dialog.selectFile(sessionBaseName + QLatin1String(".html"));
    QTimer::singleShot(0, &dialog, [sessionBaseName, &dialog]() {
        if (QLineEdit* fileNameEdit = dialog.findChild<QLineEdit*>())
            fileNameEdit->setSelection(0, sessionBaseName.size());
    });

    if (dialog.exec() != QDialog::Accepted)
        return;

    const QStringList selectedFiles = dialog.selectedFiles();
    if (selectedFiles.isEmpty())
        return;

    const QString fname = selectedFiles.constFirst();
    if (fname.isEmpty())
        return;

    QFile file(fname);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, tr("Error"), tr("Can't write to file %1").arg(fname));
        return;
    }

    QTextStream stream(& file);
    stream << m_widgets.display->exportHtml();

    file.close();
}

void MainWindow::exportPlainText()
{
    if (m_session == nullptr)
        return;

    const QString sessionBaseName = sessionFileBaseName(m_session->name());
    QFileDialog dialog(this, tr("Export session as plain text"), documentsLocation(),
                       tr("Text file (*.txt);;Any file (*.*)"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setDefaultSuffix(QStringLiteral("txt"));
    dialog.selectFile(sessionBaseName + QLatin1String(".txt"));
    QTimer::singleShot(0, &dialog, [sessionBaseName, &dialog]() {
        if (QLineEdit* fileNameEdit = dialog.findChild<QLineEdit*>())
            fileNameEdit->setSelection(0, sessionBaseName.size());
    });

    if (dialog.exec() != QDialog::Accepted)
        return;

    const QStringList selectedFiles = dialog.selectedFiles();
    if (selectedFiles.isEmpty())
        return;

    const QString fname = selectedFiles.constFirst();
    if (fname.isEmpty())
        return;

    QFile file(fname);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, tr("Error"), tr("Can't write to file %1").arg(fname));
        return;
    }

    QByteArray text;
    QTextStream stream(&text, QIODevice::WriteOnly | QIODevice::Text);
    stream << m_widgets.display->document()->toPlainText();
    stream.flush();

    file.write(text);
    file.close();
}

void MainWindow::setWidgetsDirection()
{
    QLocale::Language lang = QLocale().language();
    bool rtlSystem = (lang == QLocale::Hebrew || lang == QLocale::Arabic || lang == QLocale::Persian);

    QString code = m_settings->language;
    bool rtlCustom = (code.contains("he") || code.contains("ar") || code.contains("fa"));

    if ((m_settings->language == "C" && rtlSystem) || rtlCustom)
        qApp->setLayoutDirection(Qt::RightToLeft);
    else
        qApp->setLayoutDirection(Qt::LeftToRight);
}

void MainWindow::showFontDialog()
{
    QFontDialog::FontDialogOptions options;
#if defined(Q_OS_MACOS)
    // QTBUG-151608: macOS 27 breaks the native font panel's OK/Cancel buttons.
    // Revisit this workaround when https://bugreports.qt.io/browse/QTBUG-151608 is fixed upstream.
    if (QOperatingSystemVersion::current()
        >= QOperatingSystemVersion(QOperatingSystemVersion::MacOS, 27)) {
        options |= QFontDialog::DontUseNativeDialog;
    }
#endif
    bool ok;
    QFont f = QFontDialog::getFont(&ok, m_widgets.display->font(), this,
                                 tr("Editor and Results Font"), options);
    if (!ok)
        return;
    setDisplayFont(f);
}

void MainWindow::setDisplayFont(const QFont& font)
{
    // New windows read the shared preference. Existing tabs share the editor
    // and result display in their pane, so updating every pane covers them too.
    m_settings->displayFont = font.toString();
    for (const QPointer<MainWindow>& window : allMainWindows()) {
        if (!window)
            continue;
        for (ResultDisplay* display : window->splitPaneDisplays())
            display->setFont(font);
        for (Editor* editor : window->splitPaneEditors())
            editor->setFont(font);
        if (window->m_widgets.state != nullptr && window->m_widgets.state->isVisible())
            window->showStateLabel(window->m_widgets.state->text());
    }
}

void MainWindow::setStatusBarVisible(bool b)
{
    b ? createStatusBar() : deleteStatusBar();
}

void MainWindow::syncEditMenuActionState()
{
    MainWindow* target = activeMainWindowForMenuAction(this);
    // Focus can change while a window's child widgets are being destroyed.
    if (!allMainWindows().contains(QPointer<MainWindow>(target)))
        return;
    Editor* editor = target->m_widgets.editor;
    if (editor == nullptr)
        return;

    const bool dockInput = target->isDockTextInput(QApplication::focusWidget());
    const bool editing = !editor->isReadOnly() && !dockInput;
    const bool canCopy = target->m_copyWidget != nullptr
        && target->m_copyWidget->textCursor().hasSelection()
        && !dockInput;
    const QMimeData* clipboardData = QApplication::clipboard()->mimeData();
    const bool canPaste = editing && clipboardData != nullptr && clipboardData->hasText();
    for (const QPointer<MainWindow>& window : allMainWindows()) {
        if (window == nullptr || window->m_widgets.editor == nullptr)
            continue;
        window->m_actions.editUndo->setEnabled(editing && editor->document()->isUndoAvailable());
        window->m_actions.editRedo->setEnabled(editing && editor->document()->isRedoAvailable());
        window->m_actions.editCut->setEnabled(editing && editor->textCursor().hasSelection());
        window->m_actions.editCopy->setEnabled(canCopy);
        window->m_actions.editPaste->setEnabled(canPaste);
    }
}

void MainWindow::syncSessionMenuActionState()
{
    MainWindow* target = activeMainWindowForMenuAction(this);
    if (!allMainWindows().contains(QPointer<MainWindow>(target)))
        return;
    const bool canClose = target->m_session != nullptr;
    const bool canReopen = !target->m_closedSessionTabs.isEmpty();
    const QString privacyLabel = canClose && target->m_session->isPrivate()
        ? tr("Keep") : tr("Make Private...");
    for (const QPointer<MainWindow>& window : allMainWindows()) {
        if (window == nullptr || window->m_actions.sessionClose == nullptr)
            continue;
        window->m_actions.sessionClose->setEnabled(canClose);
        window->m_actions.sessionReopen->setEnabled(canReopen);
        window->m_actions.sessionConvertPrivacy->setText(privacyLabel);
    }
}

void MainWindow::syncViewMenuActionState()
{
    QStatusBar* existingStatusBar =
        findChild<QStatusBar*>(QString(), Qt::FindDirectChildrenOnly);
    const bool statusBarVisible =
        existingStatusBar != nullptr && !existingStatusBar->isHidden();
    const auto dockIsVisible = [](const QDockWidget* dock) {
        return dock != nullptr && !dock->isHidden();
    };
    const bool formulaBookVisible = dockIsVisible(m_docks.book);
    const bool constantsVisible = dockIsVisible(m_docks.constants);
    const bool functionsVisible = dockIsVisible(m_docks.functions);
    const bool historyVisible = dockIsVisible(m_docks.history);
    const bool variablesVisible = dockIsVisible(m_docks.variables);
    const bool userFunctionsVisible = dockIsVisible(m_docks.userFunctions);
    const bool userUnitsVisible = dockIsVisible(m_docks.userUnits);
    const bool bitfieldVisible = dockIsVisible(m_docks.bitField);
    const Settings::KeypadMode keypadMode = m_widgets.keypad != nullptr
        ? m_keypadMode
        : Settings::KeypadModeDisabled;
    const int keypadZoomPercent = m_keypadZoomPercent;
    const bool keypadZoomEnabled = keypadMode != Settings::KeypadModeDisabled;
    const bool menuBarVisible = menuBar() != nullptr && !menuBar()->isHidden();
    const bool fullScreenEnabled = isFullScreen();

    // Qt can reuse any window's QAction for the native application menu. Mirror
    // the active window's complete View state into every action copy while
    // leaving the inactive windows' widgets untouched.
    for (const QPointer<MainWindow>& ptr : allMainWindows()) {
        MainWindow* window = ptr.data();
        if (window == nullptr)
            continue;

        const auto setChecked = [](QAction* action, bool checked) {
            if (action == nullptr)
                return;
            action->setChecked(checked);
        };
        setChecked(window->m_actions.viewFormulaBook, formulaBookVisible);
        setChecked(window->m_actions.viewConstants, constantsVisible);
        setChecked(window->m_actions.viewFunctions, functionsVisible);
        setChecked(window->m_actions.viewHistory, historyVisible);
        setChecked(window->m_actions.viewVariables, variablesVisible);
        setChecked(window->m_actions.viewUserFunctions, userFunctionsVisible);
        setChecked(window->m_actions.viewUserUnits, userUnitsVisible);
        setChecked(window->m_actions.viewBitfield, bitfieldVisible);
        setChecked(window->m_actions.viewStatusBar, statusBarVisible);
        setChecked(window->m_actions.viewMenuBar, menuBarVisible);
        setChecked(window->m_actions.viewFullScreenMode, fullScreenEnabled);
        window->m_menus.keypadZoom->setEnabled(keypadZoomEnabled);

        switch (keypadMode) {
        case Settings::KeypadModeBasicWide:
            setChecked(window->m_actions.viewKeypadBasicWide, true);
            break;
        case Settings::KeypadModeScientificWide:
            setChecked(window->m_actions.viewKeypadScientificWide, true);
            break;
        case Settings::KeypadModeScientificNarrow:
            setChecked(window->m_actions.viewKeypadScientificNarrow, true);
            break;
        case Settings::KeypadModeCustom:
            setChecked(window->m_actions.viewKeypadCustom, true);
            break;
        case Settings::KeypadModeDisabled:
        default:
            setChecked(window->m_actions.viewKeypadDisabled, true);
            break;
        }
        switch (keypadZoomPercent) {
        case 50:
            setChecked(window->m_actions.viewKeypadZoom50, true);
            break;
        case 150:
            setChecked(window->m_actions.viewKeypadZoom150, true);
            break;
        case 200:
            setChecked(window->m_actions.viewKeypadZoom200, true);
            break;
        case 100:
        default:
            setChecked(window->m_actions.viewKeypadZoom100, true);
            break;
        }
        window->updateKeypadDisabledActionText();
    }
}

void MainWindow::setStatusBarSelectionActionState(char angleUnit, char resultFormat,
                                                  int resultPrecision)
{
    switch (angleUnit) {
    case 'r': m_actions.settingsAngleUnitRadian->setChecked(true); break;
    case 'g': m_actions.settingsAngleUnitGradian->setChecked(true); break;
    case 't': m_actions.settingsAngleUnitTurn->setChecked(true); break;
    case 'v': m_actions.settingsAngleUnitRevolution->setChecked(true); break;
    default: m_actions.settingsAngleUnitDegree->setChecked(true); break;
    }

    switch (resultFormat) {
    case 'g': m_actions.settingsResultFormatGeneral->setChecked(true); break;
    case 'n': m_actions.settingsResultFormatEngineering->setChecked(true); break;
    case 'e': m_actions.settingsResultFormatScientific->setChecked(true); break;
    case 'r': m_actions.settingsResultFormatRational->setChecked(true); break;
    case 'h': m_actions.settingsResultFormatHexadecimal->setChecked(true); break;
    case 'o': m_actions.settingsResultFormatOctal->setChecked(true); break;
    case 'b': m_actions.settingsResultFormatBinary->setChecked(true); break;
    case 's': m_actions.settingsResultFormatSexagesimal->setChecked(true); break;
    default: m_actions.settingsResultFormatFixed->setChecked(true); break;
    }

    switch (resultPrecision) {
    case 0: m_actions.settingsResultFormat0Digits->setChecked(true); break;
    case 2: m_actions.settingsResultFormat2Digits->setChecked(true); break;
    case 3: m_actions.settingsResultFormat3Digits->setChecked(true); break;
    case 8: m_actions.settingsResultFormat8Digits->setChecked(true); break;
    case 15: m_actions.settingsResultFormat15Digits->setChecked(true); break;
    case 50: m_actions.settingsResultFormat50Digits->setChecked(true); break;
    case -1: m_actions.settingsResultFormatAutoPrecision->setChecked(true); break;
    default: m_actions.settingsResultFormatCustomDigits->setChecked(true); break;
    }
}

void MainWindow::syncStatusBarSelectionMenuActionState()
{
    for (const QPointer<MainWindow>& ptr : allMainWindows()) {
        MainWindow* window = ptr.data();
        if (window == nullptr)
            continue;
        window->setStatusBarSelectionActionState(m_status.selectedAngleUnit,
                                                 m_status.selectedResultFormat,
                                                 m_status.selectedResultPrecision);
    }
}

void MainWindow::applyStatusBarSelectionState()
{
    // Evaluators still consume the shared Settings values. Window activation
    // projects this window's durable selection into that shared runtime without
    // overwriting the selections retained by inactive windows.
    const bool angleChanged = m_settings->angleUnit != m_status.selectedAngleUnit;
    const bool formatChanged = m_settings->resultFormat != m_status.selectedResultFormat;
    const bool precisionChanged =
        m_settings->resultPrecision != m_status.selectedResultPrecision;

    m_settings->angleUnit = m_status.selectedAngleUnit;
    m_settings->resultFormat = m_status.selectedResultFormat;
    m_settings->resultPrecision = m_status.selectedResultPrecision;
    syncStatusBarSelectionMenuActionState();

    if (angleChanged) {
        m_evaluator->initializeAngleUnits();
        emit angleUnitChanged();
    }
    if (formatChanged)
        emit resultFormatChanged();
    if (precisionChanged)
        emit resultPrecisionChanged();
}

void MainWindow::setMenuBarVisible(bool b)
{
    menuBar()->setVisible(b);
    m_settings->menuBarVisible = b;
}

void MainWindow::showStateLabel(const QString& msg)
{
    if (msg.contains(QStringLiteral("Current result:")))
        m_lastCurrentResultPreviewMessage = msg;

    const GeneratedThemeSurfaces surfaces = generatedSurfaceColors(m_settings);
    const ThemeSurfaceColors tooltipSurface =
        themeSurfaceForShadeIndex(surfaces, UiConfig::ResultTooltipBackgroundShade);
    const ThemeSurfaceColors tooltipOutline =
        themeSurfaceForShadeIndex(surfaces, UiConfig::ResultTooltipOutlineShade);
    m_widgets.state->setStyleSheet(ToolTipStyleUtils::labelToolTipStyleSheet(
        QStringLiteral("QLabel"),
        tooltipSurface.background,
        tooltipSurface.foreground,
        tooltipOutline.background,
        UiConfig::ResultTooltipCornerRadius));
    m_widgets.stateCloseButton->setStyleSheet(QStringLiteral(R"(
        QPushButton {
            border: none;
            background: transparent;
            color: %1;
            padding: 0;
            margin: 0;
            outline: none;
        }

        QPushButton:hover {
            background: transparent;
            color: %1;
        }

        QPushButton:pressed {
            background: transparent;
            color: %1;
        }
    )").arg(tooltipSurface.foreground.name()));

    Editor* positionEditor = m_widgets.editor;
    if (positionEditor == nullptr || positionEditor->window() != this)
        positionEditor = globallyActiveEditor();

    const int closeButtonSize = qMax(14, positionEditor->fontMetrics().height() - 2);
    const int closeButtonRightPadding = 2;
    const int closeButtonTopPadding = 1;
    const int closeButtonReservedWidth = closeButtonSize + closeButtonRightPadding + 2;
    m_widgets.state->setContentsMargins(6, 3, closeButtonReservedWidth, 3);
    m_widgets.state->setFont(positionEditor->font());
    m_widgets.state->setText(msg);
    m_widgets.stateCloseButton->setFixedSize(closeButtonSize, closeButtonSize);
    m_widgets.state->adjustSize();
    m_widgets.stateCloseButton->move(
        m_widgets.state->width() - closeButtonSize - closeButtonRightPadding,
        closeButtonTopPadding);
    m_widgets.stateCloseButton->show();
    m_widgets.stateCloseButton->raise();
    m_widgets.state->show();
    m_widgets.state->raise();
    const int height = m_widgets.state->height();
    const int horizontalMargin = m_settings->editorDisplayStyle == Settings::EditorDisplayClassic
        ? UiConfig::ClassicTextInset : UiConfig::ResultTooltipStartMargin;
    QPoint pos = mapFromGlobal(
        positionEditor->mapToGlobal(QPoint(horizontalMargin, -height)));
    m_widgets.state->move(pos);
}

void MainWindow::handleAutoCalcMessageAvailable(const QString& message)
{
    if (pendingDockFocusTarget() != nullptr)
        return;
    if (Editor* editor = qobject_cast<Editor*>(sender()); editor != nullptr && editor != m_widgets.editor)
        return;
    if (message.contains(QStringLiteral("Current result:"))
        && !m_widgets.state->isVisible()
        && message == m_lastCurrentResultPreviewMessage) {
        return;
    }
    if (m_currentResultPreviewDismissed && message.contains(QStringLiteral("Current result:")))
        return;
    showStateLabel(message);
}

void MainWindow::handleAutoCalcQuantityAvailable(const Quantity& quantity)
{
    if (Editor* editor = qobject_cast<Editor*>(sender()); editor != nullptr && editor != m_widgets.editor)
        return;
    if (m_widgets.bitField && m_docks.bitField && !m_docks.bitField->isHidden())
        m_widgets.bitField->updateBits(quantity);
}

void MainWindow::setFullScreenEnabled(bool b)
{
    m_settings->windowOnfullScreen = b;
    b ? showFullScreen() : showNormal();
}

bool MainWindow::event(QEvent* e)
{
    if (e != nullptr && m_settings != nullptr
        && m_settings->statusBarAppearance == Settings::StatusBarAppearanceSystem
        && (e->type() == QEvent::ApplicationPaletteChange || e->type() == QEvent::ThemeChange
            || e->type() == QEvent::StyleChange)
        && !property("speedcrunchStatusBarRefreshPending").toBool()) {
        setProperty("speedcrunchStatusBarRefreshPending", true);
        QTimer::singleShot(0, this, [this]() {
            applyStatusBarAppearance(findChild<QStatusBar*>(QString(), Qt::FindDirectChildrenOnly),
                                     generatedSurfaceColors(m_settings));
            updateStatusBarSectionVisibility();
            setProperty("speedcrunchStatusBarRefreshPending", false);
        });
    }
    if (e != nullptr && !m_allDocks.isEmpty()
        && m_settings->dockAppearance == Settings::DockAppearanceSystem
        && (e->type() == QEvent::ApplicationPaletteChange || e->type() == QEvent::ThemeChange
            || e->type() == QEvent::StyleChange)) {
        scheduleDockAppearanceRefresh(true);
    }
    if (e != nullptr) {
        const bool updateDockSeparators =
            e->type() == QEvent::Enter
            || e->type() == QEvent::Leave
            || e->type() == QEvent::HoverEnter
            || e->type() == QEvent::HoverMove
            || e->type() == QEvent::HoverLeave
            || e->type() == QEvent::MouseButtonPress
            || e->type() == QEvent::MouseButtonRelease
            || e->type() == QEvent::MouseMove;
        if (updateDockSeparators && !m_allDocks.isEmpty())
            update();

        const bool splitCursor = cursor().shape() == Qt::SplitHCursor
            || cursor().shape() == Qt::SplitVCursor;
        if (splitCursor && e->type() == QEvent::MouseButtonPress) {
            const QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(e);
            if (mouseEvent->button() == Qt::LeftButton)
                hideCurrentResultPreview();
        } else if (splitCursor && e->type() == QEvent::MouseMove) {
            const QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(e);
            if (mouseEvent->buttons() & Qt::LeftButton)
                hideCurrentResultPreview();
        }
    }

    if (e != nullptr
        && (e->type() == QEvent::KeyPress || e->type() == QEvent::ShortcutOverride)) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
        const Qt::KeyboardModifiers shortcutModifiers =
            keyEvent->modifiers() & ~(Qt::KeypadModifier);
        if (e->type() == QEvent::KeyPress
            && keyEvent->key() == Qt::Key_F6
            && (shortcutModifiers == Qt::NoModifier
                || shortcutModifiers == Qt::ShiftModifier)
            && qApp->activeModalWidget() == nullptr
            && qApp->activePopupWidget() == nullptr) {
            if (shortcutModifiers == Qt::ShiftModifier)
                cycleFocusBackward();
            else
                cycleFocusForward();
            keyEvent->accept();
            return true;
        }
        if (keyEvent->key() == Qt::Key_Escape
            && m_widgets.state != nullptr
            && m_widgets.state->isVisible()
            && m_widgets.state->text().contains(QStringLiteral("Current result:"))) {
            m_currentResultPreviewDismissed = true;
            if (m_widgets.editor != nullptr)
                m_widgets.editor->dismissCurrentAutoCalc();
        }
    }

    if (e != nullptr && e->type() == QEvent::WindowDeactivate) {
        QWidget* focusWidget = lastFocusWidgetInActiveWindow();
        if (focusWidget == nullptr)
            focusWidget = QApplication::focusWidget();
        focusWidgetBeforeWindowDeactivate() =
            focusWidget != nullptr && focusWidget->window() == this
            ? focusWidget
            : m_widgets.editor;
        editorBeforeWindowDeactivate() = m_widgets.editor;
    }

    if (e != nullptr && e->type() == QEvent::WindowActivate) {
        // Restore the exact widget that had focus before the app lost focus.
        // On macOS the toolkit may later replay passive editor FocusIn events
        // in widget order, and the replay can otherwise promote the last pane.
        QPointer<QWidget> focusWidget(focusWidgetBeforeWindowDeactivate());
        if (focusWidget != nullptr && focusWidget->window() != this)
            focusWidget = nullptr;
        QPointer<Editor> activeEditor(qobject_cast<Editor*>(focusWidget.data()));
        if (activeEditor == nullptr && focusWidget != nullptr)
            activeEditor = qobject_cast<Editor*>(focusWidget->parentWidget());
        if (activeEditor != nullptr && activeEditor->window() != this)
            activeEditor = nullptr;
        if (activeEditor == nullptr) {
            QPointer<Editor> previousEditor(editorBeforeWindowDeactivate());
            if (previousEditor != nullptr && previousEditor->window() == this)
                activeEditor = previousEditor;
        }
        if (activeEditor == nullptr)
            activeEditor = m_widgets.editor;
        focusWidgetBeforeWindowDeactivate() = nullptr;
        editorBeforeWindowDeactivate() = nullptr;
        pendingWindowActivationEditor() = activeEditor;
        pendingWindowActivationFocusWidget() = focusWidget;
        const int restoreGeneration = ++windowActivationRestoreGeneration();
        const auto restoreWindowFocus = [this, activeEditor, focusWidget, restoreGeneration]() {
            if (activeEditor == nullptr
                || windowActivationRestoreGeneration() != restoreGeneration
                || QApplication::activePopupWidget() != nullptr)
                return;
            QWidget* pane = activeEditor->parentWidget();
            ResultDisplay* display = pane
                ? pane->findChild<ResultDisplay*>(QString(), Qt::FindDirectChildrenOnly)
                : nullptr;
            if (display != nullptr)
                setActiveEditorDisplayPane(display, activeEditor);
            if (focusWidget != nullptr
                && focusWidget->window() == this
                && windowActivationRestoreGeneration() == restoreGeneration) {
                focusWidget->setFocus(Qt::ActiveWindowFocusReason);
            }
        };
        QTimer::singleShot(0, this, restoreWindowFocus);
        QTimer::singleShot(50, this, restoreWindowFocus);
        QTimer::singleShot(150, this, restoreWindowFocus);
        // Some platforms deliver a delayed focus replay after the activation
        // event has returned. Keep one late restore before clearing the guard.
        QTimer::singleShot(300, this, restoreWindowFocus);
        QTimer::singleShot(350, this, [activeEditor, focusWidget, restoreGeneration]() {
            if (windowActivationRestoreGeneration() == restoreGeneration
                && pendingWindowActivationEditor() == activeEditor
                && pendingWindowActivationFocusWidget() == focusWidget) {
                pendingWindowActivationEditor() = nullptr;
                pendingWindowActivationFocusWidget() = nullptr;
            }
        });

        const QString activeName = m_paneSessionNames.value(m_widgets.display);
        if (!activeName.isEmpty()) {
            if (Session* activeSession = m_loadedSessions.value(activeName, nullptr))
                activateSession(activeSession);
        } else if (m_session != nullptr) {
            m_evaluator = m_session->evaluator();
            m_evaluator->initializeBuiltInVariables();
        }

        applyStatusBarSelectionState();
        syncViewMenuActionState();
    }

    return QMainWindow::event(e);
}

bool MainWindow::eventFilter(QObject* o, QEvent* e)
{
    if (o == qApp && e != nullptr && e->type() == QEvent::Quit
        && primaryMainWindow() == this) {
        // QApplication closes top-level windows in an unspecified order.
        // Save all windows before floating docks receive Close and hide.
        qApp->setProperty("speedcrunchShutdownInProgress", true);
        persistSessionAndSettingsForShutdown();
    }

    if (e != nullptr && e->type() == QEvent::KeyPress) {
        if (QWidget* widget = qobject_cast<QWidget*>(o);
            widget != nullptr
            && (widget->window() == this || isDockWidgetDescendant(widget))) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
            const Qt::KeyboardModifiers shortcutModifiers =
                keyEvent->modifiers() & ~(Qt::KeypadModifier);
            if (keyEvent->key() == Qt::Key_F6
                && (shortcutModifiers == Qt::NoModifier
                    || shortcutModifiers == Qt::ShiftModifier)
                && qApp->activeModalWidget() == nullptr
                && qApp->activePopupWidget() == nullptr) {
                if (shortcutModifiers == Qt::ShiftModifier)
                    cycleFocusBackward();
                else
                    cycleFocusForward();
                keyEvent->accept();
                return true;
            }
        }
    }

    if (qobject_cast<QSplitterHandle*>(o) != nullptr) {
        if (e->type() == QEvent::MouseButtonPress) {
            const QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(e);
            if (mouseEvent->button() == Qt::LeftButton)
                hideCurrentResultPreview();
        } else if (e->type() == QEvent::MouseMove) {
            const QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(e);
            if (mouseEvent->buttons() & Qt::LeftButton)
                hideCurrentResultPreview();
        }
        return QMainWindow::eventFilter(o, e);
    }

    if (QAbstractButton* button = qobject_cast<QAbstractButton*>(o);
        button != nullptr && button->property("speedcrunchDockHeaderButton").toBool()) {
        if (e->type() == QEvent::Paint) {
            // Qt's dock title button ignores QAbstractButton::iconSize() and
            // shrinks icons to 5/8 of PM_SmallIconSize on framed styles (Windows).
            // Paint our themed icon directly at its intended logical size.
            QPainter painter(button);
            const QSize size = button->iconSize().boundedTo(button->size());
            const QRect iconRect(QPoint((button->width() - size.width()) / 2,
                                        (button->height() - size.height()) / 2), size);
            button->icon().paint(&painter, iconRect, Qt::AlignCenter,
                                 button->isEnabled() ? QIcon::Normal : QIcon::Disabled);
            return true;
        } else if (e->type() == QEvent::Enter
            || e->type() == QEvent::HoverEnter
            || e->type() == QEvent::MouseMove) {
            applyDockTitleButtonIcon(button, true);
        } else if (e->type() == QEvent::Leave) {
            applyDockTitleButtonIcon(button, false);
        }
        return QMainWindow::eventFilter(o, e);
    }

    if (QTabBar* tabBar = qobject_cast<QTabBar*>(o);
        tabBar != nullptr && tabBar->property("speedcrunchDockSystemTabBar").toBool()) {
        if (e->type() == QEvent::Enter) {
            QEnterEvent* enterEvent = static_cast<QEnterEvent*>(e);
            updateDockSystemTabCursor(tabBar, enterEvent->position().toPoint());
        } else if (e->type() == QEvent::HoverEnter || e->type() == QEvent::HoverMove) {
            QHoverEvent* hoverEvent = static_cast<QHoverEvent*>(e);
            updateDockSystemTabCursor(tabBar, hoverEvent->position().toPoint());
        } else if (e->type() == QEvent::MouseMove) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(e);
            updateDockSystemTabCursor(tabBar, mouseEvent->pos());
        } else if (e->type() == QEvent::Leave) {
            tabBar->setCursor(Qt::ArrowCursor);
        }
        return QMainWindow::eventFilter(o, e);
    }

    if (QWidget* widget = qobject_cast<QWidget*>(o); isDockTextInput(widget)) {
        if (e->type() == QEvent::MouseButtonPress) {
            const int textInputGeneration = setPendingDockTextInputFocusTarget(widget);
            widget->setFocus(Qt::MouseFocusReason);
            deactivateActiveEditorForTextInputFocus();
            QPointer<QWidget> textInput(widget);
            QTimer::singleShot(0, widget, [textInput, textInputGeneration]() {
                if (textInput != nullptr
                    && pendingDockTextInputFocusTargetGeneration() == textInputGeneration
                    && pendingDockTextInputFocusTarget() == textInput)
                    textInput->setFocus(Qt::MouseFocusReason);
            });
            QTimer::singleShot(100, widget, [textInput, textInputGeneration]() {
                if (pendingDockTextInputFocusTargetGeneration() == textInputGeneration
                    && pendingDockTextInputFocusTarget() == textInput) {
                    setPendingDockTextInputFocusTarget(nullptr);
                }
            });
        } else if (e->type() == QEvent::FocusIn) {
            deactivateActiveEditorForTextInputFocus();
            if (QLineEdit* lineEdit = qobject_cast<QLineEdit*>(widget);
                lineEdit && m_settings->dockAppearance == Settings::DockAppearanceSpeedCrunch)
                applyDockSearchClearButtonIcon(lineEdit, generatedSurfaceColors(m_settings), true);
        } else if (e->type() == QEvent::FocusOut) {
            if (QLineEdit* lineEdit = qobject_cast<QLineEdit*>(widget);
                lineEdit && m_settings->dockAppearance == Settings::DockAppearanceSpeedCrunch)
                applyDockSearchClearButtonIcon(lineEdit, generatedSurfaceColors(m_settings), false);
        }
        return QMainWindow::eventFilter(o, e);
    }

    if (e->type() == QEvent::MouseButtonPress) {
        if (QWidget* widget = qobject_cast<QWidget*>(o)) {
            QDockWidget* dock = dockWidgetForDescendant(widget);
            if (dock != nullptr) {
                QWidget* focusTarget = dock;
                if (QAbstractItemView* view = dockItemViewFocusTarget(widget)) {
                    view->setFocusPolicy(Qt::StrongFocus);
                    focusTarget = view;
                } else {
                    dock->setFocusPolicy(Qt::StrongFocus);
                }
                const int focusGeneration = setPendingDockFocusTarget(focusTarget);
                deactivateActiveEditorForTextInputFocus();
                hideStateLabel();
                setPendingDockTextInputFocusTarget(nullptr);
                const QColor inactivePrimary = generatedSurfaceColors(m_settings).primary.background;
                for (Editor* paneEditor : splitPaneEditors()) {
                    if (paneEditor == nullptr)
                        continue;
                    paneEditor->clearFocus();
                    paneEditor->setCustomCursorVisible(false);
                    paneEditor->setThemePrimaryColor(inactivePrimary, false);
                }
                focusTarget->setFocus(Qt::MouseFocusReason);
                QPointer<QWidget> focusTargetGuard(focusTarget);
                const auto restoreDockFocus = [focusTargetGuard, focusGeneration]() {
                    if (focusTargetGuard != nullptr
                        && pendingDockFocusTargetGeneration() == focusGeneration
                        && pendingDockFocusTarget() == focusTargetGuard)
                        focusTargetGuard->setFocus(Qt::MouseFocusReason);
                };
                QTimer::singleShot(0, focusTarget, restoreDockFocus);
                QTimer::singleShot(50, focusTarget, restoreDockFocus);
                QTimer::singleShot(250, focusTarget, [focusTargetGuard, focusGeneration]() {
                    if (pendingDockFocusTargetGeneration() == focusGeneration
                        && pendingDockFocusTarget() == focusTargetGuard) {
                        setPendingDockFocusTarget(nullptr);
                    }
                });
            }
        }
    }

    Editor* filteredEditor = qobject_cast<Editor*>(o);
    if (filteredEditor == nullptr) {
        if (QWidget* widget = qobject_cast<QWidget*>(o))
            filteredEditor = qobject_cast<Editor*>(widget->parentWidget());
    }
    if (Editor* editor = filteredEditor) {
        // Every main window filters application-wide events. Input in another
        // window must not replace this window's active editor/display pointers.
        if (editor->window() != this)
            return QMainWindow::eventFilter(o, e);

        // Recalling wrapped input can move the editor after its result appears.
        // Keep the preview above the editor as the pane layout catches up.
        if (o == editor && editor == m_widgets.editor
            && (e->type() == QEvent::Move || e->type() == QEvent::Resize)
            && m_widgets.state != nullptr && m_widgets.state->isVisible()) {
            showStateLabel(m_widgets.state->text());
        }

        if (e->type() == QEvent::FocusIn && pendingDockFocusTarget() != nullptr) {
            // A dock click is explicit focus intent. Handle it before editor
            // completion focus recovery, otherwise a pending completion owner can
            // pull focus back to an editor and make dock selection use stale pane
            // state.
            QPointer<QWidget> focusTarget(pendingDockFocusTarget());
            editor->clearFocus();
            QTimer::singleShot(0, focusTarget, [focusTarget]() {
                if (focusTarget != nullptr)
                    focusTarget->setFocus(Qt::MouseFocusReason);
            });
            return true;
        }
        if (e->type() == QEvent::FocusIn && pendingDockTextInputFocusTarget() != nullptr) {
            QPointer<QWidget> textInput(pendingDockTextInputFocusTarget());
            QTimer::singleShot(0, textInput, [textInput]() {
                if (textInput != nullptr)
                    textInput->setFocus(Qt::MouseFocusReason);
            });
            return true;
        }
        if (Editor* completionOwner = Editor::completionMouseSelectionOwner()) {
            if (editor != completionOwner
                && (e->type() == QEvent::FocusIn || e->type() == QEvent::MouseButtonPress)) {
                QPointer<Editor> owner(completionOwner);
                QTimer::singleShot(0, completionOwner, [owner]() {
                    if (owner != nullptr) {
                        owner->window()->activateWindow();
                        owner->setFocus(Qt::OtherFocusReason);
                        owner->viewport()->setFocus(Qt::OtherFocusReason);
                    }
                });
                return true;
            }
        }
        if (dockTextInputFocusTransferInProgress() && e->type() == QEvent::FocusIn)
            return QMainWindow::eventFilter(o, e);
        if (e->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
            if (keyEvent->key() == Qt::Key_Escape
                && m_widgets.state->isVisible()
                && m_widgets.state->text().contains(QStringLiteral("Current result:"))) {
                m_currentResultPreviewDismissed = true;
                editor->dismissCurrentAutoCalc();
            }
        }
        // A bare FocusIn is not pane-selection intent. Window activation can
        // replay FocusIn through every editor viewport in layout order, which
        // used to make the last pane active after each app switch. Only input
        // events that originate from the editor path below may select a pane.
        if (e->type() == QEvent::MouseButtonPress
            || e->type() == QEvent::KeyPress
            || e->type() == QEvent::InputMethod) {
            if (pendingWindowActivationEditor() != nullptr
                && (e->type() == QEvent::MouseButtonPress
                    || e->type() == QEvent::KeyPress)) {
                cancelWindowActivationRestore();
            }
            if (pendingWindowActivationEditor() != nullptr
                && editor != pendingWindowActivationEditor()) {
                // Input method events can still arrive while the activation
                // restore is settling. Until mouse/key intent cancels the
                // guard above, keep the previously active editor selected.
                QPointer<Editor> activeEditor(pendingWindowActivationEditor());
                QPointer<MainWindow> window(this);
                QTimer::singleShot(0, this, [window, activeEditor]() {
                    if (activeEditor != nullptr
                        && window != nullptr
                        && pendingWindowActivationEditor() == activeEditor) {
                        QWidget* pane = activeEditor->parentWidget();
                        ResultDisplay* display = pane
                            ? pane->findChild<ResultDisplay*>(QString(), Qt::FindDirectChildrenOnly)
                            : nullptr;
                        if (display != nullptr)
                            window->setActiveEditorDisplayPane(display, activeEditor);
                    }
                });
                return true;
            }
            if (pendingSessionTabActivationEditor() != nullptr
                && editor != pendingSessionTabActivationEditor()) {
                QPointer<Editor> targetEditor(pendingSessionTabActivationEditor());
                QTimer::singleShot(0, targetEditor, [targetEditor]() {
                    if (targetEditor != nullptr)
                        targetEditor->setFocus(Qt::MouseFocusReason);
                });
                return true;
            }
            QWidget* pane = editor->parentWidget();
            ResultDisplay* display = pane ? pane->findChild<ResultDisplay*>(QString(), Qt::FindDirectChildrenOnly) : nullptr;
            if (display != nullptr)
                setActiveEditorDisplayPane(display, editor);
        }
        return QMainWindow::eventFilter(o, e);
    }

    if (o == m_widgets.state && e->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(e);
        if (mouseEvent->button() == Qt::LeftButton) {
            hideStateLabel();
            return true;
        }
    }

    if (o == m_status.angleUnitLabel || o == m_status.resultFormatLabel
            || o == m_status.resultPrecisionLabel) {
        if (e->type() == QEvent::MouseButtonPress) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(e);
            if (mouseEvent->button() == Qt::LeftButton) {
                const QPoint popupPoint(0, static_cast<QWidget*>(o)->height());
                if (o == m_status.angleUnitLabel)
                    showAngleModeContextMenu(m_status.angleUnit->mapFromGlobal(static_cast<QWidget*>(o)->mapToGlobal(popupPoint)));
                else if (o == m_status.resultPrecisionLabel)
                    showPrecisionContextMenu(m_status.resultPrecision->mapFromGlobal(static_cast<QWidget*>(o)->mapToGlobal(popupPoint)));
                else
                    showResultFormatContextMenu(m_status.resultFormat->mapFromGlobal(static_cast<QWidget*>(o)->mapToGlobal(popupPoint)));
                return true;
            }
            if (mouseEvent->button() == Qt::RightButton) {
                const QPoint globalPoint = static_cast<QWidget*>(o)->mapToGlobal(mouseEvent->pos());
                if (o == m_status.angleUnitLabel)
                    showAngleModeContextMenu(m_status.angleUnit->mapFromGlobal(globalPoint));
                else if (o == m_status.resultPrecisionLabel)
                    showPrecisionContextMenu(m_status.resultPrecision->mapFromGlobal(globalPoint));
                else
                    showResultFormatContextMenu(m_status.resultFormat->mapFromGlobal(globalPoint));
                return true;
            }
        }
        if (e->type() == QEvent::ContextMenu) {
            QContextMenuEvent* contextMenuEvent = static_cast<QContextMenuEvent*>(e);
            const QPoint globalPoint = contextMenuEvent->globalPos();
            if (o == m_status.angleUnitLabel)
                showAngleModeContextMenu(m_status.angleUnit->mapFromGlobal(globalPoint));
            else if (o == m_status.resultPrecisionLabel)
                showPrecisionContextMenu(m_status.resultPrecision->mapFromGlobal(globalPoint));
            else
                showResultFormatContextMenu(m_status.resultFormat->mapFromGlobal(globalPoint));
            return true;
        }
    }

    if (o == m_docks.book) {
        if (e->type() == QEvent::Close) {
            deleteBookDock();
            return true;
        }
        return false;
    }

    if (o == m_docks.bitField) {
        if (e->type() == QEvent::Close) {
            deleteBitField();
            return true;
        }
        return false;
    }

    if (o == m_docks.constants) {
        if (e->type() == QEvent::Close) {
            deleteConstantsDock();
            return true;
        }
        return false;
    }

    if (o == m_docks.functions) {
        if (e->type() == QEvent::Close) {
            deleteFunctionsDock();
            return true;
        }
        return false;
    }

    if (o == m_docks.history) {
        if (e->type() == QEvent::Close) {
            deleteHistoryDock();
            return true;
        }
        return false;
    }

    if (o == m_docks.variables) {
        if (e->type() == QEvent::Close) {
            deleteVariablesDock();
            return true;
        }
        return false;
    }

    if (o == m_docks.userFunctions) {
        if (e->type() == QEvent::Close) {
            deleteUserFunctionsDock();
            return true;
        }
        return false;
    }

    if (o == m_docks.userUnits) {
        if (e->type() == QEvent::Close) {
            deleteUserUnitsDock();
            return true;
        }
        return false;
    }

    return QMainWindow::eventFilter(o, e);
}

void MainWindow::deleteKeypad(bool deferredDeletion)
{
    if (!m_widgets.keypad)
        return;

    disconnect(m_widgets.keypad);
    if (deferredDeletion)
        m_widgets.keypad->deleteLater();
    else
        delete m_widgets.keypad;
    m_widgets.keypad = 0;

    if (m_widgets.keypadContainer) {
        m_layouts.root->removeWidget(m_widgets.keypadContainer);
        if (deferredDeletion)
            m_widgets.keypadContainer->deleteLater();
        else
            delete m_widgets.keypadContainer;
        m_widgets.keypadContainer = 0;
    } else {
        m_layouts.root->removeItem(m_layouts.keypad);
        if (deferredDeletion)
            m_layouts.keypad->deleteLater();
        else
            delete m_layouts.keypad;
    }
    m_layouts.keypad = 0;
}

void MainWindow::deleteStatusBar()
{
    QStatusBar* existingStatusBar = findChild<QStatusBar*>(QString(), Qt::FindDirectChildrenOnly);
    if (existingStatusBar != nullptr)
        existingStatusBar->hide();

    if (m_status.angleUnitSection)
        m_status.angleUnitSection->deleteLater();
    m_status.angleUnit = 0;
    m_status.angleUnitSection = 0;
    m_status.angleUnitLabel = 0;

    if (m_status.resultFormatSection)
        m_status.resultFormatSection->deleteLater();
    m_status.resultFormat = 0;
    m_status.resultFormatSection = 0;
    m_status.resultFormatLabel = 0;
    if (m_status.resultPrecisionSeparator)
        m_status.resultPrecisionSeparator->deleteLater();
    m_status.resultPrecisionSeparator = 0;
    if (m_status.resultPrecisionSection)
        m_status.resultPrecisionSection->deleteLater();
    m_status.resultPrecision = 0;
    m_status.resultPrecisionSection = 0;
    m_status.resultPrecisionLabel = 0;
    if (m_status.angleUnitSeparator)
        m_status.angleUnitSeparator->deleteLater();
    m_status.angleUnitSeparator = 0;

    if (existingStatusBar != nullptr)
        setStatusBar(0);
}

void MainWindow::deleteBitField()
{
    if (!m_docks.bitField)
        return;

    deleteDock(m_docks.bitField);
    m_actions.viewBitfield->setChecked(false);
    m_settings->bitfieldVisible = false;
}

void MainWindow::deleteBookDock()
{
    if (!m_docks.book)
        return;

    deleteDock(m_docks.book);
    m_actions.viewFormulaBook->setChecked(false);
    m_settings->formulaBookDockVisible = false;
}

void MainWindow::deleteConstantsDock()
{
    if (!m_docks.constants)
        return;

    deleteDock(m_docks.constants);
    m_actions.viewConstants->setChecked(false);
    m_settings->constantsDockVisible = false;
}

void MainWindow::deleteFunctionsDock()
{
    if (!m_docks.functions)
        return;

    deleteDock(m_docks.functions);
    m_actions.viewFunctions->setChecked(false);
    m_settings->functionsDockVisible = false;
}

void MainWindow::deleteHistoryDock()
{
    if (!m_docks.history)
        return;

    deleteDock(m_docks.history);
    m_actions.viewHistory->setChecked(false);
    m_settings->historyDockVisible = false;
}

void MainWindow::deleteVariablesDock()
{
    if (!m_docks.variables)
        return;

    deleteDock(m_docks.variables);
    m_actions.viewVariables->setChecked(false);
    m_settings->variablesDockVisible = false;
}

void MainWindow::deleteUserFunctionsDock()
{
    if (!m_docks.userFunctions)
        return;

    deleteDock(m_docks.userFunctions);
    m_actions.viewUserFunctions->setChecked(false);
    m_settings->userFunctionsDockVisible = false;
}

void MainWindow::deleteUserUnitsDock()
{
    if (!m_docks.userUnits)
        return;

    deleteDock(m_docks.userUnits);
    m_actions.viewUserUnits->setChecked(false);
    m_settings->userUnitsDockVisible = false;
}

void MainWindow::setFunctionsDockVisible(bool b, bool takeFocus)
{
    if (b)
        createFunctionsDock(takeFocus);
    else
        deleteFunctionsDock();
}

void MainWindow::setFormulaBookDockVisible(bool b, bool takeFocus)
{
    if (b)
        createBookDock(takeFocus);
    else
        deleteBookDock();
}

void MainWindow::setConstantsDockVisible(bool b, bool takeFocus)
{
    if (b)
        createConstantsDock(takeFocus);
    else
        deleteConstantsDock();
}

void MainWindow::setHistoryDockVisible(bool b, bool takeFocus)
{
    if (b)
        createHistoryDock(takeFocus);
    else
        deleteHistoryDock();
}

void MainWindow::setVariablesDockVisible(bool b, bool takeFocus)
{
    if (b)
        createVariablesDock(takeFocus);
    else
        deleteVariablesDock();
}

void MainWindow::setUserFunctionsDockVisible(bool b, bool takeFocus)
{
    if (b)
        createUserFunctionsDock(takeFocus);
    else
        deleteUserFunctionsDock();
}

void MainWindow::setUserUnitsDockVisible(bool b, bool takeFocus)
{
    if (b)
        createUserUnitsDock(takeFocus);
    else
        deleteUserUnitsDock();
}

void MainWindow::setKeypadVisible(bool b)
{
    if (b && !m_widgets.keypad)
        createKeypad();
    else if (!b && m_widgets.keypad)
        deleteKeypad();
}

void MainWindow::setKeypadMode(QAction* action)
{
    if (!action)
        return;

    const Settings::KeypadMode mode = static_cast<Settings::KeypadMode>(action->data().toInt());
    const bool isCustomMode = (mode == Settings::KeypadModeCustom);
    if (isCustomMode && !configureCustomKeypad()) {
        updateKeypadModeActionState();
        return;
    }

    if (m_keypadMode == mode && !isCustomMode)
        return;

    const bool wasVisible = isVisibleKeypadMode(m_keypadMode);
    const bool nowVisible = isVisibleKeypadMode(mode);
    m_keypadMode = mode;
    m_settings->keypadMode = mode;
    updateKeypadModeActionState();

    if (wasVisible && nowVisible) {
        deleteKeypad();
        createKeypad();
        return;
    }

    setKeypadVisible(nowVisible);
}

void MainWindow::setKeypadZoom(QAction* action)
{
    if (!action)
        return;

    const int zoomPercent = action->data().toInt();
    if (zoomPercent != 50 && zoomPercent != 100
            && zoomPercent != 150 && zoomPercent != 200)
        return;
    if (m_keypadZoomPercent == zoomPercent)
        return;

    m_keypadZoomPercent = zoomPercent;
    m_settings->keypadZoomPercent = zoomPercent;
    if (m_widgets.keypad) {
        deleteKeypad();
        createKeypad();
    }
}

bool MainWindow::configureCustomKeypad()
{
    CustomKeypadDialog dialog(m_settings->customKeypad, this);
    if (dialog.exec() != QDialog::Accepted)
        return false;

    m_settings->customKeypad = dialog.customKeypad();
    return true;
}

void MainWindow::setResultFormatBinary()
{
    setResultFormat('b');
    setStatusBarText();
}

void MainWindow::setResultFormatCartesian()
{
    if (m_settings->resultComplexForm == ComplexForm::Rectangular)
        return;

    m_settings->complexNumbers = true;
    m_settings->secondaryComplexNumbers = true;
    m_settings->tertiaryComplexNumbers = true;
    m_settings->quaternaryComplexNumbers = true;
    m_settings->quinaryComplexNumbers = true;
    m_settings->resultComplexForm = ComplexForm::Rectangular;
    m_settings->secondaryResultComplexForm = ComplexForm::Rectangular;
    m_settings->tertiaryResultComplexForm = ComplexForm::Rectangular;
    m_settings->quaternaryResultComplexForm = ComplexForm::Rectangular;
    m_settings->quinaryResultComplexForm = ComplexForm::Rectangular;
    DMath::complexMode = true;
    setStatusBarText();
    emit resultFormatChanged();
}

void MainWindow::setResultFormatEngineering()
{
    setResultFormat('n');
    setStatusBarText();
}

void MainWindow::setResultFormatFixed()
{
    setResultFormat('f');
    setStatusBarText();
}
void MainWindow::setResultFormatGeneral()
{
    setResultFormat('g');
    setStatusBarText();
}

void MainWindow::setResultFormatHexadecimal()
{
    setResultFormat('h');
    setStatusBarText();
}

void MainWindow::setImaginaryUnitI()
{
    if (m_settings->imaginaryUnit == 'i')
        return;

    m_settings->complexNumbers = true;
    DMath::complexMode = true;
    m_settings->imaginaryUnit = 'i';
    CMath::setImaginaryUnitSymbol(QLatin1Char('i'));
    m_evaluator->initializeBuiltInVariables();
    setStatusBarText();
    emit complexNumbersChanged();
    emit resultFormatChanged();
}

void MainWindow::setImaginaryUnitJ()
{
    if (m_settings->imaginaryUnit == 'j')
        return;

    m_settings->complexNumbers = true;
    DMath::complexMode = true;
    m_settings->imaginaryUnit = 'j';
    CMath::setImaginaryUnitSymbol(QLatin1Char('j'));
    m_evaluator->initializeBuiltInVariables();
    setStatusBarText();
    emit complexNumbersChanged();
    emit resultFormatChanged();
}

void MainWindow::setResultFormatOctal()
{
    setResultFormat('o');
    setStatusBarText();
}

void MainWindow::setResultFormatPolar()
{
    if (m_settings->resultComplexForm == ComplexForm::Exponential)
        return;

    m_settings->complexNumbers = true;
    m_settings->secondaryComplexNumbers = true;
    m_settings->tertiaryComplexNumbers = true;
    m_settings->quaternaryComplexNumbers = true;
    m_settings->quinaryComplexNumbers = true;
    m_settings->resultComplexForm = ComplexForm::Exponential;
    m_settings->secondaryResultComplexForm = ComplexForm::Exponential;
    m_settings->tertiaryResultComplexForm = ComplexForm::Exponential;
    m_settings->quaternaryResultComplexForm = ComplexForm::Exponential;
    m_settings->quinaryResultComplexForm = ComplexForm::Exponential;
    DMath::complexMode = true;
    setStatusBarText();
    emit resultFormatChanged();
}

void MainWindow::setResultFormatTrigonometric()
{
    if (m_settings->resultComplexForm == ComplexForm::Trigonometric)
        return;

    m_settings->complexNumbers = true;
    m_settings->secondaryComplexNumbers = true;
    m_settings->tertiaryComplexNumbers = true;
    m_settings->quaternaryComplexNumbers = true;
    m_settings->quinaryComplexNumbers = true;
    m_settings->resultComplexForm = ComplexForm::Trigonometric;
    m_settings->secondaryResultComplexForm = ComplexForm::Trigonometric;
    m_settings->tertiaryResultComplexForm = ComplexForm::Trigonometric;
    m_settings->quaternaryResultComplexForm = ComplexForm::Trigonometric;
    m_settings->quinaryResultComplexForm = ComplexForm::Trigonometric;
    DMath::complexMode = true;
    setStatusBarText();
    emit resultFormatChanged();
}

void MainWindow::setResultFormatCis()
{
    if (m_settings->resultComplexForm == ComplexForm::Cis)
        return;

    m_settings->complexNumbers = true;
    m_settings->secondaryComplexNumbers = true;
    m_settings->tertiaryComplexNumbers = true;
    m_settings->quaternaryComplexNumbers = true;
    m_settings->quinaryComplexNumbers = true;
    m_settings->resultComplexForm = ComplexForm::Cis;
    m_settings->secondaryResultComplexForm = ComplexForm::Cis;
    m_settings->tertiaryResultComplexForm = ComplexForm::Cis;
    m_settings->quaternaryResultComplexForm = ComplexForm::Cis;
    m_settings->quinaryResultComplexForm = ComplexForm::Cis;
    DMath::complexMode = true;
    setStatusBarText();
    emit resultFormatChanged();
}

void MainWindow::setResultFormatPolarAngle()
{
    if (m_settings->resultComplexForm == ComplexForm::Phasor)
        return;

    m_settings->complexNumbers = true;
    m_settings->secondaryComplexNumbers = true;
    m_settings->tertiaryComplexNumbers = true;
    m_settings->quaternaryComplexNumbers = true;
    m_settings->quinaryComplexNumbers = true;
    m_settings->resultComplexForm = ComplexForm::Phasor;
    m_settings->secondaryResultComplexForm = ComplexForm::Phasor;
    m_settings->tertiaryResultComplexForm = ComplexForm::Phasor;
    m_settings->quaternaryResultComplexForm = ComplexForm::Phasor;
    m_settings->quinaryResultComplexForm = ComplexForm::Phasor;
    DMath::complexMode = true;
    setStatusBarText();
    emit resultFormatChanged();
}

void MainWindow::showAngleModeContextMenu(const QPoint& point)
{
    m_menus.angleUnit->popup(m_status.angleUnit->mapToGlobal(point));
}

void MainWindow::setResultFormatScientific()
{
    setResultFormat('e');
    setStatusBarText();
}

void MainWindow::setResultFormatRational()
{
    setResultFormat('r');
    setStatusBarText();
}

void MainWindow::setResultFormatSexagesimal()
{
    setResultFormat('s');
    setStatusBarText();
}

void MainWindow::insertConstantIntoEditor(const QString& c)
{
    if (c.isEmpty())
        return;

    // Insertion takes precedence over focus recovery from the dock click.
    setPendingDockFocusTarget(nullptr);
    setPendingDockTextInputFocusTarget(nullptr);
    QString s = c;
    s.replace(MathDsl::DotSep, m_settings->radixCharacter());
    insertTextIntoEditor(s);
}

void MainWindow::insertHistoryExpressionIntoEditor(const QString& text)
{
    const QString prefix = m_widgets.editor->text().left(m_widgets.editor->textCursor().selectionStart());
    insertTextIntoEditor(prefix.contains(MathDsl::CommentSep)
        ? text : Editor::historyExpressionForInput(text));
}

void MainWindow::insertTextIntoEditor(const QString& s)
{
    if (s.isEmpty())
        return;

    const QString prefix = m_widgets.editor->text().left(m_widgets.editor->textCursor().selectionStart());
    const QString normalized = !m_settings->automaticInputAdjustments || prefix.contains(MathDsl::CommentSep)
        ? s : EditorUtils::normalizeExpressionOperatorsForEditorInput(s);
    const bool atExpressionStart = [&]() {
        const QString text = m_widgets.editor->text();
        int i = m_widgets.editor->textCursor().position() - 1;
        while (i >= 0 && text.at(i).isSpace())
            --i;
        return i < 0;
    }();
    if (m_settings->automaticInputAdjustments && atExpressionStart && !normalized.isEmpty()) {
        int firstNonSpace = 0;
        while (firstNonSpace < normalized.size() && normalized.at(firstNonSpace).isSpace())
            ++firstNonSpace;

        if (firstNonSpace < normalized.size()) {
            const QChar leadingChar = normalized.at(firstNonSpace);
            if (!EditorUtils::isAllowedLeadingCharAtExpressionStart(leadingChar, m_settings->autoAns))
                return;
        }
    }

    bool shouldAutoComplete = m_widgets.editor->isAutoCompletionEnabled();
    m_widgets.editor->setAutoCompletionEnabled(false);
    m_widgets.editor->insert(normalized);
    m_widgets.editor->setAutoCompletionEnabled(shouldAutoComplete);

    if (!isActiveWindow())
        activateWindow();
    m_widgets.editor->setFocus();
}

void MainWindow::insertFunctionIntoEditor(const QString& f)
{
    if (f.isEmpty())
        return;

    const QString functionCall = f + QStringLiteral("()");
    const bool keepAsciiFunctionName =
        (f.compare(QStringLiteral("sqrt"), Qt::CaseInsensitive) == 0
         || f.compare(QStringLiteral("cbrt"), Qt::CaseInsensitive) == 0
         || f.compare(QStringLiteral("summation"), Qt::CaseInsensitive) == 0);
    if (keepAsciiFunctionName) {
        bool shouldAutoComplete = m_widgets.editor->isAutoCompletionEnabled();
        m_widgets.editor->setAutoCompletionEnabled(false);
        m_widgets.editor->insertPlainText(functionCall);
        m_widgets.editor->setAutoCompletionEnabled(shouldAutoComplete);

        if (!isActiveWindow())
            activateWindow();
        m_widgets.editor->setFocus();
    } else {
        insertTextIntoEditor(functionCall);
    }

    QTextCursor cursor = m_widgets.editor->textCursor();
    cursor.movePosition(QTextCursor::PreviousCharacter);
    m_widgets.editor->setTextCursor(cursor);
}

void MainWindow::handleKeypadButtonPress(Keypad::Button b)
{
    const auto typeWithRules = [this](const QString& s) {
        typeTextThroughEditorInputRules(m_widgets.editor, s);
        if (!isActiveWindow())
            activateWindow();
        m_widgets.editor->setFocus();
    };

    switch (b) {
    case Keypad::Key0: typeWithRules("0"); break;
    case Keypad::Key1: typeWithRules("1"); break;
    case Keypad::Key2: typeWithRules("2"); break;
    case Keypad::Key3: typeWithRules("3"); break;
    case Keypad::Key4: typeWithRules("4"); break;
    case Keypad::Key5: typeWithRules("5"); break;
    case Keypad::Key6: typeWithRules("6"); break;
    case Keypad::Key7: typeWithRules("7"); break;
    case Keypad::Key8: typeWithRules("8"); break;
    case Keypad::Key9: typeWithRules("9"); break;

    case Keypad::KeyPlus: typeWithRules("+"); break;
    case Keypad::KeyMinus: typeWithRules("−"); break;
    case Keypad::KeyTimes: typeWithRules(QString(MathDsl::MulCrossOp)); break;
    case Keypad::KeyDivide: typeWithRules("÷"); break;

    case Keypad::KeyEE: insertTextIntoEditor("e"); break;
    case Keypad::KeyLeftPar: typeWithRules("("); break;
    case Keypad::KeyRightPar: typeWithRules(")"); break;
    case Keypad::KeyRaise: typeWithRules("^"); break;
    case Keypad::KeyBackspace: {
        m_widgets.editor->doBackspace();
        if (!isActiveWindow())
            activateWindow();
        m_widgets.editor->setFocus();
        break;
    }
    case Keypad::KeyPercent: typeWithRules("%"); break;
    case Keypad::KeyFactorial: typeWithRules("!"); break;

    case Keypad::KeySquare: typeWithRules("^2"); break;
    case Keypad::KeyArgumentSeparator: typeWithRules(";"); break;
    case Keypad::KeyPi: insertTextIntoEditor("pi"); break;
    case Keypad::KeyAns: insertTextIntoEditor("ans"); break;

    case Keypad::KeySqrt: insertTextIntoEditor("sqrt("); break;
    case Keypad::KeyRoot: insertTextIntoEditor("root("); break;
    case Keypad::KeyLog10: insertTextIntoEditor("log10("); break;
    case Keypad::KeyMod: insertTextIntoEditor("mod("); break;
    case Keypad::KeyLn: insertTextIntoEditor("ln("); break;
    case Keypad::KeyExp:insertTextIntoEditor("exp("); break;
    case Keypad::KeySin: insertTextIntoEditor("sin("); break;
    case Keypad::KeyCos: insertTextIntoEditor("cos("); break;
    case Keypad::KeyTan: insertTextIntoEditor("tan("); break;
    case Keypad::KeyAcos: insertTextIntoEditor("arccos("); break;
    case Keypad::KeyAtan: insertTextIntoEditor("arctan("); break;
    case Keypad::KeyAsin: insertTextIntoEditor("arcsin("); break;

    case Keypad::KeyRadixChar: typeWithRules(QString(m_settings->radixCharacter())); break;

    case Keypad::KeyClear: clearEditor(); break;
    case Keypad::KeyEquals: evaluateEditorExpression(); break;

    default: break;
    }
}

void MainWindow::handleCustomKeypadButtonPress(int action, const QString& text)
{
    const auto typeWithRules = [this](const QString& s) {
        typeTextThroughEditorInputRules(m_widgets.editor, s);
        if (!isActiveWindow())
            activateWindow();
        m_widgets.editor->setFocus();
    };

    switch (static_cast<Settings::CustomKeypadButtonAction>(action)) {
    case Settings::CustomKeypadActionInsertText:
        typeWithRules(text);
        break;
    case Settings::CustomKeypadActionBackspace: {
        m_widgets.editor->doBackspace();
        if (!isActiveWindow())
            activateWindow();
        m_widgets.editor->setFocus();
        break;
    }
    case Settings::CustomKeypadActionClearExpression:
        clearEditor();
        break;
    case Settings::CustomKeypadActionEvaluateExpression:
        evaluateEditorExpression();
        break;
    default:
        break;
    }
}

#ifdef SPEEDCRUNCH_ENABLE_UPDATE_CHECKS
void MainWindow::checkForUpdates()
{
    if (!m_versionCheck)
        return;
    m_versionCheck->checkForUpdateNow();
}
#endif

void MainWindow::openFeedbackURL()
{
    QDesktopServices::openUrl(QUrl(QString::fromLatin1(kFeedbackUrl)));
}

void MainWindow::openSourceURL()
{
    QDesktopServices::openUrl(QUrl(QString::fromLatin1(kSourceUrl)));
}

void MainWindow::openCommunityURL()
{
    QDesktopServices::openUrl(QUrl(QString::fromLatin1(kCommunityUrl)));
}

void MainWindow::openDonateURL()
{
    QDesktopServices::openUrl(QUrl(QString::fromLatin1(kDonateUrl)));
}

void MainWindow::copy()
{
    m_copyWidget->copy();
}

void MainWindow::restoreSession(bool restoreHistory) {
    StartupDiagnostics::Scope timing("Migrate history and schedule session restore");
    migrateLegacyHistoryIfNeeded();
    ensureSessionsPath();

    if (restoreSessionLayout(restoreHistory))
        return;

    restoreWindowLayoutState(m_settings->windowState);

    const QString name = m_session->name();
    const QString filePath = sessionFilePath(name);
    QPointer<MainWindow> windowGuard(this);
    const auto startupTask = StartupDiagnostics::task("Restore single session (async total)");
    QThread* thread = QThread::create([windowGuard, filePath, name, restoreHistory, startupTask]() {
        StartupDiagnostics::Scope timing("Read single session JSON");
        QJsonObject json;
        const bool ok = readValidSessionJson(filePath, &json);
        if (ok && !restoreHistory)
            json.remove(QLatin1String(SessionJsonKeys::History));

        if (!windowGuard)
            return;

        timing.finish();
        QMetaObject::invokeMethod(windowGuard.data(), [windowGuard, ok, json, name, restoreHistory, startupTask]() mutable {
            StartupDiagnostics::Scope timing("Apply restored single session");
            if (!windowGuard)
                return;
            if (!ok) {
                windowGuard->finishStartupSessionRestore(false);
                return;
            }

            Session* loadedSession = new Session();
            loadedSession->deSerialize(json, false);
            loadedSession->setName(name);
            MainWindow* window = windowGuard.data();
            Session* oldSession = window->m_session;
            window->m_loadedSessions.remove(oldSession ? oldSession->name() : QString());
            window->m_loadedSessions.insert(name, loadedSession);
            window->activateSession(loadedSession);
            if (oldSession != nullptr)
                delete oldSession;

            // The initial session receives globals before this async load finishes.
            window->applyUserDefinitions();
            window->refreshPaneThemes();
            emit window->historyChanged();
            emit window->variablesChanged();
            emit window->functionsChanged();
            emit window->unitsChanged();
            window->restoreEditorTextFromCurrentSession();
            window->m_conditions.autoAns = restoreHistory && !loadedSession->historyIsEmpty();
            window->finishStartupSessionRestore(true);
        }, Qt::QueuedConnection);
    });
    {
        QMutexLocker locker(&asyncSessionIoMutex());
        asyncSessionIoThreads().append(thread);
    }
    QObject::connect(thread, &QThread::finished, thread, [thread]() {
        unregisterAsyncSessionIoThread(thread);
        thread->deleteLater();
    });
    thread->start();
}

bool MainWindow::restoreSessionLayout(bool restoreHistory)
{
    StartupDiagnostics::Scope timing("Parse saved window layout");
    if (m_settings->sessionLayoutJson.isEmpty())
        return false;

    const QJsonDocument layoutDoc = QJsonDocument::fromJson(m_settings->sessionLayoutJson.toUtf8());
    if (!layoutDoc.isObject())
        return false;

    const QJsonObject layout = layoutDoc.object();
    if (layout.value(QStringLiteral("scheme")).toInt() != 1
            || layout.value(QStringLiteral("kind")).toString() != QLatin1String("session-layout")) {
        return false;
    }

    const QJsonArray windows = layout.value(QStringLiteral("windows")).toArray();
    if (windows.isEmpty())
        return false;

    const QString activeWindowId = layout.value(QStringLiteral("activeWindow")).toString();
    QJsonObject window;
    for (const QJsonValue& value : windows) {
        if (!value.isObject())
            continue;
        const QJsonObject candidate = value.toObject();
        if ((!activeWindowId.isEmpty() && candidate.value(QStringLiteral("id")).toString() == activeWindowId)
                || (activeWindowId.isEmpty() && candidate.value(QStringLiteral("active")).toBool())) {
            window = candidate;
            break;
        }
    }
    if (window.isEmpty() && windows.first().isObject())
        window = windows.first().toObject();
    if (window.isEmpty())
        return false;

    const QJsonObject root = window.value(QStringLiteral("root")).toObject();
    const QString rootType = root.value(QStringLiteral("type")).toString();
    if (rootType != QLatin1String("tabs") && rootType != QLatin1String("split"))
        return false;

    QJsonArray tabs = rootType == QLatin1String("tabs")
        ? root.value(QStringLiteral("tabs")).toArray()
        : QJsonArray();
    const auto appendPaneTabs = [&tabs](const QJsonObject& node, const auto& appendPaneTabsRef) -> void {
        const QString type = node.value(QStringLiteral("type")).toString();
        if (type == QLatin1String("pane")) {
            const QJsonArray paneTabs = node.value(QStringLiteral("tabs")).toArray();
            for (const QJsonValue& tabValue : paneTabs)
                tabs.append(tabValue);
            return;
        }
        if (type != QLatin1String("split"))
            return;
        const QJsonArray children = node.value(QStringLiteral("children")).toArray();
        for (const QJsonValue& childValue : children) {
            if (childValue.isObject())
                appendPaneTabsRef(childValue.toObject(), appendPaneTabsRef);
        }
    };
    if (rootType == QLatin1String("split"))
        appendPaneTabs(root, appendPaneTabs);
    if (tabs.isEmpty())
        return false;

    // Window-local chrome is independent of session-file I/O. Restore it now
    // so every window, including the first, has its final UI before it is shown.
    restoreWindowUiState(window);

    QString activeSessionName = normalizedSessionName(root.value(QStringLiteral("active")).toString());
    QList<SessionLoadSpec> sessionLoadSpecs;
    QStringList loadNames;

    for (const QJsonValue& value : tabs) {
        if (!value.isObject())
            continue;

        const QJsonObject tab = value.toObject();
        const QString name = normalizedSessionName(tab.value(QStringLiteral("name")).toString());
        if (loadNames.contains(name, Qt::CaseInsensitive))
            continue;
        loadNames.append(name);

        QString fileName = tab.value(QStringLiteral("file")).toString();
        if (fileName.isEmpty())
            fileName = sessionFileBaseName(name) + QLatin1String(".json");

        const QString filePath = QDir(sessionsPath()).filePath(fileName);
        sessionLoadSpecs.append(SessionLoadSpec{ name, filePath, tab });
    }

    if (sessionLoadSpecs.isEmpty())
        return false;

    QPointer<MainWindow> windowGuard(this);
    const auto startupTask = StartupDiagnostics::task("Restore window sessions (async total)");
    QThread* thread = QThread::create([windowGuard, sessionLoadSpecs, activeSessionName,
                                       layout, window, root, tabs, restoreHistory, startupTask]() {
        StartupDiagnostics::Scope timing("Read saved session files (worker total)");
        QHash<QString, QJsonObject> sessionJsons;
        QHash<QString, QPair<int, int>> viewportAnchors;
        QHash<QString, int> scrollValues;

        for (const SessionLoadSpec& spec : sessionLoadSpecs) {
            StartupDiagnostics::Scope fileTiming("Read and parse session JSON");
            QJsonObject sessionJson;
            if (!readValidSessionJson(spec.filePath, &sessionJson))
                continue;

            if (!restoreHistory)
                sessionJson.remove(QLatin1String(SessionJsonKeys::History));

            sessionJson.insert(QLatin1String(SessionJsonKeys::Session), spec.name);
            sessionJsons.insert(spec.name, sessionJson);

            const QJsonObject scroll = spec.tab.value(QStringLiteral("scroll")).toObject();
            const int block = scroll.value(QStringLiteral("block")).toInt(-1);
            const int offset = scroll.value(QStringLiteral("offset")).toInt(0);
            const int scrollValue = scroll.value(QStringLiteral("value")).toInt(-1);
            if (block >= 0)
                viewportAnchors.insert(spec.name, qMakePair(block, offset));
            if (scrollValue >= 0)
                scrollValues.insert(spec.name, scrollValue);
        }

        if (!windowGuard)
            return;

        timing.finish();
        QMetaObject::invokeMethod(windowGuard.data(), [windowGuard, layout, window, root, tabs,
                                                       activeSessionName, restoreHistory,
                                                       sessionJsons, viewportAnchors,
                                                       scrollValues, startupTask]() mutable {
            StartupDiagnostics::Scope timing("Apply restored window sessions (total)");
            if (!windowGuard)
                return;
            windowGuard->finishRestoreSessionLayout(layout, window, root, tabs, activeSessionName,
                                                    restoreHistory, sessionJsons, viewportAnchors,
                                                    scrollValues);
        }, Qt::QueuedConnection);
    });
    {
        QMutexLocker locker(&asyncSessionIoMutex());
        asyncSessionIoThreads().append(thread);
    }
    QObject::connect(thread, &QThread::finished, thread, [thread]() {
        unregisterAsyncSessionIoThread(thread);
        thread->deleteLater();
    });
    thread->start();
    return true;
}

void MainWindow::finishRestoreSessionLayout(const QJsonObject& layout,
                                            const QJsonObject& window,
                                            const QJsonObject& root,
                                            const QJsonArray& tabs,
                                            const QString& activeSessionName,
                                            bool restoreHistory,
                                            QHash<QString, QJsonObject> sessionJsons,
                                            QHash<QString, QPair<int, int>> viewportAnchors,
                                            QHash<QString, int> scrollValues)
{
    StartupDiagnostics::AppearanceContext context("Restore session panes");
    StartupDiagnostics::Scope timing("Deserialize saved sessions (total)");
    const QJsonArray windows = layout.value(QStringLiteral("windows")).toArray();
    const QString rootType = root.value(QStringLiteral("type")).toString();

    if (sessionJsons.isEmpty()) {
        finishStartupSessionRestore(false);
        return;
    }

    QHash<QString, Session*> restoredSessions;
    for (auto it = sessionJsons.constBegin(); it != sessionJsons.constEnd(); ++it) {
        Session* session = new Session();
        session->deSerialize(it.value(), false);
        session->setName(it.key());
        restoredSessions.insert(it.key(), session);
    }

    timing.next("Rebuild session panes and tabs");
    m_sessionViewportAnchors = viewportAnchors;
    m_sessionScrollValues = scrollValues;

    QString activeSessionNameToRestore = activeSessionName;
    Session* activeSession = restoredSessions.value(activeSessionNameToRestore, nullptr);

    if (activeSession == nullptr) {
        activeSessionNameToRestore = restoredSessions.keys().constFirst();
        activeSession = restoredSessions.value(activeSessionNameToRestore);
    }

    const QFont displayFont = m_widgets.display->font();
    const QFont editorFont = m_widgets.editor->font();
    QWidget* initialPane = paneWidgetForDisplay(m_widgets.display);
    std::unique_ptr<QWidget> reusablePane(
        m_widgets.splitContainer->count() == 1
            && initialPane == m_widgets.splitContainer->widget(0) ? initialPane : nullptr);
    ResultDisplay* initialDisplay = reusablePane ? m_widgets.display : nullptr;
    Editor* initialEditor = reusablePane ? m_widgets.editor : nullptr;
    QTabBar* initialTabBar = m_paneTabBars.value(initialDisplay, nullptr);
    if (initialEditor != nullptr) {
        initialEditor->stopAutoCalc();
        initialEditor->stopAutoComplete();
    }
    const QList<Session*> oldSessions = m_loadedSessions.values();
    m_loadedSessions = restoredSessions;
    m_session = nullptr;
    m_evaluator = nullptr;

    m_widgets.display = nullptr;
    m_widgets.editor = nullptr;
    m_copyWidget = nullptr;
    // Keep the retained pane parented. A split layout can transfer it directly
    // to its new splitter without turning the whole pane into a top-level widget.
    for (int i = m_widgets.splitContainer->count() - 1; i >= 0; --i) {
        QWidget* child = m_widgets.splitContainer->widget(i);
        if (child != reusablePane.get())
            delete child;
    }

    m_paneSessionNames.clear();
    m_paneSessionTabs.clear();
    m_paneTabBars.clear();
    m_tabBarDisplays.clear();

    ResultDisplay* activeDisplay = nullptr;
    Editor* activeEditor = nullptr;
    ResultDisplay* firstDisplay = nullptr;
    Editor* firstEditor = nullptr;

    const auto paneNamesFromNode = [](const QJsonObject& node) -> QStringList {
        QStringList names;
        const QJsonArray paneTabs = node.value(QStringLiteral("tabs")).toArray();
        for (const QJsonValue& tabValue : paneTabs) {
            if (!tabValue.isObject())
                continue;
            const QString name = normalizedSessionName(tabValue.toObject().value(QStringLiteral("name")).toString());
            if (!name.isEmpty() && !names.contains(name, Qt::CaseInsensitive))
                names.append(name);
        }
        const QString activeName = normalizedSessionName(node.value(QStringLiteral("active")).toString());
        if (!activeName.isEmpty() && !names.contains(activeName, Qt::CaseInsensitive))
            names.prepend(activeName);
        return names;
    };

    const auto firstLoadedName = [this](const QStringList& names) -> QString {
        for (const QString& name : names) {
            if (m_loadedSessions.contains(name))
                return name;
        }
        return QString();
    };

    const auto editorStateFromNode = [](const QJsonObject& node, const QString& activeName) -> QJsonObject {
        const QJsonArray paneTabs = node.value(QStringLiteral("tabs")).toArray();
        for (const QJsonValue& tabValue : paneTabs) {
            if (!tabValue.isObject())
                continue;
            const QJsonObject tab = tabValue.toObject();
            const QString tabName = normalizedSessionName(tab.value(QStringLiteral("name")).toString());
            if (tabName.compare(activeName, Qt::CaseInsensitive) == 0)
                return tab.value(QStringLiteral("editor")).toObject();
        }
        return QJsonObject();
    };

    const auto createPane = [this, &displayFont, &editorFont, &activeSessionNameToRestore, &paneNamesFromNode,
                             &activeDisplay, &activeEditor, &firstDisplay, &firstEditor,
                             &reusablePane, initialDisplay, initialEditor, initialTabBar,
                             &firstLoadedName, &editorStateFromNode](const QJsonObject& node) -> QWidget* {
        const QStringList names = paneNamesFromNode(node);
        QStringList loadedNames;
        for (const QString& name : names) {
            if (m_loadedSessions.contains(name) && !loadedNames.contains(name, Qt::CaseInsensitive))
                loadedNames.append(name);
        }
        if (loadedNames.isEmpty())
            return nullptr;

        QString activeName = normalizedSessionName(node.value(QStringLiteral("active")).toString());
        if (!m_loadedSessions.contains(activeName))
            activeName = firstLoadedName(loadedNames);
        if (activeName.isEmpty())
            return nullptr;
        if (!loadedNames.contains(activeName, Qt::CaseInsensitive))
            loadedNames.prepend(activeName);

        ResultDisplay* display = nullptr;
        Editor* editor = nullptr;
        QWidget* pane = nullptr;
        if (reusablePane) {
            StartupDiagnostics::Scope reuseTiming("Reuse initial session pane");
            pane = reusablePane.release();
            display = initialDisplay;
            editor = initialEditor;
            // Keep the existing signal connections and the configured controls.
            m_paneTabBars.insert(display, initialTabBar);
            m_tabBarDisplays.insert(initialTabBar, display);
        } else {
            display = new ResultDisplay();
            display->setFrameStyle(QFrame::NoFrame);
            display->setFont(displayFont);
            display->setHoverHighlightEnabled(m_settings->hoverHighlightResults);
            display->setLoadedSessionCount(1);
            display->rehighlight();

            editor = new Editor();
            editor->setFrameStyle(QFrame::NoFrame);
            editor->setFont(editorFont);
            editor->setAutoCalcEnabled(m_settings->autoCalc);
            editor->setAutoCompletionEnabled(m_settings->autoCompletion);
            editor->rehighlight();

            pane = createEditorDisplayPane(display, editor);
            configureEditorDisplayPane(display, editor);
        }

        m_paneSessionNames.insert(display, activeName);
        m_paneSessionTabs.insert(display, loadedNames);
        Session* paneSession = m_loadedSessions.value(activeName, nullptr);
        const QSignalBlocker displaySignals(display);
        const QSignalBlocker editorSignals(editor);
        // The final historyChanged signal renders after settings and themes are ready.
        display->setSession(paneSession, false);
        if (paneSession != nullptr)
            editor->setSession(paneSession);
        restoreEditorState(editor,
                           editorStateFromNode(node, activeName),
                           paneSession != nullptr ? paneSession->editorText() : QString());

        if (firstDisplay == nullptr) {
            firstDisplay = display;
            firstEditor = editor;
        }
        if (activeName == activeSessionNameToRestore) {
            activeDisplay = display;
            activeEditor = editor;
        }
        return pane;
    };

    const auto restoreSplitterSizes = [](QSplitter* splitter, const QJsonObject& node) {
        QJsonArray splitSizes = node.value(QStringLiteral("sizes")).toArray();
        if (splitSizes.isEmpty())
            return;
        QList<int> sizes;
        for (const QJsonValue& value : splitSizes)
            sizes.append(value.toInt());
        if (sizes.size() == splitter->count())
            splitter->setSizes(sizes);
    };

    const auto restoreNode = [this, &createPane, &restoreSplitterSizes](const QJsonObject& node,
                                                                        const auto& restoreNodeRef) -> QWidget* {
        const QString type = node.value(QStringLiteral("type")).toString();
        if (type == QLatin1String("pane"))
            return createPane(node);

        QSplitter* splitter = new QSplitter(
            node.value(QStringLiteral("orientation")).toString() == QLatin1String("vertical")
                ? Qt::Vertical
                : Qt::Horizontal);
        splitter->setChildrenCollapsible(false);
        splitter->setHandleWidth(UiConfig::SessionPaneSplitterWidth);
        splitter->setStyleSheet(m_widgets.splitContainer->styleSheet());

        const QJsonArray children = node.value(QStringLiteral("children")).toArray();
        for (const QJsonValue& childValue : children) {
            if (!childValue.isObject())
                continue;
            QWidget* child = restoreNodeRef(childValue.toObject(), restoreNodeRef);
            if (child != nullptr)
                splitter->addWidget(child);
        }
        restoreSplitterSizes(splitter, node);
        return splitter;
    };

    if (rootType == QLatin1String("tabs")) {
        if (QWidget* pane = createPane(root)) {
            if (m_widgets.splitContainer->indexOf(pane) < 0)
                m_widgets.splitContainer->addWidget(pane);
        }
    } else if (rootType == QLatin1String("split")) {
        m_widgets.splitContainer->setOrientation(
            root.value(QStringLiteral("orientation")).toString() == QLatin1String("vertical")
                ? Qt::Vertical
                : Qt::Horizontal);
        const QJsonArray children = root.value(QStringLiteral("children")).toArray();
        int restoredIndex = 0;
        for (const QJsonValue& childValue : children) {
            if (!childValue.isObject())
                continue;
            QWidget* child = restoreNode(childValue.toObject(), restoreNode);
            if (child != nullptr) {
                if (m_widgets.splitContainer->indexOf(child) != restoredIndex)
                    m_widgets.splitContainer->insertWidget(restoredIndex, child);
                ++restoredIndex;
            }
        }
        restoreSplitterSizes(m_widgets.splitContainer, root);
    }

    if (m_widgets.splitContainer->count() == 0 || firstDisplay == nullptr) {
        for (int i = m_widgets.splitContainer->count() - 1; i >= 0; --i) {
            QWidget* child = m_widgets.splitContainer->widget(i);
            if (child != reusablePane.get())
                delete child;
        }
        m_paneSessionNames.clear();
        m_paneSessionTabs.clear();
        m_paneTabBars.clear();
        m_tabBarDisplays.clear();
        activeDisplay = nullptr;
        activeEditor = nullptr;
        firstDisplay = nullptr;
        firstEditor = nullptr;

        QJsonObject pane;
        pane.insert(QStringLiteral("type"), QStringLiteral("pane"));
        pane.insert(QStringLiteral("active"), activeSessionNameToRestore);
        pane.insert(QStringLiteral("tabs"), tabs);
        m_widgets.splitContainer->addWidget(createPane(pane));
    }

    m_widgets.display = activeDisplay ? activeDisplay : firstDisplay;
    m_widgets.editor = activeEditor ? activeEditor : firstEditor;
    m_copyWidget = m_widgets.editor;
    updatePaneLoadedSessionCounts();
    m_session = nullptr;
    activateSession(activeSession);
    // Apply globals to every restored tab after replacing the initial sessions.
    applyUserDefinitions();
    // Rebind retained widgets and docks before releasing the initial session.
    const QList<Session*> retainedSessions = restoredSessions.values();
    for (Session* oldSession : oldSessions) {
        if (!retainedSessions.contains(oldSession))
            delete oldSession;
    }
    m_conditions.autoAns = restoreHistory && !m_session->historyIsEmpty();
    updatePaneEditorCursorVisibility();
    timing.next("Restore per-window settings and theme");
    // Pane rebuilding keeps the docks and keypad restored before session loading.
    // Reapply selections after the central layout changes.
    restoreWindowUiState(window, false);
    // Existing controls keep their theme. Refresh the new panes below.
    restoreWindowThemeBackgrounds();
    refreshPaneThemes();
    emit historyChanged();
    emit variablesChanged();
    emit functionsChanged();
    emit unitsChanged();
    restoreVisibleSessionViewports();
    const auto viewportTask = StartupDiagnostics::task("Deferred history viewport restoration");
    QTimer::singleShot(0, this, [this, viewportTask]() {
        restoreVisibleSessionViewports();
    });

    timing.next("Restore additional windows");
    if (this == primaryMainWindow() && !g_restoringExtraWindows && !g_multiWindowSpawnDone && windows.size() > 1) {
        g_multiWindowSpawnDone = true;
        g_restoringExtraWindows = true;
        for (int i = 0; i < windows.size(); ++i) {
            const QJsonValue value = windows.at(i);
            if (!value.isObject())
                continue;
            const QJsonObject candidate = value.toObject();
            if (candidate.value(QStringLiteral("id")).toString() == window.value(QStringLiteral("id")).toString())
                continue;

            QJsonObject singleLayout = layout;
            singleLayout.insert(QStringLiteral("windows"), QJsonArray({ candidate }));
            singleLayout.insert(QStringLiteral("activeWindow"), candidate.value(QStringLiteral("id")).toString());
            const QString previousLayoutJson = m_settings->sessionLayoutJson;
            m_settings->sessionLayoutJson = QString::fromUtf8(QJsonDocument(singleLayout).toJson(QJsonDocument::Compact));
            MainWindow* extraWindow = new MainWindow();
            m_settings->sessionLayoutJson = previousLayoutJson;
            const QString geometryBase64 = candidate.value(QStringLiteral("geometry")).toString();
            extraWindow->showRestoredWindow(
                QByteArray::fromBase64(geometryBase64.toLatin1()));
        }
        g_restoringExtraWindows = false;
    }
    finishStartupSessionRestore(true);
}

void MainWindow::restoreWindowUiState(const QJsonObject& window, bool restoreLayout)
{
    StartupDiagnostics::AppearanceScope timing(restoreLayout
        ? "Restore window UI state" : "Restore window selections and geometry", this);
    const QString restoredAngleUnit =
        window.value(QStringLiteral("statusBarAngleUnit")).toString();
    if (restoredAngleUnit.size() == 1
        && QByteArray("drgtv").contains(restoredAngleUnit.at(0).toLatin1())) {
        m_status.selectedAngleUnit = restoredAngleUnit.at(0).toLatin1();
    }
    const QString restoredResultFormat =
        window.value(QStringLiteral("statusBarResultFormat")).toString();
    if (restoredResultFormat.size() == 1
        && QByteArray("gnerhobsf").contains(restoredResultFormat.at(0).toLatin1())) {
        m_status.selectedResultFormat = restoredResultFormat.at(0).toLatin1();
    }
    if (window.contains(QStringLiteral("statusBarResultPrecision"))) {
        const int restoredPrecision =
            window.value(QStringLiteral("statusBarResultPrecision")).toInt(-1);
        if (restoredPrecision >= -1 && restoredPrecision <= 50)
            m_status.selectedResultPrecision = restoredPrecision;
    }
    if (restoreLayout && window.contains(QStringLiteral("statusBarVisible")))
        setStatusBarVisible(window.value(QStringLiteral("statusBarVisible")).toBool(true));
    else
        setStatusBarText();
    if (QApplication::activeWindow() == this)
        applyStatusBarSelectionState();
    if (restoreLayout) {
        if (window.value(QStringLiteral("dockColumnOrders")).isObject())
            restoreDockColumnOrders(window.value(QStringLiteral("dockColumnOrders")).toObject());
        const QString windowStateBase64 = window.value(QStringLiteral("windowState")).toString();
        const QByteArray state = windowStateBase64.isEmpty()
            ? m_settings->windowState
            : QByteArray::fromBase64(windowStateBase64.toLatin1());
        if (window.value(QStringLiteral("openDocks")).isArray()) {
            QStringList names;
            for (const QJsonValue& value : window.value(QStringLiteral("openDocks")).toArray()) {
                if (value.isString())
                    names.append(value.toString());
            }
            restoreDockLayout(state, names);
        } else {
            restoreWindowLayoutState(state);
        }
        if (!window.contains(QStringLiteral("openDocks")) && window.contains(QStringLiteral("bitfieldVisible")))
            setBitfieldVisible(window.value(QStringLiteral("bitfieldVisible")).toBool(false));
        if (window.contains(QStringLiteral("keypadVisible"))) {
            restoreWindowKeypadLayout(
                window.value(QStringLiteral("keypadVisible")).toBool(false),
                window.value(QStringLiteral("keypadMode")).toInt(static_cast<int>(m_keypadMode)));
        }
        if (window.contains(QStringLiteral("keypadZoomPercent"))) {
            restoreWindowKeypadZoom(
                window.value(QStringLiteral("keypadZoomPercent")).toInt(m_keypadZoomPercent));
        }
    }
    const QString geometryBase64 = window.value(QStringLiteral("geometry")).toString();
    if (restoreLayout && !geometryBase64.isEmpty())
        restoreWindowGeometry(QByteArray::fromBase64(geometryBase64.toLatin1()));
}

void MainWindow::restoreWindowGeometry(const QByteArray& geometry)
{
    if (geometry.isEmpty())
        return;

    StartupDiagnostics::Scope timing("Restore saved window geometry");
    if (!restoreGeometry(geometry))
        return;
    m_savedWindowGeometryRestored = true;

    // Restoring window controls and rebuilding session panes can change size
    // constraints on later event-loop turns. Reapply the saved geometry once
    // those layout passes have completed.
    QTimer::singleShot(0, this, [this, geometry]() {
        StartupDiagnostics::Scope timing("Restore geometry after layout events");
        restoreGeometry(geometry);
        QTimer::singleShot(0, this, [this, geometry]() {
            StartupDiagnostics::Scope timing("Restore geometry after native events");
            restoreGeometry(geometry);
        });
    });
}

void MainWindow::showRestoredWindow(const QByteArray& geometry)
{
    // Keep restored secondary windows hidden until their controls and central
    // layout have settled, so the first visible frame uses the saved layout.
    QTimer::singleShot(0, this, [this, geometry]() {
        QTimer::singleShot(0, this, [this, geometry]() {
            if (!geometry.isEmpty())
                restoreGeometry(geometry);
            showWhenReady();
        });
    });
}

void MainWindow::applyDockCornerLayout()
{
    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
}

QJsonObject MainWindow::dockColumnOrders() const
{
    // Keep unopened docks too. Their widgets may not exist yet on this launch.
    QJsonObject orders = m_dockColumnOrders;
    for (QDockWidget* dock : m_allDocks) {
        const QTreeWidget* table = dock->findChild<QTreeWidget*>();
        if (!table)
            continue;
        const QHeaderView* header = table->header();
        QJsonArray order;
        for (int visual = 0; visual < header->count(); ++visual)
            order.append(header->logicalIndex(visual));
        orders.insert(dock->objectName(), order);
    }
    return orders;
}

void MainWindow::restoreDockColumnOrders(const QJsonObject& orders)
{
    m_dockColumnOrders = orders;
    for (QDockWidget* dock : m_allDocks)
        restoreDockColumnOrder(dock);
}

void MainWindow::restoreDockColumnOrder(QDockWidget* dock)
{
    QTreeWidget* table = dock->findChild<QTreeWidget*>();
    if (!table)
        return;
    QHeaderView* header = table->header();
    const QJsonArray order = m_dockColumnOrders.value(dock->objectName()).toArray();
    QSet<int> columns;
    bool valid = order.size() == header->count();
    for (const QJsonValue& value : order) {
        const int column = value.toInt(-1);
        if (column < 0 || column >= header->count() || columns.contains(column)) {
            valid = false;
            break;
        }
        columns.insert(column);
    }
    // Ignore stale or malformed orders instead of applying a partial layout.
    for (int visual = 0; visual < header->count(); ++visual) {
        const int logical = valid ? order.at(visual).toInt() : visual;
        header->moveSection(header->visualIndex(logical), visual);
    }
}

void MainWindow::restoreWindowLayoutState(const QByteArray& state)
{
    bool valid = false;
    const QStringList names = state.isEmpty() ? QStringList() : legacyOpenDockNames(state, &valid);
    restoreDockLayout(state, valid ? names : configuredOpenDockNames());
}

QStringList MainWindow::openDockNames() const
{
    QStringList names;
    for (const QDockWidget* dock : m_allDocks) {
        // A panel behind another tab is open even when it is not exposed.
        if (!dock->isHidden())
            names.append(dock->objectName());
    }
    return names;
}

QStringList MainWindow::configuredOpenDockNames() const
{
    const bool enabled[] = {m_settings->formulaBookDockVisible, m_settings->constantsDockVisible,
        m_settings->functionsDockVisible, m_settings->historyDockVisible,
        m_settings->variablesDockVisible, m_settings->userFunctionsDockVisible,
        m_settings->userUnitsDockVisible, m_settings->bitfieldVisible};
    QStringList names;
    const QStringList available = allDockNames();
    for (int i = 0; i < available.size(); ++i) {
        if (enabled[i])
            names.append(available.at(i));
    }
    return names;
}

void MainWindow::applyOpenDocks(const QStringList& names)
{
    StartupDiagnostics::Scope timing("Create and configure open docks (total)");
    setFormulaBookDockVisible(names.contains(QStringLiteral("BookDock")), false);
    setConstantsDockVisible(names.contains(QStringLiteral("ConstantsDock")), false);
    setFunctionsDockVisible(names.contains(QStringLiteral("FunctionsDock")), false);
    setHistoryDockVisible(names.contains(QStringLiteral("HistoryDock")), false);
    setVariablesDockVisible(names.contains(QStringLiteral("VariablesDock")), false);
    setUserFunctionsDockVisible(names.contains(QStringLiteral("UserFunctionsDock")), false);
    setUserUnitsDockVisible(names.contains(QStringLiteral("UserUnitsDock")), false);
    setBitfieldVisible(names.contains(QStringLiteral("BitfieldDock")));
    syncViewMenuActionState();
}

void MainWindow::restoreDockLayout(const QByteArray& state, const QStringList& names)
{
    m_hasRestoredDockLayout = restoreState(state, DockLayoutStateVersion);
    applyOpenDocks(names);
    // Reapply the final sizes and selected tabs after adding the required docks.
    if (m_hasRestoredDockLayout)
        restoreState(state, DockLayoutStateVersion);
    for (QDockWidget* dock : m_allDocks) {
        if (!names.contains(dock->objectName()))
            dock->hide();
    }
    // Saved layouts include corner ownership. Keep side docks at full height
    // even when restoring a layout saved with top or bottom corners.
    applyDockCornerLayout();
    syncViewMenuActionState();
    if (m_initializingUi && state.isEmpty() && m_docks.constants != nullptr) {
        QPointer<QDockWidget> constantsDock(m_docks.constants);
        QTimer::singleShot(0, this, [this, constantsDock]() {
            if (constantsDock && !constantsDock->isHidden())
                resizeDocks({constantsDock}, {UiConfig::ConstantsDockDefaultWidth}, Qt::Horizontal);
        });
    }
}

void MainWindow::restoreWindowKeypadLayout(bool visible, int modeValue)
{
    if (modeValue < static_cast<int>(Settings::KeypadModeDisabled)
            || modeValue > static_cast<int>(Settings::KeypadModeCustom)) {
        modeValue = static_cast<int>(m_keypadMode);
    }

    Settings::KeypadMode mode = static_cast<Settings::KeypadMode>(modeValue);
    if (mode == Settings::KeypadModeBasicNarrow)
        mode = Settings::KeypadModeBasicWide;
    if (!visible || !isVisibleKeypadMode(mode))
        mode = Settings::KeypadModeDisabled;

    if (m_keypadMode != mode && m_widgets.keypad != nullptr)
        deleteKeypad(isVisible());
    m_keypadMode = mode;
    setKeypadVisible(isVisibleKeypadMode(mode));
    updateKeypadModeActionState();
}

void MainWindow::restoreWindowKeypadZoom(int zoomPercent)
{
    if (zoomPercent != 50 && zoomPercent != 100
            && zoomPercent != 150 && zoomPercent != 200)
        return;
    if (m_keypadZoomPercent == zoomPercent)
        return;

    m_keypadZoomPercent = zoomPercent;
    if (m_widgets.keypad != nullptr) {
        deleteKeypad(isVisible());
        createKeypad();
    }
}

void MainWindow::evaluateEditorExpression()
{
    const bool startedFromHistoryEdit = (m_pendingHistoryEditIndex >= 0);
    const QString enteredExpr = m_widgets.editor->text();
    QString expr = m_evaluator->autoFix(enteredExpr);
    const bool isCommentOnly = Evaluator::isCommentOnlyExpression(expr);

    if (expr.isEmpty())
        return;

    if (m_pendingHistoryEditIndex >= 0) {
        const int previousDisplayScrollValue = m_widgets.display->verticalScrollBar()->value();
        const auto restoreDisplayScroll = [this, previousDisplayScrollValue]() {
            QScrollBar* bar = m_widgets.display->verticalScrollBar();
            const int clamped = qBound(bar->minimum(), previousDisplayScrollValue, bar->maximum());
            bar->setValue(clamped);
            QTimer::singleShot(0, this, [this, previousDisplayScrollValue]() {
                QScrollBar* deferredBar = m_widgets.display->verticalScrollBar();
                const int deferredClamped = qBound(deferredBar->minimum(),
                                                   previousDisplayScrollValue,
                                                   deferredBar->maximum());
                deferredBar->setValue(deferredClamped);
            });
        };
        const int historySize = m_session->historySize();
        if (m_pendingHistoryEditIndex >= historySize) {
            m_pendingHistoryEditIndex = -1;
            m_widgets.display->setEditingHistoryIndex(-1);
            m_widgets.editor->setHistoryArrowNavigationEnabled(true);
            m_widgets.editor->clear();
            restoreDisplayScroll();
        } else {
            const QList<HistoryEntry> previousEntries = historyEntries();
            QList<HistoryEntry> updatedEntries = previousEntries;
            HistoryEntry updatedEntry = updatedEntries.at(m_pendingHistoryEditIndex);
            updatedEntry.setExpr(enteredExpr);
            updatedEntries[m_pendingHistoryEditIndex] = updatedEntry;

            int errorIndex = -1;
            QString errorText;
            if (!rebuildSessionFromEntries(updatedEntries, m_pendingHistoryEditIndex, &errorIndex, &errorText)) {
                m_widgets.display->setEditingHistoryIndex(m_pendingHistoryEditIndex);
                restoreDisplayScroll();
                showStateLabel(tr("Could not recalculate from calculation %1: %2").arg(errorIndex + 1).arg(errorText));
                return;
            }

            m_pendingHistoryEditIndex = -1;
            m_widgets.display->setEditingHistoryIndex(-1);
            m_widgets.editor->setHistoryArrowNavigationEnabled(true);
            emit historyChanged();
            emit variablesChanged();
            emit functionsChanged();
            emit unitsChanged();
            restoreDisplayScroll();
            m_widgets.editor->clear();

            m_widgets.editor->stopAutoCalc();
            m_widgets.editor->stopAutoComplete();
            saveSessionToDefaultPath();
            return;
        }
    }

    const EvaluationContext evalContext = currentEvaluationContext(m_settings);
    m_evaluator->setExpression(expr);
    Quantity result = m_evaluator->evalUpdateAns();

    if (!m_evaluator->error().isEmpty()) {
        showStateLabel(m_evaluator->error());
        return;
    }

    if (m_evaluator->isUserFunctionAssign()) {
        result = CMath::nan();
        if (m_bulkEvaluationInProgress)
            m_bulkFunctionsChanged = true;
        else
            emit functionsChanged();
    } else if (m_evaluator->isUserUnitAssign()) {
        result = CMath::nan();
        if (m_bulkEvaluationInProgress)
            m_bulkUnitsChanged = true;
        else
            emit unitsChanged();
    } else if (result.isNan() && !isCommentOnly)
        return;

    const QString interpretedExpr = m_evaluator->interpretedExpression();
    const bool warnHistoryLimitReached = m_session->nextHistoryEntryReachesLimit();
    HistoryEntry historyEntry(enteredExpr, result, interpretedExpr, evalContext);
    historyEntry.setRenderedLines(renderedLinesForHistoryEntry(historyEntry, m_settings, m_evaluator));
    m_session->addHistoryEntry(historyEntry);
    const bool userVariableAssign = m_evaluator->isUserVariableAssign();
    if (m_bulkEvaluationInProgress)
        m_bulkHistoryChanged = true;
    else
        emit historyChanged();
    if (!startedFromHistoryEdit)
        m_widgets.display->verticalScrollBar()->setValue(m_widgets.display->verticalScrollBar()->maximum());
    if (userVariableAssign) {
        if (m_bulkEvaluationInProgress)
            m_bulkVariablesChanged = true;
        else
            emit variablesChanged();
    }
    if (m_evaluator->isUserUnitAssign()) {
        if (m_bulkEvaluationInProgress)
            m_bulkUnitsChanged = true;
        else
            emit unitsChanged();
    }

    if (m_widgets.bitField && m_docks.bitField && !m_docks.bitField->isHidden())
        m_widgets.bitField->updateBits(result);

    if (m_settings->autoResultToClipboard)
        copyResultToClipboard();

    if (m_settings->leaveLastExpression)
        m_widgets.editor->selectAll();
    else
        m_widgets.editor->clear();

    m_widgets.editor->stopAutoCalc();
    m_widgets.editor->stopAutoComplete();
    if (!result.isNan())
        m_conditions.autoAns = true;
    if (warnHistoryLimitReached) {
        QTimer::singleShot(0, this, [this]() {
            QMessageBox::information(
                this,
                tr("History Size Limit Reached"),
                tr("This calculation fills the last available history slot. "
                   "Future calculations will remove the oldest calculation from history. "
                   "You can increase the limit from Session > History Size Limit."));
        });
    }
    if (m_bulkEvaluationInProgress)
        m_sessionSavePending = true;
    else
        saveSessionToDefaultPath();
}

void MainWindow::startHistoryEntryEdit(int index)
{
    const int historySize = m_session->historySize();
    if (index < 0 || index >= historySize)
        return;

    m_pendingHistoryEditIndex = index;
    m_widgets.display->setEditingHistoryIndex(index);
    m_widgets.editor->setHistoryArrowNavigationEnabled(false);
    m_widgets.editor->setText(Editor::historyExpressionForInput(m_session->historyEntryAt(index).expr()));
    m_widgets.editor->setFocus();
    m_widgets.editor->setCursorPosition(m_widgets.editor->text().size());
    showStateLabel(tr("Editing calculation. Press Esc twice to cancel."));
}

void MainWindow::handleBulkEvaluationStarted()
{
    m_bulkEvaluationInProgress = true;
    m_bulkHistoryChanged = false;
    m_bulkVariablesChanged = false;
    m_bulkFunctionsChanged = false;
    m_bulkUnitsChanged = false;
}

void MainWindow::handleBulkEvaluationFinished()
{
    m_bulkEvaluationInProgress = false;

    if (m_bulkHistoryChanged)
        emit historyChanged();
    if (m_bulkVariablesChanged)
        emit variablesChanged();
    if (m_bulkFunctionsChanged)
        emit functionsChanged();
    if (m_bulkUnitsChanged)
        emit unitsChanged();

    if (m_bulkHistoryChanged && m_widgets.display)
        m_widgets.display->verticalScrollBar()->setValue(m_widgets.display->verticalScrollBar()->maximum());

    if (m_bulkHistoryChanged || m_bulkVariablesChanged || m_bulkFunctionsChanged || m_bulkUnitsChanged || m_sessionSavePending)
        saveSessionToDefaultPath();
}

void MainWindow::editHistoryEntryContext(int index)
{
    const int historySize = m_session->historySize();
    if (index < 0 || index >= historySize)
        return;

    HistoryEntry entry = m_session->historyEntryAt(index);
    EvaluationContext ctx = entry.context();

    ResultSlotsDialog dialog(tr("Calculation Settings"), ctx, this);

    if (dialog.exec() != QDialog::Accepted)
        return;

    ctx = dialog.evaluationContext(ctx);

    const int previousDisplayScrollValue = m_widgets.display->verticalScrollBar()->value();
    const QList<HistoryEntry> previousEntries = historyEntries();
    QList<HistoryEntry> updatedEntries = previousEntries;
    HistoryEntry updatedEntry = updatedEntries.at(index);
    updatedEntry.setContext(ctx);
    updatedEntries[index] = updatedEntry;

    int errorIndex = -1;
    QString errorText;
    if (!rebuildSessionFromEntries(updatedEntries, index, &errorIndex, &errorText)) {
        QScrollBar* bar = m_widgets.display->verticalScrollBar();
        bar->setValue(qBound(bar->minimum(), previousDisplayScrollValue, bar->maximum()));
        showStateLabel(tr("Could not recalculate from calculation %1: %2").arg(errorIndex + 1).arg(errorText));
        return;
    }

    emit historyChanged();
    emit variablesChanged();
    emit functionsChanged();
    emit unitsChanged();
    QScrollBar* bar = m_widgets.display->verticalScrollBar();
    const int clamped = qBound(bar->minimum(), previousDisplayScrollValue, bar->maximum());
    bar->setValue(clamped);
    QTimer::singleShot(0, this, [this, previousDisplayScrollValue]() {
        QScrollBar* deferredBar = m_widgets.display->verticalScrollBar();
        const int deferredClamped = qBound(deferredBar->minimum(),
                                           previousDisplayScrollValue,
                                           deferredBar->maximum());
        deferredBar->setValue(deferredClamped);
    });
    saveSessionToDefaultPath();
}

void MainWindow::cancelHistoryEntryEdit()
{
    if (m_pendingHistoryEditIndex < 0)
        return;

    const int previousDisplayScrollValue = m_widgets.display->verticalScrollBar()->value();
    m_pendingHistoryEditIndex = -1;
    m_widgets.display->setEditingHistoryIndex(-1);
    m_widgets.editor->setHistoryArrowNavigationEnabled(true);
    m_widgets.display->verticalScrollBar()->setValue(previousDisplayScrollValue);
    m_widgets.editor->clear();
    hideStateLabel();
}

QList<HistoryEntry> MainWindow::historyEntries() const
{
    QList<HistoryEntry> entries;
    const int historySize = m_session->historySize();
    entries.reserve(historySize);
    for (int i = 0; i < historySize; ++i)
        entries.append(m_session->historyEntryAtRef(i));
    return entries;
}

bool MainWindow::rebuildSessionFromEntries(const QList<HistoryEntry>& entries,
                                           int startIndex,
                                           int* errorIndex,
                                           QString* errorText)
{
    if (errorIndex)
        *errorIndex = -1;
    if (errorText)
        *errorText = QString();

    if (startIndex < 0 || startIndex > entries.size()) {
        if (errorText)
            *errorText = tr("Invalid recalculation start index");
        return false;
    }

    const Session previousSessionState = *m_session;
    const bool previousAutoAns = m_conditions.autoAns;

    if (startIndex == 0) {
        bool hasBaselineAns = false;
        Quantity baselineAnsValue = CMath::nan();
        const bool hadPreviousAns = m_evaluator->hasVariable(QStringLiteral("ans"));
        if (hadPreviousAns) {
            const Variable previousAnsVariable = m_evaluator->getVariable(QStringLiteral("ans"));
            baselineAnsValue = previousAnsVariable.value();
            hasBaselineAns = !baselineAnsValue.isNan();
        }
        if (!hasBaselineAns) {
            for (int i = previousSessionState.historySize() - 1; i >= 0; --i) {
                const Quantity candidate = previousSessionState.historyEntryAtRef(i).result();
                if (!candidate.isNan()) {
                    baselineAnsValue = candidate;
                    hasBaselineAns = true;
                    break;
                }
            }
        }
        m_session->clearHistory();
        m_session->clearVariables();
        m_session->clearUserFunctions();
        m_evaluator->initializeBuiltInVariables();
        if (hasBaselineAns) {
            m_evaluator->setVariable(
                QStringLiteral("ans"),
                baselineAnsValue,
                Variable::BuiltIn);
        }
        m_conditions.autoAns = false;
    } else {
        while (m_session->historySize() > startIndex)
            m_session->removeHistoryEntryAt(m_session->historySize() - 1);
    }

    const EvaluationContext originalContext = currentEvaluationContext(m_settings);

    for (int i = startIndex; i < entries.size(); ++i) {
        const HistoryEntry entry = entries.at(i);
        const QString currentExpr = entry.expr();
        applyEvaluationContext(m_settings, entry.contextRef());
        const QString evalExpr = m_evaluator->autoFix(currentExpr);
        const bool isCommentOnly = Evaluator::isCommentOnlyExpression(evalExpr);

        m_evaluator->setExpression(evalExpr);
        Quantity result = m_evaluator->evalUpdateAns();
        if (!m_evaluator->error().isEmpty()) {
            if (errorIndex)
                *errorIndex = i;
            if (errorText)
                *errorText = m_evaluator->error();
            applyEvaluationContext(m_settings, originalContext);
            *m_session = previousSessionState;
            m_conditions.autoAns = previousAutoAns;
            return false;
        }

        if (m_evaluator->isUserFunctionAssign())
            result = CMath::nan();
        else if (result.isNan() && !isCommentOnly)
            continue;

        const QString interpretedExpr = m_evaluator->interpretedExpression();
        HistoryEntry rebuiltEntry(currentExpr, result, interpretedExpr, entry.contextRef());
        rebuiltEntry.setEditTimestamp(entry.editTimestamp());
        rebuiltEntry.setRenderedLines(renderedLinesForHistoryEntry(rebuiltEntry, m_settings, m_evaluator));
        m_session->addHistoryEntry(rebuiltEntry);
    }

    applyEvaluationContext(m_settings, originalContext);
    const bool hasAns = m_evaluator->hasVariable(QStringLiteral("ans"));
    m_conditions.autoAns = hasAns && !m_evaluator->getVariable(QStringLiteral("ans")).value().isNan();
    return true;
}

void MainWindow::removeHistoryEntryAt(int index)
{
    const int historySize = m_session->historySize();
    if (index < 0 || index >= historySize)
        return;

    m_session->removeHistoryEntryAt(index);
    if (m_pendingHistoryEditIndex == index)
        m_pendingHistoryEditIndex = -1;
    else if (m_pendingHistoryEditIndex > index)
        --m_pendingHistoryEditIndex;
    m_widgets.display->setEditingHistoryIndex(m_pendingHistoryEditIndex);
    m_widgets.editor->setHistoryArrowNavigationEnabled(m_pendingHistoryEditIndex < 0);
    m_conditions.autoAns = !m_session->historyIsEmpty();
    emit historyChanged();
    saveSessionToDefaultPath();
}

void MainWindow::removeHistoryEntriesAbove(int index)
{
    const int historySize = m_session->historySize();
    if (historySize == 0 || index <= 0 || index >= historySize)
        return;

    for (int i = 0; i < index; ++i)
        m_session->removeHistoryEntryAt(0);

    if (m_pendingHistoryEditIndex >= 0) {
        if (m_pendingHistoryEditIndex < index)
            m_pendingHistoryEditIndex = -1;
        else
            m_pendingHistoryEditIndex -= index;
    }
    m_widgets.display->setEditingHistoryIndex(m_pendingHistoryEditIndex);
    m_widgets.editor->setHistoryArrowNavigationEnabled(m_pendingHistoryEditIndex < 0);
    m_conditions.autoAns = !m_session->historyIsEmpty();
    emit historyChanged();
    saveSessionToDefaultPath();
}

void MainWindow::removeHistoryEntriesBelow(int index)
{
    const int historySize = m_session->historySize();
    if (historySize == 0 || index < 0 || index >= historySize - 1)
        return;

    for (int i = historySize - 1; i > index; --i)
        m_session->removeHistoryEntryAt(i);

    if (m_pendingHistoryEditIndex > index)
        m_pendingHistoryEditIndex = -1;
    m_widgets.display->setEditingHistoryIndex(m_pendingHistoryEditIndex);
    m_widgets.editor->setHistoryArrowNavigationEnabled(m_pendingHistoryEditIndex < 0);
    m_conditions.autoAns = !m_session->historyIsEmpty();
    emit historyChanged();
    saveSessionToDefaultPath();
}

void MainWindow::clearTextEditSelection(QPlainTextEdit* edit)
{
    QTextCursor cursor = edit->textCursor();
    if (cursor.hasSelection()) {
        cursor.clearSelection();
        edit->setTextCursor(cursor);
    }
}

void MainWindow::handleManualClosed()
{
    disconnect(m_widgets.manual);
    m_settings->manualWindowGeometry = m_settings->windowPositionSave ? m_widgets.manual->saveGeometry() : QByteArray();
    m_widgets.manual->deleteLater();
    m_widgets.manual = 0;
}

void MainWindow::handleDisplaySelectionChange()
{
    const QTextCursor displayCursor = m_widgets.display->textCursor();
    if (displayCursor.hasSelection()) {
        clearTextEditSelection(m_widgets.editor);
        const QString rawSelected = displayCursor.selectedText();
        if (rawSelected.contains(RegExpPatterns::lineBreak())) {
            m_widgets.editor->autoCalcSelection(rawSelected);
            return;
        }

        const QString selected = normalizedDisplaySelectionForEvaluation(rawSelected);
        m_widgets.editor->autoCalcSelection(selected);
        return;
    }

    hideStateLabel();
}

void MainWindow::handleEditorSelectionChange()
{
    if (m_widgets.editor->textCursor().hasSelection()) {
        clearTextEditSelection(m_widgets.display);
        return;
    }

    if (m_widgets.editor->text().trimmed().isEmpty())
        hideStateLabel();
}

void MainWindow::handleCopyAvailable(bool copyAvailable)
{
    QPlainTextEdit* const textEdit = static_cast<QPlainTextEdit*>(sender());
    if (copyAvailable && textEdit)
        m_copyWidget = textEdit;
    syncEditMenuActionState();
}

void MainWindow::handleBitsChanged(const QString& str)
{
    Quantity num(CNumber(str.toLatin1().data()));
    auto result = DMath::format(num, Quantity::Format::Fixed() + Quantity::Format::Hexadecimal());
    insertTextIntoEditor(result);
    showStateLabel(QString("Current value: %1").arg(NumberFormatter::format(num)));

    auto cursor = m_widgets.editor->textCursor();
    if (cursor.hasSelection())
        cursor.removeSelectedText();
    cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, result.length());
    cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, result.length());
    m_widgets.editor->setTextCursor(cursor);
}

void MainWindow::handleEditorTextChange()
{
    m_currentResultPreviewDismissed = false;
    captureEditorTextInCurrentSession();
    m_widgets.display->clearHoverFeedback();
    clearTextEditSelection(m_widgets.display);
    if (m_widgets.editor->text().trimmed().isEmpty()) {
        hideStateLabel();
        if (m_widgets.bitField)
            m_widgets.bitField->clear();
        return;
    }

    if (m_conditions.autoAns && m_settings->autoAns) {
        QString expr = m_evaluator->autoFix(m_widgets.editor->text());
        if (expr.isEmpty())
            return;

        Tokens tokens = m_evaluator->scan(expr);
        if (tokens.count() == 1) {
            const auto mode = EditorUtils::autoAnsRewriteModeForLeadingOperator(tokens.at(0).text());
            if (mode != EditorUtils::AutoAnsNoRewrite) {
                m_conditions.autoAns = false;
                expr = EditorUtils::applyAutoAnsRewrite(expr, mode);
                m_widgets.editor->setText(expr);
                m_widgets.editor->setCursorPosition(expr.length());
            }
        }
    }
}

void MainWindow::handleDockWidgetVisibilityChanged(bool visible)
{
    QDockWidget* dock = qobject_cast<QDockWidget*>(sender());
    if (!dock)
        return;

    // Pass the focus back to the editor if the dock that is being hidden has the focus.
    QWidget* focusWidget = dock->focusWidget();
    if (focusWidget && !visible && focusWidget->hasFocus())
        m_widgets.editor->setFocus();

    if (activeMainWindowForMenuAction(nullptr) == this)
        syncViewMenuActionState();

    if (m_dockVisibilityRefreshPending)
        return;

    m_dockVisibilityRefreshPending = true;
    QTimer::singleShot(0, this, [this]() {
        // Changes during the refresh can schedule another pass.
        m_dockVisibilityRefreshPending = false;
        StartupDiagnostics::AppearanceContext context("Deferred dock visibility refresh");
        StartupDiagnostics::AppearanceScope timing("Refresh deferred dock visibility appearance", this);
        updateSplitterStyleSheet();
        applyDockTabBarAppearance();
    });
}

void MainWindow::insertVariableIntoEditor(const QString& v)
{
    insertTextIntoEditor(v);
}

void MainWindow::insertUserFunctionIntoEditor(const QString& v)
{
    insertTextIntoEditor(v);
}

void MainWindow::insertUserUnitIntoEditor(const QString& v)
{
    insertTextIntoEditor(v);
}

void MainWindow::setRadixCharacterAutomatic()
{
    setRadixCharacter(0);
}

void MainWindow::setRadixCharacterDot()
{
    setRadixCharacter(MathDsl::DotSep.toLatin1());
}

void MainWindow::setRadixCharacterComma()
{
    setRadixCharacter(MathDsl::CommaSep.toLatin1());
}

void MainWindow::setRadixCharacterBoth()
{
    setRadixCharacter(MathDsl::MulOpAl1.toLatin1());
}

void MainWindow::closeEvent(QCloseEvent* e)
{
    if (primaryMainWindow() == this) {
        const bool shutdownAlreadyInProgress = applicationShutdownInProgress();
        if (!shutdownAlreadyInProgress) {
            // Mark shutdown before saving/restoring layouts so child close events
            // caused by QApplication::quit() do not rewrite the multi-window layout
            // as if each child had been closed manually.
            qApp->setProperty("speedcrunchShutdownInProgress", true);
            appShutdownInProgress() = true;
        }
        persistSessionAndSettingsForShutdown();
        if (!shutdownAlreadyInProgress)
            qApp->quit();
    } else {
        if (!applicationShutdownInProgress()) {
            allMainWindows().removeAll(QPointer<MainWindow>(this));
            updatePaneEditorCursorVisibility();
            saveSessionLayout(false);
        }
    }
    e->accept();
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    updateStatusBarSectionVisibility();

    if (m_widgets.state->isVisible())
        showStateLabel(m_widgets.state->text());
}

void MainWindow::persistSessionAndSettingsForShutdown()
{
    if (m_shutdownStateSaved)
        return;

    m_shutdownStateSaved = true;
    if (m_sessionSavePending)
        flushPendingSessionSave();

    if (m_widgets.manual)
        m_settings->manualWindowGeometry = m_settings->windowPositionSave ? m_widgets.manual->saveGeometry() : QByteArray();
    ensureSessionsPath();
    QSet<Session*> savedSessions;
    // Shutdown has to persist every restored window, not just the primary one.
    // Saving by Session* avoids duplicate writes when a session is referenced
    // by more than one pane or window during layout transitions.
    for (const QPointer<MainWindow>& ptr : allMainWindows()) {
        MainWindow* window = ptr.data();
        if (window == nullptr)
            continue;

        window->captureEditorTextInCurrentSession();
        for (auto it = window->m_loadedSessions.constBegin(); it != window->m_loadedSessions.constEnd(); ++it) {
            Session* session = it.value();
            if (session == nullptr || savedSessions.contains(session))
                continue;
            savedSessions.insert(session);

            saveSessionAsync(*session, sessionFilePath(it.key()));
        }
    }
    waitForAsyncSessionIo();
    saveSessionLayout();
    saveSettings();
}

void MainWindow::saveSessionToDefaultPath()
{
    if (m_session == nullptr || m_session->isPrivate())
        return;
    m_sessionSavePending = true;
    m_deferredSessionSaveTimer->start();
}

void MainWindow::flushPendingSessionSave()
{
    if (!m_sessionSavePending)
        return;

    m_sessionSavePending = false;
    if (m_session == nullptr || m_session->isPrivate())
        return;
    ensureSessionsPath();
    captureEditorTextInCurrentSession();

    const QString sessionName = m_session->name().isEmpty()
        ? QLatin1String(SessionJsonKeys::SessionValueMain)
        : m_session->name();
    QString dataPath = sessionFilePath(sessionName);
    saveSessionAsync(*m_session, dataPath);
}

void MainWindow::setResultPrecision(int p)
{
    if (m_settings->resultPrecision == p && m_status.selectedResultPrecision == p)
        return;

    m_settings->resultPrecision = p;
    m_status.selectedResultPrecision = p;
    setStatusBarText();
    syncStatusBarSelectionMenuActionState();
    emit resultPrecisionChanged();
}

void MainWindow::setResultFormat(char c)
{
    if (m_settings->resultFormat == c && m_status.selectedResultFormat == c)
        return;

    m_settings->resultFormat = c;
    m_status.selectedResultFormat = c;
    setStatusBarText();
    syncStatusBarSelectionMenuActionState();
    emit resultFormatChanged();
}

void MainWindow::setUnitNegativeExponentStyle(QAction* action)
{
    const Settings::UnitNegativeExponentStyle style =
        static_cast<Settings::UnitNegativeExponentStyle>(action->data().toInt());
    if (style != Settings::UnitNegativeExponentSuperscript
            && style != Settings::UnitNegativeExponentFraction) {
        return;
    }
    if (m_settings->unitNegativeExponentStyle == style)
        return;

    m_settings->unitNegativeExponentStyle = style;
    setRuntimeUnitNegativeExponentStyle(style);
    // Unit exponent style changes should affect only future evaluations and
    // live editor previews, not previously displayed history entries.
    m_widgets.editor->refreshAutoCalc();
}

void MainWindow::setRemoveTrailingZeroesEnabled(bool enabled)
{
    m_settings->removeTrailingZeroes = enabled;
    QList<QPointer<MainWindow>> windows = allMainWindows();
    if (!windows.contains(QPointer<MainWindow>(this)))
        windows.append(QPointer<MainWindow>(this));
    for (const QPointer<MainWindow>& window : windows) {
        if (!window)
            continue;
        const QSignalBlocker blocker(window->m_actions.settingsRemoveTrailingZeroes);
        window->m_actions.settingsRemoveTrailingZeroes->setChecked(enabled);
        for (Editor* editor : window->findChildren<Editor*>())
            editor->refreshAutoCalc();
    }
}

void MainWindow::setResultRoundingMode(QAction* action)
{
    const Settings::ResultRoundingMode mode =
        static_cast<Settings::ResultRoundingMode>(action->data().toInt());
    if (mode != Settings::ResultRoundingHalfAwayFromZero
            && mode != Settings::ResultRoundingHalfEven
            && mode != Settings::ResultRoundingTowardZero
            && mode != Settings::ResultRoundingTowardPositiveInfinity
            && mode != Settings::ResultRoundingTowardNegativeInfinity) {
        return;
    }
    if (m_settings->resultRoundingMode == mode)
        return;

    m_settings->resultRoundingMode = mode;
    setRuntimeResultRoundingMode(mode);
    emit resultRoundingModeChanged();
}

void MainWindow::setRadixCharacter(char c)
{
    m_settings->setRadixCharacter(c);
    emit radixCharacterChanged();
}

void MainWindow::showNumberFormatDialog()
{
    NumberFormatDialog dialog(this);
    dialog.setSelection(m_settings->numberFormatStyle);

    if (dialog.exec() != QDialog::Accepted)
        return;

    const Settings::NumberFormatStyle selectedStyle = dialog.selectedStyle();
    if (m_settings->numberFormatStyle == selectedStyle)
        return;

    m_settings->numberFormatStyle = selectedStyle;
    m_settings->applyNumberFormatStyle();
    // Number format changes from this dialog should not rewrite previous
    // history entries in Result Display. Only refresh live editor previews.
    for (const QPointer<MainWindow>& window : allMainWindows()) {
        if (!window)
            continue;
        emit window->syntaxHighlightingChanged();
        for (Editor* editor : window->findChildren<Editor*>())
            editor->refreshAutoCalc();
    }
}

void MainWindow::showResultSlotsDialog()
{
    ResultSlotsDialog dialog(this);
    connect(&dialog, &ResultSlotsDialog::settingsApplied, this, [this]() {
        DMath::complexMode = m_settings->complexNumbers;
        if (m_settings->complexNumbers)
            m_evaluator->initializeBuiltInVariables();
        m_status.selectedResultFormat = m_settings->resultFormat;
        m_status.selectedResultPrecision = m_settings->resultPrecision;
        setStatusBarText();
        syncStatusBarSelectionMenuActionState();
        for (const QPointer<MainWindow>& ptr : allMainWindows()) {
            MainWindow* window = ptr.data();
            if (window == nullptr)
                continue;
            for (Editor* editor : window->splitPaneEditors())
                editor->refreshAutoCalc();
        }
    });
    dialog.exec();
}

void MainWindow::increaseDisplayFontPointSize()
{
    if (m_widgets.display == nullptr)
        return;
    m_widgets.display->increaseFontPointSize();
    setDisplayFont(m_widgets.display->font());
}

void MainWindow::decreaseDisplayFontPointSize()
{
    if (m_widgets.display == nullptr)
        return;
    m_widgets.display->decreaseFontPointSize();
    setDisplayFont(m_widgets.display->font());
}

void MainWindow::showLanguageChooserDialog()
{
    QMap<QString, QString> map;

    // List all available translations from the resource files
    QDir localeDir(":/locale/", "*.qm");
    QFileInfoList localeList = localeDir.entryInfoList();
    for (int i = 0; i < localeList.size(); ++i) {
        QFileInfo fileInfo = localeList.at(i);
        QString localeName = fileInfo.baseName();
        QString langName = QLocale(localeName).nativeLanguageName();

        // Kludges for region-specific translations
        if(localeName == "es") langName = QString::fromUtf8("Español (Latinoamérica)");
        if(localeName == "es_ES") langName = QString::fromUtf8("Español (España)");
        if(localeName == "pt_BR") langName = QString::fromUtf8("Português (Brasil)");
        if(localeName == "pt_PT") langName = QString::fromUtf8("Português (Portugal)");

        // The first letter is not always capitalized so force it
        langName[0] = langName[0].toUpper();
        map.insert(langName, localeName);
    }

    const auto values = map.values();
    int current = values.indexOf(m_settings->language) + 1;

    QString defaultKey = tr("System Default");
    QStringList keys(QStringList() << defaultKey << map.keys());

    bool ok;
    QString langName = QInputDialog::getItem(this, tr("Language"), tr("Select the language:"),
        keys, current, false, &ok);
    if (ok && !langName.isEmpty()) {
        QString value = (langName == defaultKey) ? QLatin1String("C") : map.value(langName);
        if (m_settings->language != value) {
            m_settings->language = value;
            emit languageChanged();
        }
    }
}

void MainWindow::showResultFormatContextMenu(const QPoint& point)
{
    m_menus.resultFormat->popup(m_status.resultFormat->mapToGlobal(point));
}

void MainWindow::showPrecisionContextMenu(const QPoint& point)
{
    QMenu menu(this);
    QActionGroup modeGroup(&menu);
    modeGroup.setExclusive(true);

    QAction* automaticAction = menu.addAction(MainWindow::tr("Automatic"));
    automaticAction->setCheckable(true);
    automaticAction->setChecked(m_settings->resultPrecision < 0);
    modeGroup.addAction(automaticAction);
    connect(automaticAction, &QAction::triggered, this, [this]() {
        setResultPrecision(-1);
    });

    QAction* customAction = menu.addAction(MainWindow::tr("Custom"));
    customAction->setCheckable(true);
    customAction->setChecked(m_settings->resultPrecision >= 0);
    modeGroup.addAction(customAction);

    menu.addSeparator();

    QWidget* precisionEditor = new QWidget(&menu);
    QHBoxLayout* precisionLayout = new QHBoxLayout(precisionEditor);
    precisionLayout->setContentsMargins(8, 4, 8, 4);
    precisionLayout->setSpacing(6);

    QLabel* precisionLabel = new QLabel(MainWindow::tr("Decimal places:"), precisionEditor);
    MenuPrecisionSpinBox* precisionSpin = new MenuPrecisionSpinBox(precisionEditor);
    precisionSpin->setRange(0, 50);
    precisionSpin->setValue(m_settings->resultPrecision < 0 ? 8 : m_settings->resultPrecision);
    precisionSpin->setEnabled(m_settings->resultPrecision >= 0);

    connect(precisionSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
        setResultPrecision(value);
    });
    connect(precisionSpin, QOverload<int>::of(&QSpinBox::valueChanged), customAction, [customAction](int) {
        customAction->setChecked(true);
    });
    connect(customAction, &QAction::triggered, this, [this, precisionSpin]() {
        setResultPrecision(precisionSpin->value());
    });
    connect(automaticAction, &QAction::toggled, precisionSpin, [precisionSpin](bool automatic) {
        precisionSpin->setEnabled(!automatic);
    });

    precisionLayout->addWidget(precisionLabel);
    precisionLayout->addWidget(precisionSpin);

    QWidgetAction* editorAction = new QWidgetAction(&menu);
    editorAction->setDefaultWidget(precisionEditor);
    menu.addAction(editorAction);

    const auto updateAppearance = [this, &menu, precisionEditor, precisionLabel, precisionSpin]() {
        MenuStyle::apply(&menu);
        precisionEditor->setPalette(menu.palette());
        precisionLabel->setPalette(menu.palette());
        if (m_settings->menuAppearance == Settings::MenuAppearanceSpeedCrunch)
            precisionSpin->setThemeSurface(generatedSurfaceColors(m_settings).inputs);
        else
            precisionSpin->setSystemAppearance();
    };
    connect(this, &MainWindow::menuAppearanceChanged, &menu, updateAppearance);
    connect(this, &MainWindow::colorSchemeChanged, &menu, updateAppearance);
    updateAppearance();
    menu.exec(m_status.resultPrecision->mapToGlobal(point));
}

void MainWindow::showKeypadContextMenu(const QPoint& point)
{
    if (!m_widgets.keypad)
        return;
    QMenu menu(this);
    menu.addActions(m_menus.keypad->actions());
    menu.addSeparator();
    menu.addMenu(m_menus.keypadZoom);
    MenuStyle::apply(&menu);
    menu.exec(m_widgets.keypad->mapToGlobal(point));
}
