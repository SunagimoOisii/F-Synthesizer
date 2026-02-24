param()

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-NormalizedChangedFiles {
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

function Update-SpecialNotesTemplate {
    param(
        [string]$DocPath,
        [string]$Category,
        [string[]]$RelatedFiles,
        [string]$DateLabel
    )

    if (-not (Test-Path $DocPath)) {
        return $false
    }

    $lines = New-Object System.Collections.Generic.List[string]
    foreach ($l in (Get-Content -Path $DocPath)) {
        $lines.Add($l)
    }

    $categoryHeading = "### $Category"
    $categoryIndex = -1
    for ($i = 0; $i -lt $lines.Count; $i++) {
        if ($lines[$i].Trim() -eq $categoryHeading) {
            $categoryIndex = $i
            break
        }
    }

    $marker = "#### ${DateLabel}: TODO (auto-generated)"
    if ($categoryIndex -ge 0) {
        $endIndex = $lines.Count
        for ($j = $categoryIndex + 1; $j -lt $lines.Count; $j++) {
            if ($lines[$j] -match '^###\s') {
                $endIndex = $j
                break
            }
        }

        for ($k = $categoryIndex + 1; $k -lt $endIndex; $k++) {
            if ($lines[$k].Trim() -eq $marker) {
                return $false
            }
        }

        $related = if ($RelatedFiles.Count -gt 0) { ($RelatedFiles -join ", ") } else { "(none)" }
        $insert = @(
            "",
            $marker,
            "- カテゴリ: $Category",
            "- 背景:",
            "- 判断:",
            "- 代替案:",
            "- 影響範囲:",
            "- 関連ファイル: $related",
            ""
        )

        for ($n = $insert.Count - 1; $n -ge 0; $n--) {
            $lines.Insert($endIndex, $insert[$n])
        }
    }
    else {
        if ($lines -contains $marker) {
            return $false
        }

        $related = if ($RelatedFiles.Count -gt 0) { ($RelatedFiles -join ", ") } else { "(none)" }
        $append = @(
            "",
            "### $Category",
            "",
            $marker,
            "- カテゴリ: $Category",
            "- 背景:",
            "- 判断:",
            "- 代替案:",
            "- 影響範囲:",
            "- 関連ファイル: $related",
            ""
        )

        foreach ($line in $append) {
            $lines.Add($line)
        }
    }

    Set-Content -Path $DocPath -Value $lines -Encoding utf8
    return $true
}

$repoRoot = Split-Path -Parent $PSScriptRoot
Push-Location $repoRoot
try {
    $projectDirName = Split-Path -Leaf $repoRoot
    $changed = @(Get-NormalizedChangedFiles -ProjectDirName $projectDirName)
    if ($changed.Count -eq 0) {
        return
    }

    $rules = @(
        @{
            Pattern = '^(src/gui|include/gui)/'
            Doc = "docs/architecture/gui.md"
            Category = "GUI操作・状態管理"
        },
        @{
            Pattern = '^(src/app|include/app|src/core|include/core|src/midi|include/midi|src/SoundGenerate\.cpp|include/AppCore\.h)'
            Doc = "docs/architecture/runtime-flow.md"
            Category = "実行フロー/キャンセル"
        },
        @{
            Pattern = '^(src/midi|include/midi)/'
            Doc = "docs/architecture/runtime-flow.md"
            Category = "MIDI時間変換"
        },
        @{
            Pattern = '^(src/config|include/config|src/io|include/io)/'
            Doc = "docs/architecture/config-and-io.md"
            Category = "Config互換性"
        },
        @{
            Pattern = '^(src/SynthEngine|include/SynthEngine|src/synth|include/synth)/'
            Doc = "docs/architecture/module-map.md"
            Category = "音響アルゴリズム上の制約"
        },
        @{
            Pattern = '^(src|include)/'
            Doc = "docs/architecture/module-map.md"
            Category = "依存方向・責務境界"
        }
    )

    $today = (Get-Date).ToString("yyyy-MM-dd")
    foreach ($rule in $rules) {
        $hit = @($changed | Where-Object { $_ -match $rule.Pattern })
        if ($hit.Count -eq 0) {
            continue
        }

        $rel = @($hit | Select-Object -First 5)
        $docPath = Join-Path $repoRoot $rule.Doc
        if (Update-SpecialNotesTemplate -DocPath $docPath -Category $rule.Category -RelatedFiles $rel -DateLabel $today) {
            Write-Host "$($rule.Doc) special note template auto-added ($($rule.Category))."
        }
    }
}
finally {
    Pop-Location
}
