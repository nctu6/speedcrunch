// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef SPEEDCRUNCH_GTKMENUPALETTE_H
#define SPEEDCRUNCH_GTKMENUPALETTE_H

#include <QPalette>
#include <optional>

namespace GtkMenuPalette {
// GTK and Cairo are optional runtime libraries. Keep their public ABI here
// so the GNOME menu colors do not add a build dependency on other platforms.
struct Rgba { double red, green, blue, alpha; };
struct Api {
    void* (*popoverNew)(void*);
    void* (*modelButtonNew)();
    void (*add)(void*, void*);
    void* (*context)(void*);
    void (*setState)(void*, unsigned);
    void (*color)(void*, unsigned, Rgba*);
    void (*background)(void*, void*, double, double, double, double);
    void (*destroyWidget)(void*);
    void* (*refSink)(void*);
    void (*unref)(void*);
    void* (*surfaceForData)(unsigned char*, int, int, int, int);
    void* (*createPainter)(void*);
    void (*destroyPainter)(void*);
    void (*flushSurface)(void*);
    void (*destroySurface)(void*);
};

QPalette read(const Api& api, const QPalette& fallback);
std::optional<QPalette> systemPalette(const QPalette& fallback);
void invalidate();
}

#endif
