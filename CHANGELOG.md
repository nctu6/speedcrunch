# Changelog

## 1.1.0

### Functions

- Add `root(n; x)` for nth roots, including negative values and quantities
  with units. For example, `root(5; -32)` returns `-2`.
- Fix factorials in user-defined functions, such as `f(n) = n!`.

### Constants and units

- Search constants by name or unit with `Ctrl+Space`. Select a result with
  the arrow keys and insert it with Enter, Tab, or a click.
- Improve searching and keyboard selection in the Constants panel.
- Correct the units of the muon, proton, and neutron energy equivalents.
- Allow powers of grouped quantities, such as `(2[m])^2`.
- Correct results shown in `J/m²` and `J/m³` when using prefixed units.

### Input

- Fix undo and redo, and copy and paste.
- Fix clipped text and excess space in wrapped input with different fonts.
- Fix crashes when typing operators in long, wrapped expressions.
- Restore digit grouping while typing, following the selected number format.
- Add an option to turn off automatic input adjustments, such as operator
  replacements and closing brackets.
- Add optional Enter-to-accept autocomplete and Escape-to-clear behavior.
- Add an option to use a steady cursor instead of a blinking one.
- Preserve comments as written and prevent them from affecting result formatting.
- Improve recalling expressions from history and focus the editor after startup.
- Remove the empty-history hint.

### Results

- Restore free copying.
- Copy a whole calculation, including all displayed result formats, either
  across multiple lines or as a single line.
- Add optional removal of trailing decimal zeroes.
- Simplify live previews to show just the results.
- Keep live result previews above the editor when recalling long expressions
  from history.
- Avoid unnecessary extra simplification lines.
- Restore the F2-F5 and F7-F10 notation shortcuts.

### Sessions and settings

- Add private tabs that discard their contents when closed and are not
  restored on the next launch. Use `Ctrl+Shift+N` (`Cmd+Shift+N` on macOS)
  to open one.
- Allow switching between private and saved sessions, and optionally open
  a private tab at startup.
- Add **Reset All Settings...**, keeping sessions, history, and user definitions.
- Fix global user definitions missing from restored sessions.
- Make session renaming, cloning, closed-tab recovery, and pane controls
  easier to find in the menus. Cloning also copies unfinished input.
- Organize the result context menu into Copy, Remove, Split, and Session groups.
- Move **Number Format...** directly under Settings and simplify the result
  and appearance menus.
- Apply font changes across all windows and fix font selection on macOS 27
  and later.
- Fix commands acting on the wrong window when used from a floating panel.

### Appearance and panels

- Add separate system and SpeedCrunch theme choices for menus, panels,
  and the status bar.
- Add a compact **Classic** layout for input and results. Modern remains
  the default.
- Improve text contrast, menu readability, and panel button visibility.
- Match menu icons to the theme and dim disabled menu items and shortcuts.
- Tidy panel spacing, borders, and rounded corners.
- Keep side panels at full height and remember table column order.
- Preserve floating panel positions and visibility across restarts, and
  prevent floating panels from being grouped unexpectedly.

### Keypad

- Add a compact **Classic** keypad appearance and 50% zoom.
- Restore Zoom in the keypad's right-click menu and fix inconsistent sizing
  when changing zoom levels.
- Update scientific layouts with nth-root, square, and argument-separator
  buttons, and group related operations together.
- Replace the obsolete `lg` button with `log10`.

### Other improvements

- Reduce startup delays when restoring sessions, history, and panels.
- Make update checks more responsive and prevent them from delaying window
  closure.
- Add **Help > App Diagnostics** with a report that can be copied when
  asking for help. Show operating system details in About SpeedCrunch.
- Fix a crash when closing windows with certain panels open.
- Add portable macOS ZIP packages that keep settings and sessions beside
  the app in a `SpeedCrunch-data` folder.
- Set the default macOS minimum version to 14.4 on Apple Silicon.

## 1.0.0

### Functions

- Add matrix and vector operations, including products, inverses,
  determinants, transposes, and matrix dimensions.
- Add population and sample statistics, covariance, correlation, and median
  absolute deviation, with support for lists and matrices.
- Add `rand` and `randint` for random numbers, and summation over a range.
- Add `datetime` and `epoch` for converting dates and Unix timestamps.
- Add `molmass`, `mass`, and `molarity` for chemistry calculations.
- Add `powmod`, `emod`, `lcm`, and `popcount` for modular arithmetic,
  least common multiples, and counting set bits.
- Add `conj` and `cis` for working with complex numbers.
- Add IEEE 754 rounding and rounding-error functions for half, single,
  double, and quadruple precision.

### Constants and units

- Use square brackets to identify units explicitly, such as `5[m]`.
- Add angle units and aliases, Julian centuries, hectares, tonnes,
  `kph`, `mph`, and the `quad` energy unit.
- Insert constants and formulas with their units included.

### Input

- Support lists, matrices, and superscript powers in expressions.
- Accept `√`, `∛`, `Σ`, and `π` aliases, `~` for bitwise NOT,
  and more symbols for arithmetic operators.
- Show category icons in autocomplete suggestions.
- Adapt pasted numbers to the selected number format.

### Results

- Show the interpreted expression with implicit multiplication made explicit.
- Display several result formats together, including fractions, sexagesimal
  angles and times, and angles expressed as multiples of pi.
- Add complex polar forms and a choice of imaginary-unit symbol.
- Add custom precision, five rounding modes, and Indian digit grouping.
- Add `binpad`, `octpad`, and `hexpad` for output with leading zeroes,
  plus `sci` and `eng` for scientific and engineering notation.
- Keep earlier calculations' formatting when changing the current result format.

### Sessions and settings

- Work in tabs and split panes, save history between launches, and set
  history limits.
- Load personal functions and variables at startup.
- Edit earlier calculations and recalculate those below them.
- Check for updates from within the app.

### Appearance and keypad

- Add more themes, custom colors, and editor scaling.
- Choose keypad presets or create a custom layout.
- Show or hide the main menu, with access through the result context menu.
