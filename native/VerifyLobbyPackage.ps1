param([switch]$Capture,[switch]$Editor,[switch]$SkipSmoke,[switch]$CaptureOnly)
$ErrorActionPreference='Stop'
$taskProject=Join-Path $PSScriptRoot 'Unreal/Wildline/Wildline.uproject'
$taskExe=if($Editor){'D:/EpicEngine/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'}else{Join-Path $PSScriptRoot 'Build/Windows/Wildline/Binaries/Win64/Wildline.exe'}
if(!(Test-Path -LiteralPath $taskExe)){throw 'Build the native game first.'}
$taskProcesses=@()
function Start-LobbyProbe([string]$Flags,[string]$Name){
    $taskProfile=Join-Path $PSScriptRoot "Unreal/Wildline/Saved/LobbyProbes/$Name"
    $taskArgs=if($Editor){'"'+$taskProject+'" -game '}else{''}
    $taskArgs+=$Flags+' -WildlineNoIntro -unattended -UserDir="'+$taskProfile+'" -abslog="'+(Join-Path $PSScriptRoot "lobby-$Name.log")+'"'
    Start-Process -FilePath $taskExe -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
}
try{
    # Keep memory bounded: the three standalone mode checks run one at a time.
    if(!$SkipSmoke -and !$CaptureOnly){foreach($taskMode in @(@('Blackwood','classic'),@('Farmstead','farm'),@('Neighborhood','hill'))){
        $taskP=Start-LobbyProbe ('/Game/Maps/'+$taskMode[0]+' -WildlineLobbySmoke -NullRHI -NoSound') $taskMode[1];$taskProcesses+=$taskP
        while(!$taskP.WaitForExit(1000)){if((Get-Date)-$taskP.StartTime -gt [TimeSpan]::FromMinutes(3)){throw 'Lobby smoke timed out.'}}
        $taskLog=Get-Content (Join-Path $PSScriptRoot ('lobby-'+$taskMode[1]+'.log')) -Raw
        if($taskP.ExitCode -ne 0 -or $taskLog -notmatch 'WILDLINE LOBBY SMOKE: PASS'){throw "Lobby smoke failed: $($taskMode[1])"}
    }}
    if(!$CaptureOnly){
    $taskHost=Start-LobbyProbe '/Game/Maps/Blackwood?listen -port=7798 -WildlineLobbyNetProbe -NullRHI -NoSound' 'net-host';$taskProcesses+=$taskHost
    # Give the packaged host time to listen; the readiness assertion itself waits
    # for the client's replicated room entry instead of assuming connection timing.
    Start-Sleep -Seconds 20
    $taskClient=Start-LobbyProbe '127.0.0.1:7798 -WildlineLobbyNetProbe -NullRHI -NoSound' 'net-client';$taskProcesses+=$taskClient
    foreach($taskP in @($taskHost,$taskClient)){
        while(!$taskP.WaitForExit(1000)){if((Get-Date)-$taskP.StartTime -gt [TimeSpan]::FromMinutes(3)){throw 'Lobby network probe timed out.'}}
        if($taskP.ExitCode -ne 0){throw "Lobby network probe failed: $($taskP.Id)"}
    }
    foreach($taskName in @('net-host','net-client')){if((Get-Content (Join-Path $PSScriptRoot "lobby-$taskName.log") -Raw) -notmatch 'WILDLINE LOBBY NETWORK (Host|Client): PASS'){throw "Missing network pass: $taskName"}}
    }
    if($Capture -or $CaptureOnly){
        $taskP=Start-LobbyProbe '/Game/Maps/Blackwood -WildlineLobbyReview -RenderOffscreen -windowed -ResX=1600 -ResY=900 -ForceRes -preferNvidia' 'review';$taskProcesses+=$taskP
        while(!$taskP.WaitForExit(1000)){if((Get-Date)-$taskP.StartTime -gt [TimeSpan]::FromMinutes(3)){throw 'Lobby review timed out.'}}
        if($taskP.ExitCode -ne 0){throw 'Lobby render review failed.'}
        $taskLog=Get-Content (Join-Path $PSScriptRoot 'lobby-review.log') -Raw
        if($taskLog -match 'LogMaterial: Error|LogShaderCompilers: Error|Failed to compile Material|Missing waiting room model|Fatal error'){throw 'Lobby render has material or asset errors.'}
        foreach($taskName in @('LobbyHunterPlayer.png','LobbyHunterRoom.png','LobbyTrophies.png','LobbySquirrelPlayer.png','LobbySquirrelRoom.png','LobbyWeaponRack.png','LobbyAcornPantry.png','LobbyHunterWindows.png')){
            $taskShot=Join-Path $PSScriptRoot "Unreal/Wildline/Saved/LobbyProbes/review/Saved/Screenshots/Windows/$taskName"
            if(!(Test-Path -LiteralPath $taskShot)){throw "Missing screenshot: $taskName"}
            if((Get-Item -LiteralPath $taskShot).LastWriteTime -lt $taskP.StartTime){throw "Stale screenshot: $taskName"}
            Copy-Item -LiteralPath $taskShot -Destination (Join-Path $PSScriptRoot "Preview/$taskName") -Force
        }
    }
    if($CaptureOnly){Write-Output 'Lobby visual review: PASS'}
    else{Write-Output 'Waiting rooms, equipment, readiness, protection, all three modes, late entry, restart and co-op replication: PASS'}
}finally{foreach($taskP in $taskProcesses){if(!$taskP.HasExited){Stop-Process -Id $taskP.Id -ErrorAction SilentlyContinue}}}
