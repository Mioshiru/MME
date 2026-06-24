\$content = Get-Content -Path source\gui.cpp -Raw\
\$content = $content.Replace('MinSize(tools_panel->GetBestSize())', 'MinSize(42, 42)')\
