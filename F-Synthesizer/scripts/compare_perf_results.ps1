param(
    [Parameter(Mandatory = $true)]
    [string]$BaselineJson,
    [Parameter(Mandatory = $true)]
    [string]$CandidateJson,
    [string]$OutputCsv = "",
    [switch]$IncludeWavDiff
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Read-PerfResult {
    param([string]$Path)

    if (-not (Test-Path $Path)) {
        throw "Perf result JSON not found: $Path"
    }
    return Get-Content -Path $Path -Raw | ConvertFrom-Json
}

function Parse-RenderStats {
    param([string]$Line)

    if ($Line -notmatch 'peak=([0-9.eE+-]+)\s+rms=([0-9.eE+-]+)\s+nonZero=([0-9]+)/([0-9]+)') {
        return $null
    }
    return [PSCustomObject]@{
        peak = [double]$Matches[1]
        rms = [double]$Matches[2]
        nonZero = [int]$Matches[3]
        total = [int]$Matches[4]
    }
}

function Get-SummaryMap {
    param($Doc)

    $map = @{}
    foreach ($summary in @($Doc.summaries)) {
        $map[$summary.preset] = $summary
    }
    return $map
}

function Read-Ascii {
    param(
        [System.IO.BinaryReader]$Reader,
        [int]$Count
    )

    return [System.Text.Encoding]::ASCII.GetString($Reader.ReadBytes($Count))
}

function Read-WavPcmSamples {
    param([string]$Path)

    if (-not (Test-Path $Path)) {
        throw "WAV not found: $Path"
    }

    $stream = [System.IO.File]::OpenRead($Path)
    try {
        $reader = [System.IO.BinaryReader]::new($stream)
        if ((Read-Ascii -Reader $reader -Count 4) -ne "RIFF") {
            throw "Not a RIFF file: $Path"
        }
        [void]$reader.ReadUInt32()
        if ((Read-Ascii -Reader $reader -Count 4) -ne "WAVE") {
            throw "Not a WAVE file: $Path"
        }

        $audioFormat = 0
        $channels = 0
        $bitsPerSample = 0
        $data = $null
        while ($stream.Position -lt $stream.Length) {
            $chunkId = Read-Ascii -Reader $reader -Count 4
            $chunkSize = [int]$reader.ReadUInt32()
            $chunkStart = $stream.Position
            if ($chunkId -eq "fmt ") {
                $audioFormat = [int]$reader.ReadUInt16()
                $channels = [int]$reader.ReadUInt16()
                [void]$reader.ReadUInt32()
                [void]$reader.ReadUInt32()
                [void]$reader.ReadUInt16()
                $bitsPerSample = [int]$reader.ReadUInt16()
            }
            elseif ($chunkId -eq "data") {
                $data = $reader.ReadBytes($chunkSize)
            }
            $stream.Position = $chunkStart + $chunkSize
            if (($chunkSize % 2) -ne 0 -and $stream.Position -lt $stream.Length) {
                $stream.Position++
            }
        }

        if ($audioFormat -ne 1) {
            throw "Only PCM WAV is supported: $Path"
        }
        if ($channels -le 0 -or ($bitsPerSample -ne 16 -and $bitsPerSample -ne 24)) {
            throw "Unsupported WAV format: channels=$channels bits=$bitsPerSample path=$Path"
        }
        if ($null -eq $data) {
            throw "WAV data chunk not found: $Path"
        }

        $bytesPerSample = [int]($bitsPerSample / 8)
        $sampleCount = [int]($data.Length / $bytesPerSample)
        $samples = New-Object double[] $sampleCount
        for ($i = 0; $i -lt $sampleCount; $i++) {
            $offset = $i * $bytesPerSample
            if ($bitsPerSample -eq 16) {
                $v = [System.BitConverter]::ToInt16($data, $offset)
                $samples[$i] = $v / 32768.0
            }
            else {
                $v = [int]$data[$offset] -bor ([int]$data[$offset + 1] -shl 8) -bor ([int]$data[$offset + 2] -shl 16)
                if (($v -band 0x800000) -ne 0) {
                    $v = $v -bor -16777216
                }
                $samples[$i] = $v / 8388608.0
            }
        }
        return $samples
    }
    finally {
        $stream.Dispose()
    }
}

function Compare-WavPcm {
    param(
        [string]$BaselinePath,
        [string]$CandidatePath
    )

    $a = Read-WavPcmSamples -Path $BaselinePath
    $b = Read-WavPcmSamples -Path $CandidatePath
    if ($a.Length -ne $b.Length) {
        throw "WAV sample count mismatch: $BaselinePath ($($a.Length)) vs $CandidatePath ($($b.Length))"
    }

    $maxAbs = 0.0
    $sumSq = 0.0
    for ($i = 0; $i -lt $a.Length; $i++) {
        $diff = $b[$i] - $a[$i]
        $abs = [Math]::Abs($diff)
        if ($abs -gt $maxAbs) {
            $maxAbs = $abs
        }
        $sumSq += $diff * $diff
    }
    return [PSCustomObject]@{
        maxAbsDiff = $maxAbs
        rmsDiff = [Math]::Sqrt($sumSq / [Math]::Max(1, $a.Length))
        samples = $a.Length
    }
}

$baseline = Read-PerfResult -Path $BaselineJson
$candidate = Read-PerfResult -Path $CandidateJson
$baselineMap = Get-SummaryMap -Doc $baseline
$candidateMap = Get-SummaryMap -Doc $candidate

$rows = @()
foreach ($preset in ($candidateMap.Keys | Sort-Object)) {
    if (-not $baselineMap.ContainsKey($preset)) {
        throw "Preset not found in baseline: $preset"
    }

    $baseSummary = $baselineMap[$preset]
    $candSummary = $candidateMap[$preset]
    $baseStats = Parse-RenderStats -Line $baseSummary.renderStats
    $candStats = Parse-RenderStats -Line $candSummary.renderStats
    $baseAvg = [double]$baseSummary.averageMs
    $candAvg = [double]$candSummary.averageMs
    $deltaMs = $candAvg - $baseAvg
    $deltaPct = if ($baseAvg -ne 0.0) { ($deltaMs / $baseAvg) * 100.0 } else { 0.0 }

    $wavMax = $null
    $wavRms = $null
    if ($IncludeWavDiff) {
        $wav = Compare-WavPcm -BaselinePath $baseSummary.wavPath -CandidatePath $candSummary.wavPath
        $wavMax = $wav.maxAbsDiff
        $wavRms = $wav.rmsDiff
    }

    $statsMatch = $false
    if ($null -ne $baseStats -and $null -ne $candStats) {
        $statsMatch =
            $baseStats.peak -eq $candStats.peak -and
            $baseStats.rms -eq $candStats.rms -and
            $baseStats.nonZero -eq $candStats.nonZero -and
            $baseStats.total -eq $candStats.total
    }

    $rows += [PSCustomObject]@{
        preset = $preset
        baselineAvgMs = [Math]::Round($baseAvg, 4)
        candidateAvgMs = [Math]::Round($candAvg, 4)
        deltaMs = [Math]::Round($deltaMs, 4)
        deltaPct = [Math]::Round($deltaPct, 2)
        statsMatch = $statsMatch
        maxAbsDiff = $wavMax
        rmsDiff = $wavRms
    }
}

Write-Host "== F-Synthesizer perf result comparison =="
Write-Host "Baseline:  $BaselineJson"
Write-Host "Candidate: $CandidateJson"
$rows | Format-Table -AutoSize
if ($OutputCsv) {
    $parent = Split-Path -Parent $OutputCsv
    if ($parent) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
    $rows | Export-Csv -Path $OutputCsv -NoTypeInformation -Encoding UTF8
    Write-Host "Result CSV: $OutputCsv"
}
