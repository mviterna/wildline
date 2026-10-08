param([switch]$Editor,[switch]$Build,[switch]$Test)
$ErrorActionPreference = 'Stop'
$projectPath = Join-Path $PSScriptRoot 'Unreal/Wildline/Wildline.uproject'
$packageRoot = Join-Path $PSScriptRoot 'Build/Windows'
$gamePath = Join-Path $packageRoot 'Wildline.exe'
if (!$Editor -and !$Build -and !$Test -and (Test-Path -LiteralPath $gamePath)) {
    Start-Process -FilePath $gamePath -ArgumentList '-preferNvidia' -WorkingDirectory $packageRoot -WindowStyle Normal
    exit
}
$engineCandidates = @('D:/EpicEngine/UE_5.8', 'D:/Epic/UE_5.8', 'C:/Program Files/Epic Games/UE_5.8')
$installManifest = Join-Path $env:ProgramData 'Epic/UnrealEngineLauncher/LauncherInstalled.dat'
if (Test-Path -LiteralPath $installManifest) {
    $engineCandidates += (Get-Content -LiteralPath $installManifest -Raw | ConvertFrom-Json).InstallationList | Where-Object AppName -eq 'UE_5.8' | ForEach-Object InstallLocation
}
$engineRoot = $engineCandidates | Where-Object { Test-Path -LiteralPath (Join-Path $_ 'Engine/Binaries/Win64/UnrealEditor.exe') } | Select-Object -First 1
if (!$engineRoot) { throw 'Unreal Engine 5.8 was not found. Install it through Epic Games Launcher, or add its path to native/LaunchNative.ps1.' }
if ($Build) {
    & (Join-Path $engineRoot 'Engine/Build/BatchFiles/RunUAT.bat') BuildCookRun "-project=$projectPath" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -iostore -archive "-archivedirectory=$(Join-Path $PSScriptRoot 'Build')" '-UbtArgs=-MaxParallelActions=4' -unattended
    if($LASTEXITCODE -eq 0){Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Unreal/Wildline/ArtSource/Farm/PROVENANCE.md') -Destination (Join-Path $packageRoot 'FarmAssetCredits.md') -Force}
    if($LASTEXITCODE -eq 0){
        Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Unreal/Wildline/ArtSource/Hunters/PROVENANCE.md') -Destination (Join-Path $packageRoot 'HunterAssetCredits.md') -Force
        Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Unreal/Wildline/ArtSource/Hunters/UPGRADE_CANDIDATES.md') -Destination (Join-Path $packageRoot 'FreeHunterAssets.md') -Force
    }
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
