param([switch]$Editor,[switch]$Build,[switch]$Test,[string]$EngineRoot,[string]$InstallerCompilerPath,[string]$ArchiveRoot,[switch]$SkipInstaller,[switch]$SkipEditorBuild)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'PackagedBuilds.ps1')
$projectPath = Join-Path $PSScriptRoot 'Unreal/Wildline/Wildline.uproject'
$buildRoot = Join-Path $PSScriptRoot 'Build'
if (!$ArchiveRoot) { $ArchiveRoot = $buildRoot }
$ArchiveRoot = [IO.Path]::GetFullPath($ArchiveRoot)
if ($Build) {
    $buildPrefix = [IO.Path]::GetFullPath($buildRoot).TrimEnd('\', '/') + '\'
    if ($ArchiveRoot -ne $buildPrefix.TrimEnd('\') -and !$ArchiveRoot.StartsWith($buildPrefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'ArchiveRoot must be inside native/Build so the desktop launcher can discover it.'
    }
    $archiveRelative = $ArchiveRoot.Substring($buildPrefix.TrimEnd('\').Length)
    if ($archiveRelative -match '(^|[\\/])(InstallerStage|InstallerVerification|Beta)([\\/]|$)') {
        throw 'ArchiveRoot cannot use an installer output directory.'
    }
}
$packageRoot = Join-Path $ArchiveRoot 'Windows'
if (!$Editor -and !$Build -and !$Test) {
    $latestBuild = Get-LatestWildlineBuild -BuildRoot (Join-Path $PSScriptRoot 'Build')
    if ($latestBuild) {
        Start-Process -FilePath $latestBuild.GamePath -ArgumentList '-preferNvidia' -WorkingDirectory $latestBuild.PackageRoot -WindowStyle Normal
        exit
    }
}
$engineCandidates = @($EngineRoot, 'D:/EpicEngine/UE_5.8', 'D:/Epic/UE_5.8', 'C:/Program Files/Epic Games/UE_5.8') | Where-Object { $_ }
$installManifest = Join-Path $env:ProgramData 'Epic/UnrealEngineLauncher/LauncherInstalled.dat'
if (Test-Path -LiteralPath $installManifest) {
    $engineCandidates += (Get-Content -LiteralPath $installManifest -Raw | ConvertFrom-Json).InstallationList | Where-Object AppName -eq 'UE_5.8' | ForEach-Object InstallLocation
}
$engineRoot = $engineCandidates | Where-Object { Test-Path -LiteralPath (Join-Path $_ 'Engine/Binaries/Win64/UnrealEditor.exe') } | Select-Object -First 1
if (!$engineRoot) { throw 'Unreal Engine 5.8 was not found. Install it through Epic Games Launcher, or add its path to native/LaunchNative.ps1.' }
if ($Build) {
    # Retire the previous completion record before updating this archive in place.
    $completionRecord = Join-Path $packageRoot 'WildlineBuild.json'
    if (Test-Path -LiteralPath $completionRecord) { Remove-Item -LiteralPath $completionRecord }
    # Opt in only after compiling the current editor module separately. This
    # lets packaging proceed while another editor has Live Coding enabled.
    $editorBuildArguments = @()
    if ($SkipEditorBuild) { $editorBuildArguments += '-nocompileeditor' }
    & (Join-Path $engineRoot 'Engine/Build/BatchFiles/RunUAT.bat') BuildCookRun "-project=$projectPath" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -iostore -prereqs -archive "-archivedirectory=$ArchiveRoot" '-UbtArgs=-MaxParallelActions=4' -unattended @editorBuildArguments
    $buildExitCode = $LASTEXITCODE
    if ($buildExitCode -ne 0) { exit $buildExitCode }
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Unreal/Wildline/ArtSource/Farm/PROVENANCE.md') -Destination (Join-Path $packageRoot 'FarmAssetCredits.md') -Force
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Unreal/Wildline/ArtSource/Hunters/PROVENANCE.md') -Destination (Join-Path $packageRoot 'HunterAssetCredits.md') -Force
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Unreal/Wildline/ArtSource/Hunters/UPGRADE_CANDIDATES.md') -Destination (Join-Path $packageRoot 'FreeHunterAssets.md') -Force
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Unreal/Wildline/ArtSource/Presentation/PROVENANCE.md') -Destination (Join-Path $packageRoot 'PresentationCredits.md') -Force
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Unreal/Wildline/ArtSource/Lobby/PROVENANCE.md') -Destination (Join-Path $packageRoot 'LobbyAssetCredits.md') -Force
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Unreal/Wildline/ArtSource/Neighborhood/PROVENANCE.md') -Destination (Join-Path $packageRoot 'NeighborhoodAssetCredits.md') -Force
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Unreal/Wildline/ArtSource/Squirrel/PROVENANCE.md') -Destination (Join-Path $packageRoot 'SquirrelAssetCredits.md') -Force
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Unreal/Wildline/ArtSource/Lake/PROVENANCE.md') -Destination (Join-Path $packageRoot 'LakeAssetCredits.md') -Force
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Unreal/Wildline/ArtSource/Audio/PROVENANCE.md') -Destination (Join-Path $packageRoot 'AudioCredits.md') -Force
    Publish-WildlineBuild -PackageRoot $packageRoot | Out-Null
    if ($SkipInstaller) {
        & (Join-Path $PSScriptRoot 'CleanBuildOutputs.ps1')
        exit 0
    }
    # Every packaged build also produces the distributable installer. The
    # staging-only switch prevents BuildBeta from starting another game build.
    $installerArguments = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', (Join-Path $PSScriptRoot 'BuildBeta.ps1'), '-SkipGameBuild', '-EngineRoot', $engineRoot, '-PackageRoot', $packageRoot)
    if ($InstallerCompilerPath) { $installerArguments += @('-CompilerPath', $InstallerCompilerPath) }
    & powershell.exe @installerArguments
    exit $LASTEXITCODE
}
if ($Test) {
    & (Join-Path $engineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') $projectPath -unattended -NullRHI -NoSound '-ExecCmds=Automation RunTests Wildline;Quit' '-TestExit=Automation Test Queue Empty'
    if ($LASTEXITCODE) { exit $LASTEXITCODE }
    $taskSmokeResult = Join-Path (Split-Path $projectPath) 'Saved/SmokeResult.txt'
    Set-Content -LiteralPath $taskSmokeResult -Value 'RUNNING'
    & (Join-Path $engineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') $projectPath -game /Game/Maps/Blackwood -WildlineSmoke -unattended -NullRHI -NoSound
    if ($LASTEXITCODE) { exit $LASTEXITCODE }
    if ((Get-Content -LiteralPath $taskSmokeResult -Raw).Trim() -ne 'PASS') { exit 1 }
    $taskFarmResult = Join-Path (Split-Path $projectPath) 'Saved/FarmSmokeResult.txt'
    Set-Content -LiteralPath $taskFarmResult -Value 'RUNNING'
    & (Join-Path $engineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') $projectPath -game /Game/Maps/Farmstead -WildlineFarmSmoke -unattended -NullRHI -NoSound
    if ($LASTEXITCODE) { exit $LASTEXITCODE }
    if ((Get-Content -LiteralPath $taskFarmResult -Raw).Trim() -ne 'PASS') { exit 1 }
    $taskHillResult = Join-Path (Split-Path $projectPath) 'Saved/HillSmokeResult.txt'
    Set-Content -LiteralPath $taskHillResult -Value 'RUNNING'
    & (Join-Path $engineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') $projectPath -game /Game/Maps/Neighborhood -WildlineHillSmoke -unattended -NullRHI -NoSound
    if ($LASTEXITCODE) { exit $LASTEXITCODE }
    if ((Get-Content -LiteralPath $taskHillResult -Raw).Trim() -ne 'PASS') { exit 1 }
    exit 0
}
$argsForEngine = @('"' + $projectPath + '"')
if (!$Editor) { $argsForEngine += @('-game','-preferNvidia') }
Start-Process -FilePath (Join-Path $engineRoot 'Engine/Binaries/Win64/UnrealEditor.exe') -ArgumentList $argsForEngine -WorkingDirectory (Split-Path $projectPath)
