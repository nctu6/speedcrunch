// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef GUI_DOCKWIDGET_H
#define GUI_DOCKWIDGET_H

#include <QAbstractButton>
#include <QDockWidget>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QStyleOptionDockWidget>
#include <QWindow>

class DockWidget : public QDockWidget {
public:
    using QDockWidget::QDockWidget;

protected:
    bool event(QEvent* event) override
    {
        if ((event->type() == QEvent::MouseButtonPress
             || event->type() == QEvent::MouseButtonDblClick) && isFloating()
            && titleBarWidget() == nullptr && usesWaylandWindowMoves()) {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            QStyleOptionDockWidget option;
            initStyleOption(&option);
            const QPoint pos = mouseEvent->position().toPoint();
            bool onButton = false;
            for (const auto* button : findChildren<QAbstractButton*>(
                     QString(), Qt::FindDirectChildrenOnly)) {
                if (button->isVisible() && button->geometry().contains(pos)) {
                    onButton = true;
                    break;
                }
            }
            if (mouseEvent->button() == Qt::LeftButton && option.rect.contains(pos) && !onButton) {
                if (event->type() == QEvent::MouseButtonDblClick) {
                    if (features().testFlag(DockWidgetFloatable))
                        setFloating(false);
                    mouseEvent->accept();
                    return true;
                }
                // Wayland owns window positions. Ask the compositor to move
                // the dock while this press still supplies a valid input serial.
                startWindowMove();
                // Even a rejected move must not enter Qt's dock drag path,
                // which tries an unsupported mouse grab and QWidget::move().
                mouseEvent->accept();
                return true;
            }
        }
        return QDockWidget::event(event);
    }

    virtual bool usesWaylandWindowMoves() const
    {
        return QGuiApplication::platformName().startsWith(QLatin1String("wayland"));
    }

    virtual void startWindowMove()
    {
        if (QWindow* handle = windowHandle())
            handle->startSystemMove();
    }
};

#endif // GUI_DOCKWIDGET_H
