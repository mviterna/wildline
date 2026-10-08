param([switch]$Capture,[switch]$Editor)
$ErrorActionPreference='Stop'
$taskProject=Join-Path $PSScriptRoot 'Unreal/Wildline/Wildline.uproject'
$taskGame=Join-Path $PSScriptRoot 'Build/Windows/Wildline/Binaries/Win64/Wildline.exe'
if($Editor){$taskGame='D:/EpicEngine/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'}
if(!(Test-Path -LiteralPath $taskGame)){throw 'Build the game or editor first.'}
$taskProcesses=@()
$taskChecks=@()
function Start-ConfigProbe($taskFlags,$taskName){
    $taskProfile=Join-Path $PSScriptRoot "Unreal/Wildline/Saved/ConfigProbes/$taskName"
    $taskLog=Join-Path $PSScriptRoot "$taskName.log"
    $taskArgs=if($Editor){'"'+$taskProject+'" -game '}else{''}
    $taskArgs+=$taskFlags+' -unattended -UserDir="'+$taskProfile+'" -abslog="'+$taskLog+'"'
    Start-Process -FilePath $taskGame -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
}
try {
    foreach($taskMap in @('Blackwood','Farmstead','Neighborhood')){
        $taskName="config-$taskMap-package"
        $taskProcesses+=Start-ConfigProbe "/Game/Maps/$taskMap -WildlineConfigSmoke -NullRHI -NoSound" $taskName
        $taskChecks+=,@($taskName,'WILDLINE CONFIG SMOKE: PASS')
    }
    $taskProcesses+=Start-ConfigProbe '/Game/Maps/Blackwood?listen -port=7798 -WildlineAutoStart -WildlineConfigNetProbe -NullRHI -NoSound' 'config-net-host'
    $taskChecks+=,@('config-net-host','WILDLINE CONFIG NETWORK Host: PASS')
    Start-Sleep -Seconds 5
    $taskProcesses+=Start-ConfigProbe '127.0.0.1:7798 -WildlineAutoStart -WildlineSquirrel -WildlineConfigNetProbe -NullRHI -NoSound' 'config-net-client'
    $taskChecks+=,@('config-net-client','WILDLINE CONFIG NETWORK Client: PASS')
    if($Capture){$taskProcesses+=Start-ConfigProbe '/Game/Maps/Blackwood -WildlineConfigReview -RenderOffscreen -windowed -ResX=1600 -ResY=900 -ForceRes -preferNvidia' 'config-package-review'}
    foreach($taskProcess in $taskProcesses){if(!$taskProcess.WaitForExit(60000) -and !$taskProcess.WaitForExit(60000)){throw "Probe timed out: $($taskProcess.Id)"};if($taskProcess.ExitCode -ne 0){throw "Probe failed: $($taskProcess.Id), exit $($taskProcess.ExitCode)"}}
    foreach($taskCheck in $taskChecks){if((Get-Content -LiteralPath (Join-Path $PSScriptRoot ($taskCheck[0]+'.log')) -Raw) -notmatch $taskCheck[1]){throw "Missing pass marker: $($taskCheck[0])"}}
    if($Capture){
        $taskShots=Join-Path $PSScriptRoot 'Unreal/Wildline/Saved/ConfigProbes/config-package-review/Saved/Screenshots/Windows'
        foreach($taskName in @('ConfigRules.png','ConfigAdvanced.png','ConfigBots.png','ConfigSettings.png')){Copy-Item -LiteralPath (Join-Path $taskShots $taskName) -Destination (Join-Path $PSScriptRoot "Preview/$taskName") -Force}
    }
    Write-Output 'Custom rules, exact bot rosters, save/reload, all three modes and host/client replication: PASS'
} finally {
    foreach($taskProcess in $taskProcesses){if(!$taskProcess.HasExited){Stop-Process -Id $taskProcess.Id -ErrorAction SilentlyContinue}}
}
