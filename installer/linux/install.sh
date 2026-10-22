#!/usr/bin/env bash
# Installiert Bakterizid für den aktuellen Benutzer:
#   VST3-Plugin  -> ~/.vst3/Bakterizid.vst3
#   Standalone   -> ~/.local/bin/Bakterizid
# Aufruf im entpackten Ordner:  ./install.sh
set -euo pipefail
cd "$(dirname "$0")"

mkdir -p ~/.vst3 ~/.local/bin
rm -rf ~/.vst3/Bakterizid.vst3
cp -r Bakterizid.vst3 ~/.vst3/
cp Bakterizid ~/.local/bin/
chmod +x ~/.local/bin/Bakterizid

echo "Bakterizid ist installiert."
echo "  VST3:       ~/.vst3/Bakterizid.vst3"
echo "  Standalone: ~/.local/bin/Bakterizid"
echo
echo "Die Oberfläche braucht WebKitGTK. Falls das Fenster leer bleibt:"
echo "  Ubuntu/Debian: sudo apt install libwebkit2gtk-4.1-0"
echo "  Fedora:        sudo dnf install webkit2gtk4.1"
echo "  Arch:          sudo pacman -S webkit2gtk-4.1"
