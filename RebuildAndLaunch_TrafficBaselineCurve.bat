@echo off
setlocal enabledelayedexpansion

REM Hard-stop Editor/LiveCoding so rebuilt plugin DLLs actually load.
for %%I in (UnrealEditor.exe UnrealEditor-Cmd.exe LiveCodingConsole.exe) do (
  taskkill /IM %%I /F /T >nul 2>&1
)

REM Build Editor target.
call "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" ^
  L_E_OEditor Win64 Development ^
  -Project="C:\Users\rllax\L_E_O\L_E_O.uproject" ^
  -WaitMutex -NoHotReload
if errorlevel 1 exit /b %errorlevel%

REM Launch Editor directly into the traffic baseline map.
start "" "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe" ^
  "C:\Users\rllax\L_E_O\L_E_O.uproject" ^
  "/AAAtrafficSystem/Maps/Traffic_BaselineCurve"

exit /b 0
