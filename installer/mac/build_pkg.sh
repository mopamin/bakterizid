#!/usr/bin/env bash
# Baut den Mac-Installer dist/Bakterizid-<Version>-macOS.pkg aus den fertigen Plugin-Dateien.
# Aufruf:  installer/mac/build_pkg.sh build/Bakterizid_artefacts/Release
# Der Installer ist unsigniert. Wie man ihn trotzdem öffnet, steht in INSTALLATION.md.
set -euo pipefail
cd "$(dirname "$0")/../.."

ART="${1:-build/Bakterizid_artefacts/Release}"
VERSION=$(sed -n 's/^project(Bakterizid VERSION \([0-9.]*\)).*/\1/p' CMakeLists.txt)
WORK=$(mktemp -d)
mkdir -p dist "$WORK/vst3" "$WORK/au" "$WORK/app" "$WORK/pkgs"

cp -R "$ART/VST3/Bakterizid.vst3" "$WORK/vst3/"
cp -R "$ART/AU/Bakterizid.component" "$WORK/au/"
cp -R "$ART/Standalone/Bakterizid.app" "$WORK/app/"

# Ad-hoc-Signatur, damit Apple-Silicon-Macs die Dateien überhaupt laden
for b in "$WORK/vst3/Bakterizid.vst3" "$WORK/au/Bakterizid.component" "$WORK/app/Bakterizid.app"; do
  codesign --force --deep --sign - "$b"
done

component () {   # name  quellordner  zielordner
  local plist="$WORK/$1.plist"
  pkgbuild --analyze --root "$2" "$plist"
  # Nicht an eine ältere Kopie irgendwo anders "umziehen", sondern genau hierhin installieren
  # (erkennt pkgbuild kein Bundle, ist die Liste leer und es gibt nichts umzustellen)
  /usr/libexec/PlistBuddy -c "Set :0:BundleIsRelocatable false" "$plist" 2>/dev/null || true
  pkgbuild --root "$2" --component-plist "$plist" --install-location "$3" \
           --identifier "com.sarin.bakterizid.$1" --version "$VERSION" "$WORK/pkgs/$1.pkg"
}
component vst3 "$WORK/vst3" "/Library/Audio/Plug-Ins/VST3"
component au   "$WORK/au"   "/Library/Audio/Plug-Ins/Components"
component app  "$WORK/app"  "/Applications"

sed "s/@VERSION@/$VERSION/g" installer/mac/distribution.xml > "$WORK/distribution.xml"
productbuild --distribution "$WORK/distribution.xml" --resources installer/mac/resources \
             --package-path "$WORK/pkgs" "dist/Bakterizid-$VERSION-macOS.pkg"

# Zusätzlich die nackten Plugins für Leute, die lieber selbst kopieren
( cd "$WORK" && zip -qry "$OLDPWD/dist/Bakterizid-$VERSION-macOS-ohne-Installer.zip" vst3 au app )

rm -rf "$WORK"
echo "Fertig: dist/Bakterizid-$VERSION-macOS.pkg"
