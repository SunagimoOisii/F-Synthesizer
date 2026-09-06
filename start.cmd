@echo off
setlocal
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0F-Synthesizer\scripts\check.ps1" -Configuration Release
if errorlevel 1 goto failed
if /I "%~1"=="--build-only" exit /b 0
start "" /D "%~dp0F-Synthesizer" "%~dp0F-Synthesizer\build\x64\Release\F-Synthesizer.exe"
if errorlevel 1 goto failed
exit /b 0

:failed
echo Build or launch failed. See the error above.
if /I not "%~1"=="--build-only" pause
exit /b 1
