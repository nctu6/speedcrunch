// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#include "gui/dockcomboboxchevron.h"

#include "core/settings.h"
#include "gui/menustyleutils.h"
#include "gui/uiconfig.h"

#include <QAbstractAnimation>
#include <QAbstractItemView>
#include <QApplication>
#include <QBitmap>
#include <QComboBox>
#include <QEasingCurve>
#include <QEvent>
#include <QFrame>
#include <QPainter>
#include <QPainterPath>
#include <QScopedValueRollback>
#include <QScrollBar>
#include <QStyledItemDelegate>
#include <QStyleOptionComboBox>
#include <QStyleOptionMenuItem>
#include <QVariantAnimation>

namespace {

constexpr int kChevronAnimationMs = 150;
constexpr float kChevronOpacity = 0.76f;

// Qt's menu delegate paints through the combo's style, which also carries
// the closed control's stylesheet. Paint popup items with the system style.
class SystemPopupDelegate : public QStyledItemDelegate {
public:
    SystemPopupDelegate(QComboBox* combo, QAbstractItemView* view)
        : QStyledItemDelegate(view), m_combo(combo) {}

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override
    {
        QStyleOptionViewItem item(option);
        initStyleOption(&item, index);
        if (!usesMenuItems()) {
            item.showDecorationSelected = true;
            QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &item, painter, option.widget);
            return;
        }
        const QStyleOptionMenuItem menu = menuOption(item, index);
        painter->fillRect(menu.rect, menu.palette.window());
        QApplication::style()->drawControl(QStyle::CE_MenuItem, &menu, painter, option.widget);
    }

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        if (!usesMenuItems())
            return QStyledItemDelegate::sizeHint(option, index);
        QStyleOptionViewItem item(option);
        initStyleOption(&item, index);
        const QStyleOptionMenuItem menu = menuOption(item, index);
        return QApplication::style()->sizeFromContents(QStyle::CT_MenuItem, &menu,
                                                       menu.rect.size(), option.widget);
    }

private:
    bool usesMenuItems() const
    {
        QStyleOptionComboBox option;
        option.initFrom(m_combo);
        option.editable = m_combo->isEditable();
        return QApplication::style()->styleHint(QStyle::SH_ComboBox_Popup, &option, m_combo);
    }

    QStyleOptionMenuItem menuOption(const QStyleOptionViewItem& item, const QModelIndex& index) const
    {
        QStyleOptionMenuItem menu;
        menu.rect = menu.menuRect = item.rect;
        menu.direction = item.direction;
        menu.palette = item.palette;
        menu.state = item.state & (QStyle::State_Active | QStyle::State_Enabled | QStyle::State_Selected);
        if (!(index.flags() & Qt::ItemIsEnabled))
            menu.state &= ~QStyle::State_Enabled;
        menu.palette.setCurrentColorGroup(!(menu.state & QStyle::State_Enabled) ? QPalette::Disabled
            : (menu.state & QStyle::State_Active) ? QPalette::Active : QPalette::Inactive);
        menu.font = item.font;
        menu.fontMetrics = item.fontMetrics;
        menu.text = item.text;
        menu.text.replace(QLatin1Char('&'), QStringLiteral("&&"));
        menu.icon = item.icon;
        menu.maxIconWidth = item.decorationSize.width() + 4;
        menu.menuItemType = QStyleOptionMenuItem::Normal;
        menu.checkType = QStyleOptionMenuItem::NonExclusive;
        menu.checked = m_combo->currentIndex() == index.row();
        return menu;
    }

    QComboBox* m_combo;
};

void removeFrame(QWidget* widget)
{
    QFrame* frame = qobject_cast<QFrame*>(widget);
    if (frame == nullptr)
        return;

    frame->setFrameShape(QFrame::NoFrame);
    frame->setLineWidth(0);
    frame->setMidLineWidth(0);
}

void applyRoundedMask(QWidget* widget)
{
    if (widget == nullptr || widget->size().isEmpty())
        return;

    QBitmap mask(widget->size());
    mask.fill(Qt::color0);
    QPainter painter(&mask);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::color1);
    painter.drawRoundedRect(QRectF(mask.rect()).adjusted(0, 0, -1, -1),
                            UiConfig::CompletionPopupCornerRadius,
                            UiConfig::CompletionPopupCornerRadius);
    widget->setMask(mask);
}

} // namespace

DockComboBoxChevron::DockComboBoxChevron(QComboBox* comboBox)
    : QWidget(comboBox)
    , m_comboBox(comboBox)
    , m_animation(new QVariantAnimation(this))
    , m_nativeButtonStyle(comboBox->styleSheet())
{
    setObjectName(QStringLiteral("speedcrunchDockComboBoxChevron"));
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);
    setAutoFillBackground(false);
    setFocusPolicy(Qt::NoFocus);

    m_animation->setDuration(kChevronAnimationMs);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_animation, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        m_rotation = value.toReal();
        update();
    });
    connect(comboBox, QOverload<int>::of(&QComboBox::activated), this, [this]() {
        setPopupOpen(false);
    });
    comboBox->installEventFilter(this);
    installPopupEventFilters();
    reposition();
    show();
}

void DockComboBoxChevron::apply(QComboBox* comboBox,
                                const QColor& textColor,
                                const QColor& outlineColor,
                                const PopupTheme& popupTheme)
{
    if (comboBox == nullptr)
        return;

    DockComboBoxChevron* chevron = nullptr;
    if (QWidget* existing = comboBox->findChild<QWidget*>(
            QStringLiteral("speedcrunchDockComboBoxChevron"),
            Qt::FindDirectChildrenOnly)) {
        chevron = dynamic_cast<DockComboBoxChevron*>(existing);
    }
    if (chevron == nullptr)
        chevron = new DockComboBoxChevron(comboBox);

    comboBox->setProperty("speedcrunchDockComboBox", true);
    chevron->m_popupTheme = popupTheme;
    chevron->setColors(textColor, outlineColor);
    chevron->refresh();
}

void DockComboBoxChevron::refreshPopupAppearance(QComboBox* comboBox)
{
    if (comboBox == nullptr)
        return;
    if (auto* chevron = dynamic_cast<DockComboBoxChevron*>(comboBox->findChild<QWidget*>(
            QStringLiteral("speedcrunchDockComboBoxChevron"), Qt::FindDirectChildrenOnly)))
        chevron->refresh();
}

void DockComboBoxChevron::applySystemAppearance(QComboBox* comboBox)
{
    apply(comboBox, QColor(), QColor(), PopupTheme());
}

void DockComboBoxChevron::setColors(QColor chevronColor, const QColor& outlineColor)
{
    chevronColor.setAlphaF(kChevronOpacity);
    if (m_chevronColor == chevronColor && m_outlineColor == outlineColor)
        return;

    m_chevronColor = chevronColor;
    m_outlineColor = outlineColor;
    update();
}

void DockComboBoxChevron::refresh()
{
    installPopupEventFilters();
    reposition();
    setPopupOpen(m_view != nullptr && m_view->isVisible());
    raise();
    setVisible(Settings::instance()->dockAppearance != Settings::DockAppearanceSystem);
}

bool DockComboBoxChevron::eventFilter(QObject* watched, QEvent* event)
{
    if (m_stylingPopup)
        return false;
    if (watched == m_comboBox) {
        switch (event->type()) {
        case QEvent::Move:
        case QEvent::Resize:
        case QEvent::Show:
        case QEvent::StyleChange:
            reposition();
            stylePopupChrome();
            break;
        case QEvent::Hide:
            setPopupOpen(false);
            break;
        case QEvent::KeyPress:
        case QEvent::MouseButtonPress:
            installPopupEventFilters();
            break;
        default:
            break;
        }
    } else if (watched == m_view || watched == m_popupWindow) {
        if (event->type() == QEvent::Show) {
            stylePopupChrome();
            setPopupOpen(true);
        } else if (event->type() == QEvent::Hide) {
            setPopupOpen(false);
        } else if (event->type() == QEvent::Resize) {
            stylePopupChrome();
        }
    }

    return QWidget::eventFilter(watched, event);
}

void DockComboBoxChevron::paintEvent(QPaintEvent*)
{
    if (!m_chevronColor.isValid())
        return;

    const qreal dpr = devicePixelRatioF();
    const auto aligned = [dpr](qreal value) {
        return qRound(value * dpr) / dpr;
    };

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.translate(QPointF(aligned(width() / 2.0), aligned(height() / 2.0)));
    painter.rotate(m_rotation);

    QPainterPath path;
    path.moveTo(QPointF(-5.0, -3.0));
    path.lineTo(QPointF(0.0, 3.0));
    path.lineTo(QPointF(5.0, -3.0));

    QPen pen(m_chevronColor, 1.65, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);
}

void DockComboBoxChevron::installPopupEventFilters()
{
    if (m_comboBox == nullptr)
        return;

    QAbstractItemView* view = m_comboBox->view();
    if (m_view != view) {
        if (m_view != nullptr)
            m_view->removeEventFilter(this);
        m_view = view;
        if (m_view != nullptr) {
            m_themedDelegate = m_view->itemDelegate();
            m_systemDelegate = new SystemPopupDelegate(m_comboBox, m_view);
            m_nativeView.capture(m_view);
            m_nativeViewport.capture(m_view->viewport());
            m_view->installEventFilter(this);
        }
    }

    QWidget* popupWindow = m_view != nullptr ? m_view->window() : nullptr;
    if (popupWindow == m_comboBox->window())
        popupWindow = nullptr;
    if (m_popupWindow != popupWindow) {
        if (m_popupWindow != nullptr)
            m_popupWindow->removeEventFilter(this);
        m_popupWindow = popupWindow;
        if (m_popupWindow != nullptr) {
            m_nativePopup.capture(m_popupWindow);
            m_popupWindow->installEventFilter(this);
        }
    }

    stylePopupChrome();
}

QWidget* DockComboBoxChevron::popupChromeWidget() const
{
    if (m_popupWindow != nullptr)
        return m_popupWindow;
    return m_view;
}

void DockComboBoxChevron::reposition()
{
    if (m_comboBox == nullptr)
        return;

    setGeometry(qMax(0, m_comboBox->width() - IndicatorWidth),
                0,
                IndicatorWidth,
                m_comboBox->height());
}

void DockComboBoxChevron::stylePopupChrome()
{
    const bool systemDock = Settings::instance()->dockAppearance == Settings::DockAppearanceSystem;
    if (m_view == nullptr || m_stylingPopup || (!systemDock && m_popupTheme.viewStyle.isEmpty()))
        return;
    QScopedValueRollback<bool> guard(m_stylingPopup, true);
    const bool system = systemDock || Settings::instance()->menuAppearance == Settings::MenuAppearanceSystem;
    if (!system && m_view->itemDelegate() == m_systemDelegate && m_themedDelegate)
        m_view->setItemDelegate(m_themedDelegate);
    const QString comboStyle = systemDock ? m_nativeButtonStyle : m_popupTheme.buttonStyle + (system ? QString() : QStringLiteral(
        "QComboBox QAbstractItemView { background-color: %1; color: %2; border: 0; outline: 0; }")
        .arg(m_popupTheme.palette.color(QPalette::Base).name(),
             m_popupTheme.palette.color(QPalette::Text).name()));
    if (m_comboBox->styleSheet() != comboStyle) {
        // Popup rules also repolish the button. Keep its dock colors intact.
        const QPalette buttonPalette = m_comboBox->palette();
        m_comboBox->setStyleSheet(comboStyle);
        m_comboBox->setPalette(buttonPalette);
    }
    setVisible(!systemDock);

    if (system) {
        if (m_view->itemDelegate() != m_systemDelegate) {
            m_themedDelegate = m_view->itemDelegate();
            m_view->setItemDelegate(m_systemDelegate);
        }
        const QPalette palette = MenuStyle::systemComboPopupPalette(m_comboBox);
        m_nativeView.restore(m_view, palette);
        m_nativeViewport.restore(m_view->viewport(), palette);
        if (m_popupWindow != nullptr)
            m_nativePopup.restore(m_popupWindow, palette);
        for (QScrollBar* bar : {m_view->verticalScrollBar(), m_view->horizontalScrollBar()}) {
            if (!bar->styleSheet().isEmpty())
                bar->setStyleSheet(QString());
            bar->setPalette(MenuStyle::systemPalette(bar));
        }
        return;
    }

    const QString viewStyle = m_popupTheme.viewStyle + m_popupTheme.scrollBarStyle;
    if (m_view->styleSheet() != viewStyle)
        m_view->setStyleSheet(viewStyle);
    m_view->setPalette(m_popupTheme.palette);
    m_view->viewport()->setPalette(m_popupTheme.palette);
    for (QScrollBar* bar : {m_view->verticalScrollBar(), m_view->horizontalScrollBar()}) {
        if (bar->styleSheet() != m_popupTheme.scrollBarStyle)
            bar->setStyleSheet(m_popupTheme.scrollBarStyle);
        bar->setPalette(m_popupTheme.palette);
    }

    removeFrame(m_view);
    m_view->setAutoFillBackground(false);
    m_view->viewport()->setAutoFillBackground(false);
    m_view->viewport()->setAttribute(Qt::WA_StyledBackground, true);

    QWidget* popupChrome = popupChromeWidget();
    if (popupChrome == nullptr)
        return;

    removeFrame(popupChrome);
    popupChrome->setAutoFillBackground(false);
    popupChrome->setAttribute(Qt::WA_StyledBackground, true);
    if (popupChrome != m_view) {
        popupChrome->setObjectName(QStringLiteral("speedcrunchDockComboBoxPopupChrome"));
        const QString style = QStringLiteral(
            "QWidget#speedcrunchDockComboBoxPopupChrome,"
            "QFrame#speedcrunchDockComboBoxPopupChrome {"
            " background: transparent; border: 0;"
            "}");
        if (popupChrome->styleSheet() != style)
            popupChrome->setStyleSheet(style);
    }
    applyRoundedMask(popupChrome);
}

void DockComboBoxChevron::NativeSurface::capture(QWidget* widget)
{
    if (auto* frame = qobject_cast<QFrame*>(widget)) {
        shape = frame->frameShape();
        shadow = frame->frameShadow();
        lineWidth = frame->lineWidth();
        midLineWidth = frame->midLineWidth();
    }
    autoFill = widget->autoFillBackground();
    styledBackground = widget->testAttribute(Qt::WA_StyledBackground);
    mask = widget->mask();
    styleSheet = widget->styleSheet();
}

void DockComboBoxChevron::NativeSurface::restore(QWidget* widget, const QPalette& palette) const
{
    if (widget->styleSheet() != styleSheet)
        widget->setStyleSheet(styleSheet);
    if (auto* frame = qobject_cast<QFrame*>(widget)) {
        frame->setFrameShape(shape);
        frame->setFrameShadow(shadow);
        frame->setLineWidth(lineWidth);
        frame->setMidLineWidth(midLineWidth);
    }
    widget->setPalette(palette);
    widget->setAutoFillBackground(autoFill);
    widget->setAttribute(Qt::WA_StyledBackground, styledBackground);
    if (mask.isEmpty())
        widget->clearMask();
    else
        widget->setMask(mask);
}

void DockComboBoxChevron::setPopupOpen(bool open)
{
    if (open)
        stylePopupChrome();

    if (m_popupOpen == open && m_animation->state() != QAbstractAnimation::Running)
        return;

    m_popupOpen = open;
    m_animation->stop();
    m_animation->setStartValue(m_rotation);
    m_animation->setEndValue(open ? 180.0 : 0.0);
    m_animation->start();
}
