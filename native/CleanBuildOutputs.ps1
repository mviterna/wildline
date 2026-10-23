[CmdletBinding(SupportsShouldProcess)]
param([string]$BuildRoot)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'PackagedBuilds.ps1')
if (!$BuildRoot) { $BuildRoot = Join-Path $PSScriptRoot 'Build' }
$root = [IO.Path]::GetFullPath($BuildRoot).TrimEnd('\', '/')
if ((Split-Path $root -Leaf) -ne 'Build') { throw 'Cleanup requires a dedicated Build directory.' }
$prefix = $root + '\'

function Assert-CleanupPath([string]$Path, [switch]$Deleting) {
    $full = [IO.Path]::GetFullPath($Path).TrimEnd('\', '/')
    if (!$full.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Cleanup target is outside the build directory: $full"
    }
    $ancestor = $full
    while ($ancestor) {
        if ((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) {
            throw "Cleanup cannot traverse a filesystem link: $ancestor"
        }
        $ancestor = Split-Path $ancestor -Parent
    }
    if ((Get-Item -LiteralPath $full -Force).PSIsContainer) {
        foreach ($item in (Get-ChildItem -LiteralPath $full -Recurse -Force)) {
            if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Cleanup target contains a filesystem link: $($item.FullName)" }
            if ($Deleting -and ($item.Extension -eq '.sav' -or ($item.PSIsContainer -and $item.Name -eq 'SaveGames'))) {
                throw "Cleanup target contains saves; preserve them before retrying: $($item.FullName)"
            }
        }
    }
    $full
}

$latest = Get-LatestWildlineBuild -BuildRoot $root
if (!$latest) { throw 'No valid completed build exists; no files were removed.' }
$packagePath = Assert-CleanupPath $latest.PackageRoot
$keepDirectory = $packagePath.Substring($prefix.Length).Split('\')[0]
$receiptPath = Join-Path $root 'Beta/LatestInstaller.json'
$receipt = $null
if (Test-Path -LiteralPath $receiptPath) {
    $receipt = Get-Content -LiteralPath $receiptPath -Raw | ConvertFrom-Json
    if ($receipt.version -notmatch '^\d+\.\d+\.\d+-beta\.\d{8}\.\d{6}$') { throw 'Invalid latest installer version.' }
    $expectedSetup = Join-Path $root "Beta/Wildline-$($receipt.version)-Setup.exe"
    $expectedBootstrap = Join-Path $root 'Beta/WildlineSetup.exe'
    if ([IO.Path]::GetFullPath($receipt.setupPath) -ne $expectedSetup -or
        [IO.Path]::GetFullPath($receipt.bootstrapPath) -ne $expectedBootstrap) {
        throw 'Latest installer paths do not match this build directory.'
    }
    foreach ($path in @($expectedSetup, $expectedBootstrap)) { Assert-CleanupPath $path | Out-Null }
    if ((Get-Item -LiteralPath $expectedSetup).Length -ne $receipt.size -or
        (Get-FileHash -LiteralPath $expectedSetup -Algorithm SHA256).Hash -ne $receipt.sha256 -or
        (Get-FileHash -LiteralPath $expectedBootstrap -Algorithm SHA256).Hash -ne $receipt.bootstrapSha256) {
        throw 'Latest installer integrity check failed; no files were removed.'
    }
}

$targets = @(
    Get-ChildItem -LiteralPath $root -Directory -Force | Where-Object {
        $_.Name -notin @($keepDirectory, 'Beta', 'Updates', 'Launcher')
    } | ForEach-Object { $_.FullName }
    Get-ChildItem -LiteralPath $root -File -Filter '*.zip' | ForEach-Object { $_.FullName }
    if ($receipt) {
        Get-ChildItem -LiteralPath (Join-Path $root 'Beta') -File | Where-Object {
            $_.Name -match '\.exe(\.sha256)?$' -and
            $_.Name -notin @((Split-Path $receipt.setupPath -Leaf), ((Split-Path $receipt.setupPath -Leaf) + '.sha256'), 'WildlineSetup.exe', 'WildlineSetup.exe.sha256')
        } | ForEach-Object { $_.FullName }
        if (Test-Path -LiteralPath (Join-Path $root 'Updates')) {
            Get-ChildItem -LiteralPath (Join-Path $root 'Updates') -Directory -Force |
                Where-Object Name -ne $receipt.version | ForEach-Object { $_.FullName }
        }
    }
)
# Validate every target before deleting anything. Never follow junctions or touch saves.
$targets = @($targets | ForEach-Object { Assert-CleanupPath $_ -Deleting })
$processes = @(Get-CimInstance Win32_Process | Where-Object { $_.ProcessId -ne $PID -and $_.ExecutablePath })
foreach ($target in $targets) {
    foreach ($process in $processes) {
        if ($process.ExecutablePath -eq $target -or $process.ExecutablePath.StartsWith($target + '\', [StringComparison]::OrdinalIgnoreCase)) {
            throw "Cleanup target is in use by process $($process.ProcessId): $target"
        }
    }
}
foreach ($target in $targets) {
    if ($PSCmdlet.ShouldProcess($target, 'Remove obsolete Wildline build output')) {
        Remove-Item -LiteralPath $target -Recurse -Force
    }
}
Write-Verbose "Kept completed package: $packagePath"
