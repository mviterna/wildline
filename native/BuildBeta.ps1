param(
    [switch]$SkipGameBuild,
    [string]$CompilerPath,
    [string]$EngineRoot = 'D:/EpicEngine/UE_5.8',
    [string]$PackageRoot
)
$ErrorActionPreference = 'Stop'
$taskInstaller = Join-Path $PSScriptRoot 'Installer'
if ($PackageRoot -and !$SkipGameBuild) { throw 'PackageRoot requires -SkipGameBuild; use LaunchNative.ps1 -Build -ArchiveRoot to build a custom archive.' }
if (!$PackageRoot) { $PackageRoot = Join-Path $PSScriptRoot 'Build/Windows' }
$taskPackage = [IO.Path]::GetFullPath($PackageRoot)
$taskOutput = Join-Path $PSScriptRoot 'Build/Beta'

if (!$SkipGameBuild) {
    # LaunchNative now builds both the game and installer. Delegate once;
    # its installer pass calls this script with -SkipGameBuild.
    $taskBuildArguments = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', (Join-Path $PSScriptRoot 'LaunchNative.ps1'), '-Build', '-EngineRoot', $EngineRoot)
    if ($CompilerPath) { $taskBuildArguments += @('-InstallerCompilerPath', $CompilerPath) }
    & powershell.exe @taskBuildArguments
    if ($LASTEXITCODE -ne 0) { throw "Game/installer build failed: $LASTEXITCODE" }
    exit 0
}
foreach ($taskRequired in @('Wildline.exe', 'Wildline/Binaries/Win64/Wildline.exe', 'Wildline/Content/Paks')) {
    if (!(Test-Path -LiteralPath (Join-Path $taskPackage $taskRequired))) { throw "Package is missing $taskRequired. Build the Windows game first." }
}
if (!$CompilerPath) {
    $taskCompilers = @(
        (Join-Path $PSScriptRoot 'Tools/InnoSetup/ISCC.exe'),
        'C:/Program Files (x86)/Inno Setup 6/ISCC.exe',
        'C:/Program Files/Inno Setup 7/ISCC.exe',
        (Join-Path $env:LOCALAPPDATA 'Programs/Inno Setup 6/ISCC.exe')
    )
    $CompilerPath = $taskCompilers | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
}
if (!$CompilerPath -or !(Test-Path -LiteralPath $CompilerPath)) {
    throw 'Install Inno Setup 6.3+ (https://jrsoftware.org/isdl.php), or pass -CompilerPath to ISCC.exe.'
}
$taskRedist = Join-Path $EngineRoot 'Engine/Extras/Redist/en-us'
foreach ($taskName in @('vc_redist.x64.exe', 'GameInputRedist.msi')) {
    $taskSignature = Get-AuthenticodeSignature (Join-Path $taskRedist $taskName)
    if ($taskSignature.Status -ne 'Valid' -or $taskSignature.SignerCertificate.Subject -notmatch 'Microsoft Corporation') {
        throw "Microsoft signature verification failed for $taskName"
    }
}
# Take the minimum versions from the engine that built the game, not the host PC.
$taskHeader = Get-Content -LiteralPath (Join-Path $EngineRoot 'Engine/Source/Runtime/ApplicationCore/Internal/Windows/WindowsRedistributableValidation.h') -Raw
function Get-MinimumRuntimeVersion([string]$Name) {
    $taskMatch = [regex]::Match($taskHeader, "$Name\s*=\s*\{\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+)\s*\}")
    if (!$taskMatch.Success) { throw "Cannot determine $Name from the engine." }
    return (($taskMatch.Groups | Select-Object -Skip 1 | ForEach-Object Value) -join '.')
}
$taskVCVersion = Get-MinimumRuntimeVersion 'MinRedistVersion'
$taskGameInputVersion = Get-MinimumRuntimeVersion 'MinGameInputRedistVersion'
$taskConfig = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'Unreal/Wildline/Config/DefaultGame.ini') -Raw
$taskProjectVersion = [regex]::Match($taskConfig, '(?m)^ProjectVersion=([\d.]+)').Groups[1].Value
if (!$taskProjectVersion) { throw 'ProjectVersion is missing.' }
$taskBetaVersion = "$taskProjectVersion-beta.$(Get-Date -Format 'yyyyMMdd.HHmmss')"
$taskSequence = [DateTimeOffset]::UtcNow.ToUnixTimeSeconds()
$taskLauncher = & (Join-Path $PSScriptRoot 'Launcher/BuildLauncher.ps1')
$taskStage = Join-Path $PSScriptRoot "Build/InstallerStage/$taskBetaVersion"
New-Item -ItemType Directory -Path $taskStage,$taskOutput -Force | Out-Null
$taskPackagePrefix = [IO.Path]::GetFullPath($taskPackage).TrimEnd('\') + '\'
foreach ($taskFile in Get-ChildItem -LiteralPath $taskPackage -File -Recurse) {
    $taskRelative = $taskFile.FullName.Substring($taskPackagePrefix.Length)
    if ($taskRelative -match '(?i)(^|\\)(Saved|Intermediate|DerivedDataCache)(\\|$)|\.pdb$|^Manifest_.*\.txt$|^Engine\\Extras\\Redist\\') { continue }
    $taskDestination = Join-Path $taskStage $taskRelative
    New-Item -ItemType Directory -Path (Split-Path $taskDestination) -Force | Out-Null
    Copy-Item -LiteralPath $taskFile.FullName -Destination $taskDestination
}
Copy-Item -LiteralPath (Join-Path $taskInstaller 'BetaReadme.txt') -Destination $taskStage
# Development builds otherwise keep Saved beside the executable. Unreal's
# installed-project marker makes every launch use the user's AppData instead.
$taskInstalledMarker = Join-Path $taskStage 'Engine/Build/InstalledProjectBuild.txt'
New-Item -ItemType Directory -Path (Split-Path $taskInstalledMarker) -Force | Out-Null
Set-Content -LiteralPath $taskInstalledMarker -Value 'WILDLINE Beta' -Encoding ASCII
foreach ($taskCredit in @(
    @('Farm/PROVENANCE.md', 'FarmAssetCredits.md'),
    @('Hunters/PROVENANCE.md', 'HunterAssetCredits.md'),
    @('Presentation/PROVENANCE.md', 'PresentationCredits.md')
)) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot ('Unreal/Wildline/ArtSource/' + $taskCredit[0])) -Destination (Join-Path $taskStage $taskCredit[1]) -Force
}
$taskBuildInfo = [ordered]@{
    version = $taskBetaVersion
    sequence = $taskSequence
    builtAt = (Get-Date).ToUniversalTime().ToString('o')
    configuration = 'Development'
    gitCommit = (& git -C $PSScriptRoot rev-parse HEAD)
    includesWorkingTreeChanges = [bool](& git -C $PSScriptRoot status --porcelain)
    visualCppMinimum = $taskVCVersion
    gameInputMinimum = $taskGameInputVersion
    files = @(Get-ChildItem -LiteralPath $taskStage -Recurse -File | ForEach-Object {
        [ordered]@{ path = $_.FullName.Substring($taskStage.Length + 1); sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }
    })
}
$taskBuildInfo | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $taskStage 'BetaBuild.json') -Encoding UTF8
$taskDefines = @{
    BetaVersion = $taskBetaVersion; PayloadPath = $taskStage; OutputPath = $taskOutput
    RedistPath = $taskRedist; VCVersion = $taskVCVersion; GameInputVersion = $taskGameInputVersion
    LauncherPath = $taskLauncher
}
$taskDefines.GetEnumerator() | ForEach-Object { '#define ' + $_.Key + ' "' + $_.Value + '"' } |
    Set-Content -LiteralPath (Join-Path $taskInstaller 'Generated.iss') -Encoding UTF8
& $CompilerPath (Join-Path $taskInstaller 'Wildline.iss')
if ($LASTEXITCODE -ne 0) { throw "Installer compiler failed: $LASTEXITCODE" }
$taskSetup = Join-Path $taskOutput "Wildline-$taskBetaVersion-Setup.exe"
Copy-Item -LiteralPath (Join-Path $taskInstaller 'BetaReadme.txt') -Destination $taskOutput -Force
$taskHash = Get-FileHash -LiteralPath $taskSetup -Algorithm SHA256
"$($taskHash.Hash)  $([IO.Path]::GetFileName($taskSetup))" | Set-Content -LiteralPath "$taskSetup.sha256" -Encoding ASCII
& $CompilerPath (Join-Path $taskInstaller 'Bootstrap.iss')
if ($LASTEXITCODE -ne 0) { throw "Bootstrap installer compiler failed: $LASTEXITCODE" }
$taskReceipt = [ordered]@{
    version = $taskBetaVersion; sequence = $taskSequence; setupPath = $taskSetup
    sha256 = $taskHash.Hash; size = (Get-Item -LiteralPath $taskSetup).Length
    bootstrapPath = (Join-Path $taskOutput 'WildlineSetup.exe')
    bootstrapSha256 = (Get-FileHash (Join-Path $taskOutput 'WildlineSetup.exe') -Algorithm SHA256).Hash
}
$taskReceipt | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $taskOutput 'LatestInstaller.json') -Encoding UTF8
Write-Output "Ready to send: $taskSetup"
