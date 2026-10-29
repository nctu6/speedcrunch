// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#include "platformactivation.h"

#include <QGuiApplication>
#include <QScopeGuard>
#include <QWidget>
#include <QWindow>

#ifdef Q_OS_WIN
#include <windows.h>
#endif
#ifdef SPEEDCRUNCH_X11_ACTIVATION
#include <QtGui/qguiapplication_platform.h>
#include <xcb/xcb.h>
#include <cstdlib>

namespace {
xcb_atom_t atom(xcb_connection_t* connection, const char* name)
{
    const auto cookie = xcb_intern_atom(connection, false, quint16(qstrlen(name)), name);
    xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(connection, cookie, nullptr);
    const xcb_atom_t result = reply ? reply->atom : xcb_atom_t(XCB_ATOM_NONE);
    std::free(reply);
    return result;
}

void activateX11Window(QWidget* window, const QByteArray& startupId)
{
    if (startupId.isEmpty())
        return;
    auto* native = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
    if (!native)
        return;
    xcb_connection_t* connection = native->connection();
    const xcb_window_t id = xcb_window_t(window->winId());
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, id,
                        atom(connection, "_NET_STARTUP_ID"), atom(connection, "UTF8_STRING"),
                        8, quint32(startupId.size()), startupId.constData());

    // Startup IDs carry the timestamp of the launcher's user interaction.
    // The running process may otherwise still have an old X11 input timestamp.
    const int timeMarker = startupId.lastIndexOf("_TIME");
    bool validTime = false;
    const quint32 timestamp = timeMarker < 0 ? 0 : startupId.mid(timeMarker + 5).toUInt(&validTime);
    if (validTime) {
        xcb_change_property(connection, XCB_PROP_MODE_REPLACE, id,
                            atom(connection, "_NET_WM_USER_TIME"), XCB_ATOM_CARDINAL,
                            32, 1, &timestamp);
    }
    const auto cookie = xcb_query_tree(connection, id);
    xcb_query_tree_reply_t* tree = xcb_query_tree_reply(connection, cookie, nullptr);
    if (tree) {
        xcb_client_message_event_t event = {};
        event.response_type = XCB_CLIENT_MESSAGE;
        event.format = 32;
        event.window = id;
        event.type = atom(connection, "_NET_ACTIVE_WINDOW");
        event.data.data32[0] = 1;
        event.data.data32[1] = validTime ? timestamp : XCB_CURRENT_TIME;
        xcb_send_event(connection, false, tree->root,
                       XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY,
                       reinterpret_cast<const char*>(&event));
        std::free(tree);
    }
    xcb_flush(connection);
}
}
#endif

void activateLaunchWindow(QWidget* window, const LaunchRequest& request)
{
    if (!window)
        return;
    const bool wayland = QGuiApplication::platformName().startsWith(QLatin1String("wayland"));
    const QByteArray oldToken = qgetenv("XDG_ACTIVATION_TOKEN");
    const bool hadToken = qEnvironmentVariableIsSet("XDG_ACTIVATION_TOKEN");
    const auto restoreToken = qScopeGuard([wayland, oldToken, hadToken]() {
        if (!wayland)
            return;
        if (hadToken)
            qputenv("XDG_ACTIVATION_TOKEN", oldToken);
        else
            qunsetenv("XDG_ACTIVATION_TOKEN");
    });
    if (wayland && !request.activationToken.isEmpty())
        qputenv("XDG_ACTIVATION_TOKEN", request.activationToken);
    if (window->isMinimized())
        window->setWindowState(window->windowState() & ~Qt::WindowMinimized);
    window->show();
    window->raise();
#ifdef Q_OS_MACOS
    activateMacApplication();
#endif
    window->activateWindow();
#ifdef Q_OS_WIN
    SetForegroundWindow(reinterpret_cast<HWND>(window->winId()));
#endif
#ifdef SPEEDCRUNCH_X11_ACTIVATION
    if (QGuiApplication::platformName() == QLatin1String("xcb"))
        activateX11Window(window, request.startupId);
#endif
}
