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
ShowLanguageDialog=yes
LanguageDetectionMethod=none
UsePreviousLanguage=no
[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "chinesesimp"; MessagesFile: "{#SourceRoot}\installer\languages\ChineseSimplified.isl"
[CustomMessages]
english.DesktopShortcut=Create a desktop shortcut
chinesesimp.DesktopShortcut=创建桌面快捷方式
english.SettingsShortcut=QuietPin Settings
chinesesimp.SettingsShortcut=QuietPin 设置
english.ExitShortcut=Exit QuietPin
chinesesimp.ExitShortcut=退出 QuietPin
english.UninstallShortcut=Uninstall QuietPin
chinesesimp.UninstallShortcut=卸载 QuietPin
english.StartBackground=Start QuietPin in the background
chinesesimp.StartBackground=在后台启动 QuietPin
english.CloseFailed=QuietPin did not finish closing. Exit it from Settings, then retry.
chinesesimp.CloseFailed=QuietPin 尚未退出。请在设置中退出程序后重试。
[Tasks]
Name: "desktopicon"; Description: "{cm:DesktopShortcut}"; Flags: unchecked
[InstallDelete]
Type: files; Name: "{group}\QuietPin Settings.lnk"
Type: files; Name: "{group}\Exit QuietPin.lnk"
Type: files; Name: "{group}\Uninstall QuietPin.lnk"
Type: files; Name: "{group}\QuietPin 设置.lnk"
Type: files; Name: "{group}\退出 QuietPin.lnk"
Type: files; Name: "{group}\卸载 QuietPin.lnk"
Type: files; Name: "{app}\verification-v0.2.0.zh-CN.md"
Type: files; Name: "{app}\verification-v0.2.0.en.md"
Type: files; Name: "{app}\technical-design.zh-CN.md"
Type: files; Name: "{app}\pin-implementation-plan.zh-CN.md"
Type: files; Name: "{app}\language-settings.md"
[Files]
Source: "{#BinaryPath}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceRoot}\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceRoot}\README.zh.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceRoot}\CHANGELOG.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceRoot}\scripts\open-settings.cmd"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceRoot}\scripts\exit.cmd"; DestDir: "{app}"; Flags: ignoreversion
[Icons]
Name: "{group}\QuietPin"; Filename: "{app}\QuietPin.exe"
Name: "{group}\{cm:SettingsShortcut}"; Filename: "{app}\QuietPin.exe"; Parameters: "--settings"
Name: "{group}\{cm:ExitShortcut}"; Filename: "{app}\QuietPin.exe"; Parameters: "--exit"
Name: "{group}\{cm:UninstallShortcut}"; Filename: "{uninstallexe}"
Name: "{userdesktop}\QuietPin"; Filename: "{app}\QuietPin.exe"; Tasks: desktopicon
[Run]
Filename: "{app}\QuietPin.exe"; Description: "{cm:StartBackground}"; Flags: nowait postinstall skipifsilent
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
    Result := CustomMessage('CloseFailed');
end;
function InitializeUninstall: Boolean;
begin
  Result := StopOwnedInstance;
  if not Result then
    MsgBox(CustomMessage('CloseFailed'), mbError, MB_OK);
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
