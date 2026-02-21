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

Write-Host "[1/11] help output"
& $exePath --help | Out-Host

Write-Host "[2/11] cli success run (default.json)"
& $exePath --cli --config (Join-Path $repoRoot "config\default.json") | Out-Host
if ($LASTEXITCODE -ne 0) {
    throw "Expected success for default.json, but exit code was $LASTEXITCODE."
}

Write-Host "[3/11] cli success run (channel_minimal.json)"
& $exePath --cli --config (Join-Path $repoRoot "config\samples\channel_minimal.json") | Out-Host
if ($LASTEXITCODE -ne 0) {
    throw "Expected success for channel_minimal.json, but exit code was $LASTEXITCODE."
}

Write-Host "[4/11] cli success run (channel_full.json)"
& $exePath --cli --config (Join-Path $repoRoot "config\samples\channel_full.json") | Out-Host
if ($LASTEXITCODE -ne 0) {
    throw "Expected success for channel_full.json, but exit code was $LASTEXITCODE."
}

Write-Host "[5/11] cli success run (mix_all_mute.json)"
$mixAllMutePath = Join-Path $repoRoot "config\samples\mix_all_mute.json"
$mixOut = & $exePath --cli --config $mixAllMutePath | Out-String
$mixOut | Out-Host
if ($LASTEXITCODE -ne 0) {
    throw "Expected success for mix_all_mute.json, but exit code was $LASTEXITCODE."
}
if ($mixOut -notmatch "\[RenderStats\].*nonZero=0/") {
    throw "Expected nonZero=0 render stats for mix_all_mute.json."
}

Write-Host "[6/11] cli success run (preset: basic_wave)"
$basicOut = & $exePath --cli --preset basic_wave | Out-String
$basicOut | Out-Host
if ($LASTEXITCODE -ne 0) {
    throw "Expected success for preset basic_wave, but exit code was $LASTEXITCODE."
}
if ($basicOut -notmatch "\[RenderStats\].*nonZero=[1-9]") {
    throw "Expected non-zero render stats for preset basic_wave."
}

Write-Host "[7/11] cli success run (preset: fm_default)"
$fmOut = & $exePath --cli --preset fm_default | Out-String
$fmOut | Out-Host
if ($LASTEXITCODE -ne 0) {
    throw "Expected success for preset fm_default, but exit code was $LASTEXITCODE."
}
if ($fmOut -notmatch "\[RenderStats\].*nonZero=[1-9]") {
    throw "Expected non-zero render stats for preset fm_default."
}

Write-Host "[8/11] cli failure run (channel_mix_invalid.json)"
& $exePath --cli --config (Join-Path $repoRoot "config\samples\channel_mix_invalid.json") | Out-Host
if ($LASTEXITCODE -eq 0) {
    throw "Expected failure for channel_mix_invalid.json, but exit code was 0."
}

Write-Host "[9/11] cli failure run (missing config)"
& $exePath --cli --config (Join-Path $repoRoot "config\__missing__.json") | Out-Host
if ($LASTEXITCODE -eq 0) {
    throw "Expected failure for missing config, but exit code was 0."
}

Write-Host "[10/11] cli failure run (channel_invalid.json)"
& $exePath --cli --config (Join-Path $repoRoot "config\samples\channel_invalid.json") | Out-Host
if ($LASTEXITCODE -eq 0) {
    throw "Expected failure for channel_invalid.json, but exit code was 0."
}

Write-Host "[11/11] gui launch smoke (2 sec)"
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
