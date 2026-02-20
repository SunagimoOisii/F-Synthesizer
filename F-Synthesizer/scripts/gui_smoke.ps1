param(
    [string]$Configuration = "Debug",
    [string]$Platform = "x64"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$exePath = Join-Path $repoRoot "build\$Platform\$Configuration\F-Synthesizer.exe"

if (-not (Test-Path $exePath)) {
    throw "Executable not found: $exePath"
}

Write-Host "== GUI smoke test =="
Write-Host "Exe: $exePath"

Write-Host "[1/7] help output"
& $exePath --help | Out-Host

Write-Host "[2/7] cli success run (default.json)"
& $exePath --cli --config (Join-Path $repoRoot "config\default.json") | Out-Host
if ($LASTEXITCODE -ne 0) {
    throw "Expected success for default.json, but exit code was $LASTEXITCODE."
}

Write-Host "[3/7] cli success run (channel_minimal.json)"
& $exePath --cli --config (Join-Path $repoRoot "config\samples\channel_minimal.json") | Out-Host
if ($LASTEXITCODE -ne 0) {
    throw "Expected success for channel_minimal.json, but exit code was $LASTEXITCODE."
}

Write-Host "[4/7] cli success run (channel_full.json)"
& $exePath --cli --config (Join-Path $repoRoot "config\samples\channel_full.json") | Out-Host
if ($LASTEXITCODE -ne 0) {
    throw "Expected success for channel_full.json, but exit code was $LASTEXITCODE."
}

Write-Host "[5/7] cli failure run (missing config)"
& $exePath --cli --config (Join-Path $repoRoot "config\__missing__.json") | Out-Host
if ($LASTEXITCODE -eq 0) {
    throw "Expected failure for missing config, but exit code was 0."
}

Write-Host "[6/7] cli failure run (channel_invalid.json)"
& $exePath --cli --config (Join-Path $repoRoot "config\samples\channel_invalid.json") | Out-Host
if ($LASTEXITCODE -eq 0) {
    throw "Expected failure for channel_invalid.json, but exit code was 0."
}

Write-Host "[7/7] gui launch smoke (2 sec)"
$p = Start-Process -FilePath $exePath -PassThru
Start-Sleep -Seconds 2
if (-not $p.HasExited) {
    Stop-Process -Id $p.Id -Force
    Write-Host "GUI process launched successfully."
}
else {
    throw "GUI process exited unexpectedly with code $($p.ExitCode)"
}

Write-Host "GUI smoke test completed."
