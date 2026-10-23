$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'PackagedBuilds.ps1')
$fixture = Join-Path $env:TEMP ('wildline-cleanup-' + [guid]::NewGuid().ToString('N'))
$root = Join-Path $fixture 'Build'
$cleanup = Join-Path $PSScriptRoot 'CleanBuildOutputs.ps1'

function Write-FixtureFile([string]$Path, [string]$Value = 'fixture') {
    New-Item -ItemType Directory -Path (Split-Path $Path) -Force | Out-Null
    Set-Content -LiteralPath $Path -Value $Value
}
function Assert-Fixture([bool]$Condition, [string]$Message) {
    if (!$Condition) { throw $Message }
}
function Assert-CleanupRefused([string]$Expected) {
    $failure = $null
    try { & $cleanup -BuildRoot $root } catch { $failure = $_.Exception.Message }
    Assert-Fixture ($failure -like "*$Expected*") "Expected cleanup to refuse '$Expected'; received '$failure'."
    Assert-Fixture (Test-Path -LiteralPath (Join-Path $root 'Obsolete/sentinel.txt')) 'Refused cleanup deleted a target.'
}

try {
    foreach ($name in @('Old', 'Current', 'Incomplete')) {
        $package = Join-Path $root "$name/Windows"
        foreach ($relative in @('Wildline.exe', 'Wildline/Binaries/Win64/Wildline.exe', 'Wildline/Content/Paks/game.pak', 'Wildline/Content/Paks/game.utoc', 'Wildline/Content/Paks/game.ucas')) {
            Write-FixtureFile (Join-Path $package $relative)
        }
        $completed = if ($name -eq 'Old') { [DateTimeOffset]::UtcNow.AddDays(-1) } else { [DateTimeOffset]::UtcNow }
        Publish-WildlineBuild -PackageRoot $package -CompletedAtUtc $completed -SkipDesktopShortcut | Out-Null
    }
    Write-FixtureFile (Join-Path $root 'Incomplete/Windows/Wildline.exe') 'changed after publication'
    Write-FixtureFile (Join-Path $root 'Current/Windows/Wildline/Saved/SaveGames/personal.sav') 'preserve my save'
    Write-FixtureFile (Join-Path $fixture 'Source/asset.txt') 'preserve source'
    Write-FixtureFile (Join-Path $root 'InstallerStage/old/payload.bin')
    $version = '0.2.0-beta.20261020.162327'
    $setup = Join-Path $root "Beta/Wildline-$version-Setup.exe"
    $bootstrap = Join-Path $root 'Beta/WildlineSetup.exe'
    Write-FixtureFile $setup
    Write-FixtureFile $bootstrap
    Write-FixtureFile (Join-Path $root 'Beta/Wildline-old-Setup.exe')
    Write-FixtureFile (Join-Path $root 'Beta/OldCandidateBootstrap.exe')
    Write-FixtureFile (Join-Path $root 'Beta/Wildline-old-OnlineSetup.exe')
    Write-FixtureFile (Join-Path $root "Updates/$version/current.part000")
    Write-FixtureFile (Join-Path $root 'Updates/old/obsolete.part000')
    $receipt = @{
        version = $version; setupPath = $setup; bootstrapPath = $bootstrap
        size = (Get-Item -LiteralPath $setup).Length
        sha256 = (Get-FileHash -LiteralPath $setup).Hash
        bootstrapSha256 = (Get-FileHash -LiteralPath $bootstrap).Hash
    }
    $receiptPath = Join-Path $root 'Beta/LatestInstaller.json'
    $receipt | ConvertTo-Json | Set-Content -LiteralPath $receiptPath
    & $cleanup -BuildRoot $root -WhatIf
    Assert-Fixture (Test-Path -LiteralPath (Join-Path $root 'Old/Windows/Wildline.exe')) 'Dry run removed files.'
    & $cleanup -BuildRoot $root
    foreach ($relative in @('Old', 'Incomplete', 'InstallerStage', 'Updates/old', 'Beta/Wildline-old-Setup.exe', 'Beta/OldCandidateBootstrap.exe', 'Beta/Wildline-old-OnlineSetup.exe')) {
        Assert-Fixture (!(Test-Path -LiteralPath (Join-Path $root $relative))) "Obsolete output remains: $relative"
    }
    foreach ($path in @($setup, $bootstrap, (Join-Path $root "Updates/$version/current.part000"), (Join-Path $root 'Current/Windows/Wildline/Saved/SaveGames/personal.sav'), (Join-Path $fixture 'Source/asset.txt'))) {
        Assert-Fixture (Test-Path -LiteralPath $path) "Required file was removed: $path"
    }
    & $cleanup -BuildRoot $root
    Write-Output 'PASS dry run, current build selection, obsolete output removal, current distributables, save/source preservation, and repeat cleanup.'

    Write-FixtureFile (Join-Path $root 'Obsolete/sentinel.txt')
    $receipt.sha256 = 'invalid'
    $receipt | ConvertTo-Json | Set-Content -LiteralPath $receiptPath
    Assert-CleanupRefused 'integrity check failed'
    $receipt.sha256 = (Get-FileHash -LiteralPath $setup).Hash
    $receipt | ConvertTo-Json | Set-Content -LiteralPath $receiptPath
    Write-FixtureFile (Join-Path $root 'Current/Windows/Wildline.exe') 'incomplete rebuild'
    Assert-CleanupRefused 'No valid completed build'
    Publish-WildlineBuild -PackageRoot (Join-Path $root 'Current/Windows') -SkipDesktopShortcut | Out-Null
    Write-FixtureFile (Join-Path $root 'Obsolete/SaveGames/personal.sav')
    Assert-CleanupRefused 'contains saves'
    $saveFixture = [IO.Path]::GetFullPath((Join-Path $root 'Obsolete/SaveGames'))
    Assert-Fixture ($saveFixture.StartsWith($fixture + '\', [StringComparison]::OrdinalIgnoreCase)) 'Unsafe fixture path.'
    Remove-Item -LiteralPath $saveFixture -Recurse -Force
    $junction = Join-Path $root 'Obsolete/linked-source'
    New-Item -ItemType Junction -Path $junction -Target (Join-Path $fixture 'Source') | Out-Null
    Assert-CleanupRefused 'filesystem link'
    [IO.Directory]::Delete([IO.Path]::GetFullPath($junction))
    Write-Output 'PASS corrupt installer, incomplete build, old saves, and junction guards stop cleanup before any deletion.'
} finally {
    $fullFixture = [IO.Path]::GetFullPath($fixture)
    $tempPrefix = [IO.Path]::GetFullPath($env:TEMP).TrimEnd('\') + '\'
    if (!$fullFixture.StartsWith($tempPrefix, [StringComparison]::OrdinalIgnoreCase) -or (Split-Path $fullFixture -Leaf) -notlike 'wildline-cleanup-*') { throw 'Unsafe fixture cleanup path.' }
    $remainingLink = Join-Path $fullFixture 'Build/Obsolete/linked-source'
    if (Test-Path -LiteralPath $remainingLink) { [IO.Directory]::Delete([IO.Path]::GetFullPath($remainingLink)) }
    if (Test-Path -LiteralPath $fullFixture) { Remove-Item -LiteralPath $fullFixture -Recurse -Force }
}
