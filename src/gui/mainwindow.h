// SPDX-FileCopyrightText: 2007-2010, 2012-2018, 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later


#ifndef GUI_MAINWINDOW_H
#define GUI_MAINWINDOW_H

#include "core/settings.h"
#include "gui/keypad.h"
#include "math/quantity.h"

#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QMainWindow>
#include <QPair>
#include <QPointer>
#include <QStringList>

class AutoHideLabel;
class BitFieldWidget;
template <class Widget> class GenericDock;
class BookDock;
class Constants;
class ConstantsWidget;
class Editor;
class Evaluator;
class HistoryEntry;
class UserFunction;
class FunctionRepo;
class FunctionsWidget;
class HistoryWidget;
class ManualWindow;
class ManualServer;
class ResultDisplay;
class Session;
class UserFunctionListWidget;
class UserUnitListWidget;
class Variable;
class VariableListWidget;
#ifdef SPEEDCRUNCH_ENABLE_UPDATE_CHECKS
class VersionCheck;
#endif

class QAbstractItemView;
class QActionGroup;
class QDockWidget;
class QHBoxLayout;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QResizeEvent;
class QSplitter;
class QTabBar;
class QTimer;
class QTranslator;
class QVBoxLayout;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    static void initializeApplicationStyle();
    explicit MainWindow(bool restorePreviousSession = true);
    ~MainWindow();
    void showWhenReady();
    void persistSessionAndSettingsForShutdown();
    static MainWindow* subsequentLaunchWindow();
    static bool sessionRestoreInProgress();
    MainWindow* createNewSessionWindow(bool activate = true);
    void createNewSessionTab();

signals:
    void angleUnitChanged();
    void complexNumbersChanged();
    void colorSchemeChanged();
    void menuAppearanceChanged();
    void languageChanged();
    void radixCharacterChanged();
    void resultFormatChanged();
    void resultPrecisionChanged();
    void resultRoundingModeChanged();
    void syntaxHighlightingChanged();
    void historyChanged();
    void variablesChanged();
    void functionsChanged();
    void unitsChanged();

public slots:
    void setVisible(bool visible) override;
    void copy();

private slots:
    void clearEditor();
    void clearEditorAndBitfield();
    void clearHistory();
    void clearSession();
    void resetAllSettings();
    void copyResultToClipboard();
    void decreaseDisplayFontPointSize();
    void decreaseOpacity();
    void deleteVariables();
    void deleteUserFunctions();
    void evaluateEditorExpression();
    void handleBulkEvaluationStarted();
    void handleBulkEvaluationFinished();
    void handleApplicationFocusChanged(QWidget* previous, QWidget* focused);
    void cancelHistoryEntryEdit();
    void exportJson();
    void exportHtml();
    void exportPlainText();
    void handleAutoCalcMessageAvailable(const QString&);
    void handleAutoCalcQuantityAvailable(const Quantity&);
    void handleCopyAvailable(bool copyAvailable);
    void handleBitsChanged(const QString&);
    void handleKeypadButtonPress(Keypad::Button);
    void handleEditorTextChange();
    void handleEditorEscapePressed();
    void showNewSessionDialog();
    void showNewPrivateSession();
    void convertSessionPrivacy();
    void showNewSessionWindow();
    void showOpenSessionDialog();
    void showDuplicateSessionDialog();
    void showRenameSessionDialog();
    void closeCurrentSession();
    void restoreClosedSessionTab();
    void restoreWindowGeometry(const QByteArray& geometry);
    void restoreWindowLayoutState(const QByteArray& state);
    void restoreWindowKeypadLayout(bool visible, int mode);
    void restoreWindowKeypadZoom(int zoomPercent);
    void restoreWindowUiState(const QJsonObject& window, bool restoreLayout = true);
    void showRestoredWindow(const QByteArray& geometry);
    void cycleFocusForward();
    void cycleFocusBackward();
    void closeCurrentPane();
    void deleteCurrentSession();
    void splitActivePaneLeft();
    void splitActivePaneRight();
    void splitActivePaneUp();
    void splitActivePaneDown();
    void activateNextChild();
    void activatePreviousChild();
    void showLoadedSessionsMenu(const QPoint& globalPos);
    void handleCustomKeypadButtonPress(int action, const QString& text);
    void handleDisplaySelectionChange();
    void handleEditorSelectionChange();
    void handleManualClosed();
    void handleDockWidgetVisibilityChanged(bool visible);
    void hideStateLabel();
    void increaseDisplayFontPointSize();
    void increaseOpacity();
    void insertConstantIntoEditor(const QString&);
    void insertFunctionIntoEditor(const QString&);
    void insertTextIntoEditor(const QString&);
    void insertHistoryExpressionIntoEditor(const QString&);
    void insertVariableIntoEditor(const QString&);
    void insertUserFunctionIntoEditor(const QString&);
    void insertUserUnitIntoEditor(const QString&);
#ifdef SPEEDCRUNCH_ENABLE_UPDATE_CHECKS
    void checkForUpdates();
#endif
    void openFeedbackURL();
    void openCommunityURL();
    void openSourceURL();
    void openDonateURL();
    void retranslateText();
    void selectEditorExpression();
    void setAlwaysOnTopEnabled(bool);
    void setAngleModeDegree();
    void setAngleModeRadian();
    void setAngleModeGradian();
    void setAngleModeTurn();
    void setAngleModeRevolution();
    void setAutoAnsEnabled(bool);
    void setAutoCalcEnabled(bool);
    void setAutoCompletionEnabled(bool);
    void setAutoCompletionAcceptWithEnterEnabled(bool);
    void setAutoCompletionBuiltInFunctionsEnabled(bool);
    void setAutoCompletionBuiltInVariablesEnabled(bool);
    void setAutoCompletionLongFormUnitsEnabled(bool);
    void setAutoCompletionUserFunctionsEnabled(bool);
    void setAutoCompletionUserVariablesEnabled(bool);
    void setBitfieldVisible(bool);
    void setHistorySizeLimit();
    void setConstantsDockVisible(bool, bool takeFocus = true);
    void setFormulaBookDockVisible(bool, bool takeFocus = true);
    void setFullScreenEnabled(bool);
    void setFunctionsDockVisible(bool, bool takeFocus = true);
    void setHistoryDockVisible(bool, bool takeFocus = true);
    void setKeypadMode(QAction*);
    void setMenuAppearance(QAction*);
    void setSystemColorMode(QAction*);
    void setDockAppearance(QAction*);
    void setStatusBarAppearance(QAction*);
    void setKeypadAppearance(QAction*);
    void applyKeypadAppearance();
    void setEditorDisplayStyle(QAction*);
    void setKeypadZoom(QAction*);
    void setKeypadVisible(bool);
    void setLeaveLastExpressionEnabled(bool);
    void setUpDownArrowBehavior(QAction*);
    void setRadixCharacterAutomatic();
    void setRadixCharacter(char);
    void setRadixCharacterComma();
    void setRadixCharacterDot();
    void setRadixCharacterBoth();
    void setResultFormatBinary();
    void setResultFormatCartesian();
    void setResultFormat(char);
    void setResultFormatEngineering();
    void setResultFormatFixed();
    void setResultFormatGeneral();
    void setResultFormatHexadecimal();
    void setImaginaryUnitI();
    void setImaginaryUnitJ();
    void setResultFormatOctal();
    void setResultFormatPolar();
    void setResultFormatTrigonometric();
    void setResultFormatCis();
    void setResultFormatPolarAngle();
    void setResultFormatRational();
    void setResultFormatScientific();
    void setResultFormatSexagesimal();
    void setUnitNegativeExponentStyle(QAction*);
    void setResultPrecision15Digits();
    void setResultPrecision2Digits();
    void setResultPrecision3Digits();
    void setResultPrecision50Digits();
    void setResultPrecision8Digits();
    void setResultPrecisionAutomatic();
    void setResultPrecisionCustom();
    void setResultPrecision(int);
    void setResultRoundingMode(QAction*);
    void setRemoveTrailingZeroesEnabled(bool);
    void setMenuBarVisible(bool);
    void setStatusBarVisible(bool);
    void setSyntaxHighlightingEnabled(bool);
    void setInputDigitGroupingEnabled(bool);
    void setAutomaticInputAdjustmentsEnabled(bool);
    void setCursorBlinkingEnabled(bool);
    void setEscapeClearsEditorEnabled(bool);
    void setDigitGrouping(QAction*);
    void setDigitGroupingIntegerPartOnlyEnabled(bool);
    void setAutoResultToClipboardEnabled(bool);
    void setSimplifyResultExpressionsEnabled(bool);
    void setHoverHighlightResultsEnabled(bool);
    void setVariablesDockVisible(bool, bool takeFocus = true);
    void setUserFunctionsDockVisible(bool, bool takeFocus = true);
    void setUserUnitsDockVisible(bool, bool takeFocus = true);
    void setWindowPositionSaveEnabled(bool);
    void setOpenPrivateTabOnStartupEnabled(bool);
    void setSubsequentLaunchAction(QAction*);
    void setWidgetsDirection();
    void showAboutDialog();
    void showDiagnosticsDialog();
    void showStateLabel(const QString&);
    void showFontDialog();
    void showLanguageChooserDialog();
    void showManualWindow();
    void showNumberFormatDialog();
    void showResultSlotsDialog();
    void showCustomThemeDialog();
    void showContextHelp();
    void showAngleModeContextMenu(const QPoint&);
    void showPrecisionContextMenu(const QPoint&);
    void showKeypadContextMenu(const QPoint&);
    void showResultFormatContextMenu(const QPoint&);
    void showSessionImportDialog();
    void showUserDefinitionsImportDialog();
    void openSessionsFolder();
    void wrapSelection();
    void startHistoryEntryEdit(int index);
    void editHistoryEntryContext(int index);
    void removeHistoryEntryAt(int index);
    void removeHistoryEntriesAbove(int index);
    void removeHistoryEntriesBelow(int index);

protected:
    bool event(QEvent*) override;
    void closeEvent(QCloseEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    bool eventFilter(QObject*, QEvent*) override;

private:
    Q_DISABLE_COPY(MainWindow)

    void clearTextEditSelection(QPlainTextEdit*);
    void hideCurrentResultPreview();
    void addTabifiedDock(QDockWidget*, bool takeFocus, Qt::DockWidgetArea = Qt::RightDockWidgetArea);
    void applyDockCornerLayout();
    void deleteDock(QDockWidget*);
    void createUi();
    void createActions();
    void createActionGroups();
    void createActionShortcuts();
    void createMenus();
    void createStatusBar();
    void createFixedWidgets();
    void createKeypad();
    void createBitField();
    void createBookDock(bool takeFocus = true);
    void createConstantsDock(bool takeFocus = true);
    void createFunctionsDock(bool takeFocus = true);
    void createHistoryDock(bool takeFocus = true);
    void createVariablesDock(bool takeFocus = true);
    void createUserFunctionsDock(bool takeFocus = true);
    void createUserUnitsDock(bool takeFocus = true);
    void createFixedConnections();
    void applySettings(bool initializing = true);
    void setDisplayFont(const QFont& font);
    void applyEditorDisplayStyle();
    void checkInitialResultFormat();
    void checkInitialComplexFormat();
    void checkInitialImaginaryUnit();
    void checkInitialResultPrecision();
    void checkInitialLanguage();
    void checkInitialDigitGrouping();
    void restoreSession(bool restoreHistory = true);
    bool restoreSessionLayout(bool restoreHistory);
    void deleteKeypad(bool deferredDeletion = true);
    void deleteStatusBar();
    void deleteBitField();
    void deleteBookDock();
    void deleteConstantsDock();
    void deleteFunctionsDock();
    void deleteHistoryDock();
    void deleteVariablesDock();
    void deleteUserFunctionsDock();
    void deleteUserUnitsDock();
    void saveSettings();
    void saveSessionToDefaultPath();
    void saveSession(QString &fname);
    void finishRestoreSessionLayout(const QJsonObject& layout,
                                    const QJsonObject& window,
                                    const QJsonObject& root,
                                    const QJsonArray& tabs,
                                    const QString& activeSessionName,
                                    bool restoreHistory,
                                    QHash<QString, QJsonObject> sessionJsons,
                                    QHash<QString, QPair<int, int>> viewportAnchors,
                                    QHash<QString, int> scrollValues);
    void openImportedSession(Session* session);
    void activateSession(Session* session);
    void captureEditorTextInCurrentSession();
    void restoreEditorTextFromCurrentSession();
    QWidget* createEditorDisplayPane(ResultDisplay* display, Editor* editor);
    void configureEditorDisplayPane(ResultDisplay* display, Editor* editor);
    void setActiveEditorDisplayPane(ResultDisplay* display, Editor* editor, bool forceEditorFocus = false);
    QAbstractItemView* dockItemViewFocusTarget(QWidget* widget) const;
    QDockWidget* dockWidgetForDescendant(QWidget* widget) const;
    bool isDockWidgetDescendant(QWidget* widget) const;
    bool isDockTextInput(QWidget* widget) const;
    void deactivateActiveEditorForTextInputFocus();
    QList<QWidget*> focusCycleTargets() const;
    bool focusWidgetMatchesCycleTarget(QWidget* focusWidget, QWidget* target) const;
    void cycleFocusRegion(int direction);
    void splitActivePane(Qt::Orientation orientation, bool insertAfter);
    Session* createUntitledSession(bool activateCreatedSession = true);
    QString firstAvailableSessionNameAcrossWindows(bool privateSession) const;
    void renameLoadedSession(Session* session, const QString& name);
    void discardUnusedPrivateSessions();
    void finishStartupSessionRestore(bool restoredSession);
    QString firstAvailableUntitledSessionNameAcrossWindows(const MainWindow* ignoredWindow = nullptr) const;
    void copyWindowLayoutFrom(const MainWindow* source);
    QList<ResultDisplay*> splitPaneDisplays() const;
    QList<Editor*> splitPaneEditors() const;
    QStringList paneSessionNames(ResultDisplay* display) const;
    void addSessionToActivePane(const QString& name);
    void updatePaneLoadedSessionCounts();
    void updatePaneEditorCursorVisibility();
    void updateActiveSessionPaneTabColor();
    void updatePaneTabBars();
    void updateSessionWindowTitle();
    ResultDisplay* tabBarDisplay(QTabBar* tabBar) const;
    QTabBar* displayTabBar(ResultDisplay* display) const;
    void switchPaneToSession(ResultDisplay* display, const QString& name);
    bool focusOpenSession(const QString& name);
    void moveSessionTab(QTabBar* sourceTabBar, QTabBar* targetTabBar, const QString& name, int targetIndex);
    void moveSessionTabToPane(QTabBar* sourceTabBar, ResultDisplay* targetDisplay, const QString& name, const QPoint& panePos);
    void splitPaneWithSession(QTabBar* sourceTabBar, ResultDisplay* targetDisplay, const QString& name, Qt::Orientation orientation, bool insertAfter);
    void rememberClosedSessionTab(ResultDisplay* display, const QString& name);
    void removeSessionTabFromPane(ResultDisplay* display, const QString& name, bool closePaneIfEmpty);
    void removePaneForDisplay(ResultDisplay* display);
    void normalizeSplitContainerTree();
    void applyThemeSurfacePalette();
    void restoreWindowThemeBackgrounds();
    void scheduleDockAppearanceRefresh(bool forceNativePolish = false);
    void applyDockTabBarAppearance();
    QStringList openDockNames() const;
    QStringList configuredOpenDockNames() const;
    void applyOpenDocks(const QStringList& names);
    void restoreDockLayout(const QByteArray& state, const QStringList& names);
    QJsonObject dockColumnOrders() const;
    void restoreDockColumnOrders(const QJsonObject& orders);
    void restoreDockColumnOrder(QDockWidget* dock);
    void applyKeypadThemeSurfacePalette();
    void scheduleThemeRuntimeDiagnosticsReport();
    void writeThemeRuntimeDiagnosticsReport();
    void updateSplitterStyleSheet();
    void refreshPaneThemes();
    void captureVisibleSessionViewports();
    void restoreVisibleSessionViewports();
    void saveSessionLayout(bool captureCurrentViewport = true, bool captureEditorState = false);
    void flushPendingSessionSave();
    bool configureCustomKeypad();
    void updateKeypadModeActionState();
    void setActionsText();
    void updateKeypadDisabledActionText();
    void setMenusText();
    void setStatusBarText();
    void applyStatusBarSelectionState();
    void setStatusBarSelectionActionState(char angleUnit, char resultFormat,
                                          int resultPrecision);
    void syncStatusBarSelectionMenuActionState();
    void syncViewMenuActionState();
    void syncEditMenuActionState();
    void syncSessionMenuActionState();
    void updateStatusBarSectionVisibility();
    void updateColorSchemeActionState();
    QString statusBarAngleUnitValue() const;
    QString statusBarResultPrecisionValue() const;
    QString statusBarResultFormatValue() const;
    void applyUserDefinitions(int* importedVariables = nullptr,
                              int* importedFunctions = nullptr,
                              int* importedUnits = nullptr,
                              int* ignoredLines = nullptr,
                              QList<int>* ignoredLineNumbers = nullptr);
    void importUserDefinitionsFromText(const QString& text, bool overwriteExisting,
                                       int* importedVariables = nullptr,
                                       int* importedFunctions = nullptr,
                                       int* importedUnits = nullptr,
                                       int* ignoredLines = nullptr,
                                       QList<int>* ignoredLineNumbers = nullptr,
                                       bool dryRun = false);
    bool rebuildSessionFromEntries(const QList<HistoryEntry>& entries,
                                   int startIndex = 0,
                                   int* errorIndex = nullptr,
                                   QString* errorText = nullptr);
    QList<HistoryEntry> historyEntries() const;

    static QTranslator* createTranslator(const QString& langCode);

    struct {
        QAction* sessionOpen;
        QAction* sessionRename;
        QAction* sessionClone;
        QAction* sessionClose;
        QAction* sessionReopen;
        QAction* sessionNewTab;
        QAction* sessionNewPrivate;
        QAction* sessionConvertPrivacy;
        QAction* sessionNewWindow;
        QAction* sessionOpenSessionsFolder;
        QAction* sessionImport;
        QAction* sessionImportUserDefinitions;
        QAction* sessionExportJson;
        QAction* sessionExportHtml;
        QAction* sessionExportPlainText;
        QAction* sessionQuit;
        QAction* editUndo;
        QAction* editRedo;
        QAction* editCut;
        QAction* editCopy;
        QAction* editCopyLastResult;
        QAction* editPaste;
        QAction* editSelectExpression;
        QAction* editInsertFunction;
        QAction* editInsertVariable;
        QAction* editDeleteVariable;
        QAction* editInsertUserFunction;
        QAction* editDeleteUserFunction;
        QAction* editClearExpression;
        QAction* editClearHistory;
        QAction* editWrapSelection;
        QAction* viewKeypadDisabled;
        QAction* viewKeypadBasicWide;
        QAction* viewKeypadScientificWide;
        QAction* viewKeypadScientificNarrow;
        QAction* viewKeypadCustom;
        QAction* viewKeypadZoom50;
        QAction* viewKeypadZoom100;
        QAction* viewKeypadZoom150;
        QAction* viewKeypadZoom200;
        QAction* viewFormulaBook;
        QAction* viewConstants;
        QAction* viewFunctions;
        QAction* viewVariables;
        QAction* viewUserFunctions;
        QAction* viewUserUnits;
        QAction* viewHistory;
        QAction* viewStatusBar;
        QAction* viewMenuBar;
        QAction* viewFullScreenMode;
        QAction* viewBitfield;
        QAction* viewSplitLeft;
        QAction* viewSplitRight;
        QAction* viewSplitUp;
        QAction* viewSplitDown;
        QAction* viewClosePane;
        QAction* settingsResultFormatGeneral;
        QAction* settingsResultFormatFixed;
        QAction* settingsResultFormatEngineering;
        QAction* settingsResultFormatScientific;
        QAction* settingsResultFormatRational;
        QAction* settingsResultFormatAutoPrecision;
        QAction* settingsResultFormat0Digits;
        QAction* settingsResultFormat2Digits;
        QAction* settingsResultFormat3Digits;
        QAction* settingsResultFormat8Digits;
        QAction* settingsResultFormat15Digits;
        QAction* settingsResultFormat50Digits;
        QAction* settingsResultFormatCustomDigits;
        QAction* settingsResultRoundingHalfAwayFromZero;
        QAction* settingsRemoveTrailingZeroes;
        QAction* settingsResultRoundingHalfEven;
        QAction* settingsResultRoundingTowardZero;
        QAction* settingsResultRoundingTowardPositiveInfinity;
        QAction* settingsResultRoundingTowardNegativeInfinity;
        QAction* settingsResultFormatBinary;
        QAction* settingsResultFormatOctal;
        QAction* settingsResultFormatCartesian;
        QAction* settingsResultFormatPolar;
        QAction* settingsResultFormatTrigonometric;
        QAction* settingsResultFormatCis;
        QAction* settingsResultFormatPolarAngle;
        QAction* settingsImaginaryUnitI;
        QAction* settingsImaginaryUnitJ;
        QAction* settingsResultFormatHexadecimal;
        QAction* settingsResultFormatSexagesimal;
        QAction* settingsUnitNegativeExponentSuperscript;
        QAction* settingsUnitNegativeExponentFraction;
        QAction* settingsAngleUnitRadian;
        QAction* settingsAngleUnitDegree;
        QAction* settingsAngleUnitGradian;
        QAction* settingsAngleUnitTurn;
        QAction* settingsAngleUnitRevolution;
        QAction* settingsBehaviorSaveWindowPositionOnExit;
        QAction* settingsBehaviorPartialResults;
        QAction* settingsBehaviorAutoCompletion;
        QAction* settingsBehaviorAutoCompletionAcceptWithEnter;
        QAction* settingsBehaviorAutoCompletionBuiltInFunctions;
        QAction* settingsBehaviorAutoCompletionBuiltInVariables;
        QAction* settingsBehaviorAutoCompletionLongFormUnits;
        QAction* settingsBehaviorAutoCompletionUserFunctions;
        QAction* settingsBehaviorAutoCompletionUserVariables;
        QAction* settingsBehaviorSyntaxHighlighting;
        QAction* settingsEditingInputDigitGrouping;
        QAction* settingsEditingAutomaticInputAdjustments;
        QAction* settingsEditingCursorBlinking;
        QAction* settingsEditingEscapeClearsEditor;
        QAction* settingsBehaviorHoverHighlightResults;
        QAction* settingsBehaviorDigitGroupingNone;
        QAction* settingsBehaviorDigitGroupingOneSpace;
        QAction* settingsBehaviorDigitGroupingTwoSpaces;
        QAction* settingsBehaviorDigitGroupingThreeSpaces;
        QAction* settingsBehaviorDigitGroupingIntegerPartOnly;
        QAction* settingsBehaviorAutoAns;
        QAction* settingsBehaviorLeaveLastExpression;
        QAction* settingsBehaviorNumberFormat;
        QAction* settingsResetAll;
        QAction* settingsBehaviorResultSlots;
        QAction* settingsBehaviorUpDownArrowNever;
        QAction* settingsBehaviorUpDownArrowAlways;
        QAction* settingsBehaviorUpDownArrowSingleLineOnly;
        QAction* settingsBehaviorAlwaysOnTop;
        QAction* settingsOpenPrivateTabOnStartup;
        QAction* settingsLaunchActivate;
        QAction* settingsLaunchNewWindow;
        QAction* settingsLaunchNewTab;
        QAction* settingsBehaviorAutoResultToClipboard;
        QAction* settingsBehaviorSimplifyResultExpressions;
        QAction* settingsBehaviorHistorySizeLimit;
        QAction* settingsRadixCharBoth;
        QAction* settingsDisplayZoomIn;
        QAction* settingsDisplayZoomOut;
        QAction* settingsDisplayFont;
        QAction* settingsDisplayColorSchemeCustom;
        QAction* settingsSystemColorModeFollowSystem;
        QAction* settingsSystemColorModeLight;
        QAction* settingsSystemColorModeDark;
        QAction* settingsMenuAppearanceSystem;
        QAction* settingsMenuAppearanceSpeedCrunch;
        QAction* settingsDockAppearanceSystem;
        QAction* settingsDockAppearanceSpeedCrunch;
        QAction* settingsStatusBarAppearanceSystem;
        QAction* settingsStatusBarAppearanceSpeedCrunch;
        QAction* settingsKeypadAppearanceClassic;
        QAction* settingsKeypadAppearanceModern;
        QAction* settingsEditorDisplayClassic;
        QAction* settingsEditorDisplayModern;
        QVector<QAction*> settingsDisplayColorSchemes;
        QAction* settingsRadixCharDefault;
        QAction* settingsRadixCharDot;
        QAction* settingsRadixCharComma;
        QAction* settingsLanguage;
        QAction* helpManual;
#ifdef SPEEDCRUNCH_ENABLE_UPDATE_CHECKS
        QAction* helpUpdates;
#endif
        QAction* helpFeedback;
        QAction* helpCommunity;
        QAction* helpSource;
        QAction* helpDonate;
        QAction* helpAbout;
        QAction* helpDiagnostics;
        QAction* contextHelp;
    } m_actions{};

    struct {
        QActionGroup* angle;
        QActionGroup* colorScheme;
        QActionGroup* menuAppearance;
        QActionGroup* systemColorMode;
        QActionGroup* dockAppearance;
        QActionGroup* statusBarAppearance;
        QActionGroup* keypadAppearance;
        QActionGroup* editorDisplayStyle;
        QActionGroup* digits;
        QActionGroup* resultRoundingMode;
        QActionGroup* resultFormat;
        QActionGroup* complexFormat;
        QActionGroup* imaginaryUnit;
        QActionGroup* radixChar;
        QActionGroup* digitGrouping;
        QActionGroup* upDownArrowBehavior;
        QActionGroup* subsequentLaunch;
        QActionGroup* keypad;
        QActionGroup* keypadZoom;
        QActionGroup* unitNegativeExponentStyle;
    } m_actionGroups;

    struct {
        QMenu* angleUnit;
        QMenu* complexForm;
        QMenu* imaginaryUnit;
        QMenu* editing;
        QMenu* autoCompletion;
        QMenu* colorScheme;
        QMenu* decimal;
        QMenu* digitGrouping;
        QMenu* display;
        QMenu* menuAppearance;
        QMenu* systemColorMode;
        QMenu* dockAppearance;
        QMenu* statusBarAppearance;
        QMenu* keypadAppearance;
        QMenu* editorDisplayStyle;
        QMenu* edit;
        QMenu* results;
        QMenu* unitNegativeExponentStyle;
        QMenu* resultRoundingMode;
        QMenu* resultFormat;
        QMenu* inputFormat;
        QMenu* help;
        QMenu* precision;
        QMenu* radixChar;
        QMenu* upDownArrowBehavior;
        QMenu* session;
        QMenu* sessionExport;
        QMenu* settings;
        QMenu* view;
        QMenu* split;
        QMenu* keypad;
        QMenu* keypadZoom;
        QMenu* window;
        QMenu* subsequentLaunch;
    } m_menus;

    struct {
        QVBoxLayout* root;
        QHBoxLayout* keypad;
    } m_layouts;

    struct {
        QLabel* state;
        QPushButton* stateCloseButton;
        ResultDisplay* display;
        Editor* editor;
        QSplitter* splitContainer = nullptr;
        Keypad* keypad = nullptr;
        QWidget* keypadContainer = nullptr;
        QWidget* root;
        ManualWindow* manual = nullptr;
        BitFieldWidget* bitField = nullptr;
    } m_widgets{};

    struct {
        BookDock* book;
        GenericDock<ConstantsWidget>* constants;
        GenericDock<FunctionsWidget>* functions;
        GenericDock<HistoryWidget>* history;
        GenericDock<VariableListWidget>* variables;
        GenericDock<UserFunctionListWidget>* userFunctions;
        GenericDock<UserUnitListWidget>* userUnits;
        GenericDock<BitFieldWidget>* bitField;
    } m_docks;
    QList<QDockWidget*> m_allDocks;
    QJsonObject m_dockColumnOrders;
    bool m_initializingUi = true;
    bool m_hasRestoredDockLayout = false;
    bool m_dockAppearanceRefreshPending = false;
    bool m_dockNativePolishPending = false;
    bool m_dockVisibilityRefreshPending = false;
    QHash<ResultDisplay*, QString> m_paneSessionNames;
    QHash<ResultDisplay*, QStringList> m_paneSessionTabs;
    QHash<ResultDisplay*, QTabBar*> m_paneTabBars;
    QHash<QTabBar*, ResultDisplay*> m_tabBarDisplays;
    struct ClosedSessionTab {
        QString name;
        QJsonObject sessionJson;
        QString editorText;
        QPointer<ResultDisplay> display;
        int tabIndex = -1;
    };
    QList<ClosedSessionTab> m_closedSessionTabs;

    struct {
        bool autoAns;
    } m_conditions;

    struct {
        QWidget* angleUnitSection;
        QLabel* angleUnitLabel;
        QPushButton* angleUnit;
        QWidget* resultFormatSection;
        QLabel* resultFormatLabel;
        QPushButton* resultFormat;
        QLabel* resultPrecisionSeparator;
        QWidget* resultPrecisionSection;
        QLabel* resultPrecisionLabel;
        QPushButton* resultPrecision;
        QLabel* angleUnitSeparator;
        // Settings holds the active evaluator state; these values are the
        // independent, persistent selection owned by this window.
        char selectedAngleUnit = 'd';
        char selectedResultFormat = 'g';
        int selectedResultPrecision = -1;
    } m_status;

    Constants* m_constants;
    Evaluator* m_evaluator;
    FunctionRepo* m_functions;
    Settings* m_settings = nullptr;
    Settings::KeypadMode m_keypadMode;
    int m_keypadZoomPercent;
    Session* m_session;
    QHash<QString, Session*> m_loadedSessions;
    QHash<QString, QPair<int, int>> m_sessionViewportAnchors;
    QHash<QString, int> m_sessionScrollValues;
    QTranslator* m_translator;
    QPlainTextEdit* m_copyWidget = nullptr;
    ManualServer* m_manualServer;
#ifdef SPEEDCRUNCH_ENABLE_UPDATE_CHECKS
    VersionCheck* m_versionCheck;
#endif
    int m_pendingHistoryEditIndex;
    bool m_shutdownStateSaved;
    bool m_restorePreviousSessionOnStartup;
    bool m_savedWindowGeometryRestored = false;
    bool m_sessionRestorePending = false;
    bool m_showAfterSessionRestore = false;
    bool m_startupPrivateTabPending = false;
    QTimer* m_deferredSessionSaveTimer;
    bool m_sessionSavePending;
    bool m_bulkEvaluationInProgress;
    bool m_bulkHistoryChanged;
    bool m_bulkVariablesChanged;
    bool m_bulkFunctionsChanged;
    bool m_bulkUnitsChanged;
    bool m_currentResultPreviewDismissed = false;
    QString m_lastCurrentResultPreviewMessage;
};

#endif // GUI_MAINWINDOW_H
