param([switch]$Editor,[switch]$Capture,[string]$GamePath,[string[]]$Maps=@('Blackwood','Frostwood','Farmstead','Neighborhood','NeighborhoodCedar'))
$ErrorActionPreference='Stop'
if(!$GamePath){
    if($Editor){$GamePath='D:/EpicEngine/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'}
    else{. (Join-Path $PSScriptRoot 'PackagedBuilds.ps1');$GamePath=(Get-LatestWildlineBuild -BuildRoot (Join-Path $PSScriptRoot 'Build')).GamePath}
}
if(!(Test-Path -LiteralPath $GamePath)){throw 'Build the selected game/editor first.'}
foreach($taskMap in $Maps){
    $taskProfile=Join-Path $PSScriptRoot "Unreal/Wildline/Saved/GameplayBugsVerification/$taskMap"
    $taskLog=Join-Path $PSScriptRoot "gameplay-bugs-$taskMap.log"
    $taskArgs=if($Editor){'"'+(Join-Path $PSScriptRoot 'Unreal/Wildline/Wildline.uproject')+'" -game '}else{''}
    $taskArgs+="/Game/Maps/$taskMap -WildlineGameplayBugsReview -WildlineNoIntro -unattended -NoSound -UserDir=`"$taskProfile`" -abslog=`"$taskLog`" "
    $taskRender=$Capture -and $taskMap -in @('Blackwood','Frostwood')
    $taskArgs+=if($taskRender){'-RenderOffscreen -windowed -ResX=1600 -ResY=900 -ForceRes -preferNvidia'}else{'-NullRHI -UseFixedTimeStep -FPS=60'}
    $taskProcess=Start-Process -FilePath $GamePath -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
    try{
        while(!$taskProcess.WaitForExit(1000)){if((Get-Date)-$taskProcess.StartTime -gt [TimeSpan]::FromMinutes(5)){throw "Gameplay bugs probe timed out: $taskMap"}}
        $taskText=Get-Content -LiteralPath $taskLog -Raw
        if($taskProcess.ExitCode -ne 0 -or $taskText -notmatch 'WILDLINE GAMEPLAY BUGS: PASS'){throw "Gameplay bugs probe failed: $taskMap. See $taskLog"}
        if($taskText -match 'LogMaterial: Error|LogShaderCompilers: Error|Fatal error'){throw "Render diagnostics failed: $taskMap"}
        if($taskRender){
            foreach($taskShot in @('RunningJump','TreeLeaves000','TreeLeaves025','TreeLeaves050','TreeLeaves075','TreeLeaves100','TreeLeavesSlider')){
                $taskSource=@("Saved/Screenshots/Windows/$taskShot.png","Wildline/Saved/Screenshots/Windows/$taskShot.png") | ForEach-Object {Join-Path $taskProfile $_} | Where-Object {Test-Path -LiteralPath $_} | ForEach-Object {Get-Item -LiteralPath $_} | Where-Object {$_.LastWriteTime -ge $taskProcess.StartTime} | Select-Object -First 1
                if(!$taskSource){throw "Missing fresh capture: $taskMap/$taskShot"}
                Copy-Item -LiteralPath $taskSource.FullName -Destination (Join-Path $PSScriptRoot "Preview/${taskMap}_$taskShot.png") -Force
            }
        }
        Write-Output "PASS $taskMap running jump, uneven-ground grace, leaf density, saved slider and unchanged wood"
    }finally{if(!$taskProcess.HasExited){Stop-Process -Id $taskProcess.Id -ErrorAction SilentlyContinue}}
}
