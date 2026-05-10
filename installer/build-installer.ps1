param(
    [switch]$SkipRustBuild
)

$ErrorActionPreference = "Stop"

$InstallerDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Resolve-Path (Join-Path $InstallerDir "..")
$RustDir = Join-Path $RepoRoot "tcast-rust"
$ExePath = Join-Path $RustDir "target\release\tcast-rust.exe"
$IconPath = Join-Path $RepoRoot "assets\t-cast-favicon.ico"
$ScriptPath = Join-Path $InstallerDir "TCast.iss"

function Find-InnoCompiler {
    $cmd = Get-Command "ISCC.exe" -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }

    $candidates = @(
        (Join-Path ${env:ProgramFiles(x86)} "Inno Setup 6\ISCC.exe"),
        (Join-Path $env:ProgramFiles "Inno Setup 6\ISCC.exe"),
        (Join-Path $env:LOCALAPPDATA "Programs\Inno Setup 6\ISCC.exe")
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    return $null
}

if (-not (Test-Path $IconPath)) {
    throw "Missing installer icon: $IconPath"
}

if (-not $SkipRustBuild) {
    if (-not (Test-Path $ExePath)) {
        Write-Host "No release executable found. Building Rust release..."
        Push-Location $RustDir
        try {
            cargo build --release
        } finally {
            Pop-Location
        }
    } else {
        Write-Host "Found release executable: $ExePath"
    }
}

if (-not (Test-Path $ExePath)) {
    throw "Release executable missing: $ExePath"
}

$iscc = Find-InnoCompiler
if (-not $iscc) {
    throw "Inno Setup 6 compiler was not found. Install it from https://jrsoftware.org/isdl.php, then rerun this script."
}

New-Item -ItemType Directory -Force -Path (Join-Path $RepoRoot "dist") | Out-Null

Write-Host "Compiling installer with Inno Setup..."
& $iscc $ScriptPath

if ($LASTEXITCODE -ne 0) {
    throw "Inno Setup failed with exit code $LASTEXITCODE"
}

Write-Host "Installer created: $(Join-Path $RepoRoot 'dist\TCast-Setup.exe')"
