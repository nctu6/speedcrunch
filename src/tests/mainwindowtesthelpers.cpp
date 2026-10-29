// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mainwindowtesthelpers.h"

namespace MainWindowTestHelpers {

QJsonObject themeJson(QJsonObject colors)
{
    colors.insert(QStringLiteral("$schema"), QString::fromLatin1(ColorScheme::SchemaDraft));
    colors.insert(QStringLiteral("$id"), QString::fromLatin1(ColorScheme::SchemaId));
    return colors;
}

QString themeJsonString(QJsonObject colors)
{
    return QString::fromUtf8(QJsonDocument(themeJson(colors)).toJson(QJsonDocument::Compact));
}

QJsonObject sessionJson(const QString& name)
{
    QJsonObject historyEntry;
    HistoryEntry(QStringLiteral("6*7"), Quantity(42)).serialize(historyEntry);

    QJsonArray history;
    history.append(historyEntry);

    QJsonObject values;
    values.insert(QLatin1String(SessionJsonKeys::Schema), QLatin1String(SessionJsonKeys::SchemaDialect));
    values.insert(QLatin1String(SessionJsonKeys::Id), QLatin1String(SessionJsonKeys::SchemaId));
    values.insert(QLatin1String(SessionJsonKeys::Session), name);
    values.insert(QLatin1String(SessionJsonKeys::Limit), 1000);
    values.insert(QLatin1String(SessionJsonKeys::History), history);
    values.insert(QLatin1String(SessionJsonKeys::Variables), QJsonArray());
    values.insert(QLatin1String(SessionJsonKeys::Functions), QJsonArray());
    values.insert(QLatin1String(SessionJsonKeys::Units), QJsonArray());
    values.insert(QLatin1String(SessionJsonKeys::Globals), QJsonArray());
    return values;
}

void writeFile(const QString& filePath, const QByteArray& data)
{
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(data), qint64(data.size()));
}

QJsonObject startupSessionWindow(const QString& name, const QString& id)
{
    const QJsonObject tab {
        { QStringLiteral("name"), name },
        { QStringLiteral("editor"), QJsonObject {
            { QStringLiteral("text"), QStringLiteral("saved draft") }
        } }
    };
    return QJsonObject {
        { QStringLiteral("id"), id },
        { QStringLiteral("root"), QJsonObject {
            { QStringLiteral("type"), QStringLiteral("tabs") },
            { QStringLiteral("active"), name },
            { QStringLiteral("tabs"), QJsonArray { tab } }
        } }
    };
}

QWidget* paneWidgetForDisplay(ResultDisplay* display)
{
    QWidget* widget = display;
    while (widget != nullptr && !qobject_cast<QSplitter*>(widget->parentWidget()))
        widget = widget->parentWidget();
    return widget;
}

void appendPaneScrollValues(const QJsonObject& node, QList<int>* values)
{
    const QString type = node.value(QStringLiteral("type")).toString();
    if (type == QLatin1String("pane")) {
        const QJsonArray tabs = node.value(QStringLiteral("tabs")).toArray();
        for (const QJsonValue& tabValue : tabs) {
            const QJsonObject scroll = tabValue.toObject().value(QStringLiteral("scroll")).toObject();
            if (scroll.contains(QStringLiteral("value")))
                values->append(scroll.value(QStringLiteral("value")).toInt(-1));
        }
        return;
    }

    if (type != QLatin1String("split"))
        return;

    const QJsonArray children = node.value(QStringLiteral("children")).toArray();
    for (const QJsonValue& childValue : children) {
        if (childValue.isObject())
            appendPaneScrollValues(childValue.toObject(), values);
    }
}

void appendPaneEditorTexts(const QJsonObject& node, QStringList* texts)
{
    const QString type = node.value(QStringLiteral("type")).toString();
    if (type == QLatin1String("pane")) {
        const QString active = node.value(QStringLiteral("active")).toString();
        const QJsonArray tabs = node.value(QStringLiteral("tabs")).toArray();
        for (const QJsonValue& tabValue : tabs) {
            const QJsonObject tab = tabValue.toObject();
            if (tab.value(QStringLiteral("name")).toString() != active)
                continue;
            const QJsonObject editor = tab.value(QStringLiteral("editor")).toObject();
            if (editor.contains(QStringLiteral("text")))
                texts->append(editor.value(QStringLiteral("text")).toString());
        }
        return;
    }

    if (type != QLatin1String("split"))
        return;

    const QJsonArray children = node.value(QStringLiteral("children")).toArray();
    for (const QJsonValue& childValue : children) {
        if (childValue.isObject())
            appendPaneEditorTexts(childValue.toObject(), texts);
    }
}

bool menuContainsActionText(const QMenu* menu, const QString& text)
{
    for (QAction* action : menu->actions()) {
        if (action->text() == text)
            return true;
        if (action->menu() != nullptr && menuContainsActionText(action->menu(), text))
            return true;
    }
    return false;
}

QMenu* menuWithTitle(const QMenuBar* menuBar, const QString& title)
{
    for (QAction* action : menuBar->actions()) {
        QMenu* menu = action->menu();
        if (menu != nullptr && menu->title() == title)
            return menu;
    }
    return nullptr;
}

QAction* directMenuActionWithText(const QMenu* menu, const QString& text)
{
    for (QAction* action : menu->actions()) {
        if (action->text() == text)
            return action;
    }
    return nullptr;
}

QMenu* directSubmenuWithTitle(const QMenu* menu, const QString& title)
{
    QAction* action = directMenuActionWithText(menu, title);
    return action != nullptr ? action->menu() : nullptr;
}

QList<MainWindow*> topLevelMainWindows()
{
    QList<MainWindow*> windows;
    for (QWidget* widget : QApplication::topLevelWidgets()) {
        if (MainWindow* window = qobject_cast<MainWindow*>(widget))
            windows.append(window);
    }
    return windows;
}

bool rejectActiveDialogWithTitle(const QString& title)
{
    QDialog* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
    if (dialog == nullptr)
        return false;

    const bool matches = dialog->windowTitle() == title;
    dialog->reject();
    return matches;
}

bool contextMenuContainsMainMenu(MenuTestResultDisplay* display)
{
    QMenu* menu = display->createContextMenu(display->rect().center());
    const bool mainMenuSeen = menuContainsActionText(menu, QStringLiteral("Main Menu"));
    delete menu;
    return mainMenuSeen;
}

bool editorHasPrimaryOutline(const Editor* editor, const QColor& primary)
{
    return editor != nullptr
        && editor->styleSheet().contains(
            QStringLiteral("border: %1px solid %2")
                .arg(UiConfig::OutlineStrokeWidth)
                .arg(primary.name()));
}

bool anyEditorHasPrimaryOutline(const QList<Editor*>& editors, const QColor& primary)
{
    for (const Editor* editor : editors) {
        if (editorHasPrimaryOutline(editor, primary))
            return true;
    }
    return false;
}

bool colorsAreClose(const QColor& actual, const QColor& expected, int tolerance)
{
    return qAbs(actual.red() - expected.red()) <= tolerance
        && qAbs(actual.green() - expected.green()) <= tolerance
        && qAbs(actual.blue() - expected.blue()) <= tolerance;
}

QRect imageRectForWidgetRect(const QImage& image, const QRect& rect)
{
    const qreal scale = image.devicePixelRatio();
    return QRect(qRound(rect.x() * scale), qRound(rect.y() * scale),
                 qRound(rect.width() * scale), qRound(rect.height() * scale));
}

QPoint firstPixelMatchingColor(const QImage& image, const QRect& rect, const QColor& color, int tolerance)
{
    const QRect bounded = rect.intersected(image.rect());
    for (int y = bounded.top(); y <= bounded.bottom(); ++y) {
        for (int x = bounded.left(); x <= bounded.right(); ++x) {
            if (colorsAreClose(image.pixelColor(x, y), color, tolerance))
                return QPoint(x, y);
        }
    }
    return QPoint(-1, -1);
}

QPoint firstPixelMatchingColorBlend(const QImage& image, const QRect& rect,
                                  const QColor& foreground, const QColor& background,
                                  int tolerance)
{
    const int redDelta = foreground.red() - background.red();
    const int greenDelta = foreground.green() - background.green();
    const int blueDelta = foreground.blue() - background.blue();
    const int squaredDistance = redDelta * redDelta + greenDelta * greenDelta
        + blueDelta * blueDelta;
    if (squaredDistance == 0)
        return QPoint(-1, -1);

    const QRect bounded = rect.intersected(image.rect());
    for (int y = bounded.top(); y <= bounded.bottom(); ++y) {
        for (int x = bounded.left(); x <= bounded.right(); ++x) {
            const QColor pixel = image.pixelColor(x, y);
            // Antialiased glyphs can contain only partial foreground coverage.
            // Check the blend's hue as well as its contrast with the background.
            const qreal coverage = qreal((pixel.red() - background.red()) * redDelta
                + (pixel.green() - background.green()) * greenDelta
                + (pixel.blue() - background.blue()) * blueDelta) / squaredDistance;
            if (coverage < 0.25 || coverage > 1.0)
                continue;
            const QColor blended(qRound(background.red() + coverage * redDelta),
                                 qRound(background.green() + coverage * greenDelta),
                                 qRound(background.blue() + coverage * blueDelta));
            if (colorsAreClose(pixel, blended, tolerance))
                return QPoint(x, y);
        }
    }
    return QPoint(-1, -1);
}

QPoint firstPixelDistinctFromColor(const QImage& image,
                                   const QRect& rect,
                                   const QColor& color,
                                   int minimumChannelDistance)
{
    const QRect bounded = rect.intersected(image.rect());
    for (int y = bounded.top(); y <= bounded.bottom(); ++y) {
        for (int x = bounded.left(); x <= bounded.right(); ++x) {
            const QColor pixel = image.pixelColor(x, y);
            const int distance = qMax(qAbs(pixel.red() - color.red()),
                                      qMax(qAbs(pixel.green() - color.green()),
                                           qAbs(pixel.blue() - color.blue())));
            if (distance >= minimumChannelDistance)
                return QPoint(x, y);
        }
    }
    return QPoint(-1, -1);
}

QPushButton* keypadButtonWithText(Keypad* keypad, const QString& text)
{
    if (keypad == nullptr)
        return nullptr;

    for (QPushButton* button : keypad->findChildren<QPushButton*>()) {
        if (button->text() == text)
            return button;
    }
    return nullptr;
}

QAction* keypadModeAction(MainWindow* window, Settings::KeypadMode mode)
{
    if (window == nullptr)
        return nullptr;

    for (QAction* action : window->findChildren<QAction*>()) {
        if (action->isCheckable()
                && action->data().isValid()
                && action->data().toInt() == static_cast<int>(mode)) {
            return action;
        }
    }
    return nullptr;
}

QColor keypadPrimaryStateBackgroundForTest(const QColor& primary,
                                           const QColor& stateBackground,
                                           const QColor& normalBackground)
{
    Oklch primaryOklch = qColorToOklch(primary);
    const Oklch stateOklch = qColorToOklch(stateBackground);
    const Oklch normalOklch = qColorToOklch(normalBackground);
    const double offset = stateOklch.l - normalOklch.l;
    if (qAbs(offset) < 1e-9)
        return primary;

    primaryOklch.l = qBound(0.0, primaryOklch.l + offset, 1.0);
    return oklchToValidSrgbQColor(primaryOklch);
}

QColor keypadPrimaryHueFillForTest(const QColor& primary,
                                   const QColor& stateBackground,
                                   const QColor& normalBackground,
                                   int primaryPercent)
{
    const double primaryRatio =
        double(qBound(0, primaryPercent, 100)) / 100.0;
    if (primaryRatio <= 0.0)
        return stateBackground;

    const QColor primaryStateBackground =
        keypadPrimaryStateBackgroundForTest(primary, stateBackground, normalBackground);
    if (primaryRatio >= 1.0)
        return primaryStateBackground;

    const Oklch stateOklch = qColorToOklch(stateBackground);
    const Oklch primaryStateOklch = qColorToOklch(primaryStateBackground);
    return oklchToValidSrgbQColor(Oklch {
        stateOklch.l + (primaryStateOklch.l - stateOklch.l) * primaryRatio,
        stateOklch.c + (primaryStateOklch.c - stateOklch.c) * primaryRatio,
        primaryStateOklch.h,
        stateOklch.alpha
    });
}

QImage dockSeparatorPrimitiveImage(QWidget* widget,
                                   QStyle::State state,
                                   const QRect& separatorRect,
                                   const QSize& imageSize)
{
    QImage image(imageSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QStyleOption option;
    option.rect = separatorRect;
    option.state = state;
    option.palette = widget->palette();

    QPainter painter(&image);
    QApplication::style()->drawPrimitive(QStyle::PE_IndicatorDockWidgetResizeHandle,
                                         &option,
                                         &painter,
                                         widget);
    return image;
}

QImage dockSeparatorPrimitiveImage(QWidget* widget, QStyle::State state, const QSize& size)
{
    return dockSeparatorPrimitiveImage(widget, state, QRect(QPoint(0, 0), size), size);
}

QColor dockSeparatorPrimitiveColor(QWidget* widget, QStyle::State state)
{
    const QImage image = dockSeparatorPrimitiveImage(widget, state);
    return image.pixelColor(image.rect().center());
}

QRect sessionTabPillRect(const QTabBar* tabBar)
{
    return tabBar->tabRect(tabBar->currentIndex()).adjusted(2, 3, -2, 0);
}

QColor selectedSessionTabFillColor(QTabBar* tabBar)
{
    const QRect pill = sessionTabPillRect(tabBar);
    const QImage image = tabBar->grab().toImage();
    return image.pixelColor(pill.left() + 6, pill.center().y());
}

bool selectedSessionTabHasBottomIndicator(QTabBar* tabBar, const QColor& color)
{
    const QRect pill = sessionTabPillRect(tabBar);
    const QImage image = tabBar->grab().toImage();
    const int firstY = qMax(pill.top(), pill.bottom() - UiConfig::ActiveSessionTabIndicatorStrokeWidth);
    for (int y = firstY; y <= pill.bottom(); ++y) {
        for (int x = pill.left() + 4; x <= pill.right() - 4; ++x) {
            if (image.rect().contains(x, y) && colorsAreClose(image.pixelColor(x, y), color))
                return true;
        }
    }
    return false;
}

Editor* editorForDisplay(ResultDisplay* display)
{
    QWidget* page = display != nullptr ? display->parentWidget() : nullptr;
    return page ? page->findChild<Editor*>(QString(), Qt::FindDirectChildrenOnly) : nullptr;
}

QTabBar* tabBarForDisplay(ResultDisplay* display)
{
    QWidget* pane = paneWidgetForDisplay(display);
    return pane ? pane->findChild<QTabBar*>() : nullptr;
}

QString visibleResultPreviewText(const MainWindow& window)
{
    for (QLabel* label : window.findChildren<QLabel*>()) {
        if (!label->isVisible())
            continue;
        const QString text = label->text();
        if (text.contains(QStringLiteral("Current result:"))
            || text.contains(QStringLiteral("Selection result:"))) {
            return text;
        }
    }
    return QString();
}

void sendTabDragMouseEvent(QTabBar* tabBar, QEvent::Type type, const QPoint& pos,
                           Qt::MouseButton button, Qt::MouseButtons buttons)
{
    QMouseEvent event(type,
                      QPointF(pos),
                      QPointF(tabBar->mapToGlobal(pos)),
                      button,
                      buttons,
                      Qt::NoModifier);
    QCoreApplication::sendEvent(tabBar, &event);
}

QKeyCombination restoreClosedTabShortcut()
{
    return QKeyCombination(Qt::ControlModifier | Qt::ShiftModifier, Qt::Key_T);
}

QKeyCombination nextSessionTabShortcut()
{
#if defined(Q_OS_MACOS)
    return QKeyCombination(Qt::ControlModifier | Qt::AltModifier, Qt::Key_Right);
#else
    return QKeyCombination(Qt::ControlModifier, Qt::Key_PageDown);
#endif
}

QKeyCombination previousSessionTabShortcut()
{
#if defined(Q_OS_MACOS)
    return QKeyCombination(Qt::ControlModifier | Qt::AltModifier, Qt::Key_Left);
#else
    return QKeyCombination(Qt::ControlModifier, Qt::Key_PageUp);
#endif
}

bool focusIsWithin(QWidget* target)
{
    QWidget* focused = QApplication::focusWidget();
    if (target == nullptr || focused == nullptr)
        return false;
    if (focused == target || target->isAncestorOf(focused))
        return true;
    if (QAbstractItemView* view = qobject_cast<QAbstractItemView*>(target)) {
        QWidget* viewport = view->viewport();
        return focused == viewport
            || (viewport != nullptr && viewport->isAncestorOf(focused));
    }
    return false;
}

} // namespace MainWindowTestHelpers
