@echo off
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0native\LaunchNative.ps1" -Build
if errorlevel 1 (
  echo Game or installer build failed. See the error above.
  pause
  exit /b 1
)
echo Game is ready in native\Build\Windows. Installer is ready in native\Build\Beta.
pause
