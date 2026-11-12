#define AppName      "Vethida"
#define AppPublisher "Rens Engineering"
#define AppExe       "Vethida.exe"

#define VersionFile  FileOpen(AddBackslash(SourcePath) + "..\VERSION")
#define AppVersion   Trim(FileRead(VersionFile))
#expr FileClose(VersionFile)

#ifndef StageDir
  #define StageDir AddBackslash(SourcePath) + "..\out\stage"
#endif

[Setup]
; Never change AppId: it is how an update finds the installed copy.
AppId={{44E5A008-099A-4ACF-AB44-5CE50F569980}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL=https://vethida.com
AppSupportURL=https://vethida.com/support
AppUpdatesURL=https://vethida.com/download
AppCopyright=Copyright (C) {#AppPublisher}
VersionInfoVersion={#AppVersion}
; admin-only location since the app runs elevated
DefaultDirName={autopf}\{#AppName}
DisableProgramGroupPage=yes
DisableDirPage=auto
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; Windows 10 version 1903.
MinVersion=10.0.18362
LicenseFile=eula.txt
SetupIconFile=..\Vethida\resources\icons\vethida-blue.ico
UninstallDisplayIcon={app}\{#AppExe}
UninstallDisplayName={#AppName}
WizardStyle=modern
Compression=lzma2/ultra64
SolidCompression=yes
OutputDir=..\out\installer
OutputBaseFilename=VethidaSetup-{#AppVersion}
; [Run] relaunches it, not the Restart Manager
CloseApplications=yes
RestartApplications=no
AppMutex=VethidaApplicationMutex
#ifdef Sign
SignTool=signtool
SignedUninstaller=yes
#endif

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\{#AppExe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExe}"; Tasks: desktopicon

[Dirs]
; ledger, recovery data, logs. ACL'd to Administrators/SYSTEM in [Run]
Name: "{commonappdata}\{#AppPublisher}"
Name: "{commonappdata}\{#AppPublisher}\{#AppName}"

[UninstallRun]
; "Start with Windows" task
Filename: "{sys}\schtasks.exe"; Parameters: "/Delete /TN ""Vethida\Start at sign-in"" /F"; Flags: runhidden; RunOnceId: "RemoveSignInTask"

[Run]
Filename: "{sys}\icacls.exe"; Parameters: """{commonappdata}\{#AppPublisher}"" /inheritance:r /grant:r *S-1-5-18:(OI)(CI)F *S-1-5-32-544:(OI)(CI)F"; Flags: runhidden waituntilterminated
Filename: "{sys}\icacls.exe"; Parameters: """{commonappdata}\{#AppPublisher}\{#AppName}"" /inheritance:r /grant:r *S-1-5-18:(OI)(CI)F *S-1-5-32-544:(OI)(CI)F"; Flags: runhidden waituntilterminated
; also runs after silent installs (WinSparkle updates)
Filename: "{app}\{#AppExe}"; Description: "{cm:LaunchProgram,{#AppName}}"; Flags: nowait postinstall

[Code]
// offer to remove user data + ProgramData on interactive uninstall
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  DataDir: String;
  StateDir: String;
begin
  if (CurUninstallStep = usPostUninstall) and not UninstallSilent then
  begin
    DataDir := ExpandConstant('{localappdata}\{#AppPublisher}\{#AppName}');
    StateDir := ExpandConstant('{commonappdata}\{#AppPublisher}');
    if DirExists(DataDir) or DirExists(StateDir) then
      if MsgBox('Also remove your Vethida settings, saved license, logs and change history from this PC? ' +
                'Without the change history, Vethida can no longer undo what it changed.',
                mbConfirmation, MB_YESNO or MB_DEFBUTTON2) = IDYES then
      begin
        DelTree(DataDir, True, True, True);
        DelTree(StateDir, True, True, True);
      end;
  end;
end;
