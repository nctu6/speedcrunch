// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef GUI_STARTUPAPPEARANCE_H
#define GUI_STARTUPAPPEARANCE_H

#include "core/startupdiagnostics.h"
#include "core/colorscheme.h"
#include "core/settings.h"
#include <QApplication>
#include <QCryptographicHash>
#include <QDataStream>
#include <QPlainTextEdit>
#include <QTextDocument>
#include <QVariant>

namespace StartupDiagnostics {

inline QByteArray appearanceInputs(const QVariantList& values)
{
    if (!isRecording())
        return {};
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream << values;
    return QCryptographicHash::hash(bytes, QCryptographicHash::Sha256);
}

inline QByteArray themeInputs(const ColorScheme& scheme)
{
    if (!isRecording())
        return {};
    QVariantList values {scheme.isValid()};
    for (const auto& role : ColorScheme::roleNames()) {
        values.append(scheme.hasColorForRole(role.second));
        values.append(scheme.colorForRole(role.second));
    }
    // Invalid schemes and missing accents can use application palette colors.
    if (!scheme.isValid() || !scheme.hasColorForRole(ColorScheme::Primary)) {
        values.append(QApplication::palette().color(QPalette::Base));
        values.append(QApplication::palette().color(QPalette::Text));
    }
    return appearanceInputs(values);
}

inline QByteArray configuredThemeInputs()
{
    if (!isRecording())
        return {};
    const Settings* settings = Settings::instance();
    return appearanceInputs({settings->colorScheme,
        settings->colorScheme == QLatin1String("Custom") ? settings->customColorSchemeJson : QString()});
}

inline void observeHighlightInputs(AppearanceScope& scope, QPlainTextEdit* widget,
                                  const QByteArray& theme, const QVariantList& surfaces,
                                  const QObject* evaluator)
{
    if (!isRecording())
        return;
    scope.observeInputs("document", appearanceInputs({widget->document()->revision(),
        widget->document()->characterCount(), widget->document()->blockCount()}));
    scope.observeInputs("theme", appearanceInputs({theme, QVariant(surfaces)}));
    const Settings* settings = Settings::instance();
    scope.observeInputs("settings", appearanceInputs({settings->syntaxHighlighting,
        settings->digitGrouping, settings->digitGroupingIntegerPartOnly,
        settings->inputDigitGrouping, settings->digitGroupingSeparator(),
        int(settings->radixCharacter()), int(settings->editorDisplayStyle), widget->font()}));
    // Identity says which evaluator is attached, not whether its symbols change.
    const int evaluatorId = appearanceObjectId(evaluator);
    if (evaluatorId >= 0)
        scope.observeInputs("evaluator", appearanceInputs({evaluatorId}));
}

inline void observeAppearanceSettings(AppearanceScope& scope)
{
    if (!isRecording())
        return;
    const Settings* settings = Settings::instance();
    scope.observeInputs("appearance", appearanceInputs({int(settings->menuAppearance),
        int(settings->dockAppearance), int(settings->statusBarAppearance),
        int(settings->keypadAppearance), int(settings->editorDisplayStyle)}));
}

inline void setAppearancePalette(QWidget* widget, const QPalette& palette)
{
    if (isRecording())
        appearanceAssignment(widget, true, widget->palette() != palette, false);
    widget->setPalette(palette);
}

inline void setAppearanceStyleSheet(QWidget* widget, const QString& styleSheet)
{
    // Reassigning the same stylesheet makes Qt repolish the widget tree.
    // Palette updates still run because polishing can reset viewport colors.
    const bool changed = widget->styleSheet() != styleSheet;
    if (isRecording())
        appearanceAssignment(widget, false, changed, !changed);
    if (changed)
        widget->setStyleSheet(styleSheet);
}

}

#endif
