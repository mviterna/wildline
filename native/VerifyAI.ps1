param([switch]$Editor,[switch]$Capture,[string]$GamePath,[string[]]$Maps=@('Blackwood','Frostwood','Farmstead','Neighborhood','NeighborhoodCedar'))
$ErrorActionPreference='Stop'
if(!$GamePath){
    if($Editor){$GamePath='D:/EpicEngine/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'}
    else{. (Join-Path $PSScriptRoot 'PackagedBuilds.ps1');$taskBuild=Get-LatestWildlineBuild -BuildRoot (Join-Path $PSScriptRoot 'Build');$GamePath=Join-Path $taskBuild.PackageRoot 'Wildline/Binaries/Win64/Wildline.exe'}
}
if(!(Test-Path -LiteralPath $GamePath)){throw 'Build the selected game/editor first.'}
$taskResults=@()
foreach($taskMap in $Maps){
    $taskProfile=Join-Path $PSScriptRoot "Unreal/Wildline/Saved/AIVerification/$taskMap"
    $taskLog=Join-Path $PSScriptRoot "ai-$taskMap.log"
    $taskArgs=if($Editor){'"'+(Join-Path $PSScriptRoot 'Unreal/Wildline/Wildline.uproject')+'" -game '}else{''}
    $taskArgs+="/Game/Maps/$taskMap -WildlineAIReview -WildlineNoIntro -unattended -NoSound -UserDir=`"$taskProfile`" -abslog=`"$taskLog`" "
    $taskRender=$Capture -and $taskMap -eq 'Blackwood'
    $taskArgs+=if($taskRender){'-RenderOffscreen -windowed -ResX=1600 -ResY=900 -ForceRes -preferNvidia'}else{'-NullRHI -UseFixedTimeStep -FPS=60'}
    $taskProcess=Start-Process -FilePath $GamePath -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
    try{
        while(!$taskProcess.WaitForExit(1000)){if((Get-Date)-$taskProcess.StartTime -gt [TimeSpan]::FromMinutes(6)){throw "AI probe timed out: $taskMap"}}
        $taskText=Get-Content -LiteralPath $taskLog -Raw
        if($taskProcess.ExitCode -ne 0 -or $taskText -notmatch 'WILDLINE AI REVIEW: PASS' -or $taskText -match 'AI CHECK FAIL'){throw "AI probe failed: $taskMap. See $taskLog"}
        if($taskRender){
            foreach($taskShot in @('AIBehavior','AIRoster')){
                $taskSource=@("Saved/Screenshots/Windows/$taskShot.png","Wildline/Saved/Screenshots/Windows/$taskShot.png") | ForEach-Object {Join-Path $taskProfile $_} | Where-Object {Test-Path -LiteralPath $_} | ForEach-Object {Get-Item -LiteralPath $_} | Where-Object {$_.LastWriteTime -ge $taskProcess.StartTime} | Select-Object -First 1
                if(!$taskSource){throw "Missing fresh capture: $taskShot"}
                Copy-Item -LiteralPath $taskSource.FullName -Destination (Join-Path $PSScriptRoot "Preview/$taskShot.png") -Force
            }
        }
        $taskResults+=@{map=$taskMap;passed=$true;log=$taskLog;checks=@($taskText -split "`n" | Where-Object {$_ -match 'AI CHECK|AI PATROL|AI FORAGE|AI FOOD'})}
        Write-Output "PASS $taskMap patrol, eight-squirrel movement, goals, cover, search, tree recovery and saved controls"
    }finally{if(!$taskProcess.HasExited){Stop-Process -Id $taskProcess.Id -ErrorAction SilentlyContinue}}
}
@{game=$GamePath;gameSha256=(Get-FileHash -LiteralPath $GamePath -Algorithm SHA256).Hash;editor=[bool]$Editor;verifiedAt=(Get-Date).ToString('o');results=$taskResults} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'ai-verification.json')
