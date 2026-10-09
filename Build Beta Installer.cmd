@echo off
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0native\BuildBeta.ps1"
if errorlevel 1 (
  echo Beta installer build failed. See the error above.
  pause
  exit /b 1
)
echo Beta installer is ready in native\Build\Beta.
pause
