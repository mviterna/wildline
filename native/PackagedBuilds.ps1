$ErrorActionPreference = 'Stop'

function Get-WildlinePackageFiles {
    param([Parameter(Mandatory)][string]$PackageRoot)
    $root = [IO.Path]::GetFullPath($PackageRoot).TrimEnd('\', '/')
    foreach ($relative in @('Wildline.exe', 'Wildline/Binaries/Win64/Wildline.exe')) {
        $path = Join-Path $root $relative
        if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing packaged executable: $path" }
    }
    $pakRoot = Join-Path $root 'Wildline/Content/Paks'
    $paks = @(Get-ChildItem -LiteralPath $pakRoot -File -Recurse)
    foreach ($extension in @('.pak', '.utoc', '.ucas')) {
        if (!($paks | Where-Object Extension -eq $extension)) { throw "Missing $extension payload in $pakRoot" }
    }
    $files = @((Get-Item -LiteralPath (Join-Path $root 'Wildline.exe')))
    foreach ($relative in @('Wildline/Binaries', 'Engine/Binaries', 'Wildline/Content/Paks', 'Wildline/Config')) {
        $path = Join-Path $root $relative
        if (Test-Path -LiteralPath $path -PathType Container) {
            $files += Get-ChildItem -LiteralPath $path -File -Recurse
        }
    }
    foreach ($file in ($files | Sort-Object FullName)) {
        if ($file.Length -eq 0) { throw "Empty packaged file: $($file.FullName)" }
        [pscustomobject]@{
            path = $file.FullName.Substring($root.Length + 1).Replace('\', '/')
            length = $file.Length
            modifiedUtcTicks = $file.LastWriteTimeUtc.Ticks.ToString()
        }
    }
}

function Publish-WildlineBuild {
    param(
        [Parameter(Mandatory)][string]$PackageRoot,
        [DateTimeOffset]$CompletedAtUtc = [DateTimeOffset]::UtcNow,
        [switch]$SkipDesktopShortcut
    )
    $root = [IO.Path]::GetFullPath($PackageRoot)
    $files = @(Get-WildlinePackageFiles -PackageRoot $root)
    $record = [ordered]@{
        schemaVersion = 1
        completedAtUtc = $CompletedAtUtc.ToUniversalTime().ToString('o')
        files = $files
    }
    $marker = Join-Path $root 'WildlineBuild.json'
    $temporary = "$marker.$([guid]::NewGuid().ToString('N')).tmp"
    try {
        $record | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $temporary -Encoding UTF8
        Move-Item -LiteralPath $temporary -Destination $marker -Force
    } finally {
        if (Test-Path -LiteralPath $temporary) { Remove-Item -LiteralPath $temporary }
    }
    if (!$SkipDesktopShortcut) { Update-WildlineDesktopShortcut }
    Get-Item -LiteralPath $marker
}

function Update-WildlineDesktopShortcut {
    param(
        [string]$NativeRoot = $PSScriptRoot,
        [string]$DesktopPath = [Environment]::GetFolderPath('Desktop')
    )
    $nativePath = [IO.Path]::GetFullPath($NativeRoot)
    $latest = Get-LatestWildlineBuild -BuildRoot (Join-Path $nativePath 'Build')
    if (!$latest) { throw 'No completed Windows package is available for the desktop shortcut.' }
    $shortcut = (New-Object -ComObject WScript.Shell).CreateShortcut((Join-Path $DesktopPath 'WILDLINE.lnk'))
    $shortcut.TargetPath = Join-Path $env:WINDIR 'System32/WindowsPowerShell/v1.0/powershell.exe'
    $shortcut.Arguments = '-NoProfile -WindowStyle Hidden -ExecutionPolicy Bypass -File "' + (Join-Path $nativePath 'LaunchNative.ps1') + '"'
    $shortcut.WorkingDirectory = Split-Path $nativePath -Parent
    $shortcut.IconLocation = $latest.GamePath + ',0'
    $shortcut.Description = 'Launch the latest completed WILDLINE Windows build'
    $shortcut.Save()
}

function Get-LatestWildlineBuild {
    param([Parameter(Mandatory)][string]$BuildRoot)
    if (!(Test-Path -LiteralPath $BuildRoot -PathType Container)) { return }
    $candidates = foreach ($marker in (Get-ChildItem -LiteralPath $BuildRoot -Filter WildlineBuild.json -File -Recurse)) {
        # Installer staging copies are distributions, not developer builds.
        if ($marker.Directory.Name -ne 'Windows') { continue }
        $relative = $marker.FullName.Substring([IO.Path]::GetFullPath($BuildRoot).TrimEnd('\', '/').Length + 1)
        if ($relative -match '(^|[\\/])(InstallerStage|InstallerVerification|Beta)([\\/]|$)') { continue }
        try {
            $record = Get-Content -LiteralPath $marker.FullName -Raw | ConvertFrom-Json
            if ($record.schemaVersion -ne 1) { continue }
            $actual = @(Get-WildlinePackageFiles -PackageRoot $marker.Directory.FullName)
            $expected = @($record.files)
            if ($actual.Count -ne $expected.Count) { continue }
            $valid = $true
            for ($index = 0; $index -lt $actual.Count; $index++) {
                if ($actual[$index].path -cne $expected[$index].path -or
                    $actual[$index].length -ne $expected[$index].length -or
                    $actual[$index].modifiedUtcTicks -ne $expected[$index].modifiedUtcTicks) {
                    $valid = $false
                    break
                }
            }
            if (!$valid) { continue }
            [pscustomobject]@{
                PackageRoot = $marker.Directory.FullName
                GamePath = Join-Path $marker.Directory.FullName 'Wildline.exe'
                CompletedAtUtc = [DateTimeOffset]::Parse($record.completedAtUtc)
            }
        } catch {
            Write-Verbose "Skipping incomplete package $($marker.Directory.FullName): $_"
        }
    }
    $candidates | Sort-Object CompletedAtUtc -Descending | Select-Object -First 1
}
