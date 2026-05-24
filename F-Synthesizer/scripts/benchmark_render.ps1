param(
    [string]$Configuration = "Release",
    [string]$Platform = "x64",
    [string]$Preset = "sound_lead_blade",
    [string]$ConfigPath = "",
    [int]$Iterations = 3,
    [int]$TimeoutSec = 60,
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

function Invoke-Render {
    param(
        [string]$ExePath,
        [string[]]$CliArgs,
        [int]$TimeoutSec
    )

    $tempBase = [System.IO.Path]::Combine([System.IO.Path]::GetTempPath(), [System.IO.Path]::GetRandomFileName())
    $stdoutPath = "$tempBase.out"
    $stderrPath = "$tempBase.err"
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $p = Start-Process -FilePath $ExePath -ArgumentList $CliArgs -NoNewWindow -PassThru -RedirectStandardOutput $stdoutPath -RedirectStandardError $stderrPath
    if (-not $p.WaitForExit($TimeoutSec * 1000)) {
        Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
        throw "Render timed out after ${TimeoutSec}s: $($CliArgs -join ' ')"
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
        throw "Render failed with exit code $($p.ExitCode)."
    }

    $stats = ($out -split "`r?`n" | Where-Object { $_ -match '\[RenderStats\]' } | Select-Object -Last 1)
    if (-not $stats) {
        Write-Host $out
        throw "RenderStats not found in output."
    }

    $outputLine = ($out -split "`r?`n" | Where-Object { $_ -like "Output Path:*" } | Select-Object -Last 1)
    $outputPath = ""
    if ($outputLine) {
        $outputPath = $outputLine.Substring("Output Path:".Length).Trim()
        if ($outputPath -and -not (Test-Path $outputPath)) {
            throw "Rendered WAV was not found: $outputPath"
        }
    }

    return [PSCustomObject]@{
        Ms = $sw.Elapsed.TotalMilliseconds
        Stats = $stats
        OutputPath = $outputPath
    }
}

if ($Iterations -le 0) {
    throw "Iterations must be positive."
}
if ($ConfigPath -and $Preset -ne "sound_lead_blade") {
    throw "Use either -ConfigPath or -Preset, not both."
}

$repoRoot = Split-Path -Parent $PSScriptRoot
Push-Location $repoRoot
try {
    if (-not $SkipBuild) {
        $msbuildPath = Get-MsbuildPath
        if (-not $msbuildPath) {
            throw "MSBuild not found. Install Visual Studio Build Tools or add MSBuild to PATH."
        }
        & $msbuildPath "F-Synthesizer.vcxproj" /t:Build "/p:Configuration=$Configuration;Platform=$Platform" /m /nologo
        if ($LASTEXITCODE -ne 0) {
            throw "Build failed with exit code $LASTEXITCODE."
        }
    }

    $exePath = Resolve-ExePath -RepoRoot $repoRoot -Platform $Platform -Configuration $Configuration
    if (-not (Test-Path $exePath)) {
        throw "Executable not found: $exePath"
    }

    $cliArgs = @("--cli")
    if ($ConfigPath) {
        $cliArgs += @("--config", $ConfigPath)
    }
    else {
        $cliArgs += @("--preset", $Preset)
    }

    $results = @()
    for ($i = 1; $i -le $Iterations; $i++) {
        $result = Invoke-Render -ExePath $exePath -CliArgs $cliArgs -TimeoutSec $TimeoutSec
        $results += $result
        Write-Host ("Run {0}/{1}: {2:N2} ms | {3}" -f $i, $Iterations, $result.Ms, $result.Stats)
    }

    $avg = ($results | Measure-Object -Property Ms -Average).Average
    $min = ($results | Measure-Object -Property Ms -Minimum).Minimum
    $max = ($results | Measure-Object -Property Ms -Maximum).Maximum
    $lastOutput = ($results | Select-Object -Last 1).OutputPath
    Write-Host ("Average: {0:N2} ms, Min: {1:N2} ms, Max: {2:N2} ms" -f $avg, $min, $max)
    if ($lastOutput) {
        Write-Host "WAV: $lastOutput"
    }
}
finally {
    Pop-Location
}
