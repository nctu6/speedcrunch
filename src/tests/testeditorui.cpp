// SPDX-FileCopyrightText: 2026 SpeedCrunch developers
// SPDX-License-Identifier: GPL-2.0-or-later


#include "gui/editor.h"
#include "gui/editorutils.h"
#include "gui/oklchutils.h"
#include "gui/syntaxhighlighter.h"
#include "gui/uiconfig.h"
#include "core/colorscheme.h"
#include "core/constants.h"
#include "core/evaluator.h"
#include "core/session.h"
#include "core/settings.h"
#include "core/unicodechars.h"
#include "core/mathdsl.h"
#include "core/numberformatter.h"
#include "gui/displayformatutils.h"
#include "numberformatcases.h"
#include "fractionalgroupingcases.h"
#include "numbergroupingedgecases.h"
#include "malformednumbergroupingcases.h"
#include "decimalinputcases.h"
#include "interpretednumbercases.h"
#include "gui/resultdisplay.h"
#include "core/units.h"
#include "core/userfunction.h"
#include "core/userunit.h"
#include "uitestfixture.h"
#include "editorgeometrytesthelpers.h"

#include <QApplication>
#include <QAbstractItemDelegate>
#include <QClipboard>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QInputMethodEvent>
#include <QFontDatabase>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMimeData>
#include <QPalette>
#include <QPainter>
#include <QPushButton>
#include <QScrollBar>
#include <QScreen>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QStyleOptionViewItem>
#include <QTest>
#include <QTextLayout>
#include <QElapsedTimer>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWindow>

namespace {
QJsonObject themeJson(QJsonObject colors)
{
    colors.insert(QStringLiteral("$schema"), QString::fromLatin1(ColorScheme::SchemaDraft));
    colors.insert(QStringLiteral("$id"), QString::fromLatin1(ColorScheme::SchemaId));
    return colors;
}

QString themeJsonString(QJsonObject colors)
{
    return QString::fromUtf8(QJsonDocument(themeJson(colors)).toJson(QJsonDocument::Compact));
}
}

class TestEditorUi : public QObject {
    Q_OBJECT

private slots:
    void init() { UiTestFixture::resetSettings(); }
    void raw_input_preserves_text_data();
    void raw_input_preserves_text();
    void history_recall_respects_input_adjustments_data();
    void history_recall_respects_input_adjustments();
    void raw_input_keeps_character_navigation_and_deletion();
    void raw_input_preserves_calculator_commands();
    void raw_input_switch_preserves_document_and_restores_adjustments();
    void raw_input_preserves_visible_digit_grouping();
    void raw_input_preserves_tab_completion_data();
    void raw_input_preserves_tab_completion();
    void automatic_input_adjustments_survive_settings_reload();
    void preserves_comment_text_during_input_data();
    void preserves_comment_text_during_input();
    void normalizes_only_expression_text_when_replacing_comment_selection();
    void blocks_consecutive_plus();
    void respects_keyboard_layout_for_equals_and_plus_data();
    void respects_keyboard_layout_for_equals_and_plus();
    void wraps_left_shift_operator_and_blocks_immediate_duplicate_less_than();
    void wraps_right_shift_operator_and_blocks_immediate_duplicate_greater_than();
    void blocks_consecutive_caret();
    void blocks_plus_after_caret();
    void blocks_leading_operators_when_auto_ans_is_off_except_configured_exceptions();
    void keeps_disallowed_start_chars_blocked_when_auto_ans_is_on();
    void allows_special_function_symbols_as_leading_chars();
    void allows_list_start_after_operators();
    void highlights_list_braces_as_parentheses();
    void input_digit_grouping_formats_only_integer_digits_data();
    void input_digit_grouping_formats_only_integer_digits();
    void input_digit_grouping_preserves_typing_selection_and_undo();
    void input_digit_grouping_survives_settings_reload();
    void input_digit_grouping_follows_number_format_data();
    void input_digit_grouping_follows_number_format();
    void number_separators_keep_value_when_typed_or_pasted();
    void input_preserves_typed_and_pasted_number_separators();
    void validated_number_format_examples_data();
    void validated_number_format_examples();
    void ambiguous_fractional_grouping_prefers_ungrouped_data();
    void ambiguous_fractional_grouping_prefers_ungrouped();
    void number_grouping_edge_cases_data();
    void number_grouping_edge_cases();
    void malformed_number_grouping_all_formats_data();
    void malformed_number_grouping_all_formats();
    void decimal_input_without_display_grouping_data();
    void decimal_input_without_display_grouping();
    void decimal_input_all_number_formats_data();
    void decimal_input_all_number_formats();
    void number_input_interpreted_expressions_all_formats_data();
    void number_input_interpreted_expressions_all_formats();
    void input_digit_grouping_paints_separator_glyphs_data();
    void input_digit_grouping_paints_separator_glyphs();
    void typing_hex_separators_preserves_number_data();
    void typing_hex_separators_preserves_number();
    void spaced_hex_decode_live_preview();
    void manual_grouping_keeps_punctuation_data();
    void manual_grouping_keeps_punctuation();
    void auto_ans_rewrite_helper_handles_tilde_and_factorial();
    void blocks_operator_right_after_open_square_bracket();
    void inserts_value_unit_space_brackets_after_number_or_symbol();
    void inserts_closing_square_bracket_only_with_spaces_to_right_data();
    void inserts_closing_square_bracket_only_with_spaces_to_right();
    void ignores_space_on_empty_or_all_space_editor();
    void auto_inserts_space_before_question_comment_only_with_non_space_content();
    void inserts_middle_dot_on_space_after_identifier_or_closed_group();
    void inserts_parenthesis_pair_and_places_cursor_inside();
    void does_not_insert_implicit_mul_sequence_before_open_paren_after_number_or_symbol();
    void does_not_insert_implicit_mul_for_zero_radix_prefix_letters();
    void allows_unit_conversion_tail_after_spaced_subtraction_operator();
    void converts_double_minus_sequence_to_unit_conversion_with_placeholder();
    void converts_double_minus_sequence_to_unit_conversion_after_masculine_ordinal_degree();
    void inserts_unit_conversion_with_placeholder_when_typing_arrow_symbol();
    void treats_spaced_unit_conversion_as_atomic_navigation_and_edit_token();
    void treats_spaced_shift_operators_as_atomic_navigation_and_edit_tokens();
    void backspace_removes_typed_operator_after_unit();
    void editing_keys_bypass_typing_restrictions_data();
    void editing_keys_bypass_typing_restrictions();
    void undo_redo_shortcuts_preserve_expression_data();
    void undo_redo_shortcuts_preserve_expression();
    void typing_undoes_one_key_at_a_time_data();
    void typing_undoes_one_key_at_a_time();
    void non_printable_key_payloads_do_not_replace_selection();
    void clipboard_shortcuts_do_not_insert_key_payloads_data();
    void clipboard_shortcuts_do_not_insert_key_payloads();
    void copy_shortcut_preserves_editor_state_data();
    void copy_shortcut_preserves_editor_state();
    void highlight_refresh_preserves_text_and_selection_without_edit_signals();
    void primary_theme_changes_preserve_document_highlighting_data();
    void primary_theme_changes_preserve_document_highlighting();
    void highlight_refresh_updates_document_once_data();
    void highlight_refresh_updates_document_once();
    void editor_text_line_is_vertically_centered_data();
    void editor_text_line_is_vertically_centered();
    void unchanged_primary_theme_avoids_editor_style_refresh_data();
    void unchanged_primary_theme_avoids_editor_style_refresh();
    void treats_spaced_question_comment_as_atomic_navigation_and_edit_token();
    void treats_spaced_equal_as_atomic_navigation_and_edit_token();
    void treats_leading_question_comment_as_atomic_navigation_and_edit_token();
    void ignores_space_right_after_spaced_unit_conversion_operator();
    void unit_bracket_context_accepts_div_mul_and_rejects_addition();
    void unit_bracket_context_blocks_space_and_ops_when_only_spaces();
    void unit_bracket_context_accepts_quote_marks_as_arc_units();
    void unit_bracket_context_accepts_quote_marks_via_ime_commit();
    void unit_bracket_context_allows_letter_after_middle_dot();
    void unit_bracket_context_disallows_variables_and_constants();
    void evaluator_accepts_compact_arc_symbol_units();
    void unit_bracket_context_allows_digits_and_minus_only_in_exponent_positions();
    void converts_caret_exponents_to_superscripts_globally();
    void keeps_scientific_notation_exponent_minus_unwrapped();
    void blocks_shift_operator_tail_after_non_plus_operator();
    void rewrites_superscript_exponent_for_radix_and_inserts_mul_space_globally();
    void typing_asterisk_after_superscript_power_keeps_explicit_multiplication();
    void auto_inserts_zero_before_dot_in_configured_contexts();
    void allows_unrestricted_typing_inside_question_comment_context();
    void allows_currency_symbols_after_operators();
    void blocks_dead_circumflex_key_after_existing_caret();
    void blocks_dead_circumflex_key_after_multiplication_operator();
    void blocks_regular_caret_after_multiplication_operator();
    void blocks_ime_preedit_caret_echo_after_existing_caret();
    void blocks_ime_commit_caret_after_existing_caret();
    void blocks_ime_commit_caret_after_multiplication_operator();
    void finds_completion_keyword_after_superscript_power();
    void completes_replacing_trailing_identifier_after_superscript_power();
    void completes_identifier_immediately_after_digit();
    void completes_pi_identifier_as_pi_symbol();
    void accepts_degree_alias_in_unit_brackets_and_normalizes_to_degree_sign();
    void offers_unit_completion_for_degree_symbol_in_unit_context();
    void matches_micro_units_when_typing_u_in_unit_context();
    void unit_context_completion_excludes_user_variables();
    void unit_context_completion_excludes_built_in_functions();
    void unit_context_completion_excludes_user_functions();
    void unit_context_completion_excludes_built_in_variables();
    void unit_context_completion_includes_angle_units_and_long_forms();
    void unit_context_completion_includes_small_positive_si_prefixed_symbols();
    void unit_context_completion_excludes_prefixed_square_and_cubic_metre();
    void completes_binary_prefixed_information_unit_to_short_form_in_unit_context();
    void completes_day_unit_to_short_form_in_unit_context();
    void completes_hour_unit_to_short_form_in_unit_context();
    void completes_affine_temperature_units_in_unit_context();
    void completes_arc_units_in_unit_context();
    void tooltip_does_not_duplicate_degree_symbol_for_explicit_angle_conversion();
    void tooltip_does_not_append_angle_mode_symbol_after_explicit_arcsecond_unit();
    void tooltip_formats_arcminute_and_arcsecond_results_without_expression();
    void tooltip_rewrites_composite_canonical_angle_symbols_to_aliases();
    void tooltip_shows_radian_suffix_for_negative_sexagesimal_literal();
    void tooltip_trig_output_does_not_append_angle_mode_suffix();
    void tooltip_shows_only_result_for_non_trig_sexagesimal_expression();
    void tooltip_shows_only_result_for_repeated_trig_with_degree_sign();
    void tooltip_shows_only_result_for_mixed_revolution_aliases();
    void tooltip_shows_only_result_for_literal_number_products();
    void tooltip_shows_only_result_data();
    void tooltip_shows_only_result();
    void tooltip_keeps_quantsp_before_degree_celsius();
    void tooltip_handles_affine_temperature_units_without_arrow_and_with_conversion();
    void tooltip_shows_selection_result_when_selecting_with_shift_arrows();
    void tooltip_shows_selection_result_when_selecting_all_with_keyboard_shortcut();
    void tooltip_shows_selection_result_when_selecting_with_mouse();
    void tooltip_does_not_refresh_current_result_on_caret_arrow_move();
    void tooltip_does_not_refresh_current_result_on_mouse_caret_reposition();
    void tooltip_refreshes_current_result_on_char_deletion();
    void enter_evaluates_when_completion_popup_has_no_explicit_interaction();
    void completion_popup_enter_respects_accept_with_enter_data();
    void completion_popup_enter_respects_accept_with_enter();
    void enter_accepts_completion_for_names_and_units_data();
    void enter_accepts_completion_for_names_and_units();
    void enter_evaluates_without_completion_when_accept_with_enter_is_enabled();
    void completion_popup_arrow_keys_change_selection();
    void completion_popup_tab_accepts_selected_item();
    void completion_popup_closes_when_clicking_elsewhere();
    void completion_popup_clicking_row_accepts_item();
    void completion_popup_mouse_selection_keeps_focus_on_owning_editor();
    void completion_popup_scrollbar_click_keeps_popup_open();
    void completion_popup_mouse_wheel_scrolls_popup();
    void completion_popup_escape_closes_and_stays_closed();
    void completion_popup_escape_keeps_focus_on_owning_editor();
    void completion_popup_reopens_after_typing_following_escape();
    void completion_popup_reopens_after_retyping_deleted_letter_data();
    void completion_popup_reopens_after_retyping_deleted_letter();
    void completion_popup_shows_for_two_character_prefixes();
    void completion_popup_uses_positionable_nonactivating_surface();
    void completion_popup_keeps_editor_widgets_non_native_data();
    void completion_popup_keeps_editor_widgets_non_native();
    void completion_popup_follows_editor_geometry_data();
    void completion_popup_follows_editor_geometry();
    void completion_popup_geometry_avoids_editor_overlap_data();
    void completion_popup_geometry_avoids_editor_overlap();
    void completion_popup_shortens_tall_lists_without_covering_editor();
    void completion_popups_configure_editor_anchor_data();
    void completion_popups_configure_editor_anchor();
    void completion_popup_compositor_geometry_data();
    void completion_popup_compositor_geometry();
    void constant_search_filters_without_editing_expression();
    void constant_search_inserts_selected_result_data();
    void constant_search_inserts_selected_result();
    void constant_search_closes_without_stealing_outside_focus();
    void constant_search_layout_follows_popup_resize();
    void constant_search_inline_units_preserve_full_insertion_data();
    void constant_search_inline_units_preserve_full_insertion();
    void constant_search_renders_units_in_secondary_shade_data();
    void constant_search_renders_units_in_secondary_shade();
    void constant_completion_popup_follows_wrapped_editor_cursor();
    void completion_popup_uses_configured_surface_colors();
    void constant_completion_popup_uses_configured_surface_colors();
    void inactive_editor_does_not_show_completion_popup();
    void enter_evaluates_when_cursor_is_immediately_after_operator();
    void completion_popup_uses_expected_icons_for_all_symbol_types();
    void wrap_selection_method_wraps_selected_text();
    void wrap_selection_method_wraps_whole_expression_without_selection();
    void matching_parentheses_use_parens_and_generated_foreground_colors();
    void keeps_wrapped_cursor_line_visible_at_height_cap();
    void rewrites_operators_in_scrolled_expression_data();
    void rewrites_operators_in_scrolled_expression();
    void classic_editor_keeps_wrapped_input_visible_and_preserves_undo();
    void classic_completion_popups_follow_text_inset_data();
    void classic_completion_popups_follow_text_inset();
    void editor_height_adds_only_one_line_height_per_visible_line();
    void editor_keeps_unit_descenders_inside_viewport();
    void adding_second_wrapped_character_keeps_first_line_visible();
    void editor_fill_color_is_15_percent_lighter_for_dark_background_role();
    void editor_fill_color_is_15_percent_darker_for_light_background_role();
    void themed_cursor_has_only_two_states_at_display_scale_data();
    void themed_cursor_has_only_two_states_at_display_scale();
    void non_blinking_cursor_stays_visible_data();
    void non_blinking_cursor_stays_visible();
    void cursor_blinking_survives_settings_reload();
    void cursor_free_painting_preserves_text_and_selections_data();
    void cursor_free_painting_preserves_text_and_selections();
};

static QTreeWidget* s_completionPopupTree()
{
    const auto topLevelWidgets = QApplication::topLevelWidgets();
    for (QWidget* widget : topLevelWidgets) {
        if (!widget || !widget->isVisible())
            continue;
        if (widget->objectName() != QStringLiteral("editorCompletionPopup"))
            continue;
        if (QTreeWidget* tree = qobject_cast<QTreeWidget*>(widget))
            return tree;
    }

    QWidget* popup = QApplication::activePopupWidget();
    if (popup) {
        if (QTreeWidget* tree = qobject_cast<QTreeWidget*>(popup))
            return tree;
    }
    for (QWidget* widget : topLevelWidgets) {
        if (!widget || !widget->isVisible())
            continue;
        if (!(widget->windowFlags() & (Qt::Popup | Qt::Tool))
            && widget->objectName() != QStringLiteral("editorCompletionPopup"))
            continue;
        if (QTreeWidget* tree = qobject_cast<QTreeWidget*>(widget))
            return tree;
    }
    return nullptr;
}

static QString s_popupSymbolForIdentifier(QTreeWidget* popup, const QString& identifier)
{
    if (!popup)
        return QString();
    for (int i = 0; i < popup->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = popup->topLevelItem(i);
        if (item && item->text(1) == identifier)
            return item->text(0);
    }
    return QString();
}

static QTreeWidget* s_completionPopupTreeContaining(const QString& identifierPrefix)
{
    const auto topLevelWidgets = QApplication::topLevelWidgets();
    for (QWidget* widget : topLevelWidgets) {
        if (!widget || !widget->isVisible())
            continue;
        if (widget->objectName() != QStringLiteral("editorCompletionPopup"))
            continue;
        QTreeWidget* tree = qobject_cast<QTreeWidget*>(widget);
        if (!tree)
            continue;
        for (int i = 0; i < tree->topLevelItemCount(); ++i) {
            QTreeWidgetItem* item = tree->topLevelItem(i);
            if (item && item->text(1).startsWith(identifierPrefix))
                return tree;
        }
    }
    return nullptr;
}

static QList<QTreeWidget*> s_constantCompletionPopupTrees()
{
    QList<QTreeWidget*> trees;
    QWidget* popup = QApplication::activePopupWidget();
    if (!popup || popup->objectName() != QStringLiteral("constantCompletionPopup"))
        return trees;

    trees = popup->findChildren<QTreeWidget*>();
    std::sort(trees.begin(), trees.end(), [](QTreeWidget* left, QTreeWidget* right) {
        return left->objectName() < right->objectName();
    });
    return trees;
}

static bool s_editorOrViewportHasFocus(Editor* editor)
{
    QWidget* focusWidget = QApplication::focusWidget();
    return editor->hasFocus()
        || editor->viewport()->hasFocus()
        || focusWidget == editor
        || focusWidget == editor->viewport();
}


void TestEditorUi::blocks_consecutive_plus()
{
    // State: "1"
    // Action: type '+' twice.
    // Expected: second '+' is ignored.
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("1"));
    editor.setCursorPosition(editor.text().size());

    QTest::keyClick(&editor, Qt::Key_Plus, Qt::NoModifier);
    const QString afterFirstPlus = editor.text();
    QVERIFY(afterFirstPlus.contains(MathDsl::AddOp));

    QTest::keyClick(&editor, Qt::Key_Plus, Qt::NoModifier);
    QCOMPARE(editor.text(), afterFirstPlus);
}

void TestEditorUi::respects_keyboard_layout_for_equals_and_plus_data()
{
    QTest::addColumn<int>("key");
    QTest::addColumn<Qt::KeyboardModifiers>("modifiers");
    QTest::addColumn<QString>("payload");
    QTest::addColumn<QString>("initial");
    QTest::addColumn<QString>("expected");

    QTest::newRow("portuguese-shift-equals")
        << int(Qt::Key_Equal) << Qt::KeyboardModifiers(Qt::ShiftModifier)
        << QStringLiteral("=") << QStringLiteral("foo") << QStringLiteral("foo = ");
    QTest::newRow("shift-zero-equals-payload")
        << int(Qt::Key_0) << Qt::KeyboardModifiers(Qt::ShiftModifier)
        << QStringLiteral("=") << QStringLiteral("foo") << QStringLiteral("foo = ");
    QTest::newRow("unshifted-equals")
        << int(Qt::Key_Equal) << Qt::KeyboardModifiers(Qt::NoModifier)
        << QStringLiteral("=") << QStringLiteral("foo") << QStringLiteral("foo = ");
    QTest::newRow("us-shift-equals-plus-payload")
        << int(Qt::Key_Equal) << Qt::KeyboardModifiers(Qt::ShiftModifier)
        << QStringLiteral("+") << QStringLiteral("1") << QStringLiteral("1 + ");
    QTest::newRow("plus-key")
        << int(Qt::Key_Plus) << Qt::KeyboardModifiers(Qt::NoModifier)
        << QStringLiteral("+") << QStringLiteral("1") << QStringLiteral("1 + ");
    QTest::newRow("shift-equals-without-payload")
        << int(Qt::Key_Equal) << Qt::KeyboardModifiers(Qt::ShiftModifier)
        << QString() << QStringLiteral("foo") << QStringLiteral("foo = ");
    QTest::newRow("shift-equals-blocks-duplicate")
        << int(Qt::Key_Equal) << Qt::KeyboardModifiers(Qt::ShiftModifier)
        << QStringLiteral("=") << QStringLiteral("foo = ") << QStringLiteral("foo = ");
    QTest::newRow("shift-plus-blocks-duplicate")
        << int(Qt::Key_Equal) << Qt::KeyboardModifiers(Qt::ShiftModifier)
        << QStringLiteral("+") << QStringLiteral("1 + ") << QStringLiteral("1 + ");
}

void TestEditorUi::respects_keyboard_layout_for_equals_and_plus()
{
    QFETCH(int, key);
    QFETCH(Qt::KeyboardModifiers, modifiers);
    QFETCH(QString, payload);
    QFETCH(QString, initial);
    QFETCH(QString, expected);

    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();
    editor.setText(initial);
    editor.setCursorPosition(editor.text().size());

    QKeyEvent event(QEvent::KeyPress, key, modifiers, payload);
    QApplication::sendEvent(&editor, &event);
    QCOMPARE(editor.document()->toRawText(), expected);
}

void TestEditorUi::wraps_left_shift_operator_and_blocks_immediate_duplicate_less_than()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("foo"));
    editor.setCursorPosition(editor.text().size());

    QKeyEvent lessByText(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QStringLiteral("<"));
    QApplication::sendEvent(&editor, &lessByText);
    const QString afterFirstLess = editor.text();
    QCOMPARE(afterFirstLess, QStringLiteral("foo") + MathDsl::buildWrappedToken(MathDsl::ShiftLeftOp));

    QApplication::sendEvent(&editor, &lessByText);
    QCOMPARE(editor.text(), afterFirstLess);
}

void TestEditorUi::wraps_right_shift_operator_and_blocks_immediate_duplicate_greater_than()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("foo"));
    editor.setCursorPosition(editor.text().size());

    QKeyEvent greaterByText(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QStringLiteral(">"));
    QApplication::sendEvent(&editor, &greaterByText);
    const QString afterFirstGreater = editor.text();
    QCOMPARE(afterFirstGreater, QStringLiteral("foo") + MathDsl::buildWrappedToken(MathDsl::ShiftRightOp));

    QApplication::sendEvent(&editor, &greaterByText);
    QCOMPARE(editor.text(), afterFirstGreater);
}

void TestEditorUi::blocks_consecutive_caret()
{
    // State: "2"
    // Action: type '^' twice.
    // Expected: second '^' is ignored.
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("2"));
    editor.setCursorPosition(editor.text().size());

    QTest::keyClicks(&editor, QStringLiteral("^"));
    const QString afterFirstCaret = editor.text();
    QVERIFY(afterFirstCaret.contains(QLatin1Char('^')));

    QTest::keyClicks(&editor, QStringLiteral("^"));
    QCOMPARE(editor.text(), afterFirstCaret);
}

void TestEditorUi::blocks_plus_after_caret()
{
    // State: "2^"
    // Action: type '+'.
    // Expected: '+' is ignored.
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("2^"));
    editor.setCursorPosition(editor.text().size());

    QTest::keyClick(&editor, Qt::Key_Plus, Qt::NoModifier);
    QCOMPARE(editor.text(), QStringLiteral("2^"));
}

void TestEditorUi::blocks_leading_operators_when_auto_ans_is_off_except_configured_exceptions()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    Settings* settings = Settings::instance();
    const bool autoAnsBackup = settings->autoAns;
    settings->autoAns = false;

    const auto resetEmpty = [&editor]() {
        editor.setText(QString());
        editor.setCursorPosition(0);
    };
    const auto sendTextKey = [&editor](const QString& s) {
        QKeyEvent byText(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, s);
        QApplication::sendEvent(&editor, &byText);
    };
    const auto sendImeCommit = [&editor](const QString& s) {
        QList<QInputMethodEvent::Attribute> attrs;
        QInputMethodEvent imeEvent(QString(), attrs);
        imeEvent.setCommitString(s);
        QApplication::sendEvent(&editor, &imeEvent);
    };

    resetEmpty();
    QTest::keyClick(&editor, Qt::Key_Plus, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QString());
    resetEmpty();
    sendTextKey(QString(MathDsl::AddOp));
    QCOMPARE(editor.document()->toRawText(), QString());

    resetEmpty();
    QTest::keyClick(&editor, Qt::Key_Asterisk, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QString());
    resetEmpty();
    sendTextKey(QString(MathDsl::MulCrossOp));
    QCOMPARE(editor.document()->toRawText(), QString());
    resetEmpty();
    sendTextKey(QString(MathDsl::MulDotOp));
    QCOMPARE(editor.document()->toRawText(), QString());

    resetEmpty();
    QTest::keyClick(&editor, Qt::Key_Slash, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QString());
    resetEmpty();
    sendTextKey(QString(MathDsl::DivOp));
    QCOMPARE(editor.document()->toRawText(), QString());

    resetEmpty();
    sendTextKey(QStringLiteral("!"));
    QCOMPARE(editor.document()->toRawText(), QString());
    resetEmpty();
    sendTextKey(QStringLiteral("^"));
    QCOMPARE(editor.document()->toRawText(), QString());
    resetEmpty();
    sendTextKey(QStringLiteral(")"));
    QCOMPARE(editor.document()->toRawText(), QString());
    resetEmpty();
    sendTextKey(QStringLiteral(":"));
    QCOMPARE(editor.document()->toRawText(), QString());

    resetEmpty();
    QTest::keyClick(&editor, Qt::Key_Minus, Qt::NoModifier);
    QVERIFY(!editor.document()->toRawText().isEmpty());
    const QString afterFirstLeadingMinus = editor.document()->toRawText();
    QTest::keyClick(&editor, Qt::Key_Minus, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), afterFirstLeadingMinus);
    editor.setCursorPosition(0);
    QTest::keyClick(&editor, Qt::Key_Minus, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), afterFirstLeadingMinus);
    resetEmpty();
    sendTextKey(QString(MathDsl::SubOp));
    QVERIFY(!editor.document()->toRawText().isEmpty());
    const QString afterFirstLeadingMinusByImeBase = editor.document()->toRawText();
    sendImeCommit(QString(MathDsl::SubOp));
    QCOMPARE(editor.document()->toRawText(), afterFirstLeadingMinusByImeBase);

    resetEmpty();
    QTest::keyClicks(&editor, QStringLiteral("~"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("~"));

    resetEmpty();
    sendTextKey(QStringLiteral("("));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("()"));
    QCOMPARE(editor.textCursor().position(), 1);

    resetEmpty();
    sendTextKey(QStringLiteral("'"));
    QCOMPARE(editor.document()->toRawText(), QString());

    resetEmpty();
    sendTextKey(QStringLiteral("\""));
    QCOMPARE(editor.document()->toRawText(), QString());

    resetEmpty();
    sendTextKey(QStringLiteral("#"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("#"));

    resetEmpty();
    sendTextKey(QStringLiteral("?"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("? "));

    resetEmpty();
    sendTextKey(QString::fromUtf8("€"));
    QCOMPARE(editor.document()->toRawText(), QString::fromUtf8("€"));

    resetEmpty();
    sendTextKey(QString::fromUtf8("Ж"));
    QCOMPARE(editor.document()->toRawText(), QString::fromUtf8("Ж"));

    settings->autoAns = autoAnsBackup;
}

void TestEditorUi::keeps_disallowed_start_chars_blocked_when_auto_ans_is_on()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    Settings* settings = Settings::instance();
    const bool autoAnsBackup = settings->autoAns;
    settings->autoAns = true;

    const auto resetEmpty = [&editor]() {
        editor.setText(QString());
        editor.setCursorPosition(0);
    };
    const auto sendImeCommit = [&editor](const QString& s) {
        QList<QInputMethodEvent::Attribute> attrs;
        QInputMethodEvent imeEvent(QString(), attrs);
        imeEvent.setCommitString(s);
        QApplication::sendEvent(&editor, &imeEvent);
    };

    // Allowed starters in auto-ans mode.
    resetEmpty();
    sendImeCommit(QStringLiteral("!"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("!"));
    resetEmpty();
    sendImeCommit(QStringLiteral("~"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("~"));

    // Disallowed symbols still stay blocked.
    resetEmpty();
    sendImeCommit(QString::fromUtf8("§"));
    QCOMPARE(editor.document()->toRawText(), QString());
    resetEmpty();
    sendImeCommit(QStringLiteral("]"));
    QCOMPARE(editor.document()->toRawText(), QString());

    settings->autoAns = autoAnsBackup;
}

void TestEditorUi::allows_special_function_symbols_as_leading_chars()
{
    const bool autoAnsOff = false;
    const bool autoAnsOn = true;
    const QList<QChar> allowedStarters = {
        UnicodeChars::Summation,
        UnicodeChars::SquareRoot,
        UnicodeChars::CubeRoot,
        UnicodeChars::LowLine,
        UnicodeChars::DollarSign,
        MathDsl::ListStart,
        QChar(0x0436), // Ж
        QChar(0x03C0)  // π
    };
    for (const QChar ch : allowedStarters) {
        QVERIFY(EditorUtils::isAllowedLeadingCharAtExpressionStart(ch, autoAnsOff));
        QVERIFY(EditorUtils::isAllowedLeadingCharAtExpressionStart(ch, autoAnsOn));
    }

    QVERIFY(!EditorUtils::isAllowedLeadingCharAtExpressionStart(QChar(0x00A7), autoAnsOff)); // §
    QVERIFY(!EditorUtils::isAllowedLeadingCharAtExpressionStart(QChar(0x00A7), autoAnsOn)); // §
}

void TestEditorUi::allows_list_start_after_operators()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    const QString listStart(MathDsl::ListStart);
    const QVector<QPair<QString, QString>> cases = {
        {QStringLiteral("1+"), QStringLiteral("1+")},
        {QStringLiteral("1-"), QStringLiteral("1") + QString(MathDsl::SubOp)},
        {QStringLiteral("1/"), QStringLiteral("1/")},
        {QStringLiteral("1*"), QStringLiteral("1") + QString(MathDsl::MulCrossOp)},
        {QStringLiteral("1^"), QStringLiteral("1^")}
    };
    for (const auto& testCase : cases) {
        editor.setText(testCase.first);
        editor.setCursorPosition(editor.text().size());
        QKeyEvent openListByText(
            QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, listStart);
        QApplication::sendEvent(&editor, &openListByText);
        QCOMPARE(editor.document()->toRawText(), testCase.second + listStart);
    }
}

void TestEditorUi::highlights_list_braces_as_parentheses()
{
    Settings* settings = Settings::instance();
    const bool oldSyntaxHighlighting = settings->syntaxHighlighting;
    settings->syntaxHighlighting = true;

    QPlainTextEdit editor;
    SyntaxHighlighter highlighter(&editor);
    QJsonObject colors;
    colors.insert(QStringLiteral("number"), QStringLiteral("#111111"));
    colors.insert(QStringLiteral("parens"), QStringLiteral("#123456"));
    colors.insert(QStringLiteral("list"), QStringLiteral("#123456"));
    colors.insert(QStringLiteral("operator"), QStringLiteral("#222222"));
    colors.insert(QStringLiteral("separator"), QStringLiteral("#333333"));
    highlighter.setColorScheme(ColorScheme(QJsonDocument(themeJson(colors))));

    editor.setPlainText(QStringLiteral("{1; 2}"));
    highlighter.rehighlight();

    const QList<QTextLayout::FormatRange> formats =
        editor.document()->firstBlock().layout()->formats();
    auto colorAt = [&formats](int pos) {
        for (const QTextLayout::FormatRange& range : formats) {
            if (pos >= range.start && pos < range.start + range.length)
                return range.format.foreground().color();
        }
        return QColor();
    };

    const QColor parensColor(QStringLiteral("#123456"));
    QCOMPARE(colorAt(0), parensColor);
    QCOMPARE(colorAt(5), parensColor);

    settings->syntaxHighlighting = oldSyntaxHighlighting;
}

void TestEditorUi::auto_ans_rewrite_helper_handles_tilde_and_factorial()
{
    const auto modeTilde = EditorUtils::autoAnsRewriteModeForLeadingOperator(QStringLiteral("~"));
    QCOMPARE(modeTilde, EditorUtils::AutoAnsAppendAns);
    QCOMPARE(EditorUtils::applyAutoAnsRewrite(QStringLiteral("~"), modeTilde),
             QStringLiteral("~ans"));

    const auto modeFactorial = EditorUtils::autoAnsRewriteModeForLeadingOperator(QStringLiteral("!"));
    QCOMPARE(modeFactorial, EditorUtils::AutoAnsPrependAns);
    QCOMPARE(EditorUtils::applyAutoAnsRewrite(QStringLiteral("!"), modeFactorial),
             QStringLiteral("ans!"));
}

void TestEditorUi::blocks_operator_right_after_open_square_bracket()
{
    // State: "["
    // Action: type '-'.
    // Expected: operator is ignored at start of unit-bracket context.
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("["));
    editor.setCursorPosition(editor.text().size());

    QTest::keyClick(&editor, Qt::Key_Minus, Qt::NoModifier);
    QCOMPARE(editor.text(), QStringLiteral("["));
}

void TestEditorUi::inserts_value_unit_space_brackets_after_number_or_symbol()
{
    // State: "2" and "π"
    // Action: type '[' via text-based key event.
    // Expected: append "<ValueUnitSpace>[]".
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("2"));
    editor.setCursorPosition(editor.text().size());

    QKeyEvent bracketByText(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QStringLiteral("["));
    QApplication::sendEvent(&editor, &bracketByText);

    const QString actualAfterNumber = editor.document()->toRawText();
    QCOMPARE(actualAfterNumber, QStringLiteral("2") + QString(MathDsl::QuantSp) + QStringLiteral("[]"));

    editor.setText(QString::fromUtf8("π"));
    editor.setCursorPosition(editor.text().size());

    QApplication::sendEvent(&editor, &bracketByText);

    const QString actualAfterSymbol = editor.document()->toRawText();
    QCOMPARE(actualAfterSymbol, QString::fromUtf8("π") + QString(MathDsl::QuantSp) + QStringLiteral("[]"));

    editor.setText(QStringLiteral("2")
                   + QString(MathDsl::QuantSp)
                   + QStringLiteral("[m]")
                   + QString(MathDsl::MulDotWrapSp)
                   + QString(MathDsl::MulDotOp)
                   + QString(MathDsl::MulDotWrapSp)
                   + QStringLiteral("(pi)"));
    editor.setCursorPosition(editor.text().size());
    QApplication::sendEvent(&editor, &bracketByText);
    QCOMPARE(editor.document()->toRawText(),
             QStringLiteral("2")
                 + QString(MathDsl::QuantSp)
                 + QStringLiteral("[m]")
                 + QString(MathDsl::MulDotWrapSp)
                 + QString(MathDsl::MulDotOp)
                 + QString(MathDsl::MulDotWrapSp)
                 + QStringLiteral("(pi)")
                 + QString(MathDsl::QuantSp)
                 + QStringLiteral("[]"));

    editor.setText(QStringLiteral("2 [K] in "));
    editor.setCursorPosition(editor.text().size());
    QApplication::sendEvent(&editor, &bracketByText);
    QCOMPARE(editor.document()->toRawText(),
             QStringLiteral("2 [K] in")
                 + QString(MathDsl::QuantSp)
                 + QStringLiteral("[]"));
}

void TestEditorUi::inserts_closing_square_bracket_only_with_spaces_to_right_data()
{
    QTest::addColumn<QString>("initialText");
    QTest::addColumn<int>("cursorPosition");
    QTest::addColumn<QString>("expectedText");
    QTest::addColumn<int>("expectedCursorPosition");

    const QString unitSpace = QString(MathDsl::QuantSp);
    QTest::newRow("empty") << QString() << 0 << QStringLiteral("[]") << 1;
    QTest::newRow("end") << QStringLiteral("2") << 1
        << QStringLiteral("2") + unitSpace + QStringLiteral("[]") << 3;
    QTest::newRow("before-trailing-spaces") << QStringLiteral("2   ") << 1
        << QStringLiteral("2") + unitSpace + QStringLiteral("[]   ") << 3;
    QTest::newRow("after-trailing-spaces") << QStringLiteral("2   ") << 4
        << QStringLiteral("2") + unitSpace + QStringLiteral("[]") << 3;
    QTest::newRow("before-unicode-space") << QStringLiteral("2") + unitSpace << 1
        << QStringLiteral("2") + unitSpace + QStringLiteral("[]") + unitSpace << 3;
    QTest::newRow("before-operator") << QStringLiteral("2+3") << 1
        << QStringLiteral("2") + unitSpace + QStringLiteral("[+3") << 3;
    QTest::newRow("before-unit-and-existing-close") << QStringLiteral("2m]") << 1
        << QStringLiteral("2") + unitSpace + QStringLiteral("[m]") << 3;
    QTest::newRow("before-existing-close") << QStringLiteral("2]") << 1
        << QStringLiteral("2") + unitSpace + QStringLiteral("[]") << 3;
    QTest::newRow("inside-spaces-before-content") << QStringLiteral("2  +3") << 2
        << QStringLiteral("2") + unitSpace + QStringLiteral("[ +3") << 3;
    QTest::newRow("start-before-content") << QStringLiteral("2") << 0
        << QStringLiteral("[2") << 1;
    QTest::newRow("after-closed-group") << QStringLiteral("(2)+3") << 3
        << QStringLiteral("(2)") + unitSpace + QStringLiteral("[+3") << 5;
}

void TestEditorUi::inserts_closing_square_bracket_only_with_spaces_to_right()
{
    QFETCH(QString, initialText);
    QFETCH(int, cursorPosition);
    QFETCH(QString, expectedText);
    QFETCH(int, expectedCursorPosition);

    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    for (const int key : {Qt::Key_unknown, Qt::Key_BracketLeft}) {
        editor.setText(initialText);
        editor.setCursorPosition(cursorPosition);
        QKeyEvent openBracket(QEvent::KeyPress, key, Qt::NoModifier,
                              key == Qt::Key_unknown ? QStringLiteral("[") : QString());
        QApplication::sendEvent(&editor, &openBracket);
        QCOMPARE(editor.document()->toRawText(), expectedText);
        QCOMPARE(editor.textCursor().position(), expectedCursorPosition);
    }
}

void TestEditorUi::ignores_space_on_empty_or_all_space_editor()
{
    // State: empty editor and whitespace-only editor.
    // Action: type SPACE.
    // Expected: first leading space inserts; repeated space after space is ignored.
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QString());
    editor.setCursorPosition(0);
    QTest::keyClick(&editor, Qt::Key_Space, Qt::NoModifier);
    QCOMPARE(editor.text(), QStringLiteral(" "));

    const QString allSpaces =
        QStringLiteral(" ")
        + QString(MathDsl::QuantSp)
        + QStringLiteral(" ");
    editor.setText(allSpaces);
    editor.setCursorPosition(editor.text().size());
    const QString beforeSpaceOnAllSpaces = editor.document()->toRawText();
    QTest::keyClick(&editor, Qt::Key_Space, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), beforeSpaceOnAllSpaces);
}

void TestEditorUi::auto_inserts_space_before_question_comment_only_with_non_space_content()
{
    // State: content with non-space, empty, and whitespace-only.
    // Action: type '?'.
    // Expected: " ? " only when non-space already exists.
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("2+2"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClicks(&editor, QStringLiteral("?"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("2+2 ? "));

    editor.setText(QString());
    editor.setCursorPosition(0);
    QTest::keyClicks(&editor, QStringLiteral("?"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("? "));

    editor.setText(QStringLiteral("   "));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClicks(&editor, QStringLiteral("?"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("   ? "));

    editor.setText(QStringLiteral("2+2"));
    editor.setCursorPosition(editor.text().size());
    QKeyEvent questionByText(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QStringLiteral("?"));
    QApplication::sendEvent(&editor, &questionByText);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("2+2 ? "));
}

void TestEditorUi::typing_hex_separators_preserves_number_data()
{
    QTest::addColumn<QString>("expression");
    QTest::addColumn<QString>("expected");
    QTest::newRow("spaces") << QStringLiteral("0xffef ffff ffff ffff")
        << QStringLiteral("18442240474082181119");
    QTest::newRow("commas") << QStringLiteral("0xffef,ffff,ffff,ffff")
        << QStringLiteral("18442240474082181119");
    QTest::newRow("uppercase") << QStringLiteral("0XFFEF,FFFF,FFFF,FFFF")
        << QStringLiteral("18442240474082181119");
    QTest::newRow("fraction") << QStringLiteral("0xf.a") << QStringLiteral("15.625");
    QTest::newRow("alternate-prefix") << QStringLiteral("#F.A") << QStringLiteral("15.625");
    QTest::newRow("after-operator") << QStringLiteral("1 + 0xf.a") << QStringLiteral("16.625");
}

void TestEditorUi::typing_hex_separators_preserves_number()
{
    QFETCH(QString, expression);
    QFETCH(QString, expected);
    Settings* settings = Settings::instance();
    settings->automaticInputAdjustments = true;
    Session session;
    for (const auto style : {Settings::NumberFormatThreeDigitSpaceDot,
                             Settings::NumberFormatThreeDigitCommaDot,
                             Settings::NumberFormatThreeDigitDotComma}) {
        settings->numberFormatStyle = style;
        settings->applyNumberFormatStyle();
        Editor editor;
        editor.setSession(&session);
        editor.setAutoCompletionEnabled(false);
        editor.setAutoCalcEnabled(false);
        QTest::keyClicks(&editor, expression);
        QCOMPARE(editor.text(), expression);
        session.evaluator()->setExpression(editor.text());
        const Quantity result = session.evaluator()->evalNoAssign();
        QVERIFY2(session.evaluator()->error().isEmpty(), qPrintable(session.evaluator()->error()));
        QCOMPARE(DMath::format(result, Quantity::Format::Fixed()), expected);
    }
}

void TestEditorUi::number_separators_keep_value_when_typed_or_pasted()
{
    class PasteEditor : public Editor {
    public:
        using Editor::insertFromMimeData;
    };
    Settings* settings = Settings::instance();
    settings->automaticInputAdjustments = true;
    Session session;
    for (const auto style : {Settings::NumberFormatNoGroupingDot,
                             Settings::NumberFormatNoGroupingComma,
                             Settings::NumberFormatSIDot,
                             Settings::NumberFormatSIComma,
                             Settings::NumberFormatThreeDigitSpaceDot,
                             Settings::NumberFormatThreeDigitSpaceComma,
                             Settings::NumberFormatThreeDigitUnderscoreDot,
                             Settings::NumberFormatThreeDigitUnderscoreDotFraction,
                             Settings::NumberFormatThreeDigitUnderscoreComma,
                             Settings::NumberFormatThreeDigitUnderscoreCommaFraction,
                             Settings::NumberFormatThreeDigitCommaDot,
                             Settings::NumberFormatThreeDigitCommaDotFraction,
                             Settings::NumberFormatIndianCommaDot,
                             Settings::NumberFormatThreeDigitDotComma,
                             Settings::NumberFormatThreeDigitDotCommaFraction}) {
        settings->numberFormatStyle = style;
        settings->applyNumberFormatStyle();
        const QString grouping = settings->digitGroupingSeparator();
        const bool commaGroups = settings->decimalSeparator() == '.';
        const bool dotGroups = settings->decimalSeparator() == ',';
        const struct {
            const char* expression;
            const char* expected;
        } cases[] = {
            {"1,000*2", commaGroups ? "2000" : "2"},
            {"1,234", commaGroups ? "1234" : "1.234"},
            {"1.234", dotGroups ? "1234" : "1.234"},
            {"1,234e2", commaGroups ? "123400" : "123.4"},
            {"1.234e2", dotGroups ? "123400" : "123.4"},
            {"1,000.0*2", style == Settings::NumberFormatThreeDigitDotCommaFraction ? "2" : "2000"},
            {"1.000,23", style == Settings::NumberFormatThreeDigitCommaDotFraction ? "1.00023" : "1000.23"},
            {"1,000.23", style == Settings::NumberFormatThreeDigitDotCommaFraction ? "1.00023" : "1000.23"},
            {"1.234.567,12345", "1234567.12345"},
            {"1,234,567.12345", "1234567.12345"},
            {"1,23", "1.23"},
            {"1.23", "1.23"}
        };
        for (const auto& item : cases) {
            for (bool paste : {false, true}) {
                PasteEditor editor;
                editor.setSession(&session);
                editor.setAutoCompletionEnabled(false);
                editor.setAutoCalcEnabled(false);
                if (paste) {
                    QMimeData mime;
                    mime.setText(QString::fromLatin1(item.expression));
                    editor.insertFromMimeData(&mime);
                } else {
                    QTest::keyClicks(&editor, item.expression);
                }
                session.evaluator()->setExpression(editor.text());
                const Quantity result = session.evaluator()->evalNoAssign();
                QVERIFY2(session.evaluator()->error().isEmpty(), qPrintable(session.evaluator()->error()));
                QCOMPARE(DMath::format(result, Quantity::Format::Fixed()),
                         QString::fromLatin1(item.expected));
                const QString source = QString::fromLatin1(item.expression);
                const QChar decimal(settings->decimalSeparator());
                if (!paste && (commaGroups || dotGroups)
                    && source.contains(grouping) && !source.contains(decimal)
                    && QString::fromLatin1(item.expected) != QStringLiteral("1.23")) {
                    editor.rehighlight();
                    for (const auto& range : editor.document()->firstBlock().layout()->formats()) {
                        QVERIFY2(!range.format.hasProperty(SyntaxHighlighter::InputRadixReplacement),
                                 "A grouping mark must keep its appearance, including before an exponent");
                    }
                }
            }
        }
    }
}

void TestEditorUi::input_preserves_typed_and_pasted_number_separators()
{
    class PasteEditor : public Editor {
    public:
        using Editor::insertFromMimeData;
    };
    Settings* settings = Settings::instance();
    for (const auto style : {Settings::NumberFormatNoGroupingDot,
                             Settings::NumberFormatNoGroupingComma,
                             Settings::NumberFormatThreeDigitCommaDot,
                             Settings::NumberFormatThreeDigitDotComma}) {
        settings->numberFormatStyle = style;
        settings->applyNumberFormatStyle();
        for (bool adjustments : {false, true}) {
            settings->automaticInputAdjustments = adjustments;
            for (bool grouping : {false, true}) {
                settings->inputDigitGrouping = grouping;
                for (const QString& number : {QStringLiteral("1,000"), QStringLiteral("1.000"),
                                              QStringLiteral("1,234e2"), QStringLiteral("1.234e2"),
                                              QStringLiteral("1.000,23"), QStringLiteral("1,000.23")}) {
                    PasteEditor editor;
                    editor.setAutoCompletionEnabled(false);
                    editor.setAutoCalcEnabled(false);
                    QString prefix;
                    for (QChar ch : number) {
                        QTest::keyClicks(&editor, QString(ch));
                        prefix += ch;
                        editor.rehighlight();
                        QCOMPARE(editor.text(), prefix);
                        for (const auto& range : editor.document()->firstBlock().layout()->formats())
                            QVERIFY(!range.format.hasProperty(SyntaxHighlighter::InputRadixReplacement));
                    }
                    editor.clear();
                    QMimeData mime;
                    mime.setText(number);
                    editor.insertFromMimeData(&mime);
                    QCOMPARE(editor.text(), number);
                    editor.rehighlight();
                    for (const auto& range : editor.document()->firstBlock().layout()->formats())
                        QVERIFY(!range.format.hasProperty(SyntaxHighlighter::InputRadixReplacement));
                }
                if (adjustments) {
                    for (QChar separator : {QLatin1Char('.'), QLatin1Char(',')}) {
                        PasteEditor editor;
                        editor.setAutoCompletionEnabled(false);
                        editor.setAutoCalcEnabled(false);
                        QTest::keyClicks(&editor, QString(separator));
                        QCOMPARE(editor.text(), QStringLiteral("0") + separator);
                        editor.setText(QStringLiteral("1"));
                        editor.setCursorPosition(1);
                        QKeyEvent event(QEvent::KeyPress, Qt::Key_Period, Qt::KeypadModifier,
                                        QString(separator));
                        QApplication::sendEvent(&editor, &event);
                        QCOMPARE(editor.text(), QStringLiteral("1") + separator);
                    }
                }
            }
        }
    }
}

void TestEditorUi::validated_number_format_examples_data()
{
    QTest::addColumn<int>("style");
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expectedValue");
    QTest::addColumn<QString>("expectedOutput");
    QTest::addColumn<bool>("paste");
    for (const auto& format : NumberFormatCases::formats) {
        for (size_t i = 0; i <= NumberFormatCases::inputs.size(); ++i) {
            const bool example = i == NumberFormatCases::inputs.size();
            const QString input = QString::fromUtf8(example ? format.example : NumberFormatCases::inputs.at(i));
            const QString value = QString::fromLatin1(example ? "1234567.12345" : NumberFormatCases::numericValue(format, i));
            const QString output = QString::fromUtf8(example ? format.example : format.outputs.at(i));
            for (bool paste : {false, true}) {
                const QByteArray label = QStringLiteral("%1/%2/%3")
                    .arg(QString::fromUtf8(format.example), input, paste ? QStringLiteral("paste") : QStringLiteral("type"))
                    .toUtf8();
                QTest::newRow(label.constData()) << int(format.style) << input << value << output << paste;
            }
        }
    }
}

void TestEditorUi::validated_number_format_examples()
{
    QFETCH(int, style);
    QFETCH(QString, input);
    QFETCH(QString, expectedValue);
    QFETCH(QString, expectedOutput);
    QFETCH(bool, paste);
    class PasteEditor : public Editor {
    public:
        using Editor::insertFromMimeData;
    };
    Settings* settings = Settings::instance();
    settings->numberFormatStyle = static_cast<Settings::NumberFormatStyle>(style);
    settings->applyNumberFormatStyle();
    settings->automaticInputAdjustments = true;
    Session session;
    PasteEditor editor;
    editor.setSession(&session);
    editor.setAutoCompletionEnabled(false);
    editor.setAutoCalcEnabled(false);
    if (paste) {
        QMimeData mime;
        mime.setText(input);
        editor.insertFromMimeData(&mime);
    } else {
        QString keys = input;
        keys.replace(MathDsl::MulCrossOp, QLatin1Char('*'));
        QTest::keyClicks(&editor, keys);
    }
    if (!input.contains(MathDsl::MulCrossOp))
        QCOMPARE(editor.text(), input);
    session.evaluator()->setExpression(editor.text());
    const Quantity value = session.evaluator()->evalNoAssign();
    QVERIFY2(session.evaluator()->error().isEmpty(), qPrintable(session.evaluator()->error()));
    QCOMPARE(DMath::format(value, Quantity::Format::Fixed()), expectedValue);
    QCOMPARE(DisplayFormatUtils::applyDigitGroupingForDisplay(
                 NumberFormatter::format(value, 'f', -1, false, ComplexForm::Rectangular)), expectedOutput);
    editor.rehighlight();
    for (const auto& range : editor.document()->firstBlock().layout()->formats())
        QVERIFY(!range.format.hasProperty(SyntaxHighlighter::InputRadixReplacement));
}

void TestEditorUi::ambiguous_fractional_grouping_prefers_ungrouped_data()
{
    QTest::addColumn<int>("style");
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expectedValue");
    QTest::addColumn<QString>("expectedOutput");
    QTest::addColumn<bool>("paste");
    for (const auto& item : FractionalGroupingCases::cases) {
        for (bool paste : {false, true}) {
            const QByteArray label = QStringLiteral("%1/%2/%3")
                .arg(int(item.style)).arg(QString::fromUtf8(item.input),
                    paste ? QStringLiteral("paste") : QStringLiteral("type")).toUtf8();
            QTest::newRow(label.constData()) << int(item.style) << QString::fromUtf8(item.input)
                << QString::fromLatin1(item.value) << QString::fromUtf8(item.output) << paste;
        }
    }
}

void TestEditorUi::ambiguous_fractional_grouping_prefers_ungrouped()
{
    validated_number_format_examples();
}

void TestEditorUi::number_grouping_edge_cases_data()
{
    QTest::addColumn<int>("style");
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expectedValue");
    QTest::addColumn<bool>("shouldFail");
    QTest::addColumn<bool>("paste");
    for (auto style : {Settings::NumberFormatNoGroupingDot, Settings::NumberFormatNoGroupingComma}) {
        const auto& cases = style == Settings::NumberFormatNoGroupingDot
            ? NumberGroupingEdgeCases::dotCases : NumberGroupingEdgeCases::commaCases;
        for (const auto& item : cases) {
            for (bool paste : {false, true}) {
                const QByteArray label = QStringLiteral("%1/%2/%3").arg(int(style))
                    .arg(QString::fromLatin1(item.input), paste ? QStringLiteral("paste") : QStringLiteral("type")).toUtf8();
                QTest::newRow(label.constData()) << int(style) << QString::fromLatin1(item.input)
                    << QString::fromLatin1(item.value ? item.value : "") << !item.value << paste;
            }
        }
    }
}

void TestEditorUi::number_grouping_edge_cases()
{
    QFETCH(int, style);
    QFETCH(QString, input);
    QFETCH(QString, expectedValue);
    QFETCH(bool, shouldFail);
    QFETCH(bool, paste);
    class PasteEditor : public Editor {
    public:
        using Editor::insertFromMimeData;
    };
    Settings* settings = Settings::instance();
    settings->numberFormatStyle = static_cast<Settings::NumberFormatStyle>(style);
    settings->applyNumberFormatStyle();
    settings->automaticInputAdjustments = true;
    Session session;
    PasteEditor editor;
    editor.setSession(&session);
    editor.setAutoCompletionEnabled(false);
    editor.setAutoCalcEnabled(false);
    if (paste) {
        QMimeData mime;
        mime.setText(input);
        editor.insertFromMimeData(&mime);
    } else {
        QTest::keyClicks(&editor, input);
    }
    QCOMPARE(editor.text(), input);
    session.evaluator()->setExpression(editor.text());
    const Quantity value = session.evaluator()->evalNoAssign();
    QCOMPARE(!session.evaluator()->error().isEmpty(), shouldFail);
    if (!shouldFail)
        QCOMPARE(DMath::format(value, Quantity::Format::Fixed()), expectedValue);
}

void TestEditorUi::number_input_interpreted_expressions_all_formats_data()
{
    QTest::addColumn<int>("style");
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("interpreted");
    QTest::addColumn<QString>("displayed");
    QTest::addColumn<bool>("paste");
    for (size_t index = 0; index < NumberFormatCases::formats.size(); ++index) {
        const auto& format = NumberFormatCases::formats.at(index);
        for (const auto& item : InterpretedNumberCases::forFormat(index)) {
            for (bool paste : {false, true}) {
                const QByteArray label = QStringLiteral("%1/%2/%3").arg(int(format.style))
                    .arg(item.input, paste ? QStringLiteral("paste") : QStringLiteral("type")).toUtf8();
                QTest::newRow(label.constData()) << int(format.style) << item.input
                    << item.interpreted << item.displayed << paste;
            }
        }
    }
}

void TestEditorUi::number_input_interpreted_expressions_all_formats()
{
    QFETCH(int, style);
    QFETCH(QString, input);
    QFETCH(QString, interpreted);
    QFETCH(QString, displayed);
    QFETCH(bool, paste);
    class PasteEditor : public Editor {
    public:
        using Editor::insertFromMimeData;
    };
    Settings* settings = Settings::instance();
    settings->numberFormatStyle = static_cast<Settings::NumberFormatStyle>(style);
    settings->applyNumberFormatStyle();
    settings->automaticInputAdjustments = true;
    Session session;
    PasteEditor editor;
    editor.setSession(&session);
    editor.setAutoCompletionEnabled(false);
    editor.setAutoCalcEnabled(false);
    if (paste) {
        QMimeData mime;
        mime.setText(input);
        editor.insertFromMimeData(&mime);
    } else {
        QTest::keyClicks(&editor, input);
    }
    QString expectedInput = input;
    expectedInput.replace(QLatin1String("*"), paste ? QString::fromUtf8("×") : QString::fromUtf8(" × "));
    QCOMPARE(editor.text(), expectedInput);
    Evaluator* evaluator = session.evaluator();
    evaluator->setExpression(editor.text());
    Quantity value = evaluator->evalNoAssign();
    QVERIFY2(evaluator->error().isEmpty(), qPrintable(evaluator->error()));
    QCOMPARE(evaluator->interpretedExpression(), interpreted);
    ResultDisplay display;
    display.setSession(&session);
    display.append(editor.text(), value, evaluator->interpretedExpression());
    QCOMPARE(display.document()->firstBlock().text(), displayed);
}

void TestEditorUi::malformed_number_grouping_all_formats_data()
{
    QTest::addColumn<int>("style");
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expectedValue");
    QTest::addColumn<bool>("shouldFail");
    QTest::addColumn<bool>("paste");
    for (const auto& format : NumberFormatCases::formats) {
        const auto addCases = [&](const auto& cases) {
            for (const char* input : cases) {
                for (bool paste : {false, true}) {
                    const QByteArray label = QStringLiteral("%1/%2/%3").arg(int(format.style))
                        .arg(QString::fromLatin1(input),
                             paste ? QStringLiteral("paste") : QStringLiteral("type")).toUtf8();
                    QTest::newRow(label.constData()) << int(format.style) << QString::fromLatin1(input)
                        << QString() << true << paste;
                }
            }
        };
        addCases(MalformedNumberGroupingCases::decimal);
        addCases(MalformedNumberGroupingCases::nondecimal);
    }
}

void TestEditorUi::malformed_number_grouping_all_formats()
{
    number_grouping_edge_cases();
}

void TestEditorUi::decimal_input_without_display_grouping_data()
{
    QTest::addColumn<int>("style");
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expectedValue");
    QTest::addColumn<QString>("expectedOutput");
    QTest::addColumn<bool>("paste");
    for (const auto& item : DecimalInputCases::cases) {
        for (bool paste : {false, true}) {
            const QByteArray label = QStringLiteral("%1/%2").arg(QString::fromLatin1(item.input),
                paste ? QStringLiteral("paste") : QStringLiteral("type")).toUtf8();
            const QString value = QString::fromLatin1(item.value);
            QTest::newRow(label.constData()) << int(Settings::NumberFormatNoGroupingDot)
                << QString::fromLatin1(item.input) << value << value << paste;
        }
    }
}

void TestEditorUi::decimal_input_without_display_grouping()
{
    validated_number_format_examples();
}

void TestEditorUi::decimal_input_all_number_formats_data()
{
    QTest::addColumn<int>("style");
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expectedValue");
    QTest::addColumn<QString>("expectedOutput");
    QTest::addColumn<bool>("paste");
    for (const auto& format : DecimalInputCases::formats) {
        for (size_t i = 0; i < DecimalInputCases::cases.size(); ++i) {
            const auto& item = DecimalInputCases::cases.at(i);
            for (bool paste : {false, true}) {
                const QByteArray label = QStringLiteral("%1/%2/%3")
                    .arg(QString::fromUtf8(format.example), QString::fromLatin1(item.input),
                         paste ? QStringLiteral("paste") : QStringLiteral("type")).toUtf8();
                QTest::newRow(label.constData()) << int(format.style) << QString::fromLatin1(item.input)
                    << QString::fromLatin1(item.value) << QString::fromUtf8(format.outputs.at(i)) << paste;
            }
        }
    }
}

void TestEditorUi::decimal_input_all_number_formats()
{
    validated_number_format_examples();
}

void TestEditorUi::spaced_hex_decode_live_preview()
{
    class PasteEditor : public Editor {
    public:
        using Editor::insertFromMimeData;
    };

    Settings* settings = Settings::instance();
    settings->automaticInputAdjustments = true;
    Session session;
    for (const auto style : {Settings::NumberFormatThreeDigitSpaceDot,
                             Settings::NumberFormatThreeDigitCommaDot,
                             Settings::NumberFormatThreeDigitDotComma}) {
        settings->numberFormatStyle = style;
        settings->applyNumberFormatStyle();
        for (const bool paste : {false, true}) {
            PasteEditor editor;
            editor.setSession(&session);
            editor.setAutoCalcEnabled(true);
            editor.setAutoCompletionEnabled(true);
            editor.setText(QStringLiteral("ieee754_double_decode()"));
            editor.setCursorPosition(editor.text().size() - 1);
            const QString hex = QStringLiteral("0xffef ffff ffff ffff");
            if (paste) {
                QMimeData mime;
                mime.setText(hex);
                editor.insertFromMimeData(&mime);
            } else {
                QTest::keyClicks(&editor, hex);
            }
            QCOMPARE(editor.text(), QStringLiteral("ieee754_double_decode(0xffef ffff ffff ffff)"));
            bool hasPreview = false;
            Quantity preview;
            connect(&editor, &Editor::autoCalcQuantityAvailable, &editor, [&](const Quantity& value) {
                hasPreview = true;
                preview = value;
            });
            editor.refreshAutoCalc();
            QVERIFY2(hasPreview, qPrintable(session.evaluator()->error()));
            QCOMPARE(DMath::format(preview, Quantity::Format::Scientific()),
                     QStringLiteral("-1.79769313486231570815e308"));
        }
    }
}

void TestEditorUi::manual_grouping_keeps_punctuation_data()
{
    QTest::addColumn<QString>("input");
    for (const auto& input : {QStringLiteral("0xffef,ffff,ffff,ffff"),
                              QStringLiteral("0xffef.ffff.ffff.ffff"),
                              QStringLiteral("1,234,567"),
                              QStringLiteral("1.234.567"),
                              QStringLiteral("1,234,567.89"),
                              QStringLiteral("1.234.567,89"),
                              QStringLiteral("1,234.56"),
                              QStringLiteral("1.234,56")}) {
        QTest::newRow(qPrintable(input)) << input;
    }
}

void TestEditorUi::manual_grouping_keeps_punctuation()
{
    QFETCH(QString, input);
    Settings* settings = Settings::instance();
    settings->inputDigitGrouping = true;
    for (const auto style : {Settings::NumberFormatNoGroupingDot,
                             Settings::NumberFormatNoGroupingComma}) {
        settings->numberFormatStyle = style;
        settings->applyNumberFormatStyle();
        Editor editor;
        editor.setAutoCalcEnabled(false);
        editor.setAutoCompletionEnabled(false);
        editor.setText(input);
        for (const bool highlighting : {true, false}) {
            settings->syntaxHighlighting = highlighting;
            editor.rehighlight();
            for (const auto& range : editor.document()->firstBlock().layout()->formats()) {
                QVERIFY2(!range.format.hasProperty(SyntaxHighlighter::InputRadixReplacement),
                         qPrintable(input));
            }
            QCOMPARE(editor.text(), input);
        }
    }
}

void TestEditorUi::inserts_middle_dot_on_space_after_identifier_or_closed_group()
{
    Settings* settings = Settings::instance();
    const bool autoCompletionBackup = settings->autoCompletion;
    const bool builtInFnBackup = settings->autoCompletionBuiltInFunctions;
    const bool builtInVarBackup = settings->autoCompletionBuiltInVariables;
    const bool userFnBackup = settings->autoCompletionUserFunctions;
    const bool userVarBackup = settings->autoCompletionUserVariables;
    settings->autoCompletion = true;
    settings->autoCompletionBuiltInFunctions = false;
    settings->autoCompletionBuiltInVariables = true;
    settings->autoCompletionUserFunctions = false;
    settings->autoCompletionUserVariables = false;
    struct AutoCompletionRestoreGuard {
        Settings* settings;
        bool autoCompletion;
        bool builtInFunctions;
        bool builtInVariables;
        bool userFunctions;
        bool userVariables;
        ~AutoCompletionRestoreGuard()
        {
            settings->autoCompletion = autoCompletion;
            settings->autoCompletionBuiltInFunctions = builtInFunctions;
            settings->autoCompletionBuiltInVariables = builtInVariables;
            settings->autoCompletionUserFunctions = userFunctions;
            settings->autoCompletionUserVariables = userVariables;
        }
    } autoCompletionRestoreGuard {
        settings,
        autoCompletionBackup,
        builtInFnBackup,
        builtInVarBackup,
        userFnBackup,
        userVarBackup};

    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("p"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Space, Qt::NoModifier);
    const QString completedIdentifier = editor.document()->toRawText();
    const QString piSymbol = QString(UnicodeChars::Pi);
    QVERIFY(completedIdentifier == QStringLiteral("p ")
        || completedIdentifier == QStringLiteral("pi ")
        || completedIdentifier == piSymbol + QStringLiteral(" "));

    editor.setText(QStringLiteral("cos(3)"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Space, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("cos(3) "));
}

void TestEditorUi::inserts_parenthesis_pair_and_places_cursor_inside()
{
    // State: multiple states (empty, numbers, before operator, function name).
    // Action: type '(' (text path and keycode path).
    // Expected: auto-insert "()" only when right side is empty/spaces-only.
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();
    const QString groupStart = QString(MathDsl::GroupStart);
    const QString groupEnd = QString(MathDsl::GroupEnd);
    const QString groupPair = groupStart + groupEnd;

    const auto verifyParenInsertionAt = [&editor](const QString& initialText, int cursorPos) {
        editor.setText(initialText);
        editor.setCursorPosition(cursorPos);

        QKeyEvent openParenByText(
            QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QString(MathDsl::GroupStart));
        QApplication::sendEvent(&editor, &openParenByText);

        const QString after = editor.document()->toRawText();
        const int cursor = editor.textCursor().position();

        QVERIFY(cursor > 0);
        QVERIFY(cursor < after.size());
        QCOMPARE(after.at(cursor - 1), QLatin1Char('('));
        QCOMPARE(after.at(cursor), QLatin1Char(')'));
    };

    verifyParenInsertionAt(QString(), 0);
    verifyParenInsertionAt(QStringLiteral("2"), 1);
    verifyParenInsertionAt(QStringLiteral("1   "), 1);  // only spaces to the right

    editor.setText(QStringLiteral("1+2"));
    editor.setCursorPosition(1);
    QKeyEvent openParenByText(
        QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QString(MathDsl::GroupStart));
    QApplication::sendEvent(&editor, &openParenByText);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("1(+2"));
    QCOMPARE(editor.textCursor().position(), 2);

    editor.setText(QStringLiteral("1)"));
    editor.setCursorPosition(1);
    QApplication::sendEvent(&editor, &openParenByText);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("1()"));
    QCOMPARE(editor.textCursor().position(), 2);

    editor.setText(QStringLiteral("cos"));
    editor.setCursorPosition(editor.text().size());
    QKeyEvent openParenByTextForFunction(
        QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QString(MathDsl::GroupStart));
    QApplication::sendEvent(&editor, &openParenByTextForFunction);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("cos") + groupPair);
    QCOMPARE(editor.textCursor().position(), 4);

    editor.setText(QString::fromUtf8("cos³"));
    editor.setCursorPosition(editor.text().size());
    QApplication::sendEvent(&editor, &openParenByTextForFunction);
    QCOMPARE(editor.document()->toRawText(), QString::fromUtf8("cos³") + groupPair);
    QCOMPARE(editor.textCursor().position(), 5);

    editor.setText(QString::fromUtf8("2 pi⁻²³ · cos"));
    editor.setCursorPosition(editor.text().size());
    QApplication::sendEvent(&editor, &openParenByTextForFunction);
    QCOMPARE(editor.document()->toRawText(), QString::fromUtf8("2 pi⁻²³ · cos") + groupPair);
    QCOMPARE(editor.textCursor().position(),
             QString::fromUtf8("2 pi⁻²³ · cos(").size());

    Session session;
    Evaluator* evaluator = session.evaluator();
    editor.setSession(&session);
    evaluator->unsetAllUserFunctions();
    evaluator->setUserFunction(UserFunction(
        QStringLiteral("user_fun"),
        QStringList() << QStringLiteral("x"),
        QStringLiteral("x")));
    editor.setText(QStringLiteral("user_fun"));
    editor.setCursorPosition(editor.text().size());
    QApplication::sendEvent(&editor, &openParenByTextForFunction);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("user_fun") + groupPair);
    QCOMPARE(editor.textCursor().position(), QStringLiteral("user_fun(").size());
    evaluator->unsetAllUserFunctions();

    editor.setText(QStringLiteral("3"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_ParenLeft, Qt::NoModifier);
    const QString afterKeyCodePath = editor.document()->toRawText();
    const int cursorAfterKeyCodePath = editor.textCursor().position();
    QVERIFY(cursorAfterKeyCodePath > 0);
    QVERIFY(cursorAfterKeyCodePath < afterKeyCodePath.size());
    QCOMPARE(afterKeyCodePath.at(cursorAfterKeyCodePath - 1), QLatin1Char('('));
    QCOMPARE(afterKeyCodePath.at(cursorAfterKeyCodePath), QLatin1Char(')'));
}

void TestEditorUi::does_not_insert_implicit_mul_sequence_before_open_paren_after_number_or_symbol()
{
    // State: "2" and "π".
    // Action: type '(' via text-based key event.
    // Expected: insert "()" without implicit multiplication prefix.
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    auto typeGroupStartByText = [&editor]() {
        QKeyEvent openParenByText(
            QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QStringLiteral("("));
        QApplication::sendEvent(&editor, &openParenByText);
    };

    editor.setText(QStringLiteral("2"));
    editor.setCursorPosition(editor.text().size());
    typeGroupStartByText();
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("2()"));

    editor.setText(QString::fromUtf8("2³"));
    editor.setCursorPosition(editor.text().size());
    typeGroupStartByText();
    QCOMPARE(editor.document()->toRawText(), QString::fromUtf8("2³()"));

    editor.setText(QString::fromUtf8("π"));
    editor.setCursorPosition(editor.text().size());
    typeGroupStartByText();
    QCOMPARE(editor.document()->toRawText(), QString::fromUtf8("π()"));

    editor.setText(QString::fromUtf8("pi³"));
    editor.setCursorPosition(editor.text().size());
    typeGroupStartByText();
    QCOMPARE(editor.document()->toRawText(), QString::fromUtf8("pi³()"));
}

void TestEditorUi::does_not_insert_implicit_mul_for_zero_radix_prefix_letters()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("0"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClicks(&editor, QStringLiteral("b"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("0b"));

    editor.setText(QStringLiteral("0"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClicks(&editor, QStringLiteral("o"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("0o"));

    editor.setText(QStringLiteral("0"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClicks(&editor, QStringLiteral("x"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("0x"));
}

void TestEditorUi::allows_unit_conversion_tail_after_spaced_subtraction_operator()
{
    // State: "1", then subtraction operator formatting.
    // Action: type GreaterThanSign via text-based key event.
    // Expected: convert to spaced "→ []", cursor inside brackets, parser sees UnitConversion.
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("1"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Minus, Qt::NoModifier);

    const QString afterMinus = editor.document()->toRawText();
    QVERIFY(afterMinus.contains(MathDsl::SubOp));

    QKeyEvent greaterByText(
        QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QString(UnicodeChars::GreaterThanSign));
    QApplication::sendEvent(&editor, &greaterByText);

    const QString afterGreater = editor.document()->toRawText();
    const QString arrowSequence =
        QString(MathDsl::SubWrapSp)
        + QString(MathDsl::TransOp)
        + QString(MathDsl::SubWrapSp)
        + QStringLiteral("[]");
    QVERIFY(afterGreater.contains(arrowSequence));
    QVERIFY(!afterGreater.contains(UnicodeChars::GreaterThanSign));
    QCOMPARE(editor.textCursor().position(), afterGreater.size() - 1);

    const Tokens tokens = Evaluator::instance()->scan(afterGreater);
    bool hasUnitConversion = false;
    for (const Token& token : tokens) {
        if (token.isOperator() && token.asOperator() == Token::UnitConversion) {
            hasUnitConversion = true;
            break;
        }
    }
    QVERIFY(hasUnitConversion);
}

void TestEditorUi::converts_double_minus_sequence_to_unit_conversion_with_placeholder()
{
    // State: "1".
    // Action: type '--'.
    // Expected: convert to spaced "→ []", cursor inside brackets, parser sees UnitConversion.
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("1"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Minus, Qt::NoModifier);
    QTest::keyClick(&editor, Qt::Key_Minus, Qt::NoModifier);

    const QString afterDoubleMinus = editor.document()->toRawText();
    const QString arrowSequence =
        QString(MathDsl::SubWrapSp)
        + QString(MathDsl::TransOp)
        + QString(MathDsl::SubWrapSp)
        + QStringLiteral("[]");
    QVERIFY(afterDoubleMinus.contains(arrowSequence));
    QCOMPARE(editor.textCursor().position(), afterDoubleMinus.size() - 1);

    const Tokens tokens = Evaluator::instance()->scan(afterDoubleMinus);
    bool hasUnitConversion = false;
    for (const Token& token : tokens) {
        if (token.isOperator() && token.asOperator() == Token::UnitConversion) {
            hasUnitConversion = true;
            break;
        }
    }
    QVERIFY(hasUnitConversion);
}

void TestEditorUi::converts_double_minus_sequence_to_unit_conversion_after_masculine_ordinal_degree()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    const QString conversion =
        QString(MathDsl::SubWrapSp)
        + QString(MathDsl::TransOp)
        + QString(MathDsl::SubWrapSp)
        + QStringLiteral("[]");
    const QPair<QString, QString> cases[] = {
        {QString::fromUtf8("12º"), QString::fromUtf8("12°") + conversion},
        {QString::fromUtf8("23°"), QString::fromUtf8("23°") + conversion},
        {QString::fromUtf8("23′"), QString::fromUtf8("23′") + conversion},
        {QString::fromUtf8("23″"), QString::fromUtf8("23″") + conversion}
    };

    for (const auto& tc : cases) {
        editor.setText(tc.first);
        editor.setCursorPosition(editor.text().size());
        QTest::keyClick(&editor, Qt::Key_Minus, Qt::NoModifier);
        QTest::keyClick(&editor, Qt::Key_Minus, Qt::NoModifier);

        const QString actual = editor.document()->toRawText();
        QCOMPARE(actual, tc.second);
        QCOMPARE(editor.textCursor().position(), actual.size() - 1);
    }
}

void TestEditorUi::inserts_unit_conversion_with_placeholder_when_typing_arrow_symbol()
{
    // State: "1".
    // Action: type "→" directly.
    // Expected: insert " → []", with cursor inside brackets.
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("1"));
    editor.setCursorPosition(editor.text().size());

    QKeyEvent rightArrowByText(
        QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QString(MathDsl::TransOp));
    QApplication::sendEvent(&editor, &rightArrowByText);

    const QString expected =
        QStringLiteral("1")
        + QString(MathDsl::SubWrapSp)
        + QString(MathDsl::TransOp)
        + QString(MathDsl::SubWrapSp)
        + QStringLiteral("[]");
    QCOMPARE(editor.document()->toRawText(), expected);
    QCOMPARE(editor.textCursor().position(), expected.size() - 1);
}

void TestEditorUi::treats_spaced_unit_conversion_as_atomic_navigation_and_edit_token()
{
    // State: "1<space>→<space>2".
    // Action: use Left/Right, Alt+Left/Right, Backspace, Delete near token.
    // Expected: grouped " → " behaves as one atomic token.
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    const QString arrowToken =
        QString(MathDsl::SubWrapSp)
        + QString(MathDsl::TransOp)
        + QString(MathDsl::SubWrapSp);
    const QString expression = QStringLiteral("1") + arrowToken + QStringLiteral("2");

    editor.setText(expression);
    editor.setCursorPosition(4); // between arrow and trailing space
    QTest::keyClick(&editor, Qt::Key_Left, Qt::NoModifier);
    QCOMPARE(editor.textCursor().position(), 1);
    QTest::keyClick(&editor, Qt::Key_Right, Qt::NoModifier);
    QCOMPARE(editor.textCursor().position(), 4);

    editor.setText(expression);
    editor.setCursorPosition(4); // right after grouped token
    QTest::keyClick(&editor, Qt::Key_Left, Qt::AltModifier);
    QCOMPARE(editor.textCursor().position(), 1);
    editor.setCursorPosition(1); // right before grouped token
    QTest::keyClick(&editor, Qt::Key_Right, Qt::AltModifier);
    QCOMPARE(editor.textCursor().position(), 4);

    editor.setText(expression);
    editor.setCursorPosition(4); // immediately after grouped token
    QTest::keyClick(&editor, Qt::Key_Backspace, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("12"));

    editor.setText(expression);
    editor.setCursorPosition(1); // immediately before grouped token
    QTest::keyClick(&editor, Qt::Key_Delete, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("12"));
}

void TestEditorUi::treats_spaced_shift_operators_as_atomic_navigation_and_edit_tokens()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    const QString leftShift = MathDsl::buildWrappedToken(MathDsl::ShiftLeftOp);
    const QString rightShift = MathDsl::buildWrappedToken(MathDsl::ShiftRightOp);
    const QString leftExpression = QStringLiteral("1") + leftShift + QStringLiteral("2");
    const QString rightExpression = QStringLiteral("1") + rightShift + QStringLiteral("2");

    editor.setText(leftExpression);
    editor.setCursorPosition(5); // right after grouped token
    QTest::keyClick(&editor, Qt::Key_Left, Qt::NoModifier);
    QCOMPARE(editor.textCursor().position(), 1);
    QTest::keyClick(&editor, Qt::Key_Right, Qt::NoModifier);
    QCOMPARE(editor.textCursor().position(), 5);
    QTest::keyClick(&editor, Qt::Key_Backspace, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("12"));

    editor.setText(leftExpression);
    editor.setCursorPosition(1); // right before grouped token
    QTest::keyClick(&editor, Qt::Key_Delete, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("12"));

    editor.setText(rightExpression);
    editor.setCursorPosition(5); // right after grouped token
    QTest::keyClick(&editor, Qt::Key_Left, Qt::NoModifier);
    QCOMPARE(editor.textCursor().position(), 1);
    QTest::keyClick(&editor, Qt::Key_Right, Qt::NoModifier);
    QCOMPARE(editor.textCursor().position(), 5);
    QTest::keyClick(&editor, Qt::Key_Backspace, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("12"));

    editor.setText(rightExpression);
    editor.setCursorPosition(1); // right before grouped token
    QTest::keyClick(&editor, Qt::Key_Delete, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("12"));
}

void TestEditorUi::backspace_removes_typed_operator_after_unit()
{
    Editor editor;
    QTest::keyClicks(&editor, QStringLiteral("2 [g"));
    QTest::keyClick(&editor, Qt::Key_Right); // Leave the auto-inserted unit brackets.
    QTest::keyClicks(&editor, QStringLiteral("/"));
    const QString quantity = QStringLiteral("2") + MathDsl::QuantSp + QStringLiteral("[g]");
    const QString division = MathDsl::buildWrappedToken(MathDsl::DivOp, MathDsl::DivWrap);
    QCOMPARE(editor.document()->toRawText(), quantity + division);

    // Native Backspace events carry a control character; QTest::keyClick
    // does not reproduce that payload on every platform.
    QKeyEvent backspace(QEvent::KeyPress, Qt::Key_Backspace, Qt::NoModifier,
                        QString(QChar(0x08)));
    QApplication::sendEvent(&editor, &backspace);
    QCOMPARE(editor.document()->toRawText(), quantity);
    QCOMPARE(editor.textCursor().position(), quantity.size());
}

void TestEditorUi::editing_keys_bypass_typing_restrictions_data()
{
    QTest::addColumn<QString>("expression");
    QTest::addColumn<int>("position");
    QTest::addColumn<int>("key");
    QTest::addColumn<QString>("payload");
    QTest::addColumn<QString>("expected");
    QTest::addColumn<int>("expectedPosition");

    const QList<QChar> operators = {
        MathDsl::AddOp, MathDsl::SubOp, MathDsl::DivOp,
        MathDsl::MulCrossOp, MathDsl::PowOp
    };
    const QStringList prefixes = {
        QStringLiteral("2"),
        QStringLiteral("2") + MathDsl::QuantSp + QStringLiteral("[g]"),
        QStringLiteral("2") + MathDsl::QuantSp + QStringLiteral("[g")
    };
    for (int context = 0; context < prefixes.size(); ++context) {
        for (const QChar op : operators) {
            const QString token = context == 2 || op == MathDsl::PowOp
                ? QString(op) : MathDsl::buildWrappedToken(op);
            const QString expression = prefixes.at(context) + token;
            const QByteArray name = QStringLiteral("context-%1-op-%2")
                .arg(context).arg(uint(op.unicode())).toLatin1();
            QTest::newRow(QByteArray(name + "-backspace").constData())
                << expression << int(expression.size()) << int(Qt::Key_Backspace)
                << QString(QChar(0x08)) << prefixes.at(context) << int(prefixes.at(context).size());
            QTest::newRow(QByteArray(name + "-delete").constData())
                << expression + QStringLiteral("7") << int(expression.size()) << int(Qt::Key_Delete)
                << QString(QChar(0x7f)) << expression << int(expression.size());
            QTest::newRow(QByteArray(name + "-escape").constData())
                << expression << int(expression.size()) << int(Qt::Key_Escape)
                << QString(QChar(0x1b)) << expression << int(expression.size());
            QTest::newRow(QByteArray(name + "-tab").constData())
                << expression << int(expression.size()) << int(Qt::Key_Tab)
                << QString(QChar(0x09)) << expression << int(expression.size());
        }
    }
    QTest::newRow("delete-at-expression-start")
        << QStringLiteral("7") << 0 << int(Qt::Key_Delete)
        << QString(QChar(0x7f)) << QString() << 0;
    QTest::newRow("backspace-in-leading-whitespace")
        << QStringLiteral("  7") << 2 << int(Qt::Key_Backspace)
        << QString(QChar(0x08)) << QStringLiteral(" 7") << 1;
    QTest::newRow("escape-in-empty-editor")
        << QString() << 0 << int(Qt::Key_Escape)
        << QString(QChar(0x1b)) << QString() << 0;
}

void TestEditorUi::editing_keys_bypass_typing_restrictions()
{
    QFETCH(QString, expression);
    QFETCH(int, position);
    QFETCH(int, key);
    QFETCH(QString, payload);
    QFETCH(QString, expected);
    QFETCH(int, expectedPosition);

    Editor editor;
    editor.setText(expression);
    editor.setCursorPosition(position);
    QSignalSpy escapeSpy(&editor, SIGNAL(escapePressed()));
    QKeyEvent event(QEvent::KeyPress, key, Qt::NoModifier, payload);
    QApplication::sendEvent(&editor, &event);

    QCOMPARE(editor.document()->toRawText(), expected);
    QCOMPARE(editor.textCursor().position(), expectedPosition);
    QCOMPARE(escapeSpy.count(), key == Qt::Key_Escape ? 1 : 0);
}

void TestEditorUi::undo_redo_shortcuts_preserve_expression_data()
{
    QTest::addColumn<QString>("expression");
    QTest::addColumn<int>("position");
    QTest::addColumn<int>("selectionLength");
    QTest::addColumn<int>("shortcutPosition");
    QTest::addColumn<QString>("payload");

    const QStringList expressions = {
        QStringLiteral("123456"), QStringLiteral("12+34"),
        QStringLiteral("12[m/s]"), QString::fromUtf8("12°34′56″"),
        QStringLiteral("12 ? comment")
    };
    const QStringList payloads = {QString(), QString(QChar(0x1a)), QStringLiteral("z")};
    const QList<int> shortcutPositions = {3, 3, 5, 4, 7};
    for (int context = 0; context < expressions.size(); ++context) {
        for (int payload = 0; payload < payloads.size(); ++payload) {
            for (int selectionLength : {0, 2}) {
                const QByteArray name = QStringLiteral("context-%1-payload-%2-selection-%3")
                    .arg(context).arg(payload).arg(selectionLength).toLatin1();
                QTest::newRow(name.constData())
                    << expressions.at(context) << 2 << selectionLength
                    << shortcutPositions.at(context) << payloads.at(payload);
            }
        }
    }
}

void TestEditorUi::undo_redo_shortcuts_preserve_expression()
{
    QFETCH(QString, expression);
    QFETCH(int, position);
    QFETCH(int, selectionLength);
    QFETCH(int, shortcutPosition);
    QFETCH(QString, payload);

    const auto undoBindings = QKeySequence::keyBindings(QKeySequence::Undo);
    const auto redoBindings = QKeySequence::keyBindings(QKeySequence::Redo);
    QVERIFY(!undoBindings.isEmpty());
    QVERIFY(!redoBindings.isEmpty());
    for (const QKeySequence& undoBinding : undoBindings) {
        for (const QKeySequence& redoBinding : redoBindings) {
            Editor editor;
            editor.setAutoCompletionEnabled(false);
            editor.setText(expression);
            const QString original = editor.document()->toRawText();
            QTextCursor cursor = editor.textCursor();
            cursor.setPosition(position);
            cursor.setPosition(position + selectionLength, QTextCursor::KeepAnchor);
            editor.setTextCursor(cursor);
            QTest::keyClicks(&editor, QStringLiteral("9"));
            const QString edited = editor.document()->toRawText();
            QVERIFY(edited != original);
            editor.setCursorPosition(shortcutPosition);

            // Native shortcuts can carry a control character or printable text.
            const auto undoKey = undoBinding[0];
            QKeyEvent undoEvent(QEvent::KeyPress, undoKey.key(), undoKey.keyboardModifiers(), payload);
            QVERIFY(undoEvent.matches(QKeySequence::Undo));
            QApplication::sendEvent(&editor, &undoEvent);
            QCOMPARE(editor.document()->toRawText(), original);
            QVERIFY(editor.document()->isRedoAvailable());

            // Keep a selection active to catch accidental replacement by shortcut text.
            cursor = editor.textCursor();
            cursor.setPosition(0);
            cursor.setPosition(2, QTextCursor::KeepAnchor);
            editor.setTextCursor(cursor);
            const auto redoKey = redoBinding[0];
            const QString redoPayload = payload.isEmpty() ? QString()
                : payload.at(0).isPrint() ? QStringLiteral("z") : QString(QChar(0x19));
            QKeyEvent redoEvent(QEvent::KeyPress, redoKey.key(), redoKey.keyboardModifiers(), redoPayload);
            QVERIFY(redoEvent.matches(QKeySequence::Redo));
            QApplication::sendEvent(&editor, &redoEvent);
            QCOMPARE(editor.document()->toRawText(), edited);
            QApplication::sendEvent(&editor, &redoEvent);
            QCOMPARE(editor.document()->toRawText(), edited);

            QApplication::sendEvent(&editor, &undoEvent);
            QCOMPARE(editor.document()->toRawText(), original);
            QVERIFY(!editor.document()->isUndoAvailable());
            QApplication::sendEvent(&editor, &undoEvent);
            QCOMPARE(editor.document()->toRawText(), original);
            QVERIFY(editor.document()->isRedoAvailable());
        }
    }
}

void TestEditorUi::typing_undoes_one_key_at_a_time_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<bool>("inputMethod");
    for (bool inputMethod : {false, true}) {
        const QByteArray prefix = inputMethod ? "ime-" : "key-";
        QTest::newRow(QByteArray(prefix + "digits").constData()) << QStringLiteral("12345") << inputMethod;
        QTest::newRow(QByteArray(prefix + "operators").constData()) << QStringLiteral("12+34") << inputMethod;
        QTest::newRow(QByteArray(prefix + "units").constData()) << QStringLiteral("12[m/s]") << inputMethod;
        QTest::newRow(QByteArray(prefix + "degrees").constData()) << QString::fromUtf8("12º34'56\"") << inputMethod;
        QTest::newRow(QByteArray(prefix + "superscript-rewrite").constData()) << QStringLiteral("12^34") << inputMethod;
        if (!inputMethod) {
            QTest::newRow("key-unit-conversion-rewrite") << QStringLiteral("12--") << false;
            QTest::newRow("key-parenthesis-pair") << QStringLiteral("12+(34") << false;
        }
    }
}

void TestEditorUi::typing_undoes_one_key_at_a_time()
{
    QFETCH(QString, input);
    QFETCH(bool, inputMethod);
    Editor editor;
    editor.setAutoCompletionEnabled(false);
    editor.setText(QString());
    QStringList states = {editor.document()->toRawText()};
    for (const QChar ch : input) {
        if (inputMethod) {
            QInputMethodEvent event;
            event.setCommitString(QString(ch));
            QApplication::sendEvent(&editor, &event);
        } else if (ch.unicode() < 128) {
            QTest::keyClicks(&editor, QString(ch));
        } else {
            QKeyEvent event(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QString(ch));
            QApplication::sendEvent(&editor, &event);
        }
        const QString state = editor.document()->toRawText();
        if (state != states.last())
            states.append(state);
    }
    QVERIFY(states.size() > 2);
    for (int i = states.size() - 2; i >= 0; --i) {
        QVERIFY(editor.document()->isUndoAvailable());
        editor.undo();
        QCOMPARE(editor.document()->toRawText(), states.at(i));
    }
    QVERIFY(!editor.document()->isUndoAvailable());
    for (int i = 1; i < states.size(); ++i) {
        QVERIFY(editor.document()->isRedoAvailable());
        editor.redo();
        QCOMPARE(editor.document()->toRawText(), states.at(i));
    }
    QVERIFY(!editor.document()->isRedoAvailable());
}

void TestEditorUi::non_printable_key_payloads_do_not_replace_selection()
{
    for (ushort code : {0x01, 0x19, 0x1a, 0x7f}) {
        Editor editor;
        editor.setText(QStringLiteral("123456"));
        QTextCursor cursor = editor.textCursor();
        cursor.setPosition(2);
        cursor.setPosition(4, QTextCursor::KeepAnchor);
        editor.setTextCursor(cursor);
        QKeyEvent event(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QString(QChar(code)));
        QApplication::sendEvent(&editor, &event);
        QCOMPARE(editor.document()->toRawText(), QStringLiteral("123456"));
        QCOMPARE(editor.textCursor().selectedText(), QStringLiteral("34"));
        QVERIFY(!editor.document()->isUndoAvailable());
    }
}

void TestEditorUi::clipboard_shortcuts_do_not_insert_key_payloads_data()
{
    QTest::addColumn<QString>("cutPayload");
    QTest::addColumn<QString>("pastePayload");
    QTest::newRow("empty") << QString() << QString();
    QTest::newRow("control-characters") << QString(QChar(0x18)) << QString(QChar(0x16));
    QTest::newRow("printable-letters") << QStringLiteral("x") << QStringLiteral("v");
}

void TestEditorUi::clipboard_shortcuts_do_not_insert_key_payloads()
{
    QFETCH(QString, cutPayload);
    QFETCH(QString, pastePayload);
    Editor editor;
    editor.setText(QStringLiteral("123456"));
    QTextCursor cursor = editor.textCursor();
    cursor.setPosition(2);
    cursor.setPosition(4, QTextCursor::KeepAnchor);
    editor.setTextCursor(cursor);
    const auto cutKey = QKeySequence(QKeySequence::Cut)[0];
    QKeyEvent cutEvent(QEvent::KeyPress, cutKey.key(), cutKey.keyboardModifiers(), cutPayload);
    QApplication::sendEvent(&editor, &cutEvent);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("1256"));
    QCOMPARE(QApplication::clipboard()->text(), QStringLiteral("34"));

    const auto pasteKey = QKeySequence(QKeySequence::Paste)[0];
    QKeyEvent pasteEvent(QEvent::KeyPress, pasteKey.key(), pasteKey.keyboardModifiers(), pastePayload);
    QApplication::sendEvent(&editor, &pasteEvent);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("123456"));
    editor.undo();
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("1256"));
    editor.undo();
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("123456"));
}

void TestEditorUi::copy_shortcut_preserves_editor_state_data()
{
    QTest::addColumn<QString>("payload");
    QTest::addColumn<bool>("selectText");
    QTest::newRow("empty-selection") << QString() << true;
    QTest::newRow("control-character-selection") << QString(QChar(0x03)) << true;
    QTest::newRow("printable-letter-selection") << QStringLiteral("c") << true;
    QTest::newRow("empty-no-selection") << QString() << false;
    QTest::newRow("control-character-no-selection") << QString(QChar(0x03)) << false;
    QTest::newRow("printable-letter-no-selection") << QStringLiteral("c") << false;
}

void TestEditorUi::copy_shortcut_preserves_editor_state()
{
    QFETCH(QString, payload);
    QFETCH(bool, selectText);
    Editor editor;
    // MainWindow routes this signal to the widget whose selection is copied.
    // Use the editor as that widget here to exercise the clipboard operation.
    connect(&editor, &Editor::copySequencePressed, &editor, &QPlainTextEdit::copy);
    QSignalSpy copySpy(&editor, &Editor::copySequencePressed);
    QVERIFY(copySpy.isValid());

    editor.setText(QStringLiteral("123456"));
    editor.setCursorPosition(6);
    QTest::keyClicks(&editor, "78");
    editor.undo();
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("1234567"));
    QVERIFY(editor.document()->isUndoAvailable());
    QVERIFY(editor.document()->isRedoAvailable());

    QTextCursor cursor = editor.textCursor();
    cursor.setPosition(2);
    if (selectText)
        cursor.setPosition(4, QTextCursor::KeepAnchor);
    editor.setTextCursor(cursor);
    QApplication::clipboard()->setText(QStringLiteral("unchanged"));

    const auto copyKey = QKeySequence(QKeySequence::Copy)[0];
    QKeyEvent copyEvent(QEvent::KeyPress, copyKey.key(), copyKey.keyboardModifiers(), payload);
    copyEvent.ignore();
    QApplication::sendEvent(&editor, &copyEvent);

    QVERIFY(copyEvent.isAccepted());
    QCOMPARE(copySpy.count(), 1);
    QCOMPARE(QApplication::clipboard()->text(), selectText ? QStringLiteral("34") : QStringLiteral("unchanged"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("1234567"));
    QCOMPARE(editor.textCursor().position(), cursor.position());
    QCOMPARE(editor.textCursor().anchor(), cursor.anchor());
    QVERIFY(editor.document()->isUndoAvailable());
    QVERIFY(editor.document()->isRedoAvailable());
    editor.redo();
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("12345678"));
    editor.undo();
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("1234567"));
    editor.undo();
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("123456"));
}

void TestEditorUi::highlight_refresh_preserves_text_and_selection_without_edit_signals()
{
    Editor editor;
    editor.setText(QStringLiteral("123456"));
    QTextCursor cursor = editor.textCursor();
    cursor.setPosition(2);
    cursor.setPosition(4, QTextCursor::KeepAnchor);
    editor.setTextCursor(cursor);
    QSignalSpy textChangedSpy(&editor, &Editor::textChanged);
    QVERIFY(textChangedSpy.isValid());

    editor.setThemePrimaryColor(QColor(QStringLiteral("#334455")), true);
    editor.rehighlight();

    QCOMPARE(textChangedSpy.count(), 0);
    editor.setSession(nullptr);
    QCOMPARE(textChangedSpy.count(), 0);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("123456"));
    QCOMPARE(editor.textCursor().position(), cursor.position());
    QCOMPARE(editor.textCursor().anchor(), cursor.anchor());
    QVERIFY(!editor.document()->isUndoAvailable());
}

void TestEditorUi::primary_theme_changes_preserve_document_highlighting_data()
{
    QTest::addColumn<bool>("classic");
    QTest::newRow("modern") << false;
    QTest::newRow("classic") << true;
}

void TestEditorUi::primary_theme_changes_preserve_document_highlighting()
{
    QFETCH(bool, classic);
    Settings* settings = Settings::instance();
    settings->editorDisplayStyle = classic
        ? Settings::EditorDisplayClassic : Settings::EditorDisplayModern;
    settings->inputDigitGrouping = true;
    Editor editor;
    editor.setAutoCalcEnabled(false);
    editor.setAutoCompletionEnabled(false);
    editor.setThemePrimaryColor(QColor(QStringLiteral("#334455")), false);
    editor.insertPlainText(QStringLiteral("123456 + sin(2)"));
    editor.rehighlight();
    QCoreApplication::processEvents();
    QTextCursor cursor = editor.textCursor();
    cursor.setPosition(2);
    cursor.setPosition(8, QTextCursor::KeepAnchor);
    editor.setTextCursor(cursor);
    const auto formats = editor.document()->firstBlock().layout()->formats();
    QVERIFY(!formats.isEmpty());
    const int revision = editor.document()->revision();
    QSignalSpy documentChanged(editor.document(), &QTextDocument::contentsChanged);
    QSignalSpy textChanged(&editor, &Editor::textChanged);

    const QString inactiveStyle = editor.styleSheet();
    editor.setThemePrimaryColor(QColor(QStringLiteral("#334455")), true);
    QVERIFY(editor.styleSheet() != inactiveStyle);
    editor.setThemePrimaryColor(QColor(QStringLiteral("#556677")), true);
    QCOMPARE(editor.cursorColor(), QColor(QStringLiteral("#556677")));
    editor.setThemePrimaryColor(QColor(QStringLiteral("#556677")), false);
    QPalette viewportPalette = editor.viewport()->palette();
    viewportPalette.setColor(QPalette::Base, Qt::transparent);
    editor.viewport()->setPalette(viewportPalette);
    editor.setThemePrimaryColor(QColor(QStringLiteral("#556677")), false);
    QCOMPARE(editor.viewport()->palette().color(QPalette::Base),
             editor.palette().color(QPalette::Base));

    QCOMPARE(documentChanged.count(), 0);
    QCOMPARE(textChanged.count(), 0);
    QCOMPARE(editor.document()->revision(), revision);
    QCOMPARE(editor.document()->firstBlock().layout()->formats(), formats);
    QCOMPARE(editor.textCursor().position(), cursor.position());
    QCOMPARE(editor.textCursor().anchor(), cursor.anchor());
    QVERIFY(editor.document()->isUndoAvailable());
    editor.undo();
    QVERIFY(editor.text().isEmpty());
}

void TestEditorUi::highlight_refresh_updates_document_once_data()
{
    QTest::addColumn<bool>("preview");
    QTest::newRow("configured-theme") << false;
    QTest::newRow("preview-theme") << true;
}

void TestEditorUi::highlight_refresh_updates_document_once()
{
    QFETCH(bool, preview);
    Settings* settings = Settings::instance();
    settings->colorScheme = QStringLiteral("Custom");
    settings->customColorSchemeJson = themeJsonString(QJsonObject{
        {QStringLiteral("number"), QStringLiteral("#123456")},
        {QStringLiteral("background"), QStringLiteral("#102030")}
    });
    Editor editor;
    editor.setAutoCalcEnabled(false);
    editor.setAutoCompletionEnabled(false);
    editor.setText(QStringLiteral("123456"));
    const auto refreshTheme = [&]() {
        if (preview) {
            editor.setThemePreviewColorScheme(ColorScheme(QJsonDocument::fromJson(
                settings->customColorSchemeJson.toUtf8())));
        } else {
            editor.rehighlight();
        }
    };
    refreshTheme();
    QCoreApplication::processEvents();
    QCOMPARE(editor.document()->firstBlock().layout()->formats().first().format.foreground().color(),
             QColor(QStringLiteral("#123456")));
    QSignalSpy documentChanged(editor.document(), &QTextDocument::contentsChanged);
    QSignalSpy textChanged(&editor, &Editor::textChanged);
    settings->customColorSchemeJson = themeJsonString(QJsonObject{
        {QStringLiteral("number"), QStringLiteral("#abcdef")},
        {QStringLiteral("background"), QStringLiteral("#102030")}
    });

    refreshTheme();

    QCOMPARE(documentChanged.count(), 1);
    QCOMPARE(textChanged.count(), 0);
    QCOMPARE(editor.document()->firstBlock().layout()->formats().first().format.foreground().color(),
             QColor(QStringLiteral("#abcdef")));
    QCOMPARE(editor.text(), QStringLiteral("123456"));
}

void TestEditorUi::unchanged_primary_theme_avoids_editor_style_refresh_data()
{
    QTest::addColumn<bool>("classic");
    QTest::newRow("modern") << false;
    QTest::newRow("classic") << true;
}

void TestEditorUi::unchanged_primary_theme_avoids_editor_style_refresh()
{
    QFETCH(bool, classic);
    Settings::instance()->editorDisplayStyle = classic
        ? Settings::EditorDisplayClassic : Settings::EditorDisplayModern;
    Editor editor;
    editor.setAutoCompletionEnabled(false);
    const QColor primary(QStringLiteral("#334455"));
    editor.setThemePrimaryColor(primary, true);
    editor.setText(QStringLiteral("123+456"));
    editor.setCursorPosition(editor.text().size());

    class StyleChangeRecorder : public QObject {
    public:
        int count = 0;
        bool eventFilter(QObject*, QEvent* event) override
        {
            if (event->type() == QEvent::StyleChange)
                ++count;
            return false;
        }
    } recorder;
    editor.installEventFilter(&recorder);
    for (int i = 0; i < 5; ++i) {
        QTest::keyClicks(&editor, QStringLiteral("1"));
        editor.setThemePrimaryColor(primary, true);
    }
    QCOMPARE(recorder.count, 0);
    QCOMPARE(editor.text(), QStringLiteral("123+45611111"));

    const QString activeStyle = editor.styleSheet();
    editor.setThemePrimaryColor(primary, false);
    QVERIFY(recorder.count > 0);
    QVERIFY(editor.styleSheet() != activeStyle);
    recorder.count = 0;
    editor.setThemePrimaryColor(primary, false);
    QCOMPARE(recorder.count, 0);

    editor.setThemePrimaryColor(QColor(QStringLiteral("#556677")), true);
    QVERIFY(recorder.count > 0);
    QVERIFY(editor.styleSheet() != activeStyle);

    QPalette viewportPalette = editor.viewport()->palette();
    viewportPalette.setColor(QPalette::Base, Qt::transparent);
    editor.viewport()->setPalette(viewportPalette);
    editor.setThemePrimaryColor(QColor(QStringLiteral("#556677")), true);
    QCOMPARE(editor.viewport()->palette().color(QPalette::Base),
             editor.palette().color(QPalette::Base));
}

void TestEditorUi::editor_text_line_is_vertically_centered_data()
{
    QTest::addColumn<bool>("classic");
    QTest::addColumn<int>("pixelSize");
    QTest::addColumn<int>("lineCount");
    QTest::addColumn<bool>("fixedFont");
    QTest::addColumn<bool>("wrapped");
    for (const bool classic : {false, true}) {
        for (const int pixelSize : {16, 24, 32}) {
            for (const int lineCount : {1, 5}) {
                const QByteArray name = QByteArray(classic ? "classic-" : "modern-")
                    + QByteArray::number(pixelSize) + '-' + QByteArray::number(lineCount);
                QTest::newRow(name.constData()) << classic << pixelSize << lineCount << false << false;
                QTest::newRow(QByteArray(name + "-fixed").constData())
                    << classic << pixelSize << lineCount << true << false;
                if (lineCount > 1) {
                    QTest::newRow(QByteArray(name + "-wrapped").constData())
                        << classic << pixelSize << lineCount << false << true;
                    QTest::newRow(QByteArray(name + "-fixed-wrapped").constData())
                        << classic << pixelSize << lineCount << true << true;
                }
            }
        }
    }
}

void TestEditorUi::editor_text_line_is_vertically_centered()
{
    QFETCH(bool, classic);
    QFETCH(int, pixelSize);
    QFETCH(int, lineCount);
    QFETCH(bool, fixedFont);
    QFETCH(bool, wrapped);
    Settings::instance()->editorDisplayStyle = classic
        ? Settings::EditorDisplayClassic : Settings::EditorDisplayModern;
    Settings::instance()->inputDigitGrouping = false;
    Editor editor;
    QFont font = fixedFont ? QFontDatabase::systemFont(QFontDatabase::FixedFont) : editor.font();
    font.setPixelSize(pixelSize);
    editor.setFont(font);
    editor.setFixedWidth(400);
    editor.rehighlight();
    editor.setThemePrimaryColor(QColor(QStringLiteral("#90aaff")), true);
    QStringList lines;
    for (int i = 0; i < lineCount; ++i)
        lines.append(QStringLiteral("23323323"));
    editor.setText(lines.join(QLatin1Char('\n')));
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    if (wrapped) {
        const int decorations = editor.width() - editor.viewport()->width();
        editor.setFixedWidth(decorations + qCeil(QFontMetricsF(font).horizontalAdvance(lines.first()))
            + 2 * int(editor.document()->documentMargin()) + 1);
        editor.setText(lines.join(QString()));
    }
    QCoreApplication::processEvents();
    QCOMPARE(editor.document()->lineCount(), lineCount);
    QTextCursor first(editor.document());
    QTextCursor last(editor.document());
    last.movePosition(QTextCursor::End);
    const QRect firstLine = editor.cursorRect(first).translated(editor.viewport()->pos());
    const QRect lastLine = editor.cursorRect(last).translated(editor.viewport()->pos());
    const int top = classic ? UiConfig::OutlineStrokeWidth : UiConfig::ModernEditorTopMargin;
    const int bottom = editor.height()
        - (classic ? 0 : UiConfig::ModernEditorBottomMargin);
    QVERIFY2(qAbs((firstLine.top() + lastLine.bottom() + 1) - (top + bottom)) <= 1,
        qPrintable(QStringLiteral("Text spans %1..%2 inside editor area %3..%4")
            .arg(firstLine.top()).arg(lastLine.bottom() + 1).arg(top).arg(bottom)));
}

void TestEditorUi::treats_spaced_question_comment_as_atomic_navigation_and_edit_token()
{
    // State: "1 ? 2".
    // Action: use Left/Right, Alt+Left/Right, Backspace, Delete near token.
    // Expected: grouped " ? " behaves as one atomic token.
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    const QString commentToken = QStringLiteral(" ? ");
    const QString expression = QStringLiteral("1") + commentToken + QStringLiteral("2");

    editor.setText(expression);
    editor.setCursorPosition(4); // between '?' and trailing space
    QTest::keyClick(&editor, Qt::Key_Left, Qt::NoModifier);
    QCOMPARE(editor.textCursor().position(), 1);
    QTest::keyClick(&editor, Qt::Key_Right, Qt::NoModifier);
    QCOMPARE(editor.textCursor().position(), 4);

    editor.setText(expression);
    editor.setCursorPosition(4); // right after grouped token
    QTest::keyClick(&editor, Qt::Key_Left, Qt::AltModifier);
    QCOMPARE(editor.textCursor().position(), 1);
    editor.setCursorPosition(1); // right before grouped token
    QTest::keyClick(&editor, Qt::Key_Right, Qt::AltModifier);
    QCOMPARE(editor.textCursor().position(), 4);

    editor.setText(expression);
    editor.setCursorPosition(4); // immediately after grouped token
    QTest::keyClick(&editor, Qt::Key_Backspace, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("12"));

    editor.setText(expression);
    editor.setCursorPosition(1); // immediately before grouped token
    QTest::keyClick(&editor, Qt::Key_Delete, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("12"));
}

void TestEditorUi::treats_spaced_equal_as_atomic_navigation_and_edit_token()
{
    // State: "1", then type '=' with editor insertion rules.
    // Action: navigate/edit around grouped token.
    // Expected: grouped " = " behaves as one atomic token.
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("1"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Equal, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("1 = "));

    editor.insert(QStringLiteral("2"));
    const QString expression = editor.document()->toRawText();
    QCOMPARE(expression, QStringLiteral("1 = 2"));

    editor.setCursorPosition(4); // right after grouped token
    QTest::keyClick(&editor, Qt::Key_Left, Qt::NoModifier);
    QCOMPARE(editor.textCursor().position(), 1);

    editor.setCursorPosition(1); // right before grouped token
    QTest::keyClick(&editor, Qt::Key_Right, Qt::NoModifier);
    QCOMPARE(editor.textCursor().position(), 4);

    editor.setCursorPosition(4); // immediately after grouped token
    QTest::keyClick(&editor, Qt::Key_Backspace, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("12"));

    editor.setText(expression);
    editor.setCursorPosition(1); // immediately before grouped token
    QTest::keyClick(&editor, Qt::Key_Delete, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("12"));
}

void TestEditorUi::treats_leading_question_comment_as_atomic_navigation_and_edit_token()
{
    // State: "? 2" (comment token at start of editor).
    // Action: use Left/Right, Alt+Left/Right, Backspace, Delete near token.
    // Expected: grouped "? " behaves as one atomic token.
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    const QString expression = QStringLiteral("? 2");

    editor.setText(expression);
    editor.setCursorPosition(1); // between '?' and trailing space
    QTest::keyClick(&editor, Qt::Key_Left, Qt::NoModifier);
    QCOMPARE(editor.textCursor().position(), 0);
    QTest::keyClick(&editor, Qt::Key_Right, Qt::NoModifier);
    QCOMPARE(editor.textCursor().position(), 2);

    editor.setText(expression);
    editor.setCursorPosition(2); // right after grouped token
    QTest::keyClick(&editor, Qt::Key_Left, Qt::AltModifier);
    QCOMPARE(editor.textCursor().position(), 0);
    editor.setCursorPosition(0); // right before grouped token
    QTest::keyClick(&editor, Qt::Key_Right, Qt::AltModifier);
    QCOMPARE(editor.textCursor().position(), 2);

    editor.setText(expression);
    editor.setCursorPosition(2); // immediately after grouped token
    QTest::keyClick(&editor, Qt::Key_Backspace, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("2"));

    editor.setText(expression);
    editor.setCursorPosition(0); // immediately before grouped token
    QTest::keyClick(&editor, Qt::Key_Delete, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("2"));
}

void TestEditorUi::ignores_space_right_after_spaced_unit_conversion_operator()
{
    // State: after conversion to spaced "→ []" token with cursor inside [].
    // Action: type SPACE.
    // Expected: ignored because current [] unit context is empty.
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("1"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Minus, Qt::NoModifier);
    QKeyEvent greaterByText(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QStringLiteral(">"));
    QApplication::sendEvent(&editor, &greaterByText);

    const QString beforeSpace = editor.document()->toRawText();
    QTest::keyClick(&editor, Qt::Key_Space, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), beforeSpace);
}

void TestEditorUi::unit_bracket_context_accepts_div_mul_and_rejects_addition()
{
    // State: unmatched "[m" unit context.
    // Action: type '+', '/', and '*'.
    // Expected: '+' rejected; '/' and multiplication accepted.
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("[m"));
    editor.setCursorPosition(editor.text().size());

    const QString beforePlus = editor.document()->toRawText();
    QTest::keyClick(&editor, Qt::Key_Plus, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), beforePlus);

    QTest::keyClick(&editor, Qt::Key_Slash, Qt::NoModifier);
    const QString afterSlash = editor.document()->toRawText();
    QVERIFY(afterSlash.contains(MathDsl::DivOp));

    editor.setText(QStringLiteral("[m"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Asterisk, Qt::NoModifier);
    const QString afterMul = editor.document()->toRawText();
    QVERIFY(afterMul.contains(MathDsl::MulCrossOp)
            || afterMul.contains(MathDsl::MulDotOp));
}

void TestEditorUi::unit_bracket_context_blocks_space_and_ops_when_only_spaces()
{
    // State: unmatched "[   " and cursor at end.
    // Action: type SPACE and operator keys.
    // Expected: all are ignored while unit context has only whitespace.
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    const QString onlySpaces = QStringLiteral("[   ");

    editor.setText(onlySpaces);
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Space, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), onlySpaces);

    editor.setText(onlySpaces);
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Asterisk, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), onlySpaces);

    editor.setText(onlySpaces);
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Slash, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), onlySpaces);

    editor.setText(onlySpaces);
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Minus, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), onlySpaces);

    editor.setText(onlySpaces);
    editor.setCursorPosition(editor.text().size());
    QKeyEvent shiftSlashByText(QEvent::KeyPress, Qt::Key_unknown, Qt::ShiftModifier, QStringLiteral("/"));
    QApplication::sendEvent(&editor, &shiftSlashByText);
    QCOMPARE(editor.document()->toRawText(), onlySpaces);

    editor.setText(onlySpaces);
    editor.setCursorPosition(editor.text().size());
    QKeyEvent keypadSlash(QEvent::KeyPress, Qt::Key_division, Qt::KeypadModifier, QStringLiteral("/"));
    QApplication::sendEvent(&editor, &keypadSlash);
    QCOMPARE(editor.document()->toRawText(), onlySpaces);
}

void TestEditorUi::unit_bracket_context_accepts_quote_marks_as_arc_units()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("["));
    editor.setCursorPosition(editor.text().size());
    QKeyEvent apostropheByText(
        QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QStringLiteral("'"));
    QApplication::sendEvent(&editor, &apostropheByText);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[") + QString(UnicodeChars::Prime));

    editor.setText(QStringLiteral("["));
    editor.setCursorPosition(editor.text().size());
    QKeyEvent quoteByText(
        QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QStringLiteral("\""));
    QApplication::sendEvent(&editor, &quoteByText);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[") + QString(UnicodeChars::DoublePrime));
}

void TestEditorUi::unit_bracket_context_accepts_quote_marks_via_ime_commit()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    QList<QInputMethodEvent::Attribute> imeAttributes;

    editor.setText(QStringLiteral("["));
    editor.setCursorPosition(editor.text().size());
    QInputMethodEvent apostropheCommit(QString(), imeAttributes);
    apostropheCommit.setCommitString(QStringLiteral("'"));
    QApplication::sendEvent(&editor, &apostropheCommit);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[") + QString(UnicodeChars::Prime));

    editor.setText(QStringLiteral("["));
    editor.setCursorPosition(editor.text().size());
    QInputMethodEvent quoteCommit(QString(), imeAttributes);
    quoteCommit.setCommitString(QStringLiteral("\""));
    QApplication::sendEvent(&editor, &quoteCommit);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[") + QString(UnicodeChars::DoublePrime));
}

void TestEditorUi::unit_bracket_context_allows_letter_after_middle_dot()
{
    // State: "2 [m·]" with cursor after '·'.
    // Action: type letter and then digit.
    // Expected: letter accepted; digit rejected.
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("2 [m·]"));
    editor.setCursorPosition(5); // right after middle-dot, before ']'
    QTest::keyClicks(&editor, QStringLiteral("s"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("2 [m·s]"));

    editor.setText(QStringLiteral("2 [m·]"));
    editor.setCursorPosition(5); // right after middle-dot, before ']'
    QTest::keyClicks(&editor, QStringLiteral("2"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("2 [m·]"));
}

void TestEditorUi::unit_bracket_context_disallows_variables_and_constants()
{
    // State: expressions "2[foo]", "2[e]", "2[pi]".
    // Action: evaluate with foo defined as variable.
    // Expected: all fail with "unknown unit".
    Session session;
    Evaluator* evaluator = session.evaluator();
    evaluator->setVariable(QStringLiteral("foo"), Quantity(3));

    evaluator->setExpression(QStringLiteral("2[foo]"));
    evaluator->evalUpdateAns();
    QVERIFY(evaluator->error().contains(QStringLiteral("unknown unit"), Qt::CaseInsensitive));

    evaluator->setExpression(QStringLiteral("2[e]"));
    evaluator->evalUpdateAns();
    QVERIFY(evaluator->error().contains(QStringLiteral("unknown unit"), Qt::CaseInsensitive));

    evaluator->setExpression(QStringLiteral("2[pi]"));
    evaluator->evalUpdateAns();
    QVERIFY(evaluator->error().contains(QStringLiteral("unknown unit"), Qt::CaseInsensitive));

    evaluator->unsetVariable(QStringLiteral("foo"));
}

void TestEditorUi::evaluator_accepts_compact_arc_symbol_units()
{
    Session session;
    Evaluator* evaluator = session.evaluator();
    const auto tokensDebug = [evaluator](const QString& expr) {
        const Tokens tokens = evaluator->scan(expr);
        QStringList parts;
        for (const Token& token : tokens) {
            parts.append(QStringLiteral("{t=%1,text='%2'}")
                .arg(static_cast<int>(token.type()))
                .arg(token.text()));
        }
        return QStringLiteral("valid=%1 tokens=%2")
            .arg(tokens.valid() ? QStringLiteral("true") : QStringLiteral("false"))
            .arg(parts.join(QStringLiteral(", ")));
    };

    evaluator->setExpression(QString::fromUtf8("1 [′]"));
    evaluator->evalUpdateAns();
    QVERIFY2(evaluator->error().isEmpty(),
             qPrintable(QStringLiteral("Unexpected error for [′]: %1 (%2)")
                .arg(evaluator->error(), tokensDebug(QString::fromUtf8("1 [′]")))));

    evaluator->setExpression(QString::fromUtf8("1 [″]"));
    evaluator->evalUpdateAns();
    QVERIFY2(evaluator->error().isEmpty(),
             qPrintable(QStringLiteral("Unexpected error for [″]: %1 (%2)")
                .arg(evaluator->error(), tokensDebug(QString::fromUtf8("1 [″]")))));
}

void TestEditorUi::unit_bracket_context_allows_digits_and_minus_only_in_exponent_positions()
{
    // State: many "[m...]" exponent-denominator states.
    // Action: type digits, minus, slash, letters in each state.
    // Expected: only grammar-valid unit-exponent forms are accepted.
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("[m"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClicks(&editor, QStringLiteral("2"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m2"));

    editor.setText(QStringLiteral("[m^"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClicks(&editor, QStringLiteral("2"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m") + MathDsl::Pow2);

    editor.setText(QStringLiteral("[m^"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClicks(&editor, QStringLiteral("x"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m^"));

    editor.setText(QStringLiteral("[m^"));
    editor.setCursorPosition(editor.text().size());
    QList<QInputMethodEvent::Attribute> imeAttributes;
    QInputMethodEvent imeLetterEvent(QString(), imeAttributes);
    imeLetterEvent.setCommitString(QStringLiteral("x"));
    QApplication::sendEvent(&editor, &imeLetterEvent);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m^"));

    editor.setText(QStringLiteral("[m^("));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClicks(&editor, QStringLiteral("2"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m^(2"));

    editor.setText(QStringLiteral("[m^2"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClicks(&editor, QStringLiteral("3"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m^23"));

    editor.setText(QStringLiteral("[m^2"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Space, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(),
             QStringLiteral("[m") + MathDsl::Pow2 + QString(MathDsl::MulDotOp));

    editor.setText(QStringLiteral("[s") + MathDsl::Pow2);
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Space, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(),
             QStringLiteral("[s") + MathDsl::Pow2 + QString(MathDsl::MulDotOp));

    editor.setText(QStringLiteral("[m^2"));
    editor.setCursorPosition(editor.text().size());
    QKeyEvent slashAfterPlainExponent(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QStringLiteral("/"));
    QApplication::sendEvent(&editor, &slashAfterPlainExponent);
    QCOMPARE(editor.document()->toRawText(),
             QStringLiteral("[m") + MathDsl::Pow2 + QString(MathDsl::DivOp));

    editor.setText(QStringLiteral("[m^2"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Asterisk, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(),
             QStringLiteral("[m") + MathDsl::Pow2 + QString(MathDsl::MulDotOp));

    editor.setText(QStringLiteral("[m^2"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClicks(&editor, QStringLiteral("e"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m^2"));

    editor.setText(QStringLiteral("[m"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClicks(&editor, QStringLiteral("eter"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[meter"));

    editor.setText(QStringLiteral("[m"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_ParenLeft, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m()"));

    editor.setText(QStringLiteral("[m"));
    editor.setCursorPosition(editor.text().size());
    QKeyEvent nestedBracketByText(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QStringLiteral("["));
    QApplication::sendEvent(&editor, &nestedBracketByText);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m"));

    editor.setText(QStringLiteral("[m"));
    editor.setCursorPosition(editor.text().size());
    QKeyEvent closeBracketByText(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QStringLiteral("]"));
    QApplication::sendEvent(&editor, &closeBracketByText);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m]"));

    editor.setText(QStringLiteral("[s^222"));
    editor.setCursorPosition(editor.text().size());
    QKeyEvent closeBracketAfterExponent(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QStringLiteral("]"));
    QApplication::sendEvent(&editor, &closeBracketAfterExponent);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[s") + MathDsl::Pow2 + MathDsl::Pow2 + MathDsl::Pow2 + QStringLiteral("]"));

    editor.setText(QStringLiteral("[m]"));
    editor.setCursorPosition(editor.text().size());
    QKeyEvent chainedBracketByText(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QStringLiteral("["));
    QApplication::sendEvent(&editor, &chainedBracketByText);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m]"));

    editor.setText(QStringLiteral("[m]"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_ParenLeft, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m]()"));

    editor.setText(QStringLiteral("[]")
                   + QString(MathDsl::MulDotWrapSp)
                   + QString(MathDsl::MulDotOp)
                   + QString(MathDsl::MulDotWrapSp)
                   + QStringLiteral("()"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_ParenLeft, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(),
             QStringLiteral("[]")
                 + QString(MathDsl::MulDotWrapSp)
                 + QString(MathDsl::MulDotOp)
                 + QString(MathDsl::MulDotWrapSp)
                 + QStringLiteral("()")
                 + QStringLiteral("()"));

    editor.setText(QStringLiteral("[m^(2"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClicks(&editor, QStringLiteral("3"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m^(23"));

    editor.setText(QStringLiteral("[m"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Minus, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m"));

    editor.setText(QStringLiteral("[m^"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Minus, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m") + MathDsl::PowNeg);

    editor.setText(QStringLiteral("[m^("));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Minus, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m^(") + QString(MathDsl::SubOp));

    editor.setText(QStringLiteral("[m") + MathDsl::PowNeg);
    editor.setCursorPosition(editor.text().size());
    QTest::keyClicks(&editor, QStringLiteral("2"));
    QCOMPARE(editor.document()->toRawText(),
             QStringLiteral("[m") + MathDsl::PowNeg + MathDsl::Pow2);

    editor.setText(QStringLiteral("[m^(") + QString(MathDsl::SubOp));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClicks(&editor, QStringLiteral("2"));
    QCOMPARE(editor.document()->toRawText(),
             QStringLiteral("[m^(") + QString(MathDsl::SubOp) + QStringLiteral("2"));

    editor.setText(QStringLiteral("[m^(1"));
    editor.setCursorPosition(editor.text().size());
    QKeyEvent slashByText1(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QStringLiteral("/"));
    QApplication::sendEvent(&editor, &slashByText1);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m^(1/"));

    // m^/ -> reject slash right after exponent start.
    editor.setText(QStringLiteral("[m^"));
    editor.setCursorPosition(editor.text().size());
    QKeyEvent slashAfterExponentStart(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QStringLiteral("/"));
    QApplication::sendEvent(&editor, &slashAfterExponentStart);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m^"));

    // m⁻/ -> reject slash right after signed exponent start.
    editor.setText(QStringLiteral("[m") + MathDsl::PowNeg);
    editor.setCursorPosition(editor.text().size());
    QKeyEvent slashAfterSignedExponentStart(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QStringLiteral("/"));
    QApplication::sendEvent(&editor, &slashAfterSignedExponentStart);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m") + MathDsl::PowNeg);

    editor.setText(QStringLiteral("[m^(") + QString(MathDsl::SubOp) + QStringLiteral("1"));
    editor.setCursorPosition(editor.text().size());
    QKeyEvent slashByText2(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QStringLiteral("/"));
    QApplication::sendEvent(&editor, &slashByText2);
    QCOMPARE(editor.document()->toRawText(),
             QStringLiteral("[m^(") + QString(MathDsl::SubOp) + QStringLiteral("1/"));
    QKeyEvent slashByText3(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QStringLiteral("/"));
    QApplication::sendEvent(&editor, &slashByText3);
    QCOMPARE(editor.document()->toRawText(),
             QStringLiteral("[m^(") + QString(MathDsl::SubOp) + QStringLiteral("1/"));

    // m^2/3 -> reject digit after slash (non-parenthesized exponent).
    editor.setText(QStringLiteral("[m^2"));
    editor.setCursorPosition(editor.text().size());
    QKeyEvent slashByText4(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QStringLiteral("/"));
    QApplication::sendEvent(&editor, &slashByText4);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m") + MathDsl::Pow2 + QStringLiteral("/"));
    QTest::keyClicks(&editor, QStringLiteral("s"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m") + MathDsl::Pow2 + QStringLiteral("/s"));

    editor.setText(QStringLiteral("[m^3"));
    editor.setCursorPosition(editor.text().size());
    QKeyEvent slashByText4b(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QStringLiteral("/"));
    QApplication::sendEvent(&editor, &slashByText4b);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m") + MathDsl::Pow3 + QStringLiteral("/"));
    QTest::keyClicks(&editor, QStringLiteral("s"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m") + MathDsl::Pow3 + QStringLiteral("/s"));

    // m²·/ -> reject slash right after multiplication separator.
    editor.setText(QStringLiteral("[m") + MathDsl::Pow2 + QString(MathDsl::MulDotOp));
    editor.setCursorPosition(editor.text().size());
    QKeyEvent slashAfterMulDot(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QStringLiteral("/"));
    QApplication::sendEvent(&editor, &slashAfterMulDot);
    QCOMPARE(editor.document()->toRawText(),
             QStringLiteral("[m") + MathDsl::Pow2 + QString(MathDsl::MulDotOp));

    editor.setText(QStringLiteral("[m^2"));
    editor.setCursorPosition(editor.text().size());
    QKeyEvent slashByText4c(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QStringLiteral("/"));
    QApplication::sendEvent(&editor, &slashByText4c);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m") + MathDsl::Pow2 + QStringLiteral("/"));
    QTest::keyClicks(&editor, QStringLiteral("3"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m") + MathDsl::Pow2 + QStringLiteral("/"));

    // m^/ + "/" -> trim invalid slash after exponent start.
    editor.setText(QStringLiteral("[m^/"));
    editor.setCursorPosition(editor.text().size());
    QKeyEvent slashByText4d(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QStringLiteral("/"));
    QApplication::sendEvent(&editor, &slashByText4d);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m^"));

    // m^-2/3 -> reject digit after slash (non-parenthesized signed exponent).
    editor.setText(QStringLiteral("[m^") + QString(MathDsl::SubOp) + QStringLiteral("2"));
    editor.setCursorPosition(editor.text().size());
    QKeyEvent slashByText5(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QStringLiteral("/"));
    QApplication::sendEvent(&editor, &slashByText5);
    QCOMPARE(editor.document()->toRawText(),
             QStringLiteral("[m") + MathDsl::PowNeg + MathDsl::Pow2 + QStringLiteral("/"));
    QTest::keyClicks(&editor, QStringLiteral("3"));
    QCOMPARE(editor.document()->toRawText(),
             QStringLiteral("[m") + MathDsl::PowNeg + MathDsl::Pow2 + QStringLiteral("/"));

    // m^(2/3 -> accept digit after slash (parenthesized exponent).
    editor.setText(QStringLiteral("[m^(2"));
    editor.setCursorPosition(editor.text().size());
    QKeyEvent slashByText6(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QStringLiteral("/"));
    QApplication::sendEvent(&editor, &slashByText6);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m^(2/"));
    QTest::keyClicks(&editor, QStringLiteral("s"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m^(2/"));
    QTest::keyClicks(&editor, QStringLiteral("3"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m^(2/3"));

    // m².3 -> allow radix and following digits inside parenthesized exponent rewrite.
    editor.setText(QStringLiteral("[m") + MathDsl::Pow2);
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Period, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m^(2.)"));
    QCOMPARE(editor.textCursor().position(),
             editor.document()->toRawText().size() - 1);
    QTest::keyClicks(&editor, QStringLiteral("3"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m^(2.3)"));
    QCOMPARE(editor.textCursor().position(),
             editor.document()->toRawText().size() - 1);

    // m^(-2/3 -> accept digit after slash (parenthesized signed exponent).
    editor.setText(QStringLiteral("[m^(") + QString(MathDsl::SubOp) + QStringLiteral("2"));
    editor.setCursorPosition(editor.text().size());
    QKeyEvent slashByText7(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QStringLiteral("/"));
    QApplication::sendEvent(&editor, &slashByText7);
    QCOMPARE(editor.document()->toRawText(),
             QStringLiteral("[m^(") + QString(MathDsl::SubOp) + QStringLiteral("2/"));
    QTest::keyClicks(&editor, QStringLiteral("s"));
    QCOMPARE(editor.document()->toRawText(),
             QStringLiteral("[m^(") + QString(MathDsl::SubOp) + QStringLiteral("2/"));
    QTest::keyClicks(&editor, QStringLiteral("3"));
    QCOMPARE(editor.document()->toRawText(),
             QStringLiteral("[m^(") + QString(MathDsl::SubOp) + QStringLiteral("2/3"));

    editor.setText(QStringLiteral("[m^(-2"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClicks(&editor, QStringLiteral("3"));
    QCOMPARE(editor.document()->toRawText(),
             QStringLiteral("[m^(") + QString(MathDsl::SubOp) + QStringLiteral("23"));

    editor.setText(QStringLiteral("[m"));
    editor.setCursorPosition(editor.text().size());
    QKeyEvent deadTilde(QEvent::KeyPress, Qt::Key_Dead_Tilde, Qt::NoModifier);
    QApplication::sendEvent(&editor, &deadTilde);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m"));

    editor.setText(QStringLiteral("3 + "));
    editor.setCursorPosition(editor.text().size());
    QKeyEvent deadTildeAfterPlus(QEvent::KeyPress, Qt::Key_Dead_Tilde, Qt::NoModifier);
    QApplication::sendEvent(&editor, &deadTildeAfterPlus);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("3 + ~"));

    editor.setText(QStringLiteral("3 + "));
    editor.setCursorPosition(editor.text().size());
    QKeyEvent regularTildeAfterPlus(
        QEvent::KeyPress, Qt::Key_AsciiTilde, Qt::NoModifier, QStringLiteral("~"));
    QApplication::sendEvent(&editor, &regularTildeAfterPlus);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("3 + ~"));

    editor.setText(QStringLiteral("[m"));
    editor.setCursorPosition(editor.text().size());
    QList<QInputMethodEvent::Attribute> deadImeAttrs;
    QInputMethodEvent deadTildePreedit(QStringLiteral("~"), deadImeAttrs);
    QApplication::sendEvent(&editor, &deadTildePreedit);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[m"));

    editor.setText(QStringLiteral("[s") + MathDsl::Pow2);
    editor.setCursorPosition(editor.text().size());
    QTest::keyClicks(&editor, QStringLiteral("^"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[s") + MathDsl::Pow2);

    editor.setText(QStringLiteral("2")
                   + QString(MathDsl::QuantSp)
                   + QStringLiteral("[m/s")
                   + QString(MathDsl::MulDotOp));
    editor.setCursorPosition(editor.text().size());
    QKeyEvent deadCircumflexInUnitAfterDot(QEvent::KeyPress, Qt::Key_Dead_Circumflex, Qt::NoModifier);
    QApplication::sendEvent(&editor, &deadCircumflexInUnitAfterDot);
    QCOMPARE(editor.document()->toRawText(),
             QStringLiteral("2")
                 + QString(MathDsl::QuantSp)
                 + QStringLiteral("[m/s")
                 + QString(MathDsl::MulDotOp));

    QList<QInputMethodEvent::Attribute> caretImeAttrs;
    QInputMethodEvent caretCommitAfterDot(QString(), caretImeAttrs);
    caretCommitAfterDot.setCommitString(QStringLiteral("^"));
    QApplication::sendEvent(&editor, &caretCommitAfterDot);
    QCOMPARE(editor.document()->toRawText(),
             QStringLiteral("2")
                 + QString(MathDsl::QuantSp)
                 + QStringLiteral("[m/s")
                 + QString(MathDsl::MulDotOp));

    QInputMethodEvent caretPreeditAfterDot(QString::fromUtf8("ˆ"), caretImeAttrs);
    QApplication::sendEvent(&editor, &caretPreeditAfterDot);
    QInputMethodEvent digitCommitAfterCaretPreedit(QString(), caretImeAttrs);
    digitCommitAfterCaretPreedit.setCommitString(QStringLiteral("2"));
    QApplication::sendEvent(&editor, &digitCommitAfterCaretPreedit);
    QCOMPARE(editor.document()->toRawText(),
             QStringLiteral("2")
                 + QString(MathDsl::QuantSp)
                 + QStringLiteral("[m/s")
                 + QString(MathDsl::MulDotOp)
                 + QStringLiteral("2"));

    editor.setText(QStringLiteral("2")
                   + QString(MathDsl::QuantSp)
                   + QStringLiteral("[m")
                   + MathDsl::Pow2
                   + QStringLiteral("/s"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Asterisk, Qt::NoModifier);
    QTest::keyClick(&editor, Qt::Key_Asterisk, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(),
             QStringLiteral("2")
                 + QString(MathDsl::QuantSp)
                 + QStringLiteral("[m")
                 + MathDsl::Pow2
                 + QStringLiteral("/s^"));
}

void TestEditorUi::converts_caret_exponents_to_superscripts_globally()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("s^"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClicks(&editor, QStringLiteral("2"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("s") + MathDsl::Pow2);

    editor.setText(QStringLiteral("s^"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Minus, Qt::NoModifier);
    QTest::keyClicks(&editor, QStringLiteral("2"));
    QCOMPARE(editor.document()->toRawText(),
             QStringLiteral("s") + MathDsl::PowNeg + MathDsl::Pow2);

    editor.setText(QStringLiteral("[s^"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClicks(&editor, QStringLiteral("2"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[s") + MathDsl::Pow2);

    editor.setText(QStringLiteral("s"));
    editor.setCursorPosition(editor.text().size());
    QKeyEvent deadCircumflex(QEvent::KeyPress, Qt::Key_Dead_Circumflex, Qt::NoModifier);
    QApplication::sendEvent(&editor, &deadCircumflex);
    QTest::keyClicks(&editor, QStringLiteral("2"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("s") + MathDsl::Pow2);

    editor.setText(QStringLiteral("s"));
    editor.setCursorPosition(editor.text().size());
    QList<QInputMethodEvent::Attribute> imeAttrs;
    QInputMethodEvent imeWithCaretPreedit(QString::fromUtf8("ˆ"), imeAttrs);
    QApplication::sendEvent(&editor, &imeWithCaretPreedit);
    QInputMethodEvent imeDigitCommit(QString(), imeAttrs);
    imeDigitCommit.setCommitString(QStringLiteral("2"));
    QApplication::sendEvent(&editor, &imeDigitCommit);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("s") + MathDsl::Pow2);

    editor.setText(QStringLiteral("s") + MathDsl::Pow2);
    editor.setCursorPosition(editor.text().size());
    QInputMethodEvent imeCaretCommit(QString(), imeAttrs);
    imeCaretCommit.setCommitString(QStringLiteral("^"));
    QApplication::sendEvent(&editor, &imeCaretCommit);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("s") + MathDsl::Pow2);

    editor.setText(QStringLiteral("s") + MathDsl::Pow2);
    editor.setCursorPosition(editor.text().size());
    QInputMethodEvent imeCaretPreeditAfterSup(QString::fromUtf8("ˆ"), imeAttrs);
    QApplication::sendEvent(&editor, &imeCaretPreeditAfterSup);
    QInputMethodEvent imePowNegCommit(QString(), imeAttrs);
    imePowNegCommit.setCommitString(QString(MathDsl::PowNeg));
    QApplication::sendEvent(&editor, &imePowNegCommit);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("s") + MathDsl::Pow2);
}

void TestEditorUi::keeps_scientific_notation_exponent_minus_unwrapped()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("1e"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Minus, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("1e") + QString(MathDsl::SubOp));

    QTest::keyClicks(&editor, QStringLiteral("2"));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("1e") + QString(MathDsl::SubOp) + QStringLiteral("2"));
}

void TestEditorUi::blocks_shift_operator_tail_after_non_plus_operator()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("foo -"));
    editor.setCursorPosition(editor.text().size());
    QKeyEvent lessByText(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QStringLiteral("<"));
    QApplication::sendEvent(&editor, &lessByText);
    QCOMPARE(editor.text(), QStringLiteral("foo ") + QString(MathDsl::SubOp));

    editor.setText(QStringLiteral("foo /"));
    editor.setCursorPosition(editor.text().size());
    QKeyEvent greaterByText(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QStringLiteral(">"));
    QApplication::sendEvent(&editor, &greaterByText);
    QCOMPARE(editor.text(), QStringLiteral("foo /"));
}

void TestEditorUi::rewrites_superscript_exponent_for_radix_and_inserts_mul_space_globally()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("2"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Space, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("2 "));

    editor.setText(QString::fromUtf8("pi⁴"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Comma, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("pi^(4,)"));
    QCOMPARE(editor.textCursor().position(),
             editor.document()->toRawText().size() - 1);

    editor.setText(QString::fromUtf8("pi⁴"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Period, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("pi^(4.)"));
    QCOMPARE(editor.textCursor().position(),
             editor.document()->toRawText().size() - 1);

    editor.setText(QString::fromUtf8("pi⁴"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Space, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QString::fromUtf8("pi⁴ "));

    editor.setText(QString::fromUtf8("2²"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Space, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QString::fromUtf8("2² "));

    editor.setText(QString::fromUtf8("pi⁴"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Asterisk, Qt::NoModifier);
    QVERIFY2(!editor.document()->toRawText().contains(QLatin1Char('^')),
             qPrintable(QStringLiteral("after first *: ") + editor.document()->toRawText()));
    QTest::keyClick(&editor, Qt::Key_Asterisk, Qt::NoModifier);
    QVERIFY2(!editor.document()->toRawText().contains(QLatin1Char('^')),
             qPrintable(editor.document()->toRawText()));
}

void TestEditorUi::typing_asterisk_after_superscript_power_keeps_explicit_multiplication()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    const QString multiply = QString(MathDsl::MulCrossWrapSp)
        + QString(MathDsl::MulCrossOp)
        + QString(MathDsl::MulCrossWrapSp);
    editor.setText(QStringLiteral("2")
                   + multiply
                   + QStringLiteral("3")
                   + multiply
                   + QStringLiteral("10")
                   + multiply
                   + QStringLiteral("10")
                   + QString(MathDsl::Pow2));
    editor.setCursorPosition(editor.text().size());

    QTest::keyClick(&editor, Qt::Key_Asterisk, Qt::NoModifier);

    QCOMPARE(editor.document()->toRawText(),
             QStringLiteral("2")
                 + multiply
                 + QStringLiteral("3")
                 + multiply
                 + QStringLiteral("10")
                 + multiply
                 + QStringLiteral("10")
                 + QString(MathDsl::Pow2)
                 + multiply);
    QVERIFY(!editor.document()->toRawText().contains(MathDsl::MulDotOp));
}

void TestEditorUi::auto_inserts_zero_before_dot_in_configured_contexts()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();
    const auto sendTextKey = [&editor](const QString& s) {
        QKeyEvent byText(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, s);
        QApplication::sendEvent(&editor, &byText);
    };

    editor.setText(QString());
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Period, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("0."));

    editor.setText(QString());
    editor.setCursorPosition(editor.text().size());
    sendTextKey(QStringLiteral("."));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("0."));

    editor.setText(QStringLiteral("1 + "));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Period, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("1 + 0."));

    editor.setText(QStringLiteral("1 + "));
    editor.setCursorPosition(editor.text().size());
    sendTextKey(QStringLiteral("."));
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("1 + 0."));

    editor.setText(QStringLiteral("   "));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Period, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("   0."));

    editor.setText(QStringLiteral("1 × "));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Period, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("1 × 0."));

    editor.setText(QStringLiteral("1 − "));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Period, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("1 − 0."));

    editor.setText(QStringLiteral("1 / "));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Period, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("1 / 0."));

    editor.setText(QStringLiteral(" (  ) "));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Period, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral(" (  ) × 0."));

    editor.setText(QStringLiteral("[x]   "));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Period, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("[x]   × 0."));

    editor.setText(QStringLiteral("pi"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Period, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("pi × 0."));

    editor.setText(QStringLiteral("pi  "));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Comma, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("pi  × 0,"));

    editor.setText(QStringLiteral("12"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Period, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("12."));

    editor.setText(QStringLiteral("0,"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Comma, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("0,,"));
}

void TestEditorUi::preserves_comment_text_during_input_data()
{
    QTest::addColumn<QString>("comment");
    QTest::newRow("multiplication") << QStringLiteral("2*2");
    QTest::newRow("expression-aliases")
        << QStringLiteral("pi sqrt(4) cbrt(8) summation(k) 2/3-1 'quoted' \"text\" Ω μ º 𝛑");
    QTest::newRow("units-and-conversions")
        << QStringLiteral("10000 [m * s^2] -> [km] 2*[m] ? pi");
    QTest::newRow("unfinished-syntax") << QStringLiteral("[sqrt(2^ + + ^^ --");
    QTest::newRow("spacing") << QStringLiteral("2*2  pi\t10000 [m/s]");
}

void TestEditorUi::preserves_comment_text_during_input()
{
    QFETCH(QString, comment);
    const QString prefix = QStringLiteral("examplevar = 2*2");
    const QString suffix = QStringLiteral(" ? ") + comment;
    const QString expression = prefix + suffix;
    Session session;
    QCOMPARE(session.evaluator()->autoFix(QStringLiteral("4") + suffix),
             QStringLiteral("4") + suffix);
    QCOMPARE(EditorUtils::normalizeExpressionOperators(expression),
             EditorUtils::normalizeExpressionOperators(prefix) + suffix);
    QCOMPARE(EditorUtils::normalizeExpressionOperatorsForEditorInput(expression),
             EditorUtils::normalizeExpressionOperatorsForEditorInput(prefix) + suffix);

    Editor editor;
    editor.setText(expression);
    QCOMPARE(editor.text(), EditorUtils::normalizeExpressionOperatorsForEditorInput(prefix) + suffix);

    editor.setText(QStringLiteral("4 ? "));
    editor.setCursorPosition(editor.text().size());
    editor.insert(comment);
    QCOMPARE(editor.text(), QStringLiteral("4 ? ") + comment);

    const QString oldClipboard = QApplication::clipboard()->text();
    const auto restoreClipboard = qScopeGuard([&]() {
        QApplication::clipboard()->setText(oldClipboard);
    });
    editor.setText(QStringLiteral("4 ? "));
    editor.setCursorPosition(editor.text().size());
    QApplication::clipboard()->setText(comment);
    editor.paste();
    QCOMPARE(editor.text(), QStringLiteral("4 ? ") + comment);

    editor.setText(QStringLiteral("4 ? "));
    editor.setCursorPosition(editor.text().size());
    QInputMethodEvent commit;
    commit.setCommitString(comment);
    QApplication::sendEvent(&editor, &commit);
    QCOMPARE(editor.text(), QStringLiteral("4 ? ") + comment);
}

void TestEditorUi::normalizes_only_expression_text_when_replacing_comment_selection()
{
    Editor editor;
    editor.setText(QStringLiteral("4 ? old"));
    QTextCursor cursor = editor.textCursor();
    cursor.setPosition(4);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    editor.setTextCursor(cursor);
    editor.insert(QStringLiteral("2*2"));
    QCOMPARE(editor.text(), QStringLiteral("4 ? 2*2"));

    cursor.setPosition(2);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    editor.setTextCursor(cursor);
    editor.insert(QStringLiteral("*2 ? 3*3"));
    QCOMPARE(editor.text(), QStringLiteral("4 ×2 ? 3*3"));
}

void TestEditorUi::allows_unrestricted_typing_inside_question_comment_context()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("1 ? pi"));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Period, Qt::NoModifier);
    QTest::keyClick(&editor, Qt::Key_Comma, Qt::NoModifier);
    QTest::keyClick(&editor, Qt::Key_Space, Qt::NoModifier);
    QTest::keyClick(&editor, Qt::Key_Space, Qt::NoModifier);

    QCOMPARE(editor.document()->toRawText(), QStringLiteral("1 ? pi.,  "));
}

void TestEditorUi::allows_currency_symbols_after_operators()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("$123 + "));
    editor.setCursorPosition(editor.text().size());
    QTest::keyClick(&editor, Qt::Key_Dollar, Qt::NoModifier);
    QCOMPARE(editor.document()->toRawText(), QStringLiteral("$123 + $"));
}

void TestEditorUi::blocks_dead_circumflex_key_after_existing_caret()
{
    // State: "2^".
    // Action: send dead-circumflex key press.
    // Expected: event is consumed; no duplicate caret.
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("2^"));
    editor.setCursorPosition(editor.text().size());

    QKeyEvent deadCircumflex(QEvent::KeyPress, Qt::Key_Dead_Circumflex, Qt::NoModifier);
    QApplication::sendEvent(&editor, &deadCircumflex);

    QCOMPARE(editor.text(), QStringLiteral("2^"));
}

void TestEditorUi::blocks_dead_circumflex_key_after_multiplication_operator()
{
    // State: "3 × ".
    // Action: send dead-circumflex key press.
    // Expected: event is consumed; no caret is inserted.
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("3")
                   + QString(MathDsl::MulCrossOp)
                   + QStringLiteral(" "));
    editor.setCursorPosition(editor.text().size());

    QKeyEvent deadCircumflex(QEvent::KeyPress, Qt::Key_Dead_Circumflex, Qt::NoModifier);
    QApplication::sendEvent(&editor, &deadCircumflex);

    QCOMPARE(editor.text(),
             QStringLiteral("3")
                 + QString(MathDsl::MulCrossOp)
                 + QStringLiteral(" "));
}

void TestEditorUi::blocks_regular_caret_after_multiplication_operator()
{
    // State: "3 × ".
    // Action: type regular caret '^'.
    // Expected: caret is ignored.
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("3")
                   + QString(MathDsl::MulCrossOp)
                   + QStringLiteral(" "));
    editor.setCursorPosition(editor.text().size());

    QTest::keyClicks(&editor, QStringLiteral("^"));

    QCOMPARE(editor.text(),
             QStringLiteral("3")
                 + QString(MathDsl::MulCrossOp)
                 + QStringLiteral(" "));
}

void TestEditorUi::blocks_ime_preedit_caret_echo_after_existing_caret()
{
    // State: "2^".
    // Action: send IME preedit caret echo.
    // Expected: preedit echo is ignored.
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("2^"));
    editor.setCursorPosition(editor.text().size());

    QList<QInputMethodEvent::Attribute> attributes;
    QInputMethodEvent imeEvent(QString::fromUtf8("ˆ"), attributes);
    QApplication::sendEvent(&editor, &imeEvent);

    QCOMPARE(editor.text(), QStringLiteral("2^"));
}

void TestEditorUi::blocks_ime_commit_caret_after_existing_caret()
{
    // State: "2^".
    // Action: send IME commit "^".
    // Expected: committed caret is ignored.
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("2^"));
    editor.setCursorPosition(editor.text().size());

    QList<QInputMethodEvent::Attribute> attributes;
    QInputMethodEvent imeEvent(QString(), attributes);
    imeEvent.setCommitString(QStringLiteral("^"));
    QApplication::sendEvent(&editor, &imeEvent);

    QCOMPARE(editor.text(), QStringLiteral("2^"));
}

void TestEditorUi::blocks_ime_commit_caret_after_multiplication_operator()
{
    // State: "3 × ".
    // Action: send IME commit "^".
    // Expected: committed caret is ignored.
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("3")
                   + QString(MathDsl::MulCrossOp)
                   + QStringLiteral(" "));
    editor.setCursorPosition(editor.text().size());

    QList<QInputMethodEvent::Attribute> attributes;
    QInputMethodEvent imeEvent(QString(), attributes);
    imeEvent.setCommitString(QStringLiteral("^"));
    QApplication::sendEvent(&editor, &imeEvent);

    QCOMPARE(editor.text(),
             QStringLiteral("3")
                 + QString(MathDsl::MulCrossOp)
                 + QStringLiteral(" "));
}

void TestEditorUi::finds_completion_keyword_after_superscript_power()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QString::fromUtf8("pi²c"));
    editor.setCursorPosition(editor.text().size());

    const QString keyword = editor.getKeyword();
    QVERIFY(!keyword.isEmpty());
    QVERIFY(keyword.startsWith(QLatin1Char('c')));
}

void TestEditorUi::completes_replacing_trailing_identifier_after_superscript_power()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QString::fromUtf8("pi²") + QString(MathDsl::MulDotOp) + QStringLiteral("c"));
    editor.setCursorPosition(editor.text().size());

    QVERIFY(QMetaObject::invokeMethod(
        &editor,
        "autoComplete",
        Qt::DirectConnection,
        Q_ARG(QString, QStringLiteral("cos: Built-in function"))));

    QCOMPARE(
        editor.text(),
        QString::fromUtf8("pi²")
            + QString(MathDsl::MulDotOp)
            + QStringLiteral("cos()"));
}

void TestEditorUi::completes_identifier_immediately_after_digit()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("3c"));
    editor.setCursorPosition(editor.text().size());

    QVERIFY(QMetaObject::invokeMethod(&editor, "triggerAutoComplete", Qt::DirectConnection));
    QTreeWidget* popup = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((popup = s_completionPopupTree()) != nullptr, 1000);
    QVERIFY(!s_popupSymbolForIdentifier(popup, QStringLiteral("cos")).isEmpty());
    popup->hide();

    QVERIFY(QMetaObject::invokeMethod(
        &editor,
        "autoComplete",
        Qt::DirectConnection,
        Q_ARG(QString, QStringLiteral("cos: Built-in function"))));

    QCOMPARE(editor.text(), QStringLiteral("3cos()"));
}

void TestEditorUi::completes_pi_identifier_as_pi_symbol()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("p"));
    editor.setCursorPosition(editor.text().size());

    QVERIFY(QMetaObject::invokeMethod(
        &editor,
        "autoComplete",
        Qt::DirectConnection,
        Q_ARG(QString, QStringLiteral("pi: Archimedes' constant Pi"))));

    QCOMPARE(editor.text(), QString::fromUtf8("π"));
}

void TestEditorUi::accepts_degree_alias_in_unit_brackets_and_normalizes_to_degree_sign()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("100 ["));
    editor.setCursorPosition(editor.text().size());

    QTest::keyClicks(&editor, QString::fromUtf8("ºC"));
    QCOMPARE(editor.text(), QString::fromUtf8("100 [°C"));

    // QTest::keyClicks() rejects U+02DA in qasciikey, so verify via setText().
    editor.setText(QString::fromUtf8("100 [˚C"));
    QCOMPARE(editor.text(), QString::fromUtf8("100 [°C"));
}

void TestEditorUi::offers_unit_completion_for_degree_symbol_in_unit_context()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QString::fromUtf8("100 [°"));
    editor.setCursorPosition(editor.text().size());

    const auto localizedChoice = [](const QString& identifier, UnitId id) {
        return QStringLiteral("%1:%2").arg(identifier, tr(unitLocalizedName(id)));
    };
    const QStringList degreeChoices = editor.matchFragment(QString::fromUtf8("°"), true);
    QVERIFY(!degreeChoices.isEmpty());
    QVERIFY(degreeChoices.contains(localizedChoice(QString::fromUtf8("°"), UnitId::Degree)));
    QVERIFY(degreeChoices.contains(localizedChoice(QString::fromUtf8("°C"), UnitId::DegreeCelsius)));

    QVERIFY(QMetaObject::invokeMethod(
        &editor,
        "autoComplete",
        Qt::DirectConnection,
        Q_ARG(QString, QStringLiteral("u:Unit"))));
    QCOMPARE(editor.text(), QString::fromUtf8("100 [u"));
}

void TestEditorUi::matches_micro_units_when_typing_u_in_unit_context()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    const auto localizedChoice = [](const QString& identifier) {
        return QStringLiteral("%1:%2").arg(
            identifier,
            tr(unitLocalizedIdentifierName(identifier).toUtf8().constData()));
    };
    const QStringList uChoices = editor.matchFragment(QStringLiteral("u"), true);
    QVERIFY(uChoices.contains(localizedChoice(QString::fromUtf8("µm"))));
    QVERIFY(uChoices.contains(localizedChoice(QString::fromUtf8("µs"))));
    QVERIFY(uChoices.contains(localizedChoice(QString::fromUtf8("µas"))));
    QVERIFY(!uChoices.contains(localizedChoice(QStringLiteral("uas"))));
}

void TestEditorUi::unit_context_completion_excludes_user_variables()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    Session session;
    Evaluator* evaluator = session.evaluator();
    editor.setSession(&session);
    evaluator->setVariable(QStringLiteral("foo"), Quantity(3));

    const QStringList unitChoices = editor.matchFragment(QStringLiteral("fo"), true);
    for (const QString& choice : unitChoices)
        QVERIFY2(!choice.startsWith(QStringLiteral("foo:")), qPrintable(choice));

    evaluator->unsetVariable(QStringLiteral("foo"));
}

void TestEditorUi::unit_context_completion_excludes_built_in_functions()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    const QStringList choices = editor.matchFragment(QStringLiteral("si"), true);
    for (const QString& choice : choices)
        QVERIFY2(!choice.startsWith(QStringLiteral("sin:")), qPrintable(choice));
}

void TestEditorUi::unit_context_completion_excludes_user_functions()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    Session session;
    Evaluator* evaluator = session.evaluator();
    editor.setSession(&session);
    evaluator->setUserFunction(UserFunction(
        QStringLiteral("foof"),
        QStringList() << QStringLiteral("x"),
        QStringLiteral("x")));

    const QStringList choices = editor.matchFragment(QStringLiteral("foo"), true);
    for (const QString& choice : choices)
        QVERIFY2(!choice.startsWith(QStringLiteral("foof:")), qPrintable(choice));

    evaluator->unsetUserFunction(QStringLiteral("foof"));
}

void TestEditorUi::unit_context_completion_excludes_built_in_variables()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    const QStringList ansChoices = editor.matchFragment(QStringLiteral("an"), true);
    const QStringList eChoices = editor.matchFragment(QStringLiteral("e"), true);
    const QStringList piChoices = editor.matchFragment(QStringLiteral("pi"), true);

    for (const QString& choice : ansChoices)
        QVERIFY2(!choice.startsWith(QStringLiteral("ans:")), qPrintable(choice));
    for (const QString& choice : eChoices)
        QVERIFY2(!choice.startsWith(QStringLiteral("e:")), qPrintable(choice));
    for (const QString& choice : piChoices)
        QVERIFY2(!choice.startsWith(QStringLiteral("pi:")), qPrintable(choice));
}

void TestEditorUi::unit_context_completion_includes_angle_units_and_long_forms()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    const auto localizedChoice = [](const QString& identifier, UnitId id) {
        return QStringLiteral("%1:%2").arg(identifier, tr(unitLocalizedName(id)));
    };
    const auto descriptionFor = [](const QStringList& choices, const QString& identifier) {
        for (const QString& choice : choices) {
            if (choice.startsWith(identifier + QStringLiteral(":")))
                return choice.mid(identifier.size() + 1);
        }
        return QString();
    };
    const QStringList radChoices = editor.matchFragment(QStringLiteral("rad"), true);
    QVERIFY(radChoices.contains(localizedChoice(QStringLiteral("rad"), UnitId::Radian)));
    QVERIFY(radChoices.contains(localizedChoice(QStringLiteral("radian"), UnitId::Radian)));

    const QStringList arcChoices = editor.matchFragment(QStringLiteral("arc"), true);
    QVERIFY(arcChoices.contains(localizedChoice(QStringLiteral("arcminute"), UnitId::Arcminute)));
    QVERIFY(arcChoices.contains(localizedChoice(QStringLiteral("arcsecond"), UnitId::Arcsecond)));
    QVERIFY(arcChoices.contains(localizedChoice(QStringLiteral("arcsec"), UnitId::Arcsecond)));

    const QStringList masChoices = editor.matchFragment(QStringLiteral("mas"), true);
    QVERIFY(masChoices.contains(localizedChoice(QStringLiteral("mas"), UnitId::Milliarcsecond)));

    const QStringList uasChoices = editor.matchFragment(QStringLiteral("u"), true);
    QVERIFY(uasChoices.contains(localizedChoice(QString::fromUtf8("µas"), UnitId::Microarcsecond)));

    const QStringList milliArcChoices = editor.matchFragment(QStringLiteral("milliarc"), true);
    QVERIFY(milliArcChoices.contains(localizedChoice(QStringLiteral("milliarcsecond"), UnitId::Milliarcsecond)));

    const QStringList microArcChoices = editor.matchFragment(QStringLiteral("microarc"), true);
    QVERIFY(microArcChoices.contains(localizedChoice(QStringLiteral("microarcsecond"), UnitId::Microarcsecond)));

    const QStringList turnChoices = editor.matchFragment(QStringLiteral("turn"), true);
    QVERIFY(turnChoices.contains(localizedChoice(QStringLiteral("turn"), UnitId::Turn)));

    const QStringList gradChoices = editor.matchFragment(QStringLiteral("grad"), true);
    QVERIFY(gradChoices.contains(localizedChoice(QStringLiteral("gradian"), UnitId::Gradian)));

    const QStringList gonChoices = editor.matchFragment(QStringLiteral("gon"), true);
    QVERIFY(gonChoices.contains(localizedChoice(QStringLiteral("gon"), UnitId::Gradian)));

    const QStringList aChoices = editor.matchFragment(QStringLiteral("a"), true);
    const QString acDescription = descriptionFor(aChoices, QStringLiteral("ac"));
    const QString aCDescription = descriptionFor(aChoices, QStringLiteral("aC"));
    QVERIFY(!acDescription.isEmpty());
    QVERIFY(!aCDescription.isEmpty());
    QCOMPARE(acDescription, QStringLiteral("acre"));
    QVERIFY(aCDescription != QStringLiteral("acre"));
    QVERIFY(aCDescription != tr("Unit"));
}

void TestEditorUi::unit_context_completion_includes_small_positive_si_prefixed_symbols()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    const auto hasCompletionIdentifier = [](const QStringList& choices, const QString& identifier) {
        for (const QString& choice : choices) {
            if (choice.startsWith(identifier + QStringLiteral(":")))
                return true;
        }
        return false;
    };

    const QStringList choices = editor.matchFragment(QStringLiteral("da"), true);
    QVERIFY(hasCompletionIdentifier(choices, QStringLiteral("dag")));
    QVERIFY(hasCompletionIdentifier(choices, QStringLiteral("daL")));
    QVERIFY(hasCompletionIdentifier(choices, QStringLiteral("dam")));
    QVERIFY(hasCompletionIdentifier(choices, QStringLiteral("daN")));
    QVERIFY(hasCompletionIdentifier(choices, QStringLiteral("daPa")));
}

void TestEditorUi::unit_context_completion_excludes_prefixed_square_and_cubic_metre()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    const auto hasCompletionIdentifier = [](const QStringList& choices, const QString& identifier) {
        for (const QString& choice : choices) {
            if (choice.startsWith(identifier + QStringLiteral(":")))
                return true;
        }
        return false;
    };

    const QStringList choices = editor.matchFragment(QStringLiteral("deci"), true);
    QVERIFY(!hasCompletionIdentifier(choices, QStringLiteral("decisquare_metre")));
    QVERIFY(!hasCompletionIdentifier(choices, QStringLiteral("decicubic_metre")));
}

void TestEditorUi::completes_binary_prefixed_information_unit_to_short_form_in_unit_context()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("1 [meb"));
    editor.setCursorPosition(editor.text().size());

    const QStringList mebChoices = editor.matchFragment(QStringLiteral("meb"), true);
    const QString mebibyteChoice = QStringLiteral("%1:%2").arg(
        QStringLiteral("mebibyte"),
        tr(unitLocalizedIdentifierName(QStringLiteral("mebibyte")).toUtf8().constData()));
    QVERIFY(mebChoices.contains(mebibyteChoice));

    QVERIFY(QMetaObject::invokeMethod(
        &editor,
        "autoComplete",
        Qt::DirectConnection,
        Q_ARG(QString, QStringLiteral("mebibyte:Unit"))));
    QCOMPARE(editor.text(), QStringLiteral("1 [MiB"));
}

void TestEditorUi::completes_day_unit_to_short_form_in_unit_context()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("1 [d"));
    editor.setCursorPosition(editor.text().size());
    QVERIFY(QMetaObject::invokeMethod(
        &editor,
        "autoComplete",
        Qt::DirectConnection,
        Q_ARG(QString, QStringLiteral("day:Unit"))));
    QCOMPARE(editor.text(), QStringLiteral("1 [d"));
}

void TestEditorUi::completes_hour_unit_to_short_form_in_unit_context()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("1 [h"));
    editor.setCursorPosition(editor.text().size());
    QVERIFY(QMetaObject::invokeMethod(
        &editor,
        "autoComplete",
        Qt::DirectConnection,
        Q_ARG(QString, QStringLiteral("hour:Unit"))));
    QCOMPARE(editor.text(), QStringLiteral("1 [h"));
}

void TestEditorUi::completes_affine_temperature_units_in_unit_context()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("100 [c"));
    editor.setCursorPosition(editor.text().size());
    QVERIFY(QMetaObject::invokeMethod(
        &editor,
        "autoComplete",
        Qt::DirectConnection,
        Q_ARG(QString, QStringLiteral("degree_celsius:Unit"))));
    QCOMPARE(editor.text(), QString::fromUtf8("100 [°C"));

    editor.setText(QStringLiteral("100 [f"));
    editor.setCursorPosition(editor.text().size());
    QVERIFY(QMetaObject::invokeMethod(
        &editor,
        "autoComplete",
        Qt::DirectConnection,
        Q_ARG(QString, QStringLiteral("degree_fahrenheit:Unit"))));
    QCOMPARE(editor.text(), QString::fromUtf8("100 [°F"));

    editor.setText(QString::fromUtf8("100 [º"));
    editor.setCursorPosition(editor.text().size());
    QVERIFY(QMetaObject::invokeMethod(
        &editor,
        "autoComplete",
        Qt::DirectConnection,
        Q_ARG(QString, QString::fromUtf8("ºC:Unit"))));
    QCOMPARE(editor.text(), QString::fromUtf8("100 [°C"));

    editor.setText(QString::fromUtf8("100 [˚"));
    editor.setCursorPosition(editor.text().size());
    QVERIFY(QMetaObject::invokeMethod(
        &editor,
        "autoComplete",
        Qt::DirectConnection,
        Q_ARG(QString, QString::fromUtf8("˚C:Unit"))));
    QCOMPARE(editor.text(), QString::fromUtf8("100 [°C"));

    editor.setText(QString::fromUtf8("100 [˚"));
    editor.setCursorPosition(editor.text().size());
    QVERIFY(QMetaObject::invokeMethod(
        &editor,
        "autoComplete",
        Qt::DirectConnection,
        Q_ARG(QString, QString::fromUtf8("˚F:Unit"))));
    QCOMPARE(editor.text(), QString::fromUtf8("100 [°F"));

    editor.setText(QStringLiteral("100 [deg"));
    editor.setCursorPosition(editor.text().size());
    QVERIFY(QMetaObject::invokeMethod(
        &editor,
        "autoComplete",
        Qt::DirectConnection,
        Q_ARG(QString, QStringLiteral("degC:Unit"))));
    QCOMPARE(editor.text(), QString::fromUtf8("100 [°C"));

    editor.setText(QStringLiteral("100 [ce"));
    editor.setCursorPosition(editor.text().size());
    QVERIFY(QMetaObject::invokeMethod(
        &editor,
        "autoComplete",
        Qt::DirectConnection,
        Q_ARG(QString, QStringLiteral("Cel:Unit"))));
    QCOMPARE(editor.text(), QString::fromUtf8("100 [°C"));

    editor.setText(QStringLiteral("100 [deg"));
    editor.setCursorPosition(editor.text().size());
    QVERIFY(QMetaObject::invokeMethod(
        &editor,
        "autoComplete",
        Qt::DirectConnection,
        Q_ARG(QString, QStringLiteral("degF:Unit"))));
    QCOMPARE(editor.text(), QString::fromUtf8("100 [°F"));

    editor.setText(QStringLiteral("100 [fa"));
    editor.setCursorPosition(editor.text().size());
    QVERIFY(QMetaObject::invokeMethod(
        &editor,
        "autoComplete",
        Qt::DirectConnection,
        Q_ARG(QString, QStringLiteral("Fah:Unit"))));
    QCOMPARE(editor.text(), QString::fromUtf8("100 [°F"));
}

void TestEditorUi::completes_arc_units_in_unit_context()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("1 [arc"));
    editor.setCursorPosition(editor.text().size());
    QVERIFY(QMetaObject::invokeMethod(
        &editor,
        "autoComplete",
        Qt::DirectConnection,
        Q_ARG(QString, QStringLiteral("arcminute:Unit"))));
    QVERIFY(editor.text() != QStringLiteral("1 [arc"));

    editor.setText(QStringLiteral("1 [arc"));
    editor.setCursorPosition(editor.text().size());
    QVERIFY(QMetaObject::invokeMethod(
        &editor,
        "autoComplete",
        Qt::DirectConnection,
        Q_ARG(QString, QStringLiteral("arcsecond:Unit"))));
    QVERIFY(editor.text() != QStringLiteral("1 [arc"));
}

void TestEditorUi::tooltip_does_not_duplicate_degree_symbol_for_explicit_angle_conversion()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    Settings* settings = Settings::instance();
    const char oldAngleUnit = settings->angleUnit;
    const char oldResultFormat = settings->resultFormat;
    settings->angleUnit = 'd';
    settings->resultFormat = 'f';
    Evaluator::instance()->initializeAngleUnits();

    QSignalSpy spy(&editor, SIGNAL(autoCalcMessageAvailable(const QString&)));

    editor.setText(QStringLiteral("1 [rev] -> [°]"));
    editor.setCursorPosition(editor.text().size());
    editor.refreshAutoCalc();
    QCoreApplication::processEvents();

    QVERIFY(!spy.isEmpty());
    const QString message = spy.takeLast().at(0).toString();
    QCOMPARE(message, QString::fromUtf8("Current result:<br/>360°"));

    settings->angleUnit = oldAngleUnit;
    settings->resultFormat = oldResultFormat;
    Evaluator::instance()->initializeAngleUnits();
}

void TestEditorUi::tooltip_does_not_append_angle_mode_symbol_after_explicit_arcsecond_unit()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    Settings* settings = Settings::instance();
    const char oldAngleUnit = settings->angleUnit;
    const char oldResultFormat = settings->resultFormat;
    settings->angleUnit = 'd';
    settings->resultFormat = 'f';
    Evaluator::instance()->initializeAngleUnits();

    QSignalSpy spy(&editor, SIGNAL(autoCalcMessageAvailable(const QString&)));

    editor.setText(QStringLiteral("1 [rev] -> [arcsec]"));
    editor.setCursorPosition(editor.text().size());
    editor.refreshAutoCalc();
    QCoreApplication::processEvents();

    QVERIFY(!spy.isEmpty());
    const QString message = spy.takeLast().at(0).toString();
    QVERIFY(message.contains(QString(UnicodeChars::DoublePrime)));
    QVERIFY(!message.contains(QString(UnicodeChars::DoublePrime) + UnicodeChars::DegreeSign));

    settings->angleUnit = oldAngleUnit;
    settings->resultFormat = oldResultFormat;
    Evaluator::instance()->initializeAngleUnits();
}

void TestEditorUi::tooltip_formats_arcminute_and_arcsecond_results_without_expression()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    Settings* settings = Settings::instance();
    const char oldAngleUnit = settings->angleUnit;
    const char oldResultFormat = settings->resultFormat;
    settings->angleUnit = 'r';
    settings->resultFormat = 'f';
    Evaluator::instance()->initializeAngleUnits();

    QSignalSpy spy(&editor, SIGNAL(autoCalcMessageAvailable(const QString&)));

    editor.setText(QStringLiteral("1 [arcmin]"));
    editor.setCursorPosition(editor.text().size());
    editor.refreshAutoCalc();
    QCoreApplication::processEvents();

    QVERIFY(!spy.isEmpty());
    const QString arcminuteMessage = spy.takeLast().at(0).toString();
    QVERIFY2(arcminuteMessage.contains(QString(MathDsl::QuantSp) + Units::angleModeUnitSymbol('r')),
             qPrintable(QStringLiteral("Arcminute tooltip: %1").arg(arcminuteMessage)));
    QVERIFY(!arcminuteMessage.contains(UnicodeChars::Prime));
    QVERIFY(!arcminuteMessage.contains(QLatin1Char('=')));

    editor.setText(QStringLiteral("1 [arcsec]"));
    editor.setCursorPosition(editor.text().size());
    editor.refreshAutoCalc();
    QCoreApplication::processEvents();

    QVERIFY(!spy.isEmpty());
    const QString arcsecondMessage = spy.takeLast().at(0).toString();
    QVERIFY2(arcsecondMessage.contains(QString(MathDsl::QuantSp) + Units::angleModeUnitSymbol('r')),
             qPrintable(QStringLiteral("Arcsecond tooltip: %1").arg(arcsecondMessage)));
    QVERIFY(!arcsecondMessage.contains(UnicodeChars::DoublePrime));
    QVERIFY(!arcsecondMessage.contains(QLatin1Char('=')));

    settings->angleUnit = oldAngleUnit;
    settings->resultFormat = oldResultFormat;
    Evaluator::instance()->initializeAngleUnits();
}

void TestEditorUi::tooltip_rewrites_composite_canonical_angle_symbols_to_aliases()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    Settings* settings = Settings::instance();
    const char oldAngleUnit = settings->angleUnit;
    const char oldResultFormat = settings->resultFormat;
    settings->angleUnit = 'r';
    settings->resultFormat = 'f';
    Evaluator::instance()->initializeAngleUnits();

    QSignalSpy spy(&editor, SIGNAL(autoCalcMessageAvailable(const QString&)));

    editor.setText(QString::fromUtf8("1 [rad/m] -> [″/m]"));
    editor.setCursorPosition(editor.text().size());
    editor.refreshAutoCalc();
    QCoreApplication::processEvents();
    QVERIFY(!spy.isEmpty());
    const QString arcsecondMessage = spy.takeLast().at(0).toString();
    QVERIFY2(arcsecondMessage.contains(Units::arcsecondAliasSymbol()),
             qPrintable(QStringLiteral("Arcsecond composite tooltip: %1").arg(arcsecondMessage)));
    QVERIFY2(!arcsecondMessage.contains(QString::fromUtf8("″/m")),
             qPrintable(QStringLiteral("Arcsecond composite tooltip: %1").arg(arcsecondMessage)));

    editor.setText(QString::fromUtf8("1 [rad/m] -> [′/m]"));
    editor.setCursorPosition(editor.text().size());
    editor.refreshAutoCalc();
    QCoreApplication::processEvents();
    QVERIFY(!spy.isEmpty());
    const QString arcminuteMessage = spy.takeLast().at(0).toString();
    QVERIFY2(arcminuteMessage.contains(Units::arcminuteAliasSymbol()),
             qPrintable(QStringLiteral("Arcminute composite tooltip: %1").arg(arcminuteMessage)));
    QVERIFY2(!arcminuteMessage.contains(QString::fromUtf8("′/m")),
             qPrintable(QStringLiteral("Arcminute composite tooltip: %1").arg(arcminuteMessage)));

    editor.setText(QString::fromUtf8("1 [rad/m] -> [°/m]"));
    editor.setCursorPosition(editor.text().size());
    editor.refreshAutoCalc();
    QCoreApplication::processEvents();
    QVERIFY(!spy.isEmpty());
    const QString degreeMessage = spy.takeLast().at(0).toString();
    QVERIFY2(degreeMessage.contains(Units::degreeAliasSymbol()),
             qPrintable(QStringLiteral("Degree composite tooltip: %1").arg(degreeMessage)));
    QVERIFY2(!degreeMessage.contains(QString::fromUtf8("°/m")),
             qPrintable(QStringLiteral("Degree composite tooltip: %1").arg(degreeMessage)));

    settings->angleUnit = oldAngleUnit;
    settings->resultFormat = oldResultFormat;
    Evaluator::instance()->initializeAngleUnits();
}

void TestEditorUi::tooltip_shows_radian_suffix_for_negative_sexagesimal_literal()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    Settings* settings = Settings::instance();
    const char oldAngleUnit = settings->angleUnit;
    const char oldResultFormat = settings->resultFormat;
    settings->angleUnit = 'r';
    settings->resultFormat = 'f';
    Evaluator::instance()->initializeAngleUnits();

    QSignalSpy spy(&editor, SIGNAL(autoCalcMessageAvailable(const QString&)));

    editor.setText(QString::fromUtf8("−57°17′44.80624709635515647336″"));
    editor.setCursorPosition(editor.text().size());
    editor.refreshAutoCalc();
    QCoreApplication::processEvents();

    QVERIFY(!spy.isEmpty());
    const QString message = spy.takeLast().at(0).toString();
    QVERIFY(message.contains(QString(MathDsl::QuantSp) + Units::angleModeUnitSymbol('r')));

    settings->angleUnit = oldAngleUnit;
    settings->resultFormat = oldResultFormat;
    Evaluator::instance()->initializeAngleUnits();
}

void TestEditorUi::tooltip_trig_output_does_not_append_angle_mode_suffix()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    Settings* settings = Settings::instance();
    const char oldAngleUnit = settings->angleUnit;
    const char oldResultFormat = settings->resultFormat;
    settings->resultFormat = 'f';

    struct Case {
        char angleUnit;
        QString forbiddenSuffix;
    };
    const QList<Case> cases = {
        {'r', QString(MathDsl::QuantSp) + Units::angleModeUnitSymbol('r')},
        {'d', Units::angleModeUnitSymbol('d')}
    };

    for (const Case& c : cases) {
        settings->angleUnit = c.angleUnit;
        Evaluator::instance()->initializeAngleUnits();

        QSignalSpy spy(&editor, SIGNAL(autoCalcMessageAvailable(const QString&)));

        editor.setText(QString::fromUtf8("cos(180°)"));
        editor.setCursorPosition(editor.text().size());
        editor.refreshAutoCalc();
        QCoreApplication::processEvents();

        QVERIFY(!spy.isEmpty());
        const QString message = spy.takeLast().at(0).toString();
        QString normalizedMessage = message;
        normalizedMessage.replace(QString(MathDsl::SubOp), QString(MathDsl::SubOpAl1));
        QCOMPARE(normalizedMessage, QStringLiteral("Current result:<br/>-1"));
        QVERIFY2(!message.contains(QStringLiteral("cos(")),
                 qPrintable(QStringLiteral("Expected scalar trig result, got: %1").arg(message)));
        const QStringList lines = normalizedMessage.split(QStringLiteral("<br/>"));
        QVERIFY2(!lines.isEmpty(),
                 qPrintable(QStringLiteral("Unexpected empty tooltip message: %1").arg(message)));
        const QString resultLine = lines.last();
        QVERIFY2(!resultLine.contains(c.forbiddenSuffix),
                 qPrintable(QStringLiteral("Unexpected angle suffix in tooltip result line: %1").arg(message)));
        QVERIFY(!resultLine.contains(QString(MathDsl::UnitStart)));
        QVERIFY(!resultLine.contains(QString(MathDsl::UnitEnd)));
    }

    settings->angleUnit = oldAngleUnit;
    settings->resultFormat = oldResultFormat;
    Evaluator::instance()->initializeAngleUnits();
}

void TestEditorUi::tooltip_shows_only_result_for_non_trig_sexagesimal_expression()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    Settings* settings = Settings::instance();
    const char oldAngleUnit = settings->angleUnit;
    const char oldResultFormat = settings->resultFormat;
    settings->angleUnit = 'd';
    settings->resultFormat = 'f';
    Evaluator::instance()->initializeAngleUnits();

    QSignalSpy spy(&editor, SIGNAL(autoCalcMessageAvailable(const QString&)));

    const QString expression =
        QString::fromUtf8("round(2.8°) + round(2.8°) + round(2.8°)");
    editor.setText(expression);
    editor.setCursorPosition(editor.text().size());
    editor.refreshAutoCalc();
    QCoreApplication::processEvents();

    QVERIFY(!spy.isEmpty());
    const QString message = spy.takeLast().at(0).toString();
    QCOMPARE(message, QString::fromUtf8("Current result:<br/>9°"));

    settings->angleUnit = oldAngleUnit;
    settings->resultFormat = oldResultFormat;
    Evaluator::instance()->initializeAngleUnits();
}

void TestEditorUi::tooltip_shows_only_result_for_repeated_trig_with_degree_sign()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    Settings* settings = Settings::instance();
    const char oldAngleUnit = settings->angleUnit;
    const char oldResultFormat = settings->resultFormat;
    const bool oldSimplify = settings->simplifyResultExpressions;
    settings->angleUnit = 'd';
    settings->resultFormat = 'f';
    settings->simplifyResultExpressions = true;
    Evaluator::instance()->initializeAngleUnits();

    QSignalSpy spy(&editor, SIGNAL(autoCalcMessageAvailable(const QString&)));

    const QString expression = QString::fromUtf8(
        "cos(180°) + cos(180°) + cos(180°) + cos(180°)");
    editor.setText(expression);
    editor.setCursorPosition(editor.text().size());
    editor.refreshAutoCalc();
    QCoreApplication::processEvents();

    QVERIFY(!spy.isEmpty());
    const QString message = spy.takeLast().at(0).toString();
    QString normalizedMessage = message;
    normalizedMessage.replace(QString(MathDsl::SubOp), QString(MathDsl::SubOpAl1));
    QCOMPARE(normalizedMessage, QStringLiteral("Current result:<br/>-4"));

    settings->angleUnit = oldAngleUnit;
    settings->resultFormat = oldResultFormat;
    settings->simplifyResultExpressions = oldSimplify;
    Evaluator::instance()->initializeAngleUnits();
}

void TestEditorUi::tooltip_shows_only_result_for_mixed_revolution_aliases()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    Settings* settings = Settings::instance();
    const char oldAngleUnit = settings->angleUnit;
    const char oldResultFormat = settings->resultFormat;
    const bool oldSimplify = settings->simplifyResultExpressions;
    settings->angleUnit = 'd';
    settings->resultFormat = 'f';
    settings->simplifyResultExpressions = true;
    Evaluator::instance()->initializeAngleUnits();

    QSignalSpy spy(&editor, SIGNAL(autoCalcMessageAvailable(const QString&)));

    const QString expression = QString::fromUtf8(
        "cos(180[rev]) + cos(180[rev]) + cos(180[rev]) + cos(180 [revolution])");
    editor.setText(expression);
    editor.setCursorPosition(editor.text().size());
    editor.refreshAutoCalc();
    QCoreApplication::processEvents();

    QVERIFY(!spy.isEmpty());
    const QString message = spy.takeLast().at(0).toString();
    QCOMPARE(message, QStringLiteral("Current result:<br/>4"));

    settings->angleUnit = oldAngleUnit;
    settings->resultFormat = oldResultFormat;
    settings->simplifyResultExpressions = oldSimplify;
    Evaluator::instance()->initializeAngleUnits();
}

void TestEditorUi::tooltip_shows_only_result_for_literal_number_products()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    Settings* settings = Settings::instance();
    const char oldResultFormat = settings->resultFormat;
    const bool oldSimplify = settings->simplifyResultExpressions;
    settings->resultFormat = 'f';
    settings->simplifyResultExpressions = true;

    QSignalSpy spy(&editor, SIGNAL(autoCalcMessageAvailable(const QString&)));

    editor.setText(QStringLiteral("pi*2^(-2)*pi+3*4*3"));
    editor.setCursorPosition(editor.text().size());
    editor.refreshAutoCalc();
    QCoreApplication::processEvents();

    QVERIFY(!spy.isEmpty());
    const QString message = spy.takeLast().at(0).toString();
    const QStringList lines = message.split(QStringLiteral("<br/>"));
    QCOMPARE(lines.size(), 2);
    QCOMPARE(lines.first(), QStringLiteral("Current result:"));
    QVERIFY(lines.last().startsWith(QStringLiteral("38.467")));
    QVERIFY(!message.contains(QLatin1Char('=')));

    QSignalSpy interpretedSpy(&editor, SIGNAL(autoCalcMessageAvailable(const QString&)));
    editor.setText(QStringLiteral("pi*2^(-2)*pi+3^2*4"));
    editor.setCursorPosition(editor.text().size());
    editor.refreshAutoCalc();
    QCoreApplication::processEvents();

    QVERIFY(!interpretedSpy.isEmpty());
    const QString interpretedMessage = interpretedSpy.takeLast().at(0).toString();
    QCOMPARE(interpretedMessage, message);

    settings->resultFormat = oldResultFormat;
    settings->simplifyResultExpressions = oldSimplify;
}

void TestEditorUi::tooltip_shows_only_result_data()
{
    QTest::addColumn<bool>("selection");
    QTest::addColumn<bool>("simplify");
    QTest::addColumn<bool>("extraResult");
    QTest::addColumn<QString>("expression");
    QTest::addColumn<QString>("expectedResult");

    for (bool selection : {false, true}) {
        for (bool simplify : {false, true}) {
            const QString prefix = QStringLiteral("%1-%2")
                .arg(selection ? QStringLiteral("selection") : QStringLiteral("current"),
                     simplify ? QStringLiteral("simplify") : QStringLiteral("plain"));
            QTest::newRow(qPrintable(prefix + QStringLiteral("-complete")))
                << selection << simplify << false << QStringLiteral("1+24") << QStringLiteral("25");
            QTest::newRow(qPrintable(prefix + QStringLiteral("-incomplete")))
                << selection << simplify << false << QStringLiteral("1+24+") << QStringLiteral("25");
            QTest::newRow(qPrintable(prefix + QStringLiteral("-extra-result")))
                << selection << simplify << true << QStringLiteral("1+24")
                << QString::fromUtf8("25<br/>2.5 × 10¹");
        }
    }
}

void TestEditorUi::tooltip_shows_only_result()
{
    QFETCH(bool, selection);
    QFETCH(bool, simplify);
    QFETCH(bool, extraResult);
    QFETCH(QString, expression);
    QFETCH(QString, expectedResult);

    Settings* settings = Settings::instance();
    settings->resultFormat = 'f';
    settings->resultPrecision = -1;
    settings->simplifyResultExpressions = simplify;
    settings->multipleResultLinesEnabled = extraResult;
    settings->secondaryResultEnabled = extraResult;
    settings->alternativeResultFormat = 'e';
    settings->secondaryResultPrecision = -1;

    Editor editor;
    editor.setText(expression);
    QSignalSpy spy(&editor, SIGNAL(autoCalcMessageAvailable(const QString&)));
    if (selection)
        editor.autoCalcSelection(expression);
    else
        editor.refreshAutoCalc();

    QVERIFY(!spy.isEmpty());
    const QString message = spy.takeLast().at(0).toString();
    const QString title = selection ? QStringLiteral("Selection result:")
                                    : QStringLiteral("Current result:");
    QCOMPARE(message, title + QStringLiteral("<br/>") + expectedResult);
}

void TestEditorUi::tooltip_keeps_quantsp_before_degree_celsius()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    Settings* settings = Settings::instance();
    const char oldResultFormat = settings->resultFormat;
    settings->resultFormat = 'f';

    QSignalSpy spy(&editor, SIGNAL(autoCalcMessageAvailable(const QString&)));

    editor.setText(QString::fromUtf8("77 [°F] -> [°C]"));
    editor.setCursorPosition(editor.text().size());
    editor.refreshAutoCalc();
    QCoreApplication::processEvents();

    QVERIFY(!spy.isEmpty());
    const QString message = spy.takeLast().at(0).toString();
    const QString expected = QStringLiteral("Current result:<br/>25")
        + QString(MathDsl::QuantSp) + QString::fromUtf8("°C");
    QCOMPARE(message, expected);

    settings->resultFormat = oldResultFormat;
}

void TestEditorUi::tooltip_handles_affine_temperature_units_without_arrow_and_with_conversion()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    Settings* settings = Settings::instance();
    const char oldResultFormat = settings->resultFormat;
    settings->resultFormat = 'f';

    struct Case {
        QString expression;
        QString expected;
    };
    const QList<Case> cases = {
        {QString::fromUtf8("100 [°C]"),
         QStringLiteral("100") + QString(MathDsl::QuantSp) + QString::fromUtf8("°C")},
        {QString::fromUtf8("203 [°F]"),
         QStringLiteral("203") + QString(MathDsl::QuantSp) + QString::fromUtf8("°F")},
        {QString::fromUtf8("1 [K] -> [°C]"),
         QString(MathDsl::SubOp)
             + QStringLiteral("272.15")
             + QString(MathDsl::QuantSp)
             + QString::fromUtf8("°C")},
        {QString::fromUtf8("1 [K] -> [°F]"),
         QString(MathDsl::SubOp)
             + QStringLiteral("457.87")
             + QString(MathDsl::QuantSp)
             + QString::fromUtf8("°F")}
    };

    for (const Case& c : cases) {
        QSignalSpy spy(&editor, SIGNAL(autoCalcMessageAvailable(const QString&)));
        editor.setText(c.expression);
        editor.setCursorPosition(editor.text().size());
        editor.refreshAutoCalc();
        QCoreApplication::processEvents();

        QVERIFY(!spy.isEmpty());
        const QString message = spy.takeLast().at(0).toString();
        QCOMPARE(message, QStringLiteral("Current result:<br/>") + c.expected);
    }

    settings->resultFormat = oldResultFormat;
}

void TestEditorUi::tooltip_shows_selection_result_when_selecting_with_shift_arrows()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    QSignalSpy spy(&editor, SIGNAL(autoCalcMessageAvailable(const QString&)));

    editor.setText(QStringLiteral("1+24"));
    editor.setCursorPosition(editor.text().size());
    QCoreApplication::processEvents();

    QTest::keyClick(&editor, Qt::Key_Left, Qt::ShiftModifier);
    QCoreApplication::processEvents();

    QVERIFY(!spy.isEmpty());
    const QString message = spy.takeLast().at(0).toString();
    QVERIFY2(message.contains(QStringLiteral("Selection result:")),
             qPrintable(QStringLiteral("Expected selection result message, got: %1").arg(message)));
    QCOMPARE(message, QStringLiteral("Selection result:<br/>4"));
}

void TestEditorUi::tooltip_shows_selection_result_when_selecting_all_with_keyboard_shortcut()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    QSignalSpy spy(&editor, SIGNAL(autoCalcMessageAvailable(const QString&)));

    editor.setText(QStringLiteral("1+24"));
    editor.setCursorPosition(editor.text().size());
    QCoreApplication::processEvents();
    spy.clear();

    const QList<QKeySequence> bindings = QKeySequence::keyBindings(QKeySequence::SelectAll);
    QVERIFY(!bindings.isEmpty());
    const QKeyCombination selectAll = bindings.first()[0];
    QTest::keyClick(&editor, selectAll.key(), selectAll.keyboardModifiers());
    QCoreApplication::processEvents();

    QVERIFY2(editor.textCursor().hasSelection(),
             qPrintable(QStringLiteral("Expected select-all selection, got cursor position %1")
                            .arg(editor.textCursor().position())));
    QVERIFY(!spy.isEmpty());
    const QString message = spy.takeLast().at(0).toString();
    QVERIFY2(message.contains(QStringLiteral("Selection result:")),
             qPrintable(QStringLiteral("Expected selection result message, got: %1").arg(message)));
    QCOMPARE(message, QStringLiteral("Selection result:<br/>25"));
}

void TestEditorUi::tooltip_shows_selection_result_when_selecting_with_mouse()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    QSignalSpy spy(&editor, SIGNAL(autoCalcMessageAvailable(const QString&)));

    editor.setText(QStringLiteral("1+24"));
    editor.setCursorPosition(editor.text().size());
    QCoreApplication::processEvents();
    spy.clear();

    QTextCursor cursor = editor.textCursor();
    cursor.setPosition(3);
    const QPoint selectionStart = editor.cursorRect(cursor).center();
    cursor.setPosition(4);
    const QPoint selectionEnd = editor.cursorRect(cursor).center();

    QTest::mousePress(editor.viewport(), Qt::LeftButton, Qt::NoModifier, selectionStart);
    QTest::mouseMove(editor.viewport(), selectionEnd);
    QCoreApplication::processEvents();
    QVERIFY(spy.isEmpty());

    QTest::mouseRelease(editor.viewport(), Qt::LeftButton, Qt::NoModifier, selectionEnd);
    QCoreApplication::processEvents();

    QVERIFY2(editor.textCursor().hasSelection(),
             qPrintable(QStringLiteral("Expected mouse selection, got cursor position %1")
                            .arg(editor.textCursor().position())));
    QVERIFY(!spy.isEmpty());
    const QString message = spy.takeLast().at(0).toString();
    QVERIFY2(message.contains(QStringLiteral("Selection result:")),
             qPrintable(QStringLiteral("Expected selection result message, got: %1").arg(message)));
    QCOMPARE(message, QStringLiteral("Selection result:<br/>4"));
}

void TestEditorUi::tooltip_does_not_refresh_current_result_on_caret_arrow_move()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    QSignalSpy spy(&editor, SIGNAL(autoCalcMessageAvailable(const QString&)));

    editor.setText(QStringLiteral("1+24"));
    editor.setCursorPosition(editor.text().size());
    QCoreApplication::processEvents();
    spy.clear();

    QTest::keyClick(&editor, Qt::Key_Left);
    QCoreApplication::processEvents();

    QVERIFY2(spy.isEmpty(),
             qPrintable(QStringLiteral("Caret movement should not refresh the result tooltip, got: %1")
                            .arg(spy.isEmpty() ? QString() : spy.takeLast().at(0).toString())));
}

void TestEditorUi::tooltip_does_not_refresh_current_result_on_mouse_caret_reposition()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    QSignalSpy spy(&editor, SIGNAL(autoCalcMessageAvailable(const QString&)));

    editor.setText(QStringLiteral("1+24"));
    editor.setCursorPosition(editor.text().size());
    QCoreApplication::processEvents();
    spy.clear();

    QTextCursor cursor = editor.textCursor();
    cursor.setPosition(1);
    const QPoint clickPosition = editor.cursorRect(cursor).center();
    QTest::mouseClick(editor.viewport(), Qt::LeftButton, Qt::NoModifier, clickPosition);
    QCoreApplication::processEvents();

    QVERIFY2(spy.isEmpty(),
             qPrintable(QStringLiteral("Mouse caret movement should not refresh the result tooltip, got: %1")
                            .arg(spy.isEmpty() ? QString() : spy.takeLast().at(0).toString())));
}

void TestEditorUi::tooltip_refreshes_current_result_on_char_deletion()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    QSignalSpy spy(&editor, SIGNAL(autoCalcMessageAvailable(const QString&)));

    editor.setText(QStringLiteral("1+24"));
    editor.setCursorPosition(editor.text().size());
    QCoreApplication::processEvents();
    spy.clear();

    QTest::keyClick(&editor, Qt::Key_Backspace);
    QCoreApplication::processEvents();

    QVERIFY(!spy.isEmpty());
    const QString message = spy.takeLast().at(0).toString();
    QVERIFY2(message.contains(QStringLiteral("Current result:")),
             qPrintable(QStringLiteral("Expected current result message after deletion, got: %1").arg(message)));
    QCOMPARE(message, QStringLiteral("Current result:<br/>3"));
}

void TestEditorUi::enter_evaluates_when_completion_popup_has_no_explicit_interaction()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    const QString input = QStringLiteral("co");
    editor.setText(input);
    editor.setCursorPosition(input.size());

    QSignalSpy returnPressedSpy(&editor, SIGNAL(returnPressed()));

    QVERIFY(QMetaObject::invokeMethod(
        &editor,
        "triggerAutoComplete",
        Qt::DirectConnection));

    QTreeWidget* popup = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((popup = s_completionPopupTree()) != nullptr, 1000);

    QTest::keyClick(popup, Qt::Key_Return, Qt::NoModifier);
    QCoreApplication::processEvents();

    QCOMPARE(returnPressedSpy.count(), 1);
    QCOMPARE(editor.text(), input);
}

void TestEditorUi::completion_popup_enter_respects_accept_with_enter_data()
{
    QTest::addColumn<bool>("enabled");
    QTest::addColumn<bool>("navigated");
    QTest::addColumn<bool>("sendToPopup");
    QTest::addColumn<int>("key");
    for (bool enabled : {false, true}) {
        for (bool navigated : {false, true}) {
            for (bool sendToPopup : {false, true}) {
                for (int key : {Qt::Key_Return, Qt::Key_Enter}) {
                    const QString name = QStringLiteral("%1-%2-%3-%4")
                        .arg(enabled ? "enabled" : "disabled")
                        .arg(navigated ? "arrow-selection" : "first-suggestion")
                        .arg(sendToPopup ? "popup" : "editor")
                        .arg(key == Qt::Key_Return ? "return" : "keypad-enter");
                    QTest::newRow(qPrintable(name)) << enabled << navigated << sendToPopup << key;
                }
            }
        }
    }
}

void TestEditorUi::completion_popup_enter_respects_accept_with_enter()
{
    QFETCH(bool, enabled);
    QFETCH(bool, navigated);
    QFETCH(bool, sendToPopup);
    QFETCH(int, key);
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();
    editor.setText(QStringLiteral("cos"));
    editor.setCursorPosition(editor.text().size());
    QSignalSpy returnPressedSpy(&editor, &Editor::returnPressed);
    QVERIFY(QMetaObject::invokeMethod(&editor, "triggerAutoComplete", Qt::DirectConnection));
    QTreeWidget* popup = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((popup = s_completionPopupTree()) != nullptr, 1000);
    QWidget* target = sendToPopup ? static_cast<QWidget*>(popup) : &editor;
    if (navigated)
        QTest::keyClick(target, Qt::Key_Down);
    QVERIFY(popup->currentItem());
    const QString completion = popup->currentItem()->text(1) + QStringLiteral("()");
    Settings::instance()->autoCompletionAcceptWithEnter = enabled;
    QTest::keyClick(target, Qt::Key(key));
    QVERIFY(!popup->isVisible());
    if (enabled || navigated) {
        QCOMPARE(returnPressedSpy.count(), 0);
        QCOMPARE(editor.text(), completion);
        QCOMPARE(editor.textCursor().position(), completion.size() - 1);
        QTRY_VERIFY(s_editorOrViewportHasFocus(&editor));
        QTest::keyClick(&editor, Qt::Key(key));
        QCOMPARE(returnPressedSpy.count(), 1);
    } else {
        QCOMPARE(returnPressedSpy.count(), 1);
        QCOMPARE(editor.text(), QStringLiteral("cos"));
    }
}

void TestEditorUi::enter_accepts_completion_for_names_and_units_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");
    QTest::addColumn<bool>("inputAdjustments");
    QTest::addColumn<int>("key");
    for (bool inputAdjustments : {false, true}) {
        for (int key : {Qt::Key_Return, Qt::Key_Tab}) {
            const QByteArray prefix = QByteArray(inputAdjustments ? "adjusted-" : "raw-")
                + (key == Qt::Key_Return ? "enter-" : "tab-");
            QTest::newRow(QByteArray(prefix + "user-variable").constData())
                << QStringLiteral("entervar") << QStringLiteral("entervariable") << inputAdjustments << key;
            QTest::newRow(QByteArray(prefix + "user-function").constData())
                << QStringLiteral("enterfun") << QStringLiteral("enterfunction()") << inputAdjustments << key;
            QTest::newRow(QByteArray(prefix + "unit").constData())
                << QStringLiteral("[met") << QStringLiteral("[m") << inputAdjustments << key;
        }
    }
}

void TestEditorUi::enter_accepts_completion_for_names_and_units()
{
    QFETCH(QString, input);
    QFETCH(QString, expected);
    QFETCH(bool, inputAdjustments);
    QFETCH(int, key);
    Settings::instance()->autoCompletionAcceptWithEnter = true;
    Settings::instance()->automaticInputAdjustments = inputAdjustments;
    Editor editor;
    editor.evaluator()->setVariable(QStringLiteral("entervariable"), Quantity(42));
    editor.evaluator()->setUserFunction(UserFunction(
        QStringLiteral("enterfunction"), QStringList{QStringLiteral("x")}, QStringLiteral("x")));
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();
    editor.setText(input);
    editor.setCursorPosition(editor.text().size());
    QSignalSpy returnPressedSpy(&editor, &Editor::returnPressed);
    QVERIFY(QMetaObject::invokeMethod(&editor, "triggerAutoComplete", Qt::DirectConnection));
    QTreeWidget* popup = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((popup = s_completionPopupTree()) != nullptr, 1000);
    QTest::keyClick(&editor, Qt::Key(key));
    QCOMPARE(editor.text(), expected);
    QCOMPARE(returnPressedSpy.count(), 0);
    QVERIFY(!popup->isVisible());
}

void TestEditorUi::enter_evaluates_without_completion_when_accept_with_enter_is_enabled()
{
    Settings::instance()->autoCompletionAcceptWithEnter = true;
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();
    editor.setText(QStringLiteral("2+2"));
    editor.setCursorPosition(editor.text().size());
    QSignalSpy returnPressedSpy(&editor, &Editor::returnPressed);
    QVERIFY(!s_completionPopupTree());
    QTest::keyClick(&editor, Qt::Key_Return);
    QCOMPARE(returnPressedSpy.count(), 1);
    QCOMPARE(editor.text(), QStringLiteral("2+2"));
}

void TestEditorUi::completion_popup_arrow_keys_change_selection()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("co"));
    editor.setCursorPosition(editor.text().size());

    QVERIFY(QMetaObject::invokeMethod(
        &editor,
        "triggerAutoComplete",
        Qt::DirectConnection));

    QTreeWidget* popup = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((popup = s_completionPopupTree()) != nullptr, 1000);
    QVERIFY(popup->topLevelItemCount() > 1);

    QTreeWidgetItem* initialItem = popup->currentItem();
    QVERIFY(initialItem);

    QTest::keyClick(popup, Qt::Key_Down, Qt::NoModifier);
    QCoreApplication::processEvents();

    QVERIFY(popup->currentItem());
    QVERIFY(popup->currentItem() != initialItem);
    popup->hide();
}

void TestEditorUi::completion_popup_tab_accepts_selected_item()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("co"));
    editor.setCursorPosition(editor.text().size());

    QVERIFY(QMetaObject::invokeMethod(
        &editor,
        "triggerAutoComplete",
        Qt::DirectConnection));

    QTreeWidget* popup = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((popup = s_completionPopupTree()) != nullptr, 1000);
    QTreeWidgetItem* cosItem = nullptr;
    for (int i = 0; i < popup->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = popup->topLevelItem(i);
        if (item != nullptr && item->text(1) == QStringLiteral("cos")) {
            cosItem = item;
            break;
        }
    }
    QVERIFY(cosItem);
    popup->setCurrentItem(cosItem);

    QTest::keyClick(&editor, Qt::Key_Tab);
    QCoreApplication::processEvents();

    QCOMPARE(editor.text(), QStringLiteral("cos()"));
    QTRY_VERIFY_WITH_TIMEOUT(s_editorOrViewportHasFocus(&editor), 1000);
}

void TestEditorUi::completion_popup_closes_when_clicking_elsewhere()
{
    QWidget container;
    QVBoxLayout layout(&container);
    Editor editor;
    QPushButton button(QStringLiteral("Other"));
    layout.addWidget(&editor);
    layout.addWidget(&button);
    container.resize(800, 400);
    container.show();
    QVERIFY(QTest::qWaitForWindowExposed(&container));
    editor.setFocus();

    editor.setText(QStringLiteral("co"));
    editor.setCursorPosition(editor.text().size());

    QVERIFY(QMetaObject::invokeMethod(
        &editor,
        "triggerAutoComplete",
        Qt::DirectConnection));

    QTreeWidget* popup = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((popup = s_completionPopupTree()) != nullptr, 1000);

    QTest::mouseClick(&container,
                      Qt::LeftButton,
                      Qt::NoModifier,
                      QPoint(container.width() - 2, container.height() - 2));
    QCoreApplication::processEvents();

    QVERIFY(!popup->isVisible());
    QTest::qWait(250);
    QVERIFY(!popup->isVisible());
    popup->hide();
}

void TestEditorUi::completion_popup_clicking_row_accepts_item()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("co"));
    editor.setCursorPosition(editor.text().size());

    QVERIFY(QMetaObject::invokeMethod(
        &editor,
        "triggerAutoComplete",
        Qt::DirectConnection));

    QTreeWidget* popup = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((popup = s_completionPopupTree()) != nullptr, 1000);
    QTreeWidgetItem* cosItem = nullptr;
    for (int i = 0; i < popup->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = popup->topLevelItem(i);
        if (item != nullptr && item->text(1) == QStringLiteral("cos")) {
            cosItem = item;
            break;
        }
    }
    QVERIFY(cosItem);

    popup->scrollToItem(cosItem);
    const QRect itemRect = popup->visualItemRect(cosItem);
    QVERIFY(!itemRect.isEmpty());
    QTest::mouseClick(popup->viewport(), Qt::LeftButton, Qt::NoModifier, itemRect.center());
    QCoreApplication::processEvents();

    QCOMPARE(editor.text(), QStringLiteral("cos()"));
}

void TestEditorUi::completion_popup_mouse_selection_keeps_focus_on_owning_editor()
{
    QWidget container;
    QVBoxLayout layout(&container);
    Editor firstEditor;
    Editor secondEditor;
    layout.addWidget(&firstEditor);
    layout.addWidget(&secondEditor);
    container.show();
    QVERIFY(QTest::qWaitForWindowExposed(&container));

    container.activateWindow();
    secondEditor.setFocus();
    QCoreApplication::processEvents();
    QTRY_VERIFY_WITH_TIMEOUT(s_editorOrViewportHasFocus(&secondEditor), 1000);

    firstEditor.setFocus();
    QTRY_VERIFY_WITH_TIMEOUT(s_editorOrViewportHasFocus(&firstEditor), 1000);
    firstEditor.setText(QStringLiteral("co"));
    firstEditor.setCursorPosition(firstEditor.text().size());

    QVERIFY(QMetaObject::invokeMethod(
        &firstEditor,
        "triggerAutoComplete",
        Qt::DirectConnection));

    QTreeWidget* popup = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((popup = s_completionPopupTree()) != nullptr, 1000);
    QTreeWidgetItem* cosItem = nullptr;
    for (int i = 0; i < popup->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = popup->topLevelItem(i);
        if (item != nullptr && item->text(1) == QStringLiteral("cos")) {
            cosItem = item;
            break;
        }
    }
    QVERIFY(cosItem);

    popup->scrollToItem(cosItem);
    const QRect itemRect = popup->visualItemRect(cosItem);
    QVERIFY(!itemRect.isEmpty());
    QTest::mouseClick(popup->viewport(), Qt::LeftButton, Qt::NoModifier, itemRect.center());
    QCoreApplication::processEvents();

    QCOMPARE(firstEditor.text(), QStringLiteral("cos()"));
    QTRY_VERIFY_WITH_TIMEOUT(s_editorOrViewportHasFocus(&firstEditor), 1000);
    QVERIFY(!s_editorOrViewportHasFocus(&secondEditor));
}

void TestEditorUi::completion_popup_scrollbar_click_keeps_popup_open()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("[m"));
    editor.setCursorPosition(editor.text().size());

    QVERIFY(QMetaObject::invokeMethod(
        &editor,
        "triggerAutoComplete",
        Qt::DirectConnection));

    QTreeWidget* popup = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((popup = s_completionPopupTree()) != nullptr, 1000);
    QVERIFY(popup->verticalScrollBar()->isVisible());

    QTest::mouseClick(popup->verticalScrollBar(), Qt::LeftButton);
    QCoreApplication::processEvents();

    QVERIFY(popup->isVisible());
    popup->hide();
}

void TestEditorUi::completion_popup_mouse_wheel_scrolls_popup()
{
    Settings* settings = Settings::instance();
    const bool userVariablesBackup = settings->autoCompletionUserVariables;
    settings->autoCompletionUserVariables = true;
    struct UserVariablesCompletionRestore {
        Settings* settings;
        bool value;
        ~UserVariablesCompletionRestore() { settings->autoCompletionUserVariables = value; }
    } restore { settings, userVariablesBackup };

    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    for (int i = 0; i < 30; ++i)
        editor.evaluator()->setVariable(QStringLiteral("wheel_var_%1").arg(i), Quantity(i));

    editor.setText(QStringLiteral("wheel_var_"));
    editor.setCursorPosition(editor.text().size());

    QVERIFY(QMetaObject::invokeMethod(
        &editor,
        "triggerAutoComplete",
        Qt::DirectConnection));

    QTreeWidget* popup = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT(
        (popup = s_completionPopupTreeContaining(QStringLiteral("wheel_var_"))) != nullptr,
        1000);
    QVERIFY2(popup->verticalScrollBar()->maximum() > popup->verticalScrollBar()->minimum(),
             qPrintable(QStringLiteral("min=%1 max=%2 rows=%3")
                            .arg(popup->verticalScrollBar()->minimum())
                            .arg(popup->verticalScrollBar()->maximum())
                            .arg(popup->topLevelItemCount())));
    popup->verticalScrollBar()->setValue(popup->verticalScrollBar()->minimum());
    QVERIFY(popup->topLevelItemCount() > 8);
    const int initialValue = popup->verticalScrollBar()->value();

    const QPoint globalWheelPosition =
        popup->viewport()->mapToGlobal(popup->viewport()->rect().center());
    QWheelEvent wheelEvent(
        editor.mapFromGlobal(globalWheelPosition),
        globalWheelPosition,
        QPoint(),
        QPoint(0, -120),
        Qt::NoButton,
        Qt::NoModifier,
        Qt::NoScrollPhase,
        false);
    QCoreApplication::sendEvent(&editor, &wheelEvent);

    QVERIFY2(popup->verticalScrollBar()->value() > initialValue,
             qPrintable(QStringLiteral("initial=%1 current=%2 max=%3")
                            .arg(initialValue)
                            .arg(popup->verticalScrollBar()->value())
                            .arg(popup->verticalScrollBar()->maximum())));
    popup->hide();
}

void TestEditorUi::completion_popup_escape_closes_and_stays_closed()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("co"));
    editor.setCursorPosition(editor.text().size());

    QVERIFY(QMetaObject::invokeMethod(
        &editor,
        "triggerAutoComplete",
        Qt::DirectConnection));

    QTreeWidget* popup = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((popup = s_completionPopupTree()) != nullptr, 1000);

    QTest::keyClick(popup, Qt::Key_Escape);
    QCoreApplication::processEvents();

    QVERIFY(!popup->isVisible());
    QTest::qWait(250);
    QVERIFY(!popup->isVisible());
}

void TestEditorUi::completion_popup_escape_keeps_focus_on_owning_editor()
{
    QWidget container;
    QVBoxLayout layout(&container);
    Editor firstEditor;
    Editor secondEditor;
    layout.addWidget(&firstEditor);
    layout.addWidget(&secondEditor);
    container.show();
    QVERIFY(QTest::qWaitForWindowExposed(&container));

    container.activateWindow();
    firstEditor.setFocus();
    QTRY_VERIFY_WITH_TIMEOUT(s_editorOrViewportHasFocus(&firstEditor), 1000);
    firstEditor.setText(QStringLiteral("co"));
    firstEditor.setCursorPosition(firstEditor.text().size());

    QVERIFY(QMetaObject::invokeMethod(
        &firstEditor,
        "triggerAutoComplete",
        Qt::DirectConnection));

    QTreeWidget* popup = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((popup = s_completionPopupTree()) != nullptr, 1000);
    QTRY_VERIFY_WITH_TIMEOUT(s_editorOrViewportHasFocus(&firstEditor), 1000);
    QVERIFY(!s_editorOrViewportHasFocus(&secondEditor));

    QTest::keyClick(&firstEditor, Qt::Key_Escape);
    QCoreApplication::processEvents();

    QVERIFY(!popup->isVisible());
    QTRY_VERIFY_WITH_TIMEOUT(s_editorOrViewportHasFocus(&firstEditor), 1000);
    QVERIFY(!s_editorOrViewportHasFocus(&secondEditor));
}

void TestEditorUi::completion_popup_reopens_after_typing_following_escape()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("co"));
    editor.setCursorPosition(editor.text().size());

    QVERIFY(QMetaObject::invokeMethod(
        &editor,
        "triggerAutoComplete",
        Qt::DirectConnection));

    QTreeWidget* popup = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((popup = s_completionPopupTree()) != nullptr, 1000);
    QTest::keyClick(popup, Qt::Key_Escape);
    QCoreApplication::processEvents();
    QVERIFY(!popup->isVisible());

    editor.insert(QStringLiteral("s"));
    QVERIFY(QMetaObject::invokeMethod(
        &editor,
        "triggerAutoComplete",
        Qt::DirectConnection));

    QTRY_VERIFY_WITH_TIMEOUT((popup = s_completionPopupTree()) != nullptr, 1000);
    QVERIFY(popup->isVisible());
    popup->hide();
}

void TestEditorUi::completion_popup_reopens_after_retyping_deleted_letter_data()
{
    QTest::addColumn<QString>("initialText");
    QTest::addColumn<QString>("letter");
    QTest::addColumn<bool>("processDeletion");
    QTest::addColumn<bool>("dismissWithEscape");

    QTest::newRow("a-immediate") << QString() << QStringLiteral("a") << false << false;
    QTest::newRow("a-delayed") << QString() << QStringLiteral("a") << true << false;
    QTest::newRow("c-delayed") << QString() << QStringLiteral("c") << true << false;
    QTest::newRow("s-delayed") << QString() << QStringLiteral("s") << true << false;
    QTest::newRow("co-immediate") << QStringLiteral("c") << QStringLiteral("o") << false << false;
    QTest::newRow("unit-delayed") << QStringLiteral("1[") << QStringLiteral("m") << true << false;
    QTest::newRow("escape-delayed") << QString() << QStringLiteral("a") << true << true;
}

void TestEditorUi::completion_popup_reopens_after_retyping_deleted_letter()
{
    QFETCH(QString, initialText);
    QFETCH(QString, letter);
    QFETCH(bool, processDeletion);
    QFETCH(bool, dismissWithEscape);

    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.activateWindow();
    editor.setFocus();
    QTRY_VERIFY_WITH_TIMEOUT(s_editorOrViewportHasFocus(&editor), 1000);
    editor.setText(initialText);
    editor.setCursorPosition(editor.text().size());
    QTest::keyClicks(&editor, letter);
    const QString completedText = editor.text();
    const int completedPosition = editor.textCursor().position();

    QTreeWidget* popup = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((popup = s_completionPopupTree()) != nullptr, 1000);

    for (int cycle = 0; cycle < 2; ++cycle) {
        if (dismissWithEscape) {
            QTest::keyClick(&editor, Qt::Key_Escape);
            QVERIFY(!popup->isVisible());
        }
        QTest::keyClick(&editor, Qt::Key_Backspace);
        QCOMPARE(editor.text(), initialText);
        if (processDeletion)
            QTest::qWait(50);

        QTest::keyClicks(&editor, letter);
        QCOMPARE(editor.text(), completedText);
        QCOMPARE(editor.textCursor().position(), completedPosition);
        QTRY_VERIFY_WITH_TIMEOUT((popup = s_completionPopupTree()) != nullptr, 1000);
    }
    popup->hide();
}

void TestEditorUi::completion_popup_shows_for_two_character_prefixes()
{
    for (const QString& prefix : {QStringLiteral("co"), QStringLiteral("si")}) {
        Editor editor;
        editor.show();
        QVERIFY(QTest::qWaitForWindowExposed(&editor));
        editor.setFocus();
        editor.setText(prefix);
        editor.setCursorPosition(editor.text().size());

        QVERIFY(QMetaObject::invokeMethod(
            &editor,
            "triggerAutoComplete",
            Qt::DirectConnection));

        QTreeWidget* popup = nullptr;
        QTRY_VERIFY_WITH_TIMEOUT((popup = s_completionPopupTree()) != nullptr, 1000);
        QVERIFY(popup->isVisible());
        popup->hide();
    }
}

void TestEditorUi::completion_popup_uses_positionable_nonactivating_surface()
{
    QWidget firstWindow;
    QWidget secondWindow;
    QVBoxLayout firstLayout(&firstWindow);
    QVBoxLayout secondLayout(&secondWindow);
    // Match application startup: construct the editor before placing it in a pane.
    Editor editor;
    editor.setAutoCompletionEnabled(false);
    firstLayout.addWidget(&editor);
    firstWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&firstWindow));
    editor.setFocus();

    EditorCompletion* completion = editor.findChild<EditorCompletion*>();
    QVERIFY(completion);
    completion->showCompletion({QStringLiteral("cos:Cosine")});
    QTreeWidget* popup = s_completionPopupTree();
    QVERIFY(popup);
    QCOMPARE(popup->windowType(), Qt::ToolTip);
    QVERIFY(popup->testAttribute(Qt::WA_ShowWithoutActivating));
    QCOMPARE(popup->focusPolicy(), Qt::NoFocus);
    QCOMPARE(popup->windowHandle()->transientParent(), firstWindow.windowHandle());
    QVERIFY(s_editorOrViewportHasFocus(&editor));
    popup->hide();

    // Moving an existing pane into another window must refresh the native parent.
    secondLayout.addWidget(&editor);
    firstWindow.hide();
    secondWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&secondWindow));
    editor.setFocus();
    completion->showCompletion({QStringLiteral("cos:Cosine")});
    QCOMPARE(popup->windowHandle()->transientParent(), secondWindow.windowHandle());
    QVERIFY(s_editorOrViewportHasFocus(&editor));
    popup->hide();
}

void TestEditorUi::completion_popup_keeps_editor_widgets_non_native_data()
{
    QTest::addColumn<bool>("constants");
    QTest::newRow("autocomplete") << false;
    QTest::newRow("constants") << true;
}

void TestEditorUi::completion_popup_keeps_editor_widgets_non_native()
{
    QFETCH(bool, constants);
    QWidget window;
    QVBoxLayout layout(&window);
    Editor editor;
    editor.setAutoCompletionEnabled(false);
    layout.addWidget(&editor);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    editor.setFocus();
    QVERIFY(!editor.testAttribute(Qt::WA_NativeWindow));
    QVERIFY(!editor.viewport()->testAttribute(Qt::WA_NativeWindow));
    QVERIFY(!editor.verticalScrollBar()->testAttribute(Qt::WA_NativeWindow));

    if (constants) {
        QTest::keyClick(&editor, Qt::Key_Space, Qt::ControlModifier);
        QVERIFY(QApplication::activePopupWidget());
    } else {
        EditorCompletion* completion = editor.findChild<EditorCompletion*>();
        QVERIFY(completion);
        completion->showCompletion({QStringLiteral("cos:Cosine")});
        QVERIFY(s_completionPopupTree());
    }
    QCoreApplication::processEvents();

    // Forcing native siblings creates extra Wayland subsurfaces for the text
    // viewport and scrollbars and changes their backing-store composition.
    QVERIFY(!editor.testAttribute(Qt::WA_NativeWindow));
    QVERIFY(!editor.viewport()->testAttribute(Qt::WA_NativeWindow));
    QVERIFY(!editor.verticalScrollBar()->testAttribute(Qt::WA_NativeWindow));
    if (QWidget* popup = constants ? QApplication::activePopupWidget() : s_completionPopupTree())
        popup->hide();
}

void TestEditorUi::completion_popup_follows_editor_geometry_data()
{
    QTest::addColumn<bool>("wrapped");
    QTest::addColumn<bool>("nearBottom");
    QTest::addColumn<bool>("nearRight");
    QTest::newRow("below") << false << false << false;
    QTest::newRow("above") << false << true << false;
    QTest::newRow("right-edge") << false << false << true;
    QTest::newRow("wrapped") << true << false << false;
    QTest::newRow("wrapped-above") << true << true << false;
}

void TestEditorUi::completion_popup_follows_editor_geometry()
{
    QFETCH(bool, wrapped);
    QFETCH(bool, nearBottom);
    QFETCH(bool, nearRight);

    Editor editor;
    editor.setAutoCompletionEnabled(false);
    editor.setFixedWidth(340);
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();
    const QString input = wrapped
        ? QStringLiteral("1234567890 + ").repeated(30) + QStringLiteral("co")
        : QStringLiteral("123 + co");
    editor.setText(input);
    editor.setCursorPosition(input.size());
    QCoreApplication::processEvents();

    const QRect screen = editor.screen()->availableGeometry();
    const int x = nearRight ? screen.right() - 40 : screen.left() + 30;
    const int y = nearBottom ? screen.bottom() - editor.height() + 1 : screen.top() + 80;
    editor.move(x, y);
    QCoreApplication::processEvents();
    QTextCursor anchor = editor.textCursor();
    anchor.movePosition(QTextCursor::StartOfWord);
    const int anchorX = editor.viewport()->mapToGlobal(editor.cursorRect(anchor).topLeft()).x();

    EditorCompletion* completion = editor.findChild<EditorCompletion*>();
    QVERIFY(completion);
    completion->showCompletion({QStringLiteral("cos:Cosine"), QStringLiteral("cosh:Hyperbolic cosine")});
    QTreeWidget* popup = s_completionPopupTree();
    QVERIFY(popup);
    const QPoint editorTop = editor.mapToGlobal(QPoint(0, 0));
    const int belowY = editorTop.y() + editor.height();
    const int expectedY = belowY + popup->height() > screen.bottom() + 1
        ? editorTop.y() - popup->height() : belowY;
    const int expectedX = qBound(screen.left(), anchorX, screen.right() + 1 - popup->width());
    QCOMPARE(popup->geometry().topLeft(), QPoint(expectedX, expectedY));
    QVERIFY(screen.contains(popup->geometry()));
    if (wrapped)
        QVERIFY(editor.verticalScrollBar()->value() > 0);
    popup->hide();
}

void TestEditorUi::completion_popup_geometry_avoids_editor_overlap_data()
{
    QTest::addColumn<QRect>("bounds");
    QTest::addColumn<QRect>("editor");
    QTest::addColumn<int>("height");
    QTest::addColumn<QRect>("expected");
    QTest::newRow("below")
        << QRect(0, 0, 600, 400) << QRect(30, 100, 400, 60) << 100 << QRect(50, 160, 200, 100);
    QTest::newRow("above-at-window-bottom")
        << QRect(0, 0, 600, 400) << QRect(30, 330, 400, 60) << 100 << QRect(50, 230, 200, 100);
    QTest::newRow("shrink-above")
        << QRect(0, 0, 600, 400) << QRect(30, 200, 400, 60) << 300 << QRect(50, 0, 200, 200);
    QTest::newRow("shrink-below")
        << QRect(0, 0, 600, 400) << QRect(30, 80, 400, 60) << 300 << QRect(50, 140, 200, 260);
    QTest::newRow("negative-screen-origin")
        << QRect(0, -400, 600, 400) << QRect(30, -200, 400, 60) << 300 << QRect(50, -400, 200, 200);
    QTest::newRow("no-space")
        << QRect(0, 0, 600, 60) << QRect(30, 0, 400, 60) << 100 << QRect();
}

void TestEditorUi::completion_popup_geometry_avoids_editor_overlap()
{
    QFETCH(QRect, bounds);
    QFETCH(QRect, editor);
    QFETCH(int, height);
    QFETCH(QRect, expected);
    const QRect popup = EditorUtils::completionPopupGeometry(editor, 50, QSize(200, height), bounds);
    QCOMPARE(popup, expected);
    QVERIFY(!popup.intersects(editor));
    if (popup.isValid())
        QVERIFY(bounds.contains(popup));
}

void TestEditorUi::completion_popup_shortens_tall_lists_without_covering_editor()
{
    Editor editor;
    editor.setAutoCompletionEnabled(false);
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();
    const QRect screen = editor.screen()->availableGeometry();
    editor.move(screen.left() + 20, screen.center().y() - editor.height() / 2);

    QTreeWidget* popup = editor.findChild<QTreeWidget*>(QStringLiteral("editorCompletionPopup"));
    QVERIFY(popup);
    QFont font = popup->font();
    font.setPixelSize(qMax(1, screen.height() / 8));
    popup->setFont(font);
    QStringList choices;
    for (int i = 0; i < 8; ++i)
        choices.append(QStringLiteral("%1:x").arg(i));
    EditorCompletion* completion = editor.findChild<EditorCompletion*>();
    QVERIFY(completion);
    completion->showCompletion(choices);
    QCoreApplication::processEvents();

    QVERIFY(popup->isVisible());
    const QRect editorRect(editor.mapToGlobal(QPoint(0, 0)), editor.size());
    QVERIFY(!popup->geometry().intersects(editorRect));
    QVERIFY(popup->geometry().top() >= screen.top());
    QVERIFY(popup->geometry().bottom() <= screen.bottom());
    QVERIFY(popup->verticalScrollBar()->maximum() > 0);
    popup->hide();
}

void TestEditorUi::completion_popups_configure_editor_anchor_data()
{
    QTest::addColumn<bool>("constants");
    QTest::newRow("autocomplete") << false;
    QTest::newRow("constants") << true;
}

void TestEditorUi::completion_popups_configure_editor_anchor()
{
    QFETCH(bool, constants);
    QWidget window;
    QVBoxLayout layout(&window);
    layout.addStretch();
    Editor editor;
    editor.setAutoCompletionEnabled(false);
    editor.setFixedWidth(340);
    layout.addWidget(&editor);
    window.resize(400, 400);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    // Constant completion is normally constructed lazily on Ctrl+Space.
    ConstantCompletion* constantCompletion = constants ? new ConstantCompletion(&editor) : nullptr;
    QWidget* popup = editor.findChild<QWidget*>(constants
        ? QStringLiteral("constantCompletionPopup") : QStringLiteral("editorCompletionPopup"));
    QVERIFY(popup);
    // Both popups must configure the very first native surface created by show().
    QVERIFY(!popup->windowHandle());
    QRect previousAnchor;
    for (const bool wrapped : {false, true}) {
        const QString input = wrapped
            ? QStringLiteral("1234567890 + ").repeated(30) + QStringLiteral("co")
            : QStringLiteral("123 + co");
        editor.setText(input);
        editor.setCursorPosition(input.size());
        editor.setFocus();
        QCoreApplication::processEvents();
        QTextCursor cursor = editor.textCursor();
        if (!constants)
            cursor.movePosition(QTextCursor::StartOfWord);
        const QRect editorRect(editor.mapTo(&window, QPoint()), editor.size());
        const int cursorX = editor.viewport()->mapTo(&window, editor.cursorRect(cursor).topLeft()).x();
        const QRect expected(qBound(editorRect.left(), cursorX, editorRect.right()),
                             editorRect.top(), 1, editorRect.height());

        if (constants) {
            constantCompletion->showCompletion();
        } else {
            editor.findChild<EditorCompletion*>()->showCompletion({QStringLiteral("cos:Cosine")});
        }
        QVERIFY(popup->isVisible());
        QWindow* native = popup->windowHandle();
        QVERIFY(native);
        QCOMPARE(native->transientParent(), window.windowHandle());
        QCOMPARE(native->property("_q_waylandPopupAnchorRect").toRect(), expected);
        QCOMPARE(native->property("_q_waylandPopupAnchor").value<Qt::Edges>(),
                 Qt::Edges(Qt::BottomEdge | Qt::LeftEdge));
        QCOMPARE(native->property("_q_waylandPopupGravity").value<Qt::Edges>(),
                 Qt::Edges(Qt::BottomEdge | Qt::RightEdge));
        const uint constraints = native->property("_q_waylandPopupConstraintAdjustment").toUInt();
        QVERIFY(constraints & 1); // slide_x
        QVERIFY(constraints & 8); // flip_y
        QVERIFY(constraints & 32); // resize_y
        QVERIFY(!(constraints & 2)); // slide_y would overlap the editor
        if (wrapped) {
            QVERIFY(expected != previousAnchor);
            QVERIFY(editor.verticalScrollBar()->value() > 0);
        }
        previousAnchor = expected;
        popup->hide();
    }
}

void TestEditorUi::completion_popup_compositor_geometry_data()
{
    QTest::addColumn<QRect>("editor");
    QTest::addColumn<int>("cursorX");
    QTest::addColumn<QSize>("requested");
    QTest::addColumn<QRect>("expectedAnchor");
    QTest::addColumn<QRect>("expectedPopup");
    QTest::newRow("below-bottom-of-parent")
        << QRect(10, 340, 400, 60) << 50 << QSize(200, 100)
        << QRect(50, 340, 1, 60) << QRect(50, 400, 200, 100);
    QTest::newRow("negative-guessed-origin")
        << QRect(-600, -400, 400, 60) << -550 << QSize(200, 100)
        << QRect(-550, -400, 1, 60) << QRect(-550, -340, 200, 100);
    QTest::newRow("word-start-scrolled-off-left")
        << QRect(10, 340, 400, 60) << -50 << QSize(200, 100)
        << QRect(10, 340, 1, 60) << QRect(10, 400, 200, 100);
    QTest::newRow("cursor-scrolled-off-right")
        << QRect(10, 340, 400, 60) << 500 << QSize(200, 100)
        << QRect(409, 340, 1, 60) << QRect(409, 400, 200, 100);
    QTest::newRow("tall-and-wide-list")
        << QRect(10, 340, 400, 60) << 50 << QSize(1000, 1000)
        << QRect(50, 340, 1, 60) << QRect(50, 400, 800, 270);
    QTest::newRow("editor-fills-output")
        << QRect(10, 0, 400, 600) << 50 << QSize(200, 100)
        << QRect(50, 0, 1, 600) << QRect();
}

void TestEditorUi::completion_popup_compositor_geometry()
{
    QFETCH(QRect, editor);
    QFETCH(int, cursorX);
    QFETCH(QSize, requested);
    QFETCH(QRect, expectedAnchor);
    QFETCH(QRect, expectedPopup);
    const QRect anchor = EditorUtils::completionPopupAnchorRect(editor, cursorX);
    QCOMPARE(anchor, expectedAnchor);
    const QRect popup = EditorUtils::completionPopupCompositorGeometry(anchor, requested, QSize(800, 600));
    QCOMPARE(popup, expectedPopup);
    QVERIFY(!popup.intersects(editor));
    if (popup.isValid()) {
        // xdg-shell flip_y reflects both anchor and gravity, placing the bottom
        // of the popup at the editor's top, regardless of the parent's origin.
        const QRect flipped(QPoint(anchor.left(), anchor.top() - popup.height()), popup.size());
        QVERIFY(!flipped.intersects(editor));
        QCOMPARE(flipped.bottom() + 1, editor.top());
    }
}

void TestEditorUi::constant_search_filters_without_editing_expression()
{
    Editor editor;
    editor.setAutoCompletionEnabled(false);
    editor.setText(QStringLiteral("12+34"));
    QTextCursor cursor = editor.textCursor();
    cursor.setPosition(3);
    cursor.setPosition(5, QTextCursor::KeepAnchor);
    editor.setTextCursor(cursor);
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();
    QSignalSpy evaluated(&editor, &Editor::returnPressed);
    QSignalSpy changed(&editor, &Editor::textChanged);
    QTest::keyClick(&editor, Qt::Key_Space, Qt::ControlModifier);
    QPointer<QWidget> popup = QApplication::activePopupWidget();
    QVERIFY(popup);
    auto* search = popup->findChild<QLineEdit*>(QStringLiteral("constantCompletionSearch"));
    auto* list = popup->findChild<QTreeWidget*>();
    QVERIFY(search && list);
    QTRY_VERIFY(search->hasFocus());
    QCOMPARE(list->topLevelItemCount(), Constants::instance()->list().size());
    QTest::keyClick(search, Qt::Key_Tab, Qt::ShiftModifier);
    QVERIFY(list->hasFocus());
    QTest::keyClick(list, Qt::Key_Tab, Qt::ShiftModifier);
    QVERIFY(search->hasFocus());
    QTest::keyClick(search, Qt::Key_Tab, Qt::ShiftModifier);
    QVERIFY(list->hasFocus());
    QTest::keyClick(list, Qt::Key_P);
    QVERIFY(search->hasFocus());
    QCOMPARE(search->text(), QStringLiteral("p"));
    search->clear();
    QTest::keyClicks(search, "PROTON");
    QVERIFY(list->topLevelItemCount() > 1);
    for (int i = 0; i < list->topLevelItemCount(); ++i)
        QVERIFY(list->topLevelItem(i)->text(0).contains(QStringLiteral("proton"), Qt::CaseInsensitive));
    QTest::keyClick(search, Qt::Key_Down);
    QCOMPARE(list->indexOfTopLevelItem(list->currentItem()), 1);
    QTest::keyClick(search, Qt::Key_Up);
    QCOMPARE(list->indexOfTopLevelItem(list->currentItem()), 0);
    QVERIFY(search->hasFocus());
    QTest::keyClick(search, Qt::Key_Backspace);
    QCOMPARE(search->text(), QStringLiteral("PROTO"));
    search->setText(QStringLiteral("kg"));
    QVERIFY(list->topLevelItemCount() > 0);
    for (int i = 0; i < list->topLevelItemCount(); ++i) {
        auto* item = list->topLevelItem(i);
        QVERIFY(item->data(0, Qt::AccessibleTextRole).toString().contains(QStringLiteral("kg"), Qt::CaseInsensitive));
    }
    search->clear();
    QCOMPARE(list->topLevelItemCount(), Constants::instance()->list().size());
    search->setText(QStringLiteral("no such constant 49236"));
    QCOMPARE(list->topLevelItemCount(), 0);
    QTest::keyClick(search, Qt::Key_Down);
    QTest::keyClick(search, Qt::Key_Return);
    QTest::keyClick(search, Qt::Key_Tab);
    QVERIFY(popup->isVisible());
    QVERIFY(search->hasFocus());
    QVERIFY(list->currentItem() == nullptr);
    QCOMPARE(evaluated.count(), 0);
    QCOMPARE(changed.count(), 0);
    QCOMPARE(editor.text(), QStringLiteral("12+34"));
    QTest::keyClick(search, Qt::Key_Escape);
    QVERIFY(!popup || !popup->isVisible());
    QTRY_VERIFY(s_editorOrViewportHasFocus(&editor));
    QCOMPARE(editor.textCursor().selectionStart(), 3);
    QCOMPARE(editor.textCursor().selectionEnd(), 5);
    QCOMPARE(editor.text(), QStringLiteral("12+34"));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(popup.isNull());
}

void TestEditorUi::constant_search_inserts_selected_result_data()
{
    QTest::addColumn<QString>("query");
    QTest::addColumn<QString>("expected");
    QTest::addColumn<bool>("mouse");
    QTest::addColumn<int>("acceptKey");
    QTest::addColumn<bool>("listFocus");
    QTest::newRow("pi") << QStringLiteral("pi (π)") << QString(UnicodeChars::Pi) << false << int(Qt::Key_Return) << false;
    QTest::newRow("e-click") << QStringLiteral("Euler's number") << QStringLiteral("e") << true << int(Qt::Key_Return) << false;
    QTest::newRow("unit") << QStringLiteral("speed of light in vacuum")
        << QStringLiteral("299792458 [m·s⁻¹]") << false << int(Qt::Key_Return) << false;
    QTest::newRow("pi-tab") << QStringLiteral("pi (π)") << QString(UnicodeChars::Pi) << false << int(Qt::Key_Tab) << false;
    QTest::newRow("unit-tab") << QStringLiteral("speed of light in vacuum")
        << QStringLiteral("299792458 [m·s⁻¹]") << false << int(Qt::Key_Tab) << false;
    QTest::newRow("e-tab-from-list") << QStringLiteral("Euler's number") << QStringLiteral("e") << false << int(Qt::Key_Tab) << true;
}

void TestEditorUi::constant_search_inserts_selected_result()
{
    QFETCH(QString, query);
    QFETCH(QString, expected);
    QFETCH(bool, mouse);
    QFETCH(int, acceptKey);
    QFETCH(bool, listFocus);
    QWidget window;
    QVBoxLayout layout(&window);
    Editor editor;
    Editor other;
    layout.addWidget(&editor);
    layout.addWidget(&other);
    editor.setAutoCompletionEnabled(false);
    editor.setText(QStringLiteral("12+34"));
    QSignalSpy evaluated(&editor, &Editor::returnPressed);
    QTextCursor cursor = editor.textCursor();
    cursor.setPosition(3);
    cursor.setPosition(5, QTextCursor::KeepAnchor);
    editor.setTextCursor(cursor);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    editor.setFocus();
    QTest::keyClick(&editor, Qt::Key_Space, Qt::ControlModifier);
    QPointer<QWidget> popup = QApplication::activePopupWidget();
    QVERIFY(popup);
    auto* search = popup->findChild<QLineEdit*>(QStringLiteral("constantCompletionSearch"));
    auto* list = popup->findChild<QTreeWidget*>();
    QVERIFY(search && list);
    search->setText(query);
    QCOMPARE(list->topLevelItemCount(), 1);
    if (mouse) {
        QTest::mouseClick(list->viewport(), Qt::LeftButton, Qt::NoModifier,
                          list->visualItemRect(list->currentItem()).center());
    } else {
        if (listFocus) {
            QTest::keyClick(search, Qt::Key_Tab, Qt::ShiftModifier);
            QVERIFY(list->hasFocus());
        }
        QTest::keyClick(listFocus ? static_cast<QWidget*>(list) : search, static_cast<Qt::Key>(acceptKey));
    }
    QCOMPARE(editor.text(), QStringLiteral("12+") + expected);
    QCOMPARE(evaluated.count(), 0);
    QVERIFY(other.text().isEmpty());
    QTRY_VERIFY(s_editorOrViewportHasFocus(&editor));
    editor.undo();
    QCOMPARE(editor.text(), QStringLiteral("12+34"));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(popup.isNull());
}

void TestEditorUi::constant_search_closes_without_stealing_outside_focus()
{
    QWidget window;
    QVBoxLayout layout(&window);
    Editor editor;
    QLineEdit other;
    layout.addWidget(&editor);
    layout.addWidget(&other);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    editor.setFocus();
    QTest::keyClick(&editor, Qt::Key_Space, Qt::ControlModifier);
    QPointer<QWidget> popup = QApplication::activePopupWidget();
    QVERIFY(popup);
    popup->hide();
    other.setFocus();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(popup.isNull());
    QVERIFY(other.hasFocus());
    QVERIFY(editor.text().isEmpty());
}

void TestEditorUi::constant_search_layout_follows_popup_resize()
{
    Editor editor;
    editor.setAutoCompletionEnabled(false);
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    ConstantCompletion completion(&editor);
    completion.showCompletion();
    QWidget* popup = QApplication::activePopupWidget();
    QVERIFY(popup);
    auto* search = popup->findChild<QLineEdit*>(QStringLiteral("constantCompletionSearch"));
    auto* constants = popup->findChild<QTreeWidget*>();
    QVERIFY(search && constants);
    popup->resize(340, 150);
    QCoreApplication::processEvents();
    QVERIFY(popup->rect().contains(search->geometry()));
    QVERIFY(popup->rect().contains(constants->geometry()));
    QVERIFY(constants->height() > constants->sizeHintForRow(0));
    QTRY_VERIFY(constants->verticalScrollBar()->maximum() > 0);
    search->setText(QStringLiteral("no such constant 49236"));
    popup->resize(300, 130);
    QCoreApplication::processEvents();
    QLabel* empty = nullptr;
    for (auto* label : popup->findChildren<QLabel*>()) {
        if (label->property("dockListNoMatchLabel").toBool())
            empty = label;
    }
    QVERIFY(empty);
    QVERIFY(empty->isVisible());
    QCOMPARE(empty->geometry(), constants->viewport()->rect());
    QVERIFY(popup->rect().contains(constants->geometry()));
    popup->hide();
}

void TestEditorUi::constant_search_inline_units_preserve_full_insertion_data()
{
    QTest::addColumn<bool>("rightToLeft");
    QTest::newRow("left-to-right") << false;
    QTest::newRow("right-to-left") << true;
}

void TestEditorUi::constant_search_inline_units_preserve_full_insertion()
{
    QFETCH(bool, rightToLeft);
    Editor editor;
    editor.setAutoCompletionEnabled(false);
    editor.setLayoutDirection(rightToLeft ? Qt::RightToLeft : Qt::LeftToRight);
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    ConstantCompletion completion(&editor);
    QSignalSpy inserted(&completion, &ConstantCompletion::selectedCompletion);
    completion.showCompletion();
    QWidget* popup = QApplication::activePopupWidget();
    QVERIFY(popup);
    auto* search = popup->findChild<QLineEdit*>(QStringLiteral("constantCompletionSearch"));
    auto* list = popup->findChild<QTreeWidget*>();
    QVERIFY(search && list);
    const QString name = QStringLiteral("electron mass energy equivalent in MeV");
    search->setText(name);
    QCOMPARE(list->topLevelItemCount(), 1);
    auto* item = list->currentItem();
    QVERIFY(item);
    QCOMPARE(item->data(0, Qt::AccessibleTextRole).toString(), name + QStringLiteral(" MeV"));
    QVERIFY(item->toolTip(0).contains(QStringLiteral("MeV")));
    popup->resize(230, 140);
    QFont font = list->font();
    font.setPointSizeF(font.pointSizeF() * 1.5);
    list->setFont(font);
    QCoreApplication::processEvents();
    const QPixmap rendered = list->viewport()->grab();
    QVERIFY(!rendered.isNull());
    // A narrow row may elide the label, but insertion keeps the full quantity.
    QTest::keyClick(search, Qt::Key_Return);
    QCOMPARE(inserted.count(), 1);
    QCOMPARE(inserted.first().first().toString(),
             QStringLiteral("0.51099895069%1[MeV]").arg(QChar(MathDsl::QuantSp)));
}

void TestEditorUi::constant_search_renders_units_in_secondary_shade_data()
{
    QTest::addColumn<QColor>("background");
    QTest::addColumn<QColor>("foreground");
    QTest::addColumn<bool>("selected");
    QTest::addColumn<QColor>("expected");
    QTest::newRow("dark") << QColor(QStringLiteral("#454257")) << QColor(QStringLiteral("#f7f5fc"))
        << false << QColor(QStringLiteral("#cdcbe0"));
    QTest::newRow("light") << QColor(QStringLiteral("#f3f3f3")) << QColor(QStringLiteral("#202025"))
        << false << QColor(QStringLiteral("#282828"));
    QTest::newRow("selected") << QColor(QStringLiteral("#5c586f")) << QColor(QStringLiteral("#f7f5fc"))
        << true << QColor(QStringLiteral("#cdcbe0"));
    QTest::newRow("neutral") << QColor(Qt::white) << QColor(Qt::black)
        << false << QColor(QStringLiteral("#606060"));
}

void TestEditorUi::constant_search_renders_units_in_secondary_shade()
{
    QFETCH(QColor, background);
    QFETCH(QColor, foreground);
    QFETCH(bool, selected);
    QFETCH(QColor, expected);
    Editor editor;
    editor.setThemePreviewColorScheme(ColorScheme::fromJsonObject(themeJson({
        {QStringLiteral("background"), background.name()},
        {QStringLiteral("unit"), expected.name()}
    })));
    ConstantCompletion completion(&editor);
    auto* popup = editor.findChild<QWidget*>(QStringLiteral("constantCompletionPopup"));
    QVERIFY(popup);
    auto* list = popup->findChild<QTreeWidget*>();
    auto* search = popup->findChild<QLineEdit*>(QStringLiteral("constantCompletionSearch"));
    QVERIFY(list && search);
    search->setText(QStringLiteral("electron mass energy equivalent in MeV"));
    QCOMPARE(list->topLevelItemCount(), 1);
    auto* item = list->topLevelItem(0);
    // Paint the unit alone so name antialiasing cannot mimic its secondary color.
    item->setText(0, QStringLiteral(" "));
    QFont font = list->font();
    font.setPixelSize(32);
    list->setFont(font);
    QPalette palette = list->palette();
    for (auto group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
        palette.setColor(group, QPalette::Base, background);
        palette.setColor(group, QPalette::Highlight, background);
        palette.setColor(group, QPalette::Text, foreground);
        palette.setColor(group, QPalette::HighlightedText, foreground);
    }
    list->setPalette(palette);
    QStyleOptionViewItem option;
    option.initFrom(list);
    option.rect = QRect(0, 0, 400, 64);
    option.state = QStyle::State_Enabled | QStyle::State_Active;
    if (selected)
        option.state |= QStyle::State_Selected;
    option.font = font;
    option.fontMetrics = QFontMetrics(font);
    for (const QColor& unitColor : {expected, QColor(QStringLiteral("#789abc"))}) {
        editor.setThemePreviewColorScheme(ColorScheme::fromJsonObject(themeJson({
            {QStringLiteral("background"), background.name()},
            {QStringLiteral("unit"), unitColor.name()}
        })));
        QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(background);
        QPainter painter(&image);
        list->itemDelegate()->paint(&painter, option, list->model()->index(0, 0));
        painter.end();
        int unitPixels = 0;
        int mainTextPixels = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const QRgb pixel = image.pixel(x, y);
                unitPixels += pixel == unitColor.rgba();
                mainTextPixels += pixel == foreground.rgba();
            }
        }
        QVERIFY(unitPixels > 20);
        QCOMPARE(mainTextPixels, 0);
    }
}

void TestEditorUi::constant_completion_popup_follows_wrapped_editor_cursor()
{
    QWidget window;
    QVBoxLayout layout(&window);
    Editor editor;
    editor.setAutoCompletionEnabled(false);
    editor.setFixedWidth(340);
    layout.addWidget(&editor);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    editor.setFocus();
    const QString input = QStringLiteral("1234567890 + ").repeated(30);
    editor.setText(input);
    editor.setCursorPosition(input.size());
    QCoreApplication::processEvents();
    const QRect screen = editor.screen()->availableGeometry();
    window.move(screen.left() + 30, screen.top() + 80);
    QCoreApplication::processEvents();
    const int anchorX = editor.viewport()->mapToGlobal(editor.cursorRect().topLeft()).x();

    QTest::keyClick(&editor, Qt::Key_Space, Qt::ControlModifier);
    QWidget* popup = QApplication::activePopupWidget();
    QVERIFY(popup);
    QCOMPARE(popup->objectName(), QStringLiteral("constantCompletionPopup"));
    QCOMPARE(popup->windowHandle()->transientParent(), window.windowHandle());
    const QPoint editorTop = editor.mapToGlobal(QPoint(0, 0));
    const int belowY = editorTop.y() + editor.height();
    const int expectedY = belowY + popup->height() > screen.bottom() + 1
        ? editorTop.y() - popup->height() : belowY;
    const int expectedX = qBound(screen.left(), anchorX, screen.right() + 1 - popup->width());
    QCOMPARE(popup->geometry().topLeft(), QPoint(expectedX, expectedY));
    QVERIFY(editor.verticalScrollBar()->value() > 0);
    popup->hide();
}

void TestEditorUi::completion_popup_uses_configured_surface_colors()
{
    const QColor background(QStringLiteral("#334455"));
    const QColor foreground(QStringLiteral("#f4f7fb"));
    const QColor scrollbarThumb(QStringLiteral("#556677"));
    const QColor scrollbarThumbForeground(QStringLiteral("#ffffff"));
    const QColor selectedRow(QStringLiteral("#667788"));
    const QColor selectedRowForeground(QStringLiteral("#101418"));
    const QColor outline(QStringLiteral("#778899"));
    const int cornerRadius = 11;

    Editor editor;
    editor.setThemeCompletionColors(background,
                                    foreground,
                                    scrollbarThumb,
                                    scrollbarThumbForeground,
                                    selectedRow,
                                    selectedRowForeground,
                                    outline,
                                    cornerRadius);
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();
    editor.setText(QStringLiteral("co"));
    editor.setCursorPosition(editor.text().size());

    QVERIFY(QMetaObject::invokeMethod(
        &editor,
        "triggerAutoComplete",
        Qt::DirectConnection));

    QTreeWidget* popup = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((popup = s_completionPopupTree()) != nullptr, 1000);
    QCOMPARE(popup->palette().color(QPalette::Base), background);
    QCOMPARE(popup->palette().color(QPalette::Text), foreground);
    QCOMPARE(popup->palette().color(QPalette::Highlight), selectedRow);
    QCOMPARE(popup->palette().color(QPalette::HighlightedText), selectedRowForeground);
    QCOMPARE(popup->cursor().shape(), Qt::PointingHandCursor);
    QCOMPARE(popup->viewport()->cursor().shape(), Qt::PointingHandCursor);
    QVERIFY(popup->styleSheet().contains(QStringLiteral("background: #556677")));
    QVERIFY(popup->styleSheet().contains(QStringLiteral("QScrollBar:horizontal")));
    QVERIFY(popup->styleSheet().contains(QStringLiteral("QScrollBar::handle:horizontal")));
    QVERIFY(popup->styleSheet().contains(QStringLiteral("border: %1px solid #778899")
                                             .arg(UiConfig::PopupOutlineStrokeWidth)));
    QVERIFY(popup->styleSheet().contains(QStringLiteral("border-radius: 11px")));
    popup->hide();
}

void TestEditorUi::constant_completion_popup_uses_configured_surface_colors()
{
    const QColor background(QStringLiteral("#334455"));
    const QColor foreground(QStringLiteral("#f4f7fb"));
    const QColor scrollbarThumb(QStringLiteral("#556677"));
    const QColor scrollbarThumbForeground(QStringLiteral("#ffffff"));
    const QColor selectedRow(QStringLiteral("#667788"));
    const QColor selectedRowForeground(QStringLiteral("#101418"));
    const QColor outline(QStringLiteral("#778899"));
    const int cornerRadius = 11;

    Editor editor;
    editor.setThemeCompletionColors(background,
                                    foreground,
                                    scrollbarThumb,
                                    scrollbarThumbForeground,
                                    selectedRow,
                                    selectedRowForeground,
                                    outline,
                                    cornerRadius);
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    QTest::keyClick(&editor, Qt::Key_Space, Qt::ControlModifier);

    QList<QTreeWidget*> popupTrees;
    QTRY_VERIFY_WITH_TIMEOUT(!(popupTrees = s_constantCompletionPopupTrees()).isEmpty(), 1000);
    QCOMPARE(popupTrees.count(), 1);
    for (QTreeWidget* popup : popupTrees) {
        QCOMPARE(popup->palette().color(QPalette::Base), background);
        QCOMPARE(popup->palette().color(QPalette::Text), foreground);
        QCOMPARE(popup->palette().color(QPalette::Highlight), selectedRow);
        QCOMPARE(popup->palette().color(QPalette::HighlightedText), selectedRowForeground);
        QCOMPARE(popup->cursor().shape(), Qt::PointingHandCursor);
        QCOMPARE(popup->viewport()->cursor().shape(), Qt::PointingHandCursor);
        QVERIFY(popup->styleSheet().contains(QStringLiteral("background: #556677")));
        QVERIFY(popup->styleSheet().contains(QStringLiteral("QScrollBar:horizontal")));
        QVERIFY(popup->styleSheet().contains(QStringLiteral("QScrollBar::handle:horizontal")));
        QVERIFY(popup->styleSheet().contains(QStringLiteral("border: %1px solid #778899")
                                                 .arg(UiConfig::PopupOutlineStrokeWidth)));
        QVERIFY(popup->styleSheet().contains(QStringLiteral("border-radius: 11px")));
    }

    if (QWidget* popup = QApplication::activePopupWidget())
        popup->hide();
}

void TestEditorUi::inactive_editor_does_not_show_completion_popup()
{
    QWidget container;
    QVBoxLayout layout(&container);
    Editor firstEditor;
    Editor secondEditor;
    layout.addWidget(&firstEditor);
    layout.addWidget(&secondEditor);
    container.show();
    QVERIFY(QTest::qWaitForWindowExposed(&container));

    firstEditor.setFocus();
    firstEditor.setText(QStringLiteral("co"));
    firstEditor.setCursorPosition(firstEditor.text().size());
    secondEditor.setFocus();
    QCoreApplication::processEvents();

    QVERIFY(QMetaObject::invokeMethod(
        &firstEditor,
        "triggerAutoComplete",
        Qt::DirectConnection));
    QCoreApplication::processEvents();

    QVERIFY(s_completionPopupTree() == nullptr);
    QTest::qWait(250);
    QVERIFY(s_completionPopupTree() == nullptr);
}

void TestEditorUi::enter_evaluates_when_cursor_is_immediately_after_operator()
{
    struct Case {
        QString expression;
        QChar op;
    };

    const QList<Case> cases = {
        { QStringLiteral("2 + 3"), QLatin1Char('+') },
        { QString::fromUtf8("2 − 3"), QChar(MathDsl::SubOp) },
        { QStringLiteral("2 / 3"), QLatin1Char('/') },
        { QString::fromUtf8("2 × 3"), QChar(MathDsl::MulCrossOp) },
        { QString::fromUtf8("2 [rad·s⁻¹]"), QChar(MathDsl::MulDotOp) },
        { QStringLiteral("2 [rad/s]"), QLatin1Char('/') },
    };

    for (const Case& c : cases) {
        Editor editor;
        editor.show();
        QVERIFY(QTest::qWaitForWindowExposed(&editor));
        editor.setFocus();

        editor.setText(c.expression);
        const int opPos = c.expression.indexOf(c.op);
        QVERIFY2(opPos >= 0, qPrintable(QStringLiteral("Operator not found in expression: %1").arg(c.expression)));
        editor.setCursorPosition(opPos + 1);

        QSignalSpy returnPressedSpy(&editor, SIGNAL(returnPressed()));

        QTest::keyClick(&editor, Qt::Key_Return, Qt::NoModifier);
        QCoreApplication::processEvents();

        QCOMPARE(returnPressedSpy.count(), 1);
        QCOMPARE(editor.text(), c.expression);
    }
}

void TestEditorUi::completion_popup_uses_expected_icons_for_all_symbol_types()
{
    Settings* settings = Settings::instance();
    const bool builtInFnBackup = settings->autoCompletionBuiltInFunctions;
    const bool builtInVarBackup = settings->autoCompletionBuiltInVariables;
    const bool userFnBackup = settings->autoCompletionUserFunctions;
    const bool userVarBackup = settings->autoCompletionUserVariables;
    settings->autoCompletionBuiltInFunctions = true;
    settings->autoCompletionBuiltInVariables = true;
    settings->autoCompletionUserFunctions = true;
    settings->autoCompletionUserVariables = true;
    struct SettingsRestoreGuard {
        Settings* settings;
        bool builtInFn;
        bool builtInVar;
        bool userFn;
        bool userVar;
        ~SettingsRestoreGuard()
        {
            settings->autoCompletionBuiltInFunctions = builtInFn;
            settings->autoCompletionBuiltInVariables = builtInVar;
            settings->autoCompletionUserFunctions = userFn;
            settings->autoCompletionUserVariables = userVar;
        }
    } settingsRestoreGuard {settings, builtInFnBackup, builtInVarBackup, userFnBackup, userVarBackup};

    Session session;
    Evaluator* evaluator = session.evaluator();
    evaluator->unsetAllUserUnits();
    evaluator->unsetAllUserFunctions();
    evaluator->unsetVariable(QStringLiteral("icon_user_var"));
    evaluator->unsetVariable(QStringLiteral("s"));
    evaluator->setExpression(QStringLiteral("icon_user_var = 7"));
    QVERIFY(!evaluator->eval().isNan());
    evaluator->setExpression(QStringLiteral("s = 9"));
    QVERIFY(!evaluator->eval().isNan());
    evaluator->setVariable(QStringLiteral("ans"), Quantity(9), Variable::BuiltIn);
    evaluator->setUserFunction(UserFunction(
        QStringLiteral("icon_user_func"),
        QStringList() << QStringLiteral("t"),
        QStringLiteral("t")));
    evaluator->setExpression(QStringLiteral("2[m]"));
    const Quantity iconUserUnitValue = evaluator->eval();
    QVERIFY(!iconUserUnitValue.isNan());
    evaluator->setUserUnit(UserUnit(
        QStringLiteral("icon_user_unit"),
        iconUserUnitValue,
        QStringLiteral("2[m]"),
        QStringLiteral("[icon_user_unit]=2[m]")));

    Editor editor;
    editor.setSession(&session);
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("si"));
    editor.setCursorPosition(editor.text().size());
    QVERIFY(QMetaObject::invokeMethod(&editor, "triggerAutoComplete", Qt::DirectConnection));
    QTreeWidget* popup = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((popup = s_completionPopupTree()) != nullptr, 1000);
    QCOMPARE(s_popupSymbolForIdentifier(popup, QStringLiteral("sin")), QString::fromUtf8("📚 ƒ"));
    popup->hide();

    editor.setText(QStringLiteral("icon_user_f"));
    editor.setCursorPosition(editor.text().size());
    QVERIFY(QMetaObject::invokeMethod(&editor, "triggerAutoComplete", Qt::DirectConnection));
    popup = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((popup = s_completionPopupTree()) != nullptr, 1000);
    QCOMPARE(s_popupSymbolForIdentifier(popup, QStringLiteral("icon_user_func")), QString::fromUtf8("👤 ƒ"));
    popup->hide();

    editor.setText(QStringLiteral("a"));
    editor.setCursorPosition(editor.text().size());
    QVERIFY(QMetaObject::invokeMethod(&editor, "triggerAutoComplete", Qt::DirectConnection));
    popup = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((popup = s_completionPopupTree()) != nullptr, 1000);
    QCOMPARE(s_popupSymbolForIdentifier(popup, QStringLiteral("ans")), QString::fromUtf8("📏 𝑘"));
    popup->hide();

    editor.setText(QStringLiteral("icon_user_"));
    editor.setCursorPosition(editor.text().size());
    QVERIFY(QMetaObject::invokeMethod(&editor, "triggerAutoComplete", Qt::DirectConnection));
    popup = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((popup = s_completionPopupTree()) != nullptr, 1000);
    QCOMPARE(s_popupSymbolForIdentifier(popup, QStringLiteral("icon_user_var")), QString::fromUtf8("👤 𝑥"));
    popup->hide();

    editor.setText(QStringLiteral("s"));
    editor.setCursorPosition(editor.text().size());
    QVERIFY(QMetaObject::invokeMethod(&editor, "triggerAutoComplete", Qt::DirectConnection));
    popup = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((popup = s_completionPopupTree()) != nullptr, 1000);
    QCOMPARE(s_popupSymbolForIdentifier(popup, QStringLiteral("s")), QString::fromUtf8("👤 𝑥"));
    popup->hide();

    editor.setText(QStringLiteral("[met"));
    editor.setCursorPosition(editor.text().size());
    QVERIFY(QMetaObject::invokeMethod(&editor, "triggerAutoComplete", Qt::DirectConnection));
    popup = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((popup = s_completionPopupTree()) != nullptr, 1000);
    QCOMPARE(s_popupSymbolForIdentifier(popup, QStringLiteral("metre")), QString::fromUtf8("📚 𝒖"));
    popup->hide();

    editor.setText(QStringLiteral("[icon_user_u"));
    editor.setCursorPosition(editor.text().size());
    QVERIFY(QMetaObject::invokeMethod(&editor, "triggerAutoComplete", Qt::DirectConnection));
    popup = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((popup = s_completionPopupTree()) != nullptr, 1000);
    QCOMPARE(s_popupSymbolForIdentifier(popup, QStringLiteral("icon_user_unit")), QString::fromUtf8("👤 𝒖"));
    popup->hide();

}

void TestEditorUi::wrap_selection_method_wraps_selected_text()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("1+2"));
    QTextCursor cursor = editor.textCursor();
    cursor.setPosition(0);
    cursor.setPosition(1, QTextCursor::KeepAnchor);
    editor.setTextCursor(cursor);

    editor.wrapSelection();
    QCOMPARE(editor.text(), QStringLiteral("(1)+2"));
}

void TestEditorUi::wrap_selection_method_wraps_whole_expression_without_selection()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("1+2"));
    editor.setCursorPosition(0);

    editor.wrapSelection();
    QCOMPARE(editor.text(), QStringLiteral("(1+2)"));
}

void TestEditorUi::matching_parentheses_use_parens_and_generated_foreground_colors()
{
    const QColor parensColor(QStringLiteral("#c8a2c8"));
    const QColor resultBackground(QStringLiteral("#102030"));
    const QColor generatedForeground = aaForegroundForBackground(parensColor);
    const ColorScheme scheme = ColorScheme::fromJsonObject(themeJson(QJsonObject{
        {QStringLiteral("parens"), parensColor.name()},
        {QStringLiteral("background"), QStringLiteral("#010203")}
    }));
    QVERIFY(scheme.isValid());

    Editor editor;
    editor.setThemeSurfaceColor(QColor(QStringLiteral("#203040")), resultBackground);
    editor.setThemePreviewColorScheme(scheme);
    editor.setText(QStringLiteral("()"));
    editor.setCursorPosition(editor.text().size());

    QVERIFY(QMetaObject::invokeMethod(
        &editor,
        "doMatchingPar",
        Qt::DirectConnection));

    const QList<QTextEdit::ExtraSelection> selections = editor.extraSelections();
    QCOMPARE(selections.count(), 2);
    for (const QTextEdit::ExtraSelection& selection : selections) {
        QCOMPARE(selection.format.background().color(), parensColor);
        QCOMPARE(selection.format.foreground().color(), generatedForeground);
        QVERIFY(selection.format.foreground().color() != resultBackground);
    }
}

void TestEditorUi::keeps_wrapped_cursor_line_visible_at_height_cap()
{
    Editor editor;
    editor.setFixedWidth(220);
    editor.resize(220, 80);
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral(
        "12 + 312 + 3 + 12 + 312 + 3 + 123 + 123 + 12 + 3 + 123 + "
        "123 + 14 + 2 + 123 + 12 + 3 + 12 + 3 + 12 + 3 + "
        "345345345 + 345345 + 345345345 + 345345345453453453434 + "
        "234 + 234 + 234 + 23423423 + 4234 + 234"));
    editor.setCursorPosition(editor.text().size());

    QTRY_VERIFY(editor.document()->begin().layout()->lineCount() > 5);
    QTRY_VERIFY(editor.cursorRect().top() >= editor.viewport()->rect().top());
    QTRY_VERIFY(editor.cursorRect().bottom() <= editor.viewport()->rect().bottom());
}

void TestEditorUi::rewrites_operators_in_scrolled_expression_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("token");
    QTest::addColumn<int>("cursorOffset");
    QTest::addColumn<bool>("inputMethod");
    QTest::addColumn<int>("style");
    const QString conversion = MathDsl::buildWrappedToken(MathDsl::TransOp, MathDsl::SubWrapSp)
        + QStringLiteral("[]");
    for (int style : {int(Settings::EditorDisplayModern), int(Settings::EditorDisplayClassic)}) {
        const QByteArray prefix = style == Settings::EditorDisplayModern ? "modern-" : "classic-";
        QTest::newRow(QByteArray(prefix + "double-minus").constData())
            << QStringLiteral("--") << conversion << 1 << false << style;
        QTest::newRow(QByteArray(prefix + "arrow").constData())
            << QStringLiteral("->") << conversion << 1 << false << style;
        QTest::newRow(QByteArray(prefix + "power").constData())
            << QStringLiteral("**") << QStringLiteral("^") << 0 << false << style;
        QTest::newRow(QByteArray(prefix + "superscript").constData())
            << QStringLiteral("^2") << QString(MathDsl::Pow2) << 0 << false << style;
        QTest::newRow(QByteArray(prefix + "ime-superscript").constData())
            << QStringLiteral("^2") << QString(MathDsl::Pow2) << 0 << true << style;
    }
}

void TestEditorUi::rewrites_operators_in_scrolled_expression()
{
    QFETCH(QString, input);
    QFETCH(QString, token);
    QFETCH(int, cursorOffset);
    QFETCH(bool, inputMethod);
    QFETCH(int, style);
    Settings::instance()->editorDisplayStyle = Settings::EditorDisplayStyle(style);
    Editor editor;
    editor.setAutoCompletionEnabled(false);
    editor.setAutoCalcEnabled(false);
    editor.setFixedWidth(220);
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    const QString expression = QStringLiteral("1234567890+").repeated(40) + QStringLiteral("1");
    editor.setText(expression);
    editor.setCursorPosition(editor.text().size());
    QVERIFY(editor.document()->firstBlock().layout()->lineCount() > 5);
    QVERIFY(editor.verticalScrollBar()->value() > 0);

    const auto type = [&](const QString& text) {
        if (inputMethod) {
            QInputMethodEvent event;
            event.setCommitString(text);
            QApplication::sendEvent(&editor, &event);
        } else {
            QTest::keyClicks(&editor, text);
        }
    };
    type(input.left(1));
    const QString beforeRewrite = editor.document()->toRawText();
    type(input.mid(1));
    const QString afterRewrite = editor.document()->toRawText();
    QCOMPARE(afterRewrite, expression + token);
    QCOMPARE(editor.textCursor().position(), afterRewrite.size() - cursorOffset);
    QVERIFY(editor.cursorRect().top() >= 0);
    QVERIFY(editor.cursorRect().bottom() < editor.viewport()->height());

    editor.undo();
    QCOMPARE(editor.document()->toRawText(), beforeRewrite);
    editor.undo();
    QCOMPARE(editor.document()->toRawText(), expression);
    editor.redo();
    editor.redo();
    QCOMPARE(editor.document()->toRawText(), afterRewrite);
    QVERIFY(editor.cursorRect().top() >= 0);
    QVERIFY(editor.cursorRect().bottom() < editor.viewport()->height());
}

void TestEditorUi::classic_editor_keeps_wrapped_input_visible_and_preserves_undo()
{
    Settings* settings = Settings::instance();
    settings->autoCalc = false;
    settings->editorDisplayStyle = Settings::EditorDisplayClassic;
    Editor editor;
    editor.setFixedWidth(220);
    editor.rehighlight();
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();
    editor.setText(QStringLiteral("2 [gypq]"));
    editor.setCursorPosition(editor.text().size());
    const int singleLineHeight = editor.height();
    const int singleLineTextHeight = EditorGeometryTestHelpers::visibleTextHeight(editor);
    QVERIFY(editor.cursorRect().bottom() < editor.viewport()->height());
    editor.setText(QStringLiteral("1\n2\n3\n4\n5"));
    QCOMPARE(editor.height() - singleLineHeight,
             EditorGeometryTestHelpers::visibleTextHeight(editor) - singleLineTextHeight);
    const QString input = QStringLiteral("1234567890+").repeated(30);
    editor.setText(input);
    editor.setCursorPosition(input.size());
    QTRY_VERIFY(editor.document()->firstBlock().layout()->lineCount() > 5);
    QTRY_COMPARE(editor.height() - singleLineHeight,
                 EditorGeometryTestHelpers::visibleTextHeight(editor) - singleLineTextHeight);
    QTRY_VERIFY(editor.cursorRect().top() >= 0);
    QTRY_VERIFY(editor.cursorRect().bottom() < editor.viewport()->height());
    QTest::keyClicks(&editor, QStringLiteral("2"));
    QCOMPARE(editor.text(), input + QStringLiteral("2"));
    settings->editorDisplayStyle = Settings::EditorDisplayModern;
    editor.rehighlight();
    QTRY_VERIFY(editor.cursorRect().top() >= 0);
    QTRY_VERIFY(editor.cursorRect().bottom() < editor.viewport()->height());
    editor.undo();
    QCOMPARE(editor.text(), input);
    settings->editorDisplayStyle = Settings::EditorDisplayClassic;
    editor.rehighlight();
    editor.redo();
    QCOMPARE(editor.text(), input + QStringLiteral("2"));
    QTRY_VERIFY(editor.cursorRect().top() >= 0);
    QTRY_VERIFY(editor.cursorRect().bottom() < editor.viewport()->height());
}

void TestEditorUi::classic_completion_popups_follow_text_inset_data()
{
    QTest::addColumn<bool>("constant");
    QTest::newRow("symbol-completion") << false;
    QTest::newRow("constant-completion") << true;
}

void TestEditorUi::classic_completion_popups_follow_text_inset()
{
    QFETCH(bool, constant);
    Settings::instance()->editorDisplayStyle = Settings::EditorDisplayClassic;
    Editor editor;
    editor.setAutoCompletionEnabled(false);
    editor.setFixedWidth(340);
    editor.rehighlight();
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();
    const QRect screen = editor.screen()->availableGeometry();
    editor.move(screen.left() + 20, screen.top() + 80);
    editor.setText(constant ? QString() : QStringLiteral("co"));
    editor.setCursorPosition(editor.text().size());
    QCoreApplication::processEvents();
    QWidget* popup = nullptr;
    if (constant) {
        QTest::keyClick(&editor, Qt::Key_Space, Qt::ControlModifier);
        popup = QApplication::activePopupWidget();
    } else {
        auto* completion = editor.findChild<EditorCompletion*>();
        QVERIFY(completion);
        completion->showCompletion({QStringLiteral("cos:Cosine"),
                                    QStringLiteral("cosh:Hyperbolic cosine")});
        popup = s_completionPopupTree();
    }
    QVERIFY(popup);
    const int textX = editor.mapToGlobal(QPoint(UiConfig::ClassicTextInset, 0)).x();
    QCOMPARE(popup->geometry().left(), textX);
    QVERIFY(screen.contains(popup->geometry()));
    popup->hide();
}

void TestEditorUi::editor_height_adds_only_one_line_height_per_visible_line()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));

    editor.setText(QStringLiteral("1"));
    const int singleLineHeight = editor.height();
    const int singleLineTextHeight = EditorGeometryTestHelpers::visibleTextHeight(editor);

    editor.setText(QStringLiteral("1\n2\n3\n4\n5"));
    QCOMPARE(editor.height() - singleLineHeight,
             EditorGeometryTestHelpers::visibleTextHeight(editor) - singleLineTextHeight);
}

void TestEditorUi::editor_keeps_unit_descenders_inside_viewport()
{
    Editor editor;
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    editor.setText(QStringLiteral("2 [gypq]"));
    editor.setCursorPosition(editor.text().size());

    QVERIFY(editor.cursorRect().bottom() < editor.viewport()->rect().bottom());
}

void TestEditorUi::adding_second_wrapped_character_keeps_first_line_visible()
{
    Editor editor;
    editor.setFixedWidth(260);
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();

    const QString prefix(QStringLiteral("12345678901234567890"));
    editor.setText(prefix);
    editor.setCursorPosition(editor.text().size());

    while (editor.document()->begin().layout()->lineCount() < 2)
        editor.insert(QStringLiteral("1"));

    const int firstWrappedScrollValue = editor.verticalScrollBar()->value();
    editor.insert(QStringLiteral("2"));

    QCOMPARE(editor.document()->begin().layout()->lineCount(), 2);
    QCOMPARE(firstWrappedScrollValue, 0);
    QCOMPARE(editor.verticalScrollBar()->value(), 0);
}

void TestEditorUi::editor_fill_color_is_15_percent_lighter_for_dark_background_role()
{
    Settings* settings = Settings::instance();
    const QString oldColorScheme = settings->colorScheme;
    const QString oldCustomColorSchemeJson = settings->customColorSchemeJson;

    QJsonObject colors;
    colors.insert(QStringLiteral("background"), QStringLiteral("#202020"));
    settings->colorScheme = QStringLiteral("Custom");
    settings->customColorSchemeJson = themeJsonString(colors);

    Editor editor;
    editor.rehighlight();

    const QColor expected = QColor(QStringLiteral("#202020")).lighter(115);
    const QColor activeBase = editor.palette().color(QPalette::Active, QPalette::Base);
    const QColor inactiveBase = editor.palette().color(QPalette::Inactive, QPalette::Base);
    QCOMPARE(activeBase.name(QColor::HexArgb), expected.name(QColor::HexArgb));
    QCOMPARE(inactiveBase.name(QColor::HexArgb), expected.name(QColor::HexArgb));

    settings->colorScheme = oldColorScheme;
    settings->customColorSchemeJson = oldCustomColorSchemeJson;
}

void TestEditorUi::editor_fill_color_is_15_percent_darker_for_light_background_role()
{
    Settings* settings = Settings::instance();
    const QString oldColorScheme = settings->colorScheme;
    const QString oldCustomColorSchemeJson = settings->customColorSchemeJson;

    QJsonObject colors;
    colors.insert(QStringLiteral("background"), QStringLiteral("#d0d0d0"));
    settings->colorScheme = QStringLiteral("Custom");
    settings->customColorSchemeJson = themeJsonString(colors);

    Editor editor;
    editor.rehighlight();

    const QColor expected = QColor(QStringLiteral("#d0d0d0")).darker(115);
    const QColor activeBase = editor.palette().color(QPalette::Active, QPalette::Base);
    const QColor inactiveBase = editor.palette().color(QPalette::Inactive, QPalette::Base);
    QCOMPARE(activeBase.name(QColor::HexArgb), expected.name(QColor::HexArgb));
    QCOMPARE(inactiveBase.name(QColor::HexArgb), expected.name(QColor::HexArgb));

    settings->colorScheme = oldColorScheme;
    settings->customColorSchemeJson = oldCustomColorSchemeJson;
}

void TestEditorUi::themed_cursor_has_only_two_states_at_display_scale_data()
{
    QTest::addColumn<qreal>("scale");
    QTest::newRow("100-percent") << qreal(1);
    QTest::newRow("125-percent") << qreal(1.25);
    QTest::newRow("150-percent") << qreal(1.5);
    QTest::newRow("200-percent") << qreal(2);
}

void TestEditorUi::themed_cursor_has_only_two_states_at_display_scale()
{
    QFETCH(qreal, scale);
    Editor editor;
    editor.setAutoCalcEnabled(false);
    editor.setAutoCompletionEnabled(false);
    const QColor primary(QStringLiteral("#ff3377"));
    editor.setThemePrimaryColor(primary, false);
    editor.setText(QStringLiteral("123"));
    editor.setCursorPosition(3);
    editor.resize(400, editor.height());
    editor.show();
    editor.activateWindow();
    editor.setFocus();
    QTRY_VERIFY(editor.hasFocus());

    const QRect cursor = editor.cursorRect();
    const QRect themedCursor(cursor.x() + (cursor.width() - 2) / 2,
                             cursor.y(), 2, cursor.height());
    const QRect sample(qFloor((themedCursor.left() - 2) * scale),
                       qFloor(themedCursor.center().y() * scale),
                       qCeil(6 * scale), 1);
    const auto render = [&]() {
        QImage image(QSize(qCeil(editor.viewport()->width() * scale),
                           qCeil(editor.viewport()->height() * scale)),
                     QImage::Format_ARGB32_Premultiplied);
        image.setDevicePixelRatio(scale);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        editor.viewport()->render(&painter);
        return image;
    };
    editor.setCustomCursorVisible(false);
    const QImage hidden = render();
    QImage visible = hidden.copy();
    {
        QPainter painter(&visible);
        painter.fillRect(themedCursor, primary);
    }
    const QImage hiddenSample = hidden.copy(sample);
    const QImage visibleSample = visible.copy(sample);
    QVERIFY(hiddenSample != visibleSample);
    // Even a hidden custom cursor must not leave Qt's independently blinking
    // zero-width cursor behind at fractional display scales.
    const int flashTime = qMax(1000, QApplication::cursorFlashTime());
    QElapsedTimer elapsed;
    elapsed.start();
    while (elapsed.elapsed() < flashTime) {
        QVERIFY(render().copy(sample) == hiddenSample);
        QTest::qWait(20);
    }

    editor.setCustomCursorVisible(true);
    bool sawVisible = false;
    bool sawHidden = false;
    elapsed.restart();
    while (elapsed.elapsed() < flashTime * 2) {
        const QImage actual = render().copy(sample);
        QVERIFY2(actual == visibleSample || actual == hiddenSample,
                 "Cursor shows an extra color or width during its blink cycle");
        sawVisible |= actual == visibleSample;
        sawHidden |= actual == hiddenSample;
        QTest::qWait(20);
    }
    QVERIFY(sawVisible);
    QVERIFY(sawHidden);
}

void TestEditorUi::non_blinking_cursor_stays_visible_data()
{
    QTest::addColumn<qreal>("scale");
    QTest::newRow("100-percent") << qreal(1);
    QTest::newRow("125-percent") << qreal(1.25);
    QTest::newRow("150-percent") << qreal(1.5);
}

void TestEditorUi::non_blinking_cursor_stays_visible()
{
    QFETCH(qreal, scale);
    Editor editor;
    editor.setAutoCalcEnabled(false);
    editor.setAutoCompletionEnabled(false);
    const QColor primary(QStringLiteral("#ff3377"));
    editor.setThemePrimaryColor(primary, false);
    editor.setText(QStringLiteral("123"));
    editor.setCursorPosition(3);
    editor.resize(400, editor.height());
    editor.show();
    editor.activateWindow();
    editor.setFocus();
    QTRY_VERIFY(editor.hasFocus());
    QVERIFY(editor.isCursorBlinkingEnabled());

    const auto cursorPixel = [&]() {
        QImage image(QSize(qCeil(editor.viewport()->width() * scale),
                           qCeil(editor.viewport()->height() * scale)),
                     QImage::Format_ARGB32_Premultiplied);
        image.setDevicePixelRatio(scale);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        editor.viewport()->render(&painter);
        const QRect cursor = editor.cursorRect();
        const int cursorX = cursor.x() + (cursor.width() - 2) / 2 + 1;
        return image.pixelColor(qFloor(cursorX * scale), qFloor(cursor.center().y() * scale));
    };
    const int flashTime = qMax(1000, QApplication::cursorFlashTime());
    QTRY_VERIFY_WITH_TIMEOUT(cursorPixel().rgba() != primary.rgba(), flashTime * 2);
    editor.setCursorBlinkingEnabled(false);
    QCOMPARE(cursorPixel().rgba(), primary.rgba());

    // Editing, focus changes and theme updates must keep the cursor steady.
    QTest::keyClick(&editor, Qt::Key_Left);
    QTest::keyClick(&editor, Qt::Key_4);
    editor.clearFocus();
    QVERIFY(cursorPixel().rgba() != primary.rgba());
    editor.setFocus();
    QTRY_VERIFY(editor.hasFocus());
    editor.setThemePrimaryColor(primary, false);
    QElapsedTimer elapsed;
    elapsed.start();
    while (elapsed.elapsed() < flashTime * 2) {
        QCOMPARE(cursorPixel().rgba(), primary.rgba());
        QTest::qWait(20);
    }

    editor.setCustomCursorVisible(false);
    QVERIFY(cursorPixel().rgba() != primary.rgba());
    editor.setCustomCursorVisible(true);
    QCOMPARE(cursorPixel().rgba(), primary.rgba());
    editor.setCursorBlinkingEnabled(true);
    QTRY_VERIFY_WITH_TIMEOUT(cursorPixel().rgba() != primary.rgba(), flashTime * 2);
    QTRY_COMPARE_WITH_TIMEOUT(cursorPixel().rgba(), primary.rgba(), flashTime * 2);
}

void TestEditorUi::cursor_blinking_survives_settings_reload()
{
    Settings* settings = Settings::instance();
    QVERIFY(settings->cursorBlinking);
    for (bool enabled : {false, true}) {
        settings->cursorBlinking = enabled;
        settings->save();
        settings->cursorBlinking = !enabled;
        settings->load();
        QCOMPARE(settings->cursorBlinking, enabled);
        Editor editor;
        QCOMPARE(editor.isCursorBlinkingEnabled(), enabled);
    }
    settings->cursorBlinking = false;
    QVERIFY(settings->resetToDefaults());
    QVERIFY(settings->cursorBlinking);
}

void TestEditorUi::cursor_free_painting_preserves_text_and_selections_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("selectText");
    QTest::newRow("placeholder") << QString() << false;
    QTest::newRow("syntax-highlighting") << QStringLiteral("sin(123) + 456") << false;
    QTest::newRow("selected-wrapped-text")
        << QStringLiteral("123 + 456 + 789 + 123 + 456 + 789 + 123 + 456\n123 + 456") << true;
    QTest::newRow("full-width-selection") << QStringLiteral("123\n456\n789") << false;
}

void TestEditorUi::cursor_free_painting_preserves_text_and_selections()
{
    QFETCH(QString, text);
    QFETCH(bool, selectText);
    class NativePaintingEditor : public Editor {
    protected:
        void paintEvent(QPaintEvent* event) override { QPlainTextEdit::paintEvent(event); }
    };
    NativePaintingEditor reference;
    Editor editor;
    for (Editor* target : { static_cast<Editor*>(&reference), &editor }) {
        target->setAutoCalcEnabled(false);
        target->setAutoCompletionEnabled(false);
        target->setThemePrimaryColor(QColor(QStringLiteral("#ff3377")), false);
        target->setPlaceholderText(QStringLiteral("Enter an expression"));
        target->setText(text);
        target->setCursorPosition(0);
        target->setCustomCursorVisible(false);
        target->setTextInteractionFlags(Qt::TextSelectableByMouse);
        target->resize(220, target->height());
        target->show();
        if (selectText)
            target->selectAll();
        if (qstrcmp(QTest::currentDataTag(), "full-width-selection") == 0) {
            QTextEdit::ExtraSelection selection;
            selection.cursor = target->textCursor();
            selection.cursor.setPosition(4);
            selection.format.setBackground(QColor(QStringLiteral("#557799")));
            selection.format.setProperty(QTextFormat::FullWidthSelection, true);
            target->setExtraSelections({ selection });
            // Exercise explicit block backgrounds as well as extra selections.
            QTextCursor blockCursor(target->document());
            QTextBlockFormat format;
            format.setBackground(QColor(QStringLiteral("#774455")));
            blockCursor.setBlockFormat(format);
        }
    }
    QCoreApplication::processEvents();
    reference.clearFocus();
    editor.clearFocus();
    const QImage expected = reference.viewport()->grab().toImage();
    const QImage actual = editor.viewport()->grab().toImage();
    QVERIFY2(actual == expected,
             "Cursor-free painting changes Qt's text, background, or selection rendering");
}

void TestEditorUi::input_digit_grouping_formats_only_integer_digits_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QList<int>>("positions");
    QTest::newRow("short") << QStringLiteral("999") << QList<int>{};
    QTest::newRow("thousand") << QStringLiteral("1000") << QList<int>{0};
    QTest::newRow("million") << QStringLiteral("1234567") << QList<int>{0, 3};
    QTest::newRow("fraction") << QStringLiteral("1234.56789") << QList<int>{0};
    QTest::newRow("comma-fraction") << QStringLiteral("1234,56789") << QList<int>{0};
    QTest::newRow("fraction-only") << QStringLiteral("0.1234567") << QList<int>{};
    QTest::newRow("scientific") << QStringLiteral("1000e1234") << QList<int>{0};
    QTest::newRow("hex") << QStringLiteral("0x12345") << QList<int>{2};
    QTest::newRow("hex-alias") << QStringLiteral("#12345") << QList<int>{1};
    QTest::newRow("binary") << QStringLiteral("0b10101010") << QList<int>{5};
    QTest::newRow("octal") << QStringLiteral("0o1234567") << QList<int>{2, 5};
    QTest::newRow("decimal-prefix") << QStringLiteral("0d1234567") << QList<int>{2, 5};
    QTest::newRow("expression") << QStringLiteral("1000 + 20000") << QList<int>{0, 8};
    QTest::newRow("identifier-comment") << QStringLiteral("x1234 ? 1000") << QList<int>{};
    QTest::newRow("unit-power") << QStringLiteral("1000[m²] + 2000") << QList<int>{0, 11};
    QTest::newRow("manual-spaces") << QStringLiteral("1 234 567") << QList<int>{};
    QTest::newRow("manual-underscores") << QStringLiteral("1_234_567") << QList<int>{};
}

void TestEditorUi::input_digit_grouping_formats_only_integer_digits()
{
    QFETCH(QString, input);
    QFETCH(QList<int>, positions);
    Settings* settings = Settings::instance();
    settings->numberFormatStyle = Settings::NumberFormatThreeDigitSpaceDot;
    settings->applyNumberFormatStyle();
    QPlainTextEdit inputEdit;
    SyntaxHighlighter inputHighlighter(&inputEdit, true);
    inputEdit.setPlainText(input);
    QPlainTextEdit display;
    SyntaxHighlighter displayHighlighter(&display);
    display.setPlainText(input);

    auto groupedPositions = [](QPlainTextEdit& edit) {
        QList<int> result;
        for (const auto& range : edit.document()->firstBlock().layout()->formats()) {
            if (range.format.hasProperty(SyntaxHighlighter::InputDigitSeparator)) {
                for (int pos = range.start; pos < range.start + range.length; ++pos)
                    result.append(pos);
            }
        }
        return result;
    };
    for (bool highlighting : {true, false}) {
        settings->syntaxHighlighting = highlighting;
        for (bool grouping : {false, true, false}) {
            settings->inputDigitGrouping = grouping;
            inputHighlighter.rehighlight();
            displayHighlighter.rehighlight();
            QCOMPARE(groupedPositions(inputEdit), grouping ? positions : QList<int>{});
            QVERIFY(groupedPositions(display).isEmpty());
            QCOMPARE(inputEdit.toPlainText(), input);
        }
    }
}

void TestEditorUi::input_digit_grouping_preserves_typing_selection_and_undo()
{
    Settings::instance()->numberFormatStyle = Settings::NumberFormatThreeDigitSpaceDot;
    Settings::instance()->applyNumberFormatStyle();
    Settings::instance()->inputDigitGrouping = true;
    Editor editor;
    editor.setAutoCalcEnabled(false);
    editor.setAutoCompletionEnabled(false);
    editor.show();
    QTest::keyClicks(&editor, "1000");
    editor.rehighlight();
    QCOMPARE(editor.text(), QStringLiteral("1000"));
    QCOMPARE(editor.textCursor().position(), 4);
    bool hasGap = false;
    for (const auto& range : editor.document()->firstBlock().layout()->formats())
        hasGap |= range.format.hasProperty(SyntaxHighlighter::InputDigitSeparator);
    QVERIFY(hasGap);

    QTest::keyClick(&editor, Qt::Key_Left);
    QCOMPARE(editor.textCursor().position(), 3);
    QTest::keyClick(&editor, Qt::Key_Backspace);
    QCOMPARE(editor.text(), QStringLiteral("100"));
    editor.undo();
    QCOMPARE(editor.text(), QStringLiteral("1000"));
    editor.redo();
    QCOMPARE(editor.text(), QStringLiteral("100"));
    editor.undo();
    editor.selectAll();
    editor.copy();
    QCOMPARE(QApplication::clipboard()->text(), QStringLiteral("1000"));
    const QTextCursor before = editor.textCursor();
    Settings::instance()->inputDigitGrouping = false;
    editor.rehighlight();
    QCOMPARE(editor.textCursor().position(), before.position());
    QCOMPARE(editor.textCursor().anchor(), before.anchor());
    editor.undo();
    QCOMPARE(editor.text(), QStringLiteral("100"));
}

void TestEditorUi::input_digit_grouping_follows_number_format_data()
{
    QTest::addColumn<int>("style");
    QTest::addColumn<QString>("expected");
    QTest::newRow("ungrouped-dot") << int(Settings::NumberFormatNoGroupingDot) << QStringLiteral("1234567.1234567");
    QTest::newRow("ungrouped-comma") << int(Settings::NumberFormatNoGroupingComma) << QStringLiteral("1234567,1234567");
    QTest::newRow("si-dot") << int(Settings::NumberFormatSIDot) << QStringLiteral("1 234 567.123 456 7");
    QTest::newRow("si-comma") << int(Settings::NumberFormatSIComma) << QStringLiteral("1 234 567,123 456 7");
    QTest::newRow("comma-dot") << int(Settings::NumberFormatThreeDigitCommaDot) << QStringLiteral("1,234,567.1234567");
    QTest::newRow("comma-dot-fraction") << int(Settings::NumberFormatThreeDigitCommaDotFraction) << QStringLiteral("1,234,567.123,456,7");
    QTest::newRow("dot-comma") << int(Settings::NumberFormatThreeDigitDotComma) << QStringLiteral("1.234.567,1234567");
    QTest::newRow("dot-comma-fraction") << int(Settings::NumberFormatThreeDigitDotCommaFraction) << QStringLiteral("1.234.567,123.456.7");
    QTest::newRow("space-dot") << int(Settings::NumberFormatThreeDigitSpaceDot) << QStringLiteral("1 234 567.1234567");
    QTest::newRow("space-comma") << int(Settings::NumberFormatThreeDigitSpaceComma) << QStringLiteral("1 234 567,1234567");
    QTest::newRow("underscore-dot") << int(Settings::NumberFormatThreeDigitUnderscoreDot) << QStringLiteral("1_234_567.1234567");
    QTest::newRow("underscore-dot-fraction") << int(Settings::NumberFormatThreeDigitUnderscoreDotFraction) << QStringLiteral("1_234_567.123_456_7");
    QTest::newRow("underscore-comma") << int(Settings::NumberFormatThreeDigitUnderscoreComma) << QStringLiteral("1_234_567,1234567");
    QTest::newRow("underscore-comma-fraction") << int(Settings::NumberFormatThreeDigitUnderscoreCommaFraction) << QStringLiteral("1_234_567,123_456_7");
    QTest::newRow("indian") << int(Settings::NumberFormatIndianCommaDot) << QStringLiteral("12,34,567.1234567");
}

void TestEditorUi::input_digit_grouping_follows_number_format()
{
    QFETCH(int, style);
    QFETCH(QString, expected);
    Settings* settings = Settings::instance();
    settings->numberFormatStyle = static_cast<Settings::NumberFormatStyle>(style);
    settings->applyNumberFormatStyle();
    settings->inputDigitGrouping = true;
    const QString raw = QStringLiteral("1234567") + QChar(settings->decimalSeparator())
        + QStringLiteral("1234567");
    Editor editor;
    editor.setText(raw);
    editor.rehighlight();
    const auto displayedText = [&editor]() {
        QString display = editor.text();
        const auto ranges = editor.document()->firstBlock().layout()->formats();
        for (auto it = ranges.crbegin(); it != ranges.crend(); ++it) {
            const QString radix = it->format.stringProperty(SyntaxHighlighter::InputRadixReplacement);
            if (!radix.isEmpty())
                display.replace(it->start, 1, radix);
            const QString separator = it->format.stringProperty(SyntaxHighlighter::InputDigitSeparator);
            if (!separator.isEmpty()) {
                for (int pos = it->start + it->length - 1; pos >= it->start; --pos)
                    display.insert(pos + 1, separator);
            }
        }
        return display;
    };
    for (bool syntaxHighlighting : {true, false}) {
        settings->syntaxHighlighting = syntaxHighlighting;
        editor.rehighlight();
        QCOMPARE(displayedText(), expected);
        QCOMPARE(editor.text(), raw);
        QCOMPARE(NumberFormatter::formatNumericLiteralForDisplay(raw), expected);
    }
    if (settings->decimalSeparator() == ',') {
        editor.setText(QStringLiteral("1234567.1234567"));
        editor.rehighlight();
        QCOMPARE(displayedText(), QString(expected).replace(QLatin1Char(','), QLatin1Char('.')));
    }
    editor.setText(expected);
    editor.rehighlight();
    QCOMPARE(displayedText(), expected);
    settings->inputDigitGrouping = false;
    editor.setText(raw);
    editor.rehighlight();
    QCOMPARE(displayedText(), raw);
}

void TestEditorUi::input_digit_grouping_paints_separator_glyphs_data()
{
    QTest::addColumn<bool>("themed");
    QTest::addColumn<bool>("selected");
    QTest::newRow("native") << false << false;
    QTest::newRow("native-selected") << false << true;
    QTest::newRow("themed") << true << false;
    QTest::newRow("themed-selected") << true << true;
}

void TestEditorUi::input_digit_grouping_paints_separator_glyphs()
{
    QFETCH(bool, themed);
    QFETCH(bool, selected);
    Settings* settings = Settings::instance();
    settings->inputDigitGrouping = true;
    Editor editor;
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(20);
    editor.setFont(font);
    editor.setAutoCalcEnabled(false);
    editor.setAutoCompletionEnabled(false);
    editor.resize(600, 120);
    editor.show();
    editor.clearFocus();
    editor.setThemePrimaryColor(themed ? QColor(QStringLiteral("#2277aa")) : QColor(), false);
    for (auto style : {Settings::NumberFormatThreeDigitCommaDot,
                       Settings::NumberFormatThreeDigitDotComma,
                       Settings::NumberFormatThreeDigitUnderscoreDot}) {
        settings->numberFormatStyle = style;
        settings->applyNumberFormatStyle();
        editor.setText(QStringLiteral("1234567.56"));
        editor.rehighlight();
        if (selected)
            editor.selectAll();
        QCoreApplication::processEvents();
        editor.clearFocus();
        const auto separatorWidth = [&editor]() {
            for (const auto& range : editor.document()->firstBlock().layout()->formats()) {
                if (range.format.hasProperty(SyntaxHighlighter::InputDigitSeparator))
                    return range.format.fontLetterSpacing();
            }
            return qreal(0);
        };
        const QString separator = settings->digitGroupingSeparator();
        QCOMPARE(separatorWidth(), QFontMetricsF(editor.font()).horizontalAdvance(separator));
        editor.increaseFontPointSize();
        // Hinting can give adjacent point sizes the same glyph width.
        // The gap must follow the current font's metrics in either case.
        QCOMPARE(separatorWidth(), QFontMetricsF(editor.font()).horizontalAdvance(separator));
        QVERIFY(separatorWidth() > 0);
        const QImage withSeparators = editor.viewport()->grab().toImage();
        QTextLayout* layout = editor.document()->firstBlock().layout();
        auto formats = layout->formats();
        for (auto& range : formats) {
            range.format.clearProperty(SyntaxHighlighter::InputDigitSeparator);
            range.format.clearProperty(SyntaxHighlighter::InputRadixReplacement);
        }
        layout->setFormats(formats);
        editor.viewport()->update();
        const QImage withoutSeparators = editor.viewport()->grab().toImage();
        QVERIFY2(withSeparators != withoutSeparators, "Input separators must paint glyphs, not just gaps");
    }
}

void TestEditorUi::history_recall_respects_input_adjustments_data()
{
    QTest::addColumn<QString>("stored");
    QTest::addColumn<QString>("raw");
    QTest::addColumn<bool>("adjustments");
    const QList<QPair<QString, QString>> expressions {
        {QString::fromUtf8("2³"), QStringLiteral("2^3")},
        {QString::fromUtf8("2¹²"), QStringLiteral("2^12")},
        {QString::fromUtf8("2⁻³"), QStringLiteral("2^(-3)")},
        {QString::fromUtf8("sin²(1)"), QStringLiteral("sin(1)^2")},
        {QString::fromUtf8("sin²(1)³"), QStringLiteral("(sin(1)^2)^3")},
        {QString::fromUtf8("2[m²/s⁻³]"), QStringLiteral("2[m^2/s^(-3)]")},
        {QString::fromUtf8("2 × 3 · 4 / 2 − 1"), QStringLiteral("2 * 3 * 4 / 2 - 1")},
        {QString::fromUtf8("2[m] → [cm]"), QStringLiteral("2[m] -> [cm]")},
        {QString::fromUtf8("2³ ?  4² × pi -> [m] ?\t"),
         QString::fromUtf8("2^3 ?  4² × pi -> [m] ?\t")}
    };
    for (int i = 0; i < expressions.size(); ++i) {
        for (bool adjustments : {false, true}) {
            QTest::newRow(qPrintable(QStringLiteral("%1-%2").arg(i).arg(adjustments)))
                << expressions.at(i).first << expressions.at(i).second << adjustments;
        }
    }
}

void TestEditorUi::history_recall_respects_input_adjustments()
{
    QFETCH(QString, stored);
    QFETCH(QString, raw);
    QFETCH(bool, adjustments);
    Settings::instance()->automaticInputAdjustments = adjustments;
    Session session;
    session.addHistoryEntry(HistoryEntry(QStringLiteral("7"), Quantity(7)));
    session.addHistoryEntry(HistoryEntry(stored, Quantity(8)));
    Editor editor;
    editor.setSession(&session);
    editor.setAutoCompletionEnabled(false);
    editor.setAutoCalcEnabled(false);
    const QString draft = QString::fromUtf8("unfinished² ? keep ×");
    editor.setText(draft);
    QVERIFY(QMetaObject::invokeMethod(&editor, "historyBack"));
    const QString expected = adjustments ? stored : raw;
    QCOMPARE(editor.text(), expected);
    QCOMPARE(editor.cursorPosition(), expected.size());
    QVERIFY(QMetaObject::invokeMethod(&editor, "historyBack"));
    QCOMPARE(editor.text(), QStringLiteral("7"));
    QVERIFY(QMetaObject::invokeMethod(&editor, "historyForward"));
    QCOMPARE(editor.text(), expected);
    QVERIFY(QMetaObject::invokeMethod(&editor, "historyForward"));
    QCOMPARE(editor.text(), draft);
    QCOMPARE(session.historyEntryAtRef(1).expr(), stored);

    Evaluator evaluator;
    evaluator.setExpression(evaluator.autoFix(stored));
    const QString originalResult = NumberFormatter::format(evaluator.eval());
    QVERIFY2(evaluator.error().isEmpty(), qPrintable(evaluator.error()));
    evaluator.setExpression(evaluator.autoFix(expected));
    QCOMPARE(NumberFormatter::format(evaluator.eval()), originalResult);
    QVERIFY2(evaluator.error().isEmpty(), qPrintable(evaluator.error()));
}

void TestEditorUi::raw_input_preserves_text_data()
{
    QTest::addColumn<QString>("input");
    QTest::newRow("operators") << QStringLiteral("1+2*3/4--5**2");
    QTest::newRow("powers") << QStringLiteral("2^3 + 2[m^2]");
    QTest::newRow("spaces-and-brackets") << QStringLiteral("  2 (3)  [m] ?text");
    QTest::newRow("invalid-characters") << QStringLiteral("@|++^^[ +3?{}]");
    QTest::newRow("decimal-and-units") << QStringLiteral(".5[m / s^2]");
    QTest::newRow("unicode") << QString::fromUtf8("2 × 3 ÷ 4 − 5 → [µm²]");
}

void TestEditorUi::raw_input_preserves_text()
{
    QFETCH(QString, input);
    Settings::instance()->automaticInputAdjustments = false;
    for (int source = 0; source < 4; ++source) {
        Editor editor;
        editor.setAutoCompletionEnabled(false);
        editor.setAutoCalcEnabled(false);
        switch (source) {
        case 0:
            // Qt's keyClicks only supports ASCII. Send Unicode text through
            // key events too, as keyboard layouts can produce these glyphs.
            for (QChar ch : input) {
                QKeyEvent key(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QString(ch));
                QApplication::sendEvent(&editor, &key);
            }
            break;
        case 1:
            for (QChar ch : input) {
                QInputMethodEvent commit;
                commit.setCommitString(QString(ch));
                QApplication::sendEvent(&editor, &commit);
            }
            break;
        case 2:
            editor.insert(input);
            break;
        case 3:
            editor.setText(input);
            break;
        }
        QCOMPARE(editor.text(), input);
        editor.setCursorPosition(editor.text().size());
        QInputMethodEvent replacement;
        replacement.setCommitString(QStringLiteral("*^@"), -1, 1);
        QApplication::sendEvent(&editor, &replacement);
        QCOMPARE(editor.text(), input.chopped(1) + QStringLiteral("*^@"));
        editor.selectAll();
        QApplication::clipboard()->setText(input);
        QSignalSpy bulkEvaluation(&editor, &Editor::bulkEvaluationStarted);
        editor.paste();
        QCOMPARE(editor.text(), input.trimmed());
        QCOMPARE(bulkEvaluation.count(), 0);
        editor.undo();
        QCOMPARE(editor.text(), input.chopped(1) + QStringLiteral("*^@"));
        editor.redo();
        QCOMPARE(editor.text(), input.trimmed());
    }
}

void TestEditorUi::raw_input_keeps_character_navigation_and_deletion()
{
    Settings::instance()->automaticInputAdjustments = false;
    Editor editor;
    editor.setText(QStringLiteral("1 + 2"));
    editor.setCursorPosition(4);
    QTest::keyClick(&editor, Qt::Key_Left);
    QCOMPARE(editor.cursorPosition(), 3);
    QTest::keyClick(&editor, Qt::Key_Right);
    QCOMPARE(editor.cursorPosition(), 4);
    QTest::keyClick(&editor, Qt::Key_Backspace);
    QCOMPARE(editor.text(), QStringLiteral("1 +2"));
    editor.undo();
    editor.setCursorPosition(1);
    editor.doDelete();
    QCOMPARE(editor.text(), QStringLiteral("1+ 2"));
    editor.undo();
    editor.setCursorPosition(4);
    editor.doBackspace();
    QCOMPARE(editor.text(), QStringLiteral("1 +2"));
    editor.undo();
    editor.setCursorPosition(1);
    QTest::keyClick(&editor, Qt::Key_Delete);
    QCOMPARE(editor.text(), QStringLiteral("1+ 2"));
}

void TestEditorUi::raw_input_preserves_calculator_commands()
{
    Settings::instance()->automaticInputAdjustments = false;
    Editor editor;
    editor.setAutoCompletionEnabled(false);
    QTest::keyClicks(&editor, "2^3");
    QCOMPARE(editor.text(), QStringLiteral("2^3"));
    QSignalSpy enter(&editor, &Editor::returnPressed);
    QSignalSpy escape(&editor, &Editor::escapePressed);
    QTest::keyClick(&editor, Qt::Key_Return);
    QCOMPARE(enter.count(), 1);
    QCOMPARE(editor.text(), QStringLiteral("2^3"));
    QTest::keyClick(&editor, Qt::Key_Escape);
    QCOMPARE(escape.count(), 1);
    QTest::keyClick(&editor, Qt::Key_Tab);
    QCOMPARE(editor.text(), QStringLiteral("2^3"));
    editor.setHistoryArrowNavigationEnabled(false);
    QTest::keyClick(&editor, Qt::Key_Home);
    QTest::keyClick(&editor, Qt::Key_Right, Qt::ShiftModifier);
    QCOMPARE(editor.textCursor().selectedText(), QStringLiteral("2"));
    QTest::keyClicks(&editor, "@");
    QCOMPARE(editor.text(), QStringLiteral("@^3"));
}

void TestEditorUi::raw_input_switch_preserves_document_and_restores_adjustments()
{
    Settings::instance()->automaticInputAdjustments = false;
    Editor editor;
    editor.setAutoCompletionEnabled(false);
    editor.setAutoCalcEnabled(false);
    editor.setText(QStringLiteral("1+2*3^4"));
    editor.setCursorPosition(3);
    QTextCursor cursor = editor.textCursor();
    cursor.setPosition(5, QTextCursor::KeepAnchor);
    editor.setTextCursor(cursor);
    QSignalSpy changed(&editor, &Editor::textChanged);
    Settings::instance()->automaticInputAdjustments = true;
    editor.refreshInputMode();
    QCOMPARE(changed.count(), 0);
    QCOMPARE(editor.text(), QStringLiteral("1+2*3^4"));
    QCOMPARE(editor.textCursor().position(), 5);
    QCOMPARE(editor.textCursor().anchor(), 3);
    editor.clear();
    QTest::keyClicks(&editor, "2^3");
    QCOMPARE(editor.text(), QString::fromUtf8("2³"));
    Settings::instance()->automaticInputAdjustments = false;
    editor.refreshInputMode();
    QTest::keyClicks(&editor, "+4");
    QCOMPARE(editor.text(), QString::fromUtf8("2³+4"));
}

void TestEditorUi::raw_input_preserves_visible_digit_grouping()
{
    Settings* settings = Settings::instance();
    settings->automaticInputAdjustments = false;
    settings->inputDigitGrouping = true;
    settings->digitGrouping = 3;
    settings->numberFormatStyle = Settings::NumberFormatThreeDigitCommaDot;
    settings->applyNumberFormatStyle();
    Editor editor;
    editor.setAutoCompletionEnabled(false);
    editor.setAutoCalcEnabled(false);
    editor.setText(QStringLiteral("1234567.123456"));
    editor.setCustomCursorVisible(false);
    editor.refreshInputMode();
    editor.setFixedWidth(500);
    editor.show();
    QCoreApplication::processEvents();
    bool hasSeparator = false;
    for (const auto& range : editor.document()->firstBlock().layout()->formats()) {
        hasSeparator |= range.format.hasProperty(SyntaxHighlighter::InputDigitSeparator);
    }
    QVERIFY(hasSeparator);
    const QImage raw = editor.viewport()->grab().toImage();
    settings->automaticInputAdjustments = true;
    editor.refreshInputMode();
    QCOMPARE(raw, editor.viewport()->grab().toImage());
    settings->automaticInputAdjustments = false;
    editor.refreshInputMode();
    QCOMPARE(raw, editor.viewport()->grab().toImage());
    settings->inputDigitGrouping = false;
    settings->digitGrouping = 0;
    editor.rehighlight();
    QVERIFY(raw != editor.viewport()->grab().toImage());
    for (const auto& range : editor.document()->firstBlock().layout()->formats())
        QVERIFY(!range.format.hasProperty(SyntaxHighlighter::InputDigitSeparator));
    QCOMPARE(editor.text(), QStringLiteral("1234567.123456"));
}

void TestEditorUi::raw_input_preserves_tab_completion_data()
{
    QTest::addColumn<bool>("rawInput");
    QTest::newRow("automatic-editing") << false;
    QTest::newRow("raw-input") << true;
}

void TestEditorUi::raw_input_preserves_tab_completion()
{
    QFETCH(bool, rawInput);
    Settings::instance()->automaticInputAdjustments = !rawInput;
    Editor editor;
    editor.setAutoCalcEnabled(false);
    editor.show();
    QVERIFY(QTest::qWaitForWindowExposed(&editor));
    editor.setFocus();
    QTest::keyClicks(&editor, "co");
    QVERIFY(QMetaObject::invokeMethod(&editor, "triggerAutoComplete", Qt::DirectConnection));
    QTreeWidget* popup = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((popup = s_completionPopupTree()) != nullptr, 1000);
    QTreeWidgetItem* selected = nullptr;
    for (int i = 0; i < popup->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = popup->topLevelItem(i);
        if (item->text(1) == QStringLiteral("cos")) {
            selected = item;
            break;
        }
    }
    QVERIFY(selected);
    popup->setCurrentItem(selected);
    QTest::keyClick(&editor, Qt::Key_Tab);
    QCOMPARE(editor.text(), QStringLiteral("cos") + MathDsl::GroupStart + MathDsl::GroupEnd);
    QCOMPARE(editor.cursorPosition(), editor.text().size() - 1);
    QTRY_VERIFY_WITH_TIMEOUT(s_editorOrViewportHasFocus(&editor), 1000);
}

void TestEditorUi::automatic_input_adjustments_survive_settings_reload()
{
    Settings* settings = Settings::instance();
    QVERIFY(settings->automaticInputAdjustments);
    for (bool enabled : {true, false}) {
        settings->automaticInputAdjustments = enabled;
        settings->save();
        settings->automaticInputAdjustments = !enabled;
        settings->load();
        QCOMPARE(settings->automaticInputAdjustments, enabled);
    }
    settings->automaticInputAdjustments = false;
    QVERIFY(settings->resetToDefaults());
    QVERIFY(settings->automaticInputAdjustments);
}

void TestEditorUi::input_digit_grouping_survives_settings_reload()
{
    Settings* settings = Settings::instance();
    QVERIFY(settings->inputDigitGrouping);
    for (bool enabled : {true, false}) {
        settings->inputDigitGrouping = enabled;
        settings->numberFormatStyle = Settings::NumberFormatThreeDigitCommaDot;
        settings->applyNumberFormatStyle();
        QCOMPARE(settings->inputDigitGrouping, enabled);
        settings->save();
        settings->inputDigitGrouping = !enabled;
        settings->load();
        QCOMPARE(settings->inputDigitGrouping, enabled);
    }
}

int main(int argc, char** argv)
{
    return UiTestFixture::run<TestEditorUi>(argc, argv);
}
#include "testeditorui.moc"
