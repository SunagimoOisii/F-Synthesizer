param()

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
Push-Location $repoRoot
try {
    # リポジトリルート（1つ上）から見た hooksPath を設定する。
    git config core.hooksPath "F-Synthesizer/.githooks"
    Write-Host "Configured core.hooksPath = F-Synthesizer/.githooks"
}
finally {
    Pop-Location
}
