@echo off
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0native\PublishBeta.ps1" -Build
if errorlevel 1 (
  echo Beta publishing failed. See the error above.
  pause
  exit /b 1
)
echo Beta published to https://github.com/mviterna/wildline-downloads
pause
