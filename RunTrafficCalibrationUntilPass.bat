@echo off
setlocal

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0RunTrafficCalibrationUntilPass.ps1"
exit /b %ERRORLEVEL%

