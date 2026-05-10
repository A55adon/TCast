@echo off
setlocal EnableDelayedExpansion
title TCast Installer
color 0A

echo.
echo  =========================================
echo    TCast Installer
echo  =========================================
echo.

:: ── Check prerequisites ───────────────────────────────────────────────────────

where git >nul 2>&1
if errorlevel 1 (
    echo  [ERROR] Git is not installed or not in PATH.
    echo          Download it from https://git-scm.com/
    echo.
    pause
    exit /b 1
)

where cargo >nul 2>&1
if errorlevel 1 (
    echo  [ERROR] Rust/Cargo is not installed or not in PATH.
    echo          Download it from https://rustup.rs/
    echo.
    pause
    exit /b 1
)

:: ── Check if already installed ────────────────────────────────────────────────

set "REG_KEY=HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\TCast"
set "EXISTING_PATH="
for /f "tokens=2*" %%A in ('reg query "%REG_KEY%" /v InstallLocation 2^>nul') do set "EXISTING_PATH=%%B"

if defined EXISTING_PATH (
    echo  TCast is already installed at:
    echo    %EXISTING_PATH%
    echo.
    echo  What would you like to do?
    echo    [1] Update to latest version
    echo    [2] Reinstall
    echo    [3] Exit
    echo.
    set /p ACTION=" Choice: "

    if "!ACTION!"=="1" (
        echo.
        echo  Updating TCast...
        call "%EXISTING_PATH%\update.cmd"
        echo.
        echo  Update complete.
        pause
        exit /b 0
    )
    if "!ACTION!"=="3" exit /b 0
    if not "!ACTION!"=="2" exit /b 0

    echo.
    echo  Reinstalling. Removing old files...
    rd /s /q "%EXISTING_PATH%" 2>nul
)

:: ── Choose install path ───────────────────────────────────────────────────────

set "DEFAULT_PATH=%LOCALAPPDATA%\Programs\TCast"
echo.
echo  Default install location:
echo    %DEFAULT_PATH%
echo.
set /p CUSTOM=" Enter a custom path or press Enter to use default: "
if "!CUSTOM!"=="" (
    set "INSTALL_DIR=%DEFAULT_PATH%"
) else (
    set "INSTALL_DIR=!CUSTOM!"
)

echo.
echo  Installing to: !INSTALL_DIR!
echo.

:: ── Clone repository ─────────────────────────────────────────────────────────

echo  Cloning TCast from GitHub...
git clone --branch main --depth 1 https://github.com/A55adon/TCast "!INSTALL_DIR!"
if errorlevel 1 (
    echo.
    echo  [ERROR] Failed to clone the repository.
    pause
    exit /b 1
)

:: ── Build ─────────────────────────────────────────────────────────────────────

echo.
echo  Building TCast (this may take a few minutes)...
pushd "!INSTALL_DIR!\tcast-rust"
cargo build --release
if errorlevel 1 (
    popd
    echo.
    echo  [ERROR] Build failed. Check the output above for details.
    pause
    exit /b 1
)
popd

set "EXE_PATH=!INSTALL_DIR!\tcast-rust\target\release\tcast-rust.exe"
if not exist "!EXE_PATH!" (
    echo  [ERROR] Executable not found after build: !EXE_PATH!
    pause
    exit /b 1
)

:: ── Copy helper scripts into install dir ─────────────────────────────────────

copy /y "%~f0" "!INSTALL_DIR!\installer.cmd" >nul 2>&1
copy /y "%~dp0update.cmd"    "!INSTALL_DIR!\update.cmd"    >nul 2>&1
copy /y "%~dp0uninstall.cmd" "!INSTALL_DIR!\uninstall.cmd" >nul 2>&1

:: ── Start Menu shortcut ───────────────────────────────────────────────────────

set "START_MENU=%APPDATA%\Microsoft\Windows\Start Menu\Programs"
powershell -NoProfile -Command ^
  "$s=(New-Object -COM WScript.Shell).CreateShortcut('%START_MENU%\TCast.lnk');" ^
  "$s.TargetPath='!EXE_PATH!';" ^
  "$s.WorkingDirectory='!INSTALL_DIR!\tcast-rust\target\release';" ^
  "$s.Description='TCast';" ^
  "$s.Save()"
echo  Start Menu shortcut created.

:: ── Desktop shortcut (optional) ──────────────────────────────────────────────

echo.
set /p DESKTOP=" Create a Desktop shortcut? [Y/N]: "
if /i "!DESKTOP!"=="Y" (
    powershell -NoProfile -Command ^
      "$s=(New-Object -COM WScript.Shell).CreateShortcut([Environment]::GetFolderPath('Desktop')+'\TCast.lnk');" ^
      "$s.TargetPath='!EXE_PATH!';" ^
      "$s.WorkingDirectory='!INSTALL_DIR!\tcast-rust\target\release';" ^
      "$s.Description='TCast';" ^
      "$s.Save()"
    echo  Desktop shortcut created.
)

:: ── Register with Windows (Apps ^& Features) ─────────────────────────────────

for /f "tokens=*" %%V in ('git -C "!INSTALL_DIR!" describe --tags --always 2^>nul') do set "VERSION=%%V"
if "!VERSION!"=="" set "VERSION=1.0.0"

set "UNINSTALL_CMD=!INSTALL_DIR!\uninstall.cmd"

reg add "%REG_KEY%" /v DisplayName       /t REG_SZ    /d "TCast"                            /f >nul
reg add "%REG_KEY%" /v DisplayVersion    /t REG_SZ    /d "!VERSION!"                        /f >nul
reg add "%REG_KEY%" /v Publisher         /t REG_SZ    /d "A55adon"                          /f >nul
reg add "%REG_KEY%" /v InstallLocation   /t REG_SZ    /d "!INSTALL_DIR!"                    /f >nul
reg add "%REG_KEY%" /v UninstallString   /t REG_SZ    /d "cmd /c \"!UNINSTALL_CMD!\""       /f >nul
reg add "%REG_KEY%" /v DisplayIcon       /t REG_SZ    /d "!EXE_PATH!"                       /f >nul
reg add "%REG_KEY%" /v NoModify          /t REG_DWORD /d 1                                  /f >nul
reg add "%REG_KEY%" /v NoRepair          /t REG_DWORD /d 1                                  /f >nul

:: ── Done ─────────────────────────────────────────────────────────────────────

echo.
echo  =========================================
echo    TCast installed successfully!
echo    You can find it in the Start Menu
echo    or by searching "TCast" in Windows.
echo  =========================================
echo.
set /p LAUNCH=" Launch TCast now? [Y/N]: "
if /i "!LAUNCH!"=="Y" start "" "!EXE_PATH!"

endlocal
pause
exit /b 0
