param(
    [string]$Configuration = "Debug",
    [string]$Platform = "x64",
    [switch]$SkipBuild,
    [switch]$SkipRun,
    [switch]$AllowDocMismatch,
    [ValidateSet("off", "warn", "error")]
    [string]$DocRules = "warn"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
if (Get-Variable -Name PSNativeCommandUseErrorActionPreference -ErrorAction SilentlyContinue) {
    $PSNativeCommandUseErrorActionPreference = $false
}

function Get-ChangedFiles {
    param(
        [string]$ProjectDirName
    )

    $lines = @(git status --porcelain=v1)
    $files = @()
    foreach ($line in $lines) {
        if ($line.Length -lt 4) {
            continue
        }

        $pathPart = $line.Substring(3).Trim()
        if ($pathPart -match " -> ") {
            $pathPart = ($pathPart -split " -> ")[-1]
        }

        $normalized = $pathPart -replace "\\", "/"
        $prefix = "$ProjectDirName/"
        if ($normalized.StartsWith($prefix)) {
            $normalized = $normalized.Substring($prefix.Length)
        }

        if ($normalized) {
            $files += $normalized
        }
    }

    return @($files | Sort-Object -Unique)
}

function Test-DocRules {
    param(
        [string[]]$ChangedFiles
    )

    $rules = @(
        @{
            Name = "Code changes should update STATUS.md"
            Trigger = '^(src|include)/'
            RequiredFile = "STATUS.md"
        },
        @{
            Name = "SynthEngine changes should update Architecture.md"
            Trigger = '^(src/SynthEngine|include/SynthEngine)/'
            RequiredFile = "Architecture.md"
        }
    )

    $violations = @()
    foreach ($rule in $rules) {
        $triggered = $ChangedFiles | Where-Object { $_ -match $rule.Trigger }
        if (-not $triggered) {
            continue
        }

        if ($ChangedFiles -contains $rule.RequiredFile) {
            continue
        }

        $violations += [PSCustomObject]@{
            Rule = $rule.Name
            RequiredFile = $rule.RequiredFile
            TriggeredBy = ($triggered -join ", ")
        }
    }

    return $violations
}

$repoRoot = Split-Path -Parent $PSScriptRoot
Push-Location $repoRoot
try {
    Write-Host "== F-Synthesizer check =="
    Write-Host "Repo: $repoRoot"
    Write-Host "Config: $Configuration | Platform: $Platform"

    $projectDirName = Split-Path -Leaf $repoRoot
    $changedFiles = @(Get-ChangedFiles -ProjectDirName $projectDirName)
    if ($changedFiles.Count -eq 0) {
        Write-Host "No local changes found."
    }
    else {
        Write-Host "Changed files:"
        $changedFiles | ForEach-Object { Write-Host "  - $_" }
    }

    $violations = @()
    if ($DocRules -ne "off") {
        $violations = @(Test-DocRules -ChangedFiles $changedFiles)
    }

    if ($AllowDocMismatch -and $DocRules -eq "error") {
        Write-Warning "-AllowDocMismatch is set. Downgrading DocRules from 'error' to 'warn'."
        $DocRules = "warn"
    }

    $docCheckFailed = $false
    if ($DocRules -eq "off") {
        Write-Host "Documentation rules: OFF"
    }
    elseif ($violations.Count -gt 0) {
        Write-Warning "Documentation update rules are not satisfied."
        foreach ($v in $violations) {
            Write-Warning "Rule: $($v.Rule)"
            Write-Warning "  Required update: $($v.RequiredFile)"
            Write-Warning "  Triggered by: $($v.TriggeredBy)"
        }
        if ($DocRules -eq "error") {
            $docCheckFailed = $true
        }
    }
    else {
        Write-Host "Documentation rules: OK"
    }

    if (-not $SkipBuild) {
        $msbuild = Get-Command msbuild -ErrorAction SilentlyContinue
        if ($null -eq $msbuild) {
            Write-Warning "msbuild is not available in PATH. Build step skipped."
        }
        else {
            Write-Host "Running build..."
            & $msbuild.Source "F-Synthesizer.vcxproj" /t:Build "/p:Configuration=$Configuration;Platform=$Platform" /m /nologo
        }
    }
    else {
        Write-Host "Build step skipped by option."
    }

    if (-not $SkipRun) {
        $exePath = Join-Path $repoRoot "$Platform\$Configuration\F-Synthesizer.exe"
        if (Test-Path $exePath) {
            Write-Host "Running executable: $exePath"
            & $exePath
        }
        else {
            Write-Warning "Executable not found: $exePath"
            Write-Warning "Run with -SkipRun or build first."
        }
    }
    else {
        Write-Host "Run step skipped by option."
    }

    if ($docCheckFailed) {
        Write-Error "Documentation check failed. Update required markdown files or run with -AllowDocMismatch."
    }

    Write-Host "Check completed."
}
finally {
    Pop-Location
}
