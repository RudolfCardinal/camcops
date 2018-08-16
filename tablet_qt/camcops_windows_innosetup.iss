; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!
;
; RNC: looks like InnoSetup uses many similar variables names and that's why the
; convention is to use "My" as a prefix for the Innosetup preprocessor (ISPP) add-on.
;
; For simultaneous 32/64-bit use, see
; https://stackoverflow.com/questions/4833831/inno-setup-32bit-and-64bit-in-one

#define MyAppName "CamCOPS"
#define MyAppNameLowerCase "camcops"
#define MyAppVersion "2.2.6"
#define MyAppPublisher "Rudolf Cardinal"
#define MyAppURL "http://www.camcops.org/"
#define SrcBaseDir "D:\dev\camcops"
#define QtBuildDir "D:\dev\qt_local_build"
#define OpenSSLVersion "1.1.0g"
#define OpenSSLMajorVersionUnderscores "1_1"

; It looks like ISPP can't do a #define involving an existing #define...
; ... but you can define macros and compositions.
; So this fails: #define MyAppExeName "{#MyAppNameLowerCase}.exe"
; But this works: #define MyAppExeName MyAppNameLowerCase + ".exe"

#define MyAppExeName MyAppNameLowerCase + ".exe"
#define InstallableOutputDir SrcBaseDir + "\distributables"
#define SrcTabletDir SrcBaseDir + "\tablet_qt"
#define SrcBuild32Dir SrcBaseDir + "\build-camcops-Custom_Windows_x86_32-Release\release"
#define SrcBuild64Dir SrcBaseDir + "\build-camcops-Custom_Windows_x86_64-Release\release"
#define SrcExe32 SrcBuild32Dir + "\" + MyAppNameLowerCase + ".exe"
#define SrcExe64 SrcBuild64Dir + "\" + MyAppNameLowerCase + ".exe"
#define LibCrypto32 QtBuildDir + "\openssl_windows_x86_32_build\openssl-" + OpenSSLVersion + "\libcrypto-" + OpenSSLMajorVersionUnderscores + ".dll"
#define LibCrypto64 QtBuildDir + "\openssl_windows_x86_64_build\openssl-" + OpenSSLVersion + "\libcrypto-" + OpenSSLMajorVersionUnderscores + "-x64.dll"
#define LibSSL32 QtBuildDir + "\openssl_windows_x86_32_build\openssl-" + OpenSSLVersion + "\libssl-" + OpenSSLMajorVersionUnderscores + ".dll"
#define LibSSL64 QtBuildDir + "\openssl_windows_x86_64_build\openssl-" + OpenSSLVersion + "\libssl-" + OpenSSLMajorVersionUnderscores + "-x64.dll"
#define IconName MyAppNameLowerCase + ".ico"
#define SrcIconFilename SrcTabletDir + "\windows\" + IconName

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{29B3F489-C33C-4915-A3DB-DEA9F53E2E79}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
Compression=lzma
DefaultDirName={pf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
LicenseFile={#SrcBaseDir}\LICENSE.txt
OutputDir={#InstallableOutputDir}
OutputBaseFilename={#MyAppNameLowerCase}_{#MyAppVersion}_windows
SetupIconFile={#SrcIconFilename}
SolidCompression=yes
UninstallDisplayIcon={app}\{#IconName}

; "ArchitecturesInstallIn64BitMode=x64" requests that the install be
; done in "64-bit mode" on x64, meaning it should use the native
; 64-bit Program Files directory and the 64-bit view of the registry.
; On all other architectures it will install in "32-bit mode".
ArchitecturesInstallIn64BitMode=x64
; Note: We don't set ProcessorsAllowed because we want this
; installation to run on all architectures (including Itanium,
; since it's capable of running 32-bit code too).

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

; Main executable.
; Install 64-bit version if running in 64-bit mode (x64; see above), otherwise 32-bit version.
Source: "{#SrcExe32}"; DestDir: "{app}"; DestName: "{#MyAppExeName}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "{#SrcExe64}"; DestDir: "{app}"; DestName: "{#MyAppExeName}"; Flags: ignoreversion; Check: Is64BitInstallMode

; We need libcrypto and libssl too.
Source: "{#LibCrypto32}"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "{#LibCrypto64}"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode
Source: "{#LibSSL32}"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "{#LibSSL64}"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode

; Other files:
Source: "{#SrcIconFilename}"; DestDir: "{app}"; DestName: "{#IconName}"
; Source: "Readme.txt"; DestDir: "{app}"; Flags: isreadme

[Icons]
; The "[Icons]" section means "shortcuts".
Name: "{commonprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\{#IconName}"
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\{#IconName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

