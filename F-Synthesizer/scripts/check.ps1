param(
    [string]$Configuration = "Debug",
    [string]$Platform = "x64",
    [switch]$RunRuntimeSmoke
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
        (Join-Path $vcpkgRoot "debug/bin/glfw3.dll"),
        (Join-Path $vcpkgRoot "bin/glfw3.dll")
    )
    foreach ($src in $sourceCandidates) {
        if (Test-Path $src) {
            Copy-Item -Path $src -Destination $glfwPath -Force
            Write-Host "Runtime dependency restored: $glfwPath"
            return
        }
    }
}

function Parse-RenderStats {
    param(
        [string]$Text
    )

    $line = ($Text -split "`r?`n" | Where-Object { $_ -like "*RenderStats*" } | Select-Object -Last 1)
    if (-not $line) {
        throw "RenderStats not found in command output."
    }

    if ($line -notmatch 'peak=([^ ]+) rms=([^ ]+) nonZero=([0-9]+)/([0-9]+)') {
        throw "RenderStats parse failed: $line"
    }

    return [PSCustomObject]@{
        Peak = [double]$matches[1]
        Rms = [double]$matches[2]
        NonZero = [int]$matches[3]
        Total = [int]$matches[4]
    }
}

function Invoke-CLI {
    param(
        [string]$ExePath,
        [string[]]$CliArgs
    )

    $out = & $ExePath @CliArgs 2>&1 | Out-String
    return [PSCustomObject]@{
        ExitCode = $LASTEXITCODE
        Output = $out
    }
}

function Run-RuntimeSmoke {
    param(
        [string]$RepoRoot,
        [string]$ExePath
    )

    Ensure-RuntimeDependencies -ExePath $ExePath

    $smokeDir = Join-Path $RepoRoot "output/check/smoke"
    New-Item -ItemType Directory -Path $smokeDir -Force | Out-Null

    $configPath = Join-Path $smokeDir "quick_smoke.json"
    $wavPath = Join-Path $smokeDir "quick_smoke.wav"
    $midiPath = (Join-Path $RepoRoot "assets/midi/logo.mid") -replace "\\", "/"
    $wavPathNorm = ($wavPath -replace "\\", "/")

    $json = @"
{
  "midiPath": "$midiPath",
  "wavPath": "$wavPathNorm",
  "targetChannel": 0,
  "defaultWave": "saw",
  "initialSeconds": 1,
  "bits": 16,
  "sampleRate": 44100,
  "extraReleaseSec": 0.05
}
"@
    Set-Content -Path $configPath -Value $json -Encoding UTF8

    $ok = Invoke-CLI -ExePath $ExePath -CliArgs @("--cli", "--config", $configPath)
    if ($ok.ExitCode -ne 0) {
        Write-Host $ok.Output
        throw "Runtime smoke render failed (exit=$($ok.ExitCode))."
    }

    $stats = Parse-RenderStats -Text $ok.Output
    if ($stats.NonZero -le 0) {
        throw "Runtime smoke produced silent output."
    }
    if (-not (Test-Path $wavPath)) {
        throw "Runtime smoke WAV not found: $wavPath"
    }

    $missing = Invoke-CLI -ExePath $ExePath -CliArgs @("--cli", "--config", (Join-Path $RepoRoot "config/__missing__.json"))
    if ($missing.ExitCode -eq 0) {
        throw "Missing config should fail, but exited 0."
    }

    Write-Host "Runtime smoke: OK (peak=$($stats.Peak), rms=$($stats.Rms), nonZero=$($stats.NonZero)/$($stats.Total))"
}

$repoRoot = Split-Path -Parent $PSScriptRoot
Push-Location $repoRoot
try {
    Write-Host "== F-Synthesizer check =="
    Write-Host "Repo: $repoRoot"
    Write-Host "Config: $Configuration | Platform: $Platform"

    $msbuildPath = Get-MsbuildPath
    if (-not $msbuildPath) {
        throw "MSBuild not found. Install Visual Studio Build Tools or add MSBuild to PATH."
    }

    Write-Host "Running build..."
    & $msbuildPath "F-Synthesizer.vcxproj" /t:Build "/p:Configuration=$Configuration;Platform=$Platform" /m /nologo
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed with exit code $LASTEXITCODE."
    }

    if ($RunRuntimeSmoke) {
        $exePath = Resolve-ExePath -RepoRoot $repoRoot -Platform $Platform -Configuration $Configuration
        if (-not (Test-Path $exePath)) {
            throw "Executable not found after build: $exePath"
        }
        Run-RuntimeSmoke -RepoRoot $repoRoot -ExePath $exePath
    }
    else {
        Write-Host "Runtime smoke: skipped (use -RunRuntimeSmoke to enable)"
    }

    Write-Host "Check completed."
}
finally {
    Pop-Location
}
