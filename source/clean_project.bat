@echo off
setlocal

echo %~dp0
pushd "%~dp0.."

echo [CLEAN] Lösche Build-Artefakte und temporäre Dateien in %CD%...

REM Build-Ordner von CMake
if exist "build" (
    echo   [-] Entferne build/ Verzeichnis...
    rmdir /s /q "build"
)

REM Visual Studio Cache
if exist ".vs" (
    echo   [-] Entferne .vs/ Cache...
    rmdir /s /q ".vs"
)

REM Lokale vcpkg Installationen (können mehrere GB groß sein)
if exist "vcpkg_installed" (
    echo   [-] Entferne lokale vcpkg Bibliotheken...
    rmdir /s /q "vcpkg_installed"
)

echo [OK] Fertig. Das Verzeichnis ist nun wieder schlank.
popd
pause
endlocal