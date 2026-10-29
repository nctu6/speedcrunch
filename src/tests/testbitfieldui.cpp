// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mainwindowtesthelpers.h"

using namespace MainWindowTestHelpers;

class TestBitfieldUi : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init() { UiTestFixture::resetSettings(); }
    void bitfield_selected_bit_keeps_primary_fill_while_hovered();
    void system_bitfield_keeps_on_and_off_bits_distinct_data();
    void system_bitfield_keeps_on_and_off_bits_distinct();
    void system_bitfield_paints_background_data();
    void system_bitfield_paints_background();
    void system_bitfield_background_is_visible_in_window_data();
    void system_bitfield_background_is_visible_in_window();
    void bitfield_buttons_use_configured_generated_shades();
    void repeated_bit_theme_keeps_state_repainting();
    void system_bitfield_repeated_theme_refresh_keeps_current_bit_colors_data();
    void system_bitfield_repeated_theme_refresh_keeps_current_bit_colors();
};

void TestBitfieldUi::initTestCase()
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

void TestBitfieldUi::bitfield_selected_bit_keeps_primary_fill_while_hovered()
{
    const QColor background(QStringLiteral("#202124"));
    const QColor foreground(QStringLiteral("#d6d8dc"));
    const QColor hoverBackground(QStringLiteral("#4a5568"));
    const QColor hoverForeground(QStringLiteral("#f8fafc"));
    const QColor primaryBackground(QStringLiteral("#2f80ed"));
    const QColor primaryForeground(QStringLiteral("#ffffff"));
    const QColor popupBackground(QStringLiteral("#30384a"));
    const QColor popupForeground(QStringLiteral("#f4f7ff"));
    const QColor popupOutline(QStringLiteral("#596274"));

    BitWidget bit(3);
    bit.setThemeColors(background,
                       foreground,
                       hoverBackground,
                       hoverForeground,
                       primaryBackground,
                       primaryForeground);
    bit.setToolTipThemeColors(popupBackground, popupForeground, popupOutline, 8);
    bit.resize(48, 48);
    bit.show();
    QVERIFY(QTest::qWaitForWindowExposed(&bit));

    const QPoint sampledFill(4, 4);
    QTest::mouseMove(&bit, bit.rect().center());
    QTRY_VERIFY(bit.underMouse());
    QCOMPARE(bit.toolTip(), QString());
    QFrame* summaryPopup = bit.findChild<QFrame*>(QStringLiteral("bitSummaryPopup"));
    QTRY_VERIFY(summaryPopup != nullptr && summaryPopup->isVisible());
    QLabel* summaryPopupLabel =
        summaryPopup->findChild<QLabel*>(QStringLiteral("bitSummaryPopupLabel"));
    QVERIFY(summaryPopupLabel != nullptr);
    QCOMPARE(summaryPopupLabel->text(), QStringLiteral("2<sup>3</sup> = 8"));
    QVERIFY(summaryPopup->styleSheet().contains(popupBackground.name()));
    QVERIFY(summaryPopup->styleSheet().contains(popupForeground.name()));
    QVERIFY(summaryPopup->styleSheet().contains(QStringLiteral("border: %1px solid %2")
                                                    .arg(UiConfig::PopupOutlineStrokeWidth)
                                                    .arg(popupOutline.name())));
    QVERIFY(!summaryPopup->mask().isEmpty());
    QTRY_COMPARE(bit.grab().toImage().pixelColor(sampledFill).name(),
                 hoverBackground.name());

    bit.setState(true);
    QTRY_COMPARE(bit.grab().toImage().pixelColor(sampledFill).name(),
                 primaryBackground.name());
}

void TestBitfieldUi::system_bitfield_keeps_on_and_off_bits_distinct_data()
{
    QTest::addColumn<bool>("dark");
    QTest::addColumn<bool>("switchFromTheme");
    QTest::newRow("dark-system-startup") << true << false;
    QTest::newRow("light-system-startup") << false << false;
    QTest::newRow("dark-switch-from-theme") << true << true;
    QTest::newRow("light-switch-from-theme") << false << true;
}

void TestBitfieldUi::system_bitfield_keeps_on_and_off_bits_distinct()
{
    QFETCH(bool, dark);
    QFETCH(bool, switchFromTheme);
    MainWindowStateGuard guard;
    const QPalette originalPalette = QApplication::palette();
    const auto restorePalette = qScopeGuard([originalPalette]() { QApplication::setPalette(originalPalette); });
    const auto systemPalette = [originalPalette](bool dark) {
        QPalette palette = originalPalette;
        palette.setColor(QPalette::Window, QColor(dark ? "#202020" : "#eeeeee"));
        palette.setColor(QPalette::WindowText, QColor(dark ? "#ffffff" : "#000000"));
        // Some system palettes use the same neutral shade for both roles.
        palette.setColor(QPalette::Mid, palette.color(QPalette::Window));
        palette.setColor(QPalette::Button, QColor(dark ? "#383838" : "#dddddd"));
        palette.setColor(QPalette::ButtonText, palette.color(QPalette::WindowText));
        palette.setColor(QPalette::Highlight, QColor(dark ? "#246fc0" : "#185ca8"));
        palette.setColor(QPalette::HighlightedText, QColor("#ffffff"));
        return palette;
    };
    QApplication::setPalette(systemPalette(dark));
    guard.settings->dockAppearance = switchFromTheme ? Settings::DockAppearanceSpeedCrunch : Settings::DockAppearanceSystem;
    guard.settings->bitfieldVisible = true;
    guard.settings->windowState.clear();
    guard.settings->sessionLayoutJson.clear();
    MainWindow window(false);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QMenu* appearance = window.findChild<QMenu*>(QStringLiteral("DockAppearanceMenu"));
    QVERIFY(appearance);
    if (switchFromTheme)
        appearance->actions().at(0)->trigger();
    BitFieldWidget* field = window.findChild<BitFieldWidget*>();
    QVERIFY(field);
    QDockWidget* dock = window.findChild<QDockWidget*>(QStringLiteral("BitfieldDock"));
    QVERIFY(dock);
    dock->setFloating(true);
    dock->resize(700, 240);
    dock->move(20, 20);
    dock->show();
    QVERIFY(QTest::qWaitForWindowExposed(dock));
    BitWidget* onBit = nullptr;
    BitWidget* offBit = nullptr;
    for (BitWidget* bit : field->findChildren<BitWidget*>()) {
        if (bit->text() == QStringLiteral("0"))
            onBit = bit;
        if (bit->text() == QStringLiteral("1"))
            offBit = bit;
    }
    QVERIFY(onBit);
    QVERIFY(offBit);
    field->clear();
    onBit->setState(true);
    offBit->setState(false);
    const auto fill = [](BitWidget* bit) {
        const QImage image = bit->grab().toImage();
        const int inset = qRound(2 * bit->devicePixelRatioF());
        return image.pixelColor(inset, inset).rgba();
    };
    for (bool useDark : {dark, !dark}) {
        const QPalette palette = systemPalette(useDark);
        QApplication::setPalette(palette);
        QTest::mouseMove(dock->windowHandle(), QPoint(1, 1));
        QTRY_COMPARE(fill(onBit), palette.color(QPalette::Highlight).rgba());
        QTRY_COMPARE(fill(offBit), palette.color(QPalette::Window).rgba());
        QVERIFY(fill(onBit) != fill(offBit));
        QCOMPARE(onBit->palette().color(QPalette::WindowText).rgba(), palette.color(QPalette::HighlightedText).rgba());

        QTest::mouseMove(dock->windowHandle(), offBit->mapTo(dock, offBit->rect().center()));
        QTRY_VERIFY(offBit->underMouse());
        QTRY_COMPARE(fill(offBit), palette.color(QPalette::Button).rgba());
        QVERIFY(fill(onBit) != fill(offBit));
        QTest::mouseMove(dock->windowHandle(), onBit->mapTo(dock, onBit->rect().center()));
        QTRY_VERIFY(onBit->underMouse());
        QTRY_COMPARE(fill(onBit), palette.color(QPalette::Highlight).rgba());

        QSignalSpy changes(field, &BitFieldWidget::bitsChanged);
        QTest::mouseClick(onBit, Qt::LeftButton);
        QVERIFY(!onBit->state());
        QTRY_COMPARE(fill(onBit), palette.color(QPalette::Button).rgba());
        QCOMPARE(changes.count(), 1);
        QCOMPARE(changes.takeFirst().at(0).toString(), QStringLiteral("0b0"));
        QTest::mouseClick(onBit, Qt::LeftButton);
        QVERIFY(onBit->state());
        QTRY_COMPARE(fill(onBit), palette.color(QPalette::Highlight).rgba());
        QCOMPARE(changes.count(), 1);
        QCOMPARE(changes.takeFirst().at(0).toString(), QStringLiteral("0b1"));
    }
}

void TestBitfieldUi::system_bitfield_paints_background_data()
{
    QTest::addColumn<bool>("dark");
    QTest::addColumn<bool>("floating");
    QTest::addColumn<bool>("switchFromTheme");
    for (bool dark : {false, true}) {
        for (bool floating : {false, true}) {
            for (bool switchFromTheme : {false, true}) {
                const QString name = QStringLiteral("%1-%2-%3")
                    .arg(dark ? "dark" : "light", floating ? "floating" : "attached",
                         switchFromTheme ? "switch-from-theme" : "system-startup");
                QTest::newRow(qPrintable(name)) << dark << floating << switchFromTheme;
            }
        }
    }
}

void TestBitfieldUi::system_bitfield_paints_background()
{
    QFETCH(bool, dark);
    QFETCH(bool, floating);
    QFETCH(bool, switchFromTheme);
    MainWindowStateGuard guard;
    const QPalette original = QApplication::palette();
    const auto restorePalette = qScopeGuard([original]() { QApplication::setPalette(original); });
    QPalette native = original;
    native.setColor(QPalette::Window, QColor(dark ? "#202020" : "#eeeeee"));
    native.setColor(QPalette::WindowText, QColor(dark ? "#ffffff" : "#000000"));
    QApplication::setPalette(native);
    guard.settings->dockAppearance = switchFromTheme ? Settings::DockAppearanceSpeedCrunch : Settings::DockAppearanceSystem;
    guard.settings->colorScheme = QStringLiteral("Custom");
    guard.settings->customColorSchemeJson = themeJsonString(
        QJsonObject{{QStringLiteral("background"), QStringLiteral("#631840")}});
    guard.settings->bitfieldVisible = true;
    guard.settings->windowState.clear();
    guard.settings->sessionLayoutJson.clear();
    MainWindow window(false);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QDockWidget* dock = window.findChild<QDockWidget*>(QStringLiteral("BitfieldDock"));
    BitFieldWidget* field = window.findChild<BitFieldWidget*>();
    QMenu* appearance = window.findChild<QMenu*>(QStringLiteral("DockAppearanceMenu"));
    QVERIFY(dock);
    QVERIFY(field);
    QVERIFY(appearance);
    dock->setFloating(floating);
    dock->show();
    dock->raise();
    const auto background = [field]() {
        const QImage image = field->grab().toImage();
        const int inset = qRound(2 * field->devicePixelRatioF());
        return image.pixelColor(inset, inset).rgba();
    };
    if (!switchFromTheme)
        QTRY_COMPARE(background(), native.color(QPalette::Window).rgba());
    else
        QTRY_VERIFY(background() != native.color(QPalette::Window).rgba());
    for (int cycle = 0; cycle < 2; ++cycle) {
        appearance->actions().at(0)->trigger();
        QTRY_COMPARE(background(), native.color(QPalette::Window).rgba());
        native.setColor(QPalette::Window, QColor(cycle ? "#303030" : "#dddddd"));
        QApplication::setPalette(native);
        QTRY_COMPARE(background(), native.color(QPalette::Window).rgba());
        appearance->actions().at(1)->trigger();
        QTRY_VERIFY(background() != native.color(QPalette::Window).rgba());
    }
}

void TestBitfieldUi::system_bitfield_background_is_visible_in_window_data()
{
    system_bitfield_paints_background_data();
}

void TestBitfieldUi::system_bitfield_background_is_visible_in_window()
{
    QFETCH(bool, dark);
    QFETCH(bool, floating);
    QFETCH(bool, switchFromTheme);
    MainWindowStateGuard guard;
    const QPalette original = QApplication::palette();
    const auto restorePalette = qScopeGuard([original]() { QApplication::setPalette(original); });
    QPalette native = original;
    native.setColor(QPalette::Window, QColor(dark ? "#202020" : "#eeeeee"));
    native.setColor(QPalette::WindowText, QColor(dark ? "#ffffff" : "#000000"));
    QApplication::setPalette(native);
    guard.settings->dockAppearance = switchFromTheme ? Settings::DockAppearanceSpeedCrunch : Settings::DockAppearanceSystem;
    guard.settings->colorScheme = QStringLiteral("Custom");
    guard.settings->customColorSchemeJson = themeJsonString(
        QJsonObject{{QStringLiteral("background"), QStringLiteral("#062852")}});
    guard.settings->bitfieldVisible = true;
    guard.settings->constantsDockVisible = true;
    guard.settings->windowState.clear();
    guard.settings->sessionLayoutJson.clear();
    MainWindow window(false);
    window.resize(1000, 700);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QDockWidget* dock = window.findChild<QDockWidget*>(QStringLiteral("BitfieldDock"));
    BitFieldWidget* field = window.findChild<BitFieldWidget*>();
    QMenu* appearance = window.findChild<QMenu*>(QStringLiteral("DockAppearanceMenu"));
    QVERIFY(dock);
    QVERIFY(field);
    QVERIFY(appearance);
    window.addDockWidget(Qt::RightDockWidgetArea, dock);
    dock->setFloating(floating);
    dock->show();
    dock->raise();
    const auto background = [field]() {
        // Capture the parent so QWidget::grab cannot force a hidden background fill.
        QWidget* root = field->window();
        const QImage image = root->grab().toImage();
        const QPoint point = field->mapTo(root, QPoint(2, 2));
        return image.pixelColor(qRound(point.x() * root->devicePixelRatioF()),
                                qRound(point.y() * root->devicePixelRatioF())).rgba();
    };
    if (switchFromTheme)
        appearance->actions().at(0)->trigger();
    QTRY_COMPARE(background(), native.color(QPalette::Window).rgba());
    for (int cycle = 0; cycle < 2; ++cycle) {
        appearance->actions().at(1)->trigger();
        QTRY_VERIFY(background() != native.color(QPalette::Window).rgba());
        appearance->actions().at(0)->trigger();
        QTRY_COMPARE(background(), native.color(QPalette::Window).rgba());
        native.setColor(QPalette::Window, QColor(cycle ? "#303030" : "#dddddd"));
        QApplication::setPalette(native);
        QTRY_COMPARE(background(), native.color(QPalette::Window).rgba());
    }
}

void TestBitfieldUi::bitfield_buttons_use_configured_generated_shades()
{
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;
    settings->colorScheme = QStringLiteral("Custom");
    settings->customColorSchemeJson = themeJsonString(QJsonObject{{QStringLiteral("background"), QStringLiteral("#e5eee8")}});
    settings->bitfieldVisible = true;
    settings->keypadVisible = false;

    const QVector<QColor> shades =
        generateOklchShades(QColor(QStringLiteral("#e5eee8")), 6, ThemePolarity::Light);
    const QVector<QColor> foregrounds = aaForegroundsForBackgrounds(shades);
    const QColor buttonFill = shades.at(UiConfig::BitfieldButtonFillShade);
    const QColor buttonForeground = foregrounds.at(UiConfig::BitfieldButtonFillShade);
    const QColor buttonHoverFill = shades.at(UiConfig::BitfieldButtonHoverFillShade);
    const QColor buttonHoverForeground =
        foregrounds.at(UiConfig::BitfieldButtonHoverFillShade);
    const QColor buttonPressedFill = shades.at(UiConfig::BitfieldButtonPressedFillShade);
    const QColor buttonPressedForeground =
        foregrounds.at(UiConfig::BitfieldButtonPressedFillShade);
    const QColor popupBackground = shades.at(UiConfig::CompletionPopupBackgroundShade);
    const QColor popupForeground = foregrounds.at(UiConfig::CompletionPopupBackgroundShade);
    const QColor popupOutline = shades.at(UiConfig::CompletionPopupOutlineShade);

    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    BitFieldWidget* bitfield = window.findChild<BitFieldWidget*>();
    QVERIFY(bitfield != nullptr);
    const QList<QPushButton*> buttons = bitfield->findChildren<QPushButton*>();
    QCOMPARE(buttons.size(), 4);
    QPushButton* shiftLeftButton = nullptr;

    for (QPushButton* button : buttons) {
        if (button->text() == QLatin1String("<<"))
            shiftLeftButton = button;

        const QString style = button->styleSheet();
        QCOMPARE(button->toolTip(), QString());
        QCOMPARE(button->palette().color(QPalette::Button).name(), buttonFill.name());
        QCOMPARE(button->palette().color(QPalette::ButtonText).name(),
                 buttonForeground.name());
        QVERIFY(style.contains(QStringLiteral("background-color: %1")
                                   .arg(buttonFill.name())));
        QVERIFY(style.contains(QStringLiteral("color: %1").arg(buttonForeground.name())));
        QVERIFY(style.contains(QStringLiteral("background-color: %1")
                                   .arg(buttonHoverFill.name())));
        QVERIFY(style.contains(QStringLiteral("color: %1").arg(buttonHoverForeground.name())));
        QVERIFY(style.contains(QStringLiteral("background-color: %1")
                                   .arg(buttonPressedFill.name())));
        QVERIFY(style.contains(QStringLiteral("color: %1")
                                   .arg(buttonPressedForeground.name())));
    }

    QVERIFY(shiftLeftButton != nullptr);
    const QPoint buttonCenter = shiftLeftButton->rect().center();
    QMouseEvent moveEvent(QEvent::MouseMove,
                          QPointF(buttonCenter),
                          QPointF(shiftLeftButton->mapToGlobal(buttonCenter)),
                          Qt::NoButton,
                          Qt::NoButton,
                          Qt::NoModifier);
    QCoreApplication::sendEvent(shiftLeftButton, &moveEvent);
    QFrame* summaryPopup =
        bitfield->findChild<QFrame*>(QStringLiteral("bitfieldButtonSummaryPopup"));
    QTRY_VERIFY(summaryPopup != nullptr && summaryPopup->isVisible());
    QLabel* summaryPopupLabel =
        summaryPopup->findChild<QLabel*>(QStringLiteral("bitfieldButtonSummaryPopupLabel"));
    QVERIFY(summaryPopupLabel != nullptr);
    QCOMPARE(summaryPopupLabel->text(), QStringLiteral("Shift bits left"));
    QVERIFY(summaryPopup->styleSheet().contains(popupBackground.name()));
    QVERIFY(summaryPopup->styleSheet().contains(popupForeground.name()));
    QVERIFY(summaryPopup->styleSheet().contains(QStringLiteral("border: %1px solid %2")
                                                    .arg(UiConfig::PopupOutlineStrokeWidth)
                                                    .arg(popupOutline.name())));
    QVERIFY(!summaryPopup->mask().isEmpty());
}

void TestBitfieldUi::repeated_bit_theme_keeps_state_repainting()
{
    class StyleCounter : public QObject {
    public:
        int changes = 0;
        bool eventFilter(QObject*, QEvent* event) override
        {
            if (event->type() == QEvent::StyleChange)
                ++changes;
            return false;
        }
    } counter;
    const QColor background("#202124");
    const QColor selected("#2f80ed");
    BitWidget bit(3);
    const auto applyColors = [&]() {
        bit.setThemeColors(background, QColor("#d6d8dc"), background,
                           QColor("#ffffff"), selected, QColor("#ffffff"));
    };
    applyColors();
    bit.resize(48, 48);
    bit.show();
    QVERIFY(QTest::qWaitForWindowExposed(&bit));
    QCoreApplication::processEvents();
    bit.installEventFilter(&counter);
    for (int i = 0; i < 5; ++i)
        applyColors();
    QCOMPARE(counter.changes, 0);

    bit.setState(true);
    QCOMPARE(bit.grab().toImage().pixelColor(4, 4).name(), selected.name());
    applyColors();
    QCOMPARE(bit.grab().toImage().pixelColor(4, 4).name(), selected.name());
    bit.setState(false);
    QCOMPARE(bit.grab().toImage().pixelColor(4, 4).name(), background.name());
}

void TestBitfieldUi::system_bitfield_repeated_theme_refresh_keeps_current_bit_colors_data()
{
    QTest::addColumn<bool>("floating");
    QTest::newRow("attached") << false;
    QTest::newRow("floating") << true;
}

void TestBitfieldUi::system_bitfield_repeated_theme_refresh_keeps_current_bit_colors()
{
    QFETCH(bool, floating);
    MainWindowStateGuard guard;
    const QPalette original = QApplication::palette();
    const auto restorePalette = qScopeGuard([original]() { QApplication::setPalette(original); });
    guard.settings->dockAppearance = Settings::DockAppearanceSystem;
    guard.settings->bitfieldVisible = true;
    guard.settings->sessionLayoutJson.clear();
    guard.settings->windowState.clear();
    MainWindow window(false);
    window.resize(1000, 700);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto* field = window.findChild<BitFieldWidget*>();
    auto* dock = window.findChild<QDockWidget*>(QStringLiteral("BitfieldDock"));
    QVERIFY(field);
    QVERIFY(dock);
    dock->setFloating(floating);
    dock->show();
    QVERIFY(QTest::qWaitForWindowExposed(dock->window()));
    BitWidget* onBit = nullptr;
    BitWidget* offBit = nullptr;
    for (BitWidget* bit : field->findChildren<BitWidget*>()) {
        if (bit->text() == QLatin1String("0"))
            onBit = bit;
        if (bit->text() == QLatin1String("1"))
            offBit = bit;
    }
    QVERIFY(onBit);
    QVERIFY(offBit);
    onBit->setState(true);
    offBit->setState(false);
    QWidget* root = field->window();
    QTest::mouseMove(root->windowHandle(), QPoint(1, 1));
    QCoreApplication::processEvents();
    for (bool dark : {true, false, true}) {
        QPalette current = original;
        current.setColor(QPalette::Window, QColor(dark ? "#202020" : "#eeeeee"));
        current.setColor(QPalette::WindowText, dark ? Qt::white : Qt::black);
        current.setColor(QPalette::Highlight, QColor(dark ? "#246fc0" : "#185ca8"));
        current.setColor(QPalette::HighlightedText, Qt::white);
        QApplication::setPalette(current);
        QCoreApplication::processEvents();
        for (int pass = 0; pass < 3; ++pass) {
            // Reproduce the native dock restoration that follows a mode change.
            window.colorSchemeChanged();
            QCoreApplication::processEvents();
            const QImage image = root->grab().toImage();
            const auto fill = [&](BitWidget* bit) {
                const QPoint point = bit->mapTo(root, QPoint(3, 3));
                return image.pixelColor(qRound(point.x() * root->devicePixelRatioF()),
                                        qRound(point.y() * root->devicePixelRatioF()));
            };
            QCOMPARE(fill(onBit), current.color(QPalette::Highlight));
            QCOMPARE(fill(offBit), current.color(QPalette::Window));
            QCOMPARE(onBit->palette().color(QPalette::WindowText), current.color(QPalette::HighlightedText));
            QCOMPARE(offBit->palette().color(QPalette::WindowText), current.color(QPalette::WindowText));
            QVERIFY(onBit->state());
            QVERIFY(!offBit->state());
        }
    }
}

int main(int argc, char** argv)
{
    return UiTestFixture::run<TestBitfieldUi>(argc, argv);
}

#include "testbitfieldui.moc"
