// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later


#include "core/evaluator.h"
#include "core/session.h"
#include "core/settings.h"
#include "gui/constantswidget.h"
#include "gui/genericdock.h"
#include "gui/splittertreeutils.h"
#include "gui/userunitlistwidget.h"
#include "gui/variablelistwidget.h"
#include "uitestfixture.h"

#include <QSignalSpy>
#include <QTest>
#include <QCoreApplication>
#include <QComboBox>
#include <QFrame>
#include <QHeaderView>
#include <QHelpEvent>
#include <QHoverEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QMainWindow>
#include <QTranslator>
#include <QSplitter>
#include <QTreeWidget>

class ConstantsTestTranslator : public QTranslator {
public:
    QString translate(const char* context,
                      const char* sourceText,
                      const char* disambiguation = nullptr,
                      int n = -1) const override
    {
        Q_UNUSED(disambiguation);
        Q_UNUSED(n);

        if (qstrcmp(context, "Constants") == 0
            && qstrcmp(sourceText, "pi (π)") == 0) {
            return QStringLiteral("translated pi");
        }

        return QString();
    }
};

class TestableConstantsWidget : public ConstantsWidget {
public:
    using ConstantsWidget::handleItem;
    using ConstantsWidget::updateList;
};

class TestableWaylandDock : public GenericDock<QWidget> {
public:
    explicit TestableWaylandDock(QWidget* parent)
        : GenericDock<QWidget>("TestDocksUi", "Test dock", parent)
    {
    }

    QRect titleRect() const
    {
        QStyleOptionDockWidget option;
        initStyleOption(&option);
        return option.rect;
    }

    bool waylandMoves = true;
    int moveRequests = 0;

protected:
    bool usesWaylandWindowMoves() const override { return waylandMoves; }
    void startWindowMove() override { ++moveRequests; }
};

class TestDocksUi : public QObject {
    Q_OBJECT

private slots:
    void init() { UiTestFixture::resetSettings(); }
    void floating_wayland_dock_uses_system_move_data();
    void floating_wayland_dock_uses_system_move();
    void floating_wayland_dock_keeps_title_controls();
    void constants_dock_inserts_pi_symbol_with_translated_name();
    void constants_search_navigates_and_flushes_pending_filter();
    void constants_columns_fit_after_hidden_filter_changes();
    void constants_summary_popup_ignores_header_and_padding_data();
    void constants_summary_popup_ignores_header_and_padding();
    void user_units_dock_shows_rhs_and_description_after_definition();
    void user_variables_dock_keeps_existing_value_text_after_new_definition();
    void splitter_normalization_removes_single_child_nested_splitter_after_pane_close_shape();
};

void TestDocksUi::constants_search_navigates_and_flushes_pending_filter()
{
    TestableConstantsWidget widget;
    auto* search = widget.findChild<QLineEdit*>();
    auto* list = widget.findChild<QTreeWidget*>();
    auto* domain = widget.findChild<QComboBox*>();
    QVERIFY(search && list && domain);
    widget.restoreState(domain->itemText(0), QString(), QString());
    widget.resize(600, 400);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    search->setFocus();
    QSignalSpy inserted(&widget, &ConstantsWidget::constantSelected);
    QTest::keyClicks(search, "PROTON");
    QTest::keyClick(search, Qt::Key_Down);
    QVERIFY(search->hasFocus());
    QVERIFY(list->topLevelItemCount() > 1);
    QCOMPARE(list->indexOfTopLevelItem(list->currentItem()), 1);
    for (int i = 0; i < list->topLevelItemCount(); ++i)
        QVERIFY(list->topLevelItem(i)->text(0).contains(QStringLiteral("proton"), Qt::CaseInsensitive));
    QTest::keyClick(search, Qt::Key_Up);
    QCOMPARE(list->indexOfTopLevelItem(list->currentItem()), 0);
    search->setText(QStringLiteral("Euler's number"));
    QTest::keyClick(search, Qt::Key_Return);
    QCOMPARE(inserted.count(), 1);
    QCOMPARE(inserted.takeFirst().at(0).toString(), QStringLiteral("e"));
    search->setText(QStringLiteral("kg"));
    QTest::keyClick(search, Qt::Key_Down);
    QVERIFY(list->topLevelItemCount() > 0);
    for (int i = 0; i < list->topLevelItemCount(); ++i) {
        auto* item = list->topLevelItem(i);
        QVERIFY(item->text(0).contains(QStringLiteral("kg"), Qt::CaseInsensitive)
                || item->text(2).contains(QStringLiteral("kg"), Qt::CaseInsensitive));
    }
    search->setText(QStringLiteral("no such constant 49236"));
    QTest::keyClick(search, Qt::Key_Return);
    QCOMPARE(list->topLevelItemCount(), 0);
    QCOMPARE(inserted.count(), 0);
    QTest::keyClick(search, Qt::Key_Up);
    QVERIFY(list->currentItem() == nullptr);
}

void TestDocksUi::constants_columns_fit_after_hidden_filter_changes()
{
    TestableConstantsWidget widget;
    QTreeWidget* list = widget.findChild<QTreeWidget*>();
    QVERIFY(list);
    QVERIFY(list->topLevelItemCount() > 0);
    QComboBox* domain = widget.findChild<QComboBox*>();
    QVERIFY(domain);
    const QString allDomains = domain->itemText(0);
    widget.restoreState(allDomains, QString(), QStringLiteral("pi"));
    QVERIFY(list->topLevelItemCount() > 0);
    widget.resize(500, 400);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    const auto verifyColumnWidths = [list]() {
        for (int column = 0; column < list->columnCount(); ++column) {
            const int width = list->columnWidth(column);
            list->resizeColumnToContents(column);
            QCOMPARE(list->columnWidth(column), width);
        }
    };
    verifyColumnWidths();
    widget.hide();
    widget.restoreState(QString(), QString(), QStringLiteral("no-such-constant"));
    QCOMPARE(list->topLevelItemCount(), 0);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QTRY_VERIFY(list->header()->length() >= list->viewport()->width());
    widget.restoreState(QString(), QString(), QStringLiteral(""));
    QVERIFY(list->topLevelItemCount() > 0);
    verifyColumnWidths();
    const QByteArray headerState = list->header()->saveState();
    widget.hide();
    widget.show();
    QCOMPARE(list->header()->saveState(), headerState);
}

void TestDocksUi::floating_wayland_dock_uses_system_move_data()
{
    QTest::addColumn<bool>("vertical");
    QTest::newRow("horizontal-title") << false;
    QTest::newRow("vertical-title") << true;
}

void TestDocksUi::floating_wayland_dock_uses_system_move()
{
    QFETCH(bool, vertical);
    QMainWindow window;
    auto* dock = new TestableWaylandDock(&window);
    if (vertical)
        dock->setFeatures(dock->features() | QDockWidget::DockWidgetVerticalTitleBar);
    window.setCentralWidget(new QWidget(&window));
    window.addDockWidget(Qt::RightDockWidgetArea, dock);
    window.resize(640, 480);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QCoreApplication::processEvents();
    const QPoint titlePos = dock->titleRect().center();
    QVERIFY(!dock->titleRect().isEmpty());

    // Attached docks keep Qt's docking behavior.
    QTest::mouseClick(dock, Qt::LeftButton, Qt::NoModifier, titlePos);
    QCOMPARE(dock->moveRequests, 0);
    QVERIFY(!dock->isFloating());

    dock->setFloating(true);
    QVERIFY(QTest::qWaitForWindowExposed(dock));
    QCoreApplication::processEvents();
    // Offscreen uses native decorations. Qt retains the attached title rect,
    // allowing us to exercise the Wayland event route without a compositor.
    QVERIFY(dock->titleRect().contains(titlePos));
    const QRect originalGeometry = dock->geometry();
    QTest::mouseClick(dock, Qt::RightButton, Qt::NoModifier, titlePos);
    QCOMPARE(dock->moveRequests, 0);
    const QPoint contentPos(dock->width() - 20, dock->height() - 20);
    QVERIFY(!dock->titleRect().contains(contentPos));
    QTest::mouseClick(dock, Qt::LeftButton, Qt::NoModifier, contentPos);
    QCOMPARE(dock->moveRequests, 0);

    for (int attempt = 1; attempt <= 2; ++attempt) {
        QTest::mousePress(dock, Qt::LeftButton, Qt::NoModifier, titlePos);
        QCOMPARE(dock->moveRequests, attempt);
        const QPoint movedPos = titlePos + QPoint(QApplication::startDragDistance() + 20, 30);
        QMouseEvent move(QEvent::MouseMove, QPointF(movedPos),
                         QPointF(dock->mapToGlobal(movedPos)),
                         Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(dock, &move);
        QTest::mouseRelease(dock, Qt::LeftButton, Qt::NoModifier, movedPos);
        QCOMPARE(dock->geometry(), originalGeometry);
        QVERIFY(QWidget::mouseGrabber() != dock);
        QVERIFY(dock->isFloating());
    }

    // A custom title bar owns its input handling.
    dock->setTitleBarWidget(new QWidget(dock));
    QCoreApplication::processEvents();
    QTest::mouseClick(dock, Qt::LeftButton, Qt::NoModifier, titlePos);
    QCOMPARE(dock->moveRequests, 2);
    dock->setTitleBarWidget(nullptr);
    dock->setFloating(false);
    QCoreApplication::processEvents();
    dock->waylandMoves = false;
    dock->setFloating(true);
    QTest::mouseClick(dock, Qt::LeftButton, Qt::NoModifier, titlePos);
    QCOMPARE(dock->moveRequests, 2);
}

void TestDocksUi::floating_wayland_dock_keeps_title_controls()
{
    QMainWindow window;
    auto* dock = new TestableWaylandDock(&window);
    window.addDockWidget(Qt::RightDockWidgetArea, dock);
    window.resize(640, 480);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    const QPoint titlePos = dock->titleRect().center();
    dock->setFloating(true);
    QCoreApplication::processEvents();
    QTest::mouseDClick(dock, Qt::LeftButton, Qt::NoModifier, titlePos);
    QVERIFY(!dock->isFloating());
    QCOMPARE(dock->moveRequests, 0);

    dock->setFloating(true);
    dock->setFeatures(dock->features() & ~QDockWidget::DockWidgetFloatable);
    QTest::mouseDClick(dock, Qt::LeftButton, Qt::NoModifier, titlePos);
    QVERIFY(dock->isFloating());
    dock->setFeatures(dock->features() | QDockWidget::DockWidgetFloatable);
    dock->setFloating(false);

    auto* floatButton = dock->findChild<QAbstractButton*>(QStringLiteral("qt_dockwidget_floatbutton"));
    auto* closeButton = dock->findChild<QAbstractButton*>(QStringLiteral("qt_dockwidget_closebutton"));
    QVERIFY(floatButton != nullptr);
    QVERIFY(closeButton != nullptr);
    QTest::mouseClick(floatButton, Qt::LeftButton);
    QVERIFY(dock->isFloating());
    // Expose Qt's client title buttons on the offscreen backend as on Wayland.
    floatButton->show();
    closeButton->show();
    const QPoint buttonPos = floatButton->geometry().center();
    QTest::mouseClick(dock, Qt::LeftButton, Qt::NoModifier, buttonPos);
    QCOMPARE(dock->moveRequests, 0);
    QTest::mouseClick(floatButton, Qt::LeftButton);
    QVERIFY(!dock->isFloating());
    QTest::mouseClick(floatButton, Qt::LeftButton);
    QVERIFY(dock->isFloating());
    closeButton->show();
    QTest::mouseClick(closeButton, Qt::LeftButton);
    QVERIFY(dock->isHidden());
    QCOMPARE(dock->moveRequests, 0);
}

void TestDocksUi::constants_dock_inserts_pi_symbol_with_translated_name()
{
    ConstantsTestTranslator translator;
    QCoreApplication::installTranslator(&translator);

    TestableConstantsWidget widget;
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    widget.updateList();

    QTreeWidget* tree = widget.findChild<QTreeWidget*>();
    QVERIFY(tree != nullptr);

    QTreeWidgetItem* piItem = nullptr;
    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = tree->topLevelItem(i);
        if (item && item->text(0) == QStringLiteral("translated pi")) {
            piItem = item;
            break;
        }
    }

    QVERIFY(piItem != nullptr);

    QSignalSpy spy(&widget, &ConstantsWidget::constantSelected);
    widget.handleItem(piItem);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), QString::fromUtf8("π"));

    QCoreApplication::removeTranslator(&translator);
}

void TestDocksUi::constants_summary_popup_ignores_header_and_padding_data()
{
    QTest::addColumn<int>("eventType");
    QTest::addColumn<bool>("sendToViewport");

    for (bool sendToViewport : {false, true}) {
        const QByteArray target = sendToViewport ? "viewport-" : "tree-";
        QTest::newRow(QByteArray(target + "mouse").constData())
            << int(QEvent::MouseMove) << sendToViewport;
        QTest::newRow(QByteArray(target + "hover").constData())
            << int(QEvent::HoverMove) << sendToViewport;
        QTest::newRow(QByteArray(target + "tooltip").constData())
            << int(QEvent::ToolTip) << sendToViewport;
    }
}

void TestDocksUi::constants_summary_popup_ignores_header_and_padding()
{
    QFETCH(int, eventType);
    QFETCH(bool, sendToViewport);

    ConstantsWidget widget;
    widget.resize(420, 360);
    QTreeWidget* tree = widget.findChild<QTreeWidget*>();
    QVERIFY(tree != nullptr);
    tree->setStyleSheet(QStringLiteral("QTreeWidget { padding: 6px 8px; border: 0; }"));
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QVERIFY(tree->topLevelItemCount() > 0);
    QTreeWidgetItem* firstItem = tree->topLevelItem(0);
    tree->scrollToItem(firstItem);
    QCoreApplication::processEvents();

    QWidget* viewport = tree->viewport();
    QWidget* target = sendToViewport ? viewport : tree;
    const QPoint rowPos = tree->visualItemRect(firstItem).center();
    QVERIFY(viewport->rect().contains(rowPos));
    const auto sendPosition = [&](const QPoint& viewportPos) {
        const QPoint pos = viewport->mapTo(target, viewportPos);
        const QPoint globalPos = viewport->mapToGlobal(viewportPos);
        if (eventType == QEvent::MouseMove) {
            QMouseEvent event(QEvent::MouseMove, QPointF(pos), QPointF(globalPos),
                              Qt::NoButton, Qt::NoButton, Qt::NoModifier);
            QCoreApplication::sendEvent(target, &event);
        } else if (eventType == QEvent::HoverMove) {
            QHoverEvent event(QEvent::HoverMove, QPointF(pos), QPointF(globalPos),
                              QPointF(pos));
            QCoreApplication::sendEvent(target, &event);
        } else {
            QHelpEvent event(QEvent::ToolTip, pos, globalPos);
            QCoreApplication::sendEvent(target, &event);
        }
    };

    // Header and padding positions must not resolve to the first constant.
    const QList<QPoint> outsidePositions = {
        viewport->mapFrom(tree, QPoint(rowPos.x(), 1)),
        viewport->mapFromGlobal(tree->header()->mapToGlobal(tree->header()->rect().center())),
        QPoint(rowPos.x(), -1),
        QPoint(-1, rowPos.y()),
        QPoint(viewport->width(), rowPos.y()),
        QPoint(rowPos.x(), viewport->height())
    };
    for (const QPoint& outsidePos : outsidePositions) {
        QVERIFY(!viewport->rect().contains(outsidePos));
        sendPosition(rowPos);
        QFrame* popup = widget.findChild<QFrame*>(QStringLiteral("constantsSummaryPopup"));
        QVERIFY(popup != nullptr);
        QVERIFY(popup->isVisible());
        QLabel* label = popup->findChild<QLabel*>(QStringLiteral("constantsSummaryPopupLabel"));
        QVERIFY(label != nullptr);
        QCOMPARE(label->text(), firstItem->toolTip(tree->indexAt(rowPos).column()));

        sendPosition(outsidePos);
        QVERIFY2(!popup->isVisible(), qPrintable(QStringLiteral("Popup remains visible at (%1, %2)")
                                                    .arg(outsidePos.x()).arg(outsidePos.y())));
        sendPosition(outsidePos);
        QVERIFY(!popup->isVisible());
    }
}

void TestDocksUi::user_units_dock_shows_rhs_and_description_after_definition()
{
    Session session;
    Evaluator* evaluator = session.evaluator();
    evaluator->unsetAllUserUnits();

    evaluator->setExpression(QStringLiteral("[cm_s] = 2 [cm/s] ? speed alias"));
    const Quantity result = evaluator->eval();
    QVERIFY(!result.isNan());

    UserUnitListWidget widget;
    widget.setEvaluator(evaluator);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    widget.updateList();

    QTreeWidget* tree = widget.findChild<QTreeWidget*>();
    QVERIFY(tree != nullptr);
    QCOMPARE(tree->topLevelItemCount(), 1);

    QTreeWidgetItem* item = tree->topLevelItem(0);
    QVERIFY(item != nullptr);
    QCOMPARE(item->text(0), QStringLiteral("cm_s"));
    QCOMPARE(item->text(1), QStringLiteral("2[cm/s]"));
    QCOMPARE(item->text(2), QStringLiteral("speed alias"));
}

void TestDocksUi::user_variables_dock_keeps_existing_value_text_after_new_definition()
{
    Session session;
    Evaluator* evaluator = session.evaluator();
    Settings* settings = Settings::instance();

    evaluator->unsetAllUserDefinedVariables();

    const char oldResultFormat = settings->resultFormat;
    const int oldResultPrecision = settings->resultPrecision;

    settings->resultFormat = 'f';
    settings->resultPrecision = 0;
    evaluator->setExpression(QStringLiteral("v_fixed = 100000"));
    QVERIFY(!evaluator->eval().isNan());

    VariableListWidget widget;
    widget.setEvaluator(evaluator);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    widget.updateList();

    QTreeWidget* tree = widget.findChild<QTreeWidget*>();
    QVERIFY(tree != nullptr);

    QTreeWidgetItem* fixedItem = nullptr;
    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = tree->topLevelItem(i);
        if (item && item->text(0) == QStringLiteral("v_fixed")) {
            fixedItem = item;
            break;
        }
    }
    QVERIFY(fixedItem != nullptr);
    const QString fixedValueBefore = fixedItem->text(1);
    QVERIFY(!fixedValueBefore.isEmpty());

    settings->resultFormat = 'e';
    settings->resultPrecision = 2;
    evaluator->setExpression(QStringLiteral("v_sci = 100000"));
    QVERIFY(!evaluator->eval().isNan());

    widget.updateList();

    fixedItem = nullptr;
    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = tree->topLevelItem(i);
        if (item && item->text(0) == QStringLiteral("v_fixed")) {
            fixedItem = item;
            break;
        }
    }
    QVERIFY(fixedItem != nullptr);
    QCOMPARE(fixedItem->text(1), fixedValueBefore);

    settings->resultFormat = oldResultFormat;
    settings->resultPrecision = oldResultPrecision;
}

void TestDocksUi::splitter_normalization_removes_single_child_nested_splitter_after_pane_close_shape()
{
    QSplitter root(Qt::Horizontal);
    QWidget* leftPane = new QWidget();
    QSplitter* nested = new QSplitter(Qt::Vertical);
    QWidget* topPane = new QWidget();
    QWidget* bottomPane = new QWidget();

    nested->addWidget(topPane);
    nested->addWidget(bottomPane);
    root.addWidget(leftPane);
    root.addWidget(nested);

    // Simulate closing one pane from a nested splitter.
    topPane->setParent(nullptr);
    topPane->deleteLater();
    QCoreApplication::processEvents();

    normalizeSplitterTree(&root);
    QCoreApplication::processEvents();

    QCOMPARE(root.count(), 2);
    QVERIFY(root.widget(0) != nullptr);
    QVERIFY(root.widget(1) != nullptr);
    QVERIFY(qobject_cast<QSplitter*>(root.widget(0)) == nullptr);
    QVERIFY(qobject_cast<QSplitter*>(root.widget(1)) == nullptr);
}

int main(int argc, char** argv)
{
    return UiTestFixture::run<TestDocksUi>(argc, argv);
}

#include "testdocksui.moc"
