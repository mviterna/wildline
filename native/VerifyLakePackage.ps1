param([string]$GamePath,[switch]$Capture,[switch]$Network,[switch]$Editor,[string]$EngineRoot='D:/EpicEngine/UE_5.8')
$ErrorActionPreference='Stop'
if(!$GamePath){if($Editor){$GamePath=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'}else{. (Join-Path $PSScriptRoot 'PackagedBuilds.ps1');$taskBuild=Get-LatestWildlineBuild -BuildRoot (Join-Path $PSScriptRoot 'Build');$GamePath=Join-Path $taskBuild.PackageRoot 'Wildline/Binaries/Win64/Wildline.exe'}}
if(!(Test-Path -LiteralPath $GamePath)){throw 'Build the game first or provide GamePath.'}
$taskEvidence=Join-Path $PSScriptRoot 'Unreal/Wildline/ArtSource/Lake/Review'
New-Item -ItemType Directory -Path $taskEvidence -Force | Out-Null
function Start-LakeProbe([string]$Name,[string]$Map,[string]$Flags){
    $taskProfile=Join-Path $PSScriptRoot "Unreal/Wildline/Saved/LakeVerification/$Name"
    $taskLog=Join-Path $PSScriptRoot "lake-$Name.log"
    $taskMapArgs=if($Editor){'"'+(Join-Path $PSScriptRoot 'Unreal/Wildline/Wildline.uproject')+'" '+$Map+' -game'}else{$Map}
    $taskArgs="$taskMapArgs $Flags -WildlineNoIntro -WildlineProfileName=LakeExplorer -NoSound -unattended -UserDir=`"$taskProfile`" -abslog=`"$taskLog`""
    $taskProcess=Start-Process -FilePath $GamePath -ArgumentList $taskArgs -WorkingDirectory (Split-Path $GamePath) -WindowStyle Hidden -PassThru
    return [pscustomobject]@{Process=$taskProcess;Profile=$taskProfile;Log=$taskLog;Started=Get-Date}
}
function Wait-LakeProbe($Probe,[string]$Marker){
    try{
        while(!$Probe.Process.WaitForExit(1000)){if((Get-Date)-$Probe.Started -gt [TimeSpan]::FromMinutes(5)){throw "Lake probe timed out: $($Probe.Log)"}}
        $taskText=Get-Content -LiteralPath $Probe.Log -Raw
        if($Probe.Process.ExitCode -ne 0 -or ($Marker -and $taskText -notmatch [regex]::Escape($Marker))){throw "Lake probe failed. See $($Probe.Log)"}
        if($taskText -match 'LogMaterial: Error|LogShaderCompilers: Error|Failed to compile Material|Fatal error|Assertion failed'){throw "Lake runtime diagnostics failed. See $($Probe.Log)"}
    }finally{if(!$Probe.Process.HasExited){Stop-Process -Id $Probe.Process.Id}}
}
$taskSmoke=Start-LakeProbe 'smoke' '/Game/Maps/TaylorsvilleLake' '-WildlineLakeSmoke -NullRHI'
Wait-LakeProbe $taskSmoke 'WILDLINE LAKE SMOKE: PASS'
Write-Output 'PASS lake map cards, forage, trails, boats, fishing outcomes, walkable doorway and dock, swimming, diving, surfacing, underwater weapon blocking, squirrel drowning and respawn.'
if($Network){
    $taskHost=Start-LakeProbe 'net-host' '/Game/Maps/TaylorsvilleLake?listen' '-WildlineLakeNetProbe -NullRHI -port=7791'
    Start-Sleep -Seconds 4
    $taskClient=Start-LakeProbe 'net-client' '127.0.0.1:7791' '-WildlineLakeNetProbe -NullRHI'
    Wait-LakeProbe $taskClient 'WILDLINE LAKE NET CLIENT: PASS'
    Wait-LakeProbe $taskHost 'WILDLINE LAKE NET HOST: PASS'
    Write-Output 'PASS two-process replication of fishing, swimming, squirrel drowning, and authoritative underwater fire rejection.'
}
if($Capture){
    $taskReview=Start-LakeProbe 'review' '/Game/Maps/TaylorsvilleLake' '-WildlineLakeReview -RenderOffscreen -windowed -ResX=1600 -ResY=900 -ForceRes -preferNvidia'
    Wait-LakeProbe $taskReview ''
    foreach($taskShot in @('MapTab','LodgeDoor','LodgeDock','RodPickup','RodCasting','FishingCast','FishingInstructionsFaded','FishingFight','FishingCatch','LakeOverview','ShoreTrail','Diving','Swimming','BassUnderwater')){
        $taskCandidates=@("Saved/Screenshots/Windows/Lake/$taskShot.png","Wildline/Saved/Screenshots/Windows/Lake/$taskShot.png")
        $taskSource=$taskCandidates | ForEach-Object {Join-Path $taskReview.Profile $_} | Where-Object {(Test-Path -LiteralPath $_) -and (Get-Item -LiteralPath $_).LastWriteTime -ge $taskReview.Started} | Select-Object -First 1
        if(!$taskSource){throw "Missing lake capture: $taskShot"}
        Copy-Item -LiteralPath $taskSource -Destination (Join-Path $taskEvidence "$taskShot.png") -Force
    }
    Write-Output "Lake captures: $taskEvidence"
}
