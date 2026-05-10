# TCast Installer

TCast uses an Inno Setup installer for the normal Windows install/update/uninstall flow.

## Build

Install Inno Setup 6, then run:

```powershell
powershell -ExecutionPolicy Bypass -File .\installer\build-installer.ps1
```

The script uses `tcast-rust\target\release\tcast-rust.exe` if it already exists. If it does not exist, it runs `cargo build --release`.

The output is:

```text
dist\TCast-Setup.exe
```

## Install And Update

Run `dist\TCast-Setup.exe`.

To update TCast, build a newer `TCast-Setup.exe` and run it again. Inno Setup uses the same app id, so it upgrades the existing install instead of creating a second app.

## Uninstall

Use Windows Settings, Apps and Features, or the Start Menu uninstall entry created by Windows. The Inno Setup uninstaller removes shortcuts and the `.tct` file association.

The old `.cmd` scripts are legacy fallbacks. The setup exe is the recommended path.
