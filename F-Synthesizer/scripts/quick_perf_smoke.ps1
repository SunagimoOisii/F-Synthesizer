param(
    [int]$Iterations = 5,
    [string]$Preset = "",
    [string]$Configuration = "Release",
    [string]$Platform = "x64",
    [int]$TimeoutSec = 20,
    [string]$OutputDir = "",
    [string]$ResultJson = "",
    [string]$ResultCsv = "",
    [switch]$NoResultFiles,
    [switch]$SkipBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
if (Get-Variable -Name PSNativeCommandUseErrorActionPreference -ErrorAction SilentlyContinue) {
    $PSNativeCommandUseErrorActionPreference = $false
}

function Get-MsbuildPath {
    $cmd = Get-Command msbuild -ErrorAction SilentlyContinue
    if ($null -ne $cmd) {
        return $cmd.Source
    }

    $vswhere = "C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
    if (Test-Path $vswhere) {
        $found = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild/**/Bin/MSBuild.exe | Select-Object -First 1
        if ($found) {
            return $found
        }
    }

    $fallbacks = @(
        "C:/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe",
        "C:/Program Files/Microsoft Visual Studio/2022/Professional/MSBuild/Current/Bin/MSBuild.exe",
        "C:/Program Files/Microsoft Visual Studio/2022/BuildTools/MSBuild/Current/Bin/MSBuild.exe"
    )
    foreach ($path in $fallbacks) {
        if (Test-Path $path) {
            return $path
        }
    }

    return $null
}

function Resolve-ExePath {
    param(
        [string]$RepoRoot,
        [string]$Platform,
        [string]$Configuration
    )

    $candidates = @(
        (Join-Path $RepoRoot "build/$Platform/$Configuration/F-Synthesizer.exe"),
        (Join-Path $RepoRoot "$Platform/$Configuration/F-Synthesizer.exe")
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    return $candidates[0]
}

function Ensure-RuntimeDependencies {
    param(
        [string]$ExePath
    )

    $exeDir = Split-Path -Parent $ExePath
    $glfwPath = Join-Path $exeDir "glfw3.dll"
    if (Test-Path $glfwPath) {
        return
    }

    $vcpkgRoot = "C:/vcpkg/installed/x64-windows"
    $sourceCandidates = @(
        (Join-Path $vcpkgRoot "bin/glfw3.dll"),
        (Join-Path $vcpkgRoot "debug/bin/glfw3.dll")
    )
    foreach ($src in $sourceCandidates) {
        if (Test-Path $src) {
            Copy-Item -Path $src -Destination $glfwPath -Force
            Write-Host "Runtime dependency restored: $glfwPath"
            return
        }
    }
}

function Write-BytesFile {
    param(
        [string]$Path,
        [byte[]]$Bytes
    )

    [System.IO.File]::WriteAllBytes($Path, $Bytes)
}

function New-MIDIFileBytes {
    param(
        [byte[]]$TrackData
    )

    $header = [byte[]](
        0x4D, 0x54, 0x68, 0x64, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x01, 0x00, 0x60
    )
    $len = [uint32]$TrackData.Length
    $trackHeader = [byte[]](
        0x4D, 0x54, 0x72, 0x6B,
        [byte](($len -shr 24) -band 0xFF),
        [byte](($len -shr 16) -band 0xFF),
        [byte](($len -shr 8) -band 0xFF),
        [byte]($len -band 0xFF)
    )
    return ($header + $trackHeader + $TrackData)
}

function New-QuickSmokeMidi {
    param(
        [string]$Path
    )

    $track = [byte[]](
        0x00,0x90,0x3C,0x64,
        0x00,0x99,0x24,0x6E,
        0x30,0x80,0x3C,0x00,
        0x00,0x89,0x24,0x00,
        0x00,0x99,0x26,0x64,
        0x30,0x89,0x26,0x00,
        0x00,0xFF,0x2F,0x00
    )
    Write-BytesFile -Path $Path -Bytes (New-MIDIFileBytes -TrackData $track)
}

function New-QuickPresetConfig {
    param(
        [string]$RepoRoot,
        [string]$PresetName,
        [string]$MidiPath,
        [string]$WavPath,
        [string]$ConfigPath
    )

    $presetPath = Join-Path $RepoRoot "config/presets/$PresetName.json"
    if (-not (Test-Path $presetPath)) {
        throw "Preset config not found: $presetPath"
    }

    $presetJson = Get-Content -Path $presetPath -Raw | ConvertFrom-Json
    if ($null -eq $presetJson.project -or $null -eq $presetJson.project.instruments -or $null -eq $presetJson.project.channels) {
        throw "Preset has no project.instruments/project.channels: $presetPath"
    }

    $project = [ordered]@{
        midiPath = ($MidiPath -replace "\\", "/")
        wavPath = ($WavPath -replace "\\", "/")
        targetChannel = -1
        initialSeconds = 1
        bits = 16
        sampleRate = 44100
        extraReleaseSec = 0.15
        instruments = $presetJson.project.instruments
        channels = $presetJson.project.channels
    }
    if ($presetJson.project.PSObject.Properties.Name -contains "effects") {
        $project.effects = $presetJson.project.effects
    }

    $config = [ordered]@{
        format = "projectModel.v3"
        project = $project
    }
    $config | ConvertTo-Json -Depth 100 | Set-Content -Path $ConfigPath -Encoding UTF8
}

function Invoke-Render {
    param(
        [string]$ExePath,
        [string]$ConfigPath,
        [int]$TimeoutSec
    )

    $tempBase = [System.IO.Path]::Combine([System.IO.Path]::GetTempPath(), [System.IO.Path]::GetRandomFileName())
    $stdoutPath = "$tempBase.out"
    $stderrPath = "$tempBase.err"
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $p = Start-Process -FilePath $ExePath -ArgumentList @("--cli", "--config", $ConfigPath) -NoNewWindow -PassThru -RedirectStandardOutput $stdoutPath -RedirectStandardError $stderrPath
    if (-not $p.WaitForExit($TimeoutSec * 1000)) {
        Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
        throw "Render timed out after ${TimeoutSec}s: $ConfigPath"
    }
    $sw.Stop()

    $out = ""
    if (Test-Path $stdoutPath) {
        $out += Get-Content -Path $stdoutPath -Raw -ErrorAction SilentlyContinue
    }
    if (Test-Path $stderrPath) {
        $err = Get-Content -Path $stderrPath -Raw -ErrorAction SilentlyContinue
        if ($err) {
            $out += "`r`n$err"
        }
    }
    Remove-Item -Path $stdoutPath, $stderrPath -Force -ErrorAction SilentlyContinue

    if ($p.ExitCode -ne 0) {
        Write-Host $out
        throw "Render failed with exit code $($p.ExitCode): $ConfigPath"
    }

    $stats = ($out -split "`r?`n" | Where-Object { $_ -match '\[RenderStats\]' } | Select-Object -Last 1)
    if (-not $stats) {
        Write-Host $out
        throw "RenderStats not found in output: $ConfigPath"
    }

    return [PSCustomObject]@{
        Ms = $sw.Elapsed.TotalMilliseconds
        Stats = $stats
    }
}

if ($Iterations -le 0) {
    throw "Iterations must be positive."
}

$repoRoot = Split-Path -Parent $PSScriptRoot
$presets = @("sound_lead_blade", "sound_bass_chip", "sound_drums_arcade")
if ($Preset) {
    $presets = @($Preset)
}

Push-Location $repoRoot
try {
    $exePath = Resolve-ExePath -RepoRoot $repoRoot -Platform $Platform -Configuration $Configuration
    if ((-not $SkipBuild) -or (-not (Test-Path $exePath))) {
        $msbuildPath = Get-MsbuildPath
        if (-not $msbuildPath) {
            throw "MSBuild not found. Install Visual Studio Build Tools or add MSBuild to PATH."
        }
        & $msbuildPath "F-Synthesizer.vcxproj" /t:Build "/p:Configuration=$Configuration;Platform=$Platform" /m /nologo
        if ($LASTEXITCODE -ne 0) {
            throw "Build failed with exit code $LASTEXITCODE."
        }
        $exePath = Resolve-ExePath -RepoRoot $repoRoot -Platform $Platform -Configuration $Configuration
    }
    if (-not (Test-Path $exePath)) {
        throw "Executable not found: $exePath"
    }
    Ensure-RuntimeDependencies -ExePath $exePath

    if ($OutputDir) {
        if ([System.IO.Path]::IsPathRooted($OutputDir)) {
            $smokeDir = $OutputDir
        }
        else {
            $smokeDir = Join-Path $repoRoot $OutputDir
        }
    }
    else {
        $smokeDir = Join-Path $repoRoot "output/check/quick_perf_smoke"
    }
    New-Item -ItemType Directory -Path $smokeDir -Force | Out-Null
    $midiPath = Join-Path $smokeDir "quick_perf.mid"
    New-QuickSmokeMidi -Path $midiPath

    Write-Host "== F-Synthesizer quick perf smoke =="
    Write-Host "Config: $Configuration | Platform: $Platform | Iterations: $Iterations"
    $startedAt = (Get-Date).ToString("o")
    $runRows = @()
    $summaryRows = @()
    foreach ($presetName in $presets) {
        $safeName = $presetName -replace '[^A-Za-z0-9_.-]', '_'
        $configPath = Join-Path $smokeDir "$safeName.json"
        $wavPath = Join-Path $smokeDir "$safeName.wav"
        New-QuickPresetConfig -RepoRoot $repoRoot -PresetName $presetName -MidiPath $midiPath -WavPath $wavPath -ConfigPath $configPath

        $results = @()
        for ($i = 1; $i -le $Iterations; $i++) {
            $result = Invoke-Render -ExePath $exePath -ConfigPath $configPath -TimeoutSec $TimeoutSec
            if (-not (Test-Path $wavPath)) {
                throw "Rendered WAV was not found: $wavPath"
            }
            $results += $result
            $runRows += [PSCustomObject]@{
                preset = $presetName
                iteration = $i
                milliseconds = [Math]::Round($result.Ms, 4)
                renderStats = $result.Stats
                wavPath = $wavPath
            }
            Write-Host ("{0} run {1}/{2}: {3:N2} ms | {4}" -f $presetName, $i, $Iterations, $result.Ms, $result.Stats)
        }
        $avg = ($results | Measure-Object -Property Ms -Average).Average
        $min = ($results | Measure-Object -Property Ms -Minimum).Minimum
        $max = ($results | Measure-Object -Property Ms -Maximum).Maximum
        $summaryRows += [PSCustomObject]@{
            preset = $presetName
            iterations = $Iterations
            averageMs = [Math]::Round($avg, 4)
            minMs = [Math]::Round($min, 4)
            maxMs = [Math]::Round($max, 4)
            renderStats = ($results | Select-Object -Last 1).Stats
            wavPath = $wavPath
        }
        Write-Host ("{0} summary: avg={1:N2} ms min={2:N2} ms max={3:N2} ms wav={4}" -f $presetName, $avg, $min, $max, $wavPath)
    }
    if (-not $NoResultFiles) {
        if (-not $ResultJson) {
            $ResultJson = Join-Path $smokeDir "quick_perf_smoke_results.json"
        }
        if (-not $ResultCsv) {
            $ResultCsv = Join-Path $smokeDir "quick_perf_smoke_summary.csv"
        }
        foreach ($path in @($ResultJson, $ResultCsv)) {
            $parent = Split-Path -Parent $path
            if ($parent) {
                New-Item -ItemType Directory -Path $parent -Force | Out-Null
            }
        }
        $resultDoc = [ordered]@{
            tool = "quick_perf_smoke"
            startedAt = $startedAt
            completedAt = (Get-Date).ToString("o")
            configuration = $Configuration
            platform = $Platform
            iterations = $Iterations
            executable = $exePath
            presets = $presets
            summaries = $summaryRows
            runs = $runRows
        }
        $resultDoc | ConvertTo-Json -Depth 8 | Set-Content -Path $ResultJson -Encoding UTF8
        $summaryRows | Export-Csv -Path $ResultCsv -NoTypeInformation -Encoding UTF8
        Write-Host "Result JSON: $ResultJson"
        Write-Host "Result CSV: $ResultCsv"
    }
    Write-Host "Quick perf smoke completed."
}
finally {
    Pop-Location
}
