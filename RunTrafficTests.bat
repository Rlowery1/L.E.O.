@echo off
setlocal enabledelayedexpansion

REM KILL ANY STALE EDITOR INSTANCES
for /f "tokens=2 delims==; " %%P in ('tasklist /FI "IMAGENAME eq UnrealEditor.exe" /FO LIST ^| find "="') do (
    taskkill /PID %%P /F >nul 2>&1
)
for /f "tokens=2 delims==; " %%P in ('tasklist /FI "IMAGENAME eq UnrealEditor-Cmd.exe" /FO LIST ^| find "="') do (
    taskkill /PID %%P /F >nul 2>&1
)

REM RUN FULL EDITOR WITH AUTOMATION COMMANDS
start "" /WAIT "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" ^
  "C:\Users\rllax\L_E_O\L_E_O.uproject" ^
  "/Game/FirstPerson/Lvl_FirstPerson" ^
  -TestFilter=Engine ^
  -nop4 ^
  -unattended ^
  -nullrhi ^
  -stdout ^
  -FullStdOutLogOutput ^
  -TestExit="Automation Test Queue Empty" ^
  -ExecCmds="Automation RunTests Traffic"

exit /b %ERRORLEVEL%
