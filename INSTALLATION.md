# Bakterizid installieren

Diese Anleitung ist für alle, die einfach nur spielen wollen. Du musst nichts programmieren.

## 1. Herunterladen

Geh auf der GitHub-Seite von Bakterizid rechts auf **Releases** und lade die Datei für dein System:

| System | Datei |
|---|---|
| Mac (Apple Silicon und Intel) | `Bakterizid-…-macOS.pkg` |
| Windows 10 / 11 (64 Bit) | `Bakterizid-…-Windows-Setup.exe` |
| Linux | `Bakterizid-Linux.tar.gz` |

## 2. Mac

1. Doppelklick auf die `.pkg`-Datei.
2. Der Mac meldet vermutlich: *„Bakterizid… kann nicht geöffnet werden, da Apple darin nicht nach Schadsoftware suchen kann.“*
   Das liegt daran, dass Bakterizid ein kostenloses Hobbyprojekt ohne kostenpflichtiges Apple-Zertifikat ist.
   So geht es trotzdem:
   - Klick auf **Fertig** (nicht „In den Papierkorb legen“).
   - Öffne **Systemeinstellungen → Datenschutz & Sicherheit**.
   - Scroll nach unten. Dort steht ein Hinweis zu Bakterizid. Klick auf **Dennoch öffnen** und bestätige mit deinem Passwort.
3. Klick dich durch den Installer. Unter **Anpassen** kannst du VST3, Audio Unit oder die App abwählen.
4. Starte deine DAW neu. In Logic Pro findest du Bakterizid unter **AU-Instrumente → Sarin → Bakterizid**.

Die Standalone-App liegt danach im **Programme**-Ordner.

## 3. Windows

1. Doppelklick auf `Bakterizid-…-Windows-Setup.exe`.
2. Windows zeigt vermutlich einen blauen Kasten: *„Der Computer wurde durch Windows geschützt.“*
   Auch das liegt nur am fehlenden kostenpflichtigen Zertifikat. So geht es trotzdem:
   - Klick auf **Weitere Informationen**.
   - Klick auf **Trotzdem ausführen**.
3. Klick dich durch den Installer. Das VST3-Plugin landet automatisch in `C:\Program Files\Common Files\VST3`,
   dort suchen alle gängigen DAWs.
4. Starte deine DAW neu. Falls Bakterizid nicht auftaucht: in den DAW-Einstellungen **nach Plugins suchen / Plugins neu scannen**.

Bleibt das Bakterizid-Fenster leer oder weiß, fehlt die **Microsoft Edge WebView2 Runtime**.
Der Installer weist dich darauf hin. Download: <https://developer.microsoft.com/microsoft-edge/webview2/> (dort den „Evergreen Bootstrapper“ nehmen).

## 4. Linux

```bash
tar -xzf Bakterizid-Linux.tar.gz
cd Bakterizid-Linux
./install.sh
```

Das Plugin landet in `~/.vst3`, die App in `~/.local/bin`. Bleibt das Fenster leer, fehlt WebKitGTK
(`sudo apt install libwebkit2gtk-4.1-0` auf Ubuntu und Debian).

## 5. Erste Schritte

- **Preset wählen:** oben in der Mitte. Es gibt Drums, Bässe, Keys, Leads, Pads und schräge Kulturen.
- **Spielen:** mit deinem MIDI-Keyboard, der Klaviatur unten oder der Computertastatur (A W S E D F …).
- **Petrischale:** hineinklicken und ziehen. Links nach rechts ist die Tonhöhe, oben nach unten der Filter.
- **Regler:** ziehen, mit Shift fein, Doppelklick setzt zurück.
- **Bakterizid** (roter Knopf): tötet alle Töne mit einem Rauschsturz ab.
- **Zellteilung:** spielt selbst zufällige Melodien.

## Deinstallieren

- **Mac:** `Bakterizid.vst3` aus `/Library/Audio/Plug-Ins/VST3`, `Bakterizid.component` aus `/Library/Audio/Plug-Ins/Components` und die App aus **Programme** löschen.
- **Windows:** Einstellungen → Apps → Bakterizid → Deinstallieren.
- **Linux:** `~/.vst3/Bakterizid.vst3` und `~/.local/bin/Bakterizid` löschen.
