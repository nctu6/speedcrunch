# UI tests

Build the tests with CMake:

```sh
cmake -S src -B src/build -DBUILD_TESTING=ON
cmake --build src/build --parallel
```

The tests that used to live together in `testdisplayui.cpp` now have separate
executables, grouped by component:

| Executable | Coverage |
| --- | --- |
| `testdisplayui` | Result display, calculation copying and formatting |
| `testeditorappearanceui` | Editor and display styles, geometry and popups |
| `testkeypadui` | Keypad input, appearance and zoom |
| `testbitfieldui` | Bitfield colors and system appearance |
| `testthemesui` | Color schemes, theme previews and generated surfaces |
| `testmenusui` | Menus, context menus and dropdown appearance |
| `testdockappearanceui` | Dock appearance, tabs, headers and header buttons |
| `testdocklayoutui` | Dock layout, spacing, scrolling and Formula Book |
| `testfocusui` | Focus, active panes, clipboard routing and tooltips |
| `teststartupfocusui` | Editor and dock focus during startup and session restoration |
| `testsettingsui` | Settings, dialogs, shared fonts and manual formatting |
| `teststatusbarui` | Status bar appearance, controls and persistence |
| `testsessionsui` | Session tabs, layout, import, export and shortcuts |
| `testprivatesessionsui` | Private sessions and startup restoration |

All 17 UI suites share one label, `ui`. This includes the component suites
above and the existing editor and dock widget suites. Run them in parallel:

```sh
ctest --test-dir src/build -L ui --parallel --output-on-failure
```

With CTest 3.29 or newer, `--parallel` without a number normally chooses the
worker limit from the machine's processor count, with a minimum of two. When
running under a build job server, it uses the available job tokens instead.
Older CTest versions need an explicit number. A number such as `--parallel 6`
limits how many tests can run at once. It does not require six CPU cores.
See the [CTest parallel option](https://cmake.org/cmake/help/latest/manual/ctest.1.html#cmdoption-ctest-j)
for details. The build command's `--parallel` option uses the native build tool's
default when no number is given.

CTest's label summary adds up the test durations for this group. Parallel tests
overlap, so use `Total Test time (real)` to compare elapsed runtime.

`testdisplayui` now covers only the result display. To run exactly its former
coverage, select UI tests and exclude the two existing widget suites:

```sh
ctest --test-dir src/build -L ui -E '^(testeditorui|testdocksui)$' --parallel --output-on-failure
```

To run the entire test set, omit the label. To work on one component, select its
executable and optionally pass a Qt Test function name:

```sh
ctest --test-dir src/build -R '^testkeypadui$' --output-on-failure
src/build/testkeypadui keypad_power_button_uses_exponent_label_but_inserts_caret
```

Each executable uses `UiTestFixture` to create its own temporary settings,
sessions and cache directories. Tests use Qt's offscreen platform unless
`QT_QPA_PLATFORM` is already set. This lets separate processes run without
sharing application storage or desktop focus. For native platform checks, run
one suite at a time so desktop focus and the clipboard cannot interfere:

```sh
QT_QPA_PLATFORM=cocoa ctest --test-dir src/build -L ui --parallel 1 --output-on-failure
```

Use the appropriate Qt platform name on other systems.

## Adding tests

Put new tests in the suite for the component they exercise. Keep a test and its
`_data` provider together. Cross-window checks belong with the feature they
cover, such as keypad zoom or status bar persistence.

`mainwindowtesthelpers.h` and `mainwindowtesthelpers.cpp` contain shared
window, menu, rendering and session helpers. CMake compiles the common application
sources and resources once in the `mainwindowuitest` object library, then
links them into each executable. The object library carries the
`SPEEDCRUNCH_UI_TEST` definition to each suite.

When adding a component suite, register it in CMake's `mainwindow_ui_tests`
list.
