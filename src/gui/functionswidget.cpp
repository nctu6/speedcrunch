// SPDX-FileCopyrightText: 2009-2011, 2013-2014, 2016, 2024, 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later


#include "gui/functionswidget.h"

#include "core/functions.h"
#include "core/settings.h"
#include "core/startupdiagnostics.h"
#include "gui/dockliststyle.h"
#include "gui/themedlineedit.h"
#include "gui/uiconfig.h"

#include <algorithm>
#include <utility>

#include <QEvent>
#include <QString>
#include <QTimer>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QShortcut>
#include <QSignalBlocker>
#include <QTreeWidget>
#include <QVBoxLayout>

FunctionsWidget::FunctionsWidget(QWidget* parent, const InitialState& state)
    : QWidget(parent)
    , m_filterTimer(new QTimer(this))
    , m_domain(new QComboBox(this))
    , m_domainLabel(new QLabel(this))
    , m_functions(new QTreeWidget(this))
    , m_noMatchLabel(new QLabel(m_functions))
    , m_searchFilter(new ThemedLineEdit(this))
    , m_searchLabel(new QLabel(this))
{
    constexpr int kControlRowHorizontalPadding = 8;

    m_filterTimer->setInterval(500);
    m_filterTimer->setSingleShot(true);

    m_functions->setAutoScroll(true);
    m_functions->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_functions->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_functions->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_functions->setColumnCount(2);
    m_functions->setRootIsDecorated(false);
    m_functions->setEditTriggers(QTreeWidget::NoEditTriggers);
    m_functions->setSelectionBehavior(QTreeWidget::SelectRows);
    DockListStyle::apply(m_functions);

    m_noMatchLabel->setAlignment(Qt::AlignCenter);
    m_noMatchLabel->adjustSize();
    m_noMatchLabel->hide();
    m_searchFilter->setClearButtonEnabled(true);

    m_domain->setEditable(false);
    m_domain->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    QWidget* domainBox = new QWidget(this);
    QHBoxLayout* domainLayout = new QHBoxLayout;
    domainLayout->addWidget(m_domainLabel);
    domainLayout->addWidget(m_domain);
    domainLayout->setSpacing(6);
    domainLayout->setContentsMargins(kControlRowHorizontalPadding,
                                     UiConfig::DockSearchRowVerticalPadding,
                                     kControlRowHorizontalPadding,
                                     0);
    domainBox->setLayout(domainLayout);

    QWidget* searchBox = new QWidget(this);
    QHBoxLayout* searchLayout = new QHBoxLayout;
    searchLayout->addWidget(m_searchLabel);
    searchLayout->addWidget(m_searchFilter);
    searchLayout->setSpacing(6);
    searchLayout->setContentsMargins(kControlRowHorizontalPadding,
                                     UiConfig::DockSearchRowVerticalPadding,
                                     kControlRowHorizontalPadding,
                                     UiConfig::DockSearchRowVerticalPadding);
    searchBox->setLayout(searchLayout);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(domainBox);
    layout->addWidget(searchBox);
    layout->addWidget(m_functions);
    setLayout(layout);

    QWidget::setTabOrder(m_searchFilter, m_functions);
    setFocusProxy(m_searchFilter);

    translateControls(ListUpdate::DeferPopulation);

    connect(m_filterTimer, SIGNAL(timeout()), SLOT(updateList()));
    connect(m_domain, SIGNAL(activated(int)), SLOT(updateList()));
    connect(m_functions, SIGNAL(itemActivated(QTreeWidgetItem*, int)), SLOT(handleItemActivated(QTreeWidgetItem*, int)));
    connect(m_searchFilter, SIGNAL(textChanged(const QString &)), SLOT(triggerFilter()));
    QShortcut* returnShortcut = new QShortcut(QKeySequence(Qt::Key_Return), this);
    returnShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(returnShortcut, &QShortcut::activated, this, [this]() {
        if (const QTreeWidgetItem* current = m_functions->currentItem())
            emit functionSelected(current->text(0));
    });
    QShortcut* enterShortcut = new QShortcut(QKeySequence(Qt::Key_Enter), this);
    enterShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(enterShortcut, &QShortcut::activated, this, [this]() {
        if (const QTreeWidgetItem* current = m_functions->currentItem())
            emit functionSelected(current->text(0));
    });

    restoreState(state.domain, state.searchText);
}

FunctionsWidget::~FunctionsWidget()
{
    m_filterTimer->stop();
}

void FunctionsWidget::updateList()
{
    m_filterTimer->stop();
    QString term = m_searchFilter->text();
    const QString selectedDomain = m_domain->currentText();
    QStringList functionNames = FunctionRepo::instance()->getIdentifiers();
    FunctionRepo::instance()->retranslateText();

    QStringList inputs {tr("All"), selectedDomain, term, QString::number(layoutDirection())};
    inputs.reserve(inputs.size() + functionNames.size() * 3);
    for (const QString& identifier : functionNames) {
        if (const Function* function = FunctionRepo::instance()->find(identifier))
            inputs << identifier << function->name() << function->domain();
    }
    const bool unchanged = inputs == m_populatedInputs;
    StartupDiagnostics::AppearanceScope timing(unchanged
        ? "Skip unchanged functions dock list" : "Populate functions dock list", this);
    if (unchanged)
        return;

    setUpdatesEnabled(false);
    m_functions->clear();
    for (int k = 0; k < functionNames.count(); ++k) {
        const QString identifier = functionNames.at(k);
        Function* f = FunctionRepo::instance()->find(identifier);
        if (!f)
            continue;

        QStringList str;
        str << identifier << f->name();

        const bool domainMatches = selectedDomain == tr("All") || f->domain() == selectedDomain;
        if (term.isEmpty()
            || str.at(0).contains(term, Qt::CaseInsensitive)
            || str.at(1).contains(term, Qt::CaseInsensitive))
        {
            if (domainMatches) {
                QTreeWidgetItem* item = new QTreeWidgetItem(m_functions, str);
                if (layoutDirection() == Qt::LeftToRight) {
                    item->setTextAlignment(0, Qt::AlignLeft);
                    item->setTextAlignment(1, Qt::AlignLeft);
                } else {
                    item->setTextAlignment(0, Qt::AlignRight);
                    item->setTextAlignment(1, Qt::AlignLeft);
                }
            }
        }
    }

    m_functions->resizeColumnToContents(0);
    m_functions->resizeColumnToContents(1);

    if (m_functions->topLevelItemCount() > 0) {
        m_noMatchLabel->hide();
        m_functions->sortItems(0, Qt::AscendingOrder);
    } else {
        DockListStyle::showCenteredNoMatchLabel(m_functions, m_noMatchLabel);
    }

    m_populatedInputs = std::move(inputs);
    setUpdatesEnabled(true);
}

void FunctionsWidget::retranslateText()
{
    translateControls(ListUpdate::Populate);
}

void FunctionsWidget::translateControls(ListUpdate mode)
{
    FunctionRepo::instance()->retranslateText();
    QStringList titles;
    const QString identifier = tr("Identifier");
    const QString name = tr("Name");
    titles << identifier << name;
    m_functions->setHeaderLabels(titles);

    m_searchLabel->setText(tr("Search"));
    m_domainLabel->setText(tr("Domain"));
    updateControlLabelWidths();
    const QString selectedDomain = m_domain->currentText();
    const int selectedDomainIndex = m_domain->currentIndex();
    m_domain->clear();
    m_domain->addItem(tr("All"));
    m_domain->addItems(FunctionRepo::instance()->domains());
    int selectedIndex = m_domain->findText(selectedDomain);
    if (selectedIndex < 0 && selectedDomainIndex >= 0 && selectedDomainIndex < m_domain->count())
        selectedIndex = selectedDomainIndex;
    m_domain->setCurrentIndex(selectedIndex >= 0 ? selectedIndex : 0);
    m_noMatchLabel->setText(tr("No match found"));

    if (mode == ListUpdate::Populate)
        updateList();
}

QList<QTreeWidgetItem*> FunctionsWidget::selectedItems() const
{
    return m_functions->selectedItems();
}

const QTreeWidgetItem* FunctionsWidget::currentItem() const
{
    return m_functions->currentItem();
}

QString FunctionsWidget::searchText() const { return m_searchFilter->text(); }
void FunctionsWidget::setSearchText(const QString& text) { m_searchFilter->setText(text); }
QString FunctionsWidget::selectedDomain() const { return m_domain->currentText(); }
void FunctionsWidget::restoreState(const QString& domain, const QString& searchText)
{
    StartupDiagnostics::AppearanceContext context("Restore functions dock filters");
    const QSignalBlocker searchBlocker(m_searchFilter);
    const int index = m_domain->findText(domain);
    if (index >= 0)
        m_domain->setCurrentIndex(index);
    if (!searchText.isNull())
        m_searchFilter->setText(searchText);
    updateList();
}

void FunctionsWidget::setSelectedDomain(const QString& domain)
{
    StartupDiagnostics::AppearanceContext context("Functions dock domain change");
    const int index = m_domain->findText(domain);
    if (index >= 0) {
        m_domain->setCurrentIndex(index);
        updateList();
    }
}

void FunctionsWidget::handleItemActivated(QTreeWidgetItem* item, int /*column*/)
{
    emit functionSelected(item->text(0));
}

void FunctionsWidget::clearSelection(QTreeWidgetItem*)
{
    m_functions->clearSelection();
}

void FunctionsWidget::triggerFilter()
{
    m_filterTimer->stop();
    m_filterTimer->start();
}

void FunctionsWidget::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        StartupDiagnostics::AppearanceContext context("Functions dock LanguageChange");
        retranslateText();
    } else {
        QWidget::changeEvent(event);
        if (event->type() == QEvent::LayoutDirectionChange)
            retranslateText();
        if (event->type() == QEvent::FontChange)
            QTimer::singleShot(0, this, [this]() { updateControlLabelWidths(); });
    }
}

void FunctionsWidget::updateControlLabelWidths()
{
    const int labelWidth = std::max(m_domainLabel->sizeHint().width(), m_searchLabel->sizeHint().width());
    m_domainLabel->setFixedWidth(labelWidth);
    m_searchLabel->setFixedWidth(labelWidth);
}
