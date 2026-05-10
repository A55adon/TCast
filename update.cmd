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
if defined TCAST_INSTALL_DIR (
    set "INSTALL_DIR=%TCAST_INSTALL_DIR%"
) else (
    for /f "tokens=2*" %%A in ('reg query "%REG_KEY%" /v InstallLocation 2^>nul') do set "INSTALL_DIR=%%B"
)

:: Fallback: script's own directory
if not defined INSTALL_DIR set "INSTALL_DIR=%~dp0"
if "!INSTALL_DIR:~-1!"=="\" set "INSTALL_DIR=!INSTALL_DIR:~0,-1!"

if not exist "!INSTALL_DIR!" (
    echo  [ERROR] Install directory not found: !INSTALL_DIR!
    pause
    exit /b 1
)

:: Run from a temp copy so Git can update update.cmd safely.
if not defined TCAST_UPDATER_RELAUNCHED (
    set "TCAST_UPDATER_RELAUNCHED=1"
    set "TCAST_INSTALL_DIR=!INSTALL_DIR!"
    set "TEMP_UPDATER=%TEMP%\tcast_update_%RANDOM%.cmd"
    copy /y "%~f0" "!TEMP_UPDATER!" >nul
    call "!TEMP_UPDATER!"
    set "UPDATE_EXIT=!ERRORLEVEL!"
    del /f /q "!TEMP_UPDATER!" >nul 2>&1
    exit /b !UPDATE_EXIT!
)

:: ── Check prerequisites ───────────────────────────────────────────────────────

where git >nul 2>&1
if errorlevel 1 (
    echo  [ERROR] Git is not installed or not in PATH.
    pause
    exit /b 1
)

:: ── Pull latest changes ───────────────────────────────────────────────────────

echo  Install directory: !INSTALL_DIR!
echo.
echo  Protecting local runtime files before update...
set "STASH_PATHS="
if exist "!INSTALL_DIR!\installer.cmd" set "STASH_PATHS=!STASH_PATHS! installer.cmd"
if exist "!INSTALL_DIR!\update.cmd" set "STASH_PATHS=!STASH_PATHS! update.cmd"
if exist "!INSTALL_DIR!\uninstall.cmd" set "STASH_PATHS=!STASH_PATHS! uninstall.cmd"
if exist "!INSTALL_DIR!\logs" set "STASH_PATHS=!STASH_PATHS! logs"
if exist "!INSTALL_DIR!\saves\recent.path" set "STASH_PATHS=!STASH_PATHS! saves/recent.path"
if defined STASH_PATHS (
    git -C "!INSTALL_DIR!" stash push --include-untracked -m "TCast updater local files %DATE% %TIME%" -- !STASH_PATHS! >nul 2>&1
)

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
git -C "!INSTALL_DIR!" merge --ff-only origin/main
if errorlevel 1 (
    echo  [ERROR] Git update failed.
    pause
    exit /b 1
)

:: ── Rebuild ───────────────────────────────────────────────────────────────────

set "EXE_PATH=!INSTALL_DIR!\tcast-rust\target\release\tcast-rust.exe"
set "HAS_PREBUILT=0"
git -C "!INSTALL_DIR!" ls-files --error-unmatch "tcast-rust/target/release/tcast-rust.exe" >nul 2>&1
if not errorlevel 1 if exist "!EXE_PATH!" set "HAS_PREBUILT=1"

if "!HAS_PREBUILT!"=="1" (
    echo.
    echo  Found tracked prebuilt executable. Skipping Rust build.
) else (
    where cargo >nul 2>&1
    if errorlevel 1 (
        echo.
        echo  [ERROR] No tracked prebuilt executable was found and Rust/Cargo is not installed.
        echo          Expected tracked file: tcast-rust\target\release\tcast-rust.exe
        pause
        exit /b 1
    )

    echo.
    echo  No tracked prebuilt executable found.
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
)

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
