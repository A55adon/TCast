@echo off
setlocal EnableDelayedExpansion
title TCast Uninstaller
color 0C

echo.
echo  =========================================
echo    TCast Uninstaller
echo  =========================================
echo.

:: ── Find install location from registry ──────────────────────────────────────

set "REG_KEY=HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\TCast"
set "INSTALL_DIR="
for /f "tokens=2*" %%A in ('reg query "%REG_KEY%" /v InstallLocation 2^>nul') do set "INSTALL_DIR=%%B"

:: Fallback: script's own directory
if not defined INSTALL_DIR (
    set "INSTALL_DIR=%~dp0"
    if "!INSTALL_DIR:~-1!"=="\" set "INSTALL_DIR=!INSTALL_DIR:~0,-1!"
)

echo  This will completely remove TCast from your system:
echo    - All program files in: !INSTALL_DIR!
echo    - Desktop shortcut
echo    - Start Menu shortcut
echo    - Windows registry entry
echo.
set /p CONFIRM=" Are you sure? This cannot be undone. [Y/N]: "
if /i not "!CONFIRM!"=="Y" (
    echo  Uninstall cancelled.
    pause
    exit /b 0
)

:: ── Kill running process ──────────────────────────────────────────────────────

echo.
echo  Stopping TCast if running...
taskkill /f /im tcast-rust.exe >nul 2>&1

:: ── Remove shortcuts ──────────────────────────────────────────────────────────

set "START_MENU=%APPDATA%\Microsoft\Windows\Start Menu\Programs\TCast.lnk"
set "DESKTOP=%USERPROFILE%\Desktop\TCast.lnk"

if exist "!START_MENU!" (
    del /f /q "!START_MENU!"
    echo  Removed Start Menu shortcut.
)
if exist "!DESKTOP!" (
    del /f /q "!DESKTOP!"
    echo  Removed Desktop shortcut.
)

:: ── Remove registry entry ─────────────────────────────────────────────────────

reg delete "%REG_KEY%" /f >nul 2>&1
echo  Removed registry entry.

:: ── Schedule install directory for deletion after exit ───────────────────────
:: (we can't delete our own running script's folder directly)

echo  Removing program files...

set "CLEANUP_SCRIPT=%TEMP%\tcast_cleanup_%RANDOM%.cmd"
(
    echo @echo off
    echo ping 127.0.0.1 -n 3 ^>nul
    echo rd /s /q "!INSTALL_DIR!" 2^>nul
    echo del /f /q "%CLEANUP_SCRIPT%" 2^>nul
) > "!CLEANUP_SCRIPT!"

start /min "" cmd /c "!CLEANUP_SCRIPT!"

:: ── Done ─────────────────────────────────────────────────────────────────────

echo.
echo  =========================================
echo    TCast has been uninstalled.
echo    Thank you for using TCast.
echo  =========================================
echo.

endlocal
pause
exit /b 0
