param(
    [string]$Configuration = 'Release',
    [string]$Platform = 'x64'
)

$ErrorActionPreference = 'Stop'

Write-Host "Building solution (Configuration=$Configuration, Platform=$Platform)"
msbuild "MSFR/MSFR.sln" /m /p:Configuration=$Configuration /p:Platform=$Platform
