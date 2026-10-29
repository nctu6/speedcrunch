# Releasing SpeedCrunch

Work through this checklist in order for each stable release.
See [BUILDING.md](BUILDING.md) for toolchain setup and packaging details, and
[TESTING.md](TESTING.md) for test instructions.

The examples below use application version `X.Y` and Git tag `vX.Y.0`.
Replace them with the intended release values. Package filenames use the CMake
version, which can differ from the three-component Git tag.

## 1. Prepare the release metadata

- [ ] Choose the application version, Git tag, and release date (`YYYY-MM-DD`).
  Use the original publication date when rebuilding the same release.
- [ ] Set the application version in the root `CMakeLists.txt`:

  ```cmake
  project(speedcrunch VERSION X.Y)
  ```

  This supplies the About dialog, Windows version resource, macOS bundle,
  CPack package versions and filenames, and the manual version. Do not edit
  generated Windows resources or bundle files to change the version.
- [ ] Add the new release as the first entry under `<releases>` in **both**:
  - `pkg/desktop-appdata/org.speedcrunch.SpeedCrunch.metainfo.xml.in`
  - `pkg/org.speedcrunch.SpeedCrunch.metainfo.xml`

  ```xml
  <release version="X.Y" date="YYYY-MM-DD"/>
  ```

  Replace the date with the chosen publication date. Keep all previous release
  entries with their original versions and dates, and add any missing published
  releases.
  There is no automatic release date or history management in CMake.
  The translated XML is installed as-is,
  and Flathub reads its display version from this metadata, not the Git tag.
- [ ] Keep the template and translated metadata consistent. Review project,
  help, issue-reporting, and screenshot URLs. Check that screenshots still
  represent the application. Update desktop-entry text and its translations
  only when that content changes.
- [ ] Add the release notes to `CHANGELOG.md`, covering the changes since the
  previous release. Check contributor credits in `src/gui/aboutbox.cpp`.
- [ ] Update the manual's RST sources for changed behavior and use the new
  version for new `versionadded` markers. Keep historical markers unchanged.
- [ ] Refresh and check the bundled manual through the maintainer documentation
  workflow described in [the manual build notes](doc/src/README.md). Ordinary
  application builds embed the prebuilt manual and do not regenerate it.
  Verify its displayed version and changed pages before packaging. Edit RST
  sources or HTML templates, not generated HTML. Automated assistants must not
  run repository scripts or modify `doc/build_html_embedded/`.
- [ ] Prepare the new release section in
  `doc/src/_templates_standalone/download.html`. Put it above older releases,
  keep older downloads accessible, and include the release notes and credits.
  Use the intended tag and exact artifact filenames in download URLs. Keep the
  Flathub link pointed at the existing app page.

## 2. Test the release source

- [ ] Review the release diff and run `git diff --check`.
- [ ] Build and run tests in a separate build directory. Official package
  variants disable tests, so do not use a package build for this step.

  ```bash
  cmake -S . -B build-release-tests -DBUILD_TESTING=ON
  cmake --build build-release-tests --config Release --parallel
  ctest --test-dir build-release-tests -C Release --output-on-failure --parallel
  ```

  Select the correct Qt kit as described in `BUILDING.md`.
- [ ] Check calculations, clipboard operations, the embedded manual,
  translations, settings persistence, and session import/export. Test upgrading
  with backed-up settings and sessions from the previous release.
- [ ] Commit the release source changes and record the commit used for every
  package build. Build all artifacts from that same source revision. If a fix
  changes it, rebuild and repeat the affected checks before tagging.

## 3. Build the official Windows packages

- [ ] Check for Qt updates before building the official packages. Use a current
  supported Qt release, keep the Windows and macOS kits on the same Qt version
  where possible, and retest after updating. Update matching Qt Sources too.
- [ ] On Windows, prepare the matching MSVC Qt kit, CMake, Qt LinguistTools,
  and NSIS. Make `makensis.exe` available on `PATH`. Install matching Qt Sources
  and the SDK's SPDX files required by the license-installation rules.
  See [the Windows build instructions](BUILDING.md#windows) and
  [license requirements](BUILDING.md#license-files-in-installed-packages).
- [ ] Open the Visual Studio developer command prompt and set `QT_ROOT` to the
  installed MSVC kit. Run these commands from the repository root, using fresh,
  separate build directories for the installer and portable package.

  ```bat
  cmake -S . -B build-win-installer -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="%QT_ROOT%" -DBUILD_VARIANT_OFFICIAL_RELEASE=ON
  cmake --build build-win-installer --config Release --target PACKAGE --parallel

  cmake -S . -B build-win-portable -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="%QT_ROOT%" -DBUILD_VARIANT_OFFICIAL_PORTABLE_RELEASE=ON
  cmake --build build-win-portable --config Release --target PACKAGE --parallel
  ```

  These produce `SpeedCrunch-<version>-windows-x64.exe` and
  `SpeedCrunch-<version>-windows-x64.zip`. CPack uses NSIS for the installer.
  Both official variants enable update checks, install licenses, and strip the
  binary. CMake cache values persist, so do not switch variants in one directory.
- [ ] Test the installer on Windows without a Qt development installation.
  Check launch, the embedded manual, shortcuts, upgrade, and uninstall.
- [ ] Extract and run the portable ZIP in a writable directory. Check that
  settings and sessions stay beside the executable, survive restarting, and
  remain available after replacing the application files.
- [ ] Check executable version properties and the About dialog. Inspect both
  packages for their required Qt libraries/plugins, license texts, and notices.

## 4. Build the official macOS packages

- [ ] On macOS, select the official Qt kit, with matching Qt Sources and SDK
  SPDX files. Check the supported architecture and minimum macOS version in
  [BUILDING.md](BUILDING.md#macos), including the requirements of bundled Qt
  libraries. Set `QT_ROOT` below to that kit.
- [ ] Build the regular DMG and portable ZIP from separate fresh directories:

  ```bash
  export QT_ROOT="$HOME/Qt/6.12.0/macos"

  "$QT_ROOT/bin/qt-cmake" -S . -B build-macos-dmg \
    -DBUILD_VARIANT_OFFICIAL_RELEASE=ON
  cmake --build build-macos-dmg --config Release --target package --parallel

  "$QT_ROOT/bin/qt-cmake" -S . -B build-macos-portable \
    -DBUILD_VARIANT_OFFICIAL_PORTABLE_RELEASE=ON
  cmake --build build-macos-portable --config Release --target package --parallel
  ```

  Adjust the Qt path to the installed kit. The outputs are
  `SpeedCrunch-<version>-macos-arm64.dmg` and
  `SpeedCrunch-<version>-macos-arm64.zip` for the current arm64 target.
  CPack deploys Qt and packages the application. No separate packaging script
  or manual installation step is needed.
- [ ] Mount the DMG, copy the app to Applications, and test that installed copy.
  Check the About version, manual, calculations, settings, sessions, and updates.
- [ ] Extract the portable ZIP into a writable folder. Check that it creates
  `SpeedCrunch-data` beside the app and preserves that folder when replacing the
  app. Confirm that it reports an error when its data folder cannot be written.
- [ ] Check bundled Qt libraries/plugins, license texts, notices, and the
  bundle's version and minimum-system metadata. Confirm signature verification
  succeeds. The current packaging applies an ad hoc signature and does not
  perform Developer ID signing or notarization.

## 5. Tag and publish the upstream release

- [ ] Confirm all four official artifacts come from the final release commit
  and that every tested package displays the intended application version.
  Recheck the publication date in both AppStream files if the schedule changes.
- [ ] Create the release tag on that exact commit and push the release commit
  and tag to `heldercorreia/speedcrunch`. Do not move an existing published tag.
- [ ] Create a GitHub Release for the tag. Copy the reviewed release notes and
  attach the Windows installer, Windows portable ZIP, macOS DMG, and macOS
  portable ZIP. Confirm the release is stable rather than a prerelease.
- [ ] Publish the release and verify each asset can be downloaded. Check that
  the website template's URLs match the published filenames exactly.
- [ ] Build and deploy the website through the normal maintainer workflow.
  Verify the live Downloads page, release notes, credits, and manual links.
- [ ] Update the published `https://speedcrunch.org/version` endpoint to the
  application version only after the release and download page are available.
  Check that the endpoint returns the expected version and that an older
  official build's update check points users to the correct downloads.

## 6. Update the Flathub package

This work belongs in the separate
[Flathub repository](https://github.com/flathub/org.speedcrunch.SpeedCrunch).

- [ ] Start an update branch from that repository's current `master`.
- [ ] In `org.speedcrunch.SpeedCrunch.yaml`, update the Git `tag` and `commit`
  to the new published release.
- [ ] Remove patches whose fixes are present in the new source, along with
  their manifest entries. Check remaining patches against the new source.
- [ ] Check that the KDE runtime branch is still supported and that its SDK
  supplies the required Qt modules, including Qt Help and LinguistTools.
  Keep the build rooted at the repository root. Leave the upstream official
  build variants off so Flathub handles updates and stripping.
- [ ] On Linux, install Flatpak and set up the build tools if needed:

  ```bash
  flatpak remote-add --user --if-not-exists flathub \
    https://dl.flathub.org/repo/flathub.flatpakrepo
  flatpak install --user flathub org.flatpak.Builder
  ```

- [ ] From the Flathub checkout, build and install the manifest:

  ```bash
  flatpak run --command=flathub-build org.flatpak.Builder \
    --install org.speedcrunch.SpeedCrunch.yaml
  ```

  If the build fails while mounting `rofiles-fuse`, retry with
  `--disable-rofiles-fuse` before `--install`. This bypasses a local cache
  optimization and does not require a manifest change.
- [ ] Run both lint checks:

  ```bash
  flatpak run --command=flatpak-builder-lint org.flatpak.Builder \
    manifest org.speedcrunch.SpeedCrunch.yaml
  flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo repo
  ```

  Resolve errors and review warnings before submitting the PR.

- [ ] Run the app and inspect its installed release metadata:

  ```bash
  flatpak run --user org.speedcrunch.SpeedCrunch
  flatpak run --user --command=cat org.speedcrunch.SpeedCrunch \
    /app/share/metainfo/org.speedcrunch.SpeedCrunch.metainfo.xml
  ```

  Confirm the About version, newest AppStream version/date, preserved release
  history, manual, clipboard, session import/export, and persistence. Check
  desktop launching and window icons. Test Wayland and X11 where available.
  Back up existing data before upgrade testing. Flatpak data is stored under
  `~/.var/app/org.speedcrunch.SpeedCrunch/`, and local builds share it with the
  published package when run by the same user.
- [ ] Push the update branch and open a PR against the app repository's
  `master`. Describe the changes and record the platforms/architectures tested.
- [ ] Wait for Flathub's PR test builds and test the resulting packages. Merge
  after successful builds and testing. Check all supported build architectures.
- [ ] Follow the separate post-merge official build through publication.
  Merging triggers it automatically. Publication normally follows within
  1–2 hours of a successful build, unless moderation is required. The app page
  can refresh later. See [Flathub's maintenance guide](https://docs.flathub.org/docs/for-app-authors/maintenance#official-builds).
- [ ] Verify the package offered by the public remote, rather than relying on
  the locally built installation:

  ```bash
  flatpak update --user --appstream flathub
  flatpak remote-info --user flathub org.speedcrunch.SpeedCrunch
  ```

  Confirm the release metadata, runtime, and source commit. Check the
  [Flathub app page](https://flathub.org/apps/org.speedcrunch.SpeedCrunch)
  once its catalogue refreshes. The website download link can remain unchanged.

## 7. Close out the release

- [ ] Notify distribution packagers through the usual channels, pointing to
  the tag, changelog, source, and updated build requirements. Mention any
  changes to the required Qt version or LinguistTools/`lrelease`.
- [ ] Publish the release announcement once the advertised downloads work.
