// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mainwindowtesthelpers.h"
#include "core/startupdiagnostics.h"

using namespace MainWindowTestHelpers;

class TestThemesUi : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init() { UiTestFixture::resetSettings(); }
    void startup_styles_parent_surfaces_before_controls_data();
    void startup_styles_parent_surfaces_before_controls();
    void repeated_pane_theme_refresh_avoids_repolishing_data();
    void repeated_pane_theme_refresh_avoids_repolishing();
    void color_scheme_roles_exclude_obsolete_scrollbar();
    void color_scheme_reads_optional_display_name();
    void color_scheme_preserves_rose_pine_names_with_ascii_resources();
    void color_scheme_validates_schema_metadata();
    void theme_dialog_preserves_list_scroll_and_fills_role_color_buttons_data();
    void theme_dialog_preserves_list_scroll_and_fills_role_color_buttons();
    void theme_dialog_repeatedly_previews_same_light_and_dark_themes_data();
    void theme_dialog_repeatedly_previews_same_light_and_dark_themes();
    void main_window_uses_generated_theme_surface_for_chrome_and_editor_data();
    void main_window_uses_generated_theme_surface_for_chrome_and_editor();
    void restored_session_layout_reapplies_generated_theme_surfaces();
    void dock_surfaces_use_successive_generated_shades();
    void repeated_theme_refresh_keeps_structural_styles();
    void generated_surface_cache_tracks_custom_color_edits();
    void dock_and_popup_text_meet_contrast_target_across_themes();
    void constant_popup_clear_button_matches_dock_search_data();
    void constant_popup_clear_button_matches_dock_search();
};

void TestThemesUi::startup_styles_parent_surfaces_before_controls_data()
{
    QTest::addColumn<bool>("restore");
    QTest::addColumn<bool>("classic");
    QTest::addColumn<QString>("background");
    for (bool restore : {false, true}) {
        for (bool classic : {false, true}) {
            for (const QString& background : {QStringLiteral("#182332"), QStringLiteral("#f2e4c8")}) {
                QTest::addRow("restore-%d-classic-%d-%s", restore, classic, qPrintable(background))
                    << restore << classic << background;
            }
        }
    }
}

void TestThemesUi::startup_styles_parent_surfaces_before_controls()
{
    QFETCH(bool, restore);
    QFETCH(bool, classic);
    QFETCH(QString, background);
    MainWindowStateGuard guard;
    PrivateSessionStorage storage;
    guard.settings->colorScheme = QStringLiteral("Custom");
    guard.settings->customColorSchemeJson = themeJsonString(QJsonObject {
        {QStringLiteral("background"), background}
    });
    guard.settings->editorDisplayStyle = classic
        ? Settings::EditorDisplayClassic : Settings::EditorDisplayModern;
    guard.settings->keypadMode = Settings::KeypadModeDisabled;
    guard.settings->sessionLayoutJson.clear();
    const QString name = QStringLiteral("ParentStyles");
    if (restore) {
        QVERIFY(QDir().mkpath(QFileInfo(storage.path(name)).path()));
        writeFile(storage.path(name), QJsonDocument(sessionJson(name)).toJson());
        const QJsonObject layout {
            {QStringLiteral("scheme"), 1},
            {QStringLiteral("kind"), QStringLiteral("session-layout")},
            {QStringLiteral("windows"), QJsonArray {startupSessionWindow(name, QStringLiteral("styles"))}}
        };
        guard.settings->sessionLayoutJson = QString::fromUtf8(QJsonDocument(layout).toJson());
    }
    MainWindow::initializeApplicationStyle();
    StartupDiagnostics::begin();
    const auto finishRecording = qScopeGuard([] {
        StartupDiagnostics::firstPaintFinished();
        StartupDiagnostics::finishWhenIdle();
    });
    MainWindow window(restore);
    window.showWhenReady();
    QTRY_VERIFY(window.isVisible());
    auto* display = window.findChild<ResultDisplay*>();
    QVERIFY(display);
    if (restore)
        QCOMPARE(display->session()->name(), name);
    QWidget* pane = paneWidgetForDisplay(display);
    QVERIFY(pane);
    const auto snapshot = StartupDiagnostics::snapshot();
    qint64 firstControlStart = -1;
    for (const auto& phase : snapshot.phases) {
        if (phase.name == QLatin1String("Construct initial result display"))
            firstControlStart = phase.firstStartNs;
    }
    QVERIFY(firstControlStart >= 0);
    const QList<QWidget*> parents {&window, window.centralWidget(), pane};
    for (QWidget* parent : parents) {
        const int id = StartupDiagnostics::appearanceObjectId(parent);
        int changes = 0;
        for (const auto& assignment : snapshot.appearanceAssignments) {
            if (assignment.palette || assignment.widgetId != id || assignment.changed == 0)
                continue;
            changes += assignment.changed;
            if (parent != pane) {
                const auto& call = snapshot.appearanceCalls.at(assignment.call - 1);
                QVERIFY(call.startNs < firstControlStart);
            }
        }
        // Restoration and the final theme pass must reuse the installed styles.
        QCOMPARE(changes, 1);
    }
}

void TestThemesUi::repeated_pane_theme_refresh_avoids_repolishing_data()
{
    QTest::addColumn<bool>("classic");
    QTest::newRow("modern") << false;
    QTest::newRow("classic") << true;
}

void TestThemesUi::repeated_pane_theme_refresh_avoids_repolishing()
{
    MainWindowStateGuard guard;
    QFETCH(bool, classic);
    guard.settings->colorScheme = QStringLiteral("Custom");
    guard.settings->customColorSchemeJson = themeJsonString(QJsonObject {
        {QStringLiteral("background"), QStringLiteral("#182332")}
    });
    guard.settings->editorDisplayStyle = classic
        ? Settings::EditorDisplayClassic : Settings::EditorDisplayModern;
    guard.settings->keypadMode = Settings::KeypadModeDisabled;
    MainWindow window(false);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QCoreApplication::processEvents();
    auto* display = window.findChild<ResultDisplay*>();
    Editor* editor = editorForDisplay(display);
    QVERIFY(editor);
    editor->setText(QStringLiteral("12+34"));
    editor->selectAll();
    QWidget* pane = paneWidgetForDisplay(display);
    QVERIFY(pane);
    auto* tabs = pane->findChild<QTabBar*>();
    QVERIFY(tabs);
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
    const QList<QWidget*> widgets {&window, window.centralWidget(), pane,
                                   tabs->parentWidget(), tabs, display, editor};
    QStringList styles;
    for (QWidget* widget : widgets) {
        widget->installEventFilter(&counter);
        styles.append(widget->styleSheet());
    }
    for (int i = 0; i < 3; ++i)
        window.colorSchemeChanged();
    QCoreApplication::processEvents();
    QCOMPARE(counter.changes, 0);
    for (int i = 0; i < widgets.size(); ++i)
        QCOMPARE(widgets.at(i)->styleSheet(), styles.at(i));

    const QColor background(QStringLiteral("#f2e4c8"));
    guard.settings->customColorSchemeJson = themeJsonString(QJsonObject {
        {QStringLiteral("background"), background.name()}
    });
    window.colorSchemeChanged();
    QCoreApplication::processEvents();
    QVERIFY(counter.changes > 0);
    const auto shades = generateOklchShades(background, UiConfig::Shade600 + 1,
                                          themePolarityForBackground(background));
    QCOMPARE(pane->palette().color(QPalette::Window).name(),
             shades.at(UiConfig::ResultDisplayShade).name());
    QCOMPARE(tabs->parentWidget()->palette().color(QPalette::Window).name(),
             shades.at(UiConfig::WindowBackgroundShade).name());
    QCOMPARE(display->viewport()->palette().color(QPalette::Base).name(),
             shades.at(UiConfig::ResultDisplayShade).name());
    QCOMPARE(editor->textCursor().selectedText(), QStringLiteral("12+34"));
}

void TestThemesUi::constant_popup_clear_button_matches_dock_search_data()
{
    QTest::addColumn<QString>("background");
    QTest::newRow("dark") << QStringLiteral("#454257");
    QTest::newRow("light") << QStringLiteral("#f4e8ee");
}

void TestThemesUi::constant_popup_clear_button_matches_dock_search()
{
    QFETCH(QString, background);
    MainWindowStateGuard guard;
    guard.settings->colorScheme = QStringLiteral("Custom");
    guard.settings->constantsDockVisible = true;
    guard.settings->dockAppearance = Settings::DockAppearanceSpeedCrunch;
    guard.settings->windowState.clear();
    guard.settings->sessionLayoutJson.clear();
    guard.settings->customColorSchemeJson = themeJsonString(QJsonObject{
        {QStringLiteral("background"), background},
        {QStringLiteral("primary"), QStringLiteral("#8cb4e8")}
    });
    MainWindow window(false);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto* editor = window.findChild<Editor*>();
    auto* dock = window.findChild<QDockWidget*>(QStringLiteral("ConstantsDock"));
    QVERIFY(editor && dock);
    dock->show();
    dock->raise();
    auto* dockSearch = dock->findChild<QLineEdit*>();
    QVERIFY(dockSearch);
    dockSearch->setText(QStringLiteral("electron"));
    auto* dockButton = dockSearch->findChild<QToolButton*>();
    QVERIFY(dockButton);
    editor->setText(QStringLiteral("12+34"));

    for (const QString& primary : {QStringLiteral("#8cb4e8"), QStringLiteral("#e7a840")}) {
        guard.settings->customColorSchemeJson = themeJsonString(QJsonObject{
            {QStringLiteral("background"), background},
            {QStringLiteral("primary"), primary}
        });
        window.colorSchemeChanged();
        dockSearch->setFocus();
        QTRY_VERIFY(dockSearch->hasFocus());
        const QIcon focusedIcon = dockButton->icon();
        QCOMPARE(focusedIcon.pixmap(QSize(16, 16), 1.0).toImage().pixelColor(8, 3), QColor(primary));
        editor->setFocus();
        QTRY_VERIFY(!dockSearch->hasFocus());
        const QIcon unfocusedIcon = dockButton->icon();
        QTest::keyClick(editor, Qt::Key_Space, Qt::ControlModifier);
        QPointer<QWidget> popup = QApplication::activePopupWidget();
        QVERIFY(popup);
        auto* search = popup->findChild<QLineEdit*>(QStringLiteral("constantCompletionSearch"));
        auto* list = popup->findChild<QTreeWidget*>();
        QVERIFY(search && list);
        QTRY_VERIFY(search->hasFocus());
        search->setText(QStringLiteral("no matching constant 987654321"));
        auto* button = search->findChild<QToolButton*>();
        QVERIFY(button && button->isVisible());
        for (qreal scale : {1.0, 2.0, 3.0}) {
            QCOMPARE(button->icon().pixmap(QSize(16, 16), scale).toImage(),
                     focusedIcon.pixmap(QSize(16, 16), scale).toImage());
        }
        // Moving focus within the popup uses the dock's unfocused shade.
        list->setFocus();
        QTRY_VERIFY(!search->hasFocus());
        QCOMPARE(button->icon().pixmap(16, 16).toImage(), unfocusedIcon.pixmap(16, 16).toImage());
        search->setFocus();
        QTRY_VERIFY(search->hasFocus());
        QCOMPARE(button->icon().pixmap(16, 16).toImage(), focusedIcon.pixmap(16, 16).toImage());
        guard.settings->customColorSchemeJson = themeJsonString(QJsonObject{
            {QStringLiteral("background"), background},
            {QStringLiteral("primary"), QStringLiteral("#4caf87")}
        });
        window.colorSchemeChanged();
        QCOMPARE(button->icon().pixmap(QSize(16, 16), 1.0).toImage().pixelColor(8, 3),
                 QColor(QStringLiteral("#4caf87")));
        QTest::mouseClick(button, Qt::LeftButton);
        QVERIFY(search->text().isEmpty());
        QVERIFY(list->topLevelItemCount() > 0);
        QCOMPARE(editor->text(), QStringLiteral("12+34"));
        QTest::keyClick(search, Qt::Key_Escape);
        QVERIFY(!popup || !popup->isVisible());
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
}

void TestThemesUi::generated_surface_cache_tracks_custom_color_edits()
{
    MainWindowStateGuard guard;
    guard.settings->colorScheme = QStringLiteral("Custom");
    guard.settings->customColorSchemeJson = themeJsonString(QJsonObject{
        {QStringLiteral("background"), QStringLiteral("#182332")},
        {QStringLiteral("primary"), QStringLiteral("#e7a840")}
    });
    MainWindow window(false);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    Editor* editor = window.findChild<Editor*>();
    ResultDisplay* display = window.findChild<ResultDisplay*>();
    QVERIFY(editor);
    QVERIFY(display);
    const auto refresh = [&]() {
        QVERIFY(QMetaObject::invokeMethod(&window, "colorSchemeChanged", Qt::DirectConnection));
        QCoreApplication::processEvents();
    };
    const auto verify = [&](const QColor& background) {
        const auto shades = generateOklchShades(background, UiConfig::Shade600 + 1,
                                              themePolarityForBackground(background));
        QVERIFY(colorsAreClose(window.palette().color(QPalette::Window),
                               shades.at(UiConfig::WindowBackgroundShade), 1));
        QVERIFY(colorsAreClose(display->palette().color(QPalette::Base),
                               shades.at(UiConfig::ResultDisplayShade), 1));
        QVERIFY(colorsAreClose(editor->palette().color(QPalette::Base),
                               shades.at(UiConfig::DockBackgroundShade), 1));
    };
    verify(QColor("#182332"));
    const QColor originalPrimary = editor->palette().color(QPalette::Text);
    guard.settings->customColorSchemeJson = themeJsonString(QJsonObject{
        {QStringLiteral("background"), QStringLiteral("#f2e4c8")},
        {QStringLiteral("primary"), QStringLiteral("#1258ad")}
    });
    refresh();
    verify(QColor("#f2e4c8"));
    QVERIFY(editor->palette().color(QPalette::Text) != originalPrimary);
    guard.settings->customColorSchemeJson = themeJsonString(QJsonObject{
        {QStringLiteral("background"), QStringLiteral("#f2e4c8")},
        {QStringLiteral("primary"), QStringLiteral("#ad2258")}
    });
    refresh();
    verify(QColor("#f2e4c8"));
    QCOMPARE(editor->palette().color(QPalette::Text), QColor("#ad2258"));
    guard.settings->customColorSchemeJson = themeJsonString(QJsonObject{
        {QStringLiteral("background"), QStringLiteral("#182332")}
    });
    refresh();
    verify(QColor("#182332"));
    const QColor generatedPrimary = generatePrimaryFromBackground(QColor("#182332"));
    QVERIFY(colorsAreClose(editor->palette().color(QPalette::Text), generatedPrimary, 1));
}

void TestThemesUi::initTestCase()
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

void TestThemesUi::color_scheme_roles_exclude_obsolete_scrollbar()
{
    const auto roles = ColorScheme::roleNames();
    bool hasPrimaryRole = false;
    for (const auto& roleEntry : roles) {
        hasPrimaryRole = hasPrimaryRole || roleEntry.first == QStringLiteral("primary");
        QVERIFY(roleEntry.first != QStringLiteral("scrollbar"));
        QVERIFY(roleEntry.first != QStringLiteral("cursor"));
        QVERIFY(roleEntry.first != QStringLiteral("matched"));
    }
    QVERIFY(hasPrimaryRole);

    const ColorScheme scheme = ColorScheme::fromJsonObject(themeJson(QJsonObject{
        {QStringLiteral("background"), QStringLiteral("#1f3229")},
        {QStringLiteral("cursor"), QStringLiteral("#ffff00")},
        {QStringLiteral("scrollbar"), QStringLiteral("#ff00ff")},
        {QStringLiteral("matched"), QStringLiteral("#00ffff")}
    }));
    QVERIFY(scheme.isValid());
    const QJsonObject schemeJson = scheme.toJsonObject();
    QCOMPARE(schemeJson.value(QStringLiteral("$schema")).toString(),
             QString::fromLatin1(ColorScheme::SchemaDraft));
    QCOMPARE(schemeJson.value(QStringLiteral("$id")).toString(),
             QString::fromLatin1(ColorScheme::SchemaId));
    QVERIFY(!schemeJson.contains(QStringLiteral("version")));
    QVERIFY(!schemeJson.contains(QStringLiteral("scheme")));
    QVERIFY(!scheme.hasColorForRole(ColorScheme::Primary));
    QVERIFY(!schemeJson.contains(QStringLiteral("scrollbar")));
    QVERIFY(!schemeJson.contains(QStringLiteral("cursor")));
    QVERIFY(!schemeJson.contains(QStringLiteral("matched")));
    QVERIFY(!schemeJson.contains(QStringLiteral("primary")));

    const ColorScheme schemeWithPrimary = ColorScheme::fromJsonObject(themeJson(QJsonObject{
        {QStringLiteral("background"), QStringLiteral("#1f3229")},
        {QStringLiteral("primary"), QStringLiteral("#abcdef")}
    }));
    QVERIFY(schemeWithPrimary.isValid());
    QVERIFY(schemeWithPrimary.hasColorForRole(ColorScheme::Primary));
    QCOMPARE(schemeWithPrimary.colorForRole(ColorScheme::Primary).name(),
             QStringLiteral("#abcdef"));
    QCOMPARE(schemeWithPrimary.toJsonObject().value(QStringLiteral("primary")).toString(),
             QStringLiteral("#abcdef"));
}

void TestThemesUi::color_scheme_reads_optional_display_name()
{
    const ColorScheme namedScheme = ColorScheme::fromJsonObject(themeJson(QJsonObject{
        {QStringLiteral("name"), QStringLiteral("  Named Theme  ")},
        {QStringLiteral("background"), QStringLiteral("#1f3229")}
    }));
    QVERIFY(namedScheme.isValid());
    QCOMPARE(namedScheme.displayName(), QStringLiteral("Named Theme"));
    QCOMPARE(namedScheme.toJsonObject().value(QStringLiteral("name")).toString(),
             QStringLiteral("Named Theme"));

    const ColorScheme blankNameScheme = ColorScheme::fromJsonObject(themeJson(QJsonObject{
        {QStringLiteral("name"), QStringLiteral("   ")},
        {QStringLiteral("background"), QStringLiteral("#1f3229")}
    }));
    QVERIFY(blankNameScheme.isValid());
    QVERIFY(blankNameScheme.displayName().isEmpty());
    QVERIFY(!blankNameScheme.toJsonObject().contains(QStringLiteral("name")));

    const ColorScheme nonStringNameScheme = ColorScheme::fromJsonObject(themeJson(QJsonObject{
        {QStringLiteral("name"), 1},
        {QStringLiteral("background"), QStringLiteral("#1f3229")}
    }));
    QVERIFY(nonStringNameScheme.isValid());
    QVERIFY(nonStringNameScheme.displayName().isEmpty());
}

void TestThemesUi::color_scheme_preserves_rose_pine_names_with_ascii_resources()
{
    const QStringList schemeNames = ColorScheme::enumerate();
    for (const QString& variant : {QStringLiteral("Moon"), QStringLiteral("Dawn")}) {
        const QString schemeName = QStringLiteral("Ros\u00e9 Pine %1").arg(variant);
        const QString resourceName = QStringLiteral("Rose Pine %1").arg(variant);
        const QString resourcePath = QStringLiteral(":/color-schemes/%1.json").arg(resourceName);
        QCOMPARE(schemeNames.count(schemeName), 1);
        QVERIFY(!schemeNames.contains(resourceName));
        QVERIFY(ColorScheme::isBuiltInName(schemeName));
        QVERIFY(!ColorScheme::isBuiltInName(resourceName));
        QCOMPARE(ColorScheme::filePathForName(schemeName), resourcePath);

        const ColorScheme selectedScheme = ColorScheme::loadByName(schemeName);
        const ColorScheme resourceScheme = ColorScheme::loadFromFile(resourcePath);
        QVERIFY(selectedScheme.isValid());
        QVERIFY(resourceScheme.isValid());
        QCOMPARE(selectedScheme.displayName(), schemeName);
        QCOMPARE(selectedScheme.toJsonObject(), resourceScheme.toJsonObject());
    }
}

void TestThemesUi::color_scheme_validates_schema_metadata()
{
    const QJsonObject baseScheme{
        {QStringLiteral("background"), QStringLiteral("#1f3229")}
    };

    QJsonObject currentSchema = baseScheme;
    currentSchema.insert(QStringLiteral("$schema"), QString::fromLatin1(ColorScheme::SchemaDraft));
    currentSchema.insert(QStringLiteral("$id"), QString::fromLatin1(ColorScheme::SchemaId));
    currentSchema.insert(QStringLiteral("version"), QStringLiteral("1"));
    QVERIFY(ColorScheme::fromJsonObject(currentSchema).isValid());

    QJsonObject futureSchemaId = baseScheme;
    futureSchemaId.insert(QStringLiteral("$schema"), QString::fromLatin1(ColorScheme::SchemaDraft));
    futureSchemaId.insert(QStringLiteral("$id"),
                          QStringLiteral("https://speedcrunch.org/schemas/theme-v2.schema.json"));
    futureSchemaId.insert(QStringLiteral("version"), QStringLiteral("1"));
    QVERIFY(!ColorScheme::fromJsonObject(futureSchemaId).isValid());

    QVERIFY(!ColorScheme::fromJsonObject(baseScheme).isValid());

    QJsonObject missingVersion = baseScheme;
    missingVersion.insert(QStringLiteral("$schema"), QString::fromLatin1(ColorScheme::SchemaDraft));
    missingVersion.insert(QStringLiteral("$id"), QString::fromLatin1(ColorScheme::SchemaId));
    QVERIFY(ColorScheme::fromJsonObject(missingVersion).isValid());

    QJsonObject authorVersion = baseScheme;
    authorVersion.insert(QStringLiteral("$schema"), QString::fromLatin1(ColorScheme::SchemaDraft));
    authorVersion.insert(QStringLiteral("$id"), QString::fromLatin1(ColorScheme::SchemaId));
    authorVersion.insert(QStringLiteral("version"), QStringLiteral("2"));
    QVERIFY(ColorScheme::fromJsonObject(authorVersion).isValid());

    QJsonObject stringVersion = baseScheme;
    stringVersion.insert(QStringLiteral("$schema"), QString::fromLatin1(ColorScheme::SchemaDraft));
    stringVersion.insert(QStringLiteral("$id"), QString::fromLatin1(ColorScheme::SchemaId));
    stringVersion.insert(QStringLiteral("version"), QStringLiteral("1.2"));
    QVERIFY(ColorScheme::fromJsonObject(stringVersion).isValid());

    QJsonObject invalidVersion = baseScheme;
    invalidVersion.insert(QStringLiteral("$schema"), QString::fromLatin1(ColorScheme::SchemaDraft));
    invalidVersion.insert(QStringLiteral("$id"), QString::fromLatin1(ColorScheme::SchemaId));
    invalidVersion.insert(QStringLiteral("version"), 1);
    QVERIFY(!ColorScheme::fromJsonObject(invalidVersion).isValid());

    QJsonObject malformedSchema = baseScheme;
    malformedSchema.insert(QStringLiteral("$schema"), 1);
    malformedSchema.insert(QStringLiteral("$id"), QString::fromLatin1(ColorScheme::SchemaId));
    malformedSchema.insert(QStringLiteral("version"), QStringLiteral("1"));
    QVERIFY(!ColorScheme::fromJsonObject(malformedSchema).isValid());

    QJsonObject missingId = baseScheme;
    missingId.insert(QStringLiteral("$schema"), QString::fromLatin1(ColorScheme::SchemaDraft));
    QVERIFY(!ColorScheme::fromJsonObject(missingId).isValid());
}

void TestThemesUi::theme_dialog_preserves_list_scroll_and_fills_role_color_buttons_data()
{
    QTest::addColumn<bool>("hasNumberFormatPreference");
    QTest::newRow("first-run") << false;
    QTest::newRow("saved-preference") << true;
}

void TestThemesUi::theme_dialog_preserves_list_scroll_and_fills_role_color_buttons()
{
    QFETCH(bool, hasNumberFormatPreference);
    Settings* settings = Settings::instance();
    const bool originalPreference = settings->hasNumberFormatStyleSetting;
    const auto restorePreference = qScopeGuard([settings, originalPreference]() {
        settings->hasNumberFormatStyleSetting = originalPreference;
    });
    settings->hasNumberFormatStyleSetting = hasNumberFormatPreference;
    UiTestFixture::resetSettings();
    QVERIFY(settings->hasNumberFormatStyleSetting);
    MainWindowStateGuard guard;

    settings->colorScheme = QStringLiteral("Custom");
    settings->customColorSchemeJson = themeJsonString(QJsonObject{
        {QStringLiteral("background"), QStringLiteral("#123456")}
    });
    settings->constantsDockVisible = false;
    settings->functionsDockVisible = false;
    settings->historyDockVisible = false;
    settings->keypadVisible = false;
    settings->formulaBookDockVisible = false;
    settings->variablesDockVisible = false;
    settings->userFunctionsDockVisible = false;
    settings->userUnitsDockVisible = false;
    settings->bitfieldVisible = false;

    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QString failure;
    int lightScrollBefore = -1;
    int lightScrollAfter = -1;
    int darkScrollBefore = -1;
    int darkScrollAfter = -1;
    QColor backgroundButtonColor;
    QColor backgroundButtonPixel;
    QColor primaryButtonColor;
    QColor expectedPrimaryButtonColor;

    const auto recordFailure = [&failure](const QString& message) {
        if (failure.isEmpty())
            failure = message;
    };

    QTimer::singleShot(0, &window, [&]() {
        QDialog* dialog = window.findChild<QDialog*>(QStringLiteral("ThemeDialog"));
        if (dialog == nullptr) {
            recordFailure(QStringLiteral("Theme dialog was not found."));
            return;
        }

        const auto finishDialog = qScopeGuard([dialog]() {
            dialog->reject();
        });
        Q_UNUSED(finishDialog);

        QListWidget* lightList = dialog->findChild<QListWidget*>(QStringLiteral("LightThemeList"));
        QListWidget* darkList = dialog->findChild<QListWidget*>(QStringLiteral("DarkThemeList"));
        QPushButton* backgroundButton = dialog->findChild<QPushButton*>(
            QStringLiteral("ThemeColorButton_background"));
        QPushButton* primaryButton = dialog->findChild<QPushButton*>(
            QStringLiteral("ThemeColorButton_primary"));

        if (lightList == nullptr || darkList == nullptr || backgroundButton == nullptr
                || primaryButton == nullptr) {
            recordFailure(QStringLiteral("Theme dialog controls were not found."));
            return;
        }
        const auto constrainListHeight = [](QListWidget* list) {
            const int rowHeight = list->sizeHintForRow(0) > 0
                ? list->sizeHintForRow(0)
                : list->fontMetrics().height() + 6;
            list->setFixedHeight(rowHeight * 3 + list->frameWidth() * 2);
        };
        constrainListHeight(lightList);
        constrainListHeight(darkList);
        if (dialog->layout() != nullptr)
            dialog->layout()->activate();
        QCoreApplication::processEvents();

        const QColor initialBackgroundButtonColor(backgroundButton->text());
        primaryButtonColor = QColor(primaryButton->text());
        expectedPrimaryButtonColor = generatePrimaryFromBackground(initialBackgroundButtonColor);

        if (lightList->verticalScrollBar()->maximum() <= 0
                || darkList->verticalScrollBar()->maximum() <= 0) {
            recordFailure(QStringLiteral("Theme lists are not scrollable: light count %1 max %2, dark count %3 max %4.")
                              .arg(lightList->count())
                              .arg(lightList->verticalScrollBar()->maximum())
                              .arg(darkList->count())
                              .arg(darkList->verticalScrollBar()->maximum()));
            return;
        }

        const auto clickVisibleTheme = [&recordFailure](QListWidget* list) {
            const QModelIndex index = list->indexAt(QPoint(list->viewport()->width() / 2,
                                                           list->viewport()->height() / 2));
            if (!index.isValid()) {
                recordFailure(QStringLiteral("No visible theme item was found for clicking."));
                return false;
            }

            QTest::mouseClick(list->viewport(),
                              Qt::LeftButton,
                              Qt::NoModifier,
                              list->visualRect(index).center());
            QCoreApplication::processEvents();
            QCoreApplication::processEvents();
            return true;
        };

        lightList->verticalScrollBar()->setValue(qMin(2, lightList->verticalScrollBar()->maximum()));
        QCoreApplication::processEvents();
        lightScrollBefore = lightList->verticalScrollBar()->value();
        if (!clickVisibleTheme(lightList))
            return;
        lightScrollAfter = lightList->verticalScrollBar()->value();

        darkList->verticalScrollBar()->setValue(qMin(2, darkList->verticalScrollBar()->maximum()));
        QCoreApplication::processEvents();
        darkScrollBefore = darkList->verticalScrollBar()->value();
        if (!clickVisibleTheme(darkList))
            return;
        darkScrollAfter = darkList->verticalScrollBar()->value();

        backgroundButtonColor = QColor(backgroundButton->text());
        const QImage buttonImage = backgroundButton->grab().toImage();
        backgroundButtonPixel = buttonImage.pixelColor(buttonImage.width() - 6,
                                                       buttonImage.height() / 2);
    });

    QVERIFY(QMetaObject::invokeMethod(&window, "showCustomThemeDialog", Qt::DirectConnection));
    QVERIFY2(failure.isEmpty(), qPrintable(failure));
    QCOMPARE(lightScrollAfter, lightScrollBefore);
    QCOMPARE(darkScrollAfter, darkScrollBefore);
    QVERIFY(colorsAreClose(backgroundButtonPixel, backgroundButtonColor, 3));
    QCOMPARE(primaryButtonColor.name(), expectedPrimaryButtonColor.name());
}

void TestThemesUi::theme_dialog_repeatedly_previews_same_light_and_dark_themes_data()
{
    QTest::addColumn<bool>("useKeyboard");
    QTest::addColumn<bool>("acceptTheme");
    QTest::newRow("mouse-accept") << false << true;
    QTest::newRow("mouse-cancel") << false << false;
    QTest::newRow("keyboard-accept") << true << true;
    QTest::newRow("keyboard-cancel") << true << false;
}

void TestThemesUi::theme_dialog_repeatedly_previews_same_light_and_dark_themes()
{
    QFETCH(bool, useKeyboard);
    QFETCH(bool, acceptTheme);
    MainWindowStateGuard guard;
    Settings* settings = Settings::instance();
    const QString initialSchemeName = QStringLiteral("Duskfox");
    settings->colorScheme = initialSchemeName;

    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    int previewsChecked = 0;
    QString lastSchemeName;
    QTimer::singleShot(0, &window, [&]() {
        QDialog* dialog = window.findChild<QDialog*>(QStringLiteral("ThemeDialog"));
        QVERIFY(dialog != nullptr);
        auto finishDialog = qScopeGuard([dialog]() { dialog->reject(); });
        Q_UNUSED(finishDialog);

        QListWidget* lightList = dialog->findChild<QListWidget*>(QStringLiteral("LightThemeList"));
        QListWidget* darkList = dialog->findChild<QListWidget*>(QStringLiteral("DarkThemeList"));
        QWidget* previewWidget = dialog->findChild<QWidget*>(QStringLiteral("ThemePreview"));
        QVERIFY(lightList != nullptr);
        QVERIFY(darkList != nullptr);
        QVERIFY(previewWidget != nullptr);
        QPlainTextEdit* resultPreview = nullptr;
        for (QPlainTextEdit* textEdit : previewWidget->findChildren<QPlainTextEdit*>()) {
            if (qobject_cast<Editor*>(textEdit) == nullptr)
                resultPreview = textEdit;
        }
        Editor* editorPreview = previewWidget->findChild<Editor*>();
        QPushButton* backgroundButton = dialog->findChild<QPushButton*>(
            QStringLiteral("ThemeColorButton_background"));
        QVERIFY(resultPreview != nullptr);
        QVERIFY(editorPreview != nullptr);
        QVERIFY(backgroundButton != nullptr);
        QVERIFY(lightList->count() > 0);
        QVERIFY(darkList->count() > 0);
        QCoreApplication::processEvents();

        for (int cycle = 0; cycle < 4; ++cycle) {
            for (QListWidget* list : {lightList, darkList}) {
                QListWidgetItem* item = list->item(0);
                list->scrollToItem(item);
                if (useKeyboard) {
                    list->setFocus();
                    QTest::keyClick(list, Qt::Key_Home);
                    QTest::keyClick(list, Qt::Key_Space);
                } else {
                    QTest::mouseClick(list->viewport(), Qt::LeftButton, Qt::NoModifier,
                                      list->visualItemRect(item).center());
                }
                QCoreApplication::processEvents();

                QCOMPARE(list->selectedItems(), QList<QListWidgetItem*>{item});
                QListWidget* otherList = list == lightList ? darkList : lightList;
                QVERIFY(otherList->selectedItems().isEmpty());
                lastSchemeName = item->data(Qt::UserRole).toString();
                const ColorScheme scheme = ColorScheme::loadByName(lastSchemeName);
                QVERIFY(scheme.isValid());
                const QColor background = scheme.colorForRole(ColorScheme::Background);
                QCOMPARE(QColor(backgroundButton->text()), background);
                QCOMPARE(resultPreview->palette().color(QPalette::Base), background);
                const QVector<QColor> shades = generateOklchShades(
                    background, 6, themePolarityForBackground(background));
                QCOMPARE(editorPreview->palette().color(QPalette::Base),
                         shades.at(UiConfig::DockBackgroundShade));
                ++previewsChecked;
            }
        }
        if (acceptTheme) {
            dialog->accept();
            finishDialog.dismiss();
        }
    });

    QVERIFY(QMetaObject::invokeMethod(&window, "showCustomThemeDialog", Qt::DirectConnection));
    QCOMPARE(previewsChecked, 8);
    QCOMPARE(settings->colorScheme, acceptTheme ? lastSchemeName : initialSchemeName);
}

void TestThemesUi::main_window_uses_generated_theme_surface_for_chrome_and_editor_data()
{
    QTest::addColumn<int>("editorDisplayStyle");
    QTest::newRow("classic") << int(Settings::EditorDisplayClassic);
    QTest::newRow("modern") << int(Settings::EditorDisplayModern);
}

void TestThemesUi::main_window_uses_generated_theme_surface_for_chrome_and_editor()
{
    QFETCH(int, editorDisplayStyle);
    Settings* settings = Settings::instance();
    struct SettingsGuard {
        Settings* settings;
        QString oldColorScheme;
        QString oldCustomColorSchemeJson;
        Settings::KeypadMode oldKeypadMode;
        bool oldKeypadVisible;
        bool oldStatusBarVisible;
        bool oldBitfieldVisible;
        Settings::EditorDisplayStyle oldEditorDisplayStyle;
        bool oldHasNumberFormatStyleSetting;
        QByteArray oldSkipUpdateCheck;
        bool hadSkipUpdateCheck;

        ~SettingsGuard()
        {
            settings->colorScheme = oldColorScheme;
            settings->customColorSchemeJson = oldCustomColorSchemeJson;
            settings->keypadMode = oldKeypadMode;
            settings->keypadVisible = oldKeypadVisible;
            settings->statusBarVisible = oldStatusBarVisible;
            settings->bitfieldVisible = oldBitfieldVisible;
            settings->editorDisplayStyle = oldEditorDisplayStyle;
            settings->hasNumberFormatStyleSetting = oldHasNumberFormatStyleSetting;
            if (hadSkipUpdateCheck)
                qputenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK", oldSkipUpdateCheck);
            else
                qunsetenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK");
        }
    } guard {
        settings,
        settings->colorScheme,
        settings->customColorSchemeJson,
        settings->keypadMode,
        settings->keypadVisible,
        settings->statusBarVisible,
        settings->bitfieldVisible,
        settings->editorDisplayStyle,
        settings->hasNumberFormatStyleSetting,
        qgetenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK"),
        qEnvironmentVariableIsSet("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK")
    };

    qputenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK", "1");
    settings->colorScheme = QStringLiteral("Custom");
    settings->keypadMode = Settings::KeypadModeBasicWide;
    settings->statusBarVisible = true;
    settings->bitfieldVisible = true;
    settings->editorDisplayStyle = static_cast<Settings::EditorDisplayStyle>(editorDisplayStyle);
    settings->hasNumberFormatStyleSetting = true;

    const auto verifyTheme = [settings](const QString& baseName, ThemePolarity polarity) {
        QJsonObject colors;
        colors.insert(QStringLiteral("background"), baseName);
        settings->customColorSchemeJson = themeJsonString(colors);
        settings->keypadVisible = true;
        settings->bitfieldVisible = true;

        const QColor base(baseName);
        const QVector<QColor> shades = generateOklchShades(base, 6, polarity);
        const QVector<QColor> foregrounds = aaForegroundsForBackgrounds(shades);
        const QColor expectedResultSurface = shades.at(UiConfig::ResultDisplayShade);
        const QColor expectedWindowSurface = shades.at(UiConfig::WindowBackgroundShade);
        const QColor expectedKeypadSurface = shades.at(UiConfig::KeypadBackgroundShade);
        const QColor expectedEditorSurface = shades.at(UiConfig::DockBackgroundShade);
        const QColor expectedHeaderSurface = shades.at(UiConfig::DockHeaderShade);
        const QColor expectedInputSurface = shades.at(UiConfig::DockUnfocusedSelectedItemShade);
        const QColor expectedKeypadButtonSurface = shades.at(UiConfig::KeypadButtonShade);
        const QColor expectedBitfieldButtonSurface =
            shades.at(UiConfig::BitfieldButtonFillShade);
        const QColor expectedBitfieldButtonHoverSurface =
            shades.at(UiConfig::BitfieldButtonHoverFillShade);
        const QColor expectedBitfieldButtonPressedSurface =
            shades.at(UiConfig::BitfieldButtonPressedFillShade);
        const QColor expectedStatusBarSurface = shades.at(UiConfig::StatusBarBackgroundShade);
        const QColor expectedPrimary = generatePrimaryFromBackground(base);
        const bool classic = settings->editorDisplayStyle == Settings::EditorDisplayClassic;
        const QColor expectedDockSeparator = shades.at(
            classic ? UiConfig::ClassicVerticalPaneDividerShade : UiConfig::SplitterShade);
        const QColor expectedWindowForeground = foregrounds.at(UiConfig::WindowBackgroundShade);
        const QColor expectedKeypadForeground = foregrounds.at(UiConfig::KeypadBackgroundShade);
        const QColor expectedEditorForeground = foregrounds.at(UiConfig::DockBackgroundShade);
        const QColor expectedHeaderForeground = foregrounds.at(UiConfig::DockHeaderShade);
        const QColor expectedInputForeground =
            foregrounds.at(UiConfig::DockUnfocusedSelectedItemShade);
        const QColor expectedKeypadButtonForeground = foregrounds.at(UiConfig::KeypadButtonShade);
        const QColor expectedBitfieldButtonForeground =
            foregrounds.at(UiConfig::BitfieldButtonFillShade);
        const QColor expectedBitfieldButtonHoverForeground =
            foregrounds.at(UiConfig::BitfieldButtonHoverFillShade);
        const QColor expectedBitfieldButtonPressedForeground =
            foregrounds.at(UiConfig::BitfieldButtonPressedFillShade);
        const QColor expectedStatusBarForeground =
            foregrounds.at(UiConfig::StatusBarBackgroundShade);

        MainWindow window;
        window.show();
        QCoreApplication::processEvents();

        QCOMPARE(window.palette().color(QPalette::Window).name(), expectedWindowSurface.name());
        QCOMPARE(window.palette().color(QPalette::WindowText).name(),
                 expectedWindowForeground.name());
        QCOMPARE(window.palette().color(QPalette::ButtonText).name(),
                 expectedWindowForeground.name());

        Editor* editor = window.findChild<Editor*>();
        ResultDisplay* display = window.findChild<ResultDisplay*>();
        QSplitter* splitContainer =
            window.findChild<QSplitter*>(QStringLiteral("MainSplitContainer"));
        Keypad* keypad = window.findChild<Keypad*>();
        BitFieldWidget* bitfield = window.findChild<BitFieldWidget*>();
        BitWidget* bit = bitfield ? bitfield->findChild<BitWidget*>() : nullptr;
        QStatusBar* statusBar =
            window.findChild<QStatusBar*>(QString(), Qt::FindDirectChildrenOnly);
        QVERIFY(editor != nullptr);
        QVERIFY(display != nullptr);
        QVERIFY(splitContainer != nullptr);
        QVERIFY(keypad != nullptr);
        QVERIFY(bitfield != nullptr);
        QVERIFY(bit != nullptr);
        QVERIFY(statusBar != nullptr);
        QCOMPARE(statusBar->findChildren<QPushButton*>().size(), 3);
        int pipeSeparators = 0;
        for (QLabel* label : statusBar->findChildren<QLabel*>()) {
            if (label->text() == QStringLiteral("|"))
                ++pipeSeparators;
        }
        QCOMPARE(pipeSeparators, 2);
        for (QPushButton* button : statusBar->findChildren<QPushButton*>()) {
            QCOMPARE(button->cursor().shape(), Qt::PointingHandCursor);
            const QImage buttonImage = button->grab().toImage();
            QVERIFY(!buttonImage.isNull());
            QVERIFY2(firstPixelMatchingColor(buttonImage,
                                             buttonImage.rect(),
                                             expectedStatusBarForeground,
                                             64) != QPoint(-1, -1),
                     qPrintable(QStringLiteral("status button %1 has no %2 text pixel")
                                    .arg(button->text(), expectedStatusBarForeground.name())));
        }
        QVERIFY(QMetaObject::invokeMethod(&window,
                                          "setStatusBarVisible",
                                          Qt::DirectConnection,
                                          Q_ARG(bool, true)));
        QCOMPARE(statusBar->findChildren<QPushButton*>().size(), 3);
        QWidget* pane = paneWidgetForDisplay(display);
        QWidget* page = display->parentWidget();
        QTabBar* sessionTabBar = pane ? pane->findChild<QTabBar*>() : nullptr;
        QVERIFY(pane != nullptr);
        QVERIFY(page != nullptr);
        QVERIFY(sessionTabBar != nullptr);
        QCOMPARE(splitContainer->palette().color(QPalette::Window).name(),
                 expectedWindowSurface.name());
        QCOMPARE(window.property("speedcrunchDockSeparatorNormalColor").value<QColor>().name(),
                 expectedDockSeparator.name());
        QCOMPARE(pane->palette().color(QPalette::Window).name(),
                 expectedResultSurface.name());
        QCOMPARE(page->palette().color(QPalette::Window).name(),
                 expectedResultSurface.name());
        QCOMPARE(page->parentWidget()->palette().color(QPalette::Window).name(),
                 expectedResultSurface.name());
        QCOMPARE(display->palette().color(QPalette::Base).name(), expectedResultSurface.name());
        QCOMPARE(display->viewport()->palette().color(QPalette::Base).name(),
                 expectedResultSurface.name());
        QVERIFY(display->styleSheet().contains(expectedResultSurface.name()));
        QVERIFY(display->viewport()->styleSheet().contains(expectedResultSurface.name()));
        const QImage displayImage = display->viewport()->grab().toImage();
        QVERIFY(!displayImage.isNull());
        QCOMPARE(displayImage.pixelColor(displayImage.width() / 2,
                                         displayImage.height() / 2).name(),
                 expectedResultSurface.name());
        sessionTabBar->show();
        QCoreApplication::processEvents();
        QVERIFY(sessionTabBar->isVisible());
        QToolButton* sessionCloseButton = qobject_cast<QToolButton*>(
            sessionTabBar->tabButton(0, QTabBar::RightSide));
        QVERIFY(sessionCloseButton != nullptr);
        QVERIFY(sessionCloseButton->toolTip().isEmpty());
        QEvent closeButtonEnterEvent(QEvent::Enter);
        QCoreApplication::sendEvent(sessionCloseButton, &closeButtonEnterEvent);
        QCoreApplication::processEvents();
        QFrame* closeButtonToolTip =
            sessionTabBar->findChild<QFrame*>(QStringLiteral("sessionTabToolTipPopup"));
        QVERIFY(closeButtonToolTip != nullptr);
        QVERIFY(closeButtonToolTip->isVisible());
        QVERIFY(closeButtonToolTip->styleSheet().contains(
            shades.at(UiConfig::CompletionPopupBackgroundShade).name()));
        QLabel* closeButtonToolTipLabel = closeButtonToolTip->findChild<QLabel*>(
            QStringLiteral("sessionTabToolTipPopupLabel"));
        QVERIFY(closeButtonToolTipLabel != nullptr);
        QCOMPARE(closeButtonToolTipLabel->text(), QStringLiteral("Close Session"));
        QEvent closeButtonLeaveEvent(QEvent::Leave);
        QCoreApplication::sendEvent(sessionCloseButton, &closeButtonLeaveEvent);
        QCoreApplication::processEvents();
        QVERIFY(!closeButtonToolTip->isVisible());
        const QImage tabBarImage = sessionTabBar->grab().toImage();
        QVERIFY(!tabBarImage.isNull());
        QCOMPARE(tabBarImage.pixelColor(tabBarImage.width() - 1,
                                        tabBarImage.height() / 2).name(),
                 expectedWindowSurface.name());
        QWidget* tabBarRow = sessionTabBar->parentWidget();
        QVERIFY(tabBarRow != nullptr);
        QCOMPARE(tabBarRow->palette().color(QPalette::Window).name(),
                 expectedWindowSurface.name());
        QVERIFY(tabBarRow->styleSheet().contains(expectedWindowSurface.name()));
        QCOMPARE(editor->palette().color(QPalette::Base).name(),
                 expectedEditorSurface.name());
        QVERIFY(editor->styleSheet().contains(expectedEditorSurface.name()));
        if (classic) {
            QVERIFY(editor->styleSheet().contains(QStringLiteral("border-radius: 0;")));
            QVERIFY(editor->styleSheet().contains(QStringLiteral("margin: 0;")));
            QCOMPARE(editor->geometry().left(), display->geometry().left());
            QCOMPARE(editor->width(), display->width());
            QCOMPARE(editor->geometry().top(), display->geometry().bottom() + 1);
            QCOMPARE(editor->viewport()->x() + editor->cursorRect().x(),
                     UiConfig::ClassicTextInset);
        } else {
            QVERIFY(editor->styleSheet().contains(QStringLiteral("border-radius: %1px")
                .arg(UiConfig::ModernEditorCornerRadius)));
            QVERIFY(editor->styleSheet().contains(QStringLiteral("padding: %1px %2px")
                .arg(UiConfig::ModernEditorVerticalPadding)
                .arg(UiConfig::ModernEditorHorizontalPadding)));
        }
        QVERIFY(editor->viewport()->styleSheet().contains(QStringLiteral("background: transparent")));
        QTRY_COMPARE(editor->cursorWidth(), 2);
        QCOMPARE(editor->graphicsEffect(), nullptr);
        QVERIFY(editor->mask().isEmpty());
        QCOMPARE(keypad->palette().color(QPalette::Window).name(), expectedKeypadSurface.name());
        QTRY_COMPARE(keypad->palette().color(QPalette::WindowText).name(),
                     expectedKeypadForeground.name());
        QPushButton* keypadButton = keypadButtonWithText(keypad, QStringLiteral("%"));
        QPushButton* keypadDigitButton = keypadButtonWithText(keypad, QStringLiteral("7"));
        QPushButton* keypadDecimalButton =
            keypadButtonWithText(keypad, QString(QChar(settings->radixCharacter())));
        QPushButton* keypadEvaluateButton = keypadButtonWithText(keypad, QStringLiteral("="));
        QVERIFY(keypadButton != nullptr);
        QVERIFY(keypadDigitButton != nullptr);
        QVERIFY(keypadDecimalButton != nullptr);
        QVERIFY(keypadEvaluateButton != nullptr);
        QWidget* keypadContainer = keypad->parentWidget();
        QVERIFY(keypadContainer != nullptr);
        QCOMPARE(keypadContainer->palette().color(QPalette::Window).name(),
                 expectedKeypadSurface.name());
        QVERIFY(keypadContainer->styleSheet().contains(expectedKeypadSurface.name()));
        const QImage keypadContainerImage = keypadContainer->grab().toImage();
        QVERIFY(!keypadContainerImage.isNull());
        QCOMPARE(keypadContainerImage.pixelColor(0, keypadContainerImage.height() / 2).name(),
                 expectedKeypadSurface.name());
        QCOMPARE(keypadContainerImage.pixelColor(keypadContainerImage.width() - 1,
                                                 keypadContainerImage.height() / 2).name(),
                 expectedKeypadSurface.name());
        const QString keypadButtonStyle = keypadButton->styleSheet();
        QCOMPARE(keypadButton->palette().color(QPalette::Button).name(),
                 expectedKeypadButtonSurface.name());
        QCOMPARE(keypadButton->palette().color(QPalette::ButtonText).name(),
                 expectedKeypadButtonForeground.name());
        QVERIFY(keypadButtonStyle.contains(expectedKeypadButtonSurface.name()));
        QVERIFY(keypadButtonStyle.contains(expectedKeypadButtonForeground.name()));
        QVERIFY(keypadButtonStyle.contains(expectedHeaderSurface.name()));
        QVERIFY(keypadButtonStyle.contains(expectedHeaderForeground.name()));
        QVERIFY(keypadButtonStyle.contains(expectedInputSurface.name()));
        QVERIFY(keypadButtonStyle.contains(expectedInputForeground.name()));
        QVERIFY(keypadButtonStyle.contains(QStringLiteral("border: none")));
        QVERIFY(keypadButtonStyle.contains(QStringLiteral("qlineargradient")));
        QVERIFY(keypadButtonStyle.contains(QStringLiteral("border-radius: %1px")
                                               .arg(UiConfig::KeypadButtonCornerRadius)));
        QVERIFY(keypadButtonStyle.contains(QStringLiteral("margin: %1px")
                                               .arg(UiConfig::KeypadButtonMargin)));
        QVERIFY2(keypadButtonStyle.contains(QStringLiteral("padding: %1px")
                                                .arg(UiConfig::KeypadButtonPadding)),
                 qPrintable(keypadButtonStyle));
        QVERIFY(keypad->layout() != nullptr);
        QCOMPARE(keypad->layout()->contentsMargins(),
                 QMargins(UiConfig::KeypadButtonMargin,
                          UiConfig::KeypadButtonMargin,
                          UiConfig::KeypadButtonMargin,
                          UiConfig::KeypadButtonMargin));
        const QColor expectedDigitSurface =
            keypadPrimaryHueFillForTest(
                expectedPrimary,
                expectedKeypadButtonSurface,
                expectedKeypadButtonSurface,
                UiConfig::KeypadDigitPrimaryHueChromaPercent);
        const QColor expectedDigitForeground = aaForegroundForBackground(expectedDigitSurface);
        for (QPushButton* digitButton : {keypadDigitButton, keypadDecimalButton}) {
            const QString digitStyle = digitButton->styleSheet();
            QCOMPARE(digitButton->palette().color(QPalette::Button).name(),
                     expectedDigitSurface.name());
            QCOMPARE(digitButton->palette().color(QPalette::ButtonText).name(),
                     expectedDigitForeground.name());
            QVERIFY(digitStyle.contains(expectedDigitSurface.name()));
            QVERIFY(digitStyle.contains(expectedDigitForeground.name()));
            QVERIFY(digitStyle.contains(QStringLiteral("qlineargradient")));
        }
        const QColor expectedOperatorSurface =
            keypadPrimaryHueFillForTest(
                expectedPrimary,
                expectedKeypadButtonSurface,
                expectedKeypadButtonSurface,
                UiConfig::KeypadOperatorPrimaryHueChromaPercent);
        const QColor expectedOperatorHoverSurface =
            keypadPrimaryHueFillForTest(
                expectedPrimary,
                expectedHeaderSurface,
                expectedKeypadButtonSurface,
                UiConfig::KeypadOperatorPrimaryHueChromaPercent);
        const QColor expectedOperatorPressedSurface =
            keypadPrimaryHueFillForTest(
                expectedPrimary,
                expectedInputSurface,
                expectedKeypadButtonSurface,
                UiConfig::KeypadOperatorPrimaryHueChromaPercent);
        const QColor expectedOperatorForeground = aaForegroundForBackground(expectedOperatorSurface);
        const QColor expectedOperatorHoverForeground =
            aaForegroundForBackground(expectedOperatorHoverSurface);
        const QColor expectedOperatorPressedForeground =
            aaForegroundForBackground(expectedOperatorPressedSurface);
        const QStringList keypadOperatorLabels = {
            QStringLiteral("+"),
            QString::fromUtf8("−"),
            QString::fromUtf8("×"),
            QString::fromUtf8("÷")
        };
        for (const QString& label : keypadOperatorLabels) {
            QPushButton* keypadOperatorButton = keypadButtonWithText(keypad, label);
            QVERIFY2(keypadOperatorButton != nullptr, qPrintable(label));
            const QString keypadOperatorStyle = keypadOperatorButton->styleSheet();
            QCOMPARE(keypadOperatorButton->palette().color(QPalette::Button).name(),
                     expectedOperatorSurface.name());
            QCOMPARE(keypadOperatorButton->palette().color(QPalette::ButtonText).name(),
                     expectedOperatorForeground.name());
            QVERIFY(keypadOperatorStyle.contains(expectedOperatorSurface.name()));
            QVERIFY(keypadOperatorStyle.contains(expectedOperatorForeground.name()));
            QVERIFY(keypadOperatorStyle.contains(expectedOperatorHoverSurface.name()));
            QVERIFY(keypadOperatorStyle.contains(expectedOperatorHoverForeground.name()));
            QVERIFY(keypadOperatorStyle.contains(expectedOperatorPressedSurface.name()));
            QVERIFY(keypadOperatorStyle.contains(expectedOperatorPressedForeground.name()));
            QVERIFY(keypadOperatorStyle.contains(QStringLiteral("qlineargradient")));
        }
        const QColor expectedEvaluateSurface =
            keypadPrimaryHueFillForTest(
                expectedPrimary,
                expectedKeypadButtonSurface,
                expectedKeypadButtonSurface,
                UiConfig::KeypadEvaluatePrimaryHueChromaPercent);
        const QColor expectedEvaluateForeground =
            aaForegroundForBackground(expectedEvaluateSurface);
        QCOMPARE(keypadEvaluateButton->palette().color(QPalette::Button).name(),
                 expectedEvaluateSurface.name());
        QCOMPARE(keypadEvaluateButton->palette().color(QPalette::ButtonText).name(),
                 expectedEvaluateForeground.name());
        QVERIFY(keypadEvaluateButton->styleSheet().contains(expectedEvaluateSurface.name()));
        QVERIFY(keypadEvaluateButton->styleSheet().contains(expectedEvaluateForeground.name()));
        QVERIFY(keypadEvaluateButton->styleSheet().contains(QStringLiteral("qlineargradient")));
        QCOMPARE(bitfield->palette().color(QPalette::Window).name(), expectedEditorSurface.name());
        QCOMPARE(bitfield->palette().color(QPalette::Button).name(), expectedEditorSurface.name());
        QVERIFY(bit->styleSheet().contains(expectedEditorForeground.name()));
        QVERIFY(bit->styleSheet().contains(expectedEditorSurface.name()));
        QVERIFY(bit->styleSheet().contains(expectedHeaderSurface.name()));
        QVERIFY(bit->styleSheet().contains(expectedHeaderForeground.name()));
        QVERIFY(bit->styleSheet().contains(expectedPrimary.name()));
        QVERIFY(bit->styleSheet().contains(aaForegroundForBackground(expectedPrimary).name()));
        QVERIFY(bitfield->styleSheet().contains(expectedEditorForeground.name()));
        QVERIFY(bitfield->styleSheet().contains(expectedEditorSurface.name()));
        QPushButton* bitfieldButton = bitfield->findChild<QPushButton*>();
        QVERIFY(bitfieldButton != nullptr);
        QCOMPARE(bitfieldButton->palette().color(QPalette::Button).name(),
                 expectedBitfieldButtonSurface.name());
        QCOMPARE(bitfieldButton->palette().color(QPalette::ButtonText).name(),
                 expectedBitfieldButtonForeground.name());
        QVERIFY(bitfieldButton->styleSheet().contains(expectedBitfieldButtonSurface.name()));
        QVERIFY(bitfieldButton->styleSheet().contains(expectedBitfieldButtonForeground.name()));
        QVERIFY(bitfieldButton->styleSheet().contains(expectedBitfieldButtonHoverSurface.name()));
        QVERIFY(bitfieldButton->styleSheet().contains(
            expectedBitfieldButtonHoverForeground.name()));
        QVERIFY(bitfieldButton->styleSheet().contains(expectedBitfieldButtonPressedSurface.name()));
        QVERIFY(bitfieldButton->styleSheet().contains(
            expectedBitfieldButtonPressedForeground.name()));
        QCOMPARE(statusBar->palette().color(QPalette::Window).name(),
                 expectedStatusBarSurface.name());
        QCOMPARE(statusBar->palette().color(QPalette::WindowText).name(),
                 expectedStatusBarForeground.name());
        QVERIFY(statusBar->styleSheet().contains(expectedStatusBarSurface.name()));
        QVERIFY(statusBar->styleSheet().contains(expectedStatusBarForeground.name()));
        QVERIFY(statusBar->styleSheet().contains(QStringLiteral("QStatusBar QPushButton")));
        QVERIFY(statusBar->styleSheet().contains(
            QStringLiteral("padding: 0px 4px")));
    };

    verifyTheme(QStringLiteral("#300a24"), ThemePolarity::Dark);
    verifyTheme(QStringLiteral("#1f3229"), ThemePolarity::Dark);
    verifyTheme(QStringLiteral("#e5eee8"), ThemePolarity::Light);

    settings->customColorSchemeJson = themeJsonString(QJsonObject{{QStringLiteral("background"), QStringLiteral("#e5eee8")}});
    MainWindow changedWindow;
    changedWindow.show();
    QCoreApplication::processEvents();

    settings->customColorSchemeJson = themeJsonString(QJsonObject{{QStringLiteral("background"), QStringLiteral("#300a24")}});
    changedWindow.colorSchemeChanged();
    QCoreApplication::processEvents();

    const QVector<QColor> changedShades =
        generateOklchShades(QColor(QStringLiteral("#300a24")), 6, ThemePolarity::Dark);
    const QColor changedPrimary = generatePrimaryFromBackground(QColor(QStringLiteral("#300a24")));
    ResultDisplay* changedDisplay = changedWindow.findChild<ResultDisplay*>();
    Editor* changedEditor = changedWindow.findChild<Editor*>();
    BitFieldWidget* changedBitfield = changedWindow.findChild<BitFieldWidget*>();
    Keypad* changedKeypad = changedWindow.findChild<Keypad*>();
    QPushButton* changedKeypadButton =
        changedKeypad ? keypadButtonWithText(changedKeypad, QStringLiteral("%")) : nullptr;
    QPushButton* changedDigitButton =
        changedKeypad ? keypadButtonWithText(changedKeypad, QStringLiteral("7")) : nullptr;
    QPushButton* changedOperatorButton =
        changedKeypad ? keypadButtonWithText(changedKeypad, QStringLiteral("+")) : nullptr;
    QPushButton* changedEvaluateButton =
        changedKeypad ? keypadButtonWithText(changedKeypad, QStringLiteral("=")) : nullptr;
    QStatusBar* changedStatusBar =
        changedWindow.findChild<QStatusBar*>(QString(), Qt::FindDirectChildrenOnly);
    QVERIFY(changedDisplay != nullptr);
    QVERIFY(changedEditor != nullptr);
    QVERIFY(changedBitfield != nullptr);
    QVERIFY(changedKeypad != nullptr);
    QVERIFY(changedKeypadButton != nullptr);
    QVERIFY(changedDigitButton != nullptr);
    QVERIFY(changedOperatorButton != nullptr);
    QVERIFY(changedEvaluateButton != nullptr);
    QVERIFY(changedStatusBar != nullptr);
    QCOMPARE(changedWindow.palette().color(QPalette::Window).name(),
             changedShades.at(UiConfig::WindowBackgroundShade).name());
    QCOMPARE(changedDisplay->palette().color(QPalette::Base).name(),
             changedShades.at(UiConfig::ResultDisplayShade).name());
    QCOMPARE(changedEditor->viewport()->palette().color(QPalette::Base).name(),
             changedShades.at(UiConfig::DockBackgroundShade).name());
    QCOMPARE(changedEditor->parentWidget()->palette().color(QPalette::Window).name(),
             changedShades.at(UiConfig::ResultDisplayShade).name());
    QCOMPARE(changedBitfield->palette().color(QPalette::Window).name(),
             changedShades.at(UiConfig::DockBackgroundShade).name());
    QCOMPARE(changedKeypadButton->palette().color(QPalette::Button).name(),
             changedShades.at(UiConfig::KeypadButtonShade).name());
    QVERIFY(changedKeypadButton->styleSheet().contains(
        changedShades.at(UiConfig::KeypadButtonShade).name()));
    QCOMPARE(changedStatusBar->palette().color(QPalette::Window).name(),
             changedShades.at(UiConfig::StatusBarBackgroundShade).name());
    QVERIFY(changedStatusBar->styleSheet().contains(
        changedShades.at(UiConfig::StatusBarBackgroundShade).name()));
    const QColor changedDigitSurface = keypadPrimaryHueFillForTest(
        changedPrimary,
        changedShades.at(UiConfig::KeypadButtonShade),
        changedShades.at(UiConfig::KeypadButtonShade),
        UiConfig::KeypadDigitPrimaryHueChromaPercent);
    QCOMPARE(changedDigitButton->palette().color(QPalette::Button).name(),
             changedDigitSurface.name());
    QCOMPARE(changedDigitButton->palette().color(QPalette::ButtonText).name(),
             aaForegroundForBackground(changedDigitSurface).name());
    QVERIFY(changedDigitButton->styleSheet().contains(changedDigitSurface.name()));
    const QColor changedOperatorSurface = keypadPrimaryHueFillForTest(
        changedPrimary,
        changedShades.at(UiConfig::KeypadButtonShade),
        changedShades.at(UiConfig::KeypadButtonShade),
        UiConfig::KeypadOperatorPrimaryHueChromaPercent);
    QCOMPARE(changedOperatorButton->palette().color(QPalette::Button).name(),
             changedOperatorSurface.name());
    QCOMPARE(changedOperatorButton->palette().color(QPalette::ButtonText).name(),
             aaForegroundForBackground(changedOperatorSurface).name());
    QVERIFY(changedOperatorButton->styleSheet().contains(changedOperatorSurface.name()));
    const QColor changedEvaluateSurface = keypadPrimaryHueFillForTest(
        changedPrimary,
        changedShades.at(UiConfig::KeypadButtonShade),
        changedShades.at(UiConfig::KeypadButtonShade),
        UiConfig::KeypadEvaluatePrimaryHueChromaPercent);
    QCOMPARE(changedEvaluateButton->palette().color(QPalette::Button).name(),
             changedEvaluateSurface.name());
    QCOMPARE(changedEvaluateButton->palette().color(QPalette::ButtonText).name(),
             aaForegroundForBackground(changedEvaluateSurface).name());
    QVERIFY(changedEvaluateButton->styleSheet().contains(changedEvaluateSurface.name()));

    QAction* scientificNarrowAction =
        keypadModeAction(&changedWindow, Settings::KeypadModeScientificNarrow);
    QVERIFY(scientificNarrowAction != nullptr);
    QVERIFY(QMetaObject::invokeMethod(&changedWindow,
                                      "setKeypadMode",
                                      Qt::DirectConnection,
                                      Q_ARG(QAction*, scientificNarrowAction)));
    QCoreApplication::processEvents();

    Keypad* switchedKeypad = changedWindow.findChild<Keypad*>();
    QPushButton* switchedDigitButton =
        switchedKeypad ? keypadButtonWithText(switchedKeypad, QStringLiteral("7")) : nullptr;
    QPushButton* switchedOperatorButton =
        switchedKeypad ? keypadButtonWithText(switchedKeypad, QStringLiteral("+")) : nullptr;
    QPushButton* switchedEvaluateButton =
        switchedKeypad ? keypadButtonWithText(switchedKeypad, QStringLiteral("=")) : nullptr;
    QVERIFY(switchedKeypad != nullptr);
    QVERIFY(switchedDigitButton != nullptr);
    QVERIFY(switchedOperatorButton != nullptr);
    QVERIFY(switchedEvaluateButton != nullptr);
    QCOMPARE(switchedDigitButton->palette().color(QPalette::Button).name(),
             changedDigitSurface.name());
    QCOMPARE(switchedDigitButton->palette().color(QPalette::ButtonText).name(),
             aaForegroundForBackground(changedDigitSurface).name());
    QVERIFY(switchedDigitButton->styleSheet().contains(changedDigitSurface.name()));
    QCOMPARE(switchedOperatorButton->palette().color(QPalette::Button).name(),
             changedOperatorSurface.name());
    QCOMPARE(switchedOperatorButton->palette().color(QPalette::ButtonText).name(),
             aaForegroundForBackground(changedOperatorSurface).name());
    QVERIFY(switchedOperatorButton->styleSheet().contains(changedOperatorSurface.name()));
    QCOMPARE(switchedEvaluateButton->palette().color(QPalette::Button).name(),
             changedEvaluateSurface.name());
    QCOMPARE(switchedEvaluateButton->palette().color(QPalette::ButtonText).name(),
             aaForegroundForBackground(changedEvaluateSurface).name());
    QVERIFY(switchedEvaluateButton->styleSheet().contains(changedEvaluateSurface.name()));

    QVERIFY(!changedKeypadButton->styleSheet().contains(
        generateOklchShades(QColor(QStringLiteral("#e5eee8")), 6, ThemePolarity::Light)
            .at(UiConfig::KeypadButtonShade)
            .name()));

    QVERIFY(QMetaObject::invokeMethod(&changedWindow,
                                      "setStatusBarVisible",
                                      Qt::DirectConnection,
                                      Q_ARG(bool, false)));
    QCoreApplication::processEvents();
    QVERIFY(QMetaObject::invokeMethod(&changedWindow,
                                      "setStatusBarVisible",
                                      Qt::DirectConnection,
                                      Q_ARG(bool, true)));
    QCoreApplication::processEvents();

    QStatusBar* recreatedStatusBar =
        changedWindow.findChild<QStatusBar*>(QString(), Qt::FindDirectChildrenOnly);
    QVERIFY(recreatedStatusBar != nullptr);
    QCOMPARE(recreatedStatusBar->palette().color(QPalette::Window).name(),
             changedShades.at(UiConfig::StatusBarBackgroundShade).name());
    QVERIFY(recreatedStatusBar->styleSheet().contains(
        changedShades.at(UiConfig::StatusBarBackgroundShade).name()));
    QVERIFY(recreatedStatusBar->styleSheet().contains(QStringLiteral("QStatusBar QPushButton")));
    QCOMPARE(recreatedStatusBar->findChildren<QPushButton*>().size(), 3);
    int recreatedPipeSeparators = 0;
    for (QLabel* label : recreatedStatusBar->findChildren<QLabel*>()) {
        if (label->text() == QStringLiteral("|"))
            ++recreatedPipeSeparators;
    }
    QCOMPARE(recreatedPipeSeparators, 2);
    for (QPushButton* button : recreatedStatusBar->findChildren<QPushButton*>())
        QCOMPARE(button->cursor().shape(), Qt::PointingHandCursor);

    const QImage changedDisplayImage = changedDisplay->viewport()->grab().toImage();
    QVERIFY(!changedDisplayImage.isNull());
    QCOMPARE(changedDisplayImage.pixelColor(changedDisplayImage.width() / 2,
                                            changedDisplayImage.height() / 2).name(),
             QStringLiteral("#300a24"));

    if (!UiConfig::OklchThemeDebugReportEnabled)
        return;

    QFile report(QDir(QDir::tempPath()).absoluteFilePath(
        QStringLiteral("speedcrunch-oklch-theme-report.html")));
    QVERIFY(report.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString reportHtml = QString::fromUtf8(report.readAll());
    QVERIFY(reportHtml.contains(QStringLiteral("Runtime widget samples")));
    QVERIFY(reportHtml.contains(QStringLiteral("<code>1=#300A24</code>")));
    QVERIFY(reportHtml.contains(QStringLiteral("ResultDisplay 1")));
    QVERIFY(reportHtml.contains(QStringLiteral("<td><code>#300A24</code></td>")));
}

void TestThemesUi::restored_session_layout_reapplies_generated_theme_surfaces()
{
    Settings* settings = Settings::instance();
    struct SettingsGuard {
        Settings* settings;
        QByteArray oldSkipUpdateCheck;
        bool hadSkipUpdateCheck;
        QString oldColorScheme;
        QString oldCustomColorSchemeJson;
        QString oldSessionLayoutJson;
        Settings::KeypadMode oldKeypadMode;
        bool oldStatusBarVisible;
        bool oldConstantsDockVisible;
        bool oldFunctionsDockVisible;
        bool oldHistoryDockVisible;
        bool oldKeypadVisible;
        bool oldFormulaBookDockVisible;
        bool oldVariablesDockVisible;
        bool oldUserFunctionsDockVisible;
        bool oldUserUnitsDockVisible;
        bool oldBitfieldVisible;
        bool oldWindowPositionSave;
        bool oldHasNumberFormatStyleSetting;

        ~SettingsGuard()
        {
            settings->colorScheme = oldColorScheme;
            settings->customColorSchemeJson = oldCustomColorSchemeJson;
            settings->sessionLayoutJson = oldSessionLayoutJson;
            settings->keypadMode = oldKeypadMode;
            settings->statusBarVisible = oldStatusBarVisible;
            settings->constantsDockVisible = oldConstantsDockVisible;
            settings->functionsDockVisible = oldFunctionsDockVisible;
            settings->historyDockVisible = oldHistoryDockVisible;
            settings->keypadVisible = oldKeypadVisible;
            settings->formulaBookDockVisible = oldFormulaBookDockVisible;
            settings->variablesDockVisible = oldVariablesDockVisible;
            settings->userFunctionsDockVisible = oldUserFunctionsDockVisible;
            settings->userUnitsDockVisible = oldUserUnitsDockVisible;
            settings->bitfieldVisible = oldBitfieldVisible;
            settings->windowPositionSave = oldWindowPositionSave;
            settings->hasNumberFormatStyleSetting = oldHasNumberFormatStyleSetting;
            if (hadSkipUpdateCheck)
                qputenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK", oldSkipUpdateCheck);
            else
                qunsetenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK");
        }
    } guard {
        settings,
        qgetenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK"),
        qEnvironmentVariableIsSet("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK"),
        settings->colorScheme,
        settings->customColorSchemeJson,
        settings->sessionLayoutJson,
        settings->keypadMode,
        settings->statusBarVisible,
        settings->constantsDockVisible,
        settings->functionsDockVisible,
        settings->historyDockVisible,
        settings->keypadVisible,
        settings->formulaBookDockVisible,
        settings->variablesDockVisible,
        settings->userFunctionsDockVisible,
        settings->userUnitsDockVisible,
        settings->bitfieldVisible,
        settings->windowPositionSave,
        settings->hasNumberFormatStyleSetting
    };

    qputenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK", "1");
    settings->colorScheme = QStringLiteral("Custom");
    settings->customColorSchemeJson = themeJsonString(QJsonObject{{QStringLiteral("background"), QStringLiteral("#300a24")}});
    settings->sessionLayoutJson.clear();
    settings->keypadMode = Settings::KeypadModeBasicWide;
    settings->statusBarVisible = true;
    settings->constantsDockVisible = false;
    settings->functionsDockVisible = false;
    settings->historyDockVisible = false;
    settings->keypadVisible = true;
    settings->formulaBookDockVisible = false;
    settings->variablesDockVisible = false;
    settings->userFunctionsDockVisible = false;
    settings->userUnitsDockVisible = false;
    settings->bitfieldVisible = false;
    settings->windowPositionSave = false;
    settings->hasNumberFormatStyleSetting = true;

    {
        MainWindow sourceWindow;
        sourceWindow.show();
        QCoreApplication::processEvents();
        QVERIFY(QMetaObject::invokeMethod(&sourceWindow, "splitActivePaneRight",
                                          Qt::DirectConnection));
        QCoreApplication::processEvents();
        QTRY_COMPARE(sourceWindow.findChildren<ResultDisplay*>().size(), 2);
        sourceWindow.persistSessionAndSettingsForShutdown();
    }

    const QString restoredLayout = settings->sessionLayoutJson;
    QVERIFY(!restoredLayout.isEmpty());

    MainWindow restoredWindow;
    restoredWindow.show();
    QCoreApplication::processEvents();
    QTRY_COMPARE(restoredWindow.findChildren<ResultDisplay*>().size(), 2);

    const QVector<QColor> shades =
        generateOklchShades(QColor(QStringLiteral("#300a24")), 6, ThemePolarity::Dark);
    const QColor paneFill = shades.at(UiConfig::ResultDisplayShade);
    const QColor chromeFill = shades.at(UiConfig::WindowBackgroundShade);
    const QColor keypadFill = shades.at(UiConfig::KeypadBackgroundShade);
    const QColor editorFill = shades.at(UiConfig::DockBackgroundShade);

    QSplitter* splitContainer =
        restoredWindow.findChild<QSplitter*>(QStringLiteral("MainSplitContainer"));
    QVERIFY(splitContainer != nullptr);
    QCOMPARE(splitContainer->palette().color(QPalette::Window).name(), chromeFill.name());

    for (ResultDisplay* display : restoredWindow.findChildren<ResultDisplay*>()) {
        QCOMPARE(display->palette().color(QPalette::Base).name(), paneFill.name());
        QCOMPARE(display->viewport()->palette().color(QPalette::Base).name(),
                 paneFill.name());
        const QImage displayImage = display->viewport()->grab().toImage();
        QVERIFY(!displayImage.isNull());
        QCOMPARE(displayImage.pixelColor(displayImage.width() / 2,
                                         displayImage.height() / 2).name(),
                 paneFill.name());
        QWidget* pane = paneWidgetForDisplay(display);
        QVERIFY(pane != nullptr);
        QCOMPARE(pane->palette().color(QPalette::Window).name(), paneFill.name());
    }

    for (Editor* editor : restoredWindow.findChildren<Editor*>())
        QCOMPARE(editor->palette().color(QPalette::Base).name(), editorFill.name());

    Keypad* keypad = restoredWindow.findChild<Keypad*>();
    QVERIFY(keypad != nullptr);
    QCOMPARE(keypad->palette().color(QPalette::Window).name(), keypadFill.name());
    QWidget* keypadContainer = keypad->parentWidget();
    QVERIFY(keypadContainer != nullptr);
    QCOMPARE(keypadContainer->palette().color(QPalette::Window).name(), keypadFill.name());
}

void TestThemesUi::dock_and_popup_text_meet_contrast_target_across_themes()
{
    Settings* settings = Settings::instance();
    settings->colorScheme = QStringLiteral("Terminal");
    settings->constantsDockVisible = true;
    settings->menuAppearance = Settings::MenuAppearanceSpeedCrunch;
    MainWindow window(false);
    window.resize(1000, 700);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto* dock = window.findChild<QDockWidget*>(QStringLiteral("ConstantsDock"));
    QVERIFY(dock);
    auto* constants = dock->findChild<ConstantsWidget*>();
    auto* table = dock->findChild<QTreeWidget*>();
    auto* search = dock->findChild<QLineEdit*>();
    auto* combo = dock->findChild<QComboBox*>();
    QVERIFY(constants && table && search && combo);

    const auto verifyContrast = [](const QPalette& palette, QPalette::ColorRole textRole,
                                   QPalette::ColorRole backgroundRole) {
        const auto luminance = [](const QColor& color) {
            const auto linear = [](double channel) {
                return channel <= 0.04045 ? channel / 12.92
                    : std::pow((channel + 0.055) / 1.055, 2.4);
            };
            return 0.2126 * linear(color.redF()) + 0.7152 * linear(color.greenF())
                + 0.0722 * linear(color.blueF());
        };
        // Match the hex colors used by the widgets' stylesheets.
        const QColor text(palette.color(textRole).name());
        const QColor background(palette.color(backgroundRole).name());
        const double textLuminance = luminance(text);
        const double backgroundLuminance = luminance(background);
        const double contrast = (qMax(textLuminance, backgroundLuminance) + 0.05)
            / (qMin(textLuminance, backgroundLuminance) + 0.05);
        const double best = qMax((backgroundLuminance + 0.05) / 0.05,
                                 1.05 / (backgroundLuminance + 0.05));
        QVERIFY2(contrast >= 10.0 || (best < 10.0 && qAbs(contrast - best) < 1e-6),
                 qPrintable(QStringLiteral("Role %1 on role %2: %3 on %4 gives %5:1")
                                .arg(textRole).arg(backgroundRole)
                                .arg(text.name(), background.name()).arg(contrast)));
    };

    const QStringList themes = ColorScheme::enumerate();
    QVERIFY(themes.contains(QStringLiteral("Terminal")));
    for (const QString& theme : themes) {
        if (!ColorScheme::isBuiltInName(theme))
            continue;
        settings->colorScheme = theme;
        window.colorSchemeChanged();
        QCoreApplication::processEvents();
        // The title has its own stylesheet fill, separate from QPalette::Window.
        const QImage dockImage = dock->grab().toImage();
        QVERIFY(!dockImage.isNull());
        QPalette titlePalette = dock->palette();
        titlePalette.setColor(QPalette::Window,
                              dockImage.pixelColor(dockImage.width() / 2,
                                                   qRound(3 * dockImage.devicePixelRatio())));
        verifyContrast(titlePalette, QPalette::WindowText, QPalette::Window);
        verifyContrast(table->palette(), QPalette::Text, QPalette::Base);
        verifyContrast(search->palette(), QPalette::Text, QPalette::Base);
        verifyContrast(combo->palette(), QPalette::ButtonText, QPalette::Button);
        combo->showPopup();
        QTRY_VERIFY(combo->view()->isVisible());
        verifyContrast(combo->view()->palette(), QPalette::Text, QPalette::Base);
        combo->hidePopup();

        QTRY_VERIFY(table->topLevelItemCount() > 0);
        auto* item = table->topLevelItem(0);
        table->scrollToItem(item);
        QCoreApplication::processEvents();
        const QPoint pos = table->visualItemRect(item).center();
        QMouseEvent move(QEvent::MouseMove, QPointF(pos),
                         QPointF(table->viewport()->mapToGlobal(pos)),
                         Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(table->viewport(), &move);
        auto* popup = constants->findChild<QFrame*>(QStringLiteral("constantsSummaryPopup"));
        QTRY_VERIFY(popup && popup->isVisible());
        QCoreApplication::processEvents();
        auto* label = popup->findChild<QLabel*>(QStringLiteral("constantsSummaryPopupLabel"));
        QVERIFY(label);
        QCOMPARE(label->palette().color(QPalette::WindowText),
                 popup->palette().color(QPalette::WindowText));
        verifyContrast(popup->palette(), QPalette::WindowText, QPalette::Window);
        popup->hide();
    }
}

void TestThemesUi::dock_surfaces_use_successive_generated_shades()
{
    MainWindowStateGuard dockGuard;
    Settings* settings = Settings::instance();
    const Settings::MenuAppearance oldMenuAppearance = settings->menuAppearance;
    const auto restoreMenuAppearance = qScopeGuard([settings, oldMenuAppearance]() {
        settings->menuAppearance = oldMenuAppearance;
    });
    settings->menuAppearance = Settings::MenuAppearanceSpeedCrunch;
    struct SettingsGuard {
        Settings* settings;
        QString oldColorScheme;
        QString oldCustomColorSchemeJson;
        QByteArray oldWindowState;
        bool oldConstantsDockVisible;
        bool oldHasNumberFormatStyleSetting;
        QByteArray oldSkipUpdateCheck;
        bool hadSkipUpdateCheck;

        ~SettingsGuard()
        {
            settings->colorScheme = oldColorScheme;
            settings->customColorSchemeJson = oldCustomColorSchemeJson;
            settings->windowState = oldWindowState;
            settings->constantsDockVisible = oldConstantsDockVisible;
            settings->hasNumberFormatStyleSetting = oldHasNumberFormatStyleSetting;
            if (hadSkipUpdateCheck)
                qputenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK", oldSkipUpdateCheck);
            else
                qunsetenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK");
        }
    } guard {
        settings,
        settings->colorScheme,
        settings->customColorSchemeJson,
        settings->windowState,
        settings->constantsDockVisible,
        settings->hasNumberFormatStyleSetting,
        qgetenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK"),
        qEnvironmentVariableIsSet("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK")
    };

    qputenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK", "1");
    settings->colorScheme = QStringLiteral("Custom");
    settings->customColorSchemeJson = themeJsonString(QJsonObject{{QStringLiteral("background"), QStringLiteral("#1f3229")}});
    settings->windowState.clear();
    settings->constantsDockVisible = true;
    settings->functionsDockVisible = true;
    settings->historyDockVisible = true;
    settings->formulaBookDockVisible = true;
    settings->variablesDockVisible = true;
    settings->userFunctionsDockVisible = true;
    settings->userUnitsDockVisible = true;
    settings->bitfieldVisible = true;
    settings->hasNumberFormatStyleSetting = true;

    const QVector<QColor> shades =
        generateOklchShades(QColor(QStringLiteral("#1f3229")), 6, ThemePolarity::Dark);
    const QVector<QColor> foregrounds = aaForegroundsForBackgrounds(shades);
    const QColor primary = generatePrimaryFromBackground(QColor(QStringLiteral("#1f3229")));

    MainWindow window;
    for (QDockWidget* dock : window.findChildren<QDockWidget*>()) {
        if (dock->objectName() != QStringLiteral("ConstantsDock"))
            dock->hide();
    }
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QCoreApplication::processEvents();

    const QColor titleFill = shades.at(UiConfig::DockHeaderShade);
    const QColor titleText = foregrounds.at(UiConfig::DockHeaderShade);
    const QColor headerButtonFill = shades.at(UiConfig::DockHeaderButtonFillShade);
    const QColor headerButtonText = foregrounds.at(UiConfig::DockHeaderButtonFillShade);
    const QColor headerButtonHoverFill =
        shades.at(UiConfig::DockHeaderButtonHoverFillShade);
    const QColor headerButtonHoverText =
        foregrounds.at(UiConfig::DockHeaderButtonHoverFillShade);
    const QColor controlFill = shades.at(4);
    const QColor controlText = foregrounds.at(4);
    const QColor contentFill = shades.at(2);
    const QColor contentText = foregrounds.at(2);
    const QColor hoveredItemFill = shades.at(UiConfig::DockHoveredItemShade);
    const QColor hoveredItemText = foregrounds.at(UiConfig::DockHoveredItemShade);
    const int comboPopupShade = qMin(UiConfig::DockBackgroundShade + 1, UiConfig::Shade600);
    const QColor comboPopupFill = shades.at(comboPopupShade);
    const QColor comboPopupText = foregrounds.at(comboPopupShade);
    const QColor completionPopupFill = shades.at(UiConfig::CompletionPopupBackgroundShade);
    const QColor completionPopupText = foregrounds.at(UiConfig::CompletionPopupBackgroundShade);
    const QColor completionPopupOutlineFill = shades.at(UiConfig::CompletionPopupOutlineShade);
    const QColor chromeFill = shades.at(UiConfig::WindowBackgroundShade);
    const QColor chromeText = foregrounds.at(UiConfig::WindowBackgroundShade);
    const QColor resultFill = shades.at(UiConfig::ResultDisplayShade);
    const QColor resultText = foregrounds.at(UiConfig::ResultDisplayShade);
    const QColor hoverFill = shades.at(5);
    const QColor textInputOutlineFill = shades.at(UiConfig::DockTextInputOutlineShade);
    const QColor scrollToBottomOutlineFill =
        shades.at(UiConfig::ScrollToBottomButtonOutlineShade);
    const QColor splitterFill = shades.at(UiConfig::SplitterShade);

    const QList<QDockWidget*> docks = window.findChildren<QDockWidget*>();
    QVERIFY(!docks.isEmpty());
    QSplitter* splitContainer =
        window.findChild<QSplitter*>(QStringLiteral("MainSplitContainer"));
    ResultDisplay* display = window.findChild<ResultDisplay*>();
    QVERIFY(splitContainer != nullptr);
    QVERIFY(display != nullptr);
    QWidget* pane = display->parentWidget();
    Editor* editor = pane ? pane->findChild<Editor*>() : nullptr;
    QVERIFY(editor != nullptr);
    QVERIFY(splitContainer->styleSheet().contains(splitterFill.name()));
    QVERIFY(splitContainer->styleSheet().contains(primary.name()));
    QVERIFY(splitContainer->styleSheet().contains(QStringLiteral("QSplitter::handle:hover")));
    QVERIFY(splitContainer->styleSheet().contains(QStringLiteral("QSplitter::handle:pressed")));
    const QList<QSplitterHandle*> splitterHandles = window.findChildren<QSplitterHandle*>();
    QVERIFY(!splitterHandles.isEmpty());
    for (QSplitterHandle* splitterHandle : splitterHandles) {
        QVERIFY(splitterHandle->styleSheet().contains(splitterFill.name()));
        QVERIFY(splitterHandle->styleSheet().contains(primary.name()));
        QVERIFY(splitterHandle->styleSheet().contains(QStringLiteral("QSplitterHandle:hover")));
        QVERIFY(splitterHandle->styleSheet().contains(QStringLiteral("QSplitterHandle:pressed")));
    }
    QCOMPARE(window.property("speedcrunchDockSeparatorNormalColor").value<QColor>(), splitterFill);
    QCOMPARE(window.property("speedcrunchDockSeparatorActiveColor").value<QColor>(), primary);
    const QString resultScrollBarStyle = display->verticalScrollBar()->styleSheet();
    QVERIFY(resultScrollBarStyle.contains(shades.at(1).name()));
    QVERIFY(resultScrollBarStyle.contains(contentFill.name()));
    QVERIFY(resultScrollBarStyle.contains(titleFill.name()));
    QVERIFY(resultScrollBarStyle.contains(controlFill.name()));
    QToolButton* scrollToBottomButton =
        display->findChild<QToolButton*>(QStringLiteral("ScrollToBottomButton"));
    QVERIFY(scrollToBottomButton != nullptr);
    QVERIFY(scrollToBottomButton->styleSheet().contains(contentFill.name()));
    QVERIFY(scrollToBottomButton->styleSheet().contains(titleFill.name()));
    QVERIFY(scrollToBottomButton->styleSheet().contains(QStringLiteral("border: %1px solid %2")
                                                            .arg(UiConfig::OutlineStrokeWidth)
                                                            .arg(scrollToBottomOutlineFill.name())));
    QVERIFY(!scrollToBottomButton->icon().isNull());
    for (QDockWidget* dock : docks) {
        QCOMPARE(dock->palette().color(QPalette::Window).name(), titleFill.name());
        QCOMPARE(dock->palette().color(QPalette::WindowText).name(), titleText.name());
        QVERIFY(dock->styleSheet().contains(titleFill.name()));
        QVERIFY(dock->styleSheet().contains(titleText.name()));
        QVERIFY(dock->styleSheet().contains(QStringLiteral("padding: 5px 4px")));
        QVERIFY(dock->styleSheet().contains(QStringLiteral("QDockWidget::close-button")));
        QVERIFY(dock->styleSheet().contains(QStringLiteral("QDockWidget::float-button")));
        QVERIFY(dock->styleSheet().contains(headerButtonFill.name()));
        QVERIFY(dock->styleSheet().contains(headerButtonText.name()));
        QVERIFY(dock->styleSheet().contains(headerButtonHoverFill.name()));
        QVERIFY(dock->styleSheet().contains(headerButtonHoverText.name()));
        QVERIFY(dock->styleSheet().contains(QStringLiteral("border: none")));
        QVERIFY(dock->styleSheet().contains(QStringLiteral("border-radius: 9px")));
        QVERIFY(dock->styleSheet().contains(QStringLiteral("width: 18px")));
        QVERIFY(dock->styleSheet().contains(QStringLiteral("height: 18px")));
    }

    for (QDockWidget* dock : docks) {
        QWidget* dockContent = dock->widget();
        QVERIFY(dockContent != nullptr);
        const QList<QComboBox*> comboBoxes = dockContent->findChildren<QComboBox*>();
        for (QAbstractItemView* view : dockContent->findChildren<QAbstractItemView*>()) {
            if (qobject_cast<QHeaderView*>(view))
                continue;

            bool comboPopup = false;
            for (const QComboBox* comboBox : comboBoxes)
                comboPopup = comboPopup || comboBox->view() == view || comboBox->isAncestorOf(view);
            if (comboPopup)
                continue;

            QVERIFY(view->parentWidget() != nullptr);
            QVERIFY(view->parentWidget()->layout() != nullptr);
            const QMargins viewMargins = view->parentWidget()->layout()->contentsMargins();
            if (viewMargins != QMargins(0, 0, 0, 0)) {
                const QString message = QStringLiteral("%1 in %2 has list/table margins %3,%4,%5,%6")
                    .arg(QString::fromLatin1(view->metaObject()->className()),
                         dock->objectName())
                    .arg(viewMargins.left())
                    .arg(viewMargins.top())
                    .arg(viewMargins.right())
                    .arg(viewMargins.bottom());
                QFAIL(qPrintable(message));
            }
        }
    }

    QDockWidget* constantsDock =
        window.findChild<QDockWidget*>(QStringLiteral("ConstantsDock"));
    QDockWidget* functionsDock =
        window.findChild<QDockWidget*>(QStringLiteral("FunctionsDock"));
    QVERIFY(constantsDock != nullptr);
    QVERIFY(functionsDock != nullptr);
    ConstantsWidget* constantsWidget = qobject_cast<ConstantsWidget*>(constantsDock->widget());
    QVERIFY(constantsWidget != nullptr);
    bool foundCloseButton = false;
    bool foundFloatButton = false;
    for (QAbstractButton* button : constantsDock->findChildren<QAbstractButton*>()) {
        const QString name = button->objectName();
        if (name != QStringLiteral("qt_dockwidget_closebutton")
            && name != QStringLiteral("qt_dockwidget_floatbutton")) {
            continue;
        }
        foundCloseButton |= name == QStringLiteral("qt_dockwidget_closebutton");
        foundFloatButton |= name == QStringLiteral("qt_dockwidget_floatbutton");
        QCOMPARE(button->palette().color(QPalette::Button).name(), headerButtonFill.name());
        QCOMPARE(button->palette().color(QPalette::ButtonText).name(), headerButtonText.name());
        QCOMPARE(button->cursor().shape(), Qt::PointingHandCursor);
        QVERIFY(button->hasMouseTracking());
        QCOMPARE(button->minimumSize(), QSize(18, 18));
        QCOMPARE(button->maximumSize(), QSize(18, 18));
        QCOMPARE(button->iconSize(), QSize(18, 18));
        QVERIFY(button->styleSheet().contains(headerButtonFill.name()));
        QVERIFY(button->styleSheet().contains(headerButtonText.name()));
        QVERIFY(button->styleSheet().contains(headerButtonHoverFill.name()));
        QVERIFY(button->styleSheet().contains(headerButtonHoverText.name()));
        QVERIFY(button->styleSheet().contains(QStringLiteral("border-radius: 9px")));
        QVERIFY(!button->icon().isNull());
        const QImage iconImage = button->icon().pixmap(QSize(18, 18)).toImage();
        QVERIFY(!iconImage.isNull());
        QVERIFY(iconImage.pixelColor(0, 0).alpha() < 32);
        QVERIFY2(colorsAreClose(iconImage.pixelColor(2, 9), headerButtonFill, 3),
                 qPrintable(QStringLiteral("icon fill %1 expected %2")
                                .arg(iconImage.pixelColor(2, 9).name(),
                                     headerButtonFill.name())));
        const QPoint buttonCenter = button->rect().center();
        QMouseEvent buttonMoveEvent(QEvent::MouseMove,
                                    QPointF(buttonCenter),
                                    QPointF(button->mapToGlobal(buttonCenter)),
                                    Qt::NoButton,
                                    Qt::NoButton,
                                    Qt::NoModifier);
        QCoreApplication::sendEvent(button, &buttonMoveEvent);
        const QImage hoverIconImage = button->icon().pixmap(QSize(18, 18)).toImage();
        QVERIFY(!hoverIconImage.isNull());
        QVERIFY2(colorsAreClose(hoverIconImage.pixelColor(2, 9), headerButtonHoverFill, 3),
                 qPrintable(QStringLiteral("hover icon fill %1 expected %2")
                                .arg(hoverIconImage.pixelColor(2, 9).name(),
                                     headerButtonHoverFill.name())));
        QEvent leaveEvent(QEvent::Leave);
        QCoreApplication::sendEvent(button, &leaveEvent);
    }
    QVERIFY(foundCloseButton);
    QVERIFY(foundFloatButton);
    QComboBox* comboBox = constantsDock->findChild<QComboBox*>();
    QLineEdit* searchBox = constantsDock->findChild<QLineEdit*>();
    QTreeWidget* table = constantsDock->findChild<QTreeWidget*>();
    QLabel* searchLabel = nullptr;
    for (QLabel* label : constantsDock->findChildren<QLabel*>()) {
        if (label->text() == QStringLiteral("Search")) {
            searchLabel = label;
            break;
        }
    }
    QVERIFY(comboBox != nullptr);
    QVERIFY(searchBox != nullptr);
    QVERIFY(searchLabel != nullptr);
    QVERIFY(table != nullptr);
    QVERIFY(table->header() != nullptr);

    QCOMPARE(constantsDock->widget()->palette().color(QPalette::Window).name(), contentFill.name());
    QVERIFY(constantsDock->widget()->styleSheet().contains(contentFill.name()));
    QCOMPARE(comboBox->palette().color(QPalette::Button).name(), contentFill.name());
    QCOMPARE(comboBox->palette().color(QPalette::ButtonText).name(), contentText.name());
    QVERIFY(comboBox->styleSheet().contains(contentFill.name()));
    QVERIFY(comboBox->styleSheet().contains(contentText.name()));
    QVERIFY(comboBox->styleSheet().contains(QStringLiteral("padding: 4px 32px 4px 8px")));
    QVERIFY(comboBox->styleSheet().contains(QStringLiteral("width: 28px")));
    QVERIFY(comboBox->styleSheet().contains(comboPopupFill.name()));
    QVERIFY(comboBox->styleSheet().contains(comboPopupText.name()));
    QCOMPARE(comboBox->view()->palette().color(QPalette::Base).name(), comboPopupFill.name());
    QCOMPARE(comboBox->view()->palette().color(QPalette::Text).name(), comboPopupText.name());
    QVERIFY(comboBox->view()->styleSheet().contains(comboPopupFill.name()));
    QVERIFY(comboBox->view()->styleSheet().contains(comboPopupText.name()));
    QVERIFY(comboBox->styleSheet().contains(QStringLiteral("QComboBox QAbstractItemView")));
    QVERIFY(comboBox->styleSheet().contains(QStringLiteral("border: 0; outline: 0")));
    QVERIFY(comboBox->view()->styleSheet().contains(QStringLiteral("border: 0; border-radius: 8px; outline: 0")));
    QVERIFY(comboBox->view()->styleSheet().contains(QStringLiteral("QAbstractItemView::item")));
    QVERIFY(comboBox->view()->styleSheet().contains(QStringLiteral("border: 0; border-radius: 6px;")));
    QVERIFY(comboBox->view()->styleSheet().contains(hoveredItemFill.name()));
    QVERIFY(comboBox->view()->styleSheet().contains(hoveredItemText.name()));
    QVERIFY(comboBox->view()->verticalScrollBar()->styleSheet().contains(comboPopupFill.name()));
    QVERIFY(comboBox->view()->verticalScrollBar()->styleSheet().contains(controlFill.name()));
    QVERIFY(comboBox->view()->verticalScrollBar()->styleSheet().contains(hoverFill.name()));
    QVERIFY(table->header()->styleSheet().contains(contentFill.name()));
    QVERIFY(table->header()->styleSheet().contains(contentText.name()));
    QVERIFY(table->header()->styleSheet().contains(titleFill.name()));
    QVERIFY(table->header()->styleSheet().contains(QStringLiteral("border: 0")));
    QVERIFY(table->header()->styleSheet().contains(QStringLiteral("border-top: 1px solid")));
    QVERIFY(table->header()->styleSheet().contains(QStringLiteral("border-right: 1px solid")));
    QVERIFY(table->header()->styleSheet().contains(QStringLiteral("border-bottom: 1px solid")));
    QVERIFY(table->header()->styleSheet().contains(QStringLiteral("QHeaderView::section:last")));
    QVERIFY(table->header()->styleSheet().contains(QStringLiteral("border-right: 0")));
    QCOMPARE(searchBox->palette().color(QPalette::Base).name(), contentFill.name());
    QCOMPARE(searchBox->palette().color(QPalette::Text).name(), contentText.name());
    ThemedLineEdit* themedSearchBox = dynamic_cast<ThemedLineEdit*>(searchBox);
    QVERIFY(themedSearchBox != nullptr);
    QCOMPARE(themedSearchBox->cursorColor().name(), primary.name());
    const QString focusRingBorderTemplate = QStringLiteral("border: %1px solid %2");
    QVERIFY(searchBox->styleSheet().contains(focusRingBorderTemplate
                                                 .arg(UiConfig::DockTextInputUnfocusedOutlineStrokeWidth)
                                                 .arg(textInputOutlineFill.name())));
    QVERIFY(searchBox->styleSheet().contains(QStringLiteral("QLineEdit:focus")));
    QVERIFY(searchBox->styleSheet().contains(focusRingBorderTemplate
                                                 .arg(UiConfig::OutlineStrokeWidth)
                                                 .arg(primary.name())));
    QVERIFY(searchBox->styleSheet().contains(primary.name()));
    QVERIFY(searchBox->property("speedcrunchDockTextInput").toBool());
    QTRY_VERIFY(editorHasPrimaryOutline(editor, primary));
    Editor inactiveEditor;
    inactiveEditor.setThemePrimaryColor(primary, true);
    QVERIFY(inactiveEditor.styleSheet().contains(focusRingBorderTemplate
                                                     .arg(UiConfig::OutlineStrokeWidth)
                                                     .arg(primary.name())));
    inactiveEditor.setThemePrimaryColor(primary, false);
    QVERIFY(inactiveEditor.styleSheet().contains(QStringLiteral("color: %1;").arg(primary.name())));
    QVERIFY2(!inactiveEditor.styleSheet().contains(focusRingBorderTemplate
                                                       .arg(UiConfig::OutlineStrokeWidth)
                                                       .arg(primary.name())),
             qPrintable(inactiveEditor.styleSheet()));
    QCOMPARE(editor->cursorColor().name(), primary.name());
    editor->setText(QStringLiteral("123"));
    editor->setCursorPosition(editor->text().size());
    QCoreApplication::processEvents();
    const QRect editorNativeCursorRect = editor->cursorRect();
    QVERIFY(editorNativeCursorRect.isValid());
    QVERIFY(editorNativeCursorRect.height() > 0);
    const QRect editorCursorRect(
        editorNativeCursorRect.x() + (editorNativeCursorRect.width() - 2) / 2,
        editorNativeCursorRect.y(),
        2,
        editorNativeCursorRect.height());
    const QImage focusedEditorImage = editor->viewport()->grab().toImage();
    QVERIFY(focusedEditorImage.rect().contains(editorCursorRect.center()));
    for (int x = editorCursorRect.left(); x <= editorCursorRect.right(); ++x)
        QCOMPARE(focusedEditorImage.pixelColor(x, editorCursorRect.center().y()).name(), primary.name());
    const int editorBlinkTimeout = qMax(1000, QApplication::cursorFlashTime() + 250);
    const auto editorCursorPixelName = [&]() {
        return editor->viewport()->grab().toImage().pixelColor(editorCursorRect.center()).name();
    };
    QTRY_VERIFY_WITH_TIMEOUT(editorCursorPixelName() != primary.name(), editorBlinkTimeout);
    QTRY_COMPARE_WITH_TIMEOUT(editorCursorPixelName(), primary.name(), editorBlinkTimeout);
    QVERIFY(QMetaObject::invokeMethod(&window,
                                      "handleApplicationFocusChanged",
                                      Qt::DirectConnection,
                                      Q_ARG(QWidget*, editor),
                                      Q_ARG(QWidget*, searchBox)));
    constantsDock->show();
    constantsDock->raise();
    constantsDock->setMinimumWidth(UiConfig::ConstantsDockDefaultWidth);
    window.resizeDocks(QList<QDockWidget*> { constantsDock },
                       QList<int> { UiConfig::ConstantsDockDefaultWidth },
                       Qt::Horizontal);
    QCoreApplication::processEvents();
    const auto visibleComboTextPixelCount = [&]() {
        const QImage image = comboBox->grab().toImage();
        int textPixels = 0;
        for (int y = 4; y < image.height() - 4; ++y) {
            for (int x = 8; x < image.width() - 40; ++x) {
                if (colorsAreClose(image.pixelColor(x, y), contentText, 42))
                    ++textPixels;
            }
        }
        return textPixels;
    };
    QTRY_VERIFY2(visibleComboTextPixelCount() > 8,
                 qPrintable(QStringLiteral("visible text pixels=%1 width=%2")
                                .arg(visibleComboTextPixelCount())
                                .arg(comboBox->width())));
    searchBox->clear();
    QVERIFY2(searchBox->focusPolicy() != Qt::NoFocus,
             qPrintable(QStringLiteral("focusPolicy=%1").arg(int(searchBox->focusPolicy()))));
    editor->setFocus(Qt::OtherFocusReason);
    QTRY_VERIFY(editor->hasFocus());
    QTest::mouseClick(searchBox, Qt::LeftButton);
    QTRY_VERIFY(searchBox->hasFocus());
    QCOMPARE(themedSearchBox->cursorColor().name(), primary.name());
    QTest::keyClicks(searchBox, "2323123das23");
    QCOMPARE(searchBox->text(), QStringLiteral("2323123das23"));
    const QRect nativeCursorRect =
        themedSearchBox->inputMethodQuery(Qt::ImCursorRectangle).toRect();
    QVERIFY(nativeCursorRect.isValid());
    QVERIFY(nativeCursorRect.height() > 0);
    const QRect cursorRect(nativeCursorRect.x() + (nativeCursorRect.width() - 2) / 2 + 1,
                           nativeCursorRect.y(),
                           2,
                           nativeCursorRect.height());
    const QImage focusedSearchImage = themedSearchBox->grab().toImage();
    QVERIFY(focusedSearchImage.rect().contains(cursorRect.center()));
    for (int x = cursorRect.left(); x <= cursorRect.right(); ++x)
        QCOMPARE(focusedSearchImage.pixelColor(x, cursorRect.center().y()).name(), primary.name());
    const QPoint leftOfCursor(cursorRect.left() - 1, cursorRect.center().y());
    const QPoint rightOfCursor(cursorRect.right() + 1, cursorRect.center().y());
    if (focusedSearchImage.rect().contains(leftOfCursor))
        QVERIFY(focusedSearchImage.pixelColor(leftOfCursor).name() != primary.name());
    if (focusedSearchImage.rect().contains(rightOfCursor))
        QVERIFY(focusedSearchImage.pixelColor(rightOfCursor).name() != primary.name());
    const int blinkTimeout = qMax(1000, QApplication::cursorFlashTime() + 250);
    const auto cursorPixelName = [&]() {
        return themedSearchBox->grab().toImage().pixelColor(cursorRect.center()).name();
    };
    QTRY_VERIFY_WITH_TIMEOUT(cursorPixelName() != primary.name(), blinkTimeout);
    QTRY_COMPARE_WITH_TIMEOUT(cursorPixelName(), primary.name(), blinkTimeout);
    const QImage visibleAgainSearchImage = themedSearchBox->grab().toImage();
    if (visibleAgainSearchImage.rect().contains(leftOfCursor))
        QVERIFY(visibleAgainSearchImage.pixelColor(leftOfCursor).name() != primary.name());
    if (visibleAgainSearchImage.rect().contains(rightOfCursor))
        QVERIFY(visibleAgainSearchImage.pixelColor(rightOfCursor).name() != primary.name());
    searchBox->clear();
    QTest::keyClicks(searchBox, "mol");
    QCOMPARE(searchBox->text(), QStringLiteral("mol"));
    searchBox->clear();
    comboBox->showPopup();
    QTRY_VERIFY(comboBox->view()->isVisible());
    QCOMPARE(comboBox->view()->frameShape(), QFrame::NoFrame);
    QWidget* comboPopupChrome = comboBox->view()->window();
    if (comboPopupChrome == comboBox->window())
        comboPopupChrome = comboBox->view();
    QVERIFY(comboPopupChrome != nullptr);
    QTRY_VERIFY(!comboPopupChrome->mask().isEmpty());
    comboBox->hidePopup();
    QTRY_VERIFY(!comboBox->view()->isVisible());
    QCOMPARE(searchLabel->palette().color(QPalette::Window).name(), contentFill.name());
    QCOMPARE(searchLabel->palette().color(QPalette::WindowText).name(), contentText.name());
    QVERIFY(searchLabel->styleSheet().contains(contentFill.name()));
    QVERIFY(searchLabel->styleSheet().contains(contentText.name()));
    QCOMPARE(table->palette().color(QPalette::Base).name(), contentFill.name());
    QCOMPARE(table->palette().color(QPalette::Text).name(), contentText.name());
    QCOMPARE(table->property("dockListHoverBackground").value<QColor>().name(),
             hoveredItemFill.name());
    QCOMPARE(table->property("dockListHoverForeground").value<QColor>().name(),
             hoveredItemText.name());
    QCOMPARE(table->viewport()->palette().color(QPalette::Base).name(), contentFill.name());
    QVERIFY(table->styleSheet().contains(QStringLiteral("padding: %1px").arg(UiConfig::DockListPadding)));
    QVERIFY(table->styleSheet().contains(QStringLiteral("border: 0")));
    QCOMPARE(table->frameShape(), QFrame::NoFrame);
    QCOMPARE(table->property("dockListInactiveSelectionBackground").value<QColor>().name(),
             controlFill.name());
    QCOMPARE(table->property("dockListInactiveSelectionForeground").value<QColor>().name(),
             controlText.name());
    const QString tableScrollBarStyle = table->verticalScrollBar()->styleSheet();
    QVERIFY(tableScrollBarStyle.contains(contentFill.name()));
    QVERIFY(tableScrollBarStyle.contains(titleFill.name()));
    QVERIFY(tableScrollBarStyle.contains(controlFill.name()));
    QVERIFY(tableScrollBarStyle.contains(hoverFill.name()));
    QTRY_VERIFY(table->topLevelItemCount() > 0);
    QTreeWidgetItem* tooltipItem = table->topLevelItem(0);
    QVERIFY(tooltipItem != nullptr);
    table->scrollToItem(tooltipItem);
    QCoreApplication::processEvents();
    const QRect tooltipRect = table->visualItemRect(tooltipItem);
    QVERIFY(tooltipRect.isValid());
    const QPoint tooltipPos = tooltipRect.center();
    QMouseEvent tooltipMoveEvent(QEvent::MouseMove,
                                 QPointF(tooltipPos),
                                 QPointF(table->viewport()->mapToGlobal(tooltipPos)),
                                 Qt::NoButton,
                                 Qt::NoButton,
                                 Qt::NoModifier);
    QCoreApplication::sendEvent(table->viewport(), &tooltipMoveEvent);
    QFrame* summaryPopup =
        constantsWidget->findChild<QFrame*>(QStringLiteral("constantsSummaryPopup"));
    QTRY_VERIFY(summaryPopup != nullptr && summaryPopup->isVisible());
    QLabel* summaryPopupLabel =
        summaryPopup->findChild<QLabel*>(QStringLiteral("constantsSummaryPopupLabel"));
    QVERIFY(summaryPopupLabel != nullptr);
    QCOMPARE(summaryPopup->palette().color(QPalette::Window).name(),
             completionPopupFill.name());
    QCOMPARE(summaryPopup->palette().color(QPalette::WindowText).name(),
             completionPopupText.name());
    QCOMPARE(summaryPopupLabel->palette().color(QPalette::WindowText).name(),
             completionPopupText.name());
    QVERIFY(summaryPopup->styleSheet().contains(completionPopupFill.name()));
    QVERIFY(summaryPopup->styleSheet().contains(completionPopupText.name()));
    QVERIFY(summaryPopup->styleSheet().contains(
        QStringLiteral("border: %1px solid %2")
            .arg(UiConfig::PopupOutlineStrokeWidth)
            .arg(completionPopupOutlineFill.name())));
    QVERIFY(summaryPopup->styleSheet().contains(
        QStringLiteral("border-radius: %1px")
            .arg(UiConfig::CompletionPopupCornerRadius)));
    QVERIFY(!summaryPopup->mask().isEmpty());
    summaryPopup->hide();
    const QMargins dockRootMargins = constantsDock->widget()->layout()->contentsMargins();
    QCOMPARE(dockRootMargins, QMargins(0, 0, 0, 0));
    const QMargins searchRowMargins =
        searchLabel->parentWidget()->layout()->contentsMargins();
    QCOMPARE(searchRowMargins, QMargins(8, UiConfig::DockSearchRowVerticalPadding,
                                      8, UiConfig::DockSearchRowVerticalPadding));
    QVERIFY(searchLabel->parentWidget()->styleSheet().contains(contentFill.name()));
    bool foundStyledDockMenu = false;
    for (QMenu* menu : window.findChildren<QMenu*>()) {
        if (!menu->styleSheet().contains(titleFill.name()))
            continue;
        foundStyledDockMenu = true;
        QVERIFY(menu->styleSheet().contains(titleText.name()));
        QVERIFY(menu->styleSheet().contains(controlFill.name()));
        QVERIFY(menu->styleSheet().contains(controlText.name()));
        QVERIFY(menu->styleSheet().contains(QStringLiteral("border-radius: 8px")));
    }
    QVERIFY(foundStyledDockMenu);

    searchBox->setText(QStringLiteral("no-such-constant-filter-value"));
    QTest::qWait(650);
    QCoreApplication::processEvents();
    QLabel* noMatchLabel = nullptr;
    for (QLabel* label : constantsDock->findChildren<QLabel*>()) {
        if (label->text() == QStringLiteral("No match found")) {
            noMatchLabel = label;
            break;
        }
    }
    QVERIFY(noMatchLabel != nullptr);
    QVERIFY(!noMatchLabel->isHidden());
    QCOMPARE(noMatchLabel->palette().color(QPalette::WindowText).name(), contentText.name());
    QVERIFY(noMatchLabel->styleSheet().contains(contentText.name()));
    QCOMPARE(table->topLevelItemCount(), 0);
    QVERIFY2(table->header()->length() >= table->header()->width() - 1,
             qPrintable(QStringLiteral("length=%1 header=%2 sections=%3,%4,%5")
                            .arg(table->header()->length())
                            .arg(table->header()->width())
                            .arg(table->header()->sectionSize(0))
                            .arg(table->header()->sectionSize(1))
                            .arg(table->header()->sectionSize(2))));
    searchBox->clear();
    QTest::qWait(650);
    QCoreApplication::processEvents();
    QVERIFY(table->topLevelItemCount() > 0);
    QVERIFY(!table->header()->stretchLastSection());

    QLabel* domainLabel = nullptr;
    for (QLabel* label : functionsDock->findChildren<QLabel*>()) {
        if (label->text() == QStringLiteral("Domain")) {
            domainLabel = label;
            break;
        }
    }
    QVERIFY(domainLabel != nullptr);
    QCOMPARE(domainLabel->palette().color(QPalette::Window).name(), contentFill.name());
    QCOMPARE(domainLabel->palette().color(QPalette::WindowText).name(), contentText.name());
    QVERIFY(domainLabel->styleSheet().contains(contentFill.name()));
    QVERIFY(domainLabel->styleSheet().contains(contentText.name()));
    const QMargins domainRowMargins =
        domainLabel->parentWidget()->layout()->contentsMargins();
    QCOMPARE(domainRowMargins, QMargins(8, 6, 8, 0));
    QVERIFY(domainLabel->parentWidget()->styleSheet().contains(contentFill.name()));

    constantsDock->show();
    constantsDock->raise();
    QCoreApplication::processEvents();
    const QImage searchImage = searchBox->grab().toImage();
    QVERIFY(!searchImage.isNull());
    QCOMPARE(searchImage.pixelColor(searchImage.width() / 2, searchImage.height() / 2).name(),
             contentFill.name());

    QDockWidget* bookDock = window.findChild<QDockWidget*>(QStringLiteral("BookDock"));
    QVERIFY(bookDock != nullptr);
    QVERIFY(bookDock->widget() != nullptr);
    QVERIFY(bookDock->widget()->layout() != nullptr);
    QCOMPARE(bookDock->widget()->layout()->contentsMargins(), QMargins(0, 0, 0, 0));
    QTextBrowser* bookBrowser = bookDock->findChild<QTextBrowser*>();
    QVERIFY(bookBrowser != nullptr);
    QCOMPARE(bookBrowser->palette().color(QPalette::Base).name(), contentFill.name());
    QCOMPARE(bookBrowser->palette().color(QPalette::Text).name(), contentText.name());
    QCOMPARE(bookBrowser->viewport()->palette().color(QPalette::Base).name(), contentFill.name());
    QVERIFY(bookBrowser->toHtml().contains(contentFill.name()));
    const QColor expectedBookSectionLink = generateSecondaryLinkFromBackground(
        contentFill,
        aaForegroundForBackground(contentFill, bookBrowser->palette().color(QPalette::Link)));
    QVERIFY(bookBrowser->toHtml().contains(expectedBookSectionLink.name()));
    QVERIFY(!bookBrowser->toHtml().contains(QStringLiteral("#555555")));
    const QColor expectedBookFormulaLink = aaForegroundForBackground(
        contentFill, bookBrowser->palette().color(QPalette::Link));
    QVERIFY(QMetaObject::invokeMethod(bookDock,
                                      "openPage",
                                      Q_ARG(QUrl, QUrl(QStringLiteral("geometry/sector")))));
    QVERIFY(bookBrowser->toHtml().contains(expectedBookFormulaLink.name()));
    const QString bookScrollBarStyle = bookBrowser->verticalScrollBar()->styleSheet();
    QVERIFY(bookScrollBarStyle.contains(contentFill.name()));
    QVERIFY(bookScrollBarStyle.contains(titleFill.name()));
    QVERIFY(bookScrollBarStyle.contains(controlFill.name()));
    QVERIFY(bookScrollBarStyle.contains(hoverFill.name()));

    bool foundDockTabs = false;
    for (QTabBar* tabBar : window.findChildren<QTabBar*>()) {
        bool dockNavigationTabBar = false;
        for (int i = 0; i < tabBar->count(); ++i) {
            const QString text = tabBar->tabText(i);
            if (text == QStringLiteral("Constants") || text == QStringLiteral("Functions")) {
                dockNavigationTabBar = true;
                break;
            }
        }
        if (dockNavigationTabBar
            && tabBar->isVisible()
            && tabBar->styleSheet().contains(titleFill.name())) {
            foundDockTabs = true;
            QCOMPARE(tabBar->palette().color(QPalette::WindowText).name(), titleText.name());
            QVERIFY(tabBar->styleSheet().contains(QStringLiteral("background-color: transparent")));
            QVERIFY(tabBar->styleSheet().contains(chromeFill.name()));
            QVERIFY(tabBar->styleSheet().contains(chromeText.name()));
            QVERIFY(tabBar->styleSheet().contains(resultFill.name()));
            QVERIFY(tabBar->styleSheet().contains(resultText.name()));
            QVERIFY(tabBar->styleSheet().contains(titleFill.name()));
            QVERIFY(tabBar->styleSheet().contains(titleText.name()));
            QVERIFY(tabBar->styleSheet().contains(QStringLiteral("padding: 5px 14px")));
            QVERIFY(tabBar->styleSheet().contains(QStringLiteral("margin: 2px 1px")));
            QVERIFY(tabBar->property("speedcrunchDockSystemTabBar").toBool());
            QVERIFY(tabBar->hasMouseTracking());
            QVERIFY(!tabBar->drawBase());
            int hoveredTab = -1;
            for (int i = 0; i < tabBar->count(); ++i) {
                if (tabBar->tabText(i) == QStringLiteral("Constants")
                    || tabBar->tabText(i) == QStringLiteral("Functions")) {
                    hoveredTab = i;
                    break;
                }
            }
            QVERIFY(hoveredTab >= 0);
            const QPoint hoverPos = tabBar->tabRect(hoveredTab).center();
            QCOMPARE(tabBar->tabAt(hoverPos), hoveredTab);
            QMouseEvent moveEvent(QEvent::MouseMove,
                                  QPointF(hoverPos),
                                  QPointF(tabBar->mapToGlobal(hoverPos)),
                                  Qt::NoButton,
                                  Qt::NoButton,
                                  Qt::NoModifier);
            QCoreApplication::sendEvent(tabBar, &moveEvent);
            QTRY_COMPARE(tabBar->cursor().shape(), Qt::PointingHandCursor);
            QWidget* tabBarParent = tabBar->parentWidget();
            QVERIFY(tabBarParent != nullptr);
            QCOMPARE(tabBarParent->palette().color(QPalette::Window).name(), chromeFill.name());
            QVERIFY(tabBarParent->styleSheet().contains(chromeFill.name()));
            const QImage tabBarImage = tabBar->grab().toImage();
            QVERIFY(!tabBarImage.isNull());
            QCOMPARE(tabBarImage.pixelColor(tabBarImage.width() - 1,
                                            tabBarImage.height() / 2).name(),
                     chromeFill.name());
            QCOMPARE(tabBarImage.pixelColor(tabBarImage.width() - 1, 0).name(),
                     chromeFill.name());
        }
    }
    QVERIFY(foundDockTabs);

    settings->customColorSchemeJson = themeJsonString(QJsonObject{{QStringLiteral("background"), QStringLiteral("#300a24")}});
    window.colorSchemeChanged();
    QCoreApplication::processEvents();

    const QVector<QColor> changedShades =
        generateOklchShades(QColor(QStringLiteral("#300a24")), 6, ThemePolarity::Dark);
    const QVector<QColor> changedForegrounds = aaForegroundsForBackgrounds(changedShades);
    const QColor changedContentFill = changedShades.at(2);
    const QColor changedContentText = changedForegrounds.at(2);
    const QColor changedTitleFill = changedShades.at(3);
    const QColor changedTitleText = changedForegrounds.at(3);
    const QColor changedPrimary = generatePrimaryFromBackground(QColor(QStringLiteral("#300a24")));

    QTRY_VERIFY(constantsDock->styleSheet().contains(changedTitleFill.name()));
    QVERIFY(constantsDock->styleSheet().contains(changedTitleText.name()));
    QVERIFY(!constantsDock->styleSheet().contains(titleFill.name()));
    bool foundUpdatedDockTab = false;
    for (QTabBar* tabBar : window.findChildren<QTabBar*>()) {
        if (!tabBar->styleSheet().contains(changedTitleFill.name()))
            continue;
        foundUpdatedDockTab = true;
        QVERIFY(tabBar->styleSheet().contains(changedTitleText.name()));
    }
    QVERIFY(foundUpdatedDockTab);
    QCOMPARE(constantsDock->widget()->palette().color(QPalette::Window).name(),
             changedContentFill.name());
    QVERIFY(constantsDock->widget()->styleSheet().contains(changedContentFill.name()));
    QVERIFY(!constantsDock->widget()->styleSheet().contains(contentFill.name()));
    QCOMPARE(searchLabel->palette().color(QPalette::Window).name(),
             changedContentFill.name());
    QCOMPARE(searchLabel->palette().color(QPalette::WindowText).name(),
             changedContentText.name());
    QVERIFY(searchLabel->parentWidget()->styleSheet().contains(changedContentFill.name()));
    QCOMPARE(domainLabel->palette().color(QPalette::Window).name(),
             changedContentFill.name());
    QCOMPARE(domainLabel->palette().color(QPalette::WindowText).name(),
             changedContentText.name());
    QVERIFY(domainLabel->parentWidget()->styleSheet().contains(changedContentFill.name()));
    QCOMPARE(searchBox->palette().color(QPalette::Base).name(), changedContentFill.name());
    QCOMPARE(searchBox->palette().color(QPalette::Text).name(), changedContentText.name());
    QCOMPARE(themedSearchBox->cursorColor().name(), changedPrimary.name());
    QVERIFY(searchBox->styleSheet().contains(QStringLiteral("border: %1px solid %2")
                                                 .arg(UiConfig::OutlineStrokeWidth)
                                                 .arg(changedPrimary.name())));
    QCOMPARE(comboBox->palette().color(QPalette::Button).name(), changedContentFill.name());
    QCOMPARE(comboBox->palette().color(QPalette::ButtonText).name(), changedContentText.name());
    QCOMPARE(table->palette().color(QPalette::Base).name(), changedContentFill.name());
    QCOMPARE(table->viewport()->palette().color(QPalette::Base).name(),
             changedContentFill.name());
    QVERIFY(table->styleSheet().contains(changedContentFill.name()));
}

void TestThemesUi::repeated_theme_refresh_keeps_structural_styles()
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
    MainWindowStateGuard guard;
    guard.settings->dockAppearance = Settings::DockAppearanceSpeedCrunch;
    guard.settings->sessionLayoutJson.clear();
    guard.settings->constantsDockVisible = true;
    guard.settings->functionsDockVisible = true;
    guard.settings->historyDockVisible = true;
    guard.settings->formulaBookDockVisible = true;
    guard.settings->variablesDockVisible = true;
    guard.settings->userFunctionsDockVisible = true;
    guard.settings->userUnitsDockVisible = true;
    guard.settings->bitfieldVisible = true;
    MainWindow window(false);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QCoreApplication::processEvents();
    window.installEventFilter(&counter);
    int surfaces = 0;
    for (QWidget* widget : window.findChildren<QWidget*>()) {
        if (widget->property("speedcrunchDockSurface").toBool()) {
            widget->installEventFilter(&counter);
            ++surfaces;
        }
    }
    QVERIFY(surfaces > 0);

    for (int i = 0; i < 3; ++i)
        QVERIFY(QMetaObject::invokeMethod(&window, "colorSchemeChanged", Qt::DirectConnection));
    QCoreApplication::processEvents();
    QCOMPARE(counter.changes, 0);
}

int main(int argc, char** argv)
{
    return UiTestFixture::run<TestThemesUi>(argc, argv);
}

#include "testthemesui.moc"
