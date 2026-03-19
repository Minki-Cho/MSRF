param(
    [string]$JsonPath = "MSFR/assets/config/gameplay_balance.json",
    [string]$CfgPath = "MSFR/assets/config/gameplay_balance.cfg",
    [switch]$FromCfg,
    [switch]$List,
    [string]$Get,
    [string[]]$Set
)

$ErrorActionPreference = "Stop"

function Parse-Scalar([string]$text)
{
    $trim = $text.Trim()
    $intVal = 0
    if ([int]::TryParse($trim, [ref]$intVal))
    {
        return $intVal
    }

    $dblVal = 0.0
    if ([double]::TryParse($trim, [System.Globalization.NumberStyles]::Float, [System.Globalization.CultureInfo]::InvariantCulture, [ref]$dblVal))
    {
        return $dblVal
    }

    if ($trim -ieq "true") { return $true }
    if ($trim -ieq "false") { return $false }
    return $trim
}

function Read-CfgToHashtable([string]$path)
{
    if (-not (Test-Path $path))
    {
        throw "CFG file not found: $path"
    }

    $map = @{}
    $lines = Get-Content -Path $path
    foreach ($line in $lines)
    {
        $noComment = ($line -split '#', 2)[0].Trim()
        if ([string]::IsNullOrWhiteSpace($noComment)) { continue }
        $idx = $noComment.IndexOf('=')
        if ($idx -lt 0) { continue }

        $key = $noComment.Substring(0, $idx).Trim()
        $value = $noComment.Substring($idx + 1).Trim()
        if ([string]::IsNullOrWhiteSpace($key)) { continue }
        if ([string]::IsNullOrWhiteSpace($value)) { continue }

        $map[$key] = Parse-Scalar $value
    }

    return $map
}

function Read-JsonToHashtable([string]$path)
{
    if (-not (Test-Path $path))
    {
        return @{}
    }

    $raw = Get-Content -Path $path -Raw
    if ([string]::IsNullOrWhiteSpace($raw))
    {
        return @{}
    }

    $obj = $raw | ConvertFrom-Json
    if ($null -eq $obj) { return @{} }

    $map = @{}
    $props = $obj.PSObject.Properties
    foreach ($p in $props)
    {
        $map[$p.Name] = $p.Value
    }

    return $map
}

function Write-HashtableToJson([hashtable]$map, [string]$path)
{
    $dir = Split-Path -Parent $path
    if (-not [string]::IsNullOrWhiteSpace($dir))
    {
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
    }

    $ordered = [ordered]@{}
    foreach ($k in ($map.Keys | Sort-Object))
    {
        $ordered[$k] = $map[$k]
    }

    $json = $ordered | ConvertTo-Json -Depth 8
    Set-Content -Path $path -Value $json -Encoding UTF8
}

if ($FromCfg)
{
    $map = Read-CfgToHashtable $CfgPath
    Write-HashtableToJson $map $JsonPath
    Write-Host "[BalanceEditor] Generated JSON from CFG: $JsonPath"
    exit 0
}

$settings = Read-JsonToHashtable $JsonPath

if ($Set -and $Set.Count -gt 0)
{
    foreach ($pair in $Set)
    {
        $idx = $pair.IndexOf('=')
        if ($idx -lt 0)
        {
            throw "Invalid -Set format '$pair'. Use key=value"
        }
        $key = $pair.Substring(0, $idx).Trim()
        $val = $pair.Substring($idx + 1).Trim()
        if ([string]::IsNullOrWhiteSpace($key))
        {
            throw "Invalid -Set key in '$pair'"
        }
        $settings[$key] = Parse-Scalar $val
        Write-Host "[BalanceEditor] Set $key = $($settings[$key])"
    }

    Write-HashtableToJson $settings $JsonPath
    Write-Host "[BalanceEditor] Saved: $JsonPath"
}

if (-not [string]::IsNullOrWhiteSpace($Get))
{
    if ($settings.ContainsKey($Get))
    {
        Write-Output "$Get = $($settings[$Get])"
    }
    else
    {
        Write-Output "$Get = <missing>"
    }
}

if ($List -or ((-not $FromCfg) -and [string]::IsNullOrWhiteSpace($Get) -and (-not $Set -or $Set.Count -eq 0)))
{
    foreach ($key in ($settings.Keys | Sort-Object))
    {
        Write-Output "$key = $($settings[$key])"
    }
}
