param(
    [ValidateSet("Release")]
    [string]$Configuration = "Release",

    [string]$Version = "",

    [string]$OutputDir = "artifacts",

    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$projectRoot = Join-Path $repoRoot "MSFR"
$solution = Join-Path $projectRoot "MSFR.sln"

if ([string]::IsNullOrWhiteSpace($Version))
{
    $timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $Version = "local-$timestamp"
}

if (-not $SkipBuild)
{
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
    & $msbuild $solution /m /p:Configuration=$Configuration /p:Platform=x64 /nologo /v:minimal
    if ($LASTEXITCODE -ne 0)
    {
        throw "Build failed with exit code $LASTEXITCODE"
    }
}

$exePath = Join-Path $projectRoot "x64\$Configuration\MSFR.exe"
$sdlPath = Join-Path $projectRoot "x64\$Configuration\SDL2.dll"
$assetsPath = Join-Path $projectRoot "assets"
$licensePath = Join-Path $repoRoot "LICENSE"
$readmePath = Join-Path $repoRoot "README.md"

if (-not (Test-Path $exePath)) { throw "MSFR.exe not found at '$exePath'" }
if (-not (Test-Path $sdlPath)) { throw "SDL2.dll not found at '$sdlPath'" }
if (-not (Test-Path $assetsPath)) { throw "assets directory not found at '$assetsPath'" }

$outputRoot = Join-Path $repoRoot $OutputDir
$packageName = "MSFR-$Version-win64"
$stagingDir = Join-Path $outputRoot $packageName
$zipPath = Join-Path $outputRoot "$packageName.zip"

if (Test-Path $stagingDir)
{
    Remove-Item -Path $stagingDir -Recurse -Force
}
New-Item -Path $stagingDir -ItemType Directory -Force | Out-Null

if (Test-Path $zipPath)
{
    Remove-Item -Path $zipPath -Force
}

Copy-Item -Path $exePath -Destination (Join-Path $stagingDir "MSFR.exe") -Force
Copy-Item -Path $sdlPath -Destination (Join-Path $stagingDir "SDL2.dll") -Force
Copy-Item -Path $assetsPath -Destination (Join-Path $stagingDir "assets") -Recurse -Force

if (Test-Path $licensePath)
{
    Copy-Item -Path $licensePath -Destination (Join-Path $stagingDir "LICENSE") -Force
}
if (Test-Path $readmePath)
{
    Copy-Item -Path $readmePath -Destination (Join-Path $stagingDir "README.md") -Force
}

Compress-Archive -Path $stagingDir -DestinationPath $zipPath -Force

Write-Host "[Package] Created: $zipPath"
