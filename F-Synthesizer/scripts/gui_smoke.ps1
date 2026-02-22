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

Write-Host "[1/15] help output"
& $exePath --help | Out-Host

Write-Host "[2/15] cli success run (default.json)"
& $exePath --cli --config (Join-Path $repoRoot "config\default.json") | Out-Host
if ($LASTEXITCODE -ne 0) {
    throw "Expected success for default.json, but exit code was $LASTEXITCODE."
}

Write-Host "[3/15] cli success run (channel_minimal.json)"
& $exePath --cli --config (Join-Path $repoRoot "config\samples\channel_minimal.json") | Out-Host
if ($LASTEXITCODE -ne 0) {
    throw "Expected success for channel_minimal.json, but exit code was $LASTEXITCODE."
}

Write-Host "[4/15] cli success run (channel_full.json)"
& $exePath --cli --config (Join-Path $repoRoot "config\samples\channel_full.json") | Out-Host
if ($LASTEXITCODE -ne 0) {
    throw "Expected success for channel_full.json, but exit code was $LASTEXITCODE."
}

Write-Host "[5/15] cli success run (mix_all_mute.json)"
$mixAllMutePath = Join-Path $repoRoot "config\samples\mix_all_mute.json"
$mixOut = & $exePath --cli --config $mixAllMutePath | Out-String
$mixOut | Out-Host
if ($LASTEXITCODE -ne 0) {
    throw "Expected success for mix_all_mute.json, but exit code was $LASTEXITCODE."
}
if ($mixOut -notmatch "\[RenderStats\].*nonZero=0/") {
    throw "Expected nonZero=0 render stats for mix_all_mute.json."
}

Write-Host "[6/15] cli success run (preset: basic_wave)"
$basicOut = & $exePath --cli --preset basic_wave | Out-String
$basicOut | Out-Host
if ($LASTEXITCODE -ne 0) {
    throw "Expected success for preset basic_wave, but exit code was $LASTEXITCODE."
}
if ($basicOut -notmatch "\[RenderStats\].*nonZero=[1-9]") {
    throw "Expected non-zero render stats for preset basic_wave."
}

Write-Host "[7/15] cli success run (preset: fm_default)"
$fmOut = & $exePath --cli --preset fm_default | Out-String
$fmOut | Out-Host
if ($LASTEXITCODE -ne 0) {
    throw "Expected success for preset fm_default, but exit code was $LASTEXITCODE."
}
if ($fmOut -notmatch "\[RenderStats\].*nonZero=[1-9]") {
    throw "Expected non-zero render stats for preset fm_default."
}

Write-Host "[8/15] cli failure run (channel_mix_invalid.json)"
& $exePath --cli --config (Join-Path $repoRoot "config\samples\channel_mix_invalid.json") | Out-Host
if ($LASTEXITCODE -eq 0) {
    throw "Expected failure for channel_mix_invalid.json, but exit code was 0."
}

Write-Host "[9/15] cli failure run (missing config)"
& $exePath --cli --config (Join-Path $repoRoot "config\__missing__.json") | Out-Host
if ($LASTEXITCODE -eq 0) {
    throw "Expected failure for missing config, but exit code was 0."
}

Write-Host "[10/15] cli failure run (channel_invalid.json)"
& $exePath --cli --config (Join-Path $repoRoot "config\samples\channel_invalid.json") | Out-Host
if ($LASTEXITCODE -eq 0) {
    throw "Expected failure for channel_invalid.json, but exit code was 0."
}

Write-Host "[11/15] cli success run (japanese path config/output)"
$jpDir = Join-Path $repoRoot "output\日本語\スモーク"
New-Item -ItemType Directory -Path $jpDir -Force | Out-Null
$jpConfigPath = Join-Path $jpDir "jp_path_smoke.json"
$jpOutPath = Join-Path $jpDir "結果.wav"
$midiPath = (Join-Path $repoRoot "assets\midi\solstice_intro.mid") -replace "\\", "/"
$wavPath = ($jpOutPath -replace "\\", "/")
$jpConfig = @"
{
  "midiPath": "$midiPath",
  "wavPath": "$wavPath",
  "targetChannel": -1,
  "defaultWave": "saw",
  "initialSeconds": 2,
  "bits": 16,
  "sampleRate": 44100,
  "extraReleaseSec": 0.1
}
"@
Set-Content -Path $jpConfigPath -Value $jpConfig -Encoding UTF8
& $exePath --cli --config $jpConfigPath | Out-Host
if ($LASTEXITCODE -ne 0) {
    throw "Expected success for japanese path smoke, but exit code was $LASTEXITCODE."
}
if (-not (Test-Path $jpOutPath)) {
    throw "Japanese path smoke did not produce output wav: $jpOutPath"
}

Write-Host "[12/15] cli success run (single channel export path)"
$singleConfigPath = Join-Path $repoRoot "output\single_channel_smoke.json"
$singleOutPath = Join-Path $repoRoot "output\single_channel_smoke.wav"
$singleConfig = @"
{
  "midiPath": "../assets/midi/solstice_intro.mid",
  "wavPath": "../output/single_channel_smoke.wav",
  "targetChannel": 0,
  "defaultWave": "saw",
  "initialSeconds": 2,
  "bits": 16,
  "sampleRate": 44100,
  "extraReleaseSec": 0.1
}
"@
Set-Content -Path $singleConfigPath -Value $singleConfig -Encoding UTF8
$singleOut = & $exePath --cli --config $singleConfigPath | Out-String
$singleOut | Out-Host
if ($LASTEXITCODE -ne 0) {
    throw "Expected success for single channel smoke, but exit code was $LASTEXITCODE."
}
if (-not (Test-Path $singleOutPath)) {
    throw "Single channel smoke did not produce output wav: $singleOutPath"
}

Write-Host "[13/15] gui launch smoke (default mode, 2 sec)"
$p = Start-Process -FilePath $exePath -PassThru
Start-Sleep -Seconds 2
if (-not $p.HasExited) {
    Stop-Process -Id $p.Id -Force
    Write-Host "GUI process (default mode) launched successfully."
}
else {
    throw "GUI process exited unexpectedly with code $($p.ExitCode)"
}

Write-Host "[14/15] gui launch smoke (--gui explicit, 2 sec)"
$p = Start-Process -FilePath $exePath -ArgumentList "--gui" -PassThru
Start-Sleep -Seconds 2
if (-not $p.HasExited) {
    Stop-Process -Id $p.Id -Force
    Write-Host "GUI process (--gui) launched successfully."
}
else {
    throw "GUI process (--gui) exited unexpectedly with code $($p.ExitCode)"
}

Write-Host "[15/15] acceptance handoff note"
Write-Host "Manual acceptance for v7 Sound/Music flow and Save/Error UX: docs/GUI_V7_ACCEPTANCE_TEST.md"

Write-Host "GUI smoke test completed."

if (Test-Path $jpDir) {
    Remove-Item -Path $jpDir -Recurse -Force -ErrorAction SilentlyContinue
}
if (Test-Path $singleConfigPath) {
    Remove-Item -Path $singleConfigPath -Force -ErrorAction SilentlyContinue
}
if (Test-Path $singleOutPath) {
    Remove-Item -Path $singleOutPath -Force -ErrorAction SilentlyContinue
}
