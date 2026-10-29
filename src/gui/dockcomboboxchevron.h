// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef GUI_DOCKCOMBOBOXCHEVRON_H
#define GUI_DOCKCOMBOBOXCHEVRON_H

#include <QColor>
#include <QFrame>
#include <QPalette>
#include <QPointer>
#include <QRegion>
#include <QWidget>

class QAbstractItemView;
class QAbstractItemDelegate;
class QComboBox;
class QEvent;
class QPaintEvent;
class QVariantAnimation;

class DockComboBoxChevron : public QWidget {
public:
    struct PopupTheme {
        QString buttonStyle;
        QString viewStyle;
        QString scrollBarStyle;
        QPalette palette;
    };

    static constexpr int IndicatorWidth = 28;

    explicit DockComboBoxChevron(QComboBox* comboBox);

    static void apply(QComboBox* comboBox, const QColor& textColor, const QColor& outlineColor,
                      const PopupTheme& popupTheme);
    static void refreshPopupAppearance(QComboBox* comboBox);
    static void applySystemAppearance(QComboBox* comboBox);

    void setColors(QColor chevronColor, const QColor& outlineColor);
    void refresh();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    struct NativeSurface {
        QFrame::Shape shape = QFrame::NoFrame;
        QFrame::Shadow shadow = QFrame::Plain;
        int lineWidth = 0;
        int midLineWidth = 0;
        bool autoFill = false;
        bool styledBackground = false;
        QRegion mask;
        QString styleSheet;
        void capture(QWidget* widget);
        void restore(QWidget* widget, const QPalette& palette) const;
    };
    void installPopupEventFilters();
    QWidget* popupChromeWidget() const;
    void reposition();
    void stylePopupChrome();
    void setPopupOpen(bool open);

    QPointer<QComboBox> m_comboBox;
    QPointer<QAbstractItemView> m_view;
    QPointer<QWidget> m_popupWindow;
    QPointer<QAbstractItemDelegate> m_themedDelegate;
    QPointer<QAbstractItemDelegate> m_systemDelegate;
    QVariantAnimation* m_animation;
    QColor m_chevronColor;
    QColor m_outlineColor;
    PopupTheme m_popupTheme;
    QString m_nativeButtonStyle;
    NativeSurface m_nativeView;
    NativeSurface m_nativeViewport;
    NativeSurface m_nativePopup;
    bool m_stylingPopup = false;
    qreal m_rotation = 0.0;
    bool m_popupOpen = false;
};

#endif // GUI_DOCKCOMBOBOXCHEVRON_H
