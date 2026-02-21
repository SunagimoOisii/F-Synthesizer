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

Write-Host "== PianoRoll smoke =="
Write-Host "1) 既存スモークを実行します..."
& powershell -ExecutionPolicy Bypass -File (Join-Path $repoRoot "scripts\gui_smoke.ps1") -Configuration $Configuration -Platform $Platform
if ($LASTEXITCODE -ne 0) {
    throw "gui_smoke.ps1 failed with exit code $LASTEXITCODE"
}

Write-Host "2) 受け入れ手順を表示します..."
$acceptPath = Join-Path $repoRoot "docs\PIANO_ROLL_ACCEPTANCE_TEST.md"
if (-not (Test-Path $acceptPath)) {
    throw "Acceptance document not found: $acceptPath"
}
Get-Content -Path $acceptPath | Out-Host

Write-Host ""
Write-Host "3) GUIを起動します。手動手順に沿って確認してください。"
Write-Host "   終了したらこのウィンドウに戻って Enter を押してください。"

$p = Start-Process -FilePath $exePath -ArgumentList "--gui" -PassThru
Read-Host | Out-Null

if (-not $p.HasExited) {
    Stop-Process -Id $p.Id -Force
}

Write-Host "PianoRoll smoke completed."
