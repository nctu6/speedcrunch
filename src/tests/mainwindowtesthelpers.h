// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef SPEEDCRUNCH_MAINWINDOWTESTHELPERS_H
#define SPEEDCRUNCH_MAINWINDOWTESTHELPERS_H

#include "core/colorscheme.h"
#include "core/evaluator.h"
#include "core/session.h"
#include "core/sessionjsonkeys.h"
#include "core/settings.h"
#include "core/userdefinitions.h"
#include "gui/aboutbox.h"
#include "gui/bitfieldwidget.h"
#include "gui/bookdock.h"
#include "gui/constantswidget.h"
#include "gui/dockliststyle.h"
#include "gui/editor.h"
#include "gui/syntaxhighlighter.h"
#include "gui/functionswidget.h"
#include "gui/gtkmenupalette.h"
#include "gui/historywidget.h"
#include "gui/keypad.h"
#include "gui/mainwindow.h"
#include "gui/menustyleutils.h"
#include "gui/manualwindow.h"
#include "gui/notationandprecisiondialog.h"
#include "gui/oklchutils.h"
#include "gui/resultdisplay.h"
#include "gui/themedlineedit.h"
#include "gui/uiconfig.h"
#include "gui/userfunctionlistwidget.h"
#include "gui/userunitlistwidget.h"
#include "gui/variablelistwidget.h"
#include "math/quantity.h"
#include "core/functions.h"
#include "uitestfixture.h"

#include <QCoreApplication>
#include <QAbstractItemView>
#include <QAbstractTextDocumentLayout>
#include <QAbstractButton>
#include <QActionGroup>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QCursor>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QEnterEvent>
#include <QFocusEvent>
#include <QFontDialog>
#include <QHeaderView>
#include <QGlyphRun>
#include <QGridLayout>
#include <QHelpEvent>
#include <QImage>
#include <QInputDialog>
#include <QLabel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLayout>
#include <QMainWindow>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QMargins>
#include <QLineEdit>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPixmap>
#include <QProxyStyle>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollBar>
#include <QScopeGuard>
#include <QShortcut>
#include <QSignalSpy>
#include <QSplitter>
#include <QSplitterHandle>
#include <QSpinBox>
#include <QStatusBar>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QStyleFactory>
#include <QStyleOption>
#include <QSysInfo>
#include <QTabBar>
#include <QTest>
#include <QTextBrowser>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextEdit>
#include <QTextFragment>
#include <QTextLayout>
#include <QToolButton>
#include <QTranslator>
#include <QTimer>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTreeWidget>
#include <QUrl>

#include <cmath>

namespace MainWindowTestHelpers {

class TransparentMenuBarTestStyle : public QProxyStyle {
public:
    TransparentMenuBarTestStyle()
        : QProxyStyle(QStyleFactory::create(QStringLiteral("Fusion")))
    {}

    void drawControl(ControlElement element, const QStyleOption* option,
                     QPainter* painter, const QWidget* widget = nullptr) const override
    {
        if (element != CE_MenuBarEmptyArea)
            QProxyStyle::drawControl(element, option, painter, widget);
    }

    void drawPrimitive(PrimitiveElement element, const QStyleOption* option,
                       QPainter* painter, const QWidget* widget = nullptr) const override
    {
        if (element != PE_PanelMenuBar)
            QProxyStyle::drawPrimitive(element, option, painter, widget);
    }
};

class MenuBarPaletteTestStyle : public QProxyStyle {
public:
    explicit MenuBarPaletteTestStyle(const QPalette& platformPalette)
        : QProxyStyle(QStyleFactory::create(QStringLiteral("Fusion")))
        , m_platformPalette(platformPalette)
    {}

    void polish(QWidget* widget) override
    {
        QProxyStyle::polish(widget);
        // Breeze gives menu bars their own desktop header palette.
        if (qobject_cast<QMenuBar*>(widget))
            widget->setPalette(m_platformPalette);
    }

private:
    QPalette m_platformPalette;
};

class DockHeaderButtonTestStyle : public QProxyStyle {
public:
    explicit DockHeaderButtonTestStyle(bool framed)
        : QProxyStyle(QStyleFactory::create(QStringLiteral("Fusion")))
        , m_framed(framed)
    {}

    int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr,
                    const QWidget* widget = nullptr) const override
    {
        if (metric == PM_SmallIconSize)
            return 16;
        return QProxyStyle::pixelMetric(metric, option, widget);
    }

    int styleHint(StyleHint hint, const QStyleOption* option = nullptr,
                  const QWidget* widget = nullptr,
                  QStyleHintReturn* returnData = nullptr) const override
    {
        if (hint == SH_DockWidget_ButtonsHaveFrame)
            return m_framed;
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }

private:
    bool m_framed;
};

QJsonObject themeJson(QJsonObject colors);

QString themeJsonString(QJsonObject colors);

QJsonObject sessionJson(const QString& name);

void writeFile(const QString& filePath, const QByteArray& data);

QJsonObject startupSessionWindow(const QString& name, const QString& id);

QWidget* paneWidgetForDisplay(ResultDisplay* display);

void appendPaneScrollValues(const QJsonObject& node, QList<int>* values);

void appendPaneEditorTexts(const QJsonObject& node, QStringList* texts);

bool menuContainsActionText(const QMenu* menu, const QString& text);

QMenu* menuWithTitle(const QMenuBar* menuBar, const QString& title);

QAction* directMenuActionWithText(const QMenu* menu, const QString& text);

QMenu* directSubmenuWithTitle(const QMenu* menu, const QString& title);

QList<MainWindow*> topLevelMainWindows();

bool rejectActiveDialogWithTitle(const QString& title);

class MenuTestResultDisplay : public ResultDisplay {
public:
    explicit MenuTestResultDisplay(QWidget* parent = nullptr)
        : ResultDisplay(parent)
    {
    }

    using ResultDisplay::createContextMenu;
};

class BadgeTestResultDisplay : public ResultDisplay {
public:
    explicit BadgeTestResultDisplay(QWidget* parent = nullptr)
        : ResultDisplay(parent)
    {
    }

    QRect copyBadgeRect(int historyIndex) const
    {
        return copyGlyphBadgeRectForHistoryIndex(historyIndex);
    }

    QRect editBadgeRect(int historyIndex) const
    {
        return editGlyphBadgeRectForHistoryIndex(historyIndex);
    }

    QRect settingsBadgeRect(int historyIndex) const
    {
        return settingsGlyphBadgeRectForHistoryIndex(historyIndex);
    }

    QRect removeBadgeRect(int historyIndex) const
    {
        return removeGlyphBadgeRectForHistoryIndex(historyIndex);
    }
};

bool contextMenuContainsMainMenu(MenuTestResultDisplay* display);

bool editorHasPrimaryOutline(const Editor* editor, const QColor& primary);

bool anyEditorHasPrimaryOutline(const QList<Editor*>& editors, const QColor& primary);

bool colorsAreClose(const QColor& actual, const QColor& expected, int tolerance = 2);

QRect imageRectForWidgetRect(const QImage& image, const QRect& rect);

QPoint firstPixelMatchingColor(const QImage& image, const QRect& rect, const QColor& color, int tolerance);

QPoint firstPixelMatchingColorBlend(const QImage& image, const QRect& rect,
                                  const QColor& foreground, const QColor& background,
                                  int tolerance);

QPoint firstPixelDistinctFromColor(const QImage& image,
                                   const QRect& rect,
                                   const QColor& color,
                                   int minimumChannelDistance);

class FunctionsTestTranslator : public QTranslator {
public:
    QString translate(const char* context,
                      const char* sourceText,
                      const char* disambiguation = nullptr,
                      int n = -1) const override
    {
        Q_UNUSED(disambiguation);
        Q_UNUSED(n);

        if (qstrcmp(context, "FunctionsWidget") == 0
            && qstrcmp(sourceText, "Domain") == 0) {
            return QStringLiteral("Translated Domain");
        }

        return QString();
    }
};

QPushButton* keypadButtonWithText(Keypad* keypad, const QString& text);

QAction* keypadModeAction(MainWindow* window, Settings::KeypadMode mode);

QColor keypadPrimaryStateBackgroundForTest(const QColor& primary,
                                           const QColor& stateBackground,
                                           const QColor& normalBackground);

QColor keypadPrimaryHueFillForTest(const QColor& primary,
                                   const QColor& stateBackground,
                                   const QColor& normalBackground,
                                   int primaryPercent);

QImage dockSeparatorPrimitiveImage(QWidget* widget,
                                   QStyle::State state,
                                   const QRect& separatorRect,
                                   const QSize& imageSize);

QImage dockSeparatorPrimitiveImage(QWidget* widget, QStyle::State state, const QSize& size = QSize(32, 8));

QColor dockSeparatorPrimitiveColor(QWidget* widget, QStyle::State state);

QRect sessionTabPillRect(const QTabBar* tabBar);

QColor selectedSessionTabFillColor(QTabBar* tabBar);

bool selectedSessionTabHasBottomIndicator(QTabBar* tabBar, const QColor& color);

Editor* editorForDisplay(ResultDisplay* display);

QTabBar* tabBarForDisplay(ResultDisplay* display);

QString visibleResultPreviewText(const MainWindow& window);

struct MainWindowStateGuard {
    Settings* settings = Settings::instance();
    QString oldColorScheme = settings->colorScheme;
    QString oldCustomColorSchemeJson = settings->customColorSchemeJson;
    Settings::MenuAppearance oldMenuAppearance = settings->menuAppearance;
    Settings::DockAppearance oldDockAppearance = settings->dockAppearance;
    Settings::StatusBarAppearance oldStatusBarAppearance = settings->statusBarAppearance;
    QString oldSessionLayoutJson = settings->sessionLayoutJson;
    QString oldConstantsDockDomain = settings->constantsDockDomain;
    QString oldConstantsDockSubdomain = settings->constantsDockSubdomain;
    QString oldConstantsDockSearchText = settings->constantsDockSearchText;
    QByteArray oldWindowState = settings->windowState;
    QByteArray oldWindowGeometry = settings->windowGeometry;
    bool oldConstantsDockVisible = settings->constantsDockVisible;
    bool oldFunctionsDockVisible = settings->functionsDockVisible;
    bool oldHistoryDockVisible = settings->historyDockVisible;
    bool oldKeypadVisible = settings->keypadVisible;
    bool oldFormulaBookDockVisible = settings->formulaBookDockVisible;
    bool oldVariablesDockVisible = settings->variablesDockVisible;
    bool oldUserFunctionsDockVisible = settings->userFunctionsDockVisible;
    bool oldUserUnitsDockVisible = settings->userUnitsDockVisible;
    bool oldBitfieldVisible = settings->bitfieldVisible;
    Settings::KeypadMode oldKeypadMode = settings->keypadMode;
    int oldKeypadZoomPercent = settings->keypadZoomPercent;
    bool oldWindowPositionSave = settings->windowPositionSave;
    bool oldStatusBarVisible = settings->statusBarVisible;
    char oldAngleUnit = settings->angleUnit;
    char oldResultFormat = settings->resultFormat;
    int oldResultPrecision = settings->resultPrecision;
    bool oldHasNumberFormatStyleSetting = settings->hasNumberFormatStyleSetting;
    QByteArray oldSkipUpdateCheck = qgetenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK");
    bool hadSkipUpdateCheck = qEnvironmentVariableIsSet("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK");

    MainWindowStateGuard()
    {
        settings->windowPositionSave = false;
        qputenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK", "1");
    }

    ~MainWindowStateGuard()
    {
        settings->colorScheme = oldColorScheme;
        settings->customColorSchemeJson = oldCustomColorSchemeJson;
        settings->menuAppearance = oldMenuAppearance;
        settings->dockAppearance = oldDockAppearance;
        settings->statusBarAppearance = oldStatusBarAppearance;
        settings->sessionLayoutJson = oldSessionLayoutJson;
        settings->constantsDockDomain = oldConstantsDockDomain;
        settings->constantsDockSubdomain = oldConstantsDockSubdomain;
        settings->constantsDockSearchText = oldConstantsDockSearchText;
        settings->windowState = oldWindowState;
        settings->windowGeometry = oldWindowGeometry;
        settings->constantsDockVisible = oldConstantsDockVisible;
        settings->functionsDockVisible = oldFunctionsDockVisible;
        settings->historyDockVisible = oldHistoryDockVisible;
        settings->keypadVisible = oldKeypadVisible;
        settings->formulaBookDockVisible = oldFormulaBookDockVisible;
        settings->variablesDockVisible = oldVariablesDockVisible;
        settings->userFunctionsDockVisible = oldUserFunctionsDockVisible;
        settings->userUnitsDockVisible = oldUserUnitsDockVisible;
        settings->bitfieldVisible = oldBitfieldVisible;
        settings->keypadMode = oldKeypadMode;
        settings->keypadZoomPercent = oldKeypadZoomPercent;
        settings->windowPositionSave = oldWindowPositionSave;
        settings->statusBarVisible = oldStatusBarVisible;
        settings->angleUnit = oldAngleUnit;
        settings->resultFormat = oldResultFormat;
        settings->resultPrecision = oldResultPrecision;
        settings->hasNumberFormatStyleSetting = oldHasNumberFormatStyleSetting;
        if (hadSkipUpdateCheck)
            qputenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK", oldSkipUpdateCheck);
        else
            qunsetenv("SPEEDCRUNCH_TEST_SKIP_UPDATE_CHECK");
    }
};

struct PrivateSessionStorage {
    QTemporaryDir directory;
    QByteArray previousRoot = qgetenv("SPEEDCRUNCH_UI_TEST_STORAGE");

    PrivateSessionStorage()
    {
        qputenv("SPEEDCRUNCH_UI_TEST_STORAGE", directory.path().toUtf8());
        for (const QString& path : {Settings::getConfigPath(), Settings::getDataPath(), Settings::getCachePath()})
            QDir().mkpath(path);
        UiTestFixture::resetSettings();
    }

    ~PrivateSessionStorage() { qputenv("SPEEDCRUNCH_UI_TEST_STORAGE", previousRoot); }

    QString path(const QString& name) const
    {
        return QDir(Settings::getDataPath()).filePath(QStringLiteral("sessions/%1.json").arg(name));
    }
};

void sendTabDragMouseEvent(QTabBar* tabBar, QEvent::Type type, const QPoint& pos,
                           Qt::MouseButton button, Qt::MouseButtons buttons);

QKeyCombination restoreClosedTabShortcut();

QKeyCombination nextSessionTabShortcut();

QKeyCombination previousSessionTabShortcut();

bool focusIsWithin(QWidget* target);

} // namespace MainWindowTestHelpers

#endif
