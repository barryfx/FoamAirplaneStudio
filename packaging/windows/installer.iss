#ifndef StageDir
  #error StageDir must name a validated Release staging directory
#endif
#ifndef OutputDir
  #error OutputDir is required
#endif
#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif

[Setup]
AppId={{C164F27B-D30B-4B61-A1B0-5855B4F52F23}
AppName=FoamAirplaneStudio
AppVersion={#AppVersion}
AppPublisher=Barry Foust
DefaultDirName={localappdata}\Programs\FoamAirplaneStudio
DefaultGroupName=FoamAirplaneStudio
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.17763
LicenseFile={#StageDir}\licenses\INSTALLER-LICENSES.txt
OutputDir={#OutputDir}
OutputBaseFilename=FoamAirplaneStudio-{#AppVersion}-Windows-x64-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\foamairplanestudio.exe
CloseApplications=yes
RestartApplications=no
SetupLogging=yes

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: unchecked

[Icons]
Name: "{userprograms}\FoamAirplaneStudio"; Filename: "{app}\foamairplanestudio.exe"
Name: "{userprograms}\FoamAirplaneStudio Licenses"; Filename: "{app}\licenses\INSTALLER-LICENSES.txt"
Name: "{userdesktop}\FoamAirplaneStudio"; Filename: "{app}\foamairplanestudio.exe"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\foamairplanestudio.exe"; Description: "Launch FoamAirplaneStudio"; Flags: nowait postinstall skipifsilent unchecked

[Code]
function InitializeSetup(): Boolean;
begin
  Result := True;
  if WizardSilent and (ExpandConstant('{param:ACCEPTLICENSES|no}') <> 'yes') then
  begin
    Log('Silent installation requires /ACCEPTLICENSES=yes after reviewing the supplied license terms.');
    Result := False;
  end;
end;

procedure VerifyLicense(const RelativeName, ExpectedHash: String);
var FileName: String;
begin
  FileName := ExpandConstant('{app}\licenses\') + RelativeName;
  if not FileExists(FileName) then
    RaiseException('Required license file is missing: ' + RelativeName);
  if CompareText(GetSHA256OfFile(FileName), ExpectedHash) <> 0 then
    RaiseException('License verification failed: ' + RelativeName);
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    #include "license-checks.iss"
    Log('All installed license files passed SHA-256 verification.');
  end;
end;
