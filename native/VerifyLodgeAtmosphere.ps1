param([switch]$Editor,[switch]$Capture,[string]$GamePath,[string[]]$Maps=@('Blackwood','Frostwood','Farmstead','Neighborhood','NeighborhoodCedar'))
$ErrorActionPreference='Stop'
$taskProject=Join-Path $PSScriptRoot 'Unreal/Wildline/Wildline.uproject'
$taskExe=if($Editor){'D:/EpicEngine/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'}elseif($GamePath){$GamePath}else{throw 'Pass the packaged GamePath.'}
$taskStamp=[guid]::NewGuid().ToString('N');$taskEvidence=Join-Path $PSScriptRoot "Unreal/Wildline/Saved/LodgeAtmosphere/$taskStamp"
$taskPreview=Join-Path $PSScriptRoot "Preview/Atmosphere-$taskStamp"
if($Capture){New-Item -ItemType Directory -Path $taskPreview -Force | Out-Null}
foreach($taskMap in $Maps){
    $taskProfile=Join-Path $taskEvidence $taskMap;New-Item -ItemType Directory -Path $taskProfile -Force | Out-Null
    $taskLog=Join-Path $taskProfile 'probe.log'
    $taskArgs=if($Editor){'"'+$taskProject+'" -game '}else{''}
    $taskArgs+='/Game/Maps/'+$taskMap+' -WildlineLodgeAtmosphereReview -WildlineNoIntro -WildlineProfileName=LodgeReview -WildlineDiscoveryPort=7793 -unattended -UserDir="'+$taskProfile+'" -abslog="'+$taskLog+'" '
    $taskArgs+=if($Capture){'-RenderOffscreen -windowed -ResX=1600 -ResY=900 -ForceRes -preferNvidia'}else{'-NullRHI -NoSound'}
    $taskProcess=Start-Process -FilePath $taskExe -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
    try{
        $taskDeadline=(Get-Date).AddMinutes(3)
        while(!$taskProcess.WaitForExit(1000)){if((Get-Date)-gt $taskDeadline){throw "Atmosphere timeout: $taskMap"}}
        $taskText=Get-Content -LiteralPath $taskLog -Raw
        if($taskProcess.ExitCode -ne 0 -or $taskText -notmatch 'WILDLINE LODGE ATMOSPHERE: PASS' -or $taskText -match 'LODGE ATMOSPHERE FAIL|Missing waiting room model|Default Material will be used in game|Failed to compile Material|Fatal error'){throw "Atmosphere failed: $taskMap. $taskLog"}
        if($Capture){foreach($taskName in @('LodgeExterior','LodgePaintings','LodgeFireGlow')){
            $taskShot=Join-Path $taskProfile "Saved/Screenshots/Windows/$taskName.png"
            if(!(Test-Path -LiteralPath $taskShot) -or (Get-Item -LiteralPath $taskShot).LastWriteTime -lt $taskProcess.StartTime){throw "Missing or stale capture: $taskShot"}
            Copy-Item -LiteralPath $taskShot -Destination (Join-Path $taskPreview "$taskMap-$taskName.png")
        }}
        Write-Output "$taskMap atmosphere: PASS"
    }finally{if(!$taskProcess.HasExited){Stop-Process -Id $taskProcess.Id}}
}
Write-Output "Evidence: $taskEvidence"
if($Capture){Write-Output "Previews: $taskPreview"}
