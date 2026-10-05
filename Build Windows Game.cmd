@echo off
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0native\LaunchNative.ps1" -Build
pause
