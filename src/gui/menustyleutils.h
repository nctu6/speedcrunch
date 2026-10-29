// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef SPEEDCRUNCH_MENUSTYLEUTILS_H
#define SPEEDCRUNCH_MENUSTYLEUTILS_H

#include <QColor>

class QMenu;
class QIcon;
class QComboBox;
class QPalette;
class QWidget;

namespace MenuStyle {
void install();
void refresh();
void apply(QWidget* widget);
QIcon themedIcon(const QIcon& source, const QPalette& palette);
QPalette systemPalette(const QWidget* widget);
QPalette systemComboPopupPalette(const QComboBox* combo);
void setThemeColors(QMenu* menu, const QColor& background, const QColor& foreground,
                    const QColor& selectedBackground, const QColor& selectedForeground);
}

#endif
