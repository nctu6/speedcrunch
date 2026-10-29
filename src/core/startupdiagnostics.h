// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CORE_STARTUPDIAGNOSTICS_H
#define CORE_STARTUPDIAGNOSTICS_H

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QPointer>
#include <QString>
#include <QThread>
#include <QVector>
#include <atomic>
#include <memory>
#include <type_traits>

// Only startup is measured. Fixed phase names keep the report free of user data.
// Repeated phases are aggregated so restoring many sessions uses bounded memory.
namespace StartupDiagnostics {

struct Phase {
    QString name;
    bool worker = false;
    qint64 firstStartNs = 0;
    qint64 totalNs = 0;
    qint64 longestNs = 0;
    int calls = 0;
    int running = 0;
};

struct AppearanceCall {
    QString name;
    QString context;
    int widgetId = 0;
    int parentCall = 0;
    qint64 startNs = 0;
    qint64 durationNs = 0;
    // -1 means no comparison, 0 means first call, 1 changed, 2 unchanged.
    int inputs = -1;
    QString inputChanges;
    bool finished = false;
};

struct AppearanceAssignment {
    int call = 0;
    int widgetId = 0;
    bool palette = false;
    int changed = 0;
    int unchanged = 0;
    int skipped = 0;
};

struct Snapshot {
    QVector<Phase> phases;
    QVector<AppearanceCall> appearanceCalls;
    QVector<AppearanceAssignment> appearanceAssignments;
    int omittedAppearanceRecords = 0;
    qint64 elapsedNs = 0;
    qint64 firstPaintNs = -1;
    bool started = false;
    bool complete = false;
};

#ifdef SPEEDCRUNCH_ENABLE_STARTUP_DIAGNOSTICS

inline constexpr bool isEnabled() { return true; }

namespace Detail {
struct State {
    QMutex mutex;
    QElapsedTimer clock;
    QVector<Phase> phases;
    QVector<AppearanceCall> appearanceCalls;
    QVector<AppearanceAssignment> appearanceAssignments;
    QVector<QPointer<const QObject>> widgets;
    QHash<const QObject*, int> widgetIds;
    QHash<QString, QByteArray> previousInputs;
    QHash<quint64, int> assignmentIndices;
    int omittedAppearanceRecords = 0;
    std::atomic_bool recording{false};
    quint64 generation = 0;
    int openScopes = 0;
    qint64 firstPaintNs = -1;
    qint64 finishedNs = 0;

};

inline State& state()
{
    static State value;
    return value;
}

inline thread_local QString appearanceContext;
inline thread_local int appearanceCall = -1;

inline int widgetId(State& state, const QObject* widget)
{
    if (!widget)
        return 0;
    const int previous = state.widgetIds.value(widget, 0);
    // A deleted widget's address can be reused during pane restoration.
    if (previous > 0 && state.widgets.at(previous - 1) == widget)
        return previous;
    if (state.widgets.size() >= 2048)
        return -1;
    state.widgets.append(QPointer<const QObject>(widget));
    const int id = state.widgets.size();
    state.widgetIds.insert(widget, id);
    return id;
}
}

inline void begin()
{
    auto& state = Detail::state();
    QMutexLocker lock(&state.mutex);
    ++state.generation;
    state.phases.clear();
    state.appearanceCalls.clear();
    state.appearanceAssignments.clear();
    state.widgets.clear();
    state.widgetIds.clear();
    state.previousInputs.clear();
    state.assignmentIndices.clear();
    state.omittedAppearanceRecords = 0;
    Detail::appearanceContext.clear();
    Detail::appearanceCall = -1;
    state.openScopes = 0;
    state.firstPaintNs = -1;
    state.finishedNs = 0;
    state.clock.start();
    state.recording.store(true);
}

inline bool isRecording() { return Detail::state().recording.load(); }

inline int appearanceObjectId(const QObject* object)
{
    if (!isRecording())
        return -1;
    auto& state = Detail::state();
    QMutexLocker lock(&state.mutex);
    return Detail::widgetId(state, object);
}

class Scope {
public:
    explicit Scope(const char* name) { start(name); }
    ~Scope() { finish(); }
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;

    void next(const char* name)
    {
        finish();
        start(name);
    }

    void finish()
    {
        if (m_index < 0)
            return;
        auto& state = Detail::state();
        QMutexLocker lock(&state.mutex);
        if (m_generation == state.generation) {
            const qint64 elapsed = state.clock.nsecsElapsed() - m_startNs;
            auto& phase = state.phases[m_index];
            phase.totalNs += elapsed;
            phase.longestNs = qMax(phase.longestNs, elapsed);
            ++phase.calls;
            --phase.running;
            --state.openScopes;
        }
        m_index = -1;
    }

private:
    void start(const char* name)
    {
        auto& state = Detail::state();
        if (!state.recording.load())
            return;
        QMutexLocker lock(&state.mutex);
        if (!state.recording.load())
            return;
        m_generation = state.generation;
        m_startNs = state.clock.nsecsElapsed();
        const bool worker = QCoreApplication::instance()
            && QThread::currentThread() != QCoreApplication::instance()->thread();
        const QString label = QString::fromLatin1(name);
        for (int i = 0; i < state.phases.size(); ++i) {
            if (state.phases[i].name == label && state.phases[i].worker == worker) {
                m_index = i;
                break;
            }
        }
        if (m_index < 0) {
            m_index = state.phases.size();
            state.phases.append(Phase{label, worker, m_startNs});
        }
        ++state.phases[m_index].running;
        ++state.openScopes;
    }

    quint64 m_generation = 0;
    qint64 m_startNs = 0;
    int m_index = -1;
};

// Context names are fixed at call sites. No user text goes into the report.
class AppearanceContext {
public:
    explicit AppearanceContext(const char* name)
        : m_previous(Detail::appearanceContext), m_active(isRecording())
    {
        if (m_active) {
            Detail::appearanceContext = m_previous.isEmpty()
                ? QString::fromLatin1(name)
                : m_previous + QStringLiteral(" / ") + QString::fromLatin1(name);
        }
    }
    ~AppearanceContext()
    {
        if (m_active)
            Detail::appearanceContext = m_previous;
    }
    AppearanceContext(const AppearanceContext&) = delete;
    AppearanceContext& operator=(const AppearanceContext&) = delete;
private:
    QString m_previous;
    bool m_active;
};

class AppearanceScope {
public:
    AppearanceScope(const char* name, QObject* widget = nullptr,
                    const QByteArray& inputs = {})
        : m_timing(name), m_previous(Detail::appearanceCall)
    {
        auto& state = Detail::state();
        if (!isRecording())
            return;
        QMutexLocker lock(&state.mutex);
        m_generation = state.generation;
        if (state.appearanceCalls.size() >= 512) {
            ++state.omittedAppearanceRecords;
            Detail::appearanceCall = -1;
            m_active = true;
            return;
        }
        const int id = Detail::widgetId(state, widget);
        AppearanceCall call;
        call.name = QString::fromLatin1(name);
        call.context = Detail::appearanceContext.isEmpty()
            ? QStringLiteral("Other startup callbacks") : Detail::appearanceContext;
        call.widgetId = id;
        call.parentCall = m_previous + 1;
        call.startNs = state.clock.nsecsElapsed();
        if (!inputs.isEmpty() && id >= 0) {
            const QString key = call.name + QString::number(id);
            const auto previous = state.previousInputs.constFind(key);
            call.inputs = previous == state.previousInputs.constEnd()
                ? 0 : (*previous == inputs ? 2 : 1);
            state.previousInputs.insert(key, inputs);
        }
        m_index = state.appearanceCalls.size();
        state.appearanceCalls.append(call);
        Detail::appearanceCall = m_index;
        m_active = true;
    }
    template <typename Factory,
              std::enable_if_t<std::is_invocable_r_v<QByteArray, Factory>, int> = 0>
    AppearanceScope(const char* name, QObject* widget, Factory&& inputs)
        : AppearanceScope(name, widget, isRecording() ? inputs() : QByteArray())
    {}
    ~AppearanceScope()
    {
        if (!m_active)
            return;
        Detail::appearanceCall = m_previous;
        auto& state = Detail::state();
        QMutexLocker lock(&state.mutex);
        if (m_index >= 0 && m_generation == state.generation) {
            auto& call = state.appearanceCalls[m_index];
            call.durationNs = state.clock.nsecsElapsed() - call.startNs;
            call.finished = true;
        }
    }
    AppearanceScope(const AppearanceScope&) = delete;
    AppearanceScope& operator=(const AppearanceScope&) = delete;
    void observeInputs(const char* category, const QByteArray& inputs)
    {
        if (m_index < 0 || inputs.isEmpty())
            return;
        auto& state = Detail::state();
        QMutexLocker lock(&state.mutex);
        if (m_generation != state.generation)
            return;
        auto& call = state.appearanceCalls[m_index];
        if (call.widgetId < 0)
            return;
        const QString label = QString::fromLatin1(category);
        const QString key = call.name + QString::number(call.widgetId) + label;
        const auto previous = state.previousInputs.constFind(key);
        const QString change = previous == state.previousInputs.constEnd()
            ? QStringLiteral("first")
            : (*previous == inputs ? QStringLiteral("unchanged") : QStringLiteral("changed"));
        state.previousInputs.insert(key, inputs);
        if (!call.inputChanges.isEmpty())
            call.inputChanges += QLatin1Char(' ');
        call.inputChanges += label + QLatin1Char('=') + change;
    }
    template <typename Factory,
              std::enable_if_t<std::is_invocable_r_v<QByteArray, Factory>, int> = 0>
    void observeInputs(const char* category, Factory&& inputs)
    {
        if (m_index >= 0)
            observeInputs(category, inputs());
    }
private:
    Scope m_timing;
    int m_previous;
    int m_index = -1;
    quint64 m_generation = 0;
    bool m_active = false;
};

inline void appearanceAssignment(QObject* widget, bool palette, bool changed, bool skipped)
{
    if (!isRecording() || Detail::appearanceCall < 0)
        return;
    auto& state = Detail::state();
    QMutexLocker lock(&state.mutex);
    const int id = Detail::widgetId(state, widget);
    const int call = Detail::appearanceCall + 1;
    if (id < 0) {
        ++state.omittedAppearanceRecords;
        return;
    }
    const quint64 key = (quint64(call) << 32) | (quint64(id) << 1) | quint64(palette);
    int index = state.assignmentIndices.value(key, -1);
    if (index < 0) {
        if (state.appearanceAssignments.size() >= 2048) {
            ++state.omittedAppearanceRecords;
            return;
        }
        index = state.appearanceAssignments.size();
        state.appearanceAssignments.append(AppearanceAssignment{call, id, palette});
        state.assignmentIndices.insert(key, index);
    }
    auto& assignment = state.appearanceAssignments[index];
    if (skipped)
        ++assignment.skipped;
    else if (changed)
        ++assignment.changed;
    else
        ++assignment.unchanged;
}

// A shared scope spans a worker and its queued UI callback, including queue wait.
inline std::shared_ptr<Scope> task(const char* name)
{
    return isRecording() ? std::make_shared<Scope>(name) : nullptr;
}

inline void firstPaintFinished()
{
    auto& state = Detail::state();
    QMutexLocker lock(&state.mutex);
    if (!state.recording.load() || state.firstPaintNs >= 0)
        return;
    state.firstPaintNs = state.clock.nsecsElapsed();
}

// The dispatcher reaches this point after handling paints and queued callbacks.
inline void finishWhenIdle()
{
    auto& state = Detail::state();
    QMutexLocker lock(&state.mutex);
    if (!state.recording.load() || state.firstPaintNs < 0 || state.openScopes != 0)
        return;
    state.finishedNs = state.clock.nsecsElapsed();
    state.recording.store(false);
}

inline Snapshot snapshot()
{
    auto& state = Detail::state();
    QMutexLocker lock(&state.mutex);
    Snapshot result;
    result.started = state.clock.isValid();
    if (!result.started)
        return result;
    result.phases = state.phases;
    result.appearanceCalls = state.appearanceCalls;
    result.appearanceAssignments = state.appearanceAssignments;
    result.omittedAppearanceRecords = state.omittedAppearanceRecords;
    result.complete = !state.recording.load();
    result.elapsedNs = result.complete ? state.finishedNs : state.clock.nsecsElapsed();
    result.firstPaintNs = state.firstPaintNs;
    return result;
}

#else

inline constexpr bool isEnabled() { return false; }
inline constexpr bool isRecording() { return false; }
inline void begin() {}
inline int appearanceObjectId(const QObject*) { return -1; }

class Scope {
public:
    explicit Scope(const char*) {}
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
    void next(const char*) {}
    void finish() {}
};

class AppearanceContext {
public:
    explicit AppearanceContext(const char*) {}
    AppearanceContext(const AppearanceContext&) = delete;
    AppearanceContext& operator=(const AppearanceContext&) = delete;
};

class AppearanceScope {
public:
    explicit AppearanceScope(const char*, QObject* = nullptr, const QByteArray& = {}) {}
    template <typename Factory,
              std::enable_if_t<std::is_invocable_r_v<QByteArray, Factory>, int> = 0>
    AppearanceScope(const char*, QObject*, Factory&&) {}
    AppearanceScope(const AppearanceScope&) = delete;
    AppearanceScope& operator=(const AppearanceScope&) = delete;
    void observeInputs(const char*, const QByteArray&) {}
    template <typename Factory,
              std::enable_if_t<std::is_invocable_r_v<QByteArray, Factory>, int> = 0>
    void observeInputs(const char*, Factory&&) {}
};

inline void appearanceAssignment(QObject*, bool, bool, bool) {}
inline std::shared_ptr<Scope> task(const char*) { return {}; }
inline void firstPaintFinished() {}
inline void finishWhenIdle() {}
inline Snapshot snapshot() { return {}; }

#endif

}

#endif
