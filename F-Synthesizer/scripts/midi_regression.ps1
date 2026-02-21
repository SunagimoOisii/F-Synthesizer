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

$workDir = Join-Path $repoRoot "output\regression_midi"
New-Item -ItemType Directory -Path $workDir -Force | Out-Null

function Write-BytesFile {
    param(
        [string]$Path,
        [byte[]]$Bytes
    )
    [System.IO.File]::WriteAllBytes($Path, $Bytes)
}

function New-MidiFileBytes {
    param(
        [byte[]]$TrackData
    )
    $header = [byte[]](
        0x4D,0x54,0x68,0x64, 0x00,0x00,0x00,0x06, 0x00,0x00, 0x00,0x01, 0x00,0x60
    )
    $len = [uint32]$TrackData.Length
    $trackHeader = [byte[]](
        0x4D,0x54,0x72,0x6B,
        [byte](($len -shr 24) -band 0xFF),
        [byte](($len -shr 16) -band 0xFF),
        [byte](($len -shr 8) -band 0xFF),
        [byte]($len -band 0xFF)
    )
    return ($header + $trackHeader + $TrackData)
}

function Invoke-RunningStatusCase {
    param(
        [string]$CaseName,
        [byte[]]$TrackData
    )
    $midiRel = "$CaseName.mid"
    $wavRel = "$CaseName.wav"
    $configRel = "output/regression_midi/$CaseName.json"
    $midiPath = Join-Path $workDir "$CaseName.mid"
    $wavPath = Join-Path $workDir "$CaseName.wav"
    $configPath = Join-Path $repoRoot $configRel

    $allBytes = New-MidiFileBytes -TrackData $TrackData
    Write-BytesFile -Path $midiPath -Bytes $allBytes

    $configJson = @"
{
  "midiPath": "$midiRel",
  "wavPath": "$wavRel",
  "targetChannel": -1,
  "defaultWave": "sine",
  "initialSeconds": 2,
  "bits": 16,
  "sampleRate": 44100,
  "extraReleaseSec": 0.05
}
"@
    Set-Content -Path $configPath -Value $configJson -Encoding UTF8

    Write-Host "[Case] $CaseName"
    $output = & $exePath --cli --config $configRel 2>&1 | Out-String
    $output | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw "Case '$CaseName' failed with exit code $LASTEXITCODE"
    }
    if ($output -notmatch "Event Counts: note=1,") {
        throw "Case '$CaseName' expected note=1 after running status clear."
    }
}

Write-Host "== MIDI running status regression =="
Write-Host "Exe: $exePath"

# delta0 NoteOn(0x90 60 100), delta0 Meta text, delta0 raw data bytes, delta0 EOT
$trackMeta = [byte[]](0x00,0x90,0x3C,0x64, 0x00,0xFF,0x01,0x01,0x41, 0x00,0x3E,0x00, 0x00,0xFF,0x2F,0x00)
Invoke-RunningStatusCase -CaseName "running_status_after_meta" -TrackData $trackMeta

# delta0 NoteOn, delta0 SysEx, delta0 raw data bytes, delta0 EOT
$trackSysEx = [byte[]](0x00,0x90,0x3C,0x64, 0x00,0xF0,0x01,0x7F, 0x00,0x3E,0x00, 0x00,0xFF,0x2F,0x00)
Invoke-RunningStatusCase -CaseName "running_status_after_sysex" -TrackData $trackSysEx

Write-Host "MIDI running status regression completed."
