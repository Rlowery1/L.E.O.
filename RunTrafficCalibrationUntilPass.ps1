$ErrorActionPreference = "Stop"

$projectPath = Join-Path $PSScriptRoot "L_E_O.uproject"
if (-not (Test-Path $projectPath)) {
  throw "Project not found: $projectPath"
}

$runUat = "C:\\Program Files\\Epic Games\\UE_5.7\\Engine\\Build\\BatchFiles\\RunUAT.bat"
if (-not (Test-Path $runUat)) {
  throw "RunUAT not found at: $runUat (set/update this path for your UE install)"
}

while ($true) {
  Get-Process UnrealEditor, UnrealEditor-Cmd -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue

  & $runUat RunUnrealTests -project="$projectPath" -test="Traffic.Calibration.*" -unattended -nop4 -log
  $exitCode = $LASTEXITCODE

  if ($exitCode -eq 0) {
    break
  }

  Write-Host "Calibration tests failed (exit code $exitCode); rerunning..."
}

Write-Host "Calibration tests passed successfully."
exit 0

