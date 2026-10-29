// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later


#ifndef OKLCHUTILS_H
#define OKLCHUTILS_H

#include "gui/uiconfig.h"

#include <QColor>
#include <QString>
#include <QVector>

#include <optional>

enum class ThemePolarity {
    Light,
    Dark
};

struct Oklch
{
    double l;
    double c;
    double h;
    double alpha;
};

Oklch qColorToOklch(const QColor& color);

QColor oklchToQColor(const Oklch& color);
QColor oklchToValidSrgbQColor(const Oklch& color);
ThemePolarity themePolarityForBackground(const QColor& background);
QColor generatePrimaryFromBackground(
    const QColor& background,
    std::optional<double> fallbackHueDegrees = std::nullopt);
QColor generateSecondaryLinkFromBackground(const QColor& background,
                                           const QColor& primaryLink,
                                           double minimumContrast = UiConfig::GeneratedTextMinimumContrast);
double defaultOklchShadeDistanceFactor();

QVector<QColor> generateOklchShades(const QColor& base,
                                    int count,
                                    ThemePolarity polarity);
QVector<QColor> generateOklchShades(const QColor& base,
                                    int count,
                                    ThemePolarity polarity,
                                    double distanceFactor);

QColor aaForegroundForBackground(const QColor& background,
                                 double minimumContrast = UiConfig::GeneratedTextMinimumContrast);
QColor aaForegroundForBackground(const QColor& background,
                                 const QColor& preferredTint,
                                 double minimumContrast = UiConfig::GeneratedTextMinimumContrast);

QVector<QColor> aaForegroundsForBackgrounds(
    const QVector<QColor>& backgrounds,
    double minimumContrast = UiConfig::GeneratedTextMinimumContrast);
QVector<QColor> aaForegroundsForBackgrounds(
    const QVector<QColor>& backgrounds,
    const QColor& preferredTint,
    double minimumContrast = UiConfig::GeneratedTextMinimumContrast);

QString writeOklchGenerationHtmlReport(const QColor& base,
                                       int leftShadeCount,
                                       int rightShadeCount,
                                       ThemePolarity polarity,
                                       double distanceFactor,
                                       bool requireAllWcag,
                                       const QVector<QColor>& backgrounds,
                                       const QVector<QColor>& foregrounds,
                                       double minimumContrast = 7.0);

#endif // OKLCHUTILS_H
