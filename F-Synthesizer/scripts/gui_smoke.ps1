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

Write-Host "[1/4] help output"
& $exePath --help | Out-Host

Write-Host "[2/4] cli success run"
& $exePath --cli --config (Join-Path $repoRoot "config\default.json") | Out-Host

Write-Host "[3/4] cli failure run (missing config)"
& $exePath --cli --config (Join-Path $repoRoot "config\__missing__.json") | Out-Host
if ($LASTEXITCODE -eq 0) {
    throw "Expected failure for missing config, but exit code was 0."
}

Write-Host "[4/4] gui launch smoke (2 sec)"
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
