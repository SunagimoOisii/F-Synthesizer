param(
    [string]$Configuration = "Debug",
    [string]$Platform = "x64",
    [switch]$RunRuntimeSmoke,
    [int]$RuntimeSmokeTimeoutSec = 10
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
    if (Test-Path -LiteralPath $glfwPath -PathType Leaf) {
        return
    }

    $vcpkgRoot = "C:/vcpkg/installed/x64-windows"
    $runtimeDirectory = if ($Configuration -eq "Debug") { "debug/bin" } else { "bin" }
    $sourceCandidates = @((Join-Path $vcpkgRoot "$runtimeDirectory/glfw3.dll"))
    foreach ($src in $sourceCandidates) {
        if (Test-Path -LiteralPath $src -PathType Leaf) {
            Copy-Item -LiteralPath $src -Destination $glfwPath -Force
            Write-Host "Runtime dependency restored: $glfwPath"
            return
        }
    }
    throw "glfw3.dll is missing. Expected beside the executable: $glfwPath. Install glfw3:x64-windows in C:/vcpkg (source: $($sourceCandidates -join ', '))."
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
        [string[]]$CliArgs,
        [int]$TimeoutSec = 10
    )

    $tempBase = [System.IO.Path]::Combine([System.IO.Path]::GetTempPath(), [System.IO.Path]::GetRandomFileName())
    $stdoutPath = "$tempBase.out"
    $stderrPath = "$tempBase.err"
    $p = Start-Process -FilePath $ExePath -ArgumentList $CliArgs -NoNewWindow -PassThru -RedirectStandardOutput $stdoutPath -RedirectStandardError $stderrPath
    if (-not $p.WaitForExit($TimeoutSec * 1000)) {
        Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
        return [PSCustomObject]@{
            ExitCode = -999
            Output = "CLI command timed out after ${TimeoutSec}s: $($CliArgs -join ' ')"
        }
    }

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

    return [PSCustomObject]@{
        ExitCode = $p.ExitCode
        Output = $out
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

function Run-RuntimeSmoke {
    param(
        [string]$RepoRoot,
        [string]$ExePath,
        [int]$TimeoutSec
    )

    $smokeDir = Join-Path $RepoRoot "output/check/smoke"
    New-Item -ItemType Directory -Path $smokeDir -Force | Out-Null

    $configPath = Join-Path $smokeDir "quick_smoke.json"
    $wavPath = Join-Path $smokeDir "quick_smoke.wav"
    $midiPathRaw = Join-Path $smokeDir "quick_smoke.mid"
    $noteTrack = [byte[]](0x00,0x90,0x3C,0x64, 0x30,0x80,0x3C,0x00, 0x00,0xFF,0x2F,0x00)
    Write-BytesFile -Path $midiPathRaw -Bytes (New-MIDIFileBytes -TrackData $noteTrack)

    $midiPath = $midiPathRaw -replace "\\", "/"
    $wavPathNorm = ($wavPath -replace "\\", "/")

    $json = @"
{
  "format": "projectModel.v3",
  "project": {
    "midiPath": "$midiPath",
    "wavPath": "$wavPathNorm",
    "targetChannel": 0,
    "initialSeconds": 1,
    "bits": 16,
    "sampleRate": 22050,
    "extraReleaseSec": 0.01
  }
}
"@
    Set-Content -Path $configPath -Value $json -Encoding UTF8

    $ok = Invoke-CLI -ExePath $ExePath -CliArgs @("--cli", "--config", $configPath) -TimeoutSec $TimeoutSec
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

    $missing = Invoke-CLI -ExePath $ExePath -CliArgs @("--cli", "--config", (Join-Path $RepoRoot "config/__missing__.json")) -TimeoutSec $TimeoutSec
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

    $exePath = Resolve-ExePath -RepoRoot $repoRoot -Platform $Platform -Configuration $Configuration
    if (-not (Test-Path -LiteralPath $exePath -PathType Leaf)) {
        throw "Executable not found after build: $exePath"
    }
    # vcpkg skips app-local DLL deployment when linking is skipped on an incremental build.
    # The normal start.cmd path must also repair a missing runtime DLL.
    Ensure-RuntimeDependencies -ExePath $exePath

    if ($RunRuntimeSmoke) {
        Run-RuntimeSmoke -RepoRoot $repoRoot -ExePath $exePath -TimeoutSec $RuntimeSmokeTimeoutSec
    }
    else {
        Write-Host "Runtime smoke: skipped (use -RunRuntimeSmoke to enable)"
    }

    Write-Host "Check completed."
}
finally {
    Pop-Location
}
