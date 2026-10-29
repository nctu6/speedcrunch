// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef GUI_CONSTANTBROWSERUTILS_H
#define GUI_CONSTANTBROWSERUTILS_H

#include "core/constants.h"
#include "core/mathdsl.h"
#include "core/unicodechars.h"

#include <QTreeWidget>

namespace ConstantBrowserUtils {

inline QString unit(const Constant& constant)
{
    QString result = constant.unit;
    result.replace(UnicodeChars::MiddleDot, MathDsl::MulDotOp);
    return result;
}

inline QString expression(const Constant& constant)
{
    if (constant.domainType == ConstantDomain::Mathematics && constant.unit.isEmpty()) {
        if (constant.value == QStringLiteral("3.14159265358979323846264338327950288419716939937511"))
            return QString(UnicodeChars::Pi);
        if (constant.value == QStringLiteral("2.71828182845904523536028747135266249775724709369996"))
            return QStringLiteral("e");
    }
    return constant.unit.isEmpty()
        ? constant.value
        : QStringLiteral("%1%2[%3]").arg(constant.value, QString(MathDsl::QuantSp), unit(constant));
}

inline bool matches(const Constant& constant, const QString& term,
                    const QString& domain = {}, const QString& subdomain = {})
{
    return (domain.isEmpty() || constant.domain == domain)
        && (subdomain.isEmpty() || constant.subdomain == subdomain)
        && (constant.name.contains(term, Qt::CaseInsensitive)
            || unit(constant).contains(term, Qt::CaseInsensitive));
}

inline void moveSelection(QTreeWidget* list, int key)
{
    const int count = list->topLevelItemCount();
    if (count == 0)
        return;
    const int current = list->indexOfTopLevelItem(list->currentItem());
    const int next = current < 0 ? (key == Qt::Key_Up ? count - 1 : 0)
        : qBound(0, current + (key == Qt::Key_Up ? -1 : 1), count - 1);
    list->setCurrentItem(list->topLevelItem(next));
    list->scrollToItem(list->currentItem());
}

} // namespace ConstantBrowserUtils

#endif
