param([switch]$Editor,[switch]$Capture,[switch]$CaptureOnly,[switch]$Net,[string]$GamePath,[string[]]$Maps=@('Blackwood','Farmstead','Neighborhood','NeighborhoodCedar','Frostwood'))
$ErrorActionPreference='Stop'
if($CaptureOnly){$Capture=$true}
if(!$GamePath){$GamePath=if($Editor){'D:/EpicEngine/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'}else{. (Join-Path $PSScriptRoot 'PackagedBuilds.ps1');(Get-LatestWildlineBuild -BuildRoot (Join-Path $PSScriptRoot 'Build')).GamePath}}
if(!(Test-Path -LiteralPath $GamePath)){throw 'Build the selected game/editor first.'}
$taskProject=Join-Path $PSScriptRoot 'Unreal/Wildline/Wildline.uproject'
foreach($taskMap in $Maps){
    $taskKinds=if($CaptureOnly){@('review')}elseif($Capture){@('smoke','review')}else{@('smoke')}
    foreach($taskKind in $taskKinds){
        $taskProfile=Join-Path $PSScriptRoot "Unreal/Wildline/Saved/WorldUpgradeVerification/$taskMap-$taskKind"
        $taskLog=Join-Path $PSScriptRoot "world-upgrade-$taskMap-$taskKind.log"
        $taskArgs=if($Editor){'"'+$taskProject+'" -game '}else{''}
        $taskArgs+="/Game/Maps/$taskMap -unattended -NoSound -UserDir=`"$taskProfile`" -abslog=`"$taskLog`" "
        $taskArgs+=if($taskKind -eq 'smoke'){'-WildlineTraversalSmoke -NullRHI -UseFixedTimeStep -FPS=60'}else{'-WildlineWorldUpgradeReview -RenderOffscreen -windowed -ResX=1600 -ResY=900 -ForceRes -preferNvidia'}
        $taskProcess=Start-Process -FilePath $GamePath -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
        try{
            while(!$taskProcess.WaitForExit(30000)){if((Get-Date)-$taskProcess.StartTime -gt [TimeSpan]::FromMinutes(7)){throw "Timed out: $taskMap/$taskKind"}}
            if($taskProcess.ExitCode -ne 0){throw "Probe failed: $taskMap/$taskKind. See $taskLog"}
            $taskText=Get-Content -LiteralPath $taskLog -Raw
            if($taskKind -eq 'smoke' -and $taskText -notmatch 'WILDLINE TRAVERSAL: PASS'){throw "Missing pass: $taskMap"}
            if($taskKind -eq 'review'){
                if($taskText -match 'LogMaterial: Error|LogShaderCompilers: Error|Fatal error'){throw "Render diagnostics failed: $taskMap"}
                $taskShots=if($taskMap -like 'Neighborhood*'){@('UpgradeCars','UpgradeNeighborhood')}else{@('UpgradeReserve','UpgradeBranch')}
                $taskShots+=@('UpgradeHunterTrophies','UpgradeSquirrelDen','UpgradeLodgeWoods')
                foreach($taskShot in $taskShots){
                    $taskCandidates=@("Saved/Screenshots/Windows/$taskShot.png","Wildline/Saved/Screenshots/Windows/$taskShot.png") | ForEach-Object {Join-Path $taskProfile $_} | Where-Object {Test-Path -LiteralPath $_} | ForEach-Object {Get-Item -LiteralPath $_} | Where-Object {$_.LastWriteTime -ge $taskProcess.StartTime}
                    $taskSource=$taskCandidates | Select-Object -First 1
                    if(!$taskSource){throw "Missing fresh image: $taskMap/$taskShot"}
                    Copy-Item -LiteralPath $taskSource.FullName -Destination (Join-Path $PSScriptRoot "Preview/${taskMap}_$taskShot.png") -Force
                }
            }
            Write-Output "PASS $taskMap/$taskKind"
        }finally{if(!$taskProcess.HasExited){Stop-Process -Id $taskProcess.Id -ErrorAction SilentlyContinue}}
    }
}
if($Net){
    $taskPeers=@()
    try{
        foreach($taskRole in @('host','client')){
            $taskProfile=Join-Path $PSScriptRoot "Unreal/Wildline/Saved/WorldUpgradeVerification/net-$taskRole"
            $taskLog=Join-Path $PSScriptRoot "world-upgrade-net-$taskRole.log"
            $taskArgs=if($Editor){'"'+$taskProject+'" -game '}else{''}
            $taskArgs+=if($taskRole -eq 'host'){'/Game/Maps/Frostwood?listen -port=7805 '}else{'127.0.0.1:7805 '}
            $taskArgs+="-WildlineTraversalNetProbe -WildlineNoIntro -NullRHI -NoSound -unattended -UserDir=`"$taskProfile`" -abslog=`"$taskLog`""
            $taskPeers+=Start-Process -FilePath $GamePath -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
            if($taskRole -eq 'host'){Start-Sleep -Seconds 20}
        }
        foreach($taskPeer in $taskPeers){while(!$taskPeer.WaitForExit(30000)){if((Get-Date)-$taskPeer.StartTime -gt [TimeSpan]::FromMinutes(5)){throw 'Traversal network probe timed out'}};if($taskPeer.ExitCode -ne 0){throw 'Traversal network probe failed'}}
        foreach($taskRole in @('host','client')){if((Get-Content (Join-Path $PSScriptRoot "world-upgrade-net-$taskRole.log") -Raw) -notmatch "WILDLINE TRAVERSAL NET ${taskRole}: PASS"){throw "Missing network pass: $taskRole"}}
        Write-Output 'PASS network: client grip RPC, branch movement, surface replication and jump release in Frostwood'
    }finally{foreach($taskPeer in $taskPeers){if(!$taskPeer.HasExited){Stop-Process -Id $taskPeer.Id -ErrorAction SilentlyContinue}}}
}
