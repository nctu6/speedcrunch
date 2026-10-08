#!/usr/bin/env bash
#
# Build an arm64 SpeedCrunch.app, bundle Qt, sign it and package it as a DMG.
#
# Environment variables:
#   CODESIGN_IDENTITY  Signing identity, e.g.
#                      "Developer ID Application: Jane Doe (TEAMID1234)".
#                      When unset, the app is ad-hoc signed (local use only).
#   NOTARY_PROFILE     notarytool keychain profile created with
#                      `xcrun notarytool store-credentials <profile> ...`.
#                      When set together with CODESIGN_IDENTITY, the DMG is
#                      notarized, stapled and checked with Gatekeeper (spctl).
#   ENTITLEMENTS       Optional entitlements plist used when signing the app.
#   JOBS               Parallel build jobs (defaults to the number of CPUs).
#
# If the Command Line Tools cannot find C++ standard headers (e.g. after a
# macOS update), build with the full Xcode toolchain instead:
#   DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer ./scripts/generate-macos-dmg.sh
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build-arm64"
OUT_DIR="$ROOT_DIR/build/dmg"
VOLUME_NAME="SpeedCrunch"

CODESIGN_IDENTITY="${CODESIGN_IDENTITY:-}"
NOTARY_PROFILE="${NOTARY_PROFILE:-}"
ENTITLEMENTS="${ENTITLEMENTS:-}"

require_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "Missing required command: $1" >&2
    exit 1
  fi
}

require_cmd cmake
require_cmd strip
require_cmd otool
require_cmd install_name_tool
require_cmd file
require_cmd rsync
require_cmd grep
require_cmd macdeployqt
require_cmd codesign
require_cmd hdiutil
require_cmd ditto
require_cmd /usr/libexec/PlistBuddy

if [[ -n "$CODESIGN_IDENTITY" ]]; then
  if ! security find-identity -v -p codesigning | grep -F "\"$CODESIGN_IDENTITY\"" >/dev/null; then
    echo "Signing identity not found in keychain: $CODESIGN_IDENTITY" >&2
    security find-identity -v -p codesigning >&2 || true
    exit 1
  fi
  if [[ -n "$ENTITLEMENTS" && ! -f "$ENTITLEMENTS" ]]; then
    echo "Entitlements file not found: $ENTITLEMENTS" >&2
    exit 1
  fi
  if [[ -n "$NOTARY_PROFILE" ]]; then
    require_cmd xcrun
    require_cmd spctl
    require_cmd plutil
  fi
elif [[ -n "$NOTARY_PROFILE" ]]; then
  echo "NOTARY_PROFILE is set but CODESIGN_IDENTITY is not; ad-hoc signed apps cannot be notarized." >&2
  exit 1
fi

mkdir -p "$OUT_DIR"

if [[ -z "${JOBS:-}" ]]; then
  JOBS="$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"
fi

echo "Configuring arm64 build..."
cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
  -DCMAKE_OSX_ARCHITECTURES="arm64" \
  -DCMAKE_BUILD_TYPE=Release

echo "Building SpeedCrunch (arm64, parallel jobs: $JOBS)..."
cmake --build "$BUILD_DIR" --config Release --target SpeedCrunch --parallel "$JOBS"

APP_BIN="$BUILD_DIR/SpeedCrunch.app/Contents/MacOS/SpeedCrunch"
APP_BUNDLE="$BUILD_DIR/SpeedCrunch.app"
APP_FRAMEWORKS_DIR="$APP_BUNDLE/Contents/Frameworks"
APP_PLIST="$APP_BUNDLE/Contents/Info.plist"
if [[ ! -f "$APP_BIN" ]]; then
  echo "Build failed (missing app binary $APP_BIN)." >&2
  exit 1
fi

echo "Stripping app binary..."
strip -x "$APP_BIN"

echo "Bundling Qt frameworks with macdeployqt..."
MACDEPLOYQT_ARGS=(-always-overwrite -no-codesign)
macdeployqt "$APP_BUNDLE" "${MACDEPLOYQT_ARGS[@]}"

if [[ ! -d "$APP_FRAMEWORKS_DIR" ]] || [[ ! -e "$APP_FRAMEWORKS_DIR/QtCore.framework" ]]; then
  echo "Qt deployment failed (missing $APP_FRAMEWORKS_DIR/QtCore.framework)." >&2
  exit 1
fi

# Lists every Mach-O file inside the bundle, NUL-separated.
list_macho_files() {
  find "$APP_BUNDLE/Contents" -type f -print0 |
    while IFS= read -r -d '' f; do
      if file -b "$f" | grep -q 'Mach-O'; then
        printf '%s\0' "$f"
      fi
    done
}

rpaths_of() {
  otool -l "$1" | awk '$1 == "cmd" && $2 == "LC_RPATH" { getline; getline; print $2 }'
}

# Dependencies only (skips the file header and, for dylibs, their own id).
deps_of() {
  local id
  id="$(otool -D "$1" | tail -n +2)"
  otool -L "$1" | tail -n +2 | awk '{ print $1 }' | { grep -vxF "${id:-<none>}" || true; }
}

# Homebrew splits Qt into one keg per module (qtbase, qtsvg, qttools, ...)
# and macdeployqt cannot resolve @rpath references into other kegs (e.g. the
# SVG icon engine plugin -> @rpath/QtSvg.framework). Copy such frameworks in.
bundle_missing_frameworks() {
  local added=0 f dep fw_name src brew_prefix
  brew_prefix="$(brew --prefix 2>/dev/null || true)"
  [[ -n "$brew_prefix" ]] || return 1
  while IFS= read -r -d '' f; do
    while IFS= read -r dep; do
      fw_name="${dep#@rpath/}"
      fw_name="${fw_name%%/*}"
      [[ "$fw_name" == *.framework ]] || continue
      [[ -e "$APP_FRAMEWORKS_DIR/$fw_name" ]] && continue
      for src in "$brew_prefix"/opt/qt*/lib/"$fw_name"; do
        if [[ -d "$src" ]]; then
          echo "  bundling $fw_name (needed by ${f#"$APP_BUNDLE/"})"
          rsync -a --exclude 'Headers' --exclude '*.prl' "$src" "$APP_FRAMEWORKS_DIR/"
          chmod -R u+w "$APP_FRAMEWORKS_DIR/$fw_name"
          added=1
          break
        fi
      done
    done < <(deps_of "$f" | grep '^@rpath/' || true)
  done < <(list_macho_files)
  [[ "$added" == 1 ]]
}

echo "Bundling frameworks macdeployqt missed..."
for _ in 1 2 3; do
  bundle_missing_frameworks || break
done

# macdeployqt also leaves the build-time rpath (/opt/homebrew/lib) in the
# executable and some @rpath references (e.g. libbrotlidec ->
# @rpath/libbrotlicommon). Point every bundled dependency at
# @executable_path/../Frameworks and drop absolute rpaths. This must run
# before signing because install_name_tool invalidates signatures.
echo "Fixing up rpaths and install names..."
while IFS= read -r -d '' f; do
  while IFS= read -r rp; do
    if [[ "$rp" == /* ]]; then
      install_name_tool -delete_rpath "$rp" "$f" 2>/dev/null
    fi
  done < <(rpaths_of "$f")
  id="$(otool -D "$f" | tail -n +2)"
  if [[ "$f" == "$APP_FRAMEWORKS_DIR/"* && -n "$id" && "$id" != @executable_path/* ]]; then
    install_name_tool -id "@executable_path/../Frameworks/${f#"$APP_FRAMEWORKS_DIR/"}" "$f" 2>/dev/null
  fi
  while IFS= read -r dep; do
    case "$dep" in
      @rpath/*) rel="${dep#@rpath/}" ;;
      /opt/homebrew/*|/usr/local/*)
        rel="${dep##*/lib/}"
        # Framework deps keep their bundle layout; plain dylibs are flat.
        [[ "$rel" == *.framework/* ]] || rel="$(basename "$dep")" ;;
      *) continue ;;
    esac
    if [[ -e "$APP_FRAMEWORKS_DIR/$rel" ]]; then
      install_name_tool -change "$dep" "@executable_path/../Frameworks/$rel" "$f" 2>/dev/null
    fi
  done < <(deps_of "$f")
done < <(list_macho_files)
if ! rpaths_of "$APP_BIN" | grep -qxF '@executable_path/../Frameworks'; then
  install_name_tool -add_rpath '@executable_path/../Frameworks' "$APP_BIN" 2>/dev/null
fi

echo "Checking bundle for external library references..."
DEPLOY_ERRORS="$(
  list_macho_files | while IFS= read -r -d '' f; do
    { deps_of "$f"; rpaths_of "$f"; } | grep -E '^(/opt/homebrew|/usr/local|.*Cellar)' |
      sed "s|^|${f#"$APP_BUNDLE/"}: |" || true
    { deps_of "$f" | grep '^@rpath/' || true; } | while IFS= read -r dep; do
      if [[ ! -e "$APP_FRAMEWORKS_DIR/${dep#@rpath/}" ]]; then
        echo "${f#"$APP_BUNDLE/"}: unresolved $dep"
      fi
    done
  done
)"
if [[ -n "$DEPLOY_ERRORS" ]]; then
  echo "Qt deployment failed: bundle still references libraries outside the app:" >&2
  echo "$DEPLOY_ERRORS" >&2
  exit 1
fi

# Ship the license inside the bundle too (GPL); must happen before signing.
cp -f "$ROOT_DIR/LICENSE" "$APP_BUNDLE/Contents/Resources/LICENSE"

if [[ -n "$CODESIGN_IDENTITY" ]]; then
  echo "Signing nested code with: $CODESIGN_IDENTITY"
  SIGN_ARGS=(--force --timestamp --options runtime --sign "$CODESIGN_IDENTITY")

  # Sign inside-out: loose dylibs (Qt plugins, bundled third-party libs) first,
  # then the frameworks, then the app itself. --deep is not used for signing
  # because it does not apply the same options reliably to nested code.
  find "$APP_BUNDLE/Contents" -type f -name '*.dylib' -print0 |
    while IFS= read -r -d '' lib; do
      codesign "${SIGN_ARGS[@]}" "$lib"
    done

  find "$APP_FRAMEWORKS_DIR" -maxdepth 1 -type d -name '*.framework' -print0 |
    while IFS= read -r -d '' fw; do
      codesign "${SIGN_ARGS[@]}" "$fw"
    done

  echo "Signing app bundle..."
  if [[ -n "$ENTITLEMENTS" ]]; then
    codesign "${SIGN_ARGS[@]}" --entitlements "$ENTITLEMENTS" "$APP_BUNDLE"
  else
    codesign "${SIGN_ARGS[@]}" "$APP_BUNDLE"
  fi
else
  echo "CODESIGN_IDENTITY not set; ad-hoc signing (not suitable for distribution)..."
  codesign --force --deep --sign - --timestamp=none "$APP_BUNDLE"
fi

echo "Verifying app signature..."
codesign --verify --deep --strict --verbose=2 "$APP_BUNDLE"

APP_VERSION="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$APP_PLIST" 2>/dev/null || true)"
if [[ -z "$APP_VERSION" ]]; then
  echo "Unable to determine app version from $APP_PLIST." >&2
  exit 1
fi
OUT_PATH="$OUT_DIR/SpeedCrunch-${APP_VERSION}-arm64.dmg"

# The DMG is built straight from the signed bundle with hdiutil. CPack is not
# used here because its install step re-copies the app and re-runs macdeployqt
# (see cmake/MacdeployQt.cmake), which would invalidate the signature.
echo "Creating DMG..."
STAGING_DIR="$(mktemp -d "${TMPDIR:-/tmp}/speedcrunch-dmg.XXXXXX")"
trap 'rm -rf "$STAGING_DIR"' EXIT
ditto "$APP_BUNDLE" "$STAGING_DIR/SpeedCrunch.app"
ln -s /Applications "$STAGING_DIR/Applications"
cp -f "$ROOT_DIR/LICENSE" "$STAGING_DIR/LICENSE.txt"
rm -f "$OUT_PATH"
hdiutil create -volname "$VOLUME_NAME" -srcfolder "$STAGING_DIR" \
  -fs HFS+ -format UDBZ -ov "$OUT_PATH" >/dev/null

if [[ -n "$CODESIGN_IDENTITY" ]]; then
  echo "Signing DMG..."
  codesign --force --timestamp --sign "$CODESIGN_IDENTITY" "$OUT_PATH"
  codesign --verify --strict --verbose=2 "$OUT_PATH"
fi

if [[ -n "$NOTARY_PROFILE" ]]; then
  echo "Submitting DMG for notarization (profile: $NOTARY_PROFILE)..."
  NOTARY_JSON="$(xcrun notarytool submit "$OUT_PATH" \
    --keychain-profile "$NOTARY_PROFILE" --wait --output-format json)" || true
  echo "$NOTARY_JSON"
  SUBMISSION_ID="$(plutil -extract id raw -o - - <<<"$NOTARY_JSON" 2>/dev/null || true)"
  NOTARY_STATUS="$(plutil -extract status raw -o - - <<<"$NOTARY_JSON" 2>/dev/null || true)"
  echo "Notarization submission: ${SUBMISSION_ID:-unknown} status: ${NOTARY_STATUS:-unknown}"
  if [[ "$NOTARY_STATUS" != "Accepted" ]]; then
    echo "Notarization failed." >&2
    if [[ -n "$SUBMISSION_ID" ]]; then
      xcrun notarytool log "$SUBMISSION_ID" --keychain-profile "$NOTARY_PROFILE" >&2 || true
    fi
    exit 1
  fi

  echo "Stapling notarization ticket..."
  xcrun stapler staple "$OUT_PATH"
  xcrun stapler validate "$OUT_PATH"

  echo "Gatekeeper assessment (app)..."
  spctl -a -vvv -t exec "$APP_BUNDLE"
  echo "Gatekeeper assessment (DMG)..."
  spctl -a -t open --context context:primary-signature -vvv "$OUT_PATH"
elif [[ -n "$CODESIGN_IDENTITY" ]]; then
  echo "NOTARY_PROFILE not set; skipping notarization (Gatekeeper will block the app on other Macs)."
fi

echo "DMG generated:"
ls -lh "$OUT_PATH"
