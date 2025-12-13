param(
  [int]$TimeoutSeconds = 1800,
  [int]$StallSeconds = 300,
  [int]$MaxAttempts = 0,
  [ValidateSet("FailFast", "Suite")]
  [string]$Mode = "FailFast",
  [string[]]$Tests = @(
    "Traffic.Calibration.BaselineStraightChaos",
    "Traffic.Calibration.BaselineCurveChaos",
    "Traffic.Calibration.RoadLab.Editor",
    "Traffic.Calibration.RoadLab.Integration",
    "Traffic.Calibration.RoadLab.PIE"
  )
)

$ErrorActionPreference = "Stop"

$projectPath = Join-Path $PSScriptRoot "L_E_O.uproject"
if (-not (Test-Path $projectPath)) {
  throw "Project not found: $projectPath"
}

$runUat = "C:\\Program Files\\Epic Games\\UE_5.7\\Engine\\Build\\BatchFiles\\RunUAT.bat"
if (-not (Test-Path $runUat)) {
  throw "RunUAT not found at: $runUat (set/update this path for your UE install)"
}

$mutexName = "AAAtraffic.RunTrafficCalibrationUntilPass"
$createdNew = $false
$mutex = New-Object System.Threading.Mutex($true, $mutexName, [ref]$createdNew)
if (-not $createdNew) {
  throw "Another instance of RunTrafficCalibrationUntilPass.ps1 is already running (mutex: $mutexName)."
}

function Stop-UatRelatedProcesses() {
  Get-Process UnrealEditor, UnrealEditor-Cmd, RunUAT, AutomationTool -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue

  # UE AutomationTool typically runs as dotnet.exe AutomationTool.dll.
  Get-CimInstance Win32_Process -Filter "name='dotnet.exe'" |
    Where-Object { $_.CommandLine -like '*AutomationTool.dll*' } |
    ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }

  # RunUAT.bat typically runs as cmd.exe /c "...RunUAT.bat ..."
  Get-CimInstance Win32_Process -Filter "name='cmd.exe'" |
    Where-Object { $_.CommandLine -like '*RunUAT.bat*' } |
    ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }
}

function Invoke-RunUatWithHangGuards([string[]]$UatArgs, [int]$TimeoutSeconds, [int]$StallSeconds) {
  $proc = Start-Process -FilePath $runUat -ArgumentList $UatArgs -PassThru -NoNewWindow

  $start = Get-Date
  $lastActivity = $start
  $lastCpuSnapshot = @{}

  while (-not $proc.HasExited) {
    Start-Sleep -Seconds 5

    $elapsed = (Get-Date) - $start
    if ($elapsed.TotalSeconds -ge $TimeoutSeconds) {
      Write-Host "RunUAT exceeded timeout; killing UnrealEditor/RunUAT..."
      try { $proc.Kill() } catch {}
      Stop-UatRelatedProcesses
      return 124
    }

    $activityChanged = $false
    foreach ($name in @("UnrealEditor-Cmd", "UnrealEditor", "AutomationTool", "RunUAT")) {
      $p = Get-Process -Name $name -ErrorAction SilentlyContinue | Select-Object -First 1
      if (-not $p) {
        continue
      }
      $cpu = [double]$p.CPU
      $key = "$($p.ProcessName):$($p.Id)"
      if ($lastCpuSnapshot.ContainsKey($key)) {
        if ($cpu -gt ($lastCpuSnapshot[$key] + 0.01)) {
          $activityChanged = $true
        }
      }
      $lastCpuSnapshot[$key] = $cpu
    }

    if ($activityChanged) {
      $lastActivity = Get-Date
    }

    $stallElapsed = (Get-Date) - $lastActivity
    if ($stallElapsed.TotalSeconds -ge $StallSeconds) {
      Write-Host "RunUAT appears stalled (no CPU activity for ${StallSeconds}s); killing UnrealEditor/RunUAT..."
      try { $proc.Kill() } catch {}
      Stop-UatRelatedProcesses
      return 125
    }
  }

  try { $proc.WaitForExit() } catch {}
  return $proc.ExitCode
}

$attempt = 0
try {
  while ($true) {
    $attempt += 1
    if ($MaxAttempts -gt 0 -and $attempt -gt $MaxAttempts) {
      Write-Host "Calibration tests still failing after $MaxAttempts attempts; stopping."
      exit 1
    }

    Stop-UatRelatedProcesses

    $timeoutMinutes = [math]::Round($TimeoutSeconds / 60.0, 1)
    Write-Host "Calibration attempt $attempt (mode $Mode, timeout ${TimeoutSeconds}s / ${timeoutMinutes}m, stall ${StallSeconds}s)..."

    $allOk = $true

  if ($Mode -eq "FailFast") {
    foreach ($testName in $Tests) {
      Stop-UatRelatedProcesses

      $args = @(
        "RunUnrealTests",
        "-project=`"$projectPath`"",
        "-build=`"Editor`"",
        "-Test=`"Gauntlet.UnrealTest.EngineTest`"",
        "-RunTest=`"$testName`"",
        "-UseEditor=true",
        "-nullrhi",
        "-nop4",
        "-log"
      )

        Write-Host "Running $testName..."
        $exitCode = Invoke-RunUatWithHangGuards -UatArgs $args -TimeoutSeconds $TimeoutSeconds -StallSeconds $StallSeconds
        if ($exitCode -ne 0) {
          Write-Host "$testName failed (exit code $exitCode); restarting attempt..."
          $allOk = $false
          break
        }
      }
  } else {
    $args = @(
      "RunUnrealTests",
      "-project=`"$projectPath`"",
      "-build=`"Editor`"",
      "-Test=`"Gauntlet.UnrealTest.EngineTest`"",
      "-RunTest=`"Traffic.Calibration.*`"",
      "-UseEditor=true",
      "-nullrhi",
      "-nop4",
      "-log"
    )
      $exitCode = Invoke-RunUatWithHangGuards -UatArgs $args -TimeoutSeconds $TimeoutSeconds -StallSeconds $StallSeconds
      if ($exitCode -ne 0) {
        Write-Host "Calibration suite failed (exit code $exitCode); restarting attempt..."
        $allOk = $false
      }
    }

    if ($allOk) {
      break
    }
  }

  Write-Host "Calibration tests passed successfully."
  exit 0
} finally {
  try { $mutex.ReleaseMutex() } catch {}
  try { $mutex.Dispose() } catch {}
}
