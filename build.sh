#!/usr/bin/env bash
# Baut Bakterizid (VST3, Audio Unit und Standalone-App) auf dem Mac
# und installiert die Plugins gleich für dich.
# Aufruf im Projektordner:  ./build.sh
# Mit Installer:            ./build.sh --pkg
set -e
cd "$(dirname "$0")"

command -v cmake >/dev/null || { echo "CMake fehlt. Installieren mit: brew install cmake"; exit 1; }
command -v git   >/dev/null || { echo "Git fehlt. Installieren mit: xcode-select --install"; exit 1; }

if [ ! -d JUCE ]; then
  echo "Lade JUCE 9.0.4 herunter ..."
  git clone --depth 1 --branch 9.0.4 https://github.com/juce-framework/JUCE.git
fi

# Universal-Build (Apple Silicon + Intel), damit es auch auf anderen Macs läuft
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build build --config Release -j "$(sysctl -n hw.ncpu)"

if [ "${1:-}" = "--pkg" ]; then
  installer/mac/build_pkg.sh build/Bakterizid_artefacts/Release
fi

echo
echo "Fertig. Installiert nach:"
echo "  ~/Library/Audio/Plug-Ins/VST3/Bakterizid.vst3"
echo "  ~/Library/Audio/Plug-Ins/Components/Bakterizid.component"
echo "Standalone-App:"
echo "  $(pwd)/build/Bakterizid_artefacts/Release/Standalone/Bakterizid.app"
