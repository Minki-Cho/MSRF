param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",

    [int]$AutoExitMs = 7000
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$projectRoot = Join-Path $repoRoot "MSFR"
$traceLog = Join-Path $projectRoot "Trace.log"
$traceBackup = Join-Path $projectRoot "Trace.log.autobak"

$exePath = Join-Path $projectRoot "x64\$Configuration\MSFR.exe"
if (-not (Test-Path $exePath))
{
    throw "MSFR.exe not found for configuration '$Configuration' at '$exePath'. Build first."
}

$hadOriginalTrace = Test-Path $traceLog
if ($hadOriginalTrace)
{
    Copy-Item $traceLog $traceBackup -Force
}

try
{
    if (Test-Path $traceLog)
    {
        Remove-Item $traceLog -Force
    }

    $args = @("--auto-exit-ms=$AutoExitMs")
    $timeoutSec = [Math]::Ceiling(($AutoExitMs + 5000) / 1000.0)

    Write-Host "[Smoke] Launching $exePath $args"
    $proc = Start-Process -FilePath $exePath -ArgumentList $args -WorkingDirectory $projectRoot -PassThru

    if (-not $proc.WaitForExit($timeoutSec * 1000))
    {
        try { Stop-Process -Id $proc.Id -Force } catch {}
        throw "Smoke run timed out after $timeoutSec seconds."
    }

    if ($proc.ExitCode -ne 0)
    {
        throw "Smoke run failed. Exit code: $($proc.ExitCode)"
    }

    if (-not (Test-Path $traceLog))
    {
        throw "Trace.log not found after smoke run."
    }

    $hasInitCore = Select-String -Path $traceLog -Pattern "Engine InitCore" -Quiet
    if (-not $hasInitCore)
    {
        throw "Trace.log does not contain expected marker: Engine InitCore"
    }

    Write-Host "[Smoke] Success ($Configuration)."
}
finally
{
    if ($hadOriginalTrace -and (Test-Path $traceBackup))
    {
        Move-Item -Force $traceBackup $traceLog
    }
    else
    {
        if (Test-Path $traceBackup)
        {
            Remove-Item $traceBackup -Force
        }
        if (Test-Path $traceLog)
        {
            Remove-Item $traceLog -Force
        }
    }
}
