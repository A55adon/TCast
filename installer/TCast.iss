#define MyAppName "TCast"
#define MyAppVersion "2.0.0-rust"
#define MyAppPublisher "A55adon"
#define MyAppExeName "TCast.exe"
#define SourceRoot ".."

[Setup]
AppId={{C0F6AE1F-9F2B-4F4A-A2E8-4B46F5745D5D}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL=https://github.com/A55adon/TCast
AppSupportURL=https://github.com/A55adon/TCast/issues
AppUpdatesURL=https://github.com/A55adon/TCast/releases
DefaultDirName={localappdata}\Programs\TCast
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputDir={#SourceRoot}\dist
OutputBaseFilename=TCast-Setup
SetupIconFile={#SourceRoot}\assets\t-cast-favicon.ico
UninstallDisplayIcon={app}\assets\t-cast-favicon.ico
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=lowest
ChangesAssociations=yes
MinVersion=10.0
CloseApplications=yes
RestartApplications=no
VersionInfoVersion=2.0.0.0
VersionInfoCompany={#MyAppPublisher}
VersionInfoDescription=TCast Installer
VersionInfoProductName={#MyAppName}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#SourceRoot}\tcast-rust\target\release\tcast-rust.exe"; DestDir: "{app}"; DestName: "{#MyAppExeName}"; Flags: ignoreversion
Source: "{#SourceRoot}\assets\t-cast.png"; DestDir: "{app}\assets"; Flags: ignoreversion
Source: "{#SourceRoot}\assets\t-cast-favicon.png"; DestDir: "{app}\assets"; Flags: ignoreversion
Source: "{#SourceRoot}\assets\t-cast-favicon.ico"; DestDir: "{app}\assets"; Flags: ignoreversion
Source: "{#SourceRoot}\README.md"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist

[Icons]
Name: "{group}\TCast"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; IconFilename: "{app}\assets\t-cast-favicon.ico"
Name: "{autodesktop}\TCast"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; IconFilename: "{app}\assets\t-cast-favicon.ico"; Tasks: desktopicon

[Registry]
Root: HKCU; Subkey: "Software\Classes\.tct"; ValueType: string; ValueName: ""; ValueData: "TCast.Project"; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\TCast.Project"; ValueType: string; ValueName: ""; ValueData: "TCast Project"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\TCast.Project\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\assets\t-cast-favicon.ico"
Root: HKCU; Subkey: "Software\Classes\TCast.Project\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,TCast}"; Flags: nowait postinstall skipifsilent
