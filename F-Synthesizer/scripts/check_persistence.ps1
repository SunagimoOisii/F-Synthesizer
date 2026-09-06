# Run after scripts/check.ps1. Reuses the application objects; no separate test framework.
$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
Push-Location $repoRoot
try {
    $vswhere = "C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
    $vsRoot = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    & (Join-Path $vsRoot "Common7/Tools/Launch-VsDevShell.ps1") -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
    $checkDir = Join-Path $repoRoot "output/check/persistence"
    New-Item -ItemType Directory -Path $checkDir -Force | Out-Null
    [xml]$project = Get-Content -Raw -Encoding UTF8 F-Synthesizer.vcxproj
    $objects = @($project.Project.ItemGroup.ClCompile | Where-Object { $_.Include } | ForEach-Object {
        $stem = [System.IO.Path]::GetFileNameWithoutExtension($_.Include)
        if ($stem -ne "SoundGenerate") { Join-Path $repoRoot "build/obj/x64/Debug/$stem.obj" }
    })
    foreach ($object in $objects) {
        if (-not (Test-Path -LiteralPath $object)) { throw "Run scripts/check.ps1 first: missing $object" }
    }
    $exe = Join-Path $checkDir "persistence-check.exe"
    $log = Join-Path $checkDir "compile.log"
    & cl.exe /nologo /std:c++20 /EHsc /MDd /D_DEBUG /utf-8 /Iinclude /Isrc /IC:/vcpkg/installed/x64-windows/include `
        "/Fo$checkDir/persistence.obj" "/Fe$exe" tests/persistence.cpp $objects `
        /link /LIBPATH:C:/vcpkg/installed/x64-windows/debug/lib `
        kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib `
        ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib *> $log
    if ($LASTEXITCODE -ne 0) {
        Get-Content -LiteralPath $log -Tail 25
        throw "Persistence check build failed."
    }
    Copy-Item -LiteralPath "./build/x64/Debug/glfw3.dll" -Destination $checkDir -Force
    & $exe
    if ($LASTEXITCODE -ne 0) { throw "Persistence checks failed." }
}
finally {
    Pop-Location
}
