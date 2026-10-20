; Generated.iss is written by BuildBeta.ps1; do not hand-edit it.
#include "Generated.iss"

[Setup]
AppId={{5BA860F0-CE7A-47E1-887D-EC02489DB723}
AppName=WILDLINE Beta
AppVersion={#BetaVersion}
AppPublisher=hot_turd
DefaultDirName={autopf}\WILDLINE Beta
UsePreviousAppDir=yes
DisableDirPage=auto
DefaultGroupName=WILDLINE Beta
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64os
ArchitecturesInstallIn64BitMode=x64os
MinVersion=10.0.19041
PrivilegesRequired=lowest
OutputDir={#OutputPath}
OutputBaseFilename=Wildline-{#BetaVersion}-Setup
; Cooked IoStore chunks are already compressed. Deflate avoids expensive solid
; recompression and keeps full installs and repairs fast with bounded memory.
Compression=zip/3
SolidCompression=no
WizardStyle=modern
UninstallDisplayIcon={app}\WildlineLauncher.exe
InfoBeforeFile=BetaReadme.txt
SetupLogging=yes
CloseApplications=yes
RestartApplications=no
ExtraDiskSpaceRequired=536870912

[Tasks]
Name: desktopicon; Description: "Create a desktop shortcut"; GroupDescription: "Shortcuts:"; Flags: unchecked

[Files]
Source: "{#PayloadPath}\*"; DestDir: "{app}\versions\{#BetaVersion}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#LauncherPath}"; DestDir: "{app}"; Flags: ignoreversion
Source: "BetaReadme.txt"; DestDir: "{app}"; DestName: "WildlineGuide.txt"; Flags: ignoreversion
Source: "{#RedistPath}\vc_redist.x64.exe"; Flags: dontcopy
Source: "{#RedistPath}\GameInputRedist.msi"; Flags: dontcopy

[InstallDelete]
; A different version is never removed during update. Same-version repair
; replaces only immutable packaged trees; Saved and AppData stay intact.
Type: filesandordirs; Name: "{app}\versions\{#BetaVersion}\Engine"
Type: filesandordirs; Name: "{app}\versions\{#BetaVersion}\Wildline\Binaries"
Type: filesandordirs; Name: "{app}\versions\{#BetaVersion}\Wildline\Content"
Type: filesandordirs; Name: "{app}\versions\{#BetaVersion}\Wildline\Config"

[UninstallDelete]
Type: files; Name: "{app}\current.version"
Type: files; Name: "{app}\previous.version"

[Icons]
Name: "{group}\WILDLINE Beta"; Filename: "{app}\WildlineLauncher.exe"; WorkingDir: "{app}"
Name: "{group}\WILDLINE Beta (offline)"; Filename: "{app}\WildlineLauncher.exe"; Parameters: "--offline"; WorkingDir: "{app}"
Name: "{group}\Beta testing guide"; Filename: "{app}\WildlineGuide.txt"
Name: "{autodesktop}\WILDLINE Beta"; Filename: "{app}\WildlineLauncher.exe"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\WildlineLauncher.exe"; WorkingDir: "{app}"; Description: "Launch WILDLINE Beta"; Flags: nowait postinstall skipifsilent runasoriginaluser; Check: CanLaunch

[Code]
var
  RuntimeRestart: Boolean;
  PrerequisiteLogDir: String;
  PreviousInstallDir: String;
  ActivationFailed: Boolean;

function GetCustomSetupExitCode: Integer;
begin
  if ActivationFailed then Result := 1 else Result := 0;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  ExitCode: Integer;
begin
  if CurStep = ssPostInstall then begin
    WizardForm.StatusLabel.Caption := 'Verifying and activating WILDLINE...';
    if not Exec(ExpandConstant('{app}\WildlineLauncher.exe'), '--activate {#BetaVersion}',
      ExpandConstant('{app}'), SW_HIDE, ewWaitUntilTerminated, ExitCode) then begin
      ActivationFailed := True;
      RaiseException('Could not verify the new game. The previous version remains selected.');
    end;
    if ExitCode <> 0 then begin
      ActivationFailed := True;
      RaiseException('Game verification failed. The previous version remains selected.');
    end;
  end;
end;

function IsGameInstallation: Boolean;
begin
  { A registered installation may need repair even if its EXE is missing. }
  Result := (PreviousInstallDir <> '') or
    FileExists(ExpandConstant('{app}\BetaBuild.json'));
end;

procedure InitializeWizard;
begin
  if PreviousInstallDir <> '' then begin
    { Always update the registered installation, even if /DIR was supplied. }
    WizardForm.DirEdit.Text := PreviousInstallDir;
    WizardForm.DirEdit.Enabled := False;
    Log('Replacing previous installation in ' + PreviousInstallDir);
  end;
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  Result := (PageID = wpSelectDir) and (PreviousInstallDir <> '');
end;

function VersionAtLeast(const Filename, Required: String): Boolean;
var
  Installed: String;
  InstalledVersion, RequiredVersion: Int64;
begin
  Installed := '';
  Result := False;
  if GetVersionNumbersString(Filename, Installed) then
    if StrToVersion(Installed, InstalledVersion) and StrToVersion(Required, RequiredVersion) then
      Result := ComparePackedVersion(InstalledVersion, RequiredVersion) >= 0;
  Log(Filename + ' version: ' + Installed + '; required: ' + Required);
end;

function HasVCRuntime: Boolean;
begin
  Result := VersionAtLeast(ExpandConstant('{sys}\vcruntime140.dll'), '{#VCVersion}') and
    VersionAtLeast(ExpandConstant('{sys}\vcruntime140_1.dll'), '{#VCVersion}') and
    VersionAtLeast(ExpandConstant('{sys}\msvcp140.dll'), '{#VCVersion}');
end;

function HasGameInput: Boolean;
begin
  Result := VersionAtLeast(ExpandConstant('{sys}\GameInputRedist.dll'), '{#GameInputVersion}');
end;

function InitializeSetup: Boolean;
var
  OS: TWindowsVersion;
  Revision: Cardinal;
begin
  PreviousInstallDir := '';
  if not RegQueryStringValue(HKCU64,
    'Software\Microsoft\Windows\CurrentVersion\Uninstall\{5BA860F0-CE7A-47E1-887D-EC02489DB723}_is1',
    'InstallLocation', PreviousInstallDir) then
    RegQueryStringValue(HKCU32,
      'Software\Microsoft\Windows\CurrentVersion\Uninstall\{5BA860F0-CE7A-47E1-887D-EC02489DB723}_is1',
      'InstallLocation', PreviousInstallDir);
  GetWindowsVersionEx(OS);
  Result := True;
  if (OS.Build >= 19041) and (OS.Build <= 19045) then begin
    Revision := 0;
    RegQueryDWordValue(HKLM64, 'SOFTWARE\Microsoft\Windows NT\CurrentVersion', 'UBR', Revision);
    Result := Revision >= 789;
  end;
  if not Result then
    SuppressibleMsgBox('Windows needs an update. WILDLINE requires Windows 10 version 2004 with update .789 or newer, or Windows 11.', mbCriticalError, MB_OK, IDOK);
  if Result and not FileExists(ExpandConstant('{sys}\d3d12.dll')) then begin
    Result := False;
    SuppressibleMsgBox('DirectX 12 is unavailable. Update Windows and your graphics driver, then run Setup again.', mbCriticalError, MB_OK, IDOK);
  end;
end;

function RunPrerequisite(const Filename, Parameters, DisplayName: String): String;
var
  ExitCode: Integer;
begin
  Result := '';
  ExitCode := -1;
  WizardForm.StatusLabel.Caption := 'Checking/installing ' + DisplayName + '...';
  if not ShellExec('runas', Filename, Parameters, '', SW_HIDE, ewWaitUntilTerminated, ExitCode) then
    Result := 'Could not start ' + DisplayName + '. System error: ' + IntToStr(ExitCode)
  else if (ExitCode = 3010) or (ExitCode = 1641) then
    RuntimeRestart := True
  else if ExitCode <> 0 then
    Result := DisplayName + ' installation failed (exit ' + IntToStr(ExitCode) + '). See logs in ' + PrerequisiteLogDir + '. Restart Windows and run Setup again.';
  Log(DisplayName + ' result: ' + Result + '; exit: ' + IntToStr(ExitCode));
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result := '';
  PrerequisiteLogDir := ExpandConstant('{localappdata}\Wildline\InstallerLogs');
  if not ForceDirectories(PrerequisiteLogDir) then begin
    Result := 'Cannot create prerequisite log directory: ' + PrerequisiteLogDir;
    exit;
  end;
  if not HasVCRuntime then begin
    ExtractTemporaryFile('vc_redist.x64.exe');
    Result := RunPrerequisite(ExpandConstant('{tmp}\vc_redist.x64.exe'),
      '/install /quiet /norestart /log "' + PrerequisiteLogDir + '\VisualCpp.log"', 'Microsoft Visual C++ x64 runtime');
    if Result <> '' then exit;
    if not HasVCRuntime and not RuntimeRestart then begin
      Result := 'Visual C++ installation completed but the required runtime is still missing. Restart Windows and retry Setup.';
      exit;
    end;
  end;
  if not HasGameInput then begin
    ExtractTemporaryFile('GameInputRedist.msi');
    Result := RunPrerequisite(ExpandConstant('{sys}\msiexec.exe'),
      ExpandConstant('/i "{tmp}\GameInputRedist.msi" /qn /norestart /L*v "') + PrerequisiteLogDir + '\GameInput.log"', 'Microsoft GameInput');
    if Result <> '' then exit;
    if not HasGameInput and not RuntimeRestart then
      Result := 'GameInput installation completed but the required runtime is still missing. Restart Windows and retry Setup.';
  end;
end;

function NeedRestart: Boolean;
begin
  Result := RuntimeRestart;
end;

function CanLaunch: Boolean;
begin
  Result := not RuntimeRestart;
end;
