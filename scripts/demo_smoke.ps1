param(
    [int]$Seconds = 5,
    [string]$ExePath = ''
)

$ErrorActionPreference = 'Stop'

if (-not $ExePath) {
    $candidate = Get-ChildItem -Path "MSFR" -Recurse -Filter "MSFR.exe" |
        Where-Object { $_.FullName -like '*x64*Release*' } |
        Select-Object -First 1

    if (-not $candidate) {
        throw 'MSFR.exe not found. Build first (e.g., scripts/build_release.ps1).'
    }

    $ExePath = $candidate.FullName
}

Write-Host "Launching demo smoke: $ExePath"
$proc = Start-Process -FilePath $ExePath -PassThru
Start-Sleep -Seconds $Seconds

if (-not $proc.HasExited) {
    Stop-Process -Id $proc.Id -Force
    Write-Host "Smoke passed: launched and kept alive for $Seconds seconds."
}
elseif ($proc.ExitCode -eq 0) {
    Write-Host 'Smoke passed: process exited with code 0.'
}
else {
    throw "Smoke failed: process exited early with code $($proc.ExitCode)."
}
