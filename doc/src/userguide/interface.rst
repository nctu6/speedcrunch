.. meta::
   :description: Explore SpeedCrunch's interface, session tabs and panes, calculation history, result formats, complex numbers, keyboard shortcuts and appearance settings.

User Interface
==============

Shortcut notation: :kbd:`Ctrl` means :kbd:`Ctrl` on Windows/Linux and
:kbd:`Cmd` on macOS. Platform-specific shortcuts spell out :kbd:`Cmd` when
they differ from this convention.

Widgets
-------

Apart from the main display, SpeedCrunch offers a number of extra panels, referred to as *widgets* here.
Most of them are dockable panels that can be moved around the main window and enabled or disabled via the
:menuselection:`View` menu.

Panels docked on the left or right fill the available height. Panels above
or below the display and keypad fit between the side panels.

Use the cross in a dock panel's title bar to hide the panel, or the square
to detach it into a floating window. Hidden panels can be shown again via
the :menuselection:`View` menu. Opening another panel leaves floating panels
in their own windows.

On Linux with Wayland, drag a floating panel by its title bar to move it.
Use its square button or double-click the title bar to attach it to the
main window again.

Drag a table's column headers to change their order. Each window keeps its
column order between launches, including for panels that are hidden.

* Formula Book
    The formula book provides access to commonly used formulas and calculations. Simply insert
    a formula into the expression editor by clicking on it.

    You can help expanding the formula book by posting your requests to the `issue tracker <tracker_>`_.

* Constants
    The constants widget shows a large list of scientific constants (including CODATA-based entries),
    grouped by category and subcategory.

    The columns show each constant's name, value, and unit.
    Use the ``Domain`` and ``Subdomain`` selectors to narrow the list, or search by name or unit.
    Press :kbd:`Up` and :kbd:`Down` in the search field to select a result while continuing to type.
    Press :kbd:`Enter` or double-click an entry to insert its value and unit and return to the editor.
    The mathematical constants pi and Euler's number are inserted as ``π`` and ``e``.

* User Variables
    The variables widget lists all :ref:`user-defined variables <variables>`. Any of them can be inserted into the editor by double-clicking it.
    Additionally, it is possible to delete a variable by selecting it and pressing the :kbd:`Delete` key on your keyboard.

* Functions and User Functions
    Similar to the variables widget, these show built-in and :ref:`user-defined functions <user_functions>` respectively.

* User Units
    Similar to the user functions widget, this lists :ref:`user-defined units <user_units>`.
    Double-click inserts ``[name]`` in the editor, and the context menu supports edit/delete operations.

* Bit Field
    The bit field dock is designed to make working with binary numbers easier. It shows a field of 64 squares,
    each representing a bit in the current result. Any bit can be toggled by clicking its square; the resulting
    number is automatically inserted into the editor. Additional buttons allow you to shift, invert and reset all the bits at once.
    While the mouse cursor is hovering over the bit field, scrolling the mouse wheel will also shift the bits.

  .. _keypad:
* Keypad
    The on-screen keypad provides clickable input for common operations and symbols. It supports multiple presets
    and a custom layout, so you can adapt it to your workflow. You can also adjust the keypad zoom level for better
    readability or touch area on touchscreens. For fast and full-featured input, SpeedCrunch's keyboard interface is still recommended.

    Available entries in :menuselection:`View --> Keypad` are:

    * ``Basic`` (**default**)
    * ``Scientific (wide)``
    * ``Scientific (narrow)``
    * ``Custom...``
    * ``Disable`` (shown as ``Disabled`` when selected)

    Both scientific layouts include ``ln`` for the natural logarithm and
    ``log10`` for the common (base-10) logarithm. These buttons insert
    ``ln(`` and ``log10(``, respectively. Copying a scientific preset into a
    custom layout uses the same labels and function names.

    Since version 1.1, both scientific layouts show ``ⁿ√`` for the nth root
    in place of the cube-root button. It inserts ``root(`` so you can enter
    a root index and a value, for example ``root(5; 32)``. Copying either
    scientific preset into a custom layout includes this button.

    The scientific layouts also include ``x²``, which inserts ``^2``, and
    ``;`` to separate function arguments. These replace the ``x`` and ``x=``
    shortcuts in version 1.1. Related buttons sit together: ``√`` and ``x²``,
    ``xʸ`` and ``ⁿ√``, ``E`` and ``exp``, ``ln`` and ``log10``, and each
    trigonometric function with its inverse.

    Available entries in :menuselection:`Settings --> Appearance --> Keypad --> Zoom` are:

    * ``50%``
    * ``100%`` (**default**)
    * ``150%``
    * ``200%``

    The zoom level scales the keypad buttons, including their text. Each window
    keeps its own zoom level. The Zoom submenu is unavailable while the keypad
    is disabled.
    You can also right-click the keypad and choose :menuselection:`Zoom`.

    ``Custom...`` opens a dialog where you can define the keypad matrix size (rows and columns)
    and configure each button individually. For each position in the matrix, you can set:

    * The button label shown in the keypad
    * The button behavior: ``Insert text``, ``Backspace``, ``Clear expression``, or ``Evaluate expression``
    * The text to insert (for ``Insert text`` behavior)

    The dialog also includes a ``Copy preset`` selector with an :guilabel:`Apply` button,
    so you can start from the ``Basic``, ``Scientific (wide)``, or
    ``Scientific (narrow)`` layout and then customize it.

    Custom keypad settings are saved and restored automatically.

    You can also right-click directly on the keypad to open the same menu as
    :menuselection:`View --> Keypad`.

    .. versionadded:: 1.0

    .. versionchanged:: 0.11
       The keypad was removed in SpeedCrunch 0.11; however, it was added back in 0.12.

* History
    The history widget lists all previous inputs. Double-click a line to recall it. Note that the main result display also provides this functionality.

* Main Menu
    Toggle the main menu bar visibility via :menuselection:`View --> Main Menu`.
    This option is available on Windows and Linux.
    On macOS, this option is not available because the application menu is managed by the system menu bar (outside the window), so SpeedCrunch cannot hide it like on Windows and Linux.
    On Windows and Linux, when the top main menu is hidden, a navigable Main Menu
    (mirroring the top menu structure) is available from the right-click context
    menu on the result-display.

    .. versionadded:: 1.0

.. _tracker: https://speedcrunch.org/issues.html


Session Tabs and Panes
----------------------

New blank tabs receive an ``Untitled-XXX`` name that is not already used in any
open window, including by empty tabs that have not been saved.

When a pane contains more than one session, SpeedCrunch shows a tab bar above
the result display. Click a tab to switch sessions, drag tabs horizontally to
reorder them within the pane, or drag a tab away from the tab bar to move it to
another pane. Dropping a dragged tab outside the current window creates a new
window for that session; dropping it on a pane in another SpeedCrunch window
moves it there.
Use :menuselection:`Session --> Rename...` to rename the active session.
You can also double-click its tab, choose :menuselection:`Rename...`
from the tab context menu, or choose :menuselection:`Session --> Rename...`
from the result display context menu.
Choose :menuselection:`Session --> Clone...` to copy the active session under
a new name, including its unfinished expression. This command is also available
as :menuselection:`Clone...` in the tab context menu and
:menuselection:`Session --> Clone...` in the result display context menu.
Use :menuselection:`Session --> New Window` to open a blank
session in a new window that copies the current window and dock layout.
Choose :menuselection:`Session --> Close Tab` to close the active tab, or
:menuselection:`Session --> Reopen Closed Tab` to restore the last ordinary tab
you closed. Private tabs cannot be reopened.
Both commands are also available from the tab context menu.
When a floating dock has focus, session commands apply to its window.
Use :menuselection:`View --> Split` to add a pane in any direction, and
:menuselection:`View --> Close Pane` to close the active pane.
The tab context menu also provides :menuselection:`Split` and
:menuselection:`Close Pane`.

The tab context menu's :menuselection:`Clear` command removes the session's
calculations, unfinished expression, variables, functions, and units after
confirmation. Global user definitions are kept. :menuselection:`Delete` removes
the session itself after confirmation.
When SpeedCrunch restores multiple windows after a restart, each window keeps
its own dock layout, dock visibility, keypad visibility, layout and zoom,
and—when window position saving is enabled—its own size and position.


Private Sessions
----------------

Use :menuselection:`Session --> New Private Tab` or :kbd:`Ctrl+Shift+N`
(:kbd:`Cmd+Shift+N` on macOS) to open a session that stays in memory.
Private sessions are named ``Private-1``, ``Private-2``, and so on,
using the first available number. A shield icon marks their tabs. The window
title also identifies the active private session, even after you rename it.
The tab bar is hidden when a private session is the only session in a single
pane, just as it is for an ordinary session.

Private sessions do not save their history, unfinished expressions, variables,
functions, or units automatically. They are excluded from the workspace
restored at the next launch. Closing a private session discards its contents,
and it cannot be reopened with the Restore Closed Tab shortcut. Copying and
explicit export remain available. Cloning a private session creates another
private session.

Choose :menuselection:`Session --> Keep` to save a private
session, including its current contents. It receives the first available
``Untitled-XXX`` name and behaves like an ordinary session from then on.

Choose :menuselection:`Session --> Make Private...` to convert an ordinary
session. SpeedCrunch asks for confirmation before deleting its saved JSON file.
Its contents stay open under the first available ``Private-XXX`` name until you
close it or exit. If the file cannot be deleted, the session remains ordinary.
If a private session cannot be saved during conversion, it remains private.

These actions are also available from the tab and result display context menus.
In the result display, open the :guilabel:`Session` submenu and choose
:guilabel:`Keep` or :guilabel:`Make Private...`.
Privacy belongs to the session, so renaming it does not change whether it is
saved. Ordinary sessions can remain open alongside private ones.

Enable :menuselection:`Settings --> Window --> Open a Private Tab on Startup`
to open and focus a fresh private tab whenever SpeedCrunch starts. This option
is off by default. Saved ordinary sessions are restored as usual, with one new
private tab in the active window, even when several windows are restored.
An empty active ``Untitled-XXX`` session is used for the private tab instead of
opening an extra ordinary tab. Sessions with history, unfinished expressions,
or their own definitions stay open. When there are no sessions to restore,
SpeedCrunch starts with just the private tab. Opening another window while
the app is running still creates an ordinary session.

.. versionadded:: 1.1


Expression Editor Features
--------------------------

Turn off :menuselection:`Settings --> Editing --> Automatic Input Adjustments`
to enter text without automatic text adjustments or character restrictions.

The expression editor provides some advanced features:

* Undo and redo
    Choose :menuselection:`Edit --> Undo` or :menuselection:`Edit --> Redo`.
    Press :kbd:`Ctrl+Z` to undo the last edit. On Windows/Linux, use
    :kbd:`Ctrl+Shift+Z` or :kbd:`Ctrl+Y` to redo it. On macOS, use
    :kbd:`Cmd+Shift+Z`. Each key press is a separate undo step.
    Automatic changes made by that key, such as inserting a closing bracket
    or converting an exponent to superscript, are undone together. Text
    committed through an input method is undone as a single edit.

    :menuselection:`Edit --> Cut` cuts selected input text. Copy also works on
    selected result text. These actions are disabled when unavailable.
    :menuselection:`Edit --> Wrap in Parentheses` wraps the selected input, or
    the whole expression if nothing is selected.

* Automatic closing parentheses and square brackets
    Typing ``(`` or ``[`` with no text selected inserts the matching closing
    character only when the cursor is at the end of the expression or only
    whitespace follows it. The cursor stays between the pair. If non-whitespace
    text follows the cursor, only the opening character is inserted.

* Autocompletion
    If you start typing a name (e.g. of a variable, function, or unit), a pop-up with matching names will appear below the editor, aligned with the start of
    the name being completed. It appears above the editor when there is insufficient space below and shifts horizontally to stay on screen.
    The list becomes shorter and scrollable when needed to avoid covering the editor.
    On Wayland with Qt 6.6 or later, the compositor chooses below or above according to the available screen space.
    With older Qt versions on Wayland, the list stays within the application window.
    Alignment follows the visible text even when the expression wraps or scrolls. Press :kbd:`Tab` to insert the current
    suggestion. Pressing :kbd:`Enter` evaluates the expression by default. If you explicitly navigate the popup with arrow/page/home/end keys or click a
    suggestion with the mouse, then :kbd:`Enter` accepts that selected suggestion.
    Enable :menuselection:`Settings --> Editing --> Autocomplete --> Accept with Enter`
    to accept the current suggestion with :kbd:`Enter` immediately, without
    first using the arrow keys. Pressing :kbd:`Escape` dismisses the autocomplete popup.

* Function signature tooltip
    While typing a function call, the live result area can show the function signature (parameter list). The parameter currently being edited is highlighted.
    This also works for user-defined functions. Pressing :kbd:`Escape` will dismiss the tooltip.

* Quick constant insertion
    Press :kbd:`Ctrl+Space` to search the same constants as the constants widget (see above).
    Type a name or unit to filter the list. Each result shows its unit beside the name using the theme's unit color.
    Press :kbd:`Up` and :kbd:`Down` to select a result. Press :kbd:`Tab` or :kbd:`Enter`, or click a result, to insert it and return to the editor.
    :kbd:`Shift+Tab` moves focus backward. With no matches, :kbd:`Tab` leaves the popup open.
    Press :kbd:`Escape` to cancel. Search text does not change the expression.
    The list follows the cursor's horizontal position and uses the same below-or-above placement rules as autocompletion.

    .. versionadded:: 1.1
       Search in the quick constant popup and select dock search results with :kbd:`Up` and :kbd:`Down`.

  .. _context-help:
* Context help
    Pressing :kbd:`F1` will show the manual page for the function under the cursor, providing quick access to detailed
    usage information for a function. Pressing :kbd:`Escape` will dismiss the manual window again.

* Selection results
    If :menuselection:`Settings --> Results --> Show Live Result Preview` is enabled, selecting a partial expression in the expression editor or in the result display will show you the result of the selected expression.
    You can dismiss this preview by pressing :kbd:`Escape` or by clicking the :guilabel:`×` button at the top-right corner of the preview.


Import/Export
-------------

SpeedCrunch automatically stores your sessions and lets you restore them with :menuselection:`Session --> Open`.
Use :menuselection:`Session --> Open Sessions Folder` to open the folder where SpeedCrunch stores these session files
when you want to back them up or clean them up manually.
You can also export the current session as JSON (:menuselection:`Session --> Export --> JSON`) and later import it
with :menuselection:`Session --> Import`. The data is stored in a SpeedCrunch-specific file format. [#f1]_ While the
session files are human-readable, they are designed for use by SpeedCrunch. If you want to export your calculations
to work on them in another program or hand them to a colleague, the other export options are preferable.

Import validates the selected JSON file as a SpeedCrunch session and opens it as a new tab. If the file is not valid
SpeedCrunch session JSON, SpeedCrunch shows an error and leaves the current session unchanged.

You can export the session as HTML (:menuselection:`Session --> Export --> HTML`). The resulting file will consist of the contents of the result
display and can be viewed in any web browser. This feature can also be used to print a SpeedCrunch session by printing the exported
HTML document. Since the syntax highlighting and color scheme are maintained in the HTML output, it is recommended to select a color scheme
with a white background (e.g. *Standard*) prior to exporting if you intend to print the document.

The final, most basic option is to export your session as a plain text file (:menuselection:`Session --> Export --> Plain text`).
In contrast to the HTML export option, the syntax highlighting will be lost.

User Definitions
++++++++++++++++++++++++

.. versionadded:: 1.0

To define user variables, user functions and user units that are loaded globally, use
:menuselection:`Settings --> User Definitions...`.

This dialog provides:

* A multi-line editor (one definition per line), with syntax highlighting and line numbers.
* A clear reminder that these definitions are global, loaded into every session and override same-name session definitions.
* :guilabel:`Apply` to apply definitions immediately.
* :guilabel:`Validate` to validate and preview results without applying.

When applying or testing, SpeedCrunch reports:

* Imported variable count
* Imported function count
* Imported unit count
* Line numbers with errors

Invalid definitions are ignored. Variable definitions that evaluate to ``NaN`` are also ignored.


Settings
--------

SpeedCrunch's behavior can be customized to a large degree using the configuration options in the
:menuselection:`Settings` menu. This section explains the settings that are available.

Resetting Settings
++++++++++++++++++

Choose :menuselection:`Settings --> Reset All Settings...` to restore the default
preferences. SpeedCrunch asks for confirmation before making any changes.
The reset applies to all open windows immediately and is saved for the next
launch. It includes appearance, editing options, number and result formats,
language, and window preferences, including dock visibility and keypad settings.

Your calculation history, open session tabs and panes, and user definitions are
kept. Existing calculations keep their recorded result formats. New calculations
and live previews use the default settings.

.. versionadded:: 1.1

Result Display Interactions
+++++++++++++++++++++++++++

The result display supports mouse-driven interactions for navigation and editing.

* Right-click context menu
    Right-clicking an expression/calculation block in the result display opens a context menu
    for that specific block.
    The :guilabel:`Copy` submenu starts with :guilabel:`Selection` to copy the
    selected text. :guilabel:`Select All` selects all text in the display.
    Open the :guilabel:`Copy` submenu and choose :guilabel:`Calculation`
    to copy all displayed lines of the calculation at once, including the
    expression, any simplification, and every result format.
    The lines are copied as plain text in the order shown, without the blank
    line between calculations. :guilabel:`Expression` and
    :guilabel:`Result` let you copy either part separately.
    :guilabel:`Remove` offers :guilabel:`This Calculation`,
    :guilabel:`All Calculations Above`, and :guilabel:`All Calculations Below`.
    This menu includes an entry to toggle the main menu bar visibility.
    Use its :guilabel:`Split` submenu to add a pane to the left, right, above,
    or below the current pane.
    :guilabel:`New Tab` and :guilabel:`New Private Tab` create sessions directly.
    The :guilabel:`Session` submenu starts with :guilabel:`Open...`,
    :guilabel:`Import...`, and :guilabel:`Export`. It also groups actions to
    duplicate, rename, change privacy, clear, delete, or close the current session.

    .. versionadded:: 1.0

    The :guilabel:`Copy` submenu also offers :guilabel:`Calculation as Single Line` to
    join all displayed lines of the calculation with spaces, for example
    ``120 / 8 = 15``.

    .. versionadded:: 1.1

* Hover highlighting of calculation blocks
    When :menuselection:`Settings --> Appearance --> Hover Highlighting` is enabled, moving the mouse over
    the result display highlights the calculation block under the cursor.
    The highlighted block also shows quick-action icons for deleting, editing, and copying.

    .. versionadded:: 1.0

* History rewriting from highlighted blocks
    From the hovered/highlighted calculation block, you can re-edit an earlier expression and apply it
    as a history rewrite. SpeedCrunch then recalculates all expressions below that edited entry.

    .. versionadded:: 1.0

* Calculation settings from highlighted blocks
    The calculation settings action opens a tabular layout with one mandatory
    ``Main Line`` row and four optional extra result-line rows. The table
    shows line enablement, notation, and decimal places together so the stored
    calculation context can be adjusted before SpeedCrunch
    recalculates the affected history.

.. _result_format:

Notation
+++++++++++++

This section allows selecting the result notation to use (for example in
:menuselection:`Settings --> Results --> Notation & Precision...` or from the
status-bar notation selector). You can select one of the following options.
On first launch, the default is :menuselection:`Automatic decimal`.

* :menuselection:`Automatic decimal`
    Use fixed-point decimal form for most results; values with exponent larger than 14 or smaller than -9 are shown in
    scientific notation. With a finite precision setting, very small values may switch to scientific notation earlier.
* :menuselection:`Fixed-point decimal`
    Display results in fixed-point decimal form. For excessively
    large or small numbers, this format may still fall back to scientific notation.
* :menuselection:`Engineering decimal`
    Display results in engineering notation. This is a variant of :ref:`scientific notation <scientific_notation>` in which
    the exponent is divisible by three.
* :menuselection:`Scientific decimal`
    Display results in :ref:`normalized scientific notation <scientific_notation>`.
* :menuselection:`Rational`
    Try to display real results as fractions (for example, ``1/3``) using continued-fraction approximation with bounded denominator. If no close match is found, results are displayed in decimal form.
    For common trigonometric and inverse-trigonometric results, SpeedCrunch prefers
    symbolic exact forms such as ``pi/2``, ``7⋅pi/6``, ``1/2``, ``sqrt(2)/2``,
    and ``sqrt(3)/3``.
    Example::

        arcsin(1)
        = pi/2

        sin(45)    (angle mode = degree)
        = √(2)/2

        arcsin(sqrt(3)/22)
        = 0.07881114207211010205

    .. versionadded:: 1.0
* :menuselection:`Binary`
    Display results as binary numbers, i.e. in base-2.
* :menuselection:`Octal`
    Display all results as octal numbers, i.e. in base-8.
* :menuselection:`Hexadecimal`
    Display all results as hexadecimal numbers, i.e. in base-16.
* :menuselection:`Sexagesimal`
    Display dimensionless and time results as :ref:`sexagesimal values <sexagesimal_values>`, i.e. with minutes and seconds. All other results are displayed in fixed-point decimal form.

    .. versionadded:: 1.0

Angle Mode
++++++++++

Select the angular unit to be used in calculations. For functions that operate on angles, notably the
:ref:`trigonometric functions <trigonometric>` like :func:`sin` or :func:`cos`, this setting
determines the angle format of the arguments.

* :menuselection:`Radian`
    Use radians for angles. A full circle corresponds to an angle of 2π radians.
* :menuselection:`Degree`
    Use degrees for angles. A full circle corresponds to an angle of 360°.
* :menuselection:`Gradian`
    Use gradians for angles. A full circle corresponds to an angle of 400 gradians.

    .. versionadded:: 1.0

* :menuselection:`Turn`
    Use turns for angles. A full circle corresponds to an angle of 1 turn.
* :menuselection:`Revolution`
    Use revolutions for angles. A full circle corresponds to an angle of
    1 revolution.

    .. versionadded:: 1.0
    
.. _complex_numbers:

Complex Numbers
+++++++++++++++

Choose a result form via :menuselection:`Settings --> Results --> Complex Form`
and a symbol via :menuselection:`Settings --> Results --> Imaginary Unit`.
Complex numbers are always enabled.

* :menuselection:`Imaginary Unit --> i`
    Use ``i`` as the imaginary unit symbol.
* :menuselection:`Imaginary Unit --> j`
    Use ``j`` as the imaginary unit symbol.
* :menuselection:`Complex Form --> Rectangular (a + bi)`
    Display complex results in rectangular form.
* :menuselection:`Complex Form --> Exponential (reⁱᶿ)`
    Display complex results in exponential form.
* :menuselection:`Complex Form --> Trigonometric (r(cos θ + i·sin θ))`
    Display complex results in trigonometric form.
* :menuselection:`Complex Form --> Cis (r·cis(θ))`
    Display complex results in cis form.
* :menuselection:`Complex Form --> Phasor (r∠θ)`
    Display complex results in phasor form.

When the angle mode is set to radians, simple phase multiples of ``pi`` are
shown symbolically in these complex forms. Other angle modes show their numeric
phase values.

.. _radix_character:

Number Format
+++++++++++++

Choose how decimal numbers are displayed and interpreted via
:menuselection:`Settings --> Number Format...`.

This shared setting controls result formatting and, when
:menuselection:`Settings --> Editing --> Group Digits in Input` is enabled,
digit grouping in the input field.

.. versionadded:: 1.0

This dialog replaces the older separate Radix Character and Digit Grouping
menus with a single combined Number Format selection.

The default option is no digit grouping with dot decimal separator
(``1234567.12345``).

A manually entered grouped number such as ``1,234,567`` keeps its grouping
marks in the input field, even with this default format selected.

Additional options let you choose:

* no digit grouping with dot or comma decimal separator
* SI grouping with space separators (integer and fractional grouped in 3s)
* 3-digit grouping with comma, dot, space, or underscore
* Indian/South Asian 3-2-2 grouping with comma separators

Some options include grouped fractional digits; those are separate explicit
choices.

The apostrophe (``'``), commonly used as a thousands separator in some
locales and contexts (for example Switzerland, Liechtenstein, and some
technical writing in Austria/Germany), is intentionally not offered because
SpeedCrunch reserves it for :ref:`sexagesimal notation <sexagesimal_values>`.

For input, SpeedCrunch remains permissive across styles to support
copy/paste from other applications. In clear mixed-separator cases, it
accepts the number and normalizes it by ignoring non-digit grouping
characters as needed.

When a single comma or dot could be either a decimal point or a grouping
mark, the selected decimal separator takes priority. In any dot-decimal
format, ``1,234`` means ``1234`` and ``1.234`` means ``1.234``. This also
applies to ``1234567.12345``, which displays numbers without grouping.
In comma-decimal formats, the roles are reversed. A grouping mark must
separate one to three leading digits from a group of three digits.
For numbers with only one comma or dot, the selected decimal mark remains
a decimal point regardless of how many digits follow it. The other mark is
also accepted as a decimal point when the number does not match the
thousands-grouping pattern. With ``1234567.12345`` selected, for example,
``1,2``, ``35,6``, ``987,1``, and ``2,34567`` mean ``1.2``, ``35.6``,
``987.1``, and ``2.34567``. These inputs are accepted without changing their
commas in the editor.
This applies to every number format, including styles that display spaces,
underscores, or fractional grouping. Accepted decimal values are displayed
with the selected decimal separator and grouping pattern. For example,
``2,34567`` has the same value in every format, but appears as ``2.345 67``
with ``1 234 567.123 45`` selected or ``2,345.67`` with
``1.234.567,123.45`` selected.
With a dot-decimal format selected, ``0,123`` means ``123``. Leading zeroes
do not change the grouping rule. In contrast, ``1234,567`` means
``1234.567`` because four leading digits do not form a conventional single
thousands group. Comma-decimal formats apply the same rules with the marks
reversed.
These rules also apply when pasting a number or entering scientific notation.
Clear mixed-separator input such as ``1.000,23`` is accepted as ``1000.23``
when it does not also match the selected fractional-grouping format.

When input contains both commas and dots and has two possible readings,
SpeedCrunch assumes the fractional part is not grouped unless the selected
format explicitly groups fractional digits with those marks. For example,
with ``1234567.12345``
selected, ``1.000,000 * 2`` means ``1000 * 2`` and gives ``2000``.
The same applies to ``1,000.000 * 2``.

For formats that group fractional digits with commas or dots, valid input in
the selected format takes priority. With ``1,234,567.123,45`` selected,
``1.234,56`` means ``1.23456``. With ``1.234.567,123.45`` selected,
``1,234.56`` has that same value. Other formats read these inputs as
``1234.56``. Spaces can separate groups on either side of the decimal point,
for example ``1 234.567 89`` means ``1234.56789``.

With ``1,234,567.123,45`` selected, ``1.000,000 * 2`` instead gives ``2``
because the comma groups fractional digits. The matching case with
``1.234.567,123.45`` selected is ``1,000.000 * 2``, which also gives ``2``.

Repeated commas or dots used for integer grouping must follow three-digit
grouping or Indian 3-2-2 grouping. Empty groups are rejected. For example,
``1,2,3``, ``1.2.3``, and ``1,,234`` report an error instead of having their
punctuation removed. Spaces remain permissive, so ``12 34`` and ``1 2 3``
mean ``1234`` and ``123`` respectively.

The editor keeps commas and dots as you type or paste them, even when they
differ from the selected format. The interpreted expression and results use
the selected number format. Digit grouping in the editor can add visual
separators, but does not replace punctuation you enter.

The interpreted expression keeps leading and trailing zeroes from the input.
With ``1234567.12345`` selected, ``0,123`` is shown as ``0123`` and evaluates
to ``123``. With ``1234567,12345`` selected, the same input is shown as
``0,123`` and evaluates to ``0,123``. Scientific notation keeps its exponent
in the interpreted expression, using the selected decimal and grouping marks.

History Size Limit
++++++++++++++++++

This setting controls how calculation history is stored for the active session.
It is available from :menuselection:`Session --> History Size Limit...`.

Calculation history in ordinary sessions is saved automatically when a session
changes and restored on the next launch. Private sessions keep history only
while they are open.

The history size limit sets the maximum number of stored history entries for
that session. By default, SpeedCrunch keeps up to 1000 entries per session and
automatically removes the oldest ones when this limit is exceeded. When a
calculation fills the last available history slot, SpeedCrunch warns that future
calculations will remove the oldest calculation from that session. Set the value
to ``0`` to disable the limit for the active session.

.. versionadded:: 1.0


Window
++++++

This section contains settings that control the main window behavior.

* :menuselection:`On Subsequent Launch`
    Choose what happens when you launch SpeedCrunch while it is already running.
    :menuselection:`Activate Existing Window` is the default.
    :menuselection:`Open New Window` opens a blank session in another window.
    :menuselection:`Open New Tab` opens a blank session in the most recently
    active calculator window, using its active pane if the window is split.
    The choice applies to all windows and is saved between launches.

    This setting works on Windows, macOS, and Linux with X11 or Wayland.
    On macOS, reopening the app from Finder or clicking its Dock icon also
    follows this choice. Switching between apps or clicking an existing window
    does not create a window or tab. On other desktops, the launcher or taskbar
    action must launch the application again to use this setting.

    If SpeedCrunch is not running, it starts normally and restores saved
    sessions regardless of this choice. Requests received during restoration
    wait until it finishes. If a modal dialog is open, finish or dismiss it
    before SpeedCrunch creates the requested window or tab. The desktop's
    focus policy still determines whether a window can come to the foreground.
* :menuselection:`Open a Private Tab on Startup`
    Open and focus a fresh private tab after restoring saved sessions. If there
    are no sessions to restore, or the active session is an empty
    ``Untitled-XXX`` tab, start without an extra ordinary tab. This option is off
    by default.
* :menuselection:`Save Window Position on Exit`
    Controls if the window position is saved and restored.
* :menuselection:`Always on Top`
    Keep the SpeedCrunch window on top of other windows. This option is hidden on
    Wayland (non-X11 Linux desktop sessions), because many Wayland compositors do
    not honor it reliably. It remains available on Windows, macOS, and Linux X11
    sessions.


Results
+++++++

This section contains settings that control result output and post-evaluation behavior.

* :menuselection:`Show Live Result Preview`
    If set, SpeedCrunch will display partial results as you type your expression as well
    as results when selecting a partial expression in the editor.

* :menuselection:`Notation & Precision...`
    Open a tabular layout with one mandatory ``Main Line`` row and four
    optional extra result-line rows. Each row has columns for enabling the
    line, notation, and decimal places. Extra lines are ignored unless their
    row is enabled.

    Applying changes from this dialog updates only the current editor previews
    immediately (live/selection) without rewriting already displayed
    calculation history.

* :menuselection:`Remove Trailing Zeroes`
    Hide trailing fractional zeroes after rounding to the selected decimal
    places. For example, at three decimal places, ``3.140`` becomes ``3.14``
    and ``12.000`` becomes ``12``. This option is off by default and applies
    to the main and extra result lines. Changing it refreshes editor previews
    without rewriting displayed history.

    .. versionadded:: 1.1

* :menuselection:`Rounding Mode`
    Choose how displayed results are rounded when formatting to a finite number
    of digits. The default is :menuselection:`Nearest, Half Away (round)`.
    Available modes are:
    :menuselection:`Nearest, Half Away (round)`,
    :menuselection:`Nearest, Half Even (roundeven)`,
    :menuselection:`Toward Zero (trunc)`,
    :menuselection:`Toward +∞ (ceil)`,
    :menuselection:`Toward −∞ (floor)`.

    * :menuselection:`Nearest, Half Away (round)`
        Round to nearest; ties increase magnitude. This matches the
        :func:`round` function.
        Example at 1 fractional digit: ``2.250 -> 2.3``,
        ``-2.250 -> -2.3``.
    * :menuselection:`Nearest, Half Even (roundeven)`
        Round to nearest; ties go to the nearest even last kept digit. This is
        Banker's rounding and matches the :func:`roundeven` function.
        Example at 1 fractional digit: ``2.250 -> 2.2``,
        ``2.350 -> 2.4``, ``-2.250 -> -2.2``.
    * :menuselection:`Toward Zero (trunc)`
        Discard extra digits without rounding up. This matches the
        :func:`trunc` function.
        Example at 1 fractional digit: ``2.290 -> 2.2``,
        ``-2.290 -> -2.2``.
    * :menuselection:`Toward +∞ (ceil)`
        Always round upward. This matches the :func:`ceil` function.
        Example at 1 fractional digit: ``2.210 -> 2.3``,
        ``-2.210 -> -2.2``.
    * :menuselection:`Toward −∞ (floor)`
        Always round downward. This matches the :func:`floor` function.
        Example at 1 fractional digit: ``2.290 -> 2.2``,
        ``-2.210 -> -2.3``.

    This setting affects result formatting (including live previews), not the
    numeric value of expressions. Use the named function in parentheses when a
    calculation must apply the same rounding strategy explicitly. Like other
    result-format settings, changes apply to subsequent calculations and
    previews; already stored history entries keep their original display.

* :menuselection:`Complex Form` and :menuselection:`Imaginary Unit`
    Configure the complex result form and imaginary unit. See :ref:`complex_numbers`.

* :menuselection:`Unit Notation`
    Configure whether unit results use exponential notation, such as
    ``m·s⁻¹``, or fractional notation, such as ``m/s``.
* :menuselection:`Copy New Results Automatically`
    Automatically copy each newly evaluated result to the clipboard.
* :menuselection:`Simplify Displayed Expressions`
    When enabled (default), SpeedCrunch adds an extra symbolic line before
    numeric results, simplifying the interpreted expression for readability
    (for example, combining repeated factors into powers and folding simple
    constant terms). This interpreted line also makes implicit-multiplication
    association explicit. Disable this option to keep only the unsimplified
    interpreted expression and numeric results.

    An extra line appears only when there is a useful reduction. For example,
    ``ans / 9`` stays as written instead of becoming ``1 / 9 · ans``.

    Entering the expression ``2.1 − 1 cos(pi)^2 cos(pi) 5 cos pi  / −cos^2(pi) + 3.4``
    gives the following interpretation and simplification:

    .. code-block:: text

      2.1 − 1 ⋅ cos²(π) ⋅ cos(π) ⋅ 5 ⋅ cos(π) / (−cos²(π)) + 3.4
      = 5 ⋅ cos²(π) + 5.5
      = 10.5

    .. versionadded:: 1.0


Editing
+++++++

* :menuselection:`Automatic Input Adjustments`
    Automatically replace operators, adjust spacing, insert closing brackets,
    convert powers to superscripts, and restrict input to valid characters.
    Turn it off to keep typed and pasted characters as entered.
    Tab still accepts the selected completion. Digit grouping
    and automatic insertion of ``ans`` follow their own settings. Pasting
    several lines evaluates each expression as usual.
    Invalid expressions can be entered, but still report an error when evaluated.

    This option is enabled by default. Changes apply immediately to all open
    editors and are saved between launches. Switching modes leaves existing
    text unchanged.

    .. versionadded:: 1.1

.. _automatic_result_reuse:

* :menuselection:`Auto-Insert ans`
    If a new expression starts with ``+``, ``-``, ``*``, or ``/``, SpeedCrunch inserts ``ans`` first.
* :menuselection:`Group Digits in Input`
    Group digits as you type, using the separator and grouping pattern selected
    in :menuselection:`Settings --> Number Format...`. For example, ``1000``
    appears as ``1 000`` with a space grouping format or ``1,000`` with a comma
    grouping format. This option is enabled by default. A number format with
    no grouping adds no separators.

    Integer digits are grouped in threes for decimal and octal numbers,
    or in fours for binary and hexadecimal numbers. The Indian format uses
    3-2-2 grouping. Fractional digits are grouped only when the selected format
    includes fractional grouping. Scientific exponents are left unchanged.
    The separators only affect how the input looks, so copying the expression
    keeps the original text.

    This setting works even when syntax highlighting is disabled.

    .. versionadded:: 1.1

* :menuselection:`Blinking Cursor`
    Make the cursor blink in the expression editor. This option is enabled
    by default. Turn it off for a steady cursor. Changes apply immediately
    to all open session panes and windows, and the choice is saved between
    launches.

    .. versionadded:: 1.1

* :menuselection:`Escape Clears Editor`
    Press :kbd:`Escape` to clear the focused expression editor. If a popup or
    result preview is open, Escape dismisses it first. Press Escape again to
    clear the expression. Cancelling a history edit still works as usual.
    This option is off by default and is saved between launches. Changes apply
    to all session panes and windows.

    .. versionadded:: 1.1

* :menuselection:`Keep Expression After Evaluation`
    If selected, the entered expression remains in the editor after evaluating it.

* :menuselection:`Autocomplete`
    Controls how suggestions are accepted and which names appear in editor
    autocompletion.

    * ``Accept with Enter`` lets :kbd:`Enter` insert the current suggestion
      without first selecting it with the arrow keys. It is off by default.
      When no autocomplete popup is open, Enter still evaluates the expression.
      Tab always accepts the current suggestion. The setting is saved between
      launches and applies to all session panes and windows.

      .. versionadded:: 1.1

    The following suggestion categories are all enabled by default:

    * ``Built-in functions`` includes built-in function names.
    * ``Built-in variables`` includes built-in constant names.
    * ``Units`` includes spelled-out unit names (for example, ``meter``).
    * ``User functions`` includes user-defined function names.
    * ``User variables`` includes user-defined variable names.

    The autocomplete popup also shows category icons for suggestions.

    .. versionadded:: 1.0

* :menuselection:`Up/Down Arrow History`
    Controls whether :kbd:`Up` and :kbd:`Down` in the editor navigate history:

    * ``Never`` always moves the cursor inside the editor.
    * ``Always`` (default) always navigates history.
    * ``Only for Single-Line Expressions`` navigates history only when the
      expression fits in one visual line; otherwise, it moves the cursor.

    .. versionadded:: 1.0

User Interface Settings
+++++++++++++++++++++++

* :menuselection:`Settings --> Appearance --> Theme...`
    Open the theme dialog. Themes are listed in :guilabel:`Light Themes` and
    :guilabel:`Dark Themes` groups. Selecting a theme updates the preview
    immediately. When no saved theme preference exists, as on first launch,
    SpeedCrunch uses the *Duskfox* theme.

    The :guilabel:`Preview` area shows representative result-display and
    editor content so that syntax-highlighting, result, and background colors
    are visible while editing. The selected ``background`` color generates
    nearby OKLCH surface colors for the expression editor and surrounding
    application chrome, followed by progressively offset surface colors for
    dock titles, dock controls and table headers, dock search and content
    areas, pane splitters, and scrollbars. It also generates the
    primary/accent color unless the theme specifies a ``primary`` color.

    Text on generated surfaces, including docks and popups, targets a contrast
    ratio of at least 10:1 against its own background. SpeedCrunch keeps a tint
    from the theme where possible. If a surface cannot reach that ratio, it
    uses black or white, whichever gives better contrast.

    The :guilabel:`Colors` area provides per-role color selection buttons showing
    both the ``#RRGGBB`` value and the actual color. Changing any color creates a
    custom theme based on the current selection.

    The dialog also includes:

    * :guilabel:`Import...` to copy a theme JSON file into SpeedCrunch's
      user theme directory and refresh the theme lists;
    * :guilabel:`Export...` to save the current theme as JSON in the same user
      theme directory.

    Import and export refuse theme names that collide with built-in themes.
    Import asks for confirmation before overwriting an existing custom theme.

    SpeedCrunch also supports loading additional theme files from the following
    directory:

    * Windows: :file:`C:/Users/<USERNAME>/AppData/Roaming/SpeedCrunch/color-schemes/`
    * Linux/Unix: :file:`~/.local/share/SpeedCrunch/color-schemes/`
    * Linux (Flatpak): :file:`~/.var/app/org.speedcrunch.SpeedCrunch/data/SpeedCrunch/color-schemes/`
    * OS X: :file:`~/Library/Application Support/SpeedCrunch/color-schemes/`
    * Portable version (any OS): :file:`color-schemes` subdirectory in the
      application directory.

    Theme files use JSON and map role names to color values. Supported role names are:
    ``number``, ``parens``, ``list``, ``unit``, ``result``, ``comment``,
    ``function``, ``operator``, ``variable``, ``separator``, ``background``,
    and optional ``primary``. The editor surface and scrollbar colors are
    generated from ``background``.
    For the full schema, see :doc:`theme_json_schema`.

    .. versionadded:: 1.0
       The :menuselection:`Theme...` dialog, including live preview and JSON import/export.

* :menuselection:`Settings --> Appearance --> System Color Mode`
    Choose the light or dark appearance for controls that use the system style:

    * :guilabel:`Follow System` (the default) follows the operating system's
      current appearance, including automatic light and dark changes.
    * :guilabel:`Light` keeps the system controls light.
    * :guilabel:`Dark` keeps the system controls dark.

    It affects menus, docks, and the status bar when their appearance is set to
    :guilabel:`System`, along with other native controls and dialogs. It does not
    change the selected SpeedCrunch theme or the Input and Results appearance.

    A system high-contrast accessibility theme may take precedence over the
    selected light or dark mode, depending on platform support.

    .. versionadded:: 1.2

* :menuselection:`Settings --> Appearance --> Menus`
    Choose how menus look:

    * :guilabel:`System` uses the system's menu colors and styling,
      including highlighted and disabled items. With :guilabel:`Follow System`
      selected under :menuselection:`System Color Mode`, menus follow changes
      to the system theme while SpeedCrunch is running.
    * :guilabel:`SpeedCrunch Theme` (the default) uses the selected SpeedCrunch
      theme for menu backgrounds and text, including highlighted items.
      Menu icons also follow the theme's text colors. In popup menus, disabled
      labels, shortcut hints, and icons appear dimmed.

    This setting applies to the menu bar shown inside the application window,
    dropdown menus, context menus, and dropdown lists in attached and floating
    docks that use the SpeedCrunch theme. Closed dropdown controls follow the
    :menuselection:`Docks` setting.
    The macOS system menu bar keeps its native appearance.
    Popup menus drawn by Qt can look slightly different from native macOS menus.
    On GNOME, popup menus use the GTK theme's popup hover colors when GTK is
    available and System Color Mode is set to :guilabel:`Follow System`.
    Otherwise they use Qt's system palette.
    The choice applies to all SpeedCrunch windows and is saved between launches.

    Selections in the expression editor and result display use the SpeedCrunch
    theme. Dock list selections follow the :menuselection:`Docks` setting.

* :menuselection:`Settings --> Appearance --> Docks`
    Choose how attached and floating docks look, including their title bars:

    * :guilabel:`System` uses the system's colors and widget styles for lists,
      search fields, dropdown controls, scrollbars, dock title bars, and the tabs
      used to switch between overlapping docks.
      Close and float buttons use the system style. Dropdown lists also
      use the system appearance, regardless of the :menuselection:`Menus` choice.
      The Formula Book and bitfield use system colors as well. Formula and
      navigation links use the system link color directly. On bits use the system
      selection colors, with a separate hover color for off bits.
    * :guilabel:`SpeedCrunch Theme` (the default) uses the selected SpeedCrunch
      theme for dock content, title bars, and tabs, including list selections and
      search fields.

    The choice applies immediately to all SpeedCrunch windows and is saved
    between launches. With :guilabel:`Follow System` selected under
    :menuselection:`System Color Mode`, system colors follow changes to the
    system theme while SpeedCrunch is running.

    .. versionadded:: 1.1

* :menuselection:`Settings --> Appearance --> Status Bar`
    Choose how the status bar looks:

    * :guilabel:`System` uses the system's palette and widget styles for the
      status bar, its labels, and its buttons. With :guilabel:`Follow System`
      selected under :menuselection:`System Color Mode`, colors follow changes
      to the system theme while SpeedCrunch is running.
    * :guilabel:`SpeedCrunch Theme` (the default) uses the selected SpeedCrunch
      theme for the status bar background and text.

    The choice applies immediately to all SpeedCrunch windows and is saved
    between launches. Popup menus follow the :menuselection:`Menus` setting.
    Show or hide the status bar with :menuselection:`View --> Status Bar`.

    .. versionadded:: 1.1

* :menuselection:`Settings --> Appearance --> Input and Results`
    Choose the layout of the expression editor and result display:

    * :guilabel:`Modern` (the default) uses a rounded input box with compact
      padding and a small gap around it. History aligns with the left edge of
      the box.
    * :guilabel:`Classic` uses a compact, full-width editor directly below the
      results and a small history gutter. All editors use the Modern input
      background color across the whole input area. Text and editor popups
      share a small inset from the edge, with enough space below
      the input for rounded window corners. A top line marks the active pane,
      using the same thickness and accent color as the Modern outline.
      This line is hidden when there is only one window and one pane,
      regardless of how many session tabs it contains.
      Vertical pane dividers remain visible through the input area.

    Both styles keep the same editing features, including input that grows as
    expressions wrap. The input grows to five visible lines, then scrolls to
    keep the cursor in view. Line spacing follows the selected font.
    The choice applies immediately to every window and pane and is saved
    between launches.

    .. versionadded:: 1.1

* :menuselection:`Settings --> Appearance --> Input and Results --> Font...`
    Select the font to use for the expression editor and result display.
    On macOS 27 and later, SpeedCrunch uses Qt's font dialog.

* :menuselection:`Settings --> Appearance --> Keypad`
    Choose the shape and spacing of the keypad buttons:

    * :guilabel:`Classic` uses shallow rectangular buttons with smaller gaps,
      similar to the keypad in SpeedCrunch 0.12.
    * :guilabel:`Modern` (the default) keeps the larger square buttons.

    The choice applies immediately to all windows and is saved between launches.
    It works with every preset and custom keypad. Each window keeps its own
    layout, selected from :menuselection:`View --> Keypad`, and zoom level,
    selected from :menuselection:`Settings --> Appearance --> Keypad --> Zoom`.

    .. versionadded:: 1.1

* :menuselection:`Settings --> Appearance --> Syntax Highlighting`
    Enable or disable syntax highlighting.
* :menuselection:`Settings --> Appearance --> Hover Highlighting`
    Enable or disable hover highlighting.
* :menuselection:`Settings --> Language`
    Select the user interface language.


Help
----

* :menuselection:`Help --> App Diagnostics`
    Show system information and app settings. Use :guilabel:`Copy to Clipboard`
    to share the report when reporting a problem.

* :menuselection:`Help --> About SpeedCrunch`
    Show the SpeedCrunch version, the operating system and its version, and the Qt
    version. Portable builds display ``Portable`` next to the SpeedCrunch
    version.

* :menuselection:`Help --> Check for Updates`
    Trigger an update check and show information when a newer version is available.
    Update checks are enabled in official SpeedCrunch builds. Other distributors
    can choose whether to enable them. When enabled, this menu entry is available
    and SpeedCrunch also checks for updates after the window first paints,
    at most once every 24 hours.
    Each network request has a five-second deadline, including redirects.
    Receiving data does not extend that deadline.
    Oversized update responses are rejected before their contents are parsed
    or saved.
    Closing a window requests that its update worker stop. Cleanup finishes
    in the background, without making the window wait for network setup.
    See :file:`BUILDING.md` at the repository root for build options and
    package provider guidance.

    .. versionadded:: 1.0


Keyboard Shortcuts
------------------

Editing
+++++++
* :kbd:`Ctrl+O`
    Open session.
* :kbd:`Ctrl+Q`
    Quit SpeedCrunch (:kbd:`Cmd+Q` on macOS).
* :kbd:`Ctrl+N`
    Create a new session in the current session pane.
* :kbd:`Ctrl+T` (:kbd:`Cmd+T` on macOS)
    Create a new session tab in the current session pane.
* :kbd:`Ctrl+Shift+N` (:kbd:`Cmd+Shift+N` on macOS)
    Create a new private tab in the current session pane.
* :kbd:`Ctrl+Shift+T` (:kbd:`Cmd+Shift+T` on macOS)
    Reopen the most recently closed session tab.
* :kbd:`Ctrl+W` (:kbd:`Cmd+W` on macOS)
    Close the active session tab.
* :kbd:`Ctrl+C`
    Copy selected text to clipboard.
* :kbd:`Ctrl+R`
    Copy last result to clipboard.
* :kbd:`Ctrl+V`
    Paste from clipboard.
* :kbd:`Ctrl+A`
    Select entire expression.
* :kbd:`Ctrl+(` or :kbd:`Ctrl+)`
    Wrap the current selection in parentheses. If no text is selected, the entire expression is wrapped.

Session Tabs
++++++++++++

* :kbd:`Cmd+Alt+Left` and :kbd:`Cmd+Alt+Right` (macOS)
    Move to the previous or next session tab or pane.
* :kbd:`Ctrl+Page Up` and :kbd:`Ctrl+Page Down` (Windows/Linux)
    Move to the previous or next session tab or pane.

Widgets and Docks
+++++++++++++++++

* :kbd:`Ctrl+1`
    Show/hide formula book.
* :kbd:`Ctrl+2`
    Show/hide constants widget.
* :kbd:`Ctrl+3`
    Show/hide functions widgets.
* :kbd:`Ctrl+4`
    Show/hide variables widget.
* :kbd:`Ctrl+5`
    Show/hide user functions widget.
* :kbd:`Ctrl+6`
    Show/hide user units widget.
* :kbd:`Ctrl+7`
    Show/hide history widget.
* :kbd:`Ctrl+8`
    Show/hide bit field dock.
* :kbd:`Ctrl+B`
    Show/hide the status bar.
    The status bar provides quick selectors for :menuselection:`Angle Mode`,
    :menuselection:`Results --> Notation`, and
    :menuselection:`Results --> Precision`. On narrow windows, selectors that
    do not fully fit are hidden automatically. Status bar visibility is set
    separately for each window; the main menu always shows and changes the
    setting for the active window. Angle mode, notation, and precision selections
    are also retained separately for each window.

Scrolling
+++++++++

* :kbd:`Page Up` and :kbd:`Page Down`
    Scroll the result window page-wise.
* :kbd:`Shift+Page Up` and :kbd:`Shift+Page Down`
    Scroll the result window line-wise.
* :kbd:`Cmd+Page Up` and :kbd:`Cmd+Page Down` (macOS)
    Scroll to the top or bottom of the result window.

Focus Navigation
++++++++++++++++

* :kbd:`F6` and :kbd:`Shift+F6`
    Move focus forward or backward between the expression editor and visible dock controls.
    Focus navigation applies to the current window, including its floating docks.


Notation
++++++++

* :kbd:`F2`
    Set result notation to automatic decimal.
* :kbd:`F3`
    Set result notation to fixed-point decimal.
* :kbd:`F4`
    Set result notation to engineering decimal.
* :kbd:`F5`
    Set result notation to scientific decimal.
* :kbd:`F7`
    Set result notation to octal.
* :kbd:`F8`
    Set result notation to hexadecimal.
* :kbd:`F9`
    Set result notation to sexagesimal.
* :kbd:`F10`
    Set result notation to binary.

    .. versionadded:: 1.0
    
Various
+++++++

* :kbd:`F1`
    Show context help (dismiss with :kbd:`Escape`).

    .. versionadded:: 0.12

* :kbd:`F11`
    Toggle full screen.
* :kbd:`Ctrl` + mouse wheel, :kbd:`Shift` + mouse wheel, or :kbd:`Shift+Up` and :kbd:`Shift+Down`
    Change the font size.
* :kbd:`Ctrl+Shift` + mouse wheel
    Change the window opacity.

    .. versionadded:: 0.12


.. rubric:: Footnotes

.. [#f1] Starting with SpeedCrunch 0.12, the session format is based on `JSON <json_>`_. Previous
         versions used a simple custom text format.

.. _json: http://json.org/
