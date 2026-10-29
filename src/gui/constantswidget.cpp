// SPDX-FileCopyrightText: 2009-2011, 2014, 2016, 2021, 2024, 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later


#include "gui/constantswidget.h"
#include "gui/constantbrowserutils.h"

#include "core/constants.h"
#include "core/settings.h"
#include "core/startupdiagnostics.h"
#include "core/unicodechars.h"
#include "core/mathdsl.h"
#include "gui/dockliststyle.h"
#include "gui/themedlineedit.h"
#include "gui/tooltipstyleutils.h"
#include "gui/uiconfig.h"

#include <QAbstractScrollArea>
#include <QEvent>
#include <QFrame>
#include <QHelpEvent>
#include <QHoverEvent>
#include <QMouseEvent>
#include <QPalette>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QTimer>
#include <QComboBox>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QKeyEvent>
#include <QLineEdit>
#include <QShortcut>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

static QString displayValue(const Constant& constant)
{
    if (constant.domainType != ConstantDomain::Mathematics)
        return constant.value;

    const int dotPos = constant.value.indexOf(MathDsl::DotSep);
    if (dotPos < 0)
        return constant.value;

    return constant.value.left(qMin(constant.value.length(), dotPos + 16));
}

static bool hasLineBreak(const QString& text)
{
    return text.contains(QLatin1Char('\n')) || text.contains(QLatin1Char('\r'))
        || text.contains(QChar::LineSeparator) || text.contains(QChar::ParagraphSeparator);
}

ConstantsWidget::ConstantsWidget(QWidget* parent, const InitialState& state)
    : QWidget(parent)
{
    constexpr int kControlRowHorizontalPadding = 8;

    m_domainLabel = new QLabel(this);
    m_domain = new QComboBox(this);
    m_domain->setEditable(false);
    m_domain->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_subdomainLabel = new QLabel(this);
    m_subdomain = new QComboBox(this);
    m_subdomain->setEditable(false);
    m_subdomain->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    connect(m_domain, SIGNAL(activated(int)), SLOT(handleDomainChanged()));
    connect(m_subdomain, SIGNAL(activated(int)), SLOT(filter()));

    m_domainBox = new QWidget(this);
    m_domainLayout = new QGridLayout;
    m_domainBox->setLayout(m_domainLayout);
    m_domainLayout->setContentsMargins(kControlRowHorizontalPadding,
                                       UiConfig::DockSearchRowVerticalPadding,
                                       kControlRowHorizontalPadding,
                                       0);
    m_domainLayout->setSpacing(6);

    m_label = new QLabel(this);

    m_filter = new ThemedLineEdit(this);
    m_filter->setMinimumWidth(fontMetrics().horizontalAdvance('X') * 10);
    m_filter->setClearButtonEnabled(true);

    connect(m_filter, SIGNAL(textChanged(const QString &)), SLOT(triggerFilter()));

    QWidget* searchBox = new QWidget(this);
    QHBoxLayout* searchLayout = new QHBoxLayout;
    searchBox->setLayout(searchLayout);
    searchLayout->setSpacing(6);
    searchLayout->addWidget(m_label);
    searchLayout->addWidget(m_filter);
    searchLayout->setContentsMargins(kControlRowHorizontalPadding,
                                     UiConfig::DockSearchRowVerticalPadding,
                                     kControlRowHorizontalPadding,
                                     UiConfig::DockSearchRowVerticalPadding);

    m_list = new QTreeWidget(this);
    m_list->setAutoScroll(true);
    m_list->setSizeAdjustPolicy(QAbstractScrollArea::AdjustIgnored);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_list->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_list->setColumnCount(3);
    m_list->setRootIsDecorated(false);
    m_list->setMouseTracking(true);
    m_list->viewport()->setMouseTracking(true);
    m_list->viewport()->setAttribute(Qt::WA_Hover, true);
    m_list->setEditTriggers(QTreeWidget::NoEditTriggers);
    m_list->setSelectionBehavior(QTreeWidget::SelectRows);
    m_list->header()->setStretchLastSection(false);
    DockListStyle::apply(m_list);
    m_list->installEventFilter(this);
    m_list->viewport()->installEventFilter(this);
    connect(m_list->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &ConstantsWidget::hideSummaryPopup);
    connect(m_list->horizontalScrollBar(), &QScrollBar::valueChanged,
            this, &ConstantsWidget::hideSummaryPopup);

    connect(m_list, SIGNAL(itemActivated(QTreeWidgetItem*, int)), SLOT(handleItem(QTreeWidgetItem*)));
    QShortcut* returnShortcut = new QShortcut(QKeySequence(Qt::Key_Return), this);
    returnShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    const auto activateCurrent = [this]() {
        if (m_filterTimer->isActive())
            filter();
        if (QTreeWidgetItem* current = m_list->currentItem())
            handleItem(current);
    };
    connect(returnShortcut, &QShortcut::activated, this, activateCurrent);
    QShortcut* enterShortcut = new QShortcut(QKeySequence(Qt::Key_Enter), this);
    enterShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(enterShortcut, &QShortcut::activated, this, activateCurrent);

    QVBoxLayout* layout = new QVBoxLayout;
    setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_domainBox);
    layout->addWidget(searchBox);
    layout->addWidget(m_list);

    m_filterTimer = new QTimer(this);
    m_filterTimer->setInterval(500);
    m_filterTimer->setSingleShot(true);
    connect(m_filterTimer, SIGNAL(timeout()), SLOT(filter()));

    m_noMatchLabel = new QLabel(this);
    m_noMatchLabel->setAlignment(Qt::AlignCenter);
    m_noMatchLabel->adjustSize();
    m_noMatchLabel->hide();

    translateControls(ListUpdate::DeferPopulation);
    updateDomainLayout();

    QWidget::setTabOrder(m_filter, m_list);
    setFocusProxy(m_filter);

    restoreState(state.domain, state.subdomain, state.searchText);
    m_filter->installEventFilter(this);
}

ConstantsWidget::~ConstantsWidget()
{
    m_filterTimer->stop();
    hideSummaryPopup();
}

QSize ConstantsWidget::minimumSizeHint() const
{
    QSize hint = QWidget::minimumSizeHint();
    hint.setWidth(UiConfig::ConstantsDockMinimumWidth);
    return hint;
}

QString ConstantsWidget::selectedDomain() const { return m_domain->currentText(); }
QString ConstantsWidget::selectedSubdomain() const { return m_subdomain->currentText(); }
QString ConstantsWidget::searchText() const { return m_filter->text(); }
void ConstantsWidget::restoreState(const QString& domain, const QString& subdomain, const QString& searchText)
{
    StartupDiagnostics::AppearanceContext context("Restore constants dock filters");
    const QSignalBlocker searchBlocker(m_filter);
    if (!searchText.isNull())
        m_filter->setText(searchText);
    if (!domain.isEmpty()) {
        const int domainIndex = m_domain->findText(domain);
        if (domainIndex >= 0)
            m_domain->setCurrentIndex(domainIndex);
    }
    refreshSubdomains();
    if (!subdomain.isEmpty()) {
        const int subdomainIndex = m_subdomain->findText(subdomain);
        if (subdomainIndex >= 0)
            m_subdomain->setCurrentIndex(subdomainIndex);
    }
    filter();
}

void ConstantsWidget::setSummaryPopupThemeColors(const QColor& background,
                                                 const QColor& foreground,
                                                 const QColor& outline,
                                                 int cornerRadius)
{
    m_summaryPopupBackgroundColor = background;
    m_summaryPopupForegroundColor = foreground;
    m_summaryPopupOutlineColor = outline;
    m_summaryPopupCornerRadius = qMax(0, cornerRadius);
    applySummaryPopupTheme();
}

void ConstantsWidget::handleRadixCharacterChange()
{
    StartupDiagnostics::AppearanceContext context("Constants dock radix change");
    updateList();
}

void ConstantsWidget::retranslateText()
{
    translateControls(ListUpdate::Populate);
}

void ConstantsWidget::translateControls(ListUpdate mode)
{
    m_domainLabel->setText(tr("Domain"));
    m_subdomainLabel->setText(tr("Subdomain"));
    m_label->setText(tr("Search"));
    m_noMatchLabel->setText(tr("No match found"));

    QStringList titles;
    const QString name = tr("Name");
    const QString value = tr("Value");
    const QString unit = tr("Unit");
    if (layoutDirection() == Qt::LeftToRight)
        titles << name << value << unit;
    else
        titles << name << unit << value;
    m_list->setHeaderLabels(titles);

    updateDomainLabelAlignment();
    refreshList(mode);
}

void ConstantsWidget::filter()
{
    const QList<Constant> &clist = Constants::instance()->list();
    const char radixChar = Settings::instance()->radixCharacter();
    const QString term = m_filter->text();

    m_filterTimer->stop();

    const QString chosenDomain = m_domain->currentText();
    const QString chosenSubdomain = m_subdomain->currentText();
    const QString all = tr("All");
    const bool rightToLeft = layoutDirection() == Qt::RightToLeft;

    // Compare the translated content itself, including fallback translations.
    QStringList inputs {all, chosenDomain, chosenSubdomain, term,
                        QString(QChar::fromLatin1(radixChar)), QString::number(layoutDirection())};
    inputs.reserve(inputs.size() + clist.size() * 6);
    for (const Constant& constant : clist) {
        inputs << constant.name << constant.domain << constant.subdomain
               << constant.value << constant.unit << QString::number(int(constant.domainType));
    }
    const bool unchanged = inputs == m_populatedInputs;
    StartupDiagnostics::AppearanceScope timing(unchanged
        ? "Skip unchanged constants dock list" : "Populate constants dock list", this);
    if (unchanged)
        return;

    setUpdatesEnabled(false);
    hideSummaryPopup();
    {
        StartupDiagnostics::AppearanceScope phase("Clear constants dock rows", this);
        m_list->clear();
    }
    struct PreparedRow {
        const Constant* constant;
        QStringList text;
    };
    QList<PreparedRow> rows;
    bool uniformRowHeights = true;
    {
        StartupDiagnostics::AppearanceScope phase("Filter and format constants dock rows", this);
        rows.reserve(clist.size());
        for (const Constant& constant : clist) {
            if (!ConstantBrowserUtils::matches(constant, term,
                    chosenDomain == all ? QString() : chosenDomain,
                    chosenSubdomain == all ? QString() : chosenSubdomain)) {
                continue;
            }

            const QString value = displayValue(constant);
            const QString radCh = (radixChar != MathDsl::DotSep)
                ? QString(value).replace(MathDsl::DotSep, radixChar) : value;
            const QString normalizedUnit = ConstantBrowserUtils::unit(constant);
            QStringList str {constant.name};
            if (rightToLeft)
                str << normalizedUnit + UnicodeChars::LeftToRightMark << radCh;
            else
                str << radCh << normalizedUnit;

            // Keep variable heights if a translation introduces multiple lines.
            if (uniformRowHeights)
                uniformRowHeights = std::none_of(str.cbegin(), str.cend(), hasLineBreak);
            rows.append({&constant, std::move(str)});
        }
    }
    {
        StartupDiagnostics::AppearanceScope phase("Create constants dock rows and tooltips", this);
        // Single-line rows share a font and have no icons or per-item size hints.
        // Reuse their height during layout without changing content-based widths.
        m_list->setUniformRowHeights(uniformRowHeights);
        for (const auto& row : rows) {
            const Constant& constant = *row.constant;
            QTreeWidgetItem* item = new QTreeWidgetItem(m_list, row.text);
            item->setData(0, Qt::UserRole, ConstantBrowserUtils::expression(constant));

            QString tip;
            tip += QString(UnicodeChars::LeftToRightMark);
            tip += QString("<b>%1</b><br>%2")
                .arg(constant.name, constant.value);
            tip += QString(UnicodeChars::LeftToRightMark);
            if (!constant.unit.isEmpty())
                tip.append(" ").append(QString(constant.unit).replace(
                    UnicodeChars::MiddleDot, MathDsl::MulDotOp));
            if (radixChar != MathDsl::DotSep)
                tip.replace(MathDsl::DotSep, radixChar);
            tip += QString(UnicodeChars::LeftToRightMark);
            item->setToolTip(0, tip);
            item->setToolTip(1, tip);
            item->setToolTip(2, tip);

            if (rightToLeft) {
                item->setTextAlignment(1, Qt::AlignRight);
                item->setTextAlignment(2, Qt::AlignLeft);
            } else {
                item->setTextAlignment(1, Qt::AlignLeft);
                item->setTextAlignment(2, Qt::AlignLeft);
            }
        }
    }

    const bool hasMatches = m_list->topLevelItemCount() > 0;
    m_columnsNeedResize = true;
    if (m_list->isVisible())
        resizeColumnsToContents();

    if (hasMatches) {
        m_noMatchLabel->hide();
        StartupDiagnostics::AppearanceScope phase("Sort constants dock rows", this);
        m_list->sortItems(0, Qt::AscendingOrder);
        m_list->setCurrentItem(m_list->topLevelItem(0));
    } else {
        updateEmptyHeaderStretch();
        scheduleEmptyHeaderStretch();
        DockListStyle::showCenteredNoMatchLabel(m_list, m_noMatchLabel);
    }

    m_populatedInputs = std::move(inputs);
    setUpdatesEnabled(true);
}

void ConstantsWidget::handleItem(QTreeWidgetItem* item)
{
    emit constantSelected(item->data(0, Qt::UserRole).toString());
}

void ConstantsWidget::triggerFilter()
{
    m_filterTimer->stop();
    m_filterTimer->start();
}

void ConstantsWidget::updateList()
{
    refreshList(ListUpdate::Populate);
}

void ConstantsWidget::refreshList(ListUpdate mode)
{
    const int chosenDomainIndex = m_domain->currentIndex();
    const QString chosenDomain = m_domain->currentText();
    const int chosenSubdomainIndex = m_subdomain->currentIndex();
    const QString chosenSubdomain = m_subdomain->currentText();

    m_domain->clear();
    Constants::instance()->retranslateText();
    m_domain->addItems(Constants::instance()->domains());
    m_domain->insertItem(0, tr("All"));

    int domainIndex = m_domain->findText(chosenDomain);
    if (domainIndex < 0
        && chosenDomainIndex >= 0
        && chosenDomainIndex < m_domain->count()) {
        domainIndex = chosenDomainIndex;
    }
    if (domainIndex < 0) {
        const QString defaultDomain = Constants::instance()->domains().value(0);
        domainIndex = m_domain->findText(defaultDomain);
    }
    if (domainIndex < 0)
        domainIndex = 0;
    m_domain->setCurrentIndex(domainIndex);

    refreshSubdomains();

    int subdomainIndex = m_subdomain->findText(chosenSubdomain);
    if (subdomainIndex < 0
        && chosenSubdomainIndex >= 0
        && chosenSubdomainIndex < m_subdomain->count()) {
        subdomainIndex = chosenSubdomainIndex;
    }
    if (subdomainIndex >= 0)
        m_subdomain->setCurrentIndex(subdomainIndex);

    if (mode == ListUpdate::Populate)
        filter();
}

void ConstantsWidget::handleDomainChanged()
{
    StartupDiagnostics::AppearanceContext context("Constants dock domain change");
    refreshSubdomains();
    filter();
}

void ConstantsWidget::refreshSubdomains()
{
    m_subdomain->clear();
    const QString chosenDomain = m_domain->currentText();
    if (chosenDomain == tr("All")) {
        m_subdomain->insertItem(0, tr("All"));
        m_subdomain->setCurrentIndex(0);
        m_subdomainLabel->setVisible(true);
        m_subdomain->setVisible(true);
        updateDomainLayout();
        return;
    }

    const QStringList subdomains = Constants::instance()->subdomains(chosenDomain);
    m_subdomain->addItems(subdomains);
    m_subdomain->insertItem(0, tr("All"));
    m_subdomain->setCurrentIndex(0);

    const bool hasConfiguredSubdomains = !subdomains.isEmpty();
    m_subdomainLabel->setVisible(hasConfiguredSubdomains);
    m_subdomain->setVisible(hasConfiguredSubdomains);
    updateDomainLayout();
}

void ConstantsWidget::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        StartupDiagnostics::AppearanceContext context("Constants dock LanguageChange");
        retranslateText();
    }
    else {
        QWidget::changeEvent(e);
        if (e->type() == QEvent::LayoutDirectionChange)
            retranslateText();
        if (e->type() == QEvent::FontChange)
            QTimer::singleShot(0, this, [this]() { updateDomainLayout(); });
    }
}

bool ConstantsWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_filter && event->type() == QEvent::KeyPress) {
        const int key = static_cast<QKeyEvent*>(event)->key();
        if (key == Qt::Key_Up || key == Qt::Key_Down) {
            if (m_filterTimer->isActive())
                filter();
            ConstantBrowserUtils::moveSelection(m_list, key);
            return true;
        }
    }
    if (watched == m_list && event->type() == QEvent::Show && m_columnsNeedResize)
        resizeColumnsToContents();
    if (watched == m_list || watched == m_list->viewport()) {
        switch (event->type()) {
        case QEvent::ToolTip: {
            QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);
            const QPoint viewportPos = watched == m_list->viewport()
                ? helpEvent->pos()
                : m_list->viewport()->mapFrom(m_list, helpEvent->pos());
            updateSummaryPopupForViewportPosition(viewportPos, helpEvent->globalPos());
            return true;
        }
        case QEvent::MouseMove: {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            const QPoint viewportPos = watched == m_list->viewport()
                ? mouseEvent->pos()
                : m_list->viewport()->mapFrom(m_list, mouseEvent->pos());
            updateSummaryPopupForViewportPosition(viewportPos,
                                                  mouseEvent->globalPosition().toPoint());
            break;
        }
        case QEvent::HoverMove: {
            QHoverEvent* hoverEvent = static_cast<QHoverEvent*>(event);
            const QPoint viewportPos = watched == m_list->viewport()
                ? hoverEvent->position().toPoint()
                : m_list->viewport()->mapFrom(m_list, hoverEvent->position().toPoint());
            updateSummaryPopupForViewportPosition(viewportPos,
                                                  m_list->viewport()->mapToGlobal(viewportPos));
            break;
        }
        case QEvent::Hide:
        case QEvent::KeyPress:
        case QEvent::Leave:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseButtonPress:
        case QEvent::Resize:
        case QEvent::Wheel:
            hideSummaryPopup();
            break;
        default:
            break;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void ConstantsWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateDomainLayout();
    updateEmptyHeaderStretch();
    scheduleEmptyHeaderStretch();
}

void ConstantsWidget::ensureSummaryPopup()
{
    if (m_summaryPopup != nullptr)
        return;

    m_summaryPopup = ToolTipStyleUtils::createPopup(
        this,
        QStringLiteral("constantsSummaryPopup"),
        QStringLiteral("constantsSummaryPopupLabel"),
        Qt::RichText,
        &m_summaryPopupLabel);
    applySummaryPopupTheme();
}

void ConstantsWidget::applySummaryPopupTheme()
{
    ToolTipStyleUtils::applyPopupTheme(
        m_summaryPopup,
        m_summaryPopupLabel,
        this,
        {m_summaryPopupBackgroundColor,
         m_summaryPopupForegroundColor,
         m_summaryPopupOutlineColor,
         m_summaryPopupCornerRadius});
}

void ConstantsWidget::hideSummaryPopup()
{
    if (m_summaryPopup != nullptr)
        m_summaryPopup->hide();
}

void ConstantsWidget::showSummaryPopup(QTreeWidgetItem* item,
                                       int column,
                                       const QPoint& globalPos)
{
    if (item == nullptr)
        return;

    const QString tip = item->toolTip(column).isEmpty()
        ? item->toolTip(0)
        : item->toolTip(column);
    if (tip.isEmpty()) {
        hideSummaryPopup();
        return;
    }

    ensureSummaryPopup();
    ToolTipStyleUtils::showPopup(m_summaryPopup,
                                 m_summaryPopupLabel,
                                 tip,
                                 m_list,
                                 globalPos,
                                 m_summaryPopupCornerRadius);
}

void ConstantsWidget::updateSummaryPopupForViewportPosition(const QPoint& viewportPos,
                                                            const QPoint& globalPos)
{
    // QTreeView can return the first row for positions above the viewport.
    if (!m_list->viewport()->rect().contains(viewportPos)) {
        hideSummaryPopup();
        return;
    }

    const QModelIndex index = m_list->indexAt(viewportPos);
    QTreeWidgetItem* item = index.isValid() ? m_list->itemAt(viewportPos) : nullptr;
    if (item == nullptr) {
        hideSummaryPopup();
        return;
    }

    showSummaryPopup(item, index.column(), globalPos);
}

void ConstantsWidget::updateSummaryPopupMask()
{
    if (m_summaryPopup == nullptr)
        return;

    ToolTipStyleUtils::applyRoundedPopupMask(m_summaryPopup,
                                             qMax(0, m_summaryPopupCornerRadius));
}

void ConstantsWidget::scheduleEmptyHeaderStretch()
{
    if (m_emptyHeaderStretchQueued
        || m_list == nullptr
        || m_list->topLevelItemCount() > 0) {
        return;
    }

    m_emptyHeaderStretchQueued = true;
    QTimer::singleShot(0, this, [this]() {
        m_emptyHeaderStretchQueued = false;
        updateEmptyHeaderStretch();
    });
}

void ConstantsWidget::updateEmptyHeaderStretch()
{
    if (m_list == nullptr || m_list->topLevelItemCount() > 0)
        return;

    QHeaderView* header = m_list->header();
    if (header == nullptr || header->count() < 3)
        return;

    const int fixedWidth = header->sectionSize(0) + header->sectionSize(1);
    const int targetWidth = qMax(header->width(), m_list->viewport()->width());
    if (targetWidth <= fixedWidth)
        return;

    header->resizeSection(2, qMax(header->sectionSizeHint(2), targetWidth - fixedWidth));
}

void ConstantsWidget::resizeColumnsToContents()
{
    StartupDiagnostics::AppearanceScope phase("Size constants dock columns", this);
    m_columnsNeedResize = false;
    m_list->header()->setStretchLastSection(false);
    m_list->header()->setSectionResizeMode(2, QHeaderView::Interactive);
    for (int column = 0; column < m_list->columnCount(); ++column)
        m_list->resizeColumnToContents(column);
    updateEmptyHeaderStretch();
}

void ConstantsWidget::updateDomainLayout()
{
    const int minInlineWidthByControls =
        m_domainLabel->sizeHint().width() + m_domain->minimumSizeHint().width()
        + m_subdomainLabel->sizeHint().width() + m_subdomain->minimumSizeHint().width()
        + (m_domainLayout->spacing() * 3);
    // Side docks are often still wide enough to satisfy minimum hints while visually cramped.
    // Use both control hints and a practical width cap so right/left panes stack reliably.
    const int practicalInlineWidth = 560;
    const int availableWidth = m_domainBox->width() > 0 ? m_domainBox->width() : width();
    const bool compact = availableWidth < std::max(minInlineWidthByControls, practicalInlineWidth);
    m_domainLayout->setColumnStretch(3, compact || m_subdomain->isHidden() ? 0 : 1);
    if (m_domainLayoutInitialized && compact == m_isCompactDomainLayout) {
        updateDomainLabelAlignment();
        return;
    }

    m_domainLayoutInitialized = true;
    m_isCompactDomainLayout = compact;
    while (QLayoutItem* item = m_domainLayout->takeAt(0))
        delete item;

    m_domainLayout->setColumnStretch(1, 1);
    m_domainLayout->addWidget(m_domainLabel, 0, 0);
    m_domainLayout->addWidget(m_domain, 0, 1);
    m_domainLayout->addWidget(m_subdomainLabel, compact ? 1 : 0, compact ? 0 : 2);
    m_domainLayout->addWidget(m_subdomain, compact ? 1 : 0, compact ? 1 : 3);

    updateDomainLabelAlignment();
}

void ConstantsWidget::updateDomainLabelAlignment()
{
    if (m_isCompactDomainLayout) {
        const int labelWidth = std::max({m_domainLabel->sizeHint().width(),
                                        m_subdomainLabel->sizeHint().width(),
                                        m_label->sizeHint().width()});
        m_domainLabel->setFixedWidth(labelWidth);
        m_subdomainLabel->setFixedWidth(labelWidth);
        m_label->setFixedWidth(labelWidth);
    } else {
        const int labelWidth = std::max(m_domainLabel->sizeHint().width(), m_label->sizeHint().width());
        m_domainLabel->setFixedWidth(labelWidth);
        m_label->setFixedWidth(labelWidth);
        m_subdomainLabel->setMinimumWidth(0);
        m_subdomainLabel->setMaximumWidth(QWIDGETSIZE_MAX);
    }
}
