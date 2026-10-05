#ifndef AppVersion
  #define AppVersion "0.2.0"
#endif
#ifndef SourceRoot
  #define SourceRoot ".."
#endif
#ifndef BinaryPath
  #define BinaryPath SourceRoot + "\build\QuietPin.exe"
#endif
[Setup]
AppId={{83C207BC-33D3-4AC0-A31C-74F595C253F4}
AppName=QuietPin
AppVersion={#AppVersion}
AppPublisher=QuietPin
AppPublisherURL=https://github.com/LE-saber/QuietPin
DefaultDirName={localappdata}\Programs\QuietPin
DefaultGroupName=QuietPin
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.22000
OutputDir={#SourceRoot}\dist
OutputBaseFilename=QuietPinSetup-v{#AppVersion}-win-x64
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\QuietPin.exe
CloseApplications=yes
RestartApplications=no
SetupLogging=yes
VersionInfoVersion={#AppVersion}
[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; Flags: unchecked
[Files]
Source: "{#BinaryPath}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceRoot}\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceRoot}\README.en.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceRoot}\CHANGELOG.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceRoot}\docs\verification-v0.2.0.zh-CN.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceRoot}\docs\verification-v0.2.0.en.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceRoot}\scripts\open-settings.cmd"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceRoot}\scripts\exit.cmd"; DestDir: "{app}"; Flags: ignoreversion
[Icons]
Name: "{group}\QuietPin"; Filename: "{app}\QuietPin.exe"
Name: "{group}\QuietPin Settings"; Filename: "{app}\QuietPin.exe"; Parameters: "--settings"
Name: "{group}\Exit QuietPin"; Filename: "{app}\QuietPin.exe"; Parameters: "--exit"
Name: "{group}\Uninstall QuietPin"; Filename: "{uninstallexe}"
Name: "{userdesktop}\QuietPin"; Filename: "{app}\QuietPin.exe"; Tasks: desktopicon
[Run]
Filename: "{app}\QuietPin.exe"; Description: "Start QuietPin in the background"; Flags: nowait postinstall skipifsilent
[Code]
function StopOwnedInstance: Boolean;
var
  ResultCode: Integer;
  Exe: String;
begin
  Exe := ExpandConstant('{app}\QuietPin.exe');
  Result := True;
  if FileExists(Exe) then
    Result := Exec(Exe, '--exit-if-owned', '', SW_HIDE, ewWaitUntilTerminated, ResultCode) and (ResultCode = 0);
end;
function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result := '';
  if not StopOwnedInstance then
    Result := 'QuietPin did not finish closing. Exit it from Settings, then retry.';
end;
function InitializeUninstall: Boolean;
begin
  Result := StopOwnedInstance;
  if not Result then
    MsgBox('QuietPin did not finish closing. Exit it from Settings, then retry.', mbError, MB_OK);
end;
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  Existing, Expected: String;
begin
  if CurUninstallStep = usUninstall then begin
    Expected := '"' + ExpandConstant('{app}\QuietPin.exe') + '" --startup';
    if RegQueryStringValue(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Run', 'QuietPin', Existing) and
       (CompareText(Existing, Expected) = 0) then
      RegDeleteValue(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Run', 'QuietPin');
  end;
end;
