// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef SPEEDCRUNCH_EDITORGEOMETRYTESTHELPERS_H
#define SPEEDCRUNCH_EDITORGEOMETRYTESTHELPERS_H

#include <QAbstractTextDocumentLayout>
#include <QPlainTextEdit>
#include <QTextBlock>
#include <QTextLayout>

namespace EditorGeometryTestHelpers {

inline int visibleTextHeight(const QPlainTextEdit& editor, int maximumLines = 5)
{
    QTextCursor first(editor.document());
    QTextCursor last(first);
    int remaining = maximumLines;
    for (QTextBlock block = editor.document()->begin(); block.isValid(); block = block.next()) {
        editor.document()->documentLayout()->blockBoundingRect(block);
        const QTextLayout* layout = block.layout();
        const int count = qMin(remaining, layout->lineCount());
        if (count > 0)
            last.setPosition(block.position() + layout->lineAt(count - 1).textStart());
        remaining -= count;
        if (remaining == 0)
            break;
    }
    // Both rectangles have the same scroll offset, even after the editor
    // scrolls past its first five lines. Measure the rendered cursor span.
    return editor.cursorRect(last).bottom() - editor.cursorRect(first).top() + 1;
}

} // namespace EditorGeometryTestHelpers

#endif
