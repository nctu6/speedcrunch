# App Diagnostics

Open **Help > App Diagnostics** for a plain text report that you can share with
the developers when reporting a slow startup or a compatibility problem. Use
**Copy to Clipboard** to copy the whole report.

Available since SpeedCrunch 1.1.

## Enabling startup recording

Startup recording is off by default. To include startup timings and detailed
appearance tracing in a beta build for troubleshooting, configure from the
repository root with:

```sh
cmake -S . -B build -DENABLE_STARTUP_DIAGNOSTICS=ON
cmake --build build --config Release --parallel
```

Enable this option explicitly for the beta builds you want to share. The
`BUILD_VARIANT_DEV_DEBUG` variant also forces it on. The version and build type
alone do not enable it. See [BUILDING.md](BUILDING.md#build-variants) for the
variant settings. CMake saves the option in the build directory's cache.
To disable it, turn off the developer variant if active and configure again
with `-DENABLE_STARTUP_DIAGNOSTICS=OFF`.

With the option off, the diagnostics layer uses no-op recording functions and
the application uses Qt's normal event dispatch. The report still includes
system information, settings, and session counts, but shows
`Startup recording disabled in this build.` instead of timings.

Recording adds overhead to the measured startup. Timings from diagnostic builds
include that overhead. The sections below describe builds with recording enabled.

## Report contents

The report includes the app version and Portable status, operating system,
Qt versions, selected and system languages, display scaling, and widget style.
It also shows process memory and CPU time where available, counts of open
windows, sessions and history entries, and relevant calculation preferences.
The editor, dock, menu, and status-bar appearance modes show the current
choices each time you open the report.

## Startup timings

Startup timings cover application setup, settings, built-in repositories,
UI construction, docks, themes, and session restoration. File reads on worker
threads, queued UI restoration, and startup painting are measured too.

Recording starts at `main()` and finishes once the UI has painted and the
startup tasks have completed. It does not include process loading before
`main()` or time waiting for an update-check response.

Times are measured in milliseconds. Nested phases and asynchronous work can
overlap, so adding the phase totals will not give the overall startup time.
Repeated phases show their call count, total time, and longest call.
If startup is still running, reopen the dialog later for the complete report.

### Menus and update checks

Menu timings separate creating the menu bar from filling the menus with
actions. Native menu-bar creation can also create the platform window.
Menu appearance reuses generated colors across menus with the same theme.
Theme and system palette changes refresh those colors. Generation and reuse
appear separately in the report.

When update checks are enabled, automatic dispatch waits until the window
has painted. `Deferred automatic update check (async total)` includes
waiting for queued UI callbacks, rather than measuring only active work.
Its timings separate the schedule check, saving the attempt timestamp, and
network request setup. Update settings access, version comparison, and network
setup run on a worker thread. The asynchronous schedule and setup total includes
queue waiting, but excludes waiting for the response.

### Constants and Functions lists

Constants and Functions list population is measured separately. Constants
timings also separate clearing the list, filtering and formatting, creating
rows and tooltips, sizing columns, and sorting. Column sizing includes any
pending row layout that Qt performs while measuring the contents.
Hidden constants lists keep their rows ready but wait until they are shown
to measure column widths. Repeated theme surface requests reuse the last
generated colors when their inputs match. These calls appear as
`Reuse theme surface colors`.

The detailed records identify calls from saved-filter restoration and
language-change events. Calls that skip an unchanged list appear separately
from population.
The checks compare the translated list content, search and domain filters,
and layout direction. Constants also compare the radix character. Search
text and list content are not included in the report.

## Startup appearance details

Startup appearance details show individual calls, their parent calls, and
whether they run during window construction, session restoration, or a
deferred refresh. Temporary numeric IDs identify widgets across calls,
including panes added during restoration. Saved layouts reuse the initial
pane and its editor, result display, and tab bar. Only additional panes need
new controls. This appears as `Reuse initial session pane` in the timings.

The report compares inputs for theme generation and highlighting, and counts
palette and stylesheet assignments with changed or unchanged values. It also
counts identical palette and stylesheet assignments that checks skip. Deferred
native dock refreshes can skip polishing when the style and stylesheet are
unchanged and both the system palette and the palette produced by the last
restoration still match. Native theme and palette changes request a full
refresh.

### Docks

`Apply dock appearance` includes saving native widget state, styling the
dock title, and styling its contents. Titles are styled first so changing
the parent stylesheet does not repeat work on the contents. Table headers
receive their own styling without the list scrollbar styles.

Startup creates only the docks that were open in each saved window,
including panels behind another dock tab and floating panels. Closed docks
are created when you open them, using their saved placement when available.
Older layouts without an open-dock list use empty temporary docks to read
the saved visibility, without building application dock contents.
Repeated themed palette assignments are skipped when their colors and
explicit roles match.

Deferred refreshes reuse native table-header measurements when the style,
font, system palette, display DPI, layout direction, and header stylesheet
are unchanged. Header data changes and model resets discard the saved
measurements. Measurement and reuse appear separately in the timings.

### Highlighting and recording limits

Highlighting comparisons use document revisions and sizes, theme inputs,
selected settings, and evaluator identity. They do not track changes to the
evaluator's symbols. Matching inputs help identify repeated work, but do
not prove that a call can be skipped. Detailed records have fixed limits,
and the report shows how many records are omitted when those limits are
reached. These measurements add some overhead while startup is recorded.

## Privacy

The measurements stay in memory until you quit. SpeedCrunch does not log or
send them anywhere. The report excludes calculation text, session names,
and file paths. Copying it puts the report on your system clipboard.
