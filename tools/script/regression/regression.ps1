#!/usr/bin/env pwsh
#
# computrendering GPU Simulator — Regression Test Script (Windows PowerShell)
#
# Parses regression_list and runs computrender (bhavmodel) for each test trace.
# Compares output PPMs against reference using pixel-level comparison.
#
# Usage:
#   pwsh tools/script/regression/regression.ps1
#   powershell -File tools/script/regression/regression.ps1
#

param(
    [string]$Config = "Debug",
    [switch]$PerfModel
)

$ErrorActionPreference = "Stop"

# ---- Resolve paths ----
$ScriptDir  = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = (Resolve-Path "$ScriptDir/../../..").Path
$RegList    = Join-Path $ScriptDir "regression_list"
$RegOut     = Join-Path $ScriptDir "regression.out"
$Simulator  = Join-Path $ProjectRoot "_BUILD_/arch/$Config/computrender.exe"
$ParamCSV   = Join-Path $ProjectRoot "arch/common/params/archParams.csv"
$TraceBase  = Join-Path $ProjectRoot "tests"
$script:D3D9ProbeDone = $false
$script:D3D9SkipReason = $null

if (-not (Test-Path $Simulator)) {
    Write-Host "ERROR: Simulator not found at $Simulator" -ForegroundColor Red
    Write-Host "       Build first: cd _BUILD_; cmake --build . --config $Config --target computrender"
    exit 1
}
if (-not (Test-Path $RegList)) {
    Write-Host "ERROR: Regression list not found at $RegList" -ForegroundColor Red
    exit 1
}

# ---- Model suffix ----
$ModelSuffix = if ($PerfModel) { "cm" } else { "bm" }
$ModelFlag   = if ($PerfModel) { "--pm" } else { "" }

# ---- Helper: resolve test directory ----
function Resolve-TestPath($listDir) {
    $parts = $listDir -split '/', 2
    $api  = $parts[0]
    $rest = $parts[1]
    # Try tests/<api>/trace/<rest>
    $candidate = Join-Path $TraceBase "$api/trace/$rest"
    if (Test-Path $candidate) { return $candidate }
    # Fallback tests/<api>/<rest>
    $candidate = Join-Path $TraceBase "$api/$rest"
    if (Test-Path $candidate) { return $candidate }
    return $null
}

function Test-IsD3DCase($listDir) {
    return $listDir.StartsWith("d3d/") -or $listDir.StartsWith("d3d9/")
}

function Test-D3D9Support($tracePath, $archVer, $modelFlag) {
    if ($script:D3D9ProbeDone) { return }
    $script:D3D9ProbeDone = $true

    $probeDir = Join-Path ([System.IO.Path]::GetTempPath()) ("computrender-d3d9-probe-" + [Guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Path $probeDir | Out-Null

    try {
        $probeOut = Join-Path $probeDir "probe.out"
        $probeErr = Join-Path $probeDir "probe.err"
        $probeArgs = @()
        if ($modelFlag) { $probeArgs += $modelFlag }
        $probeArgs += "--param", $ParamCSV
        $probeArgs += "--arch", $archVer
        $probeArgs += "--trace", $tracePath
        $probeArgs += "--frames", "1"

        $probeProc = Start-Process -FilePath $Simulator -ArgumentList $probeArgs `
                                  -WorkingDirectory $probeDir `
                                  -RedirectStandardOutput $probeOut `
                                  -RedirectStandardError $probeErr `
                                  -NoNewWindow -Wait -PassThru

        $probeText = ""
        if (Test-Path $probeOut) { $probeText += (Get-Content $probeOut -Raw) }
        if (Test-Path $probeErr) { $probeText += (Get-Content $probeErr -Raw) }

        if ($probeText -match "Unsupported API type 'd3d9'") {
            $script:D3D9SkipReason = "simulator build does not include D3D9 trace support"
        }
    } finally {
        Remove-Item -Recurse -Force $probeDir -ErrorAction SilentlyContinue
    }
}

# ---- Helper: compare PPM files ----
function Compare-PPM($refFile, $testFile) {
    $refBytes  = [System.IO.File]::ReadAllBytes($refFile)
    $testBytes = [System.IO.File]::ReadAllBytes($testFile)
    if ($refBytes.Length -ne $testBytes.Length) {
        return @{ Match = $false; Pct = 100.0; Detail = "Size mismatch: ref=$($refBytes.Length) test=$($testBytes.Length)" }
    }
    # Find header end (after 3 newlines in P6 format)
    $nlCount = 0; $hdrEnd = 0
    for ($i = 0; $i -lt [Math]::Min($refBytes.Length, 100); $i++) {
        if ($refBytes[$i] -eq 10) { $nlCount++; if ($nlCount -eq 3) { $hdrEnd = $i + 1; break } }
    }
    # Compare pixel data
    $diffCount = 0
    $totalPixels = ($refBytes.Length - $hdrEnd) / 3
    for ($i = $hdrEnd; $i -lt $refBytes.Length; $i++) {
        if ($refBytes[$i] -ne $testBytes[$i]) { $diffCount++ }
    }
    if ($diffCount -eq 0) {
        return @{ Match = $true; Pct = 0.0; Detail = "identical" }
    } else {
        $pct = [math]::Round(100.0 * $diffCount / ($refBytes.Length - $hdrEnd), 2)
        return @{ Match = $false; Pct = $pct; Detail = "$diffCount bytes differ ($pct%)" }
    }
}

# ---- Process tests ----
$Pass = 0; $Fail = 0; $Skip = 0; $Total = 0
"" | Out-File $RegOut

Write-Host "========================================"
Write-Host " computrendering GPU Regression Test (Windows)"
Write-Host " Simulator: $Simulator"
Write-Host " List: $RegList"
Write-Host "========================================"
Write-Host ""

Get-Content $RegList | ForEach-Object {
    $line = $_.Trim()
    if ([string]::IsNullOrWhiteSpace($line)) { return }
    if ($line.StartsWith("#")) { return }

    $fields = $line -split ','
    $rawDir     = $fields[0].Trim()
    $archVer    = $fields[1].Trim()
    $traceFile  = $fields[2].Trim()
    $frames     = if ($fields.Count -gt 3) { $fields[3].Trim() } else { "1" }
    $startFrame = if ($fields.Count -gt 4) { $fields[4].Trim() } else { "0" }
    $tolerance  = if ($fields.Count -gt 5) { $fields[5].Trim() } else { "0" }
    $mode       = if ($fields.Count -gt 6) { $fields[6].Trim().ToLowerInvariant() } else { "image" }

    $Total++

    # Resolve test path
    $testPath = Resolve-TestPath $rawDir
    if (-not $testPath) {
        Write-Host "SKIP: $rawDir (directory not found)" -ForegroundColor Yellow
        $Skip++
        return
    }

    $traceFullPath = Join-Path $testPath $traceFile
    if (-not (Test-Path $traceFullPath)) {
        Write-Host "SKIP: $rawDir (trace $traceFile not found)" -ForegroundColor Yellow
        $Skip++
        return
    }

    if ($mode -ne "image" -and $mode -ne "smoke") {
        Write-Host "SKIP: $rawDir (unsupported regression mode '$mode')" -ForegroundColor Yellow
        $Skip++
        return
    }

    if (Test-IsD3DCase $rawDir) {
        Test-D3D9Support $traceFullPath $archVer $ModelFlag
        if ($script:D3D9SkipReason) {
            Write-Host "SKIP: $rawDir ($script:D3D9SkipReason)" -ForegroundColor Yellow
            $Skip++
            return
        }
    }

    # Check reference PPM for image comparisons
    $refPPM = [System.IO.Path]::GetFileNameWithoutExtension($traceFile) + ".ppm"
    $refFullPath = Join-Path $testPath $refPPM
    if ($mode -eq "image" -and -not (Test-Path $refFullPath)) {
        Write-Host "SKIP: $rawDir (reference $refPPM not found)" -ForegroundColor Yellow
        $Skip++
        return
    }

    Write-Host -NoNewline "Executing $rawDir ... "

    # Clean old outputs in test directory
    $workDir = $testPath
    Get-ChildItem $workDir -Filter "frame*.$ModelSuffix.*" -ErrorAction SilentlyContinue | Remove-Item -Force
    Get-ChildItem $workDir -Filter "output.txt" -ErrorAction SilentlyContinue | Remove-Item -Force

    # Build arguments
    $args = @()
    if ($ModelFlag) { $args += $ModelFlag }
    $args += "--param", $ParamCSV
    $args += "--arch", $archVer
    $args += "--trace", $traceFullPath
    $args += "--frames", $frames
    if ([int]$startFrame -gt 0) { $args += "--start", $startFrame }

    # Run simulator
    $outFile = Join-Path $workDir "output.txt"
    $errFile = Join-Path $workDir "output_err.txt"
    $proc = Start-Process -FilePath $Simulator -ArgumentList $args `
                          -WorkingDirectory $workDir `
                          -RedirectStandardOutput $outFile `
                          -RedirectStandardError $errFile `
                          -NoNewWindow -Wait -PassThru

    if ($proc.ExitCode -ne 0) {
        $hasOutput = Test-Path (Join-Path $workDir "frame0000.$ModelSuffix.ppm")
        if (-not $hasOutput) {
            Write-Host "INTERRUPTED (exit $($proc.ExitCode))" -ForegroundColor Red
            "$rawDir : INTERRUPTED (exit $($proc.ExitCode))" | Out-File $RegOut -Append
            $Fail++
            return
        }
        Write-Host "done (exit $($proc.ExitCode), output produced)."
    } else {
        Write-Host "done."
    }

    # Compare output — use startFrame to derive the output frame filename
    $frameIdx = "{0:D4}" -f [int]$startFrame
    $testPPM = Join-Path $workDir "frame$frameIdx.$ModelSuffix.ppm"

    if ($mode -eq "smoke") {
        if ($proc.ExitCode -ne 0) {
            Write-Host "  FAILED - simulator exited with $($proc.ExitCode)" -ForegroundColor Red
            "$rawDir : FAILED, simulator exit $($proc.ExitCode)" | Out-File $RegOut -Append
            $Fail++
            return
        }

        if (-not (Test-Path $testPPM)) {
            Write-Host "  FAILED - output frame$frameIdx.$ModelSuffix.ppm not produced" -ForegroundColor Red
            "$rawDir : FAILED, missing output" | Out-File $RegOut -Append
            $Fail++
            return
        }

        Write-Host "  PASS: frame$frameIdx.$ModelSuffix.ppm - smoke run completed" -ForegroundColor Green
        "$rawDir : PASS (smoke output produced)" | Out-File $RegOut -Append
        $Pass++
        return
    }

    if (-not (Test-Path $testPPM)) {
        Write-Host "  FAILED - output frame$frameIdx.$ModelSuffix.ppm not produced" -ForegroundColor Red
        "$rawDir : FAILED, missing output" | Out-File $RegOut -Append
        $Fail++
        return
    }

    $result = Compare-PPM $refFullPath $testPPM
    if ($result.Match) {
        Write-Host "  PASS: $refPPM - $($result.Detail)" -ForegroundColor Green
        "$rawDir : PASS ($($result.Detail))" | Out-File $RegOut -Append
        $Pass++
    } elseif ([double]$tolerance -gt 0 -and $result.Pct -le [double]$tolerance) {
        Write-Host "  PASS: $refPPM - $($result.Detail) (within tolerance $tolerance%)" -ForegroundColor Green
        "$rawDir : PASS ($($result.Detail), tolerance=$tolerance%)" | Out-File $RegOut -Append
        $Pass++
    } else {
        Write-Host "  FAILED: $refPPM - $($result.Detail)" -ForegroundColor Red
        "$rawDir : FAILED ($($result.Detail))" | Out-File $RegOut -Append
        $Fail++
    }
}

Write-Host ""
Write-Host "========================================"
Write-Host " Results: $Pass passed, $Fail failed, $Skip skipped (of $Total)"
Write-Host "========================================"
Write-Host " Details: $RegOut"

if ($Fail -gt 0) { exit 1 }
exit 0
