Add-Type -AssemblyName System.Drawing
$f = (Get-ChildItem "C:\Users\weber\.gemini\antigravity-ide\brain\b4e21c72-5fe8-420c-94aa-d6ed0344ae22\.user_uploaded\*.png" | Sort-Object LastWriteTime)[-1]
$bmp = [System.Drawing.Bitmap]::FromFile($f.FullName)

# Look at x around 223 to 260
for ($x = 220; $x -lt 260; $x++) {
    $c = $bmp.GetPixel($x, 200)
    Write-Output "x=$($x): ($($c.R),$($c.G),$($c.B))"
}
$bmp.Dispose()
