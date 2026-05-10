@echo off
setlocal EnableDelayedExpansion
title TCast Updater
color 0B

echo.
echo  =========================================
echo    TCast Updater
echo  =========================================
echo.

:: ── Find install location from registry ──────────────────────────────────────

set "REG_KEY=HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\TCast"
set "INSTALL_DIR="
for /f "tokens=2*" %%A in ('reg query "%REG_KEY%" /v InstallLocation 2^>nul') do set "INSTALL_DIR=%%B"

:: Fallback: script's own directory
if not defined INSTALL_DIR set "INSTALL_DIR=%~dp0"
if "!INSTALL_DIR:~-1!"=="\" set "INSTALL_DIR=!INSTALL_DIR:~0,-1!"

if not exist "!INSTALL_DIR!" (
    echo  [ERROR] Install directory not found: !INSTALL_DIR!
    pause
    exit /b 1
)

:: ── Check prerequisites ───────────────────────────────────────────────────────

where git >nul 2>&1
if errorlevel 1 (
    echo  [ERROR] Git is not installed or not in PATH.
    pause
    exit /b 1
)

where cargo >nul 2>&1
if errorlevel 1 (
    echo  [ERROR] Rust/Cargo is not installed or not in PATH.
    pause
    exit /b 1
)

:: ── Pull latest changes ───────────────────────────────────────────────────────

echo  Install directory: !INSTALL_DIR!
echo.
echo  Fetching latest version from GitHub...
git -C "!INSTALL_DIR!" fetch origin main
if errorlevel 1 (
    echo  [ERROR] Could not reach GitHub. Check your internet connection.
    pause
    exit /b 1
)

for /f %%L in ('git -C "!INSTALL_DIR!" rev-parse HEAD') do set "OLD_COMMIT=%%L"
for /f %%L in ('git -C "!INSTALL_DIR!" rev-parse origin/main') do set "NEW_COMMIT=%%L"

if "!OLD_COMMIT!"=="!NEW_COMMIT!" (
    echo.
    echo  TCast is already up to date.
    pause
    exit /b 0
)

echo  Updating from !OLD_COMMIT:~0,7! to !NEW_COMMIT:~0,7!...
git -C "!INSTALL_DIR!" pull origin main
if errorlevel 1 (
    echo  [ERROR] Git pull failed.
    pause
    exit /b 1
)

:: ── Rebuild ───────────────────────────────────────────────────────────────────

echo.
echo  Rebuilding TCast...
pushd "!INSTALL_DIR!\tcast-rust"
cargo build --release
if errorlevel 1 (
    popd
    echo.
    echo  [ERROR] Build failed.
    pause
    exit /b 1
)
popd

set "EXE_PATH=!INSTALL_DIR!\tcast-rust\target\release\tcast-rust.exe"
set "ICON_PATH=!INSTALL_DIR!\assets\t-cast-favicon.ico"
if not exist "!ICON_PATH!" set "ICON_PATH=!EXE_PATH!"

:: ── Update version in registry ────────────────────────────────────────────────

for /f "tokens=*" %%V in ('git -C "!INSTALL_DIR!" describe --tags --always 2^>nul') do set "VERSION=%%V"
if not "!VERSION!"=="" (
    reg add "%REG_KEY%" /v DisplayVersion /t REG_SZ /d "!VERSION!" /f >nul
)
reg add "%REG_KEY%" /v DisplayIcon /t REG_SZ /d "!ICON_PATH!" /f >nul

reg add "HKCU\Software\Classes\.tct" /ve /d "TCast.Project" /f >nul
reg add "HKCU\Software\Classes\TCast.Project" /ve /d "TCast Project" /f >nul
reg add "HKCU\Software\Classes\TCast.Project\DefaultIcon" /ve /d "!ICON_PATH!" /f >nul
reg add "HKCU\Software\Classes\TCast.Project\shell\open\command" /ve /d "\"!EXE_PATH!\" \"%%1\"" /f >nul

:: ── Done ─────────────────────────────────────────────────────────────────────

echo.
echo  =========================================
echo    TCast updated successfully!
echo  =========================================
echo.

endlocal
pause
exit /b 0
