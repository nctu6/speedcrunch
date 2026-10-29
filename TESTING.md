# Testing SpeedCrunch

See [BUILDING.md](BUILDING.md) for build requirements, Qt selection, and build
configurations. Building the tests also requires the Qt Test module.

## Building and running tests

By default, only the application is built. To also build and run the tests, enable
`BUILD_TESTING` when configuring. Run these commands from the repository root:

    cmake -S . -B build -DBUILD_TESTING=ON
    cmake --build build --config Release --parallel
    ctest --test-dir build -C Release --parallel --output-on-failure

The UI test suites use temporary configuration, session and cache directories,
and reset settings before each test case. They suppress the first-run number
format prompt. An unexpected prompt fails the test instead of blocking the suite.
They use Qt's offscreen platform unless `QT_QPA_PLATFORM` is already set.

CMake saves `BUILD_TESTING` in the build directory's cache. Configure again with
`-DBUILD_TESTING=OFF` to return to application-only builds. The
`BUILD_VARIANT_DEV_DEBUG` variant forces testing on, while the two official
release variants force it off. Disable any active variant before changing
`BUILD_TESTING` independently. See [BUILDING.md](BUILDING.md#build-variants).

The shared window test library enables startup recording so appearance tests can
inspect the recorded work. This does not enable recording in the application.
Set `ENABLE_STARTUP_DIAGNOSTICS` explicitly or use `BUILD_VARIANT_DEV_DEBUG`
for that, as described in [DIAGNOSTICS.md](DIAGNOSTICS.md).

## Windows

Follow the [Windows build setup](BUILDING.md#windows), including setting
`QT_ROOT` to your Qt MSVC kit. From the repository root in **x64 Native Tools
Command Prompt for VS 2022**, configure a developer Debug build and run the tests:

    cmake -S . -B build-dev -DCMAKE_PREFIX_PATH="%QT_ROOT%" -DBUILD_VARIANT_DEV_DEBUG=ON
    cmake --build build-dev --config Debug --parallel
    ctest --test-dir build-dev -C Debug --parallel 4 --output-on-failure

Rebuild after pulling test or application changes. To rerun only the suites that
failed in the previous CTest run, use:

    ctest --test-dir build-dev -C Debug --rerun-failed --parallel 4 --output-on-failure

The build and test commands also work in Git Bash once the build is configured.
The configure command above uses Command Prompt syntax for `QT_ROOT`.

On Windows, CTest adds the configured Qt kit's DLL directory to each test's
`PATH`. This lets the tests find libraries such as `Qt6Core.dll` without copying
them beside every executable. Rerun the configure command after updating an
existing checkout to apply this setting.

When running a test executable directly, add the matching Qt kit's `bin`
directory to `PATH` in that shell first. With `QT_ROOT` set as described in
[BUILDING.md](BUILDING.md), use this in Command Prompt:

    set "PATH=%QT_ROOT%\bin;%PATH%"

Or, for the example installation in Git Bash:

    export PATH="/c/Qt/6.12.0/msvc2022_64/bin:$PATH"

Adjust the path to match your installation. A Windows dialog reporting a missing
Qt DLL means the test could not start, rather than that a test assertion failed.

### Offscreen fonts

No `QT_QPA_PLATFORM` or `QT_QPA_FONTDIR` setting is needed for a normal test run.
When using offscreen mode on Windows, the UI test fixture sets `QT_QPA_FONTDIR`
to `%SystemRoot%\Fonts` if that directory exists and no font directory is already
set. An explicit `QT_QPA_FONTDIR` value takes precedence.

If Qt reports `Cannot find font directory`, check that the directory exists and
contains font files. To select a different directory, set it before running
CTest. For example, in Command Prompt:

    set "QT_QPA_FONTDIR=C:\Windows\Fonts"
    ctest --test-dir build-dev -C Debug --parallel 4 --output-on-failure

Or in Git Bash:

    QT_QPA_FONTDIR=C:/Windows/Fonts ctest --test-dir build-dev -C Debug --parallel 4 --output-on-failure

Missing fonts can cause failures in text sizing, glyph rendering and font dialog
tests. Offscreen warnings such as `This plugin does not support raise()` or
`propagateSizeHints()` are expected and do not by themselves mean a test fails.

### Native Windows UI checks

To check the native Windows backend, use a separate shell and run UI suites one
at a time so desktop focus and the clipboard cannot interfere. In Command Prompt:

    set "QT_QPA_PLATFORM=windows"
    ctest --test-dir build-dev -C Debug -L ui --parallel 1 --output-on-failure

Or in Git Bash:

    QT_QPA_PLATFORM=windows ctest --test-dir build-dev -C Debug -L ui --parallel 1 --output-on-failure

Close the Command Prompt shell when finished, or clear `QT_QPA_PLATFORM` with
`set "QT_QPA_PLATFORM="` before returning to offscreen tests.
