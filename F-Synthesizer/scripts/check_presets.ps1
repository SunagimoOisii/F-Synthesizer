param(
    [string]$Configuration = "Debug",
    [string]$Platform = "x64",
    [double]$InitialSeconds = 0.35,
    [int]$SampleRate = 22050,
    [int]$TimeoutSec = 10
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
if (Get-Variable -Name PSNativeCommandUseErrorActionPreference -ErrorAction SilentlyContinue) {
    $PSNativeCommandUseErrorActionPreference = $false
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

function Invoke-CLI {
    param(
        [string]$ExePath,
        [string[]]$CliArgs,
        [int]$TimeoutSec
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
    $p.Refresh()
    $exitCode = $p.ExitCode
    if ($null -eq $exitCode) {
        $exitCode = 0
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
        ExitCode = $exitCode
        Output = $out
    }
}

function Parse-RenderStats {
    param(
        [string]$Text
    )

    $line = ($Text -split "`r?`n" | Where-Object { $_ -like "*RenderStats*" } | Select-Object -Last 1)
    if (-not $line) {
        throw "RenderStats not found."
    }
    if ($line -notmatch 'peak=([^ ]+) rms=([^ ]+) nonZero=([0-9]+)/([0-9]+)') {
        throw "RenderStats parse failed: $line"
    }

    return [PSCustomObject]@{
        Peak = [double]$matches[1]
        Rms = [double]$matches[2]
        NonZero = [int]$matches[3]
        Total = [int]$matches[4]
        Line = $line
    }
}

function Assert-ProjectModelJsonShape {
    param(
        [object]$Config,
        [string]$Label
    )

    if ($Config.PSObject.Properties.Name -notcontains "format") {
        throw "${Label}: missing format."
    }
    if ($Config.format -ne "projectModel.v1") {
        throw "${Label}: unsupported format '$($Config.format)'."
    }
    if ($Config.PSObject.Properties.Name -notcontains "project" -or $null -eq $Config.project) {
        throw "${Label}: missing project object."
    }
    if ($Config.project -isnot [pscustomobject]) {
        throw "${Label}: project must be an object."
    }

    foreach ($legacyKey in @("channels", "channelMix", "effects")) {
        if ($Config.PSObject.Properties.Name -contains $legacyKey) {
            throw "${Label}: legacy top-level '$legacyKey' must be under project."
        }
    }
}

function Read-ProjectModelJson {
    param(
        [string]$Path
    )

    try {
        $config = Get-Content -Path $Path -Raw -Encoding UTF8 | ConvertFrom-Json
        Assert-ProjectModelJsonShape -Config $config -Label ([System.IO.Path]::GetFileName($Path))
        return $config
    }
    catch {
        throw "ProjectModel JSON check failed for ${Path}: $($_.Exception.Message)"
    }
}

function Test-JsonProperty {
    param(
        [object]$Object,
        [string]$Name
    )

    if ($null -eq $Object) {
        return $false
    }
    foreach ($prop in $Object.PSObject.Properties) {
        if ($prop.Name -eq $Name) {
            return $true
        }
    }
    return $false
}

function Test-LayerEnabled {
    param(
        [object]$Layers,
        [string]$Name
    )

    if (-not (Test-JsonProperty -Object $Layers -Name $Name)) {
        return $false
    }
    $layer = $Layers.$Name
    return (Test-JsonProperty -Object $layer -Name "enabled") -and [bool]$layer.enabled
}

function Assert-PracticalPresetLayerPolicy {
    param(
        [object]$PresetConfig,
        [string]$Name
    )

    if (-not $Name.StartsWith("sound_")) {
        return
    }
    if ((Test-JsonProperty -Object $PresetConfig -Name "internal") -and [bool]$PresetConfig.internal) {
        return
    }

    $category = ""
    if (Test-JsonProperty -Object $PresetConfig -Name "category") {
        $category = [string]$PresetConfig.category
    }
    if ($category -ne "") {
        $allowedCategories = @("Lead", "Guitar", "Bass", "Strings", "Brass", "Reed", "Pipe", "Pad", "Piano/Keys", "Drums", "SFX")
        if ($allowedCategories -notcontains $category) {
            throw "${Name}: invalid practical preset category '${category}'."
        }
    }
    $displayName = ""
    if (Test-JsonProperty -Object $PresetConfig -Name "displayName") {
        $displayName = [string]$PresetConfig.displayName
    }

    if (-not (Test-JsonProperty -Object $PresetConfig.project -Name "channels")) {
        return
    }

    foreach ($channelProp in $PresetConfig.project.channels.PSObject.Properties) {
        $channel = $channelProp.Value
        if (-not (Test-JsonProperty -Object $channel -Name "layers")) {
            continue
        }
        $layers = $channel.layers
        if ((Test-LayerEnabled -Layers $layers -Name "chord") -and ($category -ne "Pad" -and $category -ne "Strings")) {
            throw "${Name}: chord layer is only allowed for practical Pad/Strings presets."
        }
        if ((Test-LayerEnabled -Layers $layers -Name "string") -and ($category -ne "Pad" -and $category -ne "Strings")) {
            throw "${Name}: string layer is only allowed for practical Pad/Strings presets."
        }
        foreach ($guitarLayer in @("powerChord", "chug")) {
            if ((Test-LayerEnabled -Layers $layers -Name $guitarLayer) -and $category -ne "Guitar") {
                throw "${Name}: ${guitarLayer} layer is only allowed for practical Guitar presets."
            }
        }
        if ($category -eq "Piano/Keys" -and $displayName -like "*Organ*") {
            foreach ($forbidden in @("chord", "string", "body")) {
                if (Test-LayerEnabled -Layers $layers -Name $forbidden) {
                    throw "${Name}: Keys Organ must use harmonic/source tone instead of ${forbidden} layer."
                }
            }
        }
        if (Test-LayerEnabled -Layers $layers -Name "body") {
            $isAllowedBody = $category -eq "Pad" -or $category -eq "Strings" -or ($category -eq "Piano/Keys" -and $displayName -like "*Pluck*")
            if (-not $isAllowedBody) {
                throw "${Name}: body layer is only allowed for practical Pad/Strings presets or pluck keys."
            }
            $body = $layers.body
            if (-not (Test-JsonProperty -Object $body -Name "mode")) {
                throw "${Name}: enabled body layer must declare mode."
            }
        }
    }
}

function Write-ShortPresetConfig {
    param(
        [object]$PresetConfig,
        [string]$ConfigPath,
        [string]$MidiPath,
        [string]$WavPath,
        [double]$InitialSeconds,
        [int]$SampleRate
    )

    if ($null -eq $PresetConfig.project) {
        $project = [PSCustomObject]@{}
        if ($PresetConfig.PSObject.Properties.Name -contains "channels") {
            $project | Add-Member -NotePropertyName channels -NotePropertyValue $PresetConfig.channels -Force
            $PresetConfig.PSObject.Properties.Remove("channels")
        }
        if ($PresetConfig.PSObject.Properties.Name -contains "channelMix") {
            $project | Add-Member -NotePropertyName channelMix -NotePropertyValue $PresetConfig.channelMix -Force
            $PresetConfig.PSObject.Properties.Remove("channelMix")
        }
        if ($PresetConfig.PSObject.Properties.Name -contains "effects") {
            $project | Add-Member -NotePropertyName effects -NotePropertyValue $PresetConfig.effects -Force
            $PresetConfig.PSObject.Properties.Remove("effects")
        }
        $PresetConfig | Add-Member -NotePropertyName format -NotePropertyValue "projectModel.v1" -Force
        $PresetConfig | Add-Member -NotePropertyName project -NotePropertyValue $project -Force
    }

    $PresetConfig.project | Add-Member -NotePropertyName midiPath -NotePropertyValue $MidiPath -Force
    $PresetConfig.project | Add-Member -NotePropertyName wavPath -NotePropertyValue $WavPath -Force
    $PresetConfig.project | Add-Member -NotePropertyName targetChannel -NotePropertyValue -1 -Force
    $PresetConfig.project | Add-Member -NotePropertyName initialSeconds -NotePropertyValue $InitialSeconds -Force
    $PresetConfig.project | Add-Member -NotePropertyName bits -NotePropertyValue 16 -Force
    $PresetConfig.project | Add-Member -NotePropertyName sampleRate -NotePropertyValue $SampleRate -Force
    $PresetConfig.project | Add-Member -NotePropertyName extraReleaseSec -NotePropertyValue 0.02 -Force
    $PresetConfig | ConvertTo-Json -Depth 80 | Set-Content -Path $ConfigPath -Encoding UTF8
}

function Invoke-ShortRenderCheck {
    param(
        [string]$ExePath,
        [object]$ProjectConfig,
        [string]$Name,
        [string]$RenderConfigPath,
        [string]$MidiPath,
        [string]$WavPath,
        [double]$InitialSeconds,
        [int]$SampleRate,
        [int]$TimeoutSec
    )

    Write-ShortPresetConfig -PresetConfig $ProjectConfig -ConfigPath $RenderConfigPath -MidiPath $MidiPath -WavPath $WavPath -InitialSeconds $InitialSeconds -SampleRate $SampleRate

    $result = Invoke-CLI -ExePath $ExePath -CliArgs @("--cli", "--config", $RenderConfigPath) -TimeoutSec $TimeoutSec
    if ($result.ExitCode -ne 0) {
        throw "${Name}: CLI failed exit=$($result.ExitCode)`n$($result.Output)"
    }

    $stats = Parse-RenderStats -Text $result.Output
    if ($stats.NonZero -le 0) {
        throw "${Name}: render output is silent. $($stats.Line)"
    }

    if (-not (Test-Path $WavPath)) {
        throw "${Name}: WAV not found: $WavPath"
    }
    $wav = Get-Item $WavPath
    if ($wav.Length -le 44) {
        throw "${Name}: WAV is empty or header-only: $WavPath"
    }
}

function Invoke-NamedConfigChecks {
    param(
        [string]$RepoRoot,
        [string]$ExePath,
        [string]$CheckDir,
        [string]$MidiPath,
        [double]$InitialSeconds,
        [int]$SampleRate,
        [int]$TimeoutSec
    )

    $namedConfigs = @(
        (Join-Path $RepoRoot "config/base.json"),
        (Join-Path $RepoRoot "config/default.json")
    )

    foreach ($configPath in $namedConfigs) {
        $name = [System.IO.Path]::GetFileNameWithoutExtension($configPath)
        $config = Read-ProjectModelJson -Path $configPath
        $wavPath = Join-Path $CheckDir "${name}_config.wav"
        $renderConfigPath = Join-Path $CheckDir "${name}_config.render.json"
        Invoke-ShortRenderCheck -ExePath $ExePath -ProjectConfig $config -Name "config/${name}" -RenderConfigPath $renderConfigPath -MidiPath $MidiPath -WavPath $wavPath -InitialSeconds $InitialSeconds -SampleRate $SampleRate -TimeoutSec $TimeoutSec
    }
}

$repoRoot = Split-Path -Parent $PSScriptRoot
Push-Location $repoRoot
try {
    Write-Host "== F-Synthesizer preset check =="
    Write-Host "Repo: $repoRoot"
    Write-Host "Config: $Configuration | Platform: $Platform"

    $exePath = Resolve-ExePath -RepoRoot $repoRoot -Platform $Platform -Configuration $Configuration
    if (-not (Test-Path $exePath)) {
        throw "Executable not found: $exePath. Run .\scripts\check.ps1 first."
    }
    Ensure-RuntimeDependencies -ExePath $exePath

    $presetDir = Join-Path $repoRoot "config/presets"
    $presets = Get-ChildItem -Path $presetDir -Filter "*.json" | Sort-Object Name
    if ($presets.Count -eq 0) {
        throw "No preset JSON files found: $presetDir"
    }

    $checkDir = Join-Path $repoRoot "output/check/presets"
    New-Item -ItemType Directory -Path $checkDir -Force | Out-Null

    $midiPath = Join-Path $checkDir "preset_check.mid"
    $noteTrack = [byte[]](
        0x00,0x90,0x3C,0x64,
        0x18,0x90,0x43,0x64,
        0x18,0x90,0x48,0x64,
        0x30,0x80,0x3C,0x00,
        0x00,0x80,0x43,0x00,
        0x00,0x80,0x48,0x00,
        0x00,0xFF,0x2F,0x00
    )
    Write-BytesFile -Path $midiPath -Bytes (New-MIDIFileBytes -TrackData $noteTrack)

    $failures = New-Object System.Collections.Generic.List[string]
    try {
        Invoke-NamedConfigChecks -RepoRoot $repoRoot -ExePath $exePath -CheckDir $checkDir -MidiPath $midiPath -InitialSeconds $InitialSeconds -SampleRate $SampleRate -TimeoutSec $TimeoutSec
    }
    catch {
        $failures.Add($_.Exception.Message)
    }

    foreach ($preset in $presets) {
        $name = [System.IO.Path]::GetFileNameWithoutExtension($preset.Name)

        $presetConfig = $null
        try {
            $presetConfig = Read-ProjectModelJson -Path $preset.FullName
            Assert-PracticalPresetLayerPolicy -PresetConfig $presetConfig -Name $name
        }
        catch {
            $failures.Add("${name}: $($_.Exception.Message)")
            continue
        }

        $wavPath = Join-Path $checkDir "$name.wav"
        $renderConfigPath = Join-Path $checkDir "$name.render.json"

        try {
            Invoke-ShortRenderCheck -ExePath $exePath -ProjectConfig $presetConfig -Name $name -RenderConfigPath $renderConfigPath -MidiPath $midiPath -WavPath $wavPath -InitialSeconds $InitialSeconds -SampleRate $SampleRate -TimeoutSec $TimeoutSec
        }
        catch {
            $failures.Add($_.Exception.Message)
            continue
        }
    }

    if ($failures.Count -gt 0) {
        Write-Host "Preset check failed: $($failures.Count) issue(s)."
        foreach ($failure in $failures) {
            Write-Host ""
            Write-Host $failure
        }
        exit 1
    }

    Write-Host "Preset check completed: config/base.json, config/default.json, and $($presets.Count) presets OK."
}
finally {
    Pop-Location
}
