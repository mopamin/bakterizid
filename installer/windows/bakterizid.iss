; Windows-Installer für Bakterizid (Inno Setup 6)
; Bauen:  ISCC.exe /DAppVersion=1.0.3 installer\windows\bakterizid.iss
; Erwartet die fertigen Dateien unter build\Bakterizid_artefacts\Release

#ifndef AppVersion
  #define AppVersion "1.0.3"
#endif
#define Art "..\..\build\Bakterizid_artefacts\Release"

[Setup]
AppId={{6B3F2C1E-9A4D-4E7B-8C21-BA6E5D1F7A10}
AppName=Bakterizid
AppVersion={#AppVersion}
AppPublisher=Sarin
DefaultDirName={autopf}\Bakterizid
DefaultGroupName=Bakterizid
DisableProgramGroupPage=yes
OutputDir=..\..\dist
OutputBaseFilename=Bakterizid-{#AppVersion}-Windows-Setup
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
LicenseFile=..\..\LICENSE
WizardStyle=modern
Compression=lzma2
SolidCompression=yes
UninstallDisplayName=Bakterizid

[Languages]
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full"; Description: "Alles installieren"
Name: "custom"; Description: "Selbst auswählen"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3-Plugin (für Ableton, FL Studio, Cubase, Reaper, Bitwig ...)"; Types: full custom
Name: "app"; Description: "Standalone-App (Bakterizid ohne DAW spielen)"; Types: full custom

[Tasks]
Name: "desktopicon"; Description: "Verknüpfung auf dem Desktop anlegen"; Components: app; Flags: unchecked

[Files]
Source: "{#Art}\VST3\Bakterizid.vst3\*"; DestDir: "{commoncf64}\VST3\Bakterizid.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: vst3
Source: "{#Art}\Standalone\Bakterizid.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: app
Source: "..\..\LICENSE"; DestDir: "{app}"; DestName: "LICENSE.txt"; Flags: ignoreversion

[Icons]
Name: "{group}\Bakterizid"; Filename: "{app}\Bakterizid.exe"; Components: app
Name: "{autodesktop}\Bakterizid"; Filename: "{app}\Bakterizid.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\Bakterizid.exe"; Description: "Bakterizid jetzt starten"; Flags: nowait postinstall skipifsilent; Components: app

[Code]
// Die Oberfläche braucht die WebView2-Laufzeit von Microsoft. Auf Windows 11 und aktuellen
// Windows-10-Rechnern ist sie schon da. Fehlt sie, gibt es einen Hinweis mit Download-Adresse.
function WebView2Installed: Boolean;
var
  V: String;
begin
  Result :=
    RegQueryStringValue(HKLM, 'SOFTWARE\WOW6432Node\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}', 'pv', V) or
    RegQueryStringValue(HKLM, 'SOFTWARE\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}', 'pv', V) or
    RegQueryStringValue(HKCU, 'Software\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}', 'pv', V);
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep = ssPostInstall) and (not WebView2Installed) then
    MsgBox('Bakterizid braucht die kostenlose "Microsoft Edge WebView2 Runtime" für seine Oberfläche.' + #13#10 + #13#10 +
           'Sie scheint auf diesem PC zu fehlen. Lade sie hier herunter und installiere sie:' + #13#10 +
           'https://developer.microsoft.com/microsoft-edge/webview2/' + #13#10 + #13#10 +
           '(Dort den "Evergreen Bootstrapper" wählen.)', mbInformation, MB_OK);
end;
