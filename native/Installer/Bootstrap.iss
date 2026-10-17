; Small evergreen installer. The signed feed selects the current beta build.
#include "Generated.iss"
[Setup]
AppId={{5BA860F0-CE7A-47E1-887D-EC02489DB723}
AppName=WILDLINE Beta
AppVersion={#BetaVersion}
AppPublisher=hot_turd
DefaultDirName={localappdata}\Programs\WILDLINE Beta
UsePreviousAppDir=yes
DisableDirPage=auto
DefaultGroupName=WILDLINE Beta
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64os
ArchitecturesInstallIn64BitMode=x64os
MinVersion=10.0.19041
PrivilegesRequired=lowest
OutputDir={#OutputPath}
OutputBaseFilename=WildlineSetup
Compression=lzma2
WizardStyle=modern
UninstallDisplayIcon={app}\WildlineLauncher.exe
SetupLogging=yes
CloseApplications=yes
[Files]
Source: "{#LauncherPath}"; DestDir: "{app}"; Flags: ignoreversion
Source: "BetaReadme.txt"; DestDir: "{app}"; DestName: "WildlineGuide.txt"; Flags: ignoreversion
[Tasks]
Name: desktopicon; Description: "Create a desktop shortcut"; Flags: checkedonce
[Icons]
Name: "{group}\WILDLINE Beta"; Filename: "{app}\WildlineLauncher.exe"; WorkingDir: "{app}"
Name: "{autodesktop}\WILDLINE Beta"; Filename: "{app}\WildlineLauncher.exe"; WorkingDir: "{app}"; Tasks: desktopicon
[Run]
Filename: "{app}\WildlineLauncher.exe"; WorkingDir: "{app}"; Description: "Download and launch WILDLINE"; Flags: nowait postinstall skipifsilent runasoriginaluser
