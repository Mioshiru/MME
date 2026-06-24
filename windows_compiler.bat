@echo off
REM ============================================================
REM  Mios Map Editor - Professional Windows Build Script
REM ============================================================

setlocal enabledelayedexpansion

REM --- Deep Cleanup Environment to prevent vcpkg/git mismatches ---
set GIT_DIR=
set GIT_WORK_TREE=
set "X_VCPKG_REGISTRIES_CACHE="
set "VCPKG_DEFAULT_BINARY_CACHE="
set "GIT_OPTIONAL_LOCKS=0"

REM --- Color helpers ---
for /F %%a in ('echo prompt $E ^| cmd') do set "ESC=%%a"
set "GREEN=%ESC%[92m"
set "YELLOW=%ESC%[93m"
set "RED=%ESC%[91m"
set "CYAN=%ESC%[96m"
set "RESET=%ESC%[0m"
set "BOLD=%ESC%[1m"

set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=!SCRIPT_DIR:~0,-1!"
set "BUILD_DIR=!PROJECT_ROOT!\build"
set "ERROR_FILE=!PROJECT_ROOT!\compiler_error_latest.md"
set "INSTALL_DIR=C:\Users\weber\Dokumente\Projekt\In_Arbeit\Mios Map Editor"
set "LOG_FILE=%TEMP%\mme_build.log"
set "TOTAL_STEPS=5"

REM --- Check if log file is accessible ---
type nul > "!LOG_FILE!" 2>nul || (
    echo %RED%ERROR: !LOG_FILE! is locked by another process ^(e.g. your text editor^).%RESET%
    echo Please close any programs viewing the log and run this script again.
    pause & exit /b 1
)
echo Initializing Build Log>"!LOG_FILE!"
if exist "!ERROR_FILE!" del /f /q "!ERROR_FILE!"
if exist "!PROJECT_ROOT!\compiler_error.md" del /f /q "!PROJECT_ROOT!\compiler_error.md"

echo.
echo %BOLD%%CYAN%========================================================%RESET%
echo %BOLD%%CYAN%  Mios Map Editor Build Script%RESET%
echo %BOLD%%CYAN%========================================================%RESET%
echo.
echo %YELLOW%Hinweis:%RESET% Windows drosselt Hintergrundprozesse im Sperrbildschirm.
echo Halte das Notebook aktiv, um die volle Kompilier-Geschwindigkeit zu nutzen.
echo.

:start_build
REM --- STEP 1: Prep & Environment ---
set /a "STEP=1"
echo %BOLD%[1/%TOTAL_STEPS%] Cleaning ^& Setting up environment...%RESET%
if exist "!BUILD_DIR!" rd /s /q "!BUILD_DIR!" >> "!LOG_FILE!" 2>nul
if exist "!INSTALL_DIR!" rd /s /q "!INSTALL_DIR!" >> "!LOG_FILE!" 2>nul

where git >nul 2>&1 || (echo   %RED%ERROR: Git not found.%RESET% & pause & exit /b 1)

where cmake >nul 2>&1 || (
    echo   %YELLOW%CMake not found. Downloading portable version...%RESET%
    set "CMAKE_DIR=!PROJECT_ROOT!\tools\cmake"
    if not exist "!CMAKE_DIR!" (
        mkdir "!PROJECT_ROOT!\tools" >nul 2>&1
        curl -L "https://github.com/Kitware/CMake/releases/download/v3.28.1/cmake-3.28.1-windows-x86_64.zip" -o "!PROJECT_ROOT!\cmake_portable.zip" 2>nul
        powershell -command "Expand-Archive -Force '!PROJECT_ROOT!\cmake_portable.zip' '!PROJECT_ROOT!\tools'"
        move "!PROJECT_ROOT!\tools\cmake-3.28.1-windows-x86_64" "!CMAKE_DIR!" >nul 2>&1
        del "!PROJECT_ROOT!\cmake_portable.zip" >nul 2>&1
    )
    set "PATH=!CMAKE_DIR!\bin;%PATH%"
)

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -property installationPath 2^>nul`) do set "VS_PATH=%%i"
)
if not defined VS_PATH ( echo   %RED%ERROR: VS 2022 not found.%RESET% & pause & exit /b 1 )

if not defined DevEnvDir (
    set "VCVARS=!VS_PATH!\VC\Auxiliary\Build\vcvars64.bat"
    if exist "!VCVARS!" ( call "!VCVARS!" >nul 2>&1 ) else ( echo   %RED%ERROR: vcvars64.bat not found!%RESET% & pause & exit /b 1 )
)
echo   %GREEN%Environment OK%RESET%

REM --- STEP 2: Dependencies & Tools ---
set /a "STEP+=1"
echo.
echo %BOLD%[2/%TOTAL_STEPS%] Resolving Dependencies ^& Tools (vcpkg, Vulkan, Shaders)...%RESET%

REM vcpkg
set "VCPKG_DIR="
if defined VCPKG_ROOT ( if exist "!VCPKG_ROOT!\.vcpkg-root" set "VCPKG_DIR=!VCPKG_ROOT!" )
if not defined VCPKG_DIR (
    for %%p in ("c:\vcpkg" "c:\src\vcpkg" "%USERPROFILE%\vcpkg" "D:\vcpkg") do ( if exist "%%~p\.vcpkg-root" set "VCPKG_DIR=%%~p" )
)
if not defined VCPKG_DIR ( 
    echo   Cloning vcpkg...
    git clone https://github.com/microsoft/vcpkg.git "%USERPROFILE%\vcpkg" --depth=1 >nul 2>&1
    set "VCPKG_DIR=%USERPROFILE%\vcpkg"
)
set "VCPKG_ROOT=!VCPKG_DIR!"

if not exist "!VCPKG_DIR!\vcpkg.exe" (
    echo   Bootstrapping vcpkg...
    if exist "!VCPKG_DIR!\.git" ( pushd "!VCPKG_DIR!" & git reset --hard origin/master --quiet & popd )
    call "!VCPKG_DIR!\bootstrap-vcpkg.bat" >> "!LOG_FILE!" 2>&1
)
"!VCPKG_DIR!\vcpkg.exe" integrate install >nul 2>&1

REM Vulkan Headers
set "VULKAN_DEP_DIR=!PROJECT_ROOT!\deps\vulkan"
if not exist "!VULKAN_DEP_DIR!\include\vulkan\vulkan.h" (
    echo   Downloading Vulkan Headers...
    mkdir "!VULKAN_DEP_DIR!" >nul 2>&1
    curl -L "https://github.com/KhronosGroup/Vulkan-Headers/archive/refs/tags/v1.3.275.zip" -o "!PROJECT_ROOT!\vulkan_headers.zip" 2>nul
    powershell -command "Expand-Archive -Force '!PROJECT_ROOT!\vulkan_headers.zip' '!PROJECT_ROOT!\deps'"
    move "!PROJECT_ROOT!\deps\Vulkan-Headers-1.3.275\include" "!VULKAN_DEP_DIR!\include" >nul 2>&1
    rd /s /q "!PROJECT_ROOT!\deps\Vulkan-Headers-1.3.275" >nul 2>&1
    del "!PROJECT_ROOT!\vulkan_headers.zip" >nul 2>&1
)

REM Shaders
set "GLSLANG="
if defined VULKAN_SDK (
    set "TMP_SDK=!VULKAN_SDK:"=!"
    if exist "!TMP_SDK!\Bin\glslangValidator.exe" set "GLSLANG=!TMP_SDK!\Bin\glslangValidator.exe"
)
if not defined GLSLANG set "GLSLANG=!PROJECT_ROOT!\tools\glslang\bin\glslangValidator.exe"

if exist "!GLSLANG!" (
    pushd "!PROJECT_ROOT!\data\shaders"
    for %%f in (*_vk.glsl) do (
        set "FNAME=%%f"
        set "STAGE="
        if not "!FNAME:vertex=!"=="!FNAME!" set "STAGE=vert"
        if not "!FNAME:fragment=!"=="!FNAME!" set "STAGE=frag"
        if not "!STAGE!"=="" (
            "!GLSLANG!" -V -S !STAGE! "%%f" -o "!FNAME!.spv" >> "!LOG_FILE!" 2>&1
        )
    )
    popd
)
echo   %GREEN%Dependencies OK%RESET%

REM --- STEP 3: Configure Project ---
set /a "STEP+=1"
echo.
echo %BOLD%[3/%TOTAL_STEPS%] Configuring CMake...%RESET%
mkdir "!BUILD_DIR!" >nul 2>&1
cmake -G "Visual Studio 17 2022" -A x64 -S "!PROJECT_ROOT!" -B "!BUILD_DIR!" ^
    -DCMAKE_BUILD_TYPE=Release ^
    "-DCMAKE_TOOLCHAIN_FILE=!VCPKG_DIR!\scripts\buildsystems\vcpkg.cmake" ^
    "-DVCPKG_TARGET_TRIPLET=x64-windows" ^
    "-DCMAKE_INSTALL_PREFIX=!INSTALL_DIR!" >> "!LOG_FILE!" 2>&1
if !ERRORLEVEL! neq 0 ( set "FAIL_REASON=CMake Configuration" & goto :handle_error )
echo   %GREEN%OK%RESET%

REM --- STEP 4: Build ---
set /a "STEP+=1"
echo.
echo %BOLD%[4/%TOTAL_STEPS%] Building Release...%RESET%

echo   Verifiziere Datei-Limits (Max 500 Zeilen)...
set "OVERSIZED=0"
for /r "!PROJECT_ROOT!\source" %%F in (*.cpp) do (
    for /f %%A in ('find /c /v "" ^< "%%F"') do set "LINES=%%A"
    if !LINES! gtr 500 (
        echo   %YELLOW%Warning: %%~nxF hat !LINES! Zeilen ^(Limit: 500^)%RESET%
        set "OVERSIZED=1"
    )
)

for /f %%i in ('dir /s /b "!PROJECT_ROOT!\source\*.cpp" ^| find /c /v ""') do set "TOTAL_FILES=%%i"
if not defined CORES set "CORES=%NUMBER_OF_PROCESSORS%"
echo   Compiling !TOTAL_FILES! source files...

REM Create external PowerShell build monitor script to avoid Cmd parser errors
set "PS_MONITOR=%TEMP%\mme_build_monitor.ps1"
echo $total = !TOTAL_FILES! > "!PS_MONITOR!"
echo $current = 0 >> "!PS_MONITOR!"
echo $logFile = '!LOG_FILE!' >> "!PS_MONITOR!"
echo ^& cmake --build '!BUILD_DIR!' --config Release --parallel !CORES! 2^>^&1 ^| ForEach-Object { >> "!PS_MONITOR!"
echo     if ($_ -match 'Building CXX object') { >> "!PS_MONITOR!"
echo         $current++ >> "!PS_MONITOR!"
echo         $percent = [math]::Min(100, [math]::Round(($current / $total) * 100)) >> "!PS_MONITOR!"
echo         $filled = [math]::Floor($percent / 5) >> "!PS_MONITOR!"
echo         $bar = '[' + ('=' * $filled) + (' ' * (20 - $filled)) + ']' >> "!PS_MONITOR!"
echo         Write-Host -NoNewline "`r  !CYAN!$bar $percent%% ($current/$total)!RESET! " >> "!PS_MONITOR!"
echo     } >> "!PS_MONITOR!"
echo     $_ ^| Out-File -FilePath $logFile -Append -Encoding utf8 >> "!PS_MONITOR!"
echo } >> "!PS_MONITOR!"
echo exit $LASTEXITCODE >> "!PS_MONITOR!"

powershell -NoProfile -ExecutionPolicy Bypass -File "!PS_MONITOR!"

if !ERRORLEVEL! neq 0 ( set "FAIL_REASON=C++ Compilation" & goto :handle_error )
echo.
echo   %GREEN%Build abgeschlossen.%RESET%

REM --- STEP 5: Standalone Release ---
set /a "STEP+=1"
echo.
echo %BOLD%[5/%TOTAL_STEPS%] Creating Standalone Release...%RESET%

set "EXE_PATH="
if exist "!BUILD_DIR!" (
    pushd "!BUILD_DIR!"
    for /r %%f in (*.exe) do (
        if /i "%%~nxf"=="MME.exe" ( set "EXE_PATH=%%f" & set "BIN_DIR=%%~dpf" & goto :found_exe )
        if /i "%%~nxf"=="rme.exe" ( set "EXE_PATH=%%f" & set "BIN_DIR=%%~dpf" & goto :found_exe )
    )
:found_exe
    popd
)
if defined EXE_PATH (
    echo   [+] Found executable: !EXE_PATH!
) else (
    echo   %RED%ERROR: Executable not found in build directory!%RESET%
    pause & exit /b 1
)

REM Granularer Release Monitor für Assets
set "PS_RELEASE=%TEMP%\mme_release_monitor.ps1"
(
echo $source = '!PROJECT_ROOT!'
echo $dest = '!INSTALL_DIR!'
echo $exeSource = '!EXE_PATH!'
echo $binDir = '!BIN_DIR!'
echo if ^(!(Test-Path $dest^)^) { New-Item -ItemType Directory -Path $dest -Force ^| Out-Null }
echo $folders = @('data', 'brushes', 'scripts', 'extensions', 'icons'^)
echo $files = Get-ChildItem -Path $folders.ForEach({ Join-Path $source $_ }) -Recurse -File -ErrorAction SilentlyContinue
echo $total = $files.Count + 2
echo $current = 0
echo function Upd {
echo     $p = [math]::Min(100, [math]::Round(($args[0] / $args[1]^) * 100^)^)
echo     $f = [math]::Floor($p / 5^); $b = '[' + ('=' * $f^) + (' ' * (20 - $f^)^) + ']'
echo     Write-Host -NoNewline "`r  !CYAN!$b $p%% ($($args[0])/$($args[1])^)!RESET! "
echo }
echo Copy-Item $exeSource -Destination (Join-Path $dest 'MME.exe'^) -Force; $current++; Upd $current $total
echo Get-ChildItem (Join-Path $binDir '*.dll'^) ^| ForEach-Object { Copy-Item $_.FullName -Destination $dest -Force }; $current++; Upd $current $total
echo foreach ($f in $files^) {
echo     $target = $f.FullName.Replace($source, $dest^); $tdir = Split-Path $target
echo     if ^(!(Test-Path $tdir^)^) { New-Item -ItemType Directory -Path $tdir -Force ^| Out-Null }
echo     Copy-Item $f.FullName -Destination $target -Force; $current++; Upd $current $total
echo }
) > "!PS_RELEASE!"

powershell -NoProfile -ExecutionPolicy Bypass -File "!PS_RELEASE!"
echo.
echo   %GREEN%OK%RESET%

if exist "!ERROR_FILE!" del "!ERROR_FILE!"
echo # Build Successful > "!ERROR_FILE!"
echo Last build completed at !DATE! !TIME! >> "!ERROR_FILE!"

echo.
echo %GREEN%========================================================%RESET%
echo   Build Successful! Release files copied to:
echo   !INSTALL_DIR!
echo %GREEN%========================================================%RESET%
echo.
pause
exit /b 0

:handle_error
echo.
echo %RED%ERROR: !FAIL_REASON! failed!%RESET%
echo # Compiler Error Report > "!ERROR_FILE!"
echo ## Context >> "!ERROR_FILE!"
echo - **Step:** !FAIL_REASON! >> "!ERROR_FILE!"
echo - **Time:** !DATE! !TIME! >> "!ERROR_FILE!"
echo. >> "!ERROR_FILE!"
echo ## Log Snippet (Last 20 lines) >> "!ERROR_FILE!"
echo ^`^`^`text >> "!ERROR_FILE!"
powershell -command "$content = Get-Content '!LOG_FILE!' -ErrorAction SilentlyContinue; $err = if ($content) { $content | Where-Object { $_ -match 'error' -or $_ -match 'FAILED' } } else { $null }; $out = if ($err) { $err | Select-Object -Last 20 } elseif ($content) { $content | Select-Object -Last 20 } else { 'Log is empty' }; $out | Out-File -FilePath '!ERROR_FILE!' -Encoding utf8 -Append"
echo ^`^`^` >> "!ERROR_FILE!"
echo %YELLOW%Details written to compiler_error_latest.md for AI review.%RESET%
pause & exit /b 1