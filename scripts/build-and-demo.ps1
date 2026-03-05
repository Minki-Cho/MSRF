param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",

    [int]$AutoExitMs = 7000
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$solution = Join-Path $repoRoot "MSFR\MSFR.sln"

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere))
{
    throw "vswhere.exe not found. Install Visual Studio Build Tools or run from Developer PowerShell."
}

$msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe | Select-Object -First 1
if (-not $msbuild)
{
    throw "MSBuild.exe not found via vswhere."
}

Write-Host "[Build] $Configuration x64"
& $msbuild $solution /m /p:Configuration=$Configuration /p:Platform=x64
if ($LASTEXITCODE -ne 0)
{
    throw "Build failed with exit code $LASTEXITCODE"
}

& (Join-Path $PSScriptRoot "demo-smoke.ps1") -Configuration $Configuration -AutoExitMs $AutoExitMs
