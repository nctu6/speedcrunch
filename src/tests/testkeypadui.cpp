// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mainwindowtesthelpers.h"

using namespace MainWindowTestHelpers;

class TestKeypadUi : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init() { UiTestFixture::resetSettings(); }
    void classic_keypad_boundary_follows_bordering_panes_data();
    void classic_keypad_boundary_follows_bordering_panes();
    void classic_keypad_boundary_restores_nested_panes();
    void keypad_buttons_use_custom_themed_tooltips();
    void custom_keypad_action_stays_checked_after_dialog_accepts();
    void keypad_power_button_uses_exponent_label_but_inserts_caret();
    void keypad_nth_root_button_inserts_root_data();
    void keypad_nth_root_button_inserts_root();
    void keypad_zoom_round_trip_restores_button_sizes_data();
    void keypad_zoom_round_trip_restores_button_sizes();
    void keypad_fifty_percent_zoom_scales_and_restores_data();
    void keypad_fifty_percent_zoom_scales_and_restores();
    void keypad_fifty_percent_zoom_survives_settings_reload();
    void keypad_appearance_defaults_to_modern_and_persists();
    void keypad_appearance_switches_all_windows_data();
    void keypad_appearance_switches_all_windows();
    void keypad_input_stays_in_own_window_data();
    void keypad_input_stays_in_own_window();
    void keypad_view_menu_tracks_and_changes_only_active_window();
};

void TestKeypadUi::initTestCase()
{
    // Fail before a file-operation test can open an unhandled error dialog.
    for (const QString& path : {Settings::getConfigPath(), Settings::getDataPath(),
                                Settings::getCachePath()}) {
        QVERIFY2(QDir().mkpath(path),
                 qPrintable(QStringLiteral("Cannot create test storage at %1").arg(path)));
        QTemporaryFile probe(QDir(path).filePath(QStringLiteral("storage-probe-XXXXXX")));
        QVERIFY2(probe.open(),
                 qPrintable(QStringLiteral("Cannot write test storage at %1: %2")
                                .arg(path, probe.errorString())));
    }
}

void TestKeypadUi::classic_keypad_boundary_follows_bordering_panes_data()
{
    QTest::addColumn<QString>("background");
    QTest::newRow("dark") << QStringLiteral("#310b28");
    QTest::newRow("light") << QStringLiteral("#eeeeee");
}

void TestKeypadUi::classic_keypad_boundary_follows_bordering_panes()
{
    QFETCH(QString, background);
    MainWindowStateGuard guard;
    guard.settings->editorDisplayStyle = Settings::EditorDisplayClassic;
    guard.settings->colorScheme = QStringLiteral("Custom");
    guard.settings->customColorSchemeJson = themeJsonString(
        QJsonObject{{QStringLiteral("background"), background}});
    guard.settings->keypadMode = Settings::KeypadModeBasicWide;
    guard.settings->autoCalc = false;
    MainWindow window(false);
    window.resize(1000, 900);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto* styles = window.findChild<QMenu*>(QStringLiteral("EditorDisplayStyleMenu"));
    QVERIFY(styles);
    const auto checkBoundary = [&window, &guard](int expectedCount) {
        QCoreApplication::processEvents();
        QVERIFY(window.findChildren<QWidget*>(QStringLiteral("ClassicKeypadSeparator")).isEmpty());
        Keypad* keypad = window.findChild<Keypad*>();
        const bool classic = guard.settings->editorDisplayStyle == Settings::EditorDisplayClassic;
        const int keypadTop = keypad
            ? keypad->parentWidget()->mapToGlobal(QPoint()).y() : -1;
        int borderingCount = 0;
        for (ResultDisplay* display : window.findChildren<ResultDisplay*>()) {
            QWidget* pane = paneWidgetForDisplay(display);
            QVERIFY(pane);
            const bool bordersKeypad = keypad
                && pane->mapToGlobal(QPoint(0, pane->height())).y() == keypadTop;
            if (!classic || !bordersKeypad)
                continue;
            ++borderingCount;
            Editor* editor = editorForDisplay(display);
            QVERIFY(editor);
            QCOMPARE(editor->mapToGlobal(QPoint(0, editor->height())).y(), keypadTop);
        }
        QCOMPARE(borderingCount, expectedCount);
    };

    checkBoundary(1);
    Editor* leftEditor = window.findChild<Editor*>();
    QVERIFY(QMetaObject::invokeMethod(&window, "splitActivePaneRight", Qt::DirectConnection));
    checkBoundary(2);
    Editor* upperRightEditor = qobject_cast<Editor*>(QApplication::focusWidget());
    QVERIFY(upperRightEditor);
    QVERIFY(QMetaObject::invokeMethod(&window, "splitActivePaneDown", Qt::DirectConnection));
    checkBoundary(2);
    QPointer<Editor> lowerRightEditor = qobject_cast<Editor*>(QApplication::focusWidget());
    QVERIFY(lowerRightEditor);
    QTest::mouseClick(upperRightEditor->viewport(), Qt::LeftButton);
    QVERIFY(QMetaObject::invokeMethod(&window, "splitActivePaneRight", Qt::DirectConnection));
    checkBoundary(2);
    // Both children of the upper horizontal split stay above the keypad.
    QCOMPARE(window.findChildren<ResultDisplay*>().size(), 4);
    window.resize(1100, 1000);
    checkBoundary(2);
    for (QSplitter* splitter : window.findChildren<QSplitter*>()) {
        QList<int> sizes = splitter->sizes();
        if (sizes.size() > 1) {
            sizes[0] += 30;
            sizes[1] = qMax(1, sizes[1] - 30);
            splitter->setSizes(sizes);
        }
    }
    checkBoundary(2);

    MainWindow other(false);
    QVERIFY(QMetaObject::invokeMethod(&other, "restoreWindowKeypadLayout", Qt::DirectConnection,
        Q_ARG(bool, false), Q_ARG(int, int(Settings::KeypadModeDisabled))));
    other.show();
    styles->actions().at(1)->trigger();
    checkBoundary(0);
    const QColor inactiveColor = leftEditor->palette().color(QPalette::Base);
    styles->actions().at(0)->trigger();
    checkBoundary(2);
    QVERIFY(other.findChildren<QWidget*>(QStringLiteral("ClassicKeypadSeparator")).isEmpty());
    QCOMPARE(leftEditor->palette().color(QPalette::Base).rgba(), inactiveColor.rgba());

    QVERIFY(QMetaObject::invokeMethod(&window, "restoreWindowKeypadLayout", Qt::DirectConnection,
        Q_ARG(bool, false), Q_ARG(int, int(Settings::KeypadModeDisabled))));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    checkBoundary(0);
    for (const auto mode : {Settings::KeypadModeScientificWide, Settings::KeypadModeScientificNarrow}) {
        QVERIFY(QMetaObject::invokeMethod(&window, "restoreWindowKeypadLayout", Qt::DirectConnection,
            Q_ARG(bool, true), Q_ARG(int, int(mode))));
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        checkBoundary(2);
    }
    guard.settings->customColorSchemeJson = themeJsonString(
        QJsonObject{{QStringLiteral("background"), QStringLiteral("#213c31")}});
    QVERIFY(QMetaObject::invokeMethod(&window, "colorSchemeChanged", Qt::DirectConnection));
    checkBoundary(2);
    QVERIFY(leftEditor->palette().color(QPalette::Base).rgba() != inactiveColor.rgba());

    // Closing the bottom pane brings the two upper panes down to the keypad.
    QVERIFY(lowerRightEditor);
    QTest::mouseClick(lowerRightEditor->viewport(), Qt::LeftButton);
    QVERIFY(QMetaObject::invokeMethod(&window, "closeCurrentPane", Qt::DirectConnection));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    checkBoundary(3);
}

void TestKeypadUi::classic_keypad_boundary_restores_nested_panes()
{
    MainWindowStateGuard guard;
    guard.settings->editorDisplayStyle = Settings::EditorDisplayClassic;
    guard.settings->keypadMode = Settings::KeypadModeBasicWide;
    {
        MainWindow original(false);
        QVERIFY(QMetaObject::invokeMethod(&original, "splitActivePaneRight", Qt::DirectConnection));
        QVERIFY(QMetaObject::invokeMethod(&original, "splitActivePaneDown", Qt::DirectConnection));
        original.persistSessionAndSettingsForShutdown();
    }
    MainWindow restored;
    restored.resize(1000, 900);
    restored.show();
    QVERIFY(QTest::qWaitForWindowExposed(&restored));
    QCoreApplication::processEvents();
    Keypad* keypad = restored.findChild<Keypad*>();
    QVERIFY(keypad);
    const int keypadTop = keypad->parentWidget()->mapToGlobal(QPoint()).y();
    const auto displays = restored.findChildren<ResultDisplay*>();
    QCOMPARE(displays.size(), 3);
    QVERIFY(restored.findChildren<QWidget*>(QStringLiteral("ClassicKeypadSeparator")).isEmpty());
    int borderingCount = 0;
    for (ResultDisplay* display : displays) {
        QWidget* pane = paneWidgetForDisplay(display);
        const bool bordersKeypad = pane->mapToGlobal(QPoint(0, pane->height())).y() == keypadTop;
        if (!bordersKeypad)
            continue;
        ++borderingCount;
        Editor* editor = editorForDisplay(display);
        QVERIFY(editor);
        QCOMPARE(editor->mapToGlobal(QPoint(0, editor->height())).y(), keypadTop);
    }
    QCOMPARE(borderingCount, 2);
}

void TestKeypadUi::keypad_buttons_use_custom_themed_tooltips()
{
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;
    settings->colorScheme = QStringLiteral("Custom");
    settings->customColorSchemeJson = themeJsonString(QJsonObject{{QStringLiteral("background"), QStringLiteral("#e5eee8")}});
    settings->keypadMode = Settings::KeypadModeBasicWide;
    settings->keypadVisible = true;
    settings->bitfieldVisible = false;

    const QVector<QColor> shades =
        generateOklchShades(QColor(QStringLiteral("#e5eee8")), 6, ThemePolarity::Light);
    const QVector<QColor> foregrounds = aaForegroundsForBackgrounds(shades);
    const QColor popupBackground = shades.at(UiConfig::CompletionPopupBackgroundShade);
    const QColor popupForeground = foregrounds.at(UiConfig::CompletionPopupBackgroundShade);
    const QColor popupOutline = shades.at(UiConfig::CompletionPopupOutlineShade);

    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    Keypad* keypad = window.findChild<Keypad*>();
    QVERIFY(keypad != nullptr);
    QPushButton* plusButton = keypadButtonWithText(keypad, QStringLiteral("+"));
    QVERIFY(plusButton != nullptr);
    QCOMPARE(plusButton->toolTip(), QString());

    const QPoint buttonCenter = plusButton->rect().center();
    QMouseEvent moveEvent(QEvent::MouseMove,
                          QPointF(buttonCenter),
                          QPointF(plusButton->mapToGlobal(buttonCenter)),
                          Qt::NoButton,
                          Qt::NoButton,
                          Qt::NoModifier);
    QCoreApplication::sendEvent(plusButton, &moveEvent);

    QFrame* summaryPopup = keypad->findChild<QFrame*>(QStringLiteral("keypadSummaryPopup"));
    QTRY_VERIFY(summaryPopup != nullptr && summaryPopup->isVisible());
    QLabel* summaryPopupLabel =
        summaryPopup->findChild<QLabel*>(QStringLiteral("keypadSummaryPopupLabel"));
    QVERIFY(summaryPopupLabel != nullptr);
    QCOMPARE(summaryPopupLabel->text(), QStringLiteral("Addition"));
    QVERIFY(summaryPopup->styleSheet().contains(popupBackground.name()));
    QVERIFY(summaryPopup->styleSheet().contains(popupForeground.name()));
    QVERIFY(summaryPopup->styleSheet().contains(QStringLiteral("border: %1px solid %2")
                                                    .arg(UiConfig::PopupOutlineStrokeWidth)
                                                    .arg(popupOutline.name())));
    QVERIFY(!summaryPopup->mask().isEmpty());
}

void TestKeypadUi::custom_keypad_action_stays_checked_after_dialog_accepts()
{
    Settings* settings = Settings::instance();
    struct SettingsGuard {
        Settings* settings;
        Settings::KeypadMode oldKeypadMode;
        bool oldKeypadVisible;
        Settings::CustomKeypad oldCustomKeypad;
        bool oldHasNumberFormatStyleSetting;
        QByteArray oldSkipUpdateCheck;
        bool hadSkipUpdateCheck;

        ~SettingsGuard()
        {
            settings->keypadMode = oldKeypadMode;
            settings->keypadVisible = oldKeypadVisible;
            settings->customKeypad = oldCustomKeypad;
            settings->hasNumberFormatStyleSetting = oldHasNumberFormatStyleSetting;
            if (hadSkipUpdateCheck)
                qputenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK", oldSkipUpdateCheck);
            else
                qunsetenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK");
        }
    } guard {
        settings,
        settings->keypadMode,
        settings->keypadVisible,
        settings->customKeypad,
        settings->hasNumberFormatStyleSetting,
        qgetenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK"),
        qEnvironmentVariableIsSet("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK")
    };

    qputenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK", "1");
    settings->keypadMode = Settings::KeypadModeBasicWide;
    settings->keypadVisible = true;
    settings->hasNumberFormatStyleSetting = true;

    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QAction* basicAction = keypadModeAction(&window, Settings::KeypadModeBasicWide);
    QAction* customAction = keypadModeAction(&window, Settings::KeypadModeCustom);
    QVERIFY(basicAction != nullptr);
    QVERIFY(customAction != nullptr);
    QVERIFY(basicAction->isChecked());

    QTimer::singleShot(0, &window, []() {
        QDialog* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        if (dialog != nullptr)
            dialog->accept();
    });
    QVERIFY(QMetaObject::invokeMethod(&window,
                                      "setKeypadMode",
                                      Qt::DirectConnection,
                                      Q_ARG(QAction*, customAction)));
    QCoreApplication::processEvents();

    QCOMPARE(settings->keypadMode, Settings::KeypadModeCustom);
    QVERIFY(customAction->isChecked());
    QVERIFY(!basicAction->isChecked());
}

void TestKeypadUi::keypad_power_button_uses_exponent_label_but_inserts_caret()
{
    const QString powerLabel = QString::fromUtf8("xʸ");

    for (const Keypad::LayoutMode layoutMode : {
             Keypad::LayoutModeScientificWide,
             Keypad::LayoutModeScientificNarrow
         }) {
        const QList<Keypad::CustomButtonDescription> presetButtons =
            Keypad::presetCustomButtons(layoutMode, QLatin1Char('.'));
        bool foundPowerButton = false;
        for (const auto& button : presetButtons) {
            QVERIFY(button.label != QStringLiteral("^"));
            if (button.label != powerLabel)
                continue;

            foundPowerButton = true;
            QCOMPARE(button.action, int(Settings::CustomKeypadActionInsertText));
            QCOMPARE(button.text, QStringLiteral("^"));
        }
        QVERIFY(foundPowerButton);
    }

    MainWindowStateGuard guard;
    Settings* settings = Settings::instance();
    settings->keypadMode = Settings::KeypadModeScientificWide;
    settings->keypadVisible = true;
    settings->hasNumberFormatStyleSetting = true;

    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    Keypad* keypad = window.findChild<Keypad*>();
    QVERIFY(keypad != nullptr);
    QPushButton* powerButton = keypadButtonWithText(keypad, powerLabel);
    QVERIFY(powerButton != nullptr);
    QCOMPARE(keypadButtonWithText(keypad, QStringLiteral("^")), nullptr);

    Editor* editor = window.findChild<Editor*>();
    QVERIFY(editor != nullptr);
    editor->setText(QStringLiteral("2"));
    editor->setCursorPosition(editor->text().size());

    QTest::mouseClick(powerButton, Qt::LeftButton);
    QTRY_COMPARE(editor->text(), QStringLiteral("2^"));
}

void TestKeypadUi::keypad_nth_root_button_inserts_root_data()
{
    QTest::addColumn<int>("layoutMode");
    QTest::addColumn<bool>("custom");
    QTest::newRow("scientific-wide") << int(Keypad::LayoutModeScientificWide) << false;
    QTest::newRow("scientific-narrow") << int(Keypad::LayoutModeScientificNarrow) << false;
    QTest::newRow("custom-wide") << int(Keypad::LayoutModeScientificWide) << true;
    QTest::newRow("custom-narrow") << int(Keypad::LayoutModeScientificNarrow) << true;
}

void TestKeypadUi::keypad_nth_root_button_inserts_root()
{
    QFETCH(int, layoutMode);
    QFETCH(bool, custom);
    const Function* rootFunction = FunctionRepo::instance()->find(QStringLiteral("root"));
    QVERIFY(rootFunction != nullptr);
    QCOMPARE(rootFunction->usage(), QStringLiteral("n; x"));
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;
    const auto oldCustomKeypad = settings->customKeypad;
    const auto customGuard = qScopeGuard([&]() { settings->customKeypad = oldCustomKeypad; });
    settings->sessionLayoutJson.clear();
    settings->windowState.clear();
    settings->windowGeometry.clear();
    settings->keypadVisible = true;
    settings->hasNumberFormatStyleSetting = true;

    const auto layout = static_cast<Keypad::LayoutMode>(layoutMode);
    const bool wide = layout == Keypad::LayoutModeScientificWide;
    const QString rootLabel = QString::fromUtf8("ⁿ√");
    int rows = 0;
    int columns = 0;
    const auto preset = Keypad::presetCustomButtons(layout, QLatin1Char('.'), &rows, &columns);
    int rootButtonCount = 0;
    int squareButtonCount = 0;
    int separatorButtonCount = 0;
    for (const auto& button : preset) {
        QVERIFY(button.label != QString::fromUtf8("∛"));
        QVERIFY(button.text != QStringLiteral("cbrt("));
        QVERIFY(button.label != QStringLiteral("x"));
        QVERIFY(button.label != QStringLiteral("x="));
        if (button.label == QString::fromUtf8("x²")) {
            ++squareButtonCount;
            QCOMPARE(button.action, int(Settings::CustomKeypadActionInsertText));
            QCOMPARE(button.text, QStringLiteral("^2"));
        }
        if (button.label == QStringLiteral(";")) {
            ++separatorButtonCount;
            QCOMPARE(button.action, int(Settings::CustomKeypadActionInsertText));
            QCOMPARE(button.text, QStringLiteral(";"));
        }
        if (button.label != rootLabel)
            continue;
        ++rootButtonCount;
        QCOMPARE(button.action, int(Settings::CustomKeypadActionInsertText));
        QCOMPARE(button.text, QStringLiteral("root("));
        QCOMPARE(button.row, wide ? 1 : 5);
        QCOMPARE(button.column, wide ? 5 : 1);
    }
    QCOMPARE(rootButtonCount, 1);
    QCOMPARE(squareButtonCount, 1);
    QCOMPARE(separatorButtonCount, 1);

    if (custom) {
        settings->keypadMode = Settings::KeypadModeCustom;
        settings->customKeypad.rows = rows;
        settings->customKeypad.columns = columns;
        settings->customKeypad.buttons.clear();
        for (const auto& button : preset) {
            settings->customKeypad.buttons.append({button.row, button.column, button.label,
                button.text, static_cast<Settings::CustomKeypadButtonAction>(button.action)});
        }
    } else {
        settings->keypadMode = wide ? Settings::KeypadModeScientificWide
                                   : Settings::KeypadModeScientificNarrow;
    }

    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    Keypad* keypad = window.findChild<Keypad*>();
    QVERIFY(keypad != nullptr);
    const QStringList expectedRows = wide
        ? QStringList{
            QString::fromUtf8("7 8 9 ÷ √ x² E exp ⌧ ⌫"),
            QString::fromUtf8("4 5 6 × xʸ ⁿ√ ln log10 π ans"),
            QString::fromUtf8("1 2 3 − sin arcsin cos arccos ( )"),
            QString::fromUtf8("0 . = + tan arctan mod ; % !")}
        : QStringList{
            QString::fromUtf8("7 8 9 ÷ ⌧"),
            QString::fromUtf8("4 5 6 × ⌫"),
            QString::fromUtf8("1 2 3 − ("),
            QStringLiteral("0 . = + )"),
            QString::fromUtf8("√ x² E exp π"),
            QString::fromUtf8("xʸ ⁿ√ ln log10 ans"),
            QStringLiteral("sin arcsin cos arccos mod"),
            QStringLiteral("tan arctan % ! ;")};
    QGridLayout* grid = qobject_cast<QGridLayout*>(keypad->layout());
    QVERIFY(grid != nullptr);
    QCOMPARE(rows, expectedRows.size());
    QCOMPARE(columns, wide ? 10 : 5);
    QCOMPARE(grid->count(), 40);
    for (int row = 0; row < expectedRows.size(); ++row) {
        const QStringList labels = expectedRows.at(row).split(QLatin1Char(' '));
        QCOMPARE(labels.size(), columns);
        for (int column = 0; column < columns; ++column) {
            QLayoutItem* item = grid->itemAtPosition(row, column);
            QVERIFY(item != nullptr);
            QPushButton* button = qobject_cast<QPushButton*>(item->widget());
            QVERIFY(button != nullptr);
            const QString expectedLabel = labels.at(column) == QStringLiteral(".")
                ? QString(settings->radixCharacter()) : labels.at(column);
            QCOMPARE(button->text(), expectedLabel);
            QVERIFY(button->isVisible());
        }
    }
    QPushButton* rootButton = keypadButtonWithText(keypad, rootLabel);
    QVERIFY(rootButton != nullptr);
    QCOMPARE(keypadButtonWithText(keypad, QString::fromUtf8("∛")), nullptr);

    if (!custom) {
        const QPoint center = rootButton->rect().center();
        QHelpEvent tooltipEvent(QEvent::ToolTip, center, rootButton->mapToGlobal(center));
        QCoreApplication::sendEvent(rootButton, &tooltipEvent);
        QFrame* popup = keypad->findChild<QFrame*>(QStringLiteral("keypadSummaryPopup"));
        QTRY_VERIFY(popup != nullptr && popup->isVisible());
        QLabel* label = popup->findChild<QLabel*>(QStringLiteral("keypadSummaryPopupLabel"));
        QVERIFY(label != nullptr);
        QCOMPARE(label->text(), Keypad::tr("Nth root"));
    }

    Editor* editor = window.findChild<Editor*>();
    ResultDisplay* display = window.findChild<ResultDisplay*>();
    QVERIFY(editor != nullptr);
    QVERIFY(display != nullptr);
    editor->setText(QString());
    editor->setAutoCompletionEnabled(false);
    QTest::mouseClick(rootButton, Qt::LeftButton);
    QCOMPARE(editor->text(), custom ? QStringLiteral("root()") : QStringLiteral("root("));
    QCOMPARE(editor->cursorPosition(), 5);

    const auto clickButton = [keypad](const QString& label) {
        QTest::mouseClick(keypadButtonWithText(keypad, label), Qt::LeftButton);
    };
    clickButton(QStringLiteral("5"));
    clickButton(QStringLiteral(";"));
    clickButton(QStringLiteral("3"));
    clickButton(QStringLiteral("2"));
    if (!custom)
        clickButton(QStringLiteral(")"));
    QCOMPARE(editor->text(), QStringLiteral("root(5;32)"));
    QPushButton* equalsButton = keypadButtonWithText(keypad, QStringLiteral("="));
    QVERIFY(equalsButton != nullptr);
    const int historySize = display->session()->historySize();
    QTest::mouseClick(equalsButton, Qt::LeftButton);
    QCOMPARE(display->session()->historySize(), historySize + 1);
    const Quantity result = display->session()->historyEntryAtRef(historySize).result();
    QVERIFY(result.numericValue().real == HNumber(2));
    QVERIFY(result.numericValue().imag.isZero());

    editor->setText(QString());
    clickButton(QStringLiteral("7"));
    clickButton(QString::fromUtf8("x²"));
    QCOMPARE(editor->text(), QString::fromUtf8("7²"));
    clickButton(QStringLiteral("="));
    QCOMPARE(display->session()->historySize(), historySize + 2);
    const Quantity squared = display->session()->historyEntryAtRef(historySize + 1).result();
    // Complex-mode powers can leave rounding noise beyond the displayed digits.
    QVERIFY2(HMath::abs(squared.numericValue().real - HNumber(49)) < HNumber("1e-50"),
             qPrintable(HMath::format(squared.numericValue().real,
                                     HNumber::Format::Fixed() + HNumber::Format::Precision(70))));
    QVERIFY(squared.numericValue().imag.isZero());
}

void TestKeypadUi::keypad_input_stays_in_own_window_data()
{
    QTest::addColumn<int>("mode");
    QTest::newRow("basic") << int(Settings::KeypadModeBasicWide);
    QTest::newRow("scientific-wide") << int(Settings::KeypadModeScientificWide);
    QTest::newRow("scientific-narrow") << int(Settings::KeypadModeScientificNarrow);
    QTest::newRow("custom") << int(Settings::KeypadModeCustom);
}

void TestKeypadUi::keypad_input_stays_in_own_window()
{
    QFETCH(int, mode);
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;
    const auto oldCustomKeypad = settings->customKeypad;
    struct CustomKeypadGuard {
        Settings* settings;
        Settings::CustomKeypad keypad;
        ~CustomKeypadGuard() { settings->customKeypad = keypad; }
    } customGuard { settings, oldCustomKeypad };
    settings->sessionLayoutJson.clear();
    settings->windowState.clear();
    settings->windowGeometry.clear();
    settings->keypadMode = static_cast<Settings::KeypadMode>(mode);
    settings->keypadVisible = true;
    settings->hasNumberFormatStyleSetting = true;
    if (mode == Settings::KeypadModeCustom) {
        settings->customKeypad.rows = 1;
        settings->customKeypad.columns = 3;
        settings->customKeypad.buttons.clear();
        settings->customKeypad.buttons.append(
            { 0, 0, QStringLiteral("7"), QStringLiteral("7"), Settings::CustomKeypadActionInsertText });
        settings->customKeypad.buttons.append(
            { 0, 1, QString::fromUtf8("⌫"), QString(), Settings::CustomKeypadActionBackspace });
        settings->customKeypad.buttons.append(
            { 0, 2, QStringLiteral("="), QString(), Settings::CustomKeypadActionEvaluateExpression });
    }

    MainWindow firstWindow;
    firstWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&firstWindow));
    MainWindow secondWindow;
    secondWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&secondWindow));
    Editor* firstEditor = firstWindow.findChild<Editor*>();
    Editor* secondEditor = secondWindow.findChild<Editor*>();
    ResultDisplay* firstDisplay = firstWindow.findChild<ResultDisplay*>();
    ResultDisplay* secondDisplay = secondWindow.findChild<ResultDisplay*>();
    QVERIFY(firstEditor != nullptr);
    QVERIFY(secondEditor != nullptr);
    QVERIFY(firstDisplay != nullptr);
    QVERIFY(secondDisplay != nullptr);
    Keypad* firstKeypad = firstWindow.findChild<Keypad*>();
    Keypad* secondKeypad = secondWindow.findChild<Keypad*>();
    QPushButton* firstDigit = keypadButtonWithText(firstKeypad, QStringLiteral("7"));
    QPushButton* secondDigit = keypadButtonWithText(secondKeypad, QStringLiteral("7"));
    QPushButton* firstBackspace = keypadButtonWithText(firstKeypad, QString::fromUtf8("⌫"));
    QPushButton* firstEquals = keypadButtonWithText(firstKeypad, QStringLiteral("="));
    QVERIFY(firstDigit != nullptr);
    QVERIFY(secondDigit != nullptr);
    QVERIFY(firstBackspace != nullptr);
    QVERIFY(firstEquals != nullptr);

    secondWindow.activateWindow();
    secondEditor->setFocus();
    QTRY_VERIFY(secondEditor->hasFocus());
    // Let the activation focus replay finish before exercising the keypad.
    QTest::qWait(400);
    firstEditor->setText(QString());
    secondEditor->setText(QString());
    QTest::mouseClick(secondDigit, Qt::LeftButton);
    QCOMPARE(secondEditor->text(), QStringLiteral("7"));
    QCOMPARE(firstEditor->text(), QString());

    // No explicit window activation or editor focus before this click.
    QTest::mouseClick(firstDigit, Qt::LeftButton);
    QCOMPARE(firstEditor->text(), QStringLiteral("7"));
    QCOMPARE(secondEditor->text(), QStringLiteral("7"));
    QTest::mouseClick(firstBackspace, Qt::LeftButton);
    QCOMPARE(firstEditor->text(), QString());
    QCOMPARE(secondEditor->text(), QStringLiteral("7"));
    QTest::mouseClick(firstDigit, Qt::LeftButton);
    const int firstHistorySize = firstDisplay->session()->historySize();
    const int secondHistorySize = secondDisplay->session()->historySize();
    QTest::mouseClick(firstEquals, Qt::LeftButton);
    QCOMPARE(firstDisplay->session()->historySize(), firstHistorySize + 1);
    QCOMPARE(secondDisplay->session()->historySize(), secondHistorySize);
    QCOMPARE(secondEditor->text(), QStringLiteral("7"));

    QTest::mouseClick(secondDigit, Qt::LeftButton);
    QCOMPARE(secondEditor->text(), QStringLiteral("77"));
}

void TestKeypadUi::keypad_appearance_defaults_to_modern_and_persists()
{
    const auto resetSettings = qScopeGuard([]() { UiTestFixture::resetSettings(); });
    UiTestFixture::resetSettings();
    Settings* settings = Settings::instance();
    QCOMPARE(settings->keypadAppearance, Settings::KeypadAppearanceModern);
    for (const auto appearance : {Settings::KeypadAppearanceClassic, Settings::KeypadAppearanceModern}) {
        settings->keypadAppearance = appearance;
        settings->save();
        settings->keypadAppearance = appearance == Settings::KeypadAppearanceClassic
            ? Settings::KeypadAppearanceModern : Settings::KeypadAppearanceClassic;
        settings->load();
        QCOMPARE(settings->keypadAppearance, appearance);
    }
    QSettings persisted(Settings::getConfigPath() + QStringLiteral("/SpeedCrunch.ini"),
                        QSettings::IniFormat);
    persisted.setValue(QStringLiteral("SpeedCrunch/Display/KeypadAppearance"), 99);
    persisted.sync();
    settings->load();
    QCOMPARE(settings->keypadAppearance, Settings::KeypadAppearanceModern);
    settings->keypadAppearance = Settings::KeypadAppearanceClassic;
    settings->resetToDefaults();
    QCOMPARE(settings->keypadAppearance, Settings::KeypadAppearanceModern);
}

void TestKeypadUi::keypad_appearance_switches_all_windows_data()
{
    QTest::addColumn<int>("mode");
    QTest::newRow("scientific-wide") << int(Settings::KeypadModeScientificWide);
    QTest::newRow("scientific-narrow") << int(Settings::KeypadModeScientificNarrow);
    QTest::newRow("basic") << int(Settings::KeypadModeBasicWide);
    QTest::newRow("custom") << int(Settings::KeypadModeCustom);
}

void TestKeypadUi::keypad_appearance_switches_all_windows()
{
    QFETCH(int, mode);
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;
    const auto oldAppearance = settings->keypadAppearance;
    const auto oldCustomKeypad = settings->customKeypad;
    const auto restoreKeypad = qScopeGuard([=]() {
        settings->keypadAppearance = oldAppearance;
        settings->customKeypad = oldCustomKeypad;
    });
    settings->sessionLayoutJson.clear();
    settings->windowState.clear();
    settings->windowGeometry.clear();
    settings->keypadAppearance = Settings::KeypadAppearanceModern;
    settings->keypadMode = static_cast<Settings::KeypadMode>(mode);
    settings->keypadZoomPercent = 100;
    settings->colorScheme = QStringLiteral("Terminal");
    settings->customKeypad.rows = 1;
    settings->customKeypad.columns = 2;
    settings->customKeypad.buttons.clear();
    Settings::CustomKeypadButton description;
    description.label = QStringLiteral("7");
    description.text = QStringLiteral("7");
    description.action = Settings::CustomKeypadActionInsertText;
    description.row = 0;
    description.column = 0;
    settings->customKeypad.buttons.append(description);
    description.label = QStringLiteral("long custom label");
    description.column = 1;
    settings->customKeypad.buttons.append(description);

    MainWindow first;
    MainWindow second;
    first.show();
    second.show();
    QVERIFY(QTest::qWaitForWindowExposed(&first));
    QVERIFY(QTest::qWaitForWindowExposed(&second));
    QAction zoom;
    zoom.setData(150);
    QVERIFY(QMetaObject::invokeMethod(&second, "setKeypadZoom", Qt::DirectConnection,
                                      Q_ARG(QAction*, &zoom)));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
    auto* firstKeypad = first.findChild<Keypad*>();
    auto* secondKeypad = second.findChild<Keypad*>();
    QVERIFY(firstKeypad != nullptr);
    QVERIFY(secondKeypad != nullptr);
    auto* firstButton = keypadButtonWithText(firstKeypad, QStringLiteral("7"));
    auto* secondButton = keypadButtonWithText(secondKeypad, QStringLiteral("7"));
    QVERIFY(firstButton != nullptr);
    QVERIFY(secondButton != nullptr);
    const QSize firstModernSize = firstButton->size();
    const QSize secondModernSize = secondButton->size();
    const QFont firstFont = firstButton->font();
    const QColor firstColor = firstButton->palette().color(QPalette::Button);
    auto* menu = first.findChild<QMenu*>(QStringLiteral("KeypadAppearanceMenu"));
    auto* secondMenu = second.findChild<QMenu*>(QStringLiteral("KeypadAppearanceMenu"));
    QVERIFY(menu != nullptr);
    QVERIFY(secondMenu != nullptr);
    QVERIFY(menu->actions().size() >= 2);
    QVERIFY(menu->actions().at(1)->isChecked());
    QVERIFY(menu->actions().at(0)->actionGroup()->isExclusive());
    menu->actions().at(0)->trigger();
    QCoreApplication::processEvents();
    QCOMPARE(settings->keypadAppearance, Settings::KeypadAppearanceClassic);
    QVERIFY(secondMenu->actions().at(0)->isChecked());
    QVERIFY(!secondMenu->actions().at(1)->isChecked());
    QVERIFY(firstButton->height() < firstModernSize.height());
    QVERIFY(secondButton->height() < secondModernSize.height());
    QVERIFY(firstButton->width() > firstButton->height());
    QVERIFY(secondButton->height() > firstButton->height());
    QCOMPARE(firstButton->font(), firstFont);
    QCOMPARE(firstButton->palette().color(QPalette::Button), firstColor);
    const QSize classicSize = firstButton->size();
    for (QPushButton* button : firstKeypad->findChildren<QPushButton*>()) {
        if (button->isHidden())
            continue;
        QCOMPARE(button->size(), classicSize);
        QVERIFY(button->width() >= button->fontMetrics().horizontalAdvance(button->text()) + 4);
        QVERIFY(button->height() >= button->fontMetrics().lineSpacing() + 4);
    }
    for (int percent : {50, 150, 200, 100}) {
        zoom.setData(percent);
        QVERIFY(QMetaObject::invokeMethod(&first, "setKeypadZoom", Qt::DirectConnection,
                                          Q_ARG(QAction*, &zoom)));
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCoreApplication::processEvents();
        firstKeypad = first.findChild<Keypad*>();
        firstButton = keypadButtonWithText(firstKeypad, QStringLiteral("7"));
        QVERIFY(firstButton != nullptr);
        QVERIFY(firstButton->width() > firstButton->height());
        if (percent == 100)
            QCOMPARE(firstButton->size(), classicSize);
        else if (percent == 50)
            QVERIFY(firstButton->height() < classicSize.height());
        else
            QVERIFY(firstButton->height() > classicSize.height());
    }
    MainWindow third;
    auto* thirdButton = keypadButtonWithText(third.findChild<Keypad*>(), QStringLiteral("7"));
    QVERIFY(thirdButton != nullptr);
    QCOMPARE(thirdButton->size(), classicSize);
    QSignalSpy pressed(firstKeypad, mode == Settings::KeypadModeCustom
        ? SIGNAL(customButtonPressed(int,QString)) : SIGNAL(buttonPressed(Keypad::Button)));
    firstButton->click();
    QCOMPARE(pressed.count(), 1);
    menu->actions().at(1)->trigger();
    QCoreApplication::processEvents();
    QCOMPARE(firstButton->size(), firstModernSize);
    QCOMPARE(secondButton->size(), secondModernSize);
    QCOMPARE(thirdButton->size(), firstModernSize);
    QCOMPARE(firstButton->font(), firstFont);
    QVERIFY(secondMenu->actions().at(1)->isChecked());
}

void TestKeypadUi::keypad_zoom_round_trip_restores_button_sizes_data()
{
    QTest::addColumn<int>("mode");
    QTest::newRow("scientific-wide") << int(Settings::KeypadModeScientificWide);
    QTest::newRow("scientific-narrow") << int(Settings::KeypadModeScientificNarrow);
    QTest::newRow("basic") << int(Settings::KeypadModeBasicWide);
    QTest::newRow("custom") << int(Settings::KeypadModeCustom);
}

void TestKeypadUi::keypad_zoom_round_trip_restores_button_sizes()
{
    QFETCH(int, mode);
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;
    const auto oldCustomKeypad = settings->customKeypad;
    const auto restoreCustomKeypad = qScopeGuard([settings, oldCustomKeypad]() {
        settings->customKeypad = oldCustomKeypad;
    });
    settings->customKeypad.rows = 1;
    settings->customKeypad.columns = 1;
    settings->customKeypad.buttons.clear();
    Settings::CustomKeypadButton customButton;
    customButton.label = QStringLiteral("7");
    customButton.text = QStringLiteral("7");
    customButton.row = 0;
    customButton.column = 0;
    customButton.action = Settings::CustomKeypadActionInsertText;
    settings->customKeypad.buttons.append(customButton);
    settings->sessionLayoutJson.clear();
    settings->windowState.clear();
    settings->windowGeometry.clear();
    settings->keypadMode = static_cast<Settings::KeypadMode>(mode);
    settings->keypadVisible = true;
    settings->keypadZoomPercent = 100;
    settings->constantsDockVisible = false;
    settings->functionsDockVisible = false;
    settings->historyDockVisible = false;
    settings->formulaBookDockVisible = false;
    settings->variablesDockVisible = false;
    settings->userFunctionsDockVisible = false;
    settings->userUnitsDockVisible = false;
    settings->bitfieldVisible = false;

    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QCoreApplication::processEvents();
    const auto button = [&window]() {
        return keypadButtonWithText(window.findChild<Keypad*>(), QStringLiteral("7"));
    };
    QVERIFY(button() != nullptr);
    const QSize originalSize = button()->size();
    const QFont originalFont = button()->font();
    const QSize originalKeypadSize = window.findChild<Keypad*>()->size();

    for (int zoom : {150, 200, 150}) {
        QAction zoomAction;
        zoomAction.setData(zoom);
        QVERIFY(QMetaObject::invokeMethod(&window, "setKeypadZoom", Qt::DirectConnection,
                                          Q_ARG(QAction*, &zoomAction)));
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCoreApplication::processEvents();
        QVERIFY(button() != nullptr);
        QVERIFY(button()->width() > originalSize.width());
        QVERIFY(button()->font().pointSizeF() > originalFont.pointSizeF());

        zoomAction.setData(100);
        QVERIFY(QMetaObject::invokeMethod(&window, "setKeypadZoom", Qt::DirectConnection,
                                          Q_ARG(QAction*, &zoomAction)));
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCoreApplication::processEvents();
        QVERIFY(button() != nullptr);
        QCOMPARE(button()->font(), originalFont);
        QCOMPARE(button()->size(), originalSize);
        QCOMPARE(window.findChild<Keypad*>()->size(), originalKeypadSize);
    }
}

void TestKeypadUi::keypad_fifty_percent_zoom_scales_and_restores_data()
{
    QTest::addColumn<int>("mode");
    QTest::newRow("scientific-wide") << int(Settings::KeypadModeScientificWide);
    QTest::newRow("scientific-narrow") << int(Settings::KeypadModeScientificNarrow);
    QTest::newRow("basic") << int(Settings::KeypadModeBasicWide);
    QTest::newRow("custom") << int(Settings::KeypadModeCustom);
}

void TestKeypadUi::keypad_fifty_percent_zoom_scales_and_restores()
{
    QFETCH(int, mode);
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;
    settings->sessionLayoutJson.clear();
    settings->windowState.clear();
    settings->windowGeometry.clear();
    settings->keypadMode = static_cast<Settings::KeypadMode>(mode);
    settings->keypadVisible = true;
    settings->keypadZoomPercent = 100;
    settings->constantsDockVisible = false;
    settings->functionsDockVisible = false;
    settings->historyDockVisible = false;
    settings->formulaBookDockVisible = false;
    settings->variablesDockVisible = false;
    settings->userFunctionsDockVisible = false;
    settings->userUnitsDockVisible = false;
    settings->bitfieldVisible = false;

    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    window.activateWindow();
    QTRY_COMPARE(QApplication::activeWindow(), static_cast<QWidget*>(&window));
    const auto button = [&window]() {
        return keypadButtonWithText(window.findChild<Keypad*>(), QStringLiteral("7"));
    };
    QVERIFY(button() != nullptr);
    const QSize originalSize = button()->size();
    const QFont originalFont = button()->font();
    QMenu* zoomMenu = directSubmenuWithTitle(
        window.findChild<QMenu*>(QStringLiteral("KeypadAppearanceMenu")), QStringLiteral("&Zoom"));
    QVERIFY(zoomMenu != nullptr);
    const QList<QAction*> zoomActions = zoomMenu->actions();
    QVERIFY(zoomActions.size() >= 3);
    const auto flushEvents = []() {
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCoreApplication::processEvents();
    };

    zoomActions.at(0)->trigger();
    flushEvents();
    QVERIFY(button() != nullptr);
    const QSize halfSize = button()->size();
    const QFont halfFont = button()->font();
    QVERIFY(halfSize.width() < originalSize.width());
    QVERIFY(halfSize.height() < originalSize.height());
    QCOMPARE(halfFont.pointSizeF(), originalFont.pointSizeF() * 0.5);
    QVERIFY(zoomActions.at(0)->isChecked());
    QVERIFY(!zoomActions.at(1)->isChecked());
    QCOMPARE(settings->keypadZoomPercent, 50);

    zoomActions.at(2)->trigger();
    flushEvents();
    zoomActions.at(0)->trigger();
    flushEvents();
    QVERIFY(button() != nullptr);
    QCOMPARE(button()->size(), halfSize);
    QCOMPARE(button()->font(), halfFont);
    zoomActions.at(1)->trigger();
    flushEvents();
    QVERIFY(button() != nullptr);
    QCOMPARE(button()->size(), originalSize);
    QCOMPARE(button()->font(), originalFont);

    QVERIFY(QMetaObject::invokeMethod(&window, "restoreWindowKeypadZoom",
                                      Qt::DirectConnection, Q_ARG(int, 50)));
    flushEvents();
    QVERIFY(button() != nullptr);
    QCOMPARE(button()->size(), halfSize);
    QCOMPARE(button()->font(), halfFont);
    window.persistSessionAndSettingsForShutdown();
    const QJsonArray savedWindows = QJsonDocument::fromJson(settings->sessionLayoutJson.toUtf8())
                                       .object().value(QStringLiteral("windows")).toArray();
    QCOMPARE(savedWindows.size(), 1);
    QCOMPARE(savedWindows.first().toObject().value(QStringLiteral("keypadZoomPercent")).toInt(), 50);
}

void TestKeypadUi::keypad_fifty_percent_zoom_survives_settings_reload()
{
    const auto resetSettings = qScopeGuard([]() { UiTestFixture::resetSettings(); });
    UiTestFixture::resetSettings();
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;
    settings->keypadZoomPercent = 50;
    settings->keypadMode = Settings::KeypadModeBasicWide;
    settings->save();
    settings->keypadZoomPercent = 100;
    settings->load();
    QCOMPARE(settings->keypadZoomPercent, 50);

    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QMenu* zoomMenu = directSubmenuWithTitle(
        window.findChild<QMenu*>(QStringLiteral("KeypadAppearanceMenu")), QStringLiteral("&Zoom"));
    QVERIFY(zoomMenu != nullptr);
    QVERIFY(!zoomMenu->actions().isEmpty());
    QVERIFY(zoomMenu->actions().first()->isChecked());
    QCOMPARE(zoomMenu->actions().first()->data().toInt(), 50);
}

void TestKeypadUi::keypad_view_menu_tracks_and_changes_only_active_window()
{
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;

    settings->sessionLayoutJson.clear();
    settings->windowState.clear();
    settings->windowGeometry.clear();
    settings->constantsDockVisible = false;
    settings->functionsDockVisible = false;
    settings->historyDockVisible = false;
    settings->keypadMode = Settings::KeypadModeDisabled;
    settings->keypadVisible = false;
    settings->keypadZoomPercent = 100;
    settings->formulaBookDockVisible = false;
    settings->variablesDockVisible = false;
    settings->userFunctionsDockVisible = false;
    settings->userUnitsDockVisible = false;
    settings->bitfieldVisible = false;
    settings->statusBarVisible = true;
    settings->windowPositionSave = false;
    settings->hasNumberFormatStyleSetting = true;

    MainWindow firstWindow;
    firstWindow.resize(800, 500);
    firstWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&firstWindow));

    MainWindow secondWindow;
    secondWindow.resize(800, 500);
    secondWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&secondWindow));

    QVERIFY(QMetaObject::invokeMethod(
        &firstWindow, "restoreWindowKeypadLayout", Qt::DirectConnection,
        Q_ARG(bool, true), Q_ARG(int, static_cast<int>(Settings::KeypadModeBasicWide))));
    QVERIFY(QMetaObject::invokeMethod(&firstWindow,
                                      "restoreWindowKeypadZoom",
                                      Qt::DirectConnection,
                                      Q_ARG(int, 150)));
    QVERIFY(firstWindow.findChild<Keypad*>() != nullptr);
    QVERIFY(secondWindow.findChild<Keypad*>() == nullptr);

    QMenu* firstViewMenu = menuWithTitle(firstWindow.menuBar(), QStringLiteral("&View"));
    QMenu* secondViewMenu = menuWithTitle(secondWindow.menuBar(), QStringLiteral("&View"));
    QVERIFY(firstViewMenu != nullptr);
    QVERIFY(secondViewMenu != nullptr);
    QMenu* firstKeypadMenu = directSubmenuWithTitle(firstViewMenu, QStringLiteral("&Keypad"));
    QMenu* secondKeypadMenu = directSubmenuWithTitle(secondViewMenu, QStringLiteral("&Keypad"));
    QVERIFY(firstKeypadMenu != nullptr);
    QVERIFY(secondKeypadMenu != nullptr);
    QMenu* firstZoomMenu = directSubmenuWithTitle(
        firstWindow.findChild<QMenu*>(QStringLiteral("KeypadAppearanceMenu")), QStringLiteral("&Zoom"));
    QMenu* secondZoomMenu = directSubmenuWithTitle(
        secondWindow.findChild<QMenu*>(QStringLiteral("KeypadAppearanceMenu")), QStringLiteral("&Zoom"));
    QVERIFY(firstZoomMenu != nullptr);
    QVERIFY(secondZoomMenu != nullptr);
    const auto actionForMode = [](QMenu* menu, Settings::KeypadMode mode) {
        for (QAction* action : menu->actions()) {
            if (action->data().isValid()
                && action->data().toInt() == static_cast<int>(mode)) {
                return action;
            }
        }
        return static_cast<QAction*>(nullptr);
    };
    const auto checkedModeCount = [](QMenu* menu) {
        int count = 0;
        for (QAction* action : menu->actions()) {
            if (action->isCheckable() && action->data().isValid() && action->isChecked())
                ++count;
        }
        return count;
    };
    const auto actionForZoom = [](QMenu* menu, int zoomPercent) {
        for (QAction* action : menu->actions()) {
            if (action->data().isValid() && action->data().toInt() == zoomPercent)
                return action;
        }
        return static_cast<QAction*>(nullptr);
    };
    const auto checkedZoomCount = [](QMenu* menu) {
        int count = 0;
        for (QAction* action : menu->actions()) {
            if (action->isCheckable() && action->isChecked())
                ++count;
        }
        return count;
    };

    firstWindow.raise();
    firstWindow.activateWindow();
    firstWindow.setFocus(Qt::OtherFocusReason);
    QTRY_COMPARE(QApplication::activeWindow(), static_cast<QWidget*>(&firstWindow));
    QEvent firstWindowActivate(QEvent::WindowActivate);
    QCoreApplication::sendEvent(&firstWindow, &firstWindowActivate);
    QCoreApplication::processEvents();

    QAction* firstBasicAction = actionForMode(firstKeypadMenu, Settings::KeypadModeBasicWide);
    QAction* secondBasicAction = actionForMode(secondKeypadMenu, Settings::KeypadModeBasicWide);
    QAction* firstDisableAction = actionForMode(firstKeypadMenu, Settings::KeypadModeDisabled);
    QAction* secondDisableAction = actionForMode(secondKeypadMenu, Settings::KeypadModeDisabled);
    QVERIFY(firstBasicAction != nullptr);
    QVERIFY(secondBasicAction != nullptr);
    QVERIFY(firstDisableAction != nullptr);
    QVERIFY(secondDisableAction != nullptr);
    QVERIFY(firstBasicAction->isChecked());
    QVERIFY(secondBasicAction->isChecked());
    QVERIFY(!firstDisableAction->isChecked());
    QVERIFY(!secondDisableAction->isChecked());
    QCOMPARE(checkedModeCount(firstKeypadMenu), 1);
    QCOMPARE(checkedModeCount(secondKeypadMenu), 1);
    QVERIFY(firstZoomMenu->menuAction()->isEnabled());
    QVERIFY(secondZoomMenu->menuAction()->isEnabled());
    QVERIFY(actionForZoom(firstZoomMenu, 150)->isChecked());
    QVERIFY(actionForZoom(secondZoomMenu, 150)->isChecked());
    QCOMPARE(checkedZoomCount(firstZoomMenu), 1);
    QCOMPARE(checkedZoomCount(secondZoomMenu), 1);

    secondWindow.raise();
    secondWindow.activateWindow();
    secondWindow.setFocus(Qt::OtherFocusReason);
    QTRY_COMPARE(QApplication::activeWindow(), static_cast<QWidget*>(&secondWindow));
    QEvent secondWindowActivate(QEvent::WindowActivate);
    QCoreApplication::sendEvent(&secondWindow, &secondWindowActivate);
    QCoreApplication::processEvents();

    QVERIFY(!firstZoomMenu->menuAction()->isEnabled());
    QVERIFY(!secondZoomMenu->menuAction()->isEnabled());
    QVERIFY(actionForZoom(firstZoomMenu, 100)->isChecked());
    QVERIFY(actionForZoom(secondZoomMenu, 100)->isChecked());
    QCOMPARE(checkedZoomCount(firstZoomMenu), 1);
    QCOMPARE(checkedZoomCount(secondZoomMenu), 1);

    // Simulate the native menu dispatching the inactive window's actions.
    firstBasicAction->trigger();
    QCoreApplication::processEvents();
    QVERIFY(firstWindow.findChild<Keypad*>() != nullptr);
    QVERIFY(secondWindow.findChild<Keypad*>() != nullptr);
    QVERIFY(firstZoomMenu->menuAction()->isEnabled());
    QVERIFY(secondZoomMenu->menuAction()->isEnabled());

    QAction* firstZoom200Action = actionForZoom(firstZoomMenu, 200);
    QVERIFY(firstZoom200Action != nullptr);
    firstZoom200Action->trigger();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
    QVERIFY(actionForZoom(firstZoomMenu, 200)->isChecked());
    QVERIFY(actionForZoom(secondZoomMenu, 200)->isChecked());
    QCOMPARE(checkedZoomCount(firstZoomMenu), 1);
    QCOMPARE(checkedZoomCount(secondZoomMenu), 1);

    firstWindow.raise();
    firstWindow.activateWindow();
    firstWindow.setFocus(Qt::OtherFocusReason);
    QTRY_COMPARE(QApplication::activeWindow(), static_cast<QWidget*>(&firstWindow));
    QCoreApplication::sendEvent(&firstWindow, &firstWindowActivate);
    QCoreApplication::processEvents();

    QVERIFY(actionForZoom(firstZoomMenu, 150)->isChecked());
    QVERIFY(actionForZoom(secondZoomMenu, 150)->isChecked());
    QCOMPARE(checkedZoomCount(firstZoomMenu), 1);
    QCOMPARE(checkedZoomCount(secondZoomMenu), 1);

    secondDisableAction->trigger();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
    QVERIFY(firstWindow.findChild<Keypad*>() == nullptr);
    QVERIFY(secondWindow.findChild<Keypad*>() != nullptr);
    QAction* firstDisabledAction = actionForMode(firstKeypadMenu, Settings::KeypadModeDisabled);
    QAction* secondDisabledAction = actionForMode(secondKeypadMenu, Settings::KeypadModeDisabled);
    QVERIFY(firstDisabledAction != nullptr);
    QVERIFY(secondDisabledAction != nullptr);
    QVERIFY(firstDisabledAction->isChecked());
    QVERIFY(secondDisabledAction->isChecked());
    QVERIFY(!firstBasicAction->isChecked());
    QVERIFY(!secondBasicAction->isChecked());
    QCOMPARE(checkedModeCount(firstKeypadMenu), 1);
    QCOMPARE(checkedModeCount(secondKeypadMenu), 1);
    QVERIFY(!firstZoomMenu->menuAction()->isEnabled());
    QVERIFY(!secondZoomMenu->menuAction()->isEnabled());
    QVERIFY(actionForZoom(firstZoomMenu, 150)->isChecked());
    QVERIFY(actionForZoom(secondZoomMenu, 150)->isChecked());
    QCOMPARE(checkedZoomCount(firstZoomMenu), 1);
    QCOMPARE(checkedZoomCount(secondZoomMenu), 1);
}

int main(int argc, char** argv)
{
    return UiTestFixture::run<TestKeypadUi>(argc, argv);
}

#include "testkeypadui.moc"
