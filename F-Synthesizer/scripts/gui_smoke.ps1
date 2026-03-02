param(
    [string]$Configuration = "Debug",
    [string]$Platform = "x64",
    [ValidateSet("quick", "full")]
    [string]$Profile = "quick"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$exePath = Join-Path $repoRoot "build\$Platform\$Configuration\F-Synthesizer.exe"
$quickDir = Join-Path $repoRoot "output\smoke_quick"
$jpDir = Join-Path $repoRoot "output\日本語\スモーク"
$singleConfigPath = Join-Path $repoRoot "output\single_channel_smoke.json"
$singleOutPath = Join-Path $repoRoot "output\single_channel_smoke.wav"

if (-not (Test-Path $exePath)) {
    throw "Executable not found: $exePath"
}

$step = 0
$total = if ($Profile -eq "full") { 15 } else { 6 }
function Write-Step([string]$label) {
    $script:step++
    Write-Host "[$($script:step)/$($script:total)] $label"
}

function Invoke-GuiLaunchSmoke([string[]]$args, [string]$label) {
    Write-Step $label
    $p = $null
    if ($null -eq $args -or $args.Count -eq 0) {
        $p = Start-Process -FilePath $exePath -PassThru
    }
    else {
        $p = Start-Process -FilePath $exePath -ArgumentList $args -PassThru
    }
    Start-Sleep -Seconds 2
    if (-not $p.HasExited) {
        Stop-Process -Id $p.Id -Force
        return
    }
    throw "GUI process exited unexpectedly with code $($p.ExitCode)"
}

try {
    Write-Host "== GUI smoke test =="
    Write-Host "Exe: $exePath"
    Write-Host "Profile: $Profile"

    Write-Step "help output"
    & $exePath --help | Out-Host

    Write-Step "cli success run (quick_smoke.json)"
    New-Item -ItemType Directory -Path $quickDir -Force | Out-Null
    $quickConfigPath = Join-Path $quickDir "quick_smoke.json"
    $quickOutPath = Join-Path $quickDir "quick_smoke.wav"
    $quickMidiPath = (Join-Path $repoRoot "assets\midi\logo.mid") -replace "\\", "/"
    $quickWavPath = ($quickOutPath -replace "\\", "/")
    $quickConfig = @"
{
  "midiPath": "$quickMidiPath",
  "wavPath": "$quickWavPath",
  "targetChannel": 0,
  "defaultWave": "saw",
  "initialSeconds": 1,
  "bits": 16,
  "sampleRate": 44100,
  "extraReleaseSec": 0.05
}
"@
    Set-Content -Path $quickConfigPath -Value $quickConfig -Encoding UTF8
    $quickOut = & $exePath --cli --config $quickConfigPath | Out-String
    $quickOut | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw "Expected success for quick_smoke.json, but exit code was $LASTEXITCODE."
    }
    if ($quickOut -notmatch "\[RenderStats\].*nonZero=[1-9]") {
        throw "Expected non-zero render stats for quick_smoke.json."
    }
    if (-not (Test-Path $quickOutPath)) {
        throw "Quick smoke did not produce output wav: $quickOutPath"
    }

    if ($Profile -eq "full") {
        Write-Step "cli success run (channel_minimal.json)"
        & $exePath --cli --config (Join-Path $repoRoot "config\samples\channel_minimal.json") | Out-Host
        if ($LASTEXITCODE -ne 0) {
            throw "Expected success for channel_minimal.json, but exit code was $LASTEXITCODE."
        }

        Write-Step "cli success run (channel_full.json)"
        & $exePath --cli --config (Join-Path $repoRoot "config\samples\channel_full.json") | Out-Host
        if ($LASTEXITCODE -ne 0) {
            throw "Expected success for channel_full.json, but exit code was $LASTEXITCODE."
        }

        Write-Step "cli success run (mix_all_mute.json)"
        $mixAllMutePath = Join-Path $repoRoot "config\samples\mix_all_mute.json"
        $mixOut = & $exePath --cli --config $mixAllMutePath | Out-String
        $mixOut | Out-Host
        if ($LASTEXITCODE -ne 0) {
            throw "Expected success for mix_all_mute.json, but exit code was $LASTEXITCODE."
        }
        if ($mixOut -notmatch "\[RenderStats\].*nonZero=0/") {
            throw "Expected nonZero=0 render stats for mix_all_mute.json."
        }

        Write-Step "cli success run (preset: wave_lead_soft)"
        $leadOut = & $exePath --cli --preset wave_lead_soft | Out-String
        $leadOut | Out-Host
        if ($LASTEXITCODE -ne 0) {
            throw "Expected success for preset wave_lead_soft, but exit code was $LASTEXITCODE."
        }
        if ($leadOut -notmatch "\[RenderStats\].*nonZero=[1-9]") {
            throw "Expected non-zero render stats for preset wave_lead_soft."
        }

        Write-Step "cli success run (preset: wave_bass_solid)"
        $bassOut = & $exePath --cli --preset wave_bass_solid | Out-String
        $bassOut | Out-Host
        if ($LASTEXITCODE -ne 0) {
            throw "Expected success for preset wave_bass_solid, but exit code was $LASTEXITCODE."
        }
        if ($bassOut -notmatch "\[RenderStats\].*nonZero=[1-9]") {
            throw "Expected non-zero render stats for preset wave_bass_solid."
        }

        Write-Step "cli failure run (channel_mix_invalid.json)"
        & $exePath --cli --config (Join-Path $repoRoot "config\samples\channel_mix_invalid.json") | Out-Host
        if ($LASTEXITCODE -eq 0) {
            throw "Expected failure for channel_mix_invalid.json, but exit code was 0."
        }

        Write-Step "cli failure run (missing config)"
        & $exePath --cli --config (Join-Path $repoRoot "config\__missing__.json") | Out-Host
        if ($LASTEXITCODE -eq 0) {
            throw "Expected failure for missing config, but exit code was 0."
        }

        Write-Step "cli failure run (channel_invalid.json)"
        & $exePath --cli --config (Join-Path $repoRoot "config\samples\channel_invalid.json") | Out-Host
        if ($LASTEXITCODE -eq 0) {
            throw "Expected failure for channel_invalid.json, but exit code was 0."
        }

        Write-Step "cli success run (japanese path config/output)"
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

        Write-Step "cli success run (single channel export path)"
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
    }
    else {
        Write-Step "cli failure run (missing config)"
        & $exePath --cli --config (Join-Path $repoRoot "config\__missing__.json") | Out-Host
        if ($LASTEXITCODE -eq 0) {
            throw "Expected failure for missing config, but exit code was 0."
        }
    }

    Invoke-GuiLaunchSmoke @() "gui launch smoke (default mode, 2 sec)"
    Invoke-GuiLaunchSmoke @("--gui") "gui launch smoke (--gui explicit, 2 sec)"

    Write-Step "acceptance handoff note"
    Write-Host "Manual acceptance for v8 Sound/Music flow and Save/Error UX: docs/GUI_V8_ACCEPTANCE_TEST.md"
    Write-Host "GUI smoke test completed."
}
finally {
    if (Test-Path $quickDir) {
        Remove-Item -Path $quickDir -Recurse -Force -ErrorAction SilentlyContinue
    }
    if (Test-Path $jpDir) {
        Remove-Item -Path $jpDir -Recurse -Force -ErrorAction SilentlyContinue
    }
    if (Test-Path $singleConfigPath) {
        Remove-Item -Path $singleConfigPath -Force -ErrorAction SilentlyContinue
    }
    if (Test-Path $singleOutPath) {
        Remove-Item -Path $singleOutPath -Force -ErrorAction SilentlyContinue
    }
}
