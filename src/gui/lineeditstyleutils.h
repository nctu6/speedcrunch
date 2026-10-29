// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef GUI_LINEEDITSTYLEUTILS_H
#define GUI_LINEEDITSTYLEUTILS_H

#include <QAction>
#include <QIcon>
#include <QLineEdit>
#include <QPainter>
#include <QToolButton>

namespace LineEditStyleUtils {

inline void applyClearButtonColors(QLineEdit* searchBox, const QColor& fill, const QColor& cross)
{
    if (searchBox == nullptr)
        return;

    const auto pixmapForScale = [&fill, &cross](qreal scale) {
        constexpr int logicalSize = 16;
        QPixmap pixmap(qRound(logicalSize * scale), qRound(logicalSize * scale));
        pixmap.setDevicePixelRatio(scale);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Qt::NoPen);
        painter.setBrush(fill);
        painter.drawEllipse(QRectF(1.5, 1.5, 13.0, 13.0));

        painter.setPen(QPen(cross, 1.45, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(QPointF(5.25, 5.25), QPointF(10.75, 10.75));
        painter.drawLine(QPointF(10.75, 5.25), QPointF(5.25, 10.75));
        return pixmap;
    };

    QIcon icon;
    icon.addPixmap(pixmapForScale(1.0));
    icon.addPixmap(pixmapForScale(2.0));
    icon.addPixmap(pixmapForScale(3.0));
    for (QAction* action : searchBox->actions())
        action->setIcon(icon);
    for (QToolButton* button : searchBox->findChildren<QToolButton*>()) {
        if (QAction* action = button->defaultAction())
            action->setIcon(icon);
        button->setIcon(icon);
        button->setIconSize(QSize(16, 16));
    }
}

} // namespace LineEditStyleUtils

#endif
