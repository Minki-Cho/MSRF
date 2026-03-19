Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

Add-Type -AssemblyName System.Drawing

function Clamp-Byte {
    param([double]$Value)
    if ($Value -lt 0) { return 0 }
    if ($Value -gt 255) { return 255 }
    return [int][Math]::Round($Value)
}

function Blend-Color {
    param(
        [System.Drawing.Color]$Base,
        [System.Drawing.Color]$Overlay,
        [double]$Strength
    )
    $s = [Math]::Max(0.0, [Math]::Min(1.0, $Strength))
    $inv = 1.0 - $s
    return [System.Drawing.Color]::FromArgb(
        $Base.A,
        (Clamp-Byte ($Base.R * $inv + $Overlay.R * $s)),
        (Clamp-Byte ($Base.G * $inv + $Overlay.G * $s)),
        (Clamp-Byte ($Base.B * $inv + $Overlay.B * $s))
    )
}

function Tint-NonTransparent {
    param(
        [System.Drawing.Bitmap]$Bitmap,
        [System.Drawing.Color]$Tint,
        [double]$Amount
    )
    for ($y = 0; $y -lt $Bitmap.Height; $y++) {
        for ($x = 0; $x -lt $Bitmap.Width; $x++) {
            $c = $Bitmap.GetPixel($x, $y)
            if ($c.A -eq 0) { continue }

            # Keep deep outlines dark for readability.
            if ($c.R -lt 24 -and $c.G -lt 24 -and $c.B -lt 24) { continue }

            $Bitmap.SetPixel($x, $y, (Blend-Color -Base $c -Overlay $Tint -Strength $Amount))
        }
    }
}

function Save-Bitmap {
    param(
        [System.Drawing.Bitmap]$Bitmap,
        [string]$Path
    )
    $tmp = "$Path.tmp.png"
    if (Test-Path $tmp) {
        Remove-Item -Force $tmp
    }
    $Bitmap.Save($tmp, [System.Drawing.Imaging.ImageFormat]::Png)
    Move-Item -Force $tmp $Path
}

function Create-Variant {
    param(
        [string]$SourcePath,
        [string]$OutputPath,
        [string]$Name
    )
    $bmp = [System.Drawing.Bitmap]::new($SourcePath)
    try {
        $g = [System.Drawing.Graphics]::FromImage($bmp)
        try {
            $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
            $g.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality

            switch ($Name) {
                "item_speed" {
                    Tint-NonTransparent -Bitmap $bmp -Tint ([System.Drawing.Color]::FromArgb(255, 64, 235, 136)) -Amount 0.42
                    $bolt = [System.Drawing.Point[]]@(
                        [System.Drawing.Point]::new(30, 12),
                        [System.Drawing.Point]::new(40, 12),
                        [System.Drawing.Point]::new(35, 24),
                        [System.Drawing.Point]::new(42, 24),
                        [System.Drawing.Point]::new(27, 47),
                        [System.Drawing.Point]::new(31, 33),
                        [System.Drawing.Point]::new(23, 33)
                    )
                    $g.FillPolygon([System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(235, 228, 255, 142)), $bolt)
                    $g.DrawPolygon([System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(255, 40, 88, 36), 2.0), $bolt)
                }
                "item_rapid" {
                    Tint-NonTransparent -Bitmap $bmp -Tint ([System.Drawing.Color]::FromArgb(255, 64, 160, 245)) -Amount 0.46
                    $brush = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(240, 210, 242, 255))
                    $outline = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(255, 28, 64, 104), 2.0)
                    $g.FillEllipse($brush, 18, 20, 8, 8)
                    $g.FillEllipse($brush, 28, 28, 8, 8)
                    $g.FillEllipse($brush, 38, 36, 8, 8)
                    $g.DrawEllipse($outline, 18, 20, 8, 8)
                    $g.DrawEllipse($outline, 28, 28, 8, 8)
                    $g.DrawEllipse($outline, 38, 36, 8, 8)
                }
                "item_hybrid" {
                    Tint-NonTransparent -Bitmap $bmp -Tint ([System.Drawing.Color]::FromArgb(255, 240, 135, 62)) -Amount 0.44
                    $crossPen = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(245, 255, 222, 170), 4.0)
                    $crossPen.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
                    $crossPen.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
                    $g.DrawLine($crossPen, 20, 20, 44, 44)
                    $g.DrawLine($crossPen, 44, 20, 20, 44)
                    $g.FillEllipse([System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(255, 255, 236, 194)), 27, 27, 10, 10)
                    $g.DrawEllipse([System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(255, 110, 58, 28), 2.0), 27, 27, 10, 10)
                }
            }
        }
        finally {
            $g.Dispose()
        }

        Save-Bitmap -Bitmap $bmp -Path $OutputPath
    }
    finally {
        $bmp.Dispose()
    }
}

$root = Split-Path -Parent $PSScriptRoot
$src = Join-Path $root "MSFR\\assets\\images\\items\\data_core\\data_core.png"

$variants = @("item_speed", "item_rapid", "item_hybrid")
foreach ($variant in $variants) {
    $dir = Join-Path $root ("MSFR\\assets\\images\\items\\{0}" -f $variant)
    New-Item -ItemType Directory -Force $dir | Out-Null
    $outPng = Join-Path $dir ("{0}.png" -f $variant)
    Create-Variant -SourcePath $src -OutputPath $outPng -Name $variant
    Write-Host ("Generated {0}" -f $outPng)
}

Write-Host "Item asset variants generated."
