// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mainwindowtesthelpers.h"

using namespace MainWindowTestHelpers;

class TestStatusBarUi : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init() { UiTestFixture::resetSettings(); }
    void status_bar_appearance_defaults_to_theme_and_persists();
    void status_bar_appearance_switches_all_windows_data();
    void status_bar_appearance_switches_all_windows();
    void status_bar_menu_tracks_and_changes_only_active_window();
    void status_bar_visibility_persists_for_every_window_during_shutdown();
    void status_bar_setting_selectors_update_only_active_window();
};

void TestStatusBarUi::initTestCase()
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

void TestStatusBarUi::status_bar_appearance_defaults_to_theme_and_persists()
{
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    Settings* settings = guard.settings;
    QCOMPARE(settings->statusBarAppearance, Settings::StatusBarAppearanceSpeedCrunch);
    const QString scheme = settings->colorScheme;
    const auto menus = settings->menuAppearance;
    const auto docks = settings->dockAppearance;
    for (const auto appearance : {Settings::StatusBarAppearanceSystem,
                                  Settings::StatusBarAppearanceSpeedCrunch}) {
        settings->statusBarAppearance = appearance;
        settings->save();
        settings->statusBarAppearance = appearance == Settings::StatusBarAppearanceSystem
            ? Settings::StatusBarAppearanceSpeedCrunch : Settings::StatusBarAppearanceSystem;
        settings->load();
        QCOMPARE(settings->statusBarAppearance, appearance);
        QCOMPARE(settings->colorScheme, scheme);
        QCOMPARE(settings->menuAppearance, menus);
        QCOMPARE(settings->dockAppearance, docks);
    }
    QSettings persisted(Settings::getConfigPath() + QStringLiteral("/SpeedCrunch.ini"),
                        QSettings::IniFormat);
    const QString key = QStringLiteral("SpeedCrunch/Display/StatusBarAppearance");
    persisted.remove(key);
    persisted.sync();
    settings->load();
    QCOMPARE(settings->statusBarAppearance, Settings::StatusBarAppearanceSpeedCrunch);
    persisted.setValue(key, 99);
    persisted.sync();
    settings->load();
    QCOMPARE(settings->statusBarAppearance, Settings::StatusBarAppearanceSpeedCrunch);
    settings->statusBarAppearance = Settings::StatusBarAppearanceSystem;
    settings->save();
    QVERIFY(settings->resetToDefaults());
    QCOMPARE(settings->statusBarAppearance, Settings::StatusBarAppearanceSpeedCrunch);
}

void TestStatusBarUi::status_bar_appearance_switches_all_windows_data()
{
    QTest::addColumn<bool>("systemAtStartup");
    QTest::addColumn<bool>("darkSystem");
    QTest::addColumn<bool>("themedMenus");
    for (bool startup : {false, true}) {
        for (bool dark : {false, true}) {
            for (bool menus : {false, true}) {
                const QString name = QStringLiteral("%1-%2-%3")
                    .arg(startup ? "system-startup" : "theme-startup",
                         dark ? "dark" : "light", menus ? "theme-menus" : "system-menus");
                QTest::newRow(qPrintable(name)) << startup << dark << menus;
            }
        }
    }
}

void TestStatusBarUi::status_bar_appearance_switches_all_windows()
{
    QFETCH(bool, systemAtStartup);
    QFETCH(bool, darkSystem);
    QFETCH(bool, themedMenus);
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    const QPalette oldPalette = QApplication::palette();
    const auto restorePalette = qScopeGuard([oldPalette]() { QApplication::setPalette(oldPalette); });
    QPalette native = oldPalette;
    native.setColor(QPalette::Window, QColor(darkSystem ? "#262626" : "#eeeeee"));
    native.setColor(QPalette::WindowText, QColor(darkSystem ? "#eeeeee" : "#252525"));
    native.setColor(QPalette::Button, native.color(QPalette::Window));
    native.setColor(QPalette::ButtonText, native.color(QPalette::WindowText));
    QApplication::setPalette(native);
    guard.settings->statusBarVisible = true;
    guard.settings->statusBarAppearance = systemAtStartup
        ? Settings::StatusBarAppearanceSystem : Settings::StatusBarAppearanceSpeedCrunch;
    guard.settings->menuAppearance = themedMenus
        ? Settings::MenuAppearanceSpeedCrunch : Settings::MenuAppearanceSystem;
    guard.settings->colorScheme = QStringLiteral("Custom");
    QColor themeBackground(QStringLiteral("#631840"));
    guard.settings->customColorSchemeJson = themeJsonString(
        QJsonObject{{QStringLiteral("background"), themeBackground.name()}});
    MainWindow first(false);
    MainWindow second(false);
    for (MainWindow* window : {&first, &second}) {
        window->resize(1000, 600);
        window->show();
        QVERIFY(QTest::qWaitForWindowExposed(window));
    }
    QMenu* appearance = first.findChild<QMenu*>(QStringLiteral("StatusBarAppearanceMenu"));
    QVERIFY(appearance);
    QVERIFY(appearance->actions().size() >= 2);
    const auto docks = guard.settings->dockAppearance;
    const auto verify = [&](MainWindow* window, bool system) {
        QMenu* menu = window->findChild<QMenu*>(QStringLiteral("StatusBarAppearanceMenu"));
        QVERIFY(menu);
        QVERIFY(menu->actions().at(system ? 0 : 1)->isChecked());
        QVERIFY(!menu->actions().at(system ? 1 : 0)->isChecked());
        QStatusBar* bar = window->findChild<QStatusBar*>(QString(), Qt::FindDirectChildrenOnly);
        QVERIFY(bar);
        QVERIFY(bar->isVisible());
        QCoreApplication::processEvents();
        const QColor background = system ? native.color(QPalette::Window)
            : generateOklchShades(themeBackground, 6, ThemePolarity::Dark)
                  .at(UiConfig::StatusBarBackgroundShade);
        QTRY_COMPARE(bar->palette().color(QPalette::Window).rgba(), background.rgba());
        QCOMPARE(bar->styleSheet().isEmpty(), system);
        QCOMPARE(bar->findChildren<QPushButton*>().size(), 3);
        for (QWidget* child : bar->findChildren<QWidget*>()) {
            if (system) {
                const QPalette expected = MenuStyle::systemPalette(child);
                for (auto group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
                    QCOMPARE(child->palette().color(group, QPalette::WindowText),
                             expected.color(group, QPalette::WindowText));
                    QCOMPARE(child->palette().color(group, QPalette::ButtonText),
                             expected.color(group, QPalette::ButtonText));
                }
            }
        }
        for (QPushButton* button : bar->findChildren<QPushButton*>()) {
            QVERIFY(button->isVisible());
            QCOMPARE(button->cursor().shape(), Qt::PointingHandCursor);
            QVERIFY(button->isFlat());
            if (!system) {
                QCOMPARE(button->palette().color(QPalette::ButtonText).name(),
                         aaForegroundForBackground(background).name());
            }
        }
        const QImage image = bar->grab().toImage();
        const QPoint sample(image.width() * 3 / 4, image.height() / 2);
        if (system) {
            QStatusBar reference;
            reference.setPalette(MenuStyle::systemPalette(&reference));
            reference.setAutoFillBackground(true);
            reference.resize(bar->size());
            const QImage expected = reference.grab().toImage();
            QCOMPARE(image.pixelColor(sample), expected.pixelColor(sample));
        } else {
            QCOMPARE(image.pixelColor(sample).rgba(), background.rgba());
        }
    };
    verify(&first, systemAtStartup);
    verify(&second, systemAtStartup);
    if (QTest::currentTestFailed())
        return;
    for (bool system : {true, false, true}) {
        appearance->actions().at(system ? 0 : 1)->trigger();
        for (MainWindow* window : {&first, &second})
            verify(window, system);
        if (QTest::currentTestFailed())
            return;
        QCOMPARE(guard.settings->dockAppearance, docks);
        QCOMPARE(guard.settings->menuAppearance,
                 themedMenus ? Settings::MenuAppearanceSpeedCrunch : Settings::MenuAppearanceSystem);
    }
    native.setColor(QPalette::Window, QColor(darkSystem ? "#353535" : "#dddddd"));
    native.setColor(QPalette::Button, native.color(QPalette::Window));
    QApplication::setPalette(native);
    verify(&first, true);
    verify(&second, true);
    if (QTest::currentTestFailed())
        return;
    themeBackground = QColor(QStringLiteral("#0a3048"));
    guard.settings->customColorSchemeJson = themeJsonString(
        QJsonObject{{QStringLiteral("background"), themeBackground.name()}});
    first.colorSchemeChanged();
    second.colorSchemeChanged();
    verify(&first, true);
    verify(&second, true);
    if (QTest::currentTestFailed())
        return;
    QVERIFY(QMetaObject::invokeMethod(&second, "setStatusBarVisible", Q_ARG(bool, false)));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(!second.findChild<QStatusBar*>(QString(), Qt::FindDirectChildrenOnly));
    appearance->actions().at(1)->trigger();
    QVERIFY(!second.findChild<QStatusBar*>(QString(), Qt::FindDirectChildrenOnly));
    QVERIFY(QMetaObject::invokeMethod(&second, "setStatusBarVisible", Q_ARG(bool, true)));
    verify(&second, false);
    if (QTest::currentTestFailed())
        return;
    appearance->actions().at(0)->trigger();
    QVERIFY(QMetaObject::invokeMethod(&second, "setStatusBarVisible", Q_ARG(bool, false)));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(QMetaObject::invokeMethod(&second, "setStatusBarVisible", Q_ARG(bool, true)));
    verify(&second, true);
    MainWindow third(false);
    third.resize(1000, 600);
    third.show();
    QVERIFY(QTest::qWaitForWindowExposed(&third));
    verify(&third, true);
    if (QTest::currentTestFailed())
        return;
    QPushButton* angleButton = nullptr;
    for (QPushButton* button : first.statusBar()->findChildren<QPushButton*>()) {
        if (button->text() == QStringLiteral("Radian"))
            angleButton = button;
    }
    QVERIFY(angleButton);
    QTest::mouseClick(angleButton, Qt::LeftButton);
    QMenu* popup = qobject_cast<QMenu*>(QApplication::activePopupWidget());
    QVERIFY(popup);
    QCOMPARE(popup->styleSheet().isEmpty(), !themedMenus);
    if (!themedMenus) {
        QCOMPARE(popup->palette().color(QPalette::Window),
                 MenuStyle::systemPalette(popup).color(QPalette::Window));
    }
    popup->hide();
}

void TestStatusBarUi::status_bar_menu_tracks_and_changes_only_active_window()
{
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;

    settings->sessionLayoutJson.clear();
    settings->windowState.clear();
    settings->windowGeometry.clear();
    settings->constantsDockVisible = false;
    settings->functionsDockVisible = false;
    settings->historyDockVisible = false;
    settings->keypadVisible = false;
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

    const auto statusBarIsVisible = [](const MainWindow& window) {
        const QStatusBar* bar =
            window.findChild<QStatusBar*>(QString(), Qt::FindDirectChildrenOnly);
        return bar != nullptr && bar->isVisible();
    };
    QVERIFY(statusBarIsVisible(firstWindow));
    QVERIFY(statusBarIsVisible(secondWindow));

    QVERIFY(QMetaObject::invokeMethod(&secondWindow,
                                      "setStatusBarVisible",
                                      Qt::DirectConnection,
                                      Q_ARG(bool, false)));
    QVERIFY(statusBarIsVisible(firstWindow));
    QVERIFY(!statusBarIsVisible(secondWindow));

    QMenu* firstViewMenu = menuWithTitle(firstWindow.menuBar(), QStringLiteral("&View"));
    QMenu* secondViewMenu = menuWithTitle(secondWindow.menuBar(), QStringLiteral("&View"));
    QVERIFY(firstViewMenu != nullptr);
    QVERIFY(secondViewMenu != nullptr);
    QAction* firstStatusBarAction =
        directMenuActionWithText(firstViewMenu, QStringLiteral("&Status Bar"));
    QAction* secondStatusBarAction =
        directMenuActionWithText(secondViewMenu, QStringLiteral("&Status Bar"));
    QVERIFY(firstStatusBarAction != nullptr);
    QVERIFY(secondStatusBarAction != nullptr);

    firstWindow.raise();
    firstWindow.activateWindow();
    firstWindow.setFocus(Qt::OtherFocusReason);
    QTRY_COMPARE(QApplication::activeWindow(), static_cast<QWidget*>(&firstWindow));
    QEvent firstWindowActivate(QEvent::WindowActivate);
    QCoreApplication::sendEvent(&firstWindow, &firstWindowActivate);
    QCoreApplication::processEvents();

    QVERIFY(firstStatusBarAction->isChecked());
    QVERIFY(secondStatusBarAction->isChecked());
    QVERIFY(statusBarIsVisible(firstWindow));
    QVERIFY(!statusBarIsVisible(secondWindow));

    // Simulate a native menu dispatch through the inactive window's QAction.
    secondStatusBarAction->trigger();
    QCoreApplication::processEvents();
    QVERIFY(!statusBarIsVisible(firstWindow));
    QVERIFY(!statusBarIsVisible(secondWindow));
    QVERIFY(!firstStatusBarAction->isChecked());
    QVERIFY(!secondStatusBarAction->isChecked());

    secondWindow.raise();
    secondWindow.activateWindow();
    secondWindow.setFocus(Qt::OtherFocusReason);
    QTRY_COMPARE(QApplication::activeWindow(), static_cast<QWidget*>(&secondWindow));
    QEvent secondWindowActivate(QEvent::WindowActivate);
    QCoreApplication::sendEvent(&secondWindow, &secondWindowActivate);
    QCoreApplication::processEvents();

    firstStatusBarAction->trigger();
    QCoreApplication::processEvents();
    QVERIFY(!statusBarIsVisible(firstWindow));
    QStatusBar* recreatedSecondStatusBar =
        secondWindow.findChild<QStatusBar*>(QString(), Qt::FindDirectChildrenOnly);
    QVERIFY(recreatedSecondStatusBar != nullptr);
    QVERIFY(recreatedSecondStatusBar->isVisible());
    QVERIFY(firstStatusBarAction->isChecked());
    QVERIFY(secondStatusBarAction->isChecked());

    firstWindow.raise();
    firstWindow.activateWindow();
    firstWindow.setFocus(Qt::OtherFocusReason);
    QTRY_COMPARE(QApplication::activeWindow(), static_cast<QWidget*>(&firstWindow));
    QCoreApplication::sendEvent(&firstWindow, &firstWindowActivate);
    QCoreApplication::processEvents();

    QVERIFY(!firstStatusBarAction->isChecked());
    QVERIFY(!secondStatusBarAction->isChecked());
    QVERIFY(!statusBarIsVisible(firstWindow));
    QVERIFY(statusBarIsVisible(secondWindow));
}

void TestStatusBarUi::status_bar_visibility_persists_for_every_window_during_shutdown()
{
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;

    settings->sessionLayoutJson.clear();
    settings->windowState.clear();
    settings->windowGeometry.clear();
    settings->constantsDockVisible = false;
    settings->functionsDockVisible = false;
    settings->historyDockVisible = false;
    settings->keypadVisible = false;
    settings->formulaBookDockVisible = false;
    settings->variablesDockVisible = false;
    settings->userFunctionsDockVisible = false;
    settings->userUnitsDockVisible = false;
    settings->bitfieldVisible = false;
    settings->statusBarVisible = true;
    settings->angleUnit = 'd';
    settings->resultFormat = 'g';
    settings->resultPrecision = -1;
    settings->windowPositionSave = false;
    settings->hasNumberFormatStyleSetting = true;

    const auto statusBarIsVisible = [](const MainWindow* window) {
        const QStatusBar* bar = window != nullptr
            ? window->findChild<QStatusBar*>(QString(), Qt::FindDirectChildrenOnly)
            : nullptr;
        return bar != nullptr && bar->isVisible();
    };
    const auto selectorText = [](const MainWindow* window, const QString& labelText) {
        if (window == nullptr)
            return QString();
        for (QLabel* label : window->findChildren<QLabel*>()) {
            if (label->text() != labelText)
                continue;
            QPushButton* button = label->parentWidget()->findChild<QPushButton*>(
                QString(), Qt::FindDirectChildrenOnly);
            return button != nullptr ? button->text() : QString();
        }
        return QString();
    };
    {
        MainWindow firstWindow;
        firstWindow.resize(800, 500);
        firstWindow.show();
        QVERIFY(QTest::qWaitForWindowExposed(&firstWindow));

        MainWindow secondWindow(false);
        secondWindow.resize(800, 500);
        secondWindow.show();
        QVERIFY(QTest::qWaitForWindowExposed(&secondWindow));

        MainWindow thirdWindow(false);
        thirdWindow.resize(800, 500);
        thirdWindow.show();
        QVERIFY(QTest::qWaitForWindowExposed(&thirdWindow));

        QVERIFY(QMetaObject::invokeMethod(&secondWindow,
                                          "setStatusBarVisible",
                                          Qt::DirectConnection,
                                          Q_ARG(bool, false)));
        QVERIFY(statusBarIsVisible(&firstWindow));
        QVERIFY(!statusBarIsVisible(&secondWindow));
        QVERIFY(statusBarIsVisible(&thirdWindow));
        QVERIFY(settings->statusBarVisible);

        QVERIFY(QMetaObject::invokeMethod(&secondWindow,
                                          "setStatusBarVisible",
                                          Qt::DirectConnection,
                                          Q_ARG(bool, true)));
        QTRY_VERIFY(statusBarIsVisible(&secondWindow));

        secondWindow.raise();
        secondWindow.activateWindow();
        secondWindow.setFocus(Qt::OtherFocusReason);
        QTRY_COMPARE(QApplication::activeWindow(), static_cast<QWidget*>(&secondWindow));
        QVERIFY(QMetaObject::invokeMethod(&secondWindow,
                                          "setAngleModeRadian",
                                          Qt::DirectConnection));
        QVERIFY(QMetaObject::invokeMethod(&secondWindow,
                                          "setResultFormatScientific",
                                          Qt::DirectConnection));
        QVERIFY(QMetaObject::invokeMethod(&secondWindow,
                                          "setResultPrecision3Digits",
                                          Qt::DirectConnection));
        QCOMPARE(selectorText(&firstWindow, QStringLiteral("Angle Mode:")),
                 QStringLiteral("Degree"));

        firstWindow.raise();
        firstWindow.activateWindow();
        firstWindow.setFocus(Qt::OtherFocusReason);
        QTRY_COMPARE(QApplication::activeWindow(), static_cast<QWidget*>(&firstWindow));

        // During application teardown, secondary top-level windows can already
        // be hidden when the primary window persists the layout. Their status
        // bars are still configured as shown and must be saved that way.
        secondWindow.hide();
        thirdWindow.hide();
        firstWindow.persistSessionAndSettingsForShutdown();

        const QJsonDocument savedLayout =
            QJsonDocument::fromJson(settings->sessionLayoutJson.toUtf8());
        QVERIFY(savedLayout.isObject());
        const QJsonArray savedWindows =
            savedLayout.object().value(QStringLiteral("windows")).toArray();
        QCOMPARE(savedWindows.size(), 3);

        int visibleStatusBars = 0;
        int defaultStatusBarSelections = 0;
        int changedStatusBarSelections = 0;
        for (const QJsonValue& value : savedWindows) {
            const QJsonObject savedWindow = value.toObject();
            if (savedWindow.value(QStringLiteral("statusBarVisible")).toBool(false))
                ++visibleStatusBars;
            const QString angleUnit =
                savedWindow.value(QStringLiteral("statusBarAngleUnit")).toString();
            const QString resultFormat =
                savedWindow.value(QStringLiteral("statusBarResultFormat")).toString();
            const int resultPrecision =
                savedWindow.value(QStringLiteral("statusBarResultPrecision")).toInt();
            if (angleUnit == QLatin1String("d")
                && resultFormat == QLatin1String("g")
                && resultPrecision == -1) {
                ++defaultStatusBarSelections;
            } else if (angleUnit == QLatin1String("r")
                       && resultFormat == QLatin1String("e")
                       && resultPrecision == 3) {
                ++changedStatusBarSelections;
            }
        }
        QCOMPARE(visibleStatusBars, 3);
        QCOMPARE(defaultStatusBarSelections, 2);
        QCOMPARE(changedStatusBarSelections, 1);
    }

    QCOMPARE(QJsonDocument::fromJson(settings->sessionLayoutJson.toUtf8())
                 .object()
                 .value(QStringLiteral("windows"))
                 .toArray()
                 .size(),
             3);
}

void TestStatusBarUi::status_bar_setting_selectors_update_only_active_window()
{
    MainWindowStateGuard guard;
    Settings* settings = guard.settings;

    settings->sessionLayoutJson.clear();
    settings->windowState.clear();
    settings->windowGeometry.clear();
    settings->constantsDockVisible = false;
    settings->functionsDockVisible = false;
    settings->historyDockVisible = false;
    settings->keypadVisible = false;
    settings->formulaBookDockVisible = false;
    settings->variablesDockVisible = false;
    settings->userFunctionsDockVisible = false;
    settings->userUnitsDockVisible = false;
    settings->bitfieldVisible = false;
    settings->statusBarVisible = true;
    settings->angleUnit = 'd';
    settings->resultFormat = 'g';
    settings->resultPrecision = -1;
    settings->windowPositionSave = false;
    settings->hasNumberFormatStyleSetting = true;

    MainWindow firstWindow;
    firstWindow.resize(900, 500);
    firstWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&firstWindow));

    MainWindow secondWindow;
    secondWindow.resize(900, 500);
    secondWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&secondWindow));

    const auto selectorButton = [](const MainWindow& window, const QString& labelText) {
        for (QLabel* label : window.findChildren<QLabel*>()) {
            if (label->text() == labelText) {
                return label->parentWidget()->findChild<QPushButton*>(
                    QString(), Qt::FindDirectChildrenOnly);
            }
        }
        return static_cast<QPushButton*>(nullptr);
    };
    QPushButton* firstAngle = selectorButton(firstWindow, QStringLiteral("Angle Mode:"));
    QPushButton* firstNotation = selectorButton(firstWindow, QStringLiteral("Notation:"));
    QPushButton* firstPrecision = selectorButton(firstWindow, QStringLiteral("Precision:"));
    QPushButton* secondAngle = selectorButton(secondWindow, QStringLiteral("Angle Mode:"));
    QPushButton* secondNotation = selectorButton(secondWindow, QStringLiteral("Notation:"));
    QPushButton* secondPrecision = selectorButton(secondWindow, QStringLiteral("Precision:"));
    QVERIFY(firstAngle != nullptr);
    QVERIFY(firstNotation != nullptr);
    QVERIFY(firstPrecision != nullptr);
    QVERIFY(secondAngle != nullptr);
    QVERIFY(secondNotation != nullptr);
    QVERIFY(secondPrecision != nullptr);
    QCOMPARE(firstAngle->text(), QStringLiteral("Degree"));
    QCOMPARE(firstNotation->text(), QStringLiteral("Automatic decimal"));
    QCOMPARE(firstPrecision->text(), QStringLiteral("Automatic"));
    QCOMPARE(secondAngle->text(), firstAngle->text());
    QCOMPARE(secondNotation->text(), firstNotation->text());
    QCOMPARE(secondPrecision->text(), firstPrecision->text());

    firstWindow.raise();
    firstWindow.activateWindow();
    firstWindow.setFocus(Qt::OtherFocusReason);
    QTRY_COMPARE(QApplication::activeWindow(), static_cast<QWidget*>(&firstWindow));

    QMenu* secondSettingsMenu =
        menuWithTitle(secondWindow.menuBar(), QStringLiteral("Se&ttings"));
    QVERIFY(secondSettingsMenu != nullptr);
    QMenu* secondAngleMenu =
        directSubmenuWithTitle(secondSettingsMenu, QStringLiteral("Angle &Mode"));
    QVERIFY(secondAngleMenu != nullptr);
    QAction* staleRadianAction =
        directMenuActionWithText(secondAngleMenu, QStringLiteral("&Radian"));
    QVERIFY(staleRadianAction != nullptr);

    QMenu* secondResultsMenu =
        directSubmenuWithTitle(secondSettingsMenu, QStringLiteral("&Results"));
    QVERIFY(secondResultsMenu != nullptr);
    QAction* staleResultSlotsAction =
        directMenuActionWithText(secondResultsMenu,
                                 QStringLiteral("Notation && Precision..."));
    QVERIFY(staleResultSlotsAction != nullptr);

    // Simulate native-menu dispatch through actions owned by the inactive window.
    staleRadianAction->trigger();
    QCoreApplication::processEvents();
    QCOMPARE(firstAngle->text(), QStringLiteral("Radian"));
    QCOMPARE(secondAngle->text(), QStringLiteral("Degree"));

    bool configuredActiveWindowDialog = false;
    QTimer::singleShot(0, &firstWindow, [&]() {
        ResultSlotsDialog* dialog =
            qobject_cast<ResultSlotsDialog*>(QApplication::activeModalWidget());
        if (dialog == nullptr || dialog->parentWidget() != &firstWindow)
            return;

        const QList<QComboBox*> notationCombos = dialog->findChildren<QComboBox*>();
        const QList<QCheckBox*> checkBoxes = dialog->findChildren<QCheckBox*>();
        const QList<QSpinBox*> precisionSpins = dialog->findChildren<QSpinBox*>();
        QDialogButtonBox* buttons = dialog->findChild<QDialogButtonBox*>();
        if (notationCombos.isEmpty() || precisionSpins.isEmpty() || buttons == nullptr)
            return;

        QCheckBox* mainAutoPrecision = nullptr;
        for (QCheckBox* checkBox : checkBoxes) {
            if (checkBox->text() == QStringLiteral("Auto")) {
                mainAutoPrecision = checkBox;
                break;
            }
        }
        if (mainAutoPrecision == nullptr)
            return;

        const int scientificIndex =
            notationCombos.constFirst()->findData(QStringLiteral("e"));
        if (scientificIndex < 0)
            return;
        notationCombos.constFirst()->setCurrentIndex(scientificIndex);
        mainAutoPrecision->setChecked(false);
        precisionSpins.constFirst()->setValue(3);
        configuredActiveWindowDialog = true;
        buttons->button(QDialogButtonBox::Ok)->click();
    });
    staleResultSlotsAction->trigger();
    QVERIFY(configuredActiveWindowDialog);

    QCOMPARE(firstNotation->text(), QStringLiteral("Scientific decimal"));
    QCOMPARE(secondNotation->text(), QStringLiteral("Automatic decimal"));
    QCOMPARE(firstPrecision->text(), QStringLiteral("3"));
    QCOMPARE(secondPrecision->text(), QStringLiteral("Automatic"));
}

int main(int argc, char** argv)
{
    return UiTestFixture::run<TestStatusBarUi>(argc, argv);
}

#include "teststatusbarui.moc"
