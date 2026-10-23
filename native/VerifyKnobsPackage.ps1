param([string]$GamePath,[switch]$Capture,[switch]$Network,[switch]$Editor,[string]$EngineRoot='D:/EpicEngine/UE_5.8')
$ErrorActionPreference='Stop'
if(!$GamePath){if($Editor){$GamePath=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'}else{. (Join-Path $PSScriptRoot 'PackagedBuilds.ps1');$taskBuild=Get-LatestWildlineBuild -BuildRoot (Join-Path $PSScriptRoot 'Build');$GamePath=Join-Path $taskBuild.PackageRoot 'Wildline/Binaries/Win64/Wildline.exe'}}
if(!(Test-Path -LiteralPath $GamePath)){throw 'Build the game first or provide GamePath.'}
$taskEvidence=Join-Path $PSScriptRoot 'Unreal/Wildline/ArtSource/Knobs/Review'
New-Item -ItemType Directory -Path $taskEvidence -Force | Out-Null
function Start-KnobsProbe([string]$Name,[string]$Map,[string]$Flags){
    $taskProfile=Join-Path $PSScriptRoot "Unreal/Wildline/Saved/KnobsVerification/$Name"
    $taskLog=Join-Path $PSScriptRoot "knobs-$Name.log"
    $taskMapArgs=if($Editor){'"'+(Join-Path $PSScriptRoot 'Unreal/Wildline/Wildline.uproject')+'" '+$Map+' -game'}else{$Map}
    $taskArgs="$taskMapArgs $Flags -WildlineNoIntro -WildlineProfileName=KnobsExplorer -NoSound -unattended -UserDir=`"$taskProfile`" -abslog=`"$taskLog`""
    $taskProcess=Start-Process -FilePath $GamePath -ArgumentList $taskArgs -WorkingDirectory (Split-Path $GamePath) -WindowStyle Hidden -PassThru
    [pscustomobject]@{Process=$taskProcess;Profile=$taskProfile;Log=$taskLog;Started=Get-Date}
}
function Wait-KnobsProbe($Probe,[string]$Marker){
    try{
        while(!$Probe.Process.WaitForExit(1000)){if((Get-Date)-$Probe.Started -gt [TimeSpan]::FromMinutes(5)){throw "Knobs probe timed out: $($Probe.Log)"}}
        $taskText=Get-Content -LiteralPath $Probe.Log -Raw
        if($Probe.Process.ExitCode -ne 0 -or $taskText -notmatch [regex]::Escape($Marker)){throw "Knobs probe failed. See $($Probe.Log)"}
        if($taskText -match 'LogMaterial: Error|LogShaderCompilers: Error|Failed to compile Material|Fatal error|Assertion failed'){throw "Knobs runtime diagnostics failed. See $($Probe.Log)"}
    }finally{if(!$Probe.Process.HasExited){Stop-Process -Id $Probe.Process.Id}}
}
$taskSmoke=Start-KnobsProbe 'smoke' '/Game/Maps/KnobsStateForest' '-WildlineKnobsSmoke -NullRHI'
Wait-KnobsProbe $taskSmoke 'WILDLINE KNOBS SMOKE: PASS'
$taskTravel=Start-KnobsProbe 'travel' '/Game/Maps/Blackwood' '-WildlineKnobsTravel -NullRHI'
Wait-KnobsProbe $taskTravel 'WILDLINE KNOBS SMOKE: PASS'
Write-Output 'PASS menu selection loads Knobs from Blackwood.'
Write-Output 'PASS Knobs selection, trail slopes and collision, climbable woodland, forage pickup/deposit and hunter walking.'
if($Network){
    $taskHost=Start-KnobsProbe 'net-host' '/Game/Maps/KnobsStateForest?listen' '-WildlineKnobsNetProbe -NullRHI -port=7793'
    try{
        Start-Sleep -Seconds 4
        $taskClient=Start-KnobsProbe 'net-client' '127.0.0.1:7793' '-WildlineKnobsNetProbe -NullRHI'
        Wait-KnobsProbe $taskClient 'WILDLINE KNOBS NET CLIENT: PASS'
        Wait-KnobsProbe $taskHost 'WILDLINE KNOBS NET HOST: PASS'
    }finally{if(!$taskHost.Process.HasExited){Stop-Process -Id $taskHost.Process.Id}}
    Write-Output 'PASS two-process Knobs map and host-rule replication.'
}
if($Capture){
    $taskReview=Start-KnobsProbe 'review' '/Game/Maps/KnobsStateForest' '-WildlineKnobsReview -RenderOffscreen -windowed -ResX=1600 -ResY=900 -ForceRes -preferNvidia'
    Wait-KnobsProbe $taskReview 'WILDLINE KNOBS SMOKE: PASS'
    foreach($taskShot in @('Overview','Trailhead','CrookedCreek','HayRidge','MapCard')){
        $taskCandidates=@("Saved/Screenshots/Windows/Knobs/$taskShot.png","Wildline/Saved/Screenshots/Windows/Knobs/$taskShot.png")
        $taskSource=$taskCandidates | ForEach-Object {Join-Path $taskReview.Profile $_} | Where-Object {(Test-Path -LiteralPath $_) -and (Get-Item -LiteralPath $_).LastWriteTime -ge $taskReview.Started} | Select-Object -First 1
        if(!$taskSource){throw "Missing Knobs capture: $taskShot"}
        Copy-Item -LiteralPath $taskSource -Destination (Join-Path $taskEvidence "$taskShot.png") -Force
    }
    Write-Output "Knobs captures: $taskEvidence"
}
