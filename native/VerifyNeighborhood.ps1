param([switch]$Editor,[switch]$Capture,[switch]$CaptureOnly,[string]$GamePath,[ValidateSet('Both','Neighborhood','NeighborhoodCedar')][string]$Map='Both',[ValidateRange(-1,1)][int]$Lane=-1)
$ErrorActionPreference='Stop'
if($CaptureOnly){$Capture=$true}
$taskProject=Join-Path $PSScriptRoot 'Unreal/Wildline/Wildline.uproject'
if(!$GamePath){$GamePath=if($Editor){'D:/EpicEngine/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'}else{Join-Path $PSScriptRoot 'Build/Windows/Wildline/Binaries/Win64/Wildline.exe'}}
if(!(Test-Path -LiteralPath $GamePath)){throw 'Build the selected game/editor first.'}
$taskJobs=@()
$taskFailures=@()
function Start-NeighborhoodProbe($taskMap,$taskLane,$taskReview){
    $taskName="neighborhood-$taskMap-"+$(if($taskReview){'review'}else{"lane$taskLane"})
    $taskProfile=Join-Path $PSScriptRoot "Unreal/Wildline/Saved/NeighborhoodVerification/$taskName"
    $taskLog=Join-Path $PSScriptRoot "$taskName.log"
    $taskArgs=if($Editor){'"'+$taskProject+'" -game '}else{''}
    $taskArgs+="/Game/Maps/$taskMap -unattended -NoSound -UserDir=`"$taskProfile`" -abslog=`"$taskLog`" "
    $taskArgs+=if($taskReview){'-WildlineHillReview -RenderOffscreen -windowed -ResX=1600 -ResY=900 -ForceRes -preferNvidia'}else{"-WildlineHillSmoke -WildlineCourseLane=$taskLane -NullRHI -UseFixedTimeStep -FPS=60"}
    $taskProcess=Start-Process -FilePath $GamePath -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
    [pscustomobject]@{Process=$taskProcess;Name=$taskName;Log=$taskLog;Profile=$taskProfile;Review=$taskReview}
}
try{
    $taskMaps=if($Map -eq 'Both'){@('Neighborhood','NeighborhoodCedar')}else{@($Map)}
    $taskLanes=if($Lane -lt 0){@(0,1)}else{@($Lane)}
    if(!$CaptureOnly){foreach($taskMap in $taskMaps){
        foreach($taskLane in $taskLanes){
        $taskJob=Start-NeighborhoodProbe $taskMap $taskLane $false;$taskJobs+=$taskJob
        while(!$taskJob.Process.WaitForExit(30000)){
            if((Get-Date)-$taskJob.Process.StartTime -gt [TimeSpan]::FromMinutes(5)){throw "Probe timed out: $($taskJob.Name)"}
        }
        if($taskJob.Process.ExitCode -ne 0 -or (Get-Content -LiteralPath $taskJob.Log -Raw) -notmatch 'WILDLINE HILL SMOKE: PASS'){$taskFailures+=$taskJob.Name}
        }
    }}
    if($taskFailures.Count){throw "Route verification failed: $($taskFailures -join ', '). See their logs in native."}
    if($Capture){
        foreach($taskMap in $taskMaps){
            $taskReview=Start-NeighborhoodProbe $taskMap 0 $true;$taskJobs+=$taskReview
            while(!$taskReview.Process.WaitForExit(30000)){if((Get-Date)-$taskReview.Process.StartTime -gt [TimeSpan]::FromMinutes(5)){throw "Render timed out: $taskMap"}}
            if($taskReview.Process.ExitCode -ne 0){throw "Render failed: $taskMap"}
            foreach($taskShot in @('HillCourse','HillNeighborhood','HillFeeder','HillSettings','HillPool','HillHose','HillSky')){
                $taskCandidates=@("Saved/Screenshots/Windows/$taskShot.png","Wildline/Saved/Screenshots/Windows/$taskShot.png") | ForEach-Object {Join-Path $taskReview.Profile $_} | Where-Object {Test-Path -LiteralPath $_} | ForEach-Object {Get-Item -LiteralPath $_} | Where-Object {$_.LastWriteTime -ge $taskReview.Process.StartTime}
                $taskSource=$taskCandidates | Select-Object -First 1
                if(!$taskSource){throw "Missing fresh render: $taskMap/$taskShot"}
                Copy-Item -LiteralPath $taskSource.FullName -Destination (Join-Path $PSScriptRoot "Preview/${taskMap}_${taskShot}.png") -Force
            }
            if((Get-Content -LiteralPath $taskReview.Log -Raw) -match 'LogMaterial: Error|LogShaderCompilers: Error|Fatal error'){throw "Render diagnostics failed: $taskMap"}
        }
    }
    if($CaptureOnly){Write-Output "CAPTURE PASS: fresh images for $($taskMaps -join ', ')."}else{Write-Output "PASS: $($taskMaps -join ', '), lanes $($taskLanes -join ', '), physical traversal, checkpoint order, scoring, reset and trampoline jumps."}
}finally{foreach($taskJob in $taskJobs){if(!$taskJob.Process.HasExited){Stop-Process -Id $taskJob.Process.Id -ErrorAction SilentlyContinue}}}
