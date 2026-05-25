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
    if ($Config.format -ne "projectModel.v3") {
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
    if ((Test-JsonProperty -Object $Config.project -Name "channelMix")) {
        throw "${Label}: project.channelMix is legacy v2 shape; use project.channels.*.mix."
    }
    if ((Test-JsonProperty -Object $Config.project -Name "instruments")) {
        if ($Config.project.instruments -isnot [pscustomobject]) {
            throw "${Label}: project.instruments must be an object."
        }
        foreach ($instrumentProp in $Config.project.instruments.PSObject.Properties) {
            $instrument = $instrumentProp.Value
            if ($instrument -isnot [pscustomobject]) {
                throw "${Label}: project.instruments.$($instrumentProp.Name) must be an object."
            }
            if (-not (Test-JsonProperty -Object $instrument -Name "sound")) {
                throw "${Label}: project.instruments.$($instrumentProp.Name).sound is required."
            }
            if ($Label.StartsWith("sound_")) {
                foreach ($required in @("displayName", "category", "tags", "description", "recommendedRange", "macroHints")) {
                    if (-not (Test-JsonProperty -Object $instrument -Name $required)) {
                        throw "${Label}: project.instruments.$($instrumentProp.Name).${required} is required for practical Sound Card."
                    }
                }
                if ($instrument.recommendedRange -isnot [pscustomobject]) {
                    throw "${Label}: project.instruments.$($instrumentProp.Name).recommendedRange must be an object."
                }
                foreach ($rangeKey in @("low", "high", "preview")) {
                    if (-not (Test-JsonProperty -Object $instrument.recommendedRange -Name $rangeKey)) {
                        throw "${Label}: project.instruments.$($instrumentProp.Name).recommendedRange.${rangeKey} is required."
                    }
                    $rangeValue = [int]$instrument.recommendedRange.$rangeKey
                    if ($rangeValue -lt 0 -or $rangeValue -gt 127) {
                        throw "${Label}: project.instruments.$($instrumentProp.Name).recommendedRange.${rangeKey} must be in range 0..127."
                    }
                }
                if (@($instrument.macroHints).Count -ne 4) {
                    throw "${Label}: project.instruments.$($instrumentProp.Name).macroHints must contain 4 entries."
                }
                $macroIds = @($instrument.macroHints | ForEach-Object { [string]$_.id })
                foreach ($macroId in @("brightness", "roughness", "movement", "envelope")) {
                    if ($macroIds -notcontains $macroId) {
                        throw "${Label}: project.instruments.$($instrumentProp.Name).macroHints missing '${macroId}'."
                    }
                }
            }
        }
    }
    if ((Test-JsonProperty -Object $Config.project -Name "channels")) {
        foreach ($channelProp in $Config.project.channels.PSObject.Properties) {
            $channel = $channelProp.Value
            if (-not (Test-JsonProperty -Object $channel -Name "instrumentId")) {
                throw "${Label}: project.channels.$($channelProp.Name).instrumentId is required."
            }
            $instrumentId = [string]$channel.instrumentId
            if (-not (Test-JsonProperty -Object $Config.project -Name "instruments") -or -not (Test-JsonProperty -Object $Config.project.instruments -Name $instrumentId)) {
                throw "${Label}: project.channels.$($channelProp.Name).instrumentId references unknown instrument '${instrumentId}'."
            }
            foreach ($legacyKey in @("source", "layers", "expressionMap", "amp", "attackSec", "decaySec", "sustainLevel", "releaseSec")) {
                if (Test-JsonProperty -Object $channel -Name $legacyKey) {
                    throw "${Label}: project.channels.$($channelProp.Name).${legacyKey} is legacy sound shape; move it to instrument.sound."
                }
            }
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

    if (-not (Test-JsonProperty -Object $PresetConfig.project -Name "instruments")) {
        return
    }

    foreach ($instrumentProp in $PresetConfig.project.instruments.PSObject.Properties) {
        $instrument = $instrumentProp.Value
        $category = ""
        if (Test-JsonProperty -Object $instrument -Name "category") {
            $category = [string]$instrument.category
        }
        if ($category -ne "") {
            $allowedCategories = @("Lead", "Guitar", "Bass", "Pad", "Keys", "Drums", "SFX", "Support")
            if ($allowedCategories -notcontains $category) {
                throw "${Name}: invalid practical instrument category '${category}'."
            }
        }
        $displayName = ""
        if (Test-JsonProperty -Object $instrument -Name "displayName") {
            $displayName = [string]$instrument.displayName
        }
        if (-not (Test-JsonProperty -Object $instrument -Name "sound")) {
            continue
        }
        $sound = $instrument.sound
        if (-not (Test-JsonProperty -Object $sound -Name "layers")) {
            continue
        }
        $layers = $sound.layers
        if ((Test-LayerEnabled -Layers $layers -Name "chord") -and $category -ne "Pad") {
            throw "${Name}: chord layer is only allowed for practical Pad presets."
        }
        if ((Test-LayerEnabled -Layers $layers -Name "string") -and $category -ne "Pad") {
            throw "${Name}: string layer is only allowed for practical Pad presets."
        }
        foreach ($guitarLayer in @("powerChord", "chug")) {
            if ((Test-LayerEnabled -Layers $layers -Name $guitarLayer) -and $category -ne "Guitar") {
                throw "${Name}: ${guitarLayer} layer is only allowed for practical Guitar presets."
            }
        }
        if ($category -eq "Keys" -and $displayName -like "*Organ*") {
            foreach ($forbidden in @("chord", "string", "body")) {
                if (Test-LayerEnabled -Layers $layers -Name $forbidden) {
                    throw "${Name}: Keys Organ must use harmonic/source tone instead of ${forbidden} layer."
                }
            }
        }
        if (Test-LayerEnabled -Layers $layers -Name "body") {
            $isAllowedBody = $category -eq "Pad" -or
                ($category -eq "Keys" -and $displayName -like "*Pluck*") -or
                ($category -eq "Support" -and $displayName -like "*Pizzicato*")
            if (-not $isAllowedBody) {
                throw "${Name}: body layer is only allowed for practical Pad presets, pluck keys, or pizzicato support."
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
        $PresetConfig | Add-Member -NotePropertyName format -NotePropertyValue "projectModel.v3" -Force
        $PresetConfig | Add-Member -NotePropertyName project -NotePropertyValue $project -Force
    }

    $PresetConfig.project | Add-Member -NotePropertyName midiPath -NotePropertyValue $MidiPath -Force
    $PresetConfig.project | Add-Member -NotePropertyName wavPath -NotePropertyValue $WavPath -Force
    $PresetConfig.project | Add-Member -NotePropertyName targetChannel -NotePropertyValue ([int]-1) -Force
    $PresetConfig.project | Add-Member -NotePropertyName initialSeconds -NotePropertyValue ([int][Math]::Max(1, [Math]::Ceiling($InitialSeconds))) -Force
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
    $legacySoundFiles = @(Get-ChildItem -Path $presetDir -Filter "*.json" |
        Where-Object { $_.BaseName -match '^sound_(brass|reed|pipe|strings)_' })
    if ($legacySoundFiles.Count -gt 0) {
        throw "Legacy practical preset file names remain: $($legacySoundFiles.Name -join ', ')"
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
