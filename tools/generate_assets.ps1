# generate_assets.ps1
# Programmatically generates high-DPI icons in their original classic colors,
# with updated doors (scaled larger, black borders), wood-frame windows, and improved bucket, eraser, and prefab designs.
# Everything is drawn on a 512x512 canvas and downscaled to ensure ultra-sharp lines.
# Finally, compiles all brushes/*.png files into source/pngfiles.cpp and source/pngfiles.h.

Add-Type -AssemblyName System.Drawing

$projectRoot = Resolve-Path "$PSScriptRoot\.."
$iconsDir = "$projectRoot\icons"
$brushesDir = "$projectRoot\brushes"
$sourceDir = "$projectRoot\source"

if (-not (Test-Path $iconsDir)) { New-Item -ItemType Directory -Path $iconsDir | Out-Null }
if (-not (Test-Path $brushesDir)) { New-Item -ItemType Directory -Path $brushesDir | Out-Null }

# Reusable standard drawing colors
$whiteColor = [System.Drawing.Color]::FromArgb(255, 255, 255, 255)
$blackColor = [System.Drawing.Color]::FromArgb(255, 0, 0, 0)
$grayColor = [System.Drawing.Color]::FromArgb(255, 128, 128, 128)
$lightGrayColor = [System.Drawing.Color]::FromArgb(255, 200, 200, 200)

# 1. Generate Brush Tilesets (Circular/Rectangular)
function Generate-Tileset ($type) {
    $bmp = New-Object System.Drawing.Bitmap(512, 512)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.Clear([System.Drawing.Color]::Transparent)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    
    $c1 = [System.Drawing.Color]::FromArgb(255, 60, 120, 220)
    $c2 = [System.Drawing.Color]::FromArgb(255, 20, 50, 130)
    $borderColor = [System.Drawing.Color]::FromArgb(255, 10, 30, 80)
    
    for ($i = 0; $i -lt 7; $i++) {
        $row = [Math]::Floor($i / 4)
        $col = $i % 4
        $cellX = $col * 128
        $cellY = $row * 128
        
        $size = 16 + ($i * 16)
        $offsetX = (128 - $size) / 2
        $offsetY = (128 - $size) / 2
        
        $x = $cellX + $offsetX
        $y = $cellY + $offsetY
        
        $rect = New-Object System.Drawing.RectangleF($x, $y, $size, $size)
        $brush = New-Object System.Drawing.Drawing2D.LinearGradientBrush($rect, $c1, $c2, 90.0)
        $penWidth = [Math]::Max(4.0, $size * 0.05)
        $pen = New-Object System.Drawing.Pen($borderColor, $penWidth)
        
        $rect.X += $penWidth / 2
        $rect.Y += $penWidth / 2
        $rect.Width -= $penWidth
        $rect.Height -= $penWidth
        
        if ($type -eq "circular") {
            $g.FillEllipse($brush, $rect)
            $g.DrawEllipse($pen, $rect)
        } else {
            $g.FillRectangle($brush, $rect)
            $g.DrawRectangle($pen, $rect.X, $rect.Y, $rect.Width, $rect.Height)
        }
        
        $pen.Dispose()
        $brush.Dispose()
    }
    
    $g.Dispose()
    $outputPath = "$iconsDir\$($type)_tileset.png"
    $bmp.Save($outputPath, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    Write-Host "Generated Tileset: $outputPath"
}

Generate-Tileset "circular"
Generate-Tileset "rectangular"

# 2. Slice Tilesets
function Slice-Tileset ($type) {
    $tilesetPath = "$iconsDir\$($type)_tileset.png"
    $tileset = New-Object System.Drawing.Bitmap($tilesetPath)
    
    for ($i = 0; $i -lt 7; $i++) {
        $row = [Math]::Floor($i / 4)
        $col = $i % 4
        $cellX = $col * 128
        $cellY = $row * 128
        
        $rect = New-Object System.Drawing.Rectangle($cellX, $cellY, 128, 128)
        $cellBmp = $tileset.Clone($rect, $tileset.PixelFormat)
        
        # 32x32
        $bmp32 = New-Object System.Drawing.Bitmap(32, 32)
        $g32 = [System.Drawing.Graphics]::FromImage($bmp32)
        $g32.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
        $g32.DrawImage($cellBmp, 0, 0, 32, 32)
        $g32.Dispose()
        
        $path32Brushes = "$brushesDir\$($type)_$($i + 1).png"
        $path32Icons = "$iconsDir\$($type)_$($i + 1).png"
        $bmp32.Save($path32Brushes, [System.Drawing.Imaging.ImageFormat]::Png)
        $bmp32.Save($path32Icons, [System.Drawing.Imaging.ImageFormat]::Png)
        $bmp32.Dispose()
        
        # 16x16
        $bmp16 = New-Object System.Drawing.Bitmap(16, 16)
        $g16 = [System.Drawing.Graphics]::FromImage($bmp16)
        $g16.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
        $g16.DrawImage($cellBmp, 0, 0, 16, 16)
        $g16.Dispose()
        
        $path16Brushes = "$brushesDir\$($type)_$($i + 1)_small.png"
        $bmp16.Save($path16Brushes, [System.Drawing.Imaging.ImageFormat]::Png)
        $bmp16.Dispose()
        
        $cellBmp.Dispose()
    }
    $tileset.Dispose()
    Write-Host "Sliced $type icons to brushes/ and icons/"
}

Slice-Tileset "circular"
Slice-Tileset "rectangular"

# 3. Helper to Generate and Save Utility/Special Icons (with 512x512 downsampling)
function Generate-Special-Icon ($name, $drawAction) {
    # Render at 512x512 internally for maximum antialiasing and sharpness
    $bmp512 = New-Object System.Drawing.Bitmap(512, 512)
    $g512 = [System.Drawing.Graphics]::FromImage($bmp512)
    $g512.Clear([System.Drawing.Color]::Transparent)
    $g512.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    
    # Run the draw action on the 512x512 canvas
    $drawAction.Invoke($g512, 512)
    $g512.Dispose()
    
    # Downscale to 32x32 (standard size)
    $bmp32 = New-Object System.Drawing.Bitmap(32, 32)
    $g32 = [System.Drawing.Graphics]::FromImage($bmp32)
    $g32.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $g32.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g32.DrawImage($bmp512, 0, 0, 32, 32)
    $g32.Dispose()
    
    $standardPath = "$brushesDir\$name.png"
    $bmp32.Save($standardPath, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp32.Dispose()
    
    # Downscale to 16x16 (small size)
    $bmp16 = New-Object System.Drawing.Bitmap(16, 16)
    $g16 = [System.Drawing.Graphics]::FromImage($bmp16)
    $g16.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $g16.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g16.DrawImage($bmp512, 0, 0, 16, 16)
    $g16.Dispose()
    
    $smallPath = "$brushesDir\$($name)_small.png"
    $bmp16.Save($smallPath, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp16.Dispose()
    
    $bmp512.Dispose()
    Write-Host "Generated HD Special Icon: $name"
}

# --- Drawing Routines with Dynamic Scale Support ---

# Window Normal (Wood Frame)
$drawWindowNormal = {
    param($g, $w)
    $s = $w / 32.0
    
    $woodColor = [System.Drawing.Color]::FromArgb(255, 120, 70, 30)
    $woodPen = New-Object System.Drawing.Pen($woodColor, ($s * 3.0))
    $glassBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(60, 150, 200, 240))
    $whitePen = New-Object System.Drawing.Pen($whiteColor, ($s * 1.0))
    
    $g.FillRectangle($glassBrush, ($s * 3.0), ($s * 3.0), ($s * 26.0), ($s * 26.0))
    $g.DrawRectangle($woodPen, ($s * 3.0), ($s * 3.0), ($s * 26.0), ($s * 26.0))
    $g.DrawLine($whitePen, ($s * 16.0), ($s * 3.0), ($s * 16.0), ($s * 29.0))
    $g.DrawLine($whitePen, ($s * 3.0), ($s * 16.0), ($s * 29.0), ($s * 16.0))
    
    $woodPen.Dispose(); $glassBrush.Dispose(); $whitePen.Dispose()
}

# Window Hatch (Wood Frame with diagonal lines)
$drawWindowHatch = {
    param($g, $w)
    $s = $w / 32.0
    
    $woodColor = [System.Drawing.Color]::FromArgb(255, 120, 70, 30)
    $woodPen = New-Object System.Drawing.Pen($woodColor, ($s * 3.0))
    $glassBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(60, 150, 200, 240))
    $hatchPen = New-Object System.Drawing.Pen($woodColor, ($s * 1.5))
    
    $g.FillRectangle($glassBrush, ($s * 3.0), ($s * 3.0), ($s * 26.0), ($s * 26.0))
    $g.DrawRectangle($woodPen, ($s * 3.0), ($s * 3.0), ($s * 26.0), ($s * 26.0))
    for ($offset = 6; $offset -le 24; $offset += 6) {
        $g.DrawLine($hatchPen, (($offset + 3) * $s), ($s * 3.0), ($s * 3.0), (($offset + 3) * $s))
        $g.DrawLine($hatchPen, ($s * 29.0), (($offset + 3) * $s), (($offset + 3) * $s), ($s * 29.0))
    }
    
    $woodPen.Dispose(); $glassBrush.Dispose(); $hatchPen.Dispose()
}

# Protection Zone (Green Shield)
$drawProtectionZone = {
    param($g, $w)
    $s = $w / 32.0
    
    $greenC1 = [System.Drawing.Color]::FromArgb(255, 46, 204, 113)
    $greenC2 = [System.Drawing.Color]::FromArgb(255, 39, 174, 96)
    $darkGreen = [System.Drawing.Color]::FromArgb(255, 27, 94, 32)
    
    $rect = New-Object System.Drawing.RectangleF(($s * 4.0), ($s * 3.0), ($s * 24.0), ($s * 26.0))
    $brush = New-Object System.Drawing.Drawing2D.LinearGradientBrush($rect, $greenC1, $greenC2, 90.0)
    $pen = New-Object System.Drawing.Pen($darkGreen, ($s * 2.0))
    
    $points = @(
        (New-Object System.Drawing.PointF(($s * 16.0), ($s * 3.0))),
        (New-Object System.Drawing.PointF(($s * 28.0), ($s * 6.0))),
        (New-Object System.Drawing.PointF(($s * 25.0), ($s * 20.0))),
        (New-Object System.Drawing.PointF(($s * 16.0), ($s * 29.0))),
        (New-Object System.Drawing.PointF(($s * 7.0), ($s * 20.0))),
        (New-Object System.Drawing.PointF(($s * 4.0), ($s * 6.0)))
    )
    $g.FillPolygon($brush, $points)
    $g.DrawPolygon($pen, $points)
    
    $brush.Dispose(); $pen.Dispose()
}

# No PvP Zone (Yellow Flag)
$drawNoPvp = {
    param($g, $w)
    $s = $w / 32.0
    
    $yellowC1 = [System.Drawing.Color]::FromArgb(255, 241, 196, 15)
    $yellowC2 = [System.Drawing.Color]::FromArgb(255, 243, 156, 18)
    $darkGold = [System.Drawing.Color]::FromArgb(255, 120, 90, 10)
    
    $rect = New-Object System.Drawing.RectangleF(($s * 7.0), ($s * 6.0), ($s * 18.0), ($s * 12.0))
    $brush = New-Object System.Drawing.Drawing2D.LinearGradientBrush($rect, $yellowC1, $yellowC2, 90.0)
    $pen = New-Object System.Drawing.Pen($darkGold, ($s * 1.5))
    $polePen = New-Object System.Drawing.Pen($grayColor, ($s * 2.0))
    
    $g.DrawLine($polePen, ($s * 6.0), ($s * 4.0), ($s * 6.0), ($s * 28.0))
    $g.FillRectangle($brush, $rect)
    $g.DrawRectangle($pen, ($s * 7.0), ($s * 6.0), ($s * 18.0), ($s * 12.0))
    
    $brush.Dispose(); $pen.Dispose(); $polePen.Dispose()
}

# PvP Zone (Red Flag)
$drawPvp = {
    param($g, $w)
    $s = $w / 32.0
    
    $redC1 = [System.Drawing.Color]::FromArgb(255, 231, 76, 60)
    $redC2 = [System.Drawing.Color]::FromArgb(255, 192, 57, 43)
    $darkRed = [System.Drawing.Color]::FromArgb(255, 120, 20, 20)
    
    $rect = New-Object System.Drawing.RectangleF(($s * 7.0), ($s * 6.0), ($s * 19.0), ($s * 12.0))
    $brush = New-Object System.Drawing.Drawing2D.LinearGradientBrush($rect, $redC1, $redC2, 90.0)
    $pen = New-Object System.Drawing.Pen($darkRed, ($s * 1.5))
    $polePen = New-Object System.Drawing.Pen($grayColor, ($s * 2.0))
    
    $g.DrawLine($polePen, ($s * 6.0), ($s * 4.0), ($s * 6.0), ($s * 28.0))
    $points = @(
        (New-Object System.Drawing.PointF(($s * 7.0), ($s * 6.0))),
        (New-Object System.Drawing.PointF(($s * 26.0), ($s * 12.0))),
        (New-Object System.Drawing.PointF(($s * 7.0), ($s * 18.0)))
    )
    $g.FillPolygon($brush, $points)
    $g.DrawPolygon($pen, $points)
    
    $brush.Dispose(); $pen.Dispose(); $polePen.Dispose()
}

# No Logout Zone (Gold Lock with red slash)
$drawNoLogout = {
    param($g, $w)
    $s = $w / 32.0
    
    $goldC1 = [System.Drawing.Color]::FromArgb(255, 241, 196, 15)
    $goldC2 = [System.Drawing.Color]::FromArgb(255, 212, 175, 55)
    $darkGold = [System.Drawing.Color]::FromArgb(255, 120, 90, 10)
    
    $rect = New-Object System.Drawing.RectangleF(($s * 6.0), ($s * 14.0), ($s * 20.0), ($s * 14.0))
    $brush = New-Object System.Drawing.Drawing2D.LinearGradientBrush($rect, $goldC1, $goldC2, 90.0)
    $pen = New-Object System.Drawing.Pen($darkGold, ($s * 2.0))
    $shacklePen = New-Object System.Drawing.Pen($grayColor, ($s * 3.0))
    $slashPen = New-Object System.Drawing.Pen([System.Drawing.Color]::Red, ($s * 3.0))
    
    $g.DrawArc($shacklePen, ($s * 10.0), ($s * 6.0), ($s * 12.0), ($s * 16.0), 180, 180)
    $g.FillRectangle($brush, $rect)
    $g.DrawRectangle($pen, ($s * 6.0), ($s * 14.0), ($s * 20.0), ($s * 14.0))
    $g.DrawLine($slashPen, ($s * 4.0), ($s * 4.0), ($s * 28.0), ($s * 28.0))
    
    $brush.Dispose(); $pen.Dispose(); $shacklePen.Dispose(); $slashPen.Dispose()
}

# Eraser (Tilted 3D-ish block with white sleeve)
$drawEraser = {
    param($g, $w)
    $s = $w / 32.0
    
    $g.TranslateTransform(($s * 16.0), ($s * 16.0))
    $g.RotateTransform(-20)
    
    $pinkBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 240, 98, 146))
    $blueBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 52, 152, 219))
    $whiteBrush = New-Object System.Drawing.SolidBrush($whiteColor)
    $sleeveBand = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 30, 80, 180))
    $borderPen = New-Object System.Drawing.Pen($blackColor, ($s * 1.5))
    
    # Fill parts
    $g.FillRectangle($blueBrush, (-12.0 * $s), (-6.0 * $s), (8.0 * $s), (12.0 * $s))
    $g.FillRectangle($whiteBrush, (-4.0 * $s), (-6.0 * $s), (8.0 * $s), (12.0 * $s))
    $g.FillRectangle($sleeveBand, (-2.0 * $s), (-6.0 * $s), (4.0 * $s), (12.0 * $s))
    $g.FillRectangle($pinkBrush, (4.0 * $s), (-6.0 * $s), (8.0 * $s), (12.0 * $s))
    
    # Borders
    $g.DrawRectangle($borderPen, (-12.0 * $s), (-6.0 * $s), (24.0 * $s), (12.0 * $s))
    $g.DrawLine($borderPen, (-4.0 * $s), (-6.0 * $s), (-4.0 * $s), (6.0 * $s))
    $g.DrawLine($borderPen, (4.0 * $s), (-6.0 * $s), (4.0 * $s), (6.0 * $s))
    
    $g.ResetTransform()
    
    $pinkBrush.Dispose(); $blueBrush.Dispose(); $whiteBrush.Dispose(); $sleeveBand.Dispose(); $borderPen.Dispose()
}

# Border (Gray dashed outline)
$drawBorder = {
    param($g, $w)
    $s = $w / 32.0
    $pen = New-Object System.Drawing.Pen($grayColor, ($s * 2.0))
    $pen.DashStyle = [System.Drawing.Drawing2D.DashStyle]::Dash
    $g.DrawRectangle($pen, ($s * 4.0), ($s * 4.0), ($s * 24.0), ($s * 24.0))
    $pen.Dispose()
}

# Bucket (Silver bucket spilling blue paint)
$drawBucket = {
    param($g, $w)
    $s = $w / 32.0
    
    $bucketPen = New-Object System.Drawing.Pen($blackColor, ($s * 1.5))
    $metalBrush = New-Object System.Drawing.Drawing2D.LinearGradientBrush((New-Object System.Drawing.RectangleF(($s * 8.0), ($s * 10.0), ($s * 16.0), ($s * 16.0))), $lightGrayColor, $grayColor, 45.0)
    $paintBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 60, 120, 220))
    $handlePen = New-Object System.Drawing.Pen($blackColor, ($s * 1.2))
    
    $g.DrawArc($handlePen, ($s * 6.0), ($s * 2.0), ($s * 20.0), ($s * 20.0), 180, 180)
    
    $path = New-Object System.Drawing.Drawing2D.GraphicsPath
    [void]$path.AddLine(($s * 8.0), ($s * 12.0), ($s * 11.0), ($s * 25.0))
    [void]$path.AddArc(($s * 11.0), ($s * 23.0), ($s * 10.0), ($s * 4.0), 0, 180)
    [void]$path.AddLine(($s * 21.0), ($s * 25.0), ($s * 24.0), ($s * 12.0))
    [void]$path.AddArc(($s * 8.0), ($s * 10.0), ($s * 16.0), ($s * 4.0), 0, 180)
    [void]$path.CloseFigure()
    $g.FillPath($metalBrush, $path)
    $g.DrawPath($bucketPen, $path)
    
    $g.FillEllipse($paintBrush, ($s * 8.0), ($s * 10.0), ($s * 16.0), ($s * 4.0))
    $g.DrawEllipse($bucketPen, ($s * 8.0), ($s * 10.0), ($s * 16.0), ($s * 4.0))
    
    $g.FillEllipse($paintBrush, ($s * 22.0), ($s * 14.0), ($s * 5.0), ($s * 8.0))
    $g.FillEllipse($paintBrush, ($s * 26.0), ($s * 20.0), ($s * 3.0), ($s * 5.0))
    
    $path.Dispose(); $bucketPen.Dispose(); $metalBrush.Dispose(); $paintBrush.Dispose(); $handlePen.Dispose()
}

# Pointer (Classic Arrow)
$drawPointer = {
    param($g, $w)
    $s = $w / 32.0
    
    $brush = New-Object System.Drawing.SolidBrush($whiteColor)
    $pen = New-Object System.Drawing.Pen($blackColor, ($s * 2.0))
    
    $points = @(
        (New-Object System.Drawing.PointF(($s * 8.0), ($s * 4.0))),
        (New-Object System.Drawing.PointF(($s * 22.0), ($s * 18.0))),
        (New-Object System.Drawing.PointF(($s * 16.0), ($s * 18.0))),
        (New-Object System.Drawing.PointF(($s * 20.0), ($s * 26.0))),
        (New-Object System.Drawing.PointF(($s * 17.0), ($s * 27.0))),
        (New-Object System.Drawing.PointF(($s * 13.0), ($s * 19.0))),
        (New-Object System.Drawing.PointF(($s * 8.0), ($s * 24.0)))
    )
    $g.FillPolygon($brush, $points)
    $g.DrawPolygon($pen, $points)
    
    $brush.Dispose(); $pen.Dispose()
}

# Prefab (Brick wall layout pattern)
$drawPrefab = {
    param($g, $w)
    $s = $w / 32.0
    
    $mortarBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 100, 100, 100))
    $g.FillRectangle($mortarBrush, ($s * 2.0), ($s * 4.0), ($s * 28.0), ($s * 24.0))
    
    $brickBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 211, 84, 0))
    $brickBorder = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(255, 80, 20, 0), ($s * 1.2))
    
    # Row 1
    $g.FillRectangle($brickBrush, ($s * 2.0), ($s * 4.0), ($s * 13.0), ($s * 7.0))
    $g.DrawRectangle($brickBorder, ($s * 2.0), ($s * 4.0), ($s * 13.0), ($s * 7.0))
    $g.FillRectangle($brickBrush, ($s * 17.0), ($s * 4.0), ($s * 13.0), ($s * 7.0))
    $g.DrawRectangle($brickBorder, ($s * 17.0), ($s * 4.0), ($s * 13.0), ($s * 7.0))
    
    # Row 2
    $g.FillRectangle($brickBrush, ($s * 2.0), ($s * 12.0), ($s * 5.0), ($s * 7.0))
    $g.DrawRectangle($brickBorder, ($s * 2.0), ($s * 12.0), ($s * 5.0), ($s * 7.0))
    $g.FillRectangle($brickBrush, ($s * 9.0), ($s * 12.0), ($s * 13.0), ($s * 7.0))
    $g.DrawRectangle($brickBorder, ($s * 9.0), ($s * 12.0), ($s * 13.0), ($s * 7.0))
    $g.FillRectangle($brickBrush, ($s * 24.0), ($s * 12.0), ($s * 6.0), ($s * 7.0))
    $g.DrawRectangle($brickBorder, ($s * 24.0), ($s * 12.0), ($s * 6.0), ($s * 7.0))
    
    # Row 3
    $g.FillRectangle($brickBrush, ($s * 2.0), ($s * 20.0), ($s * 13.0), ($s * 7.0))
    $g.DrawRectangle($brickBorder, ($s * 2.0), ($s * 20.0), ($s * 13.0), ($s * 7.0))
    $g.FillRectangle($brickBrush, ($s * 17.0), ($s * 20.0), ($s * 13.0), ($s * 7.0))
    $g.DrawRectangle($brickBorder, ($s * 17.0), ($s * 20.0), ($s * 13.0), ($s * 7.0))
    
    $mortarBrush.Dispose(); $brickBrush.Dispose(); $brickBorder.Dispose()
}

# Position Go (Red target bullseye)
$drawPositionGo = {
    param($g, $w)
    $s = $w / 32.0
    
    $redBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::Red)
    $whiteBrush = New-Object System.Drawing.SolidBrush($whiteColor)
    $pen = New-Object System.Drawing.Pen([System.Drawing.Color]::Red, ($s * 2.0))
    
    $g.FillEllipse($redBrush, ($s * 4.0), ($s * 4.0), ($s * 24.0), ($s * 24.0))
    $g.FillEllipse($whiteBrush, ($s * 8.0), ($s * 8.0), ($s * 16.0), ($s * 16.0))
    $g.FillEllipse($redBrush, ($s * 12.0), ($s * 12.0), ($s * 8.0), ($s * 8.0))
    
    $g.DrawLine($pen, ($s * 16.0), ($s * 1.0), ($s * 16.0), ($s * 31.0))
    $g.DrawLine($pen, ($s * 1.0), ($s * 16.0), ($s * 31.0), ($s * 16.0))
    
    $redBrush.Dispose(); $whiteBrush.Dispose(); $pen.Dispose()
}

# --- Doors (Scaled Up with high-resolution SOLID BLACK borders) ---

# Base Door Frame
function Draw-Door-Base ($g, $w) {
    $s = $w / 32.0
    $pen = New-Object System.Drawing.Pen($blackColor, ($s * 3.0))
    $g.DrawRectangle($pen, ($s * 2.0), ($s * 1.0), ($s * 28.0), ($s * 30.0))
    $pen.Dispose()
}

# Door Normal
$drawDoorNormal = {
    param($g, $w)
    $s = $w / 32.0
    $woodC1 = [System.Drawing.Color]::FromArgb(255, 160, 82, 45)
    $woodC2 = [System.Drawing.Color]::FromArgb(255, 139, 69, 19)
    $rect = New-Object System.Drawing.RectangleF(($s * 4.0), ($s * 2.0), ($s * 24.0), ($s * 28.0))
    $brush = New-Object System.Drawing.Drawing2D.LinearGradientBrush($rect, $woodC1, $woodC2, 90.0)
    
    $g.FillRectangle($brush, $rect)
    Draw-Door-Base $g $w
    
    $knobBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::Gold)
    $g.FillEllipse($knobBrush, ($s * 7.0), ($s * 16.0), ($s * 3.5), ($s * 3.5))
    $brush.Dispose(); $knobBrush.Dispose()
}

# Door Normal Alt
$drawDoorNormalAlt = {
    param($g, $w)
    $s = $w / 32.0
    $woodC1 = [System.Drawing.Color]::FromArgb(255, 160, 82, 45)
    $woodC2 = [System.Drawing.Color]::FromArgb(255, 139, 69, 19)
    $rect = New-Object System.Drawing.RectangleF(($s * 4.0), ($s * 2.0), ($s * 24.0), ($s * 28.0))
    $brush = New-Object System.Drawing.Drawing2D.LinearGradientBrush($rect, $woodC1, $woodC2, 90.0)
    $pen = New-Object System.Drawing.Pen($blackColor, ($s * 1.5))
    
    $g.FillRectangle($brush, $rect)
    $g.DrawLine($pen, ($s * 16.0), ($s * 2.0), ($s * 16.0), ($s * 29.0))
    Draw-Door-Base $g $w
    
    $brush.Dispose(); $pen.Dispose()
}

# Door Locked
$drawDoorLocked = {
    param($g, $w)
    $s = $w / 32.0
    $ironC1 = [System.Drawing.Color]::FromArgb(255, 100, 100, 100)
    $ironC2 = [System.Drawing.Color]::FromArgb(255, 60, 60, 60)
    $rect = New-Object System.Drawing.RectangleF(($s * 4.0), ($s * 2.0), ($s * 24.0), ($s * 28.0))
    $brush = New-Object System.Drawing.Drawing2D.LinearGradientBrush($rect, $ironC1, $ironC2, 90.0)
    
    $g.FillRectangle($brush, $rect)
    Draw-Door-Base $g $w
    
    $lockBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::Gold)
    $g.FillRectangle($lockBrush, ($s * 13.0), ($s * 14.0), ($s * 6.0), ($s * 6.0))
    $lockPen = New-Object System.Drawing.Pen([System.Drawing.Color]::Gold, ($s * 1.5))
    $g.DrawArc($lockPen, ($s * 14.0), ($s * 10.0), ($s * 4.0), ($s * 6.0), 180, 180)
    
    $lockBrush.Dispose(); $lockPen.Dispose(); $brush.Dispose()
}

# Door Magic
$drawDoorMagic = {
    param($g, $w)
    $s = $w / 32.0
    $magicC1 = [System.Drawing.Color]::FromArgb(255, 0, 255, 255)
    $magicC2 = [System.Drawing.Color]::FromArgb(255, 0, 128, 255)
    $rect = New-Object System.Drawing.RectangleF(($s * 4.0), ($s * 2.0), ($s * 24.0), ($s * 28.0))
    $brush = New-Object System.Drawing.Drawing2D.LinearGradientBrush($rect, $magicC1, $magicC2, 90.0)
    
    $g.FillRectangle($brush, $rect)
    Draw-Door-Base $g $w
    
    $sparkleBrush = New-Object System.Drawing.SolidBrush($whiteColor)
    $points = @(
        (New-Object System.Drawing.PointF(($s * 16.0), ($s * 11.0))),
        (New-Object System.Drawing.PointF(($s * 18.0), ($s * 16.0))),
        (New-Object System.Drawing.PointF(($s * 23.0), ($s * 16.0))),
        (New-Object System.Drawing.PointF(($s * 19.0), ($s * 19.0))),
        (New-Object System.Drawing.PointF(($s * 21.0), ($s * 24.0))),
        (New-Object System.Drawing.PointF(($s * 16.0), ($s * 21.0))),
        (New-Object System.Drawing.PointF(($s * 11.0), ($s * 24.0))),
        (New-Object System.Drawing.PointF(($s * 13.0), ($s * 19.0))),
        (New-Object System.Drawing.PointF(($s * 9.0), ($s * 16.0))),
        (New-Object System.Drawing.PointF(($s * 14.0), ($s * 16.0)))
    )
    $g.FillPolygon($sparkleBrush, $points)
    $sparkleBrush.Dispose(); $brush.Dispose()
}

# Door Quest
$drawDoorQuest = {
    param($g, $w)
    $s = $w / 32.0
    $goldC1 = [System.Drawing.Color]::FromArgb(255, 241, 196, 15)
    $goldC2 = [System.Drawing.Color]::FromArgb(255, 212, 175, 55)
    $rect = New-Object System.Drawing.RectangleF(($s * 4.0), ($s * 2.0), ($s * 24.0), ($s * 28.0))
    $brush = New-Object System.Drawing.Drawing2D.LinearGradientBrush($rect, $goldC1, $goldC2, 90.0)
    
    $g.FillRectangle($brush, $rect)
    Draw-Door-Base $g $w
    
    $markBrush = New-Object System.Drawing.SolidBrush($blackColor)
    $g.FillRectangle($markBrush, ($s * 15.0), ($s * 10.0), ($s * 2.0), ($s * 7.0))
    $g.FillEllipse($markBrush, ($s * 15.0), ($s * 19.0), ($s * 2.5), ($s * 2.5))
    $markBrush.Dispose(); $brush.Dispose()
}

# Door Archway
$drawDoorArchway = {
    param($g, $w)
    $s = $w / 32.0
    $rect = New-Object System.Drawing.RectangleF(($s * 2.0), ($s * 2.0), ($s * 28.0), ($s * 29.0))
    $brush = New-Object System.Drawing.Drawing2D.LinearGradientBrush($rect, $lightGrayColor, $grayColor, 90.0)
    $pen = New-Object System.Drawing.Pen($blackColor, ($s * 3.0))
    
    $path = New-Object System.Drawing.Drawing2D.GraphicsPath
    [void]$path.AddLine(($s * 2.0), ($s * 31.0), ($s * 2.0), ($s * 14.0))
    [void]$path.AddArc(($s * 2.0), ($s * 2.0), ($s * 28.0), ($s * 24.0), 180, 180)
    [void]$path.AddLine(($s * 30.0), ($s * 14.0), ($s * 30.0), ($s * 31.0))
    [void]$path.CloseFigure()
    
    $g.FillPath($brush, $path)
    $g.DrawPath($pen, $path)
    
    $path.Dispose(); $brush.Dispose(); $pen.Dispose()
}

# --- Generate Specials ---
Generate-Special-Icon "window_normal" $drawWindowNormal
Generate-Special-Icon "window_hatch" $drawWindowHatch
Generate-Special-Icon "protection_zone" $drawProtectionZone
Generate-Special-Icon "no_pvp" $drawNoPvp
Generate-Special-Icon "pvp_zone" $drawPvp
Generate-Special-Icon "no_logout" $drawNoLogout
Generate-Special-Icon "eraser" $drawEraser
Generate-Special-Icon "optional_border" $drawBorder

Generate-Special-Icon "bucket" $drawBucket
Generate-Special-Icon "pointer" $drawPointer
Generate-Special-Icon "prefab" $drawPrefab
Generate-Special-Icon "position_go" $drawPositionGo

Generate-Special-Icon "door_normal" $drawDoorNormal
Generate-Special-Icon "door_normal_alt" $drawDoorNormalAlt
Generate-Special-Icon "door_locked" $drawDoorLocked
Generate-Special-Icon "door_magic" $drawDoorMagic
Generate-Special-Icon "door_quest" $drawDoorQuest
Generate-Special-Icon "door_archway" $drawDoorArchway

# Copy to icons directory for C# / local assets
Copy-Item "$brushesDir\protection_zone.png" -Destination "$iconsDir\protected_zone.png" -Force
Copy-Item "$brushesDir\no_pvp.png" -Destination "$iconsDir\nopvp_zone.png" -Force
Copy-Item "$brushesDir\no_logout.png" -Destination "$iconsDir\nologout_zone.png" -Force
Copy-Item "$brushesDir\pvp_zone.png" -Destination "$iconsDir\pvp_zone.png" -Force
Copy-Item "$brushesDir\bucket.png" -Destination "$iconsDir\bucket.png" -Force
Copy-Item "$brushesDir\pointer.png" -Destination "$iconsDir\pointer.png" -Force
Copy-Item "$brushesDir\prefab.png" -Destination "$iconsDir\prefab.png" -Force
Copy-Item "$brushesDir\position_go.png" -Destination "$iconsDir\position_go.png" -Force

# Convert all png files in brushes/ to source/pngfiles.h and source/pngfiles.cpp
Write-Host "Converting brushes/*.png files to C++..."
$headerPath = "$sourceDir\pngfiles.h"
$sourcePath = "$sourceDir\pngfiles.cpp"

$headerContent = @"
#ifndef __PNG_HEADER_FILE_H__
#define __PNG_HEADER_FILE_H__

"@

$sourceContent = ""

$pngFiles = Get-ChildItem -Path $brushesDir -Filter "*.png"

foreach ($file in $pngFiles) {
    $varName = $file.BaseName -replace '\.', '_' -replace '-', '_'
    $bytes = [System.IO.File]::ReadAllBytes($file.FullName)
    $count = $bytes.Length
    
    $headerContent += "extern unsigned char $($varName)_png[$count];`n"
    
    $cppText = "/* $($file.Name) - $count bytes */`nunsigned char $($varName)_png[$count] = {`n"
    $sb = New-Object System.Text.StringBuilder
    [void]$sb.Append($cppText)
    
    for ($i = 0; $i -lt $count; $i++) {
        if (($i % 8) -eq 0) {
            [void]$sb.Append("  ")
        }
        [void]$sb.Append([string]::Format("0x{0:x2}", $bytes[$i]))
        if (($i + 1) -lt $count) {
            [void]$sb.Append(", ")
        }
        if (($i % 8) -eq 7) {
            [void]$sb.Append("`n")
        }
    }
    
    if (($count % 8) -ne 0) {
        [void]$sb.Append("`n")
    }
    
    [void]$sb.Append("};`n/* End Of File */`n`n")
    $sourceContent += $sb.ToString()
}

$headerContent += "`n#endif //__PNG_HEADER_FILE_H__`n"

[System.IO.File]::WriteAllText($headerPath, $headerContent)
[System.IO.File]::WriteAllText($sourcePath, $sourceContent)

Write-Host "Re-generated: $headerPath"
Write-Host "Re-generated: $sourcePath"
Write-Host "Asset generation and conversion complete!"
