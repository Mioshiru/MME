@echo off
REM ============================================================
REM   Mios Map Editor - Professional Windows Build Script (Ninja + .NET)
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
set "INSTALL_DIR=!BUILD_DIR!"
set "LOG_FILE=%TEMP%\mme_build.log"
set "TOTAL_STEPS=6"

REM --- Check if log file is accessible ---
type nul > "!LOG_FILE!" 2>nul || (
    echo %RED%ERROR: !LOG_FILE! is locked by another process.%RESET%
    pause & exit /b 1
)
echo Initializing Build Log>"!LOG_FILE!"
if exist "!ERROR_FILE!" del /f /q "!ERROR_FILE!"

echo.
echo %BOLD%%CYAN%========================================================%RESET%
echo %BOLD%%CYAN%   Mios Map Editor Build Script (FULL HYBRID MODE)%RESET%
echo %BOLD%%CYAN%========================================================%RESET%
echo.

:start_build
REM --- STEP 1: Prep & Environment ---
set /a "STEP=1"
echo %BOLD%[1/%TOTAL_STEPS%] Verifying environment...%RESET%

where git >nul 2>&1 || (echo   %RED%ERROR: Git not found.%RESET% & pause & exit /b 1)
where ninja >nul 2>&1 || (echo   %RED%ERROR: Ninja Build System not found in PATH.%RESET% & pause & exit /b 1)

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
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do set "VS_PATH=%%i"
)
if not defined VS_PATH ( echo   %RED%ERROR: VS 2022 Build Tools not found.%RESET% & pause & exit /b 1 )

set "VCVARS=!VS_PATH!\VC\Auxiliary\Build\vcvars64.bat"
if exist "!VCVARS!" (
    call "!VCVARS!" >nul 2>&1
    if !ERRORLEVEL! neq 0 ( echo   %RED%ERROR: Failed to initialize MSVC environment.%RESET% & pause & exit /b 1 )
) else (
    echo   %RED%ERROR: vcvars64.bat not found!%RESET% & pause & exit /b 1
)

where cl >nul 2>&1 || ( echo   %RED%ERROR: cl.exe not available after VS environment setup.%RESET% & pause & exit /b 1 )
echo   %GREEN%Environment OK%RESET%

REM --- STEP 2: .NET Core Remake Application Build ---
set /a "STEP+=1"
echo.
echo %%BOLD%%[%%STEP%%/%%TOTAL_STEPS%%] Compiling .NET Remake Application...%%RESET%%

set "CS_PROJ=!PROJECT_ROOT!\MapEditor\src\MapEditor.App\MapEditor.App.csproj"
if not exist "!CS_PROJ!" (
    echo   %%RED%%ERROR: MapEditor.App.csproj not found at expected path!%%RESET%%
    set "FAIL_REASON=.NET Project Path Verification"
    goto :handle_error
)

set "LOCAL_DOTNET=!PROJECT_ROOT!\tools\.dotnet"
set "DOTNET_EXE=!LOCAL_DOTNET!\dotnet.exe"
set "USE_LOCAL_DOTNET=0"

REM Check if system dotnet is available and is version 8
dotnet --version >nul 2>&1
if !ERRORLEVEL! neq 0 (
    set "USE_LOCAL_DOTNET=1"
) else (
    for /f "tokens=1 delims=." %%v in ('dotnet --version') do (
        if "%%v" neq "8" (
            set "USE_LOCAL_DOTNET=1"
        )
    )
)

REM If local dotnet already exists, use it
if exist "!DOTNET_EXE!" (
    set "USE_LOCAL_DOTNET=0"
)

if !USE_LOCAL_DOTNET! equ 1 (
    echo   .NET 8.0 SDK not found. Installing locally to !LOCAL_DOTNET!...
    if not exist "!PROJECT_ROOT!\tools" mkdir "!PROJECT_ROOT!\tools" >nul 2>&1
    
    powershell -NoProfile -ExecutionPolicy Bypass -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri 'https://dot.net/v1/dotnet-install.ps1' -OutFile '!PROJECT_ROOT!\tools\dotnet-install.ps1'" >> "!LOG_FILE!" 2>&1
    if !ERRORLEVEL! neq 0 (
        echo   %%RED%%ERROR: Failed to download .NET installation script!%%RESET%%
        set "FAIL_REASON=.NET SDK Script Download"
        goto :handle_error
    )
    
    powershell -NoProfile -ExecutionPolicy Bypass -File "!PROJECT_ROOT!\tools\dotnet-install.ps1" -Channel 8.0 -InstallDir "!LOCAL_DOTNET!" -NoPath >> "!LOG_FILE!" 2>&1
    if !ERRORLEVEL! neq 0 (
        echo   %%RED%%ERROR: Failed to install local .NET 8.0 SDK!%%RESET%%
        set "FAIL_REASON=.NET SDK Installation"
        goto :handle_error
    )
    
    del "!PROJECT_ROOT!\tools\dotnet-install.ps1" >nul 2>&1
)

if exist "!DOTNET_EXE!" (
    echo   Injecting local .NET environment paths...
    set "DOTNET_ROOT=!LOCAL_DOTNET!"
    set "PATH=!LOCAL_DOTNET!;!PATH!"
)

REM Executing compilation with SDK environment context
dotnet build "!CS_PROJ!" /p:Configuration=Release /p:Platform="Any CPU" /clp:ErrorsOnly >> "!LOG_FILE!" 2>&1

if !ERRORLEVEL! neq 0 ( set "FAIL_REASON=.NET Compilation" & goto :handle_error )
echo   %%GREEN%%.NET Build OK%%RESET%%

REM --- STEP 3: Dependencies & Tools ---
set /a "STEP+=1"
echo.
echo %BOLD%[%STEP%/%TOTAL_STEPS%] Resolving Dependencies ^& Tools (vcpkg, Vulkan, Shaders)...%RESET%

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

REM --- STEP 4: Configure Project ---
set /a "STEP+=1"
echo.
set "CMAKE_GENERATOR=Ninja"
set "CMAKE_GENERATOR_ARGS="
set "NINJA_EXE="
where ninja >nul 2>&1
if !ERRORLEVEL! neq 0 (
    echo %YELLOW%Ninja not found in PATH, trying Visual Studio 17 2022 generator...%RESET%
    set "CMAKE_GENERATOR=Visual Studio 17 2022"
    set "CMAKE_GENERATOR_ARGS=-A x64"
) else (
    ninja --version >nul 2>&1
    if !ERRORLEVEL! neq 0 (
        echo %YELLOW%Ninja is present but not functional, trying Visual Studio 17 2022 generator...%RESET%
        set "CMAKE_GENERATOR=Visual Studio 17 2022"
        set "CMAKE_GENERATOR_ARGS=-A x64"
    ) else (
        for /f "usebackq delims=" %%I in (`where ninja`) do set "NINJA_EXE=%%I"
        if defined NINJA_EXE set "CMAKE_GENERATOR_ARGS=-DCMAKE_MAKE_PROGRAM=!NINJA_EXE!"
    )
)
echo %BOLD%[%STEP%/%TOTAL_STEPS%] Configuring CMake with !CMAKE_GENERATOR!...%RESET%
if not exist "!BUILD_DIR!" mkdir "!BUILD_DIR!" >nul 2>&1

if exist "!BUILD_DIR!\CMakeCache.txt" (
    findstr /C:"CMAKE_GENERATOR:INTERNAL=!CMAKE_GENERATOR!" "!BUILD_DIR!\CMakeCache.txt" >nul 2>&1
    if !ERRORLEVEL! neq 0 (
        echo   CMake generator mismatch detected. Cleaning CMake cache...
        del /f /q "!BUILD_DIR!\CMakeCache.txt" >nul 2>&1
        rd /s /q "!BUILD_DIR!\CMakeFiles" >nul 2>&1
    )
)

if /I "!CMAKE_GENERATOR!"=="Visual Studio 17 2022" (
    cmake -G "!CMAKE_GENERATOR!" !CMAKE_GENERATOR_ARGS! -S "!PROJECT_ROOT!" -B "!BUILD_DIR!" ^
        -DCMAKE_BUILD_TYPE=Release ^
        "-DCMAKE_TOOLCHAIN_FILE=!PROJECT_ROOT!\cmake\vcpkg-toolchain.cmake" ^
        "-DVCPKG_TARGET_TRIPLET=x64-windows" ^
        "-DCMAKE_INSTALL_PREFIX=!INSTALL_DIR!" >> "!LOG_FILE!" 2>&1
) else (
    cmake -G "!CMAKE_GENERATOR!" -S "!PROJECT_ROOT!" -B "!BUILD_DIR!" !CMAKE_GENERATOR_ARGS! ^
        -DCMAKE_BUILD_TYPE=Release ^
        "-DCMAKE_TOOLCHAIN_FILE=!PROJECT_ROOT!\cmake\vcpkg-toolchain.cmake" ^
        "-DVCPKG_TARGET_TRIPLET=x64-windows" ^
        "-DCMAKE_INSTALL_PREFIX=!INSTALL_DIR!" >> "!LOG_FILE!" 2>&1
)
if !ERRORLEVEL! neq 0 ( set "FAIL_REASON=CMake Configuration" & goto :handle_error )
echo   %GREEN%OK (Incremental)%RESET%

REM --- STEP 5: Build C++ Native Core ---
set /a "STEP+=1"
echo.
echo %BOLD%[%STEP%/%TOTAL_STEPS%] Building Native C++ Components with Ninja...%RESET%

for /f %%i in ('dir /s /b "!PROJECT_ROOT!\source\*.cpp" ^| find /c /v ""') do set "TOTAL_FILES=%%i"
if not defined CORES set "CORES=%NUMBER_OF_PROCESSORS%"

set "PS_MONITOR=%TEMP%\mme_build_monitor.ps1"
echo $total = !TOTAL_FILES! > "!PS_MONITOR!"
echo $current = 0 >> "!PS_MONITOR!"
echo $logFile = '!LOG_FILE!' >> "!PS_MONITOR!"
echo ^& cmake --build '!BUILD_DIR!' --config Release --parallel !CORES! 2^>^&1 ^| ForEach-Object { >> "!PS_MONITOR!"
echo     if ($_ -match 'Building CXX object' -or $_ -match '\[\d+/\d+\]') { >> "!PS_MONITOR!"
echo         $current++ >> "!PS_MONITOR!"
echo         $percent = [math]::Min(100, [math]::Round(($current / $total) * 100)) >> "!PS_MONITOR!"
echo         $filled = [math]::Floor($percent / 5) >> "!PS_MONITOR!"
echo         $bar = '[' + ('=' * $filled) + (' ' * (20 - $filled)) + ']' >> "!PS_MONITOR!"
echo         Write-Host -NoNewline "`r  !CYAN!$bar $percent%% ($current/$total)!RESET! " >> "!PS_MONITOR!"
echo     } >> "!PS_MONITOR!"
echo     if ($_ -match 'error' -or $_ -match 'FAILED' -or $_ -match 'fatal' -or $_ -match 'Fehler') { >> "!PS_MONITOR!"
echo         Write-Host "`n!RED!$_!RESET!" >> "!PS_MONITOR!"
echo     } >> "!PS_MONITOR!"
echo     $_ ^| Out-File -FilePath $logFile -Append -Encoding utf8 >> "!PS_MONITOR!"
echo } >> "!PS_MONITOR!"
echo exit $LASTEXITCODE >> "!PS_MONITOR!"

powershell -NoProfile -ExecutionPolicy Bypass -File "!PS_MONITOR!"
if !ERRORLEVEL! neq 0 ( set "FAIL_REASON=C++ Compilation" & goto :handle_error )
echo.
echo   %GREEN%Native C++ Build abgeschlossen.%RESET%

REM --- STEP 6: Standalone Release Deployment ---
set /a "STEP+=1"
echo.
echo %BOLD%[%STEP%/%TOTAL_STEPS%] Packaging Runtime Release Artifacts...%RESET%

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
if not defined EXE_PATH (
    echo   %RED%ERROR: Executable not found in build directory!%RESET%
    pause & exit /b 1
)

for %%D in (data brushes scripts extensions icons) do (
    if exist "!PROJECT_ROOT!\%%D" (
        robocopy "!PROJECT_ROOT!\%%D" "!BUILD_DIR!\%%D" /E /NFL /NDL /NJH /NJS /NP /R:1 /W:1 >nul 2>&1
        if !ERRORLEVEL! geq 8 (
            set "FAIL_REASON=Asset Deployment"
            goto :handle_error
        )
    )
)
echo.
echo   %GREEN%Deployment OK%RESET%

echo.
echo %GREEN%========================================================%RESET%
echo   Build Successful! Integrated hybrid binary pack location:
echo   !BUILD_DIR!
echo %GREEN%========================================================%RESET%
echo.
pause
exit /b 0

:handle_error
echo.
echo %RED%ERROR: !FAIL_REASON! failed!%RESET%
set "ERROR_LINE="
for /f "usebackq delims=" %%L in (`powershell -NoProfile -ExecutionPolicy Bypass -Command "$lines = Get-Content '!LOG_FILE!' -ErrorAction SilentlyContinue; $found = $null; foreach ($line in $lines) { if ($line -match 'fatal error|error C[0-9]+|error:|CMake Error|FAILED:|FAILED|Fehler|Error:' -and $line.Trim()) { $found = $line.Trim(); break } }; if ($found) { $found } else { 'No matching error line found in build log.' }"`) do set "ERROR_LINE=%%L"
if not defined ERROR_LINE set "ERROR_LINE=No matching error line found in build log."
echo %RED%!ERROR_LINE!%RESET%
echo # Compiler Error Report > "!ERROR_FILE!"
echo - **Step:** !FAIL_REASON! >> "!ERROR_FILE!"
echo - **Time:** !DATE! !TIME! >> "!ERROR_FILE!"
echo. >> "!ERROR_FILE!"
echo ## First Error >> "!ERROR_FILE!"
echo !ERROR_LINE! >> "!ERROR_FILE!"
echo. >> "!ERROR_FILE!"
echo ## Log Snippet >> "!ERROR_FILE!"
echo ^`^`^`text >> "!ERROR_FILE!"
powershell -command "$content = Get-Content '!LOG_FILE!' -ErrorAction SilentlyContinue; $out = if ($content) { $content | Select-Object -Last 80 } else { 'Log is empty' }; $out | Out-File -FilePath '!ERROR_FILE!' -Encoding utf8 -Append"
echo ^`^`^` >> "!ERROR_FILE!"
echo %YELLOW%Details written to compiler_error_latest.md for review.%RESET%
pause & exit /b 1