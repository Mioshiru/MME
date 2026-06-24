@echo off
echo ==========================================
echo Bereinige CMake Cache und Build-Dateien...
echo ==========================================

if exist "build" rmdir /s /q "build"
if exist "compiler_error.md" del /q "compiler_error.md"

echo Bereinigung erfolgreich abgeschlossen! Startbereit fuer einen Clean Build.
pause