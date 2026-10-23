# One-time pruning of the downloads and Unreal caches reviewed on 2026-10-20.
# Run directly in PowerShell; -WhatIf previews all deletions.
[CmdletBinding(SupportsShouldProcess)]
param()

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'PackagedBuilds.ps1')
$nativeRoot = [IO.Path]::GetFullPath($PSScriptRoot).TrimEnd('\', '/')
$nativePrefix = $nativeRoot + '\'
$repoRoot = Split-Path $nativeRoot -Parent
if (!(Test-Path -LiteralPath (Join-Path $nativeRoot 'Unreal/Wildline/Wildline.uproject'))) {
    throw 'Run this script from the Wildline native source directory.'
}
$latest = Get-LatestWildlineBuild -BuildRoot (Join-Path $nativeRoot 'Build')
if (!$latest) { throw 'No valid completed game build exists; pruning is disabled.' }
$active = @(Get-CimInstance Win32_Process | Where-Object {
    $_.Name -match '^(Wildline|UnrealEditor.*|UnrealBuildTool|AutomationTool|ShaderCompileWorker|ISCC|blender)\.exe$'
})
if ($active.Count) { throw 'Close the game, Unreal tools, and Blender before pruning.' }

# Keep Content, ArtSource, Binaries, personal Saved folders, Git, and releases.
# Use named download targets so future downloads are not removed accidentally.
$relativeTargets = @(
    'AssetDownloads/Fab/CityParkEnvironment',
    'AssetDownloads/Blender',
    'AssetDownloads/blender.zip',
    'AssetDownloads/RetiredTravisDetailed-20261008',
    'Unreal/Wildline/Intermediate',
    'Unreal/Wildline/DerivedDataCache',
    'Unreal/Wildline/Saved/StagedBuilds',
    'Unreal/Wildline/Saved/Cooked',
    'Unreal/Wildline/Saved/Temp',
    'Unreal/Wildline/Saved/ShaderDebugInfo',
    'Unreal/Wildline/Saved/Shaders'
)
$targets = @(foreach ($relative in $relativeTargets) {
    $path = [IO.Path]::GetFullPath((Join-Path $nativeRoot $relative))
    if (!$path.StartsWith($nativePrefix, [StringComparison]::OrdinalIgnoreCase)) { throw "Unsafe target: $path" }
    if (!(Test-Path -LiteralPath $path)) { continue }
    $ancestor = $path
    while ($ancestor) {
        if ((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) {
            throw "Filesystem link in target path: $ancestor"
        }
        $ancestor = Split-Path $ancestor -Parent
    }
    $item = Get-Item -LiteralPath $path -Force
    $entries = @($item)
    if ($item.PSIsContainer) { $entries += @(Get-ChildItem -LiteralPath $path -Recurse -Force) }
    foreach ($entry in $entries) {
        if ($entry.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Filesystem link: $($entry.FullName)" }
        if ($entry.Extension -eq '.sav' -or $entry.Name -eq 'SaveGames') { throw "Save data: $($entry.FullName)" }
    }
    $tracked = @(git -C $repoRoot ls-files -- $path)
    if ($LASTEXITCODE -ne 0 -or $tracked.Count) { throw "Cannot prune tracked or unverified files: $path" }
    [pscustomobject]@{
        Path = $path
        Bytes = [long](($entries | Where-Object { !$_.PSIsContainer } | Measure-Object Length -Sum).Sum)
    }
})

$freedBytes = 0L
foreach ($target in $targets) {
    if ($PSCmdlet.ShouldProcess($target.Path, 'Remove reviewed redundant downloads or rebuildable Unreal cache')) {
        Remove-Item -LiteralPath $target.Path -Recurse -Force
        $freedBytes += $target.Bytes
    }
}
$remainingBuild = Get-LatestWildlineBuild -BuildRoot (Join-Path $nativeRoot 'Build')
if (!$remainingBuild -or $remainingBuild.PackageRoot -ne $latest.PackageRoot) { throw 'Retained latest build validation failed.' }
'Removed {0:N2} GiB. Latest completed build preserved: {1}' -f ($freedBytes / 1GB), $remainingBuild.PackageRoot
