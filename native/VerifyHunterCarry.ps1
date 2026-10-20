param([switch]$Editor,[switch]$Capture,[string]$GamePath,[string[]]$Presets=@('Travis'),[switch]$CustomBody)
$ErrorActionPreference='Stop'
$taskExe=if($Editor){'D:/EpicEngine/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'}elseif($GamePath){$GamePath}else{throw 'Pass GamePath or Editor.'}
$taskStamp=[guid]::NewGuid().ToString('N')
$taskEvidence=Join-Path $PSScriptRoot "Unreal/Wildline/Saved/HunterCarry/$taskStamp"
$taskPreview=Join-Path $PSScriptRoot "Preview/HunterCarry-$taskStamp"
foreach($taskPreset in $Presets){
    $taskProfile=Join-Path $taskEvidence $taskPreset;New-Item -ItemType Directory -Path $taskProfile -Force | Out-Null
    $taskLog=Join-Path $taskProfile 'probe.log'
    $taskArgs=if($Editor){'"'+(Join-Path $PSScriptRoot 'Unreal/Wildline/Wildline.uproject')+'" -game '}else{''}
    $taskArgs+='/Game/Maps/Blackwood -WildlineHunterCarryReview -WildlineHunterPreset='+$taskPreset+' -WildlineNoIntro -WildlineProfileName=CarryReview -WildlineDiscoveryPort=7794 -unattended -UserDir="'+$taskProfile+'" -abslog="'+$taskLog+'" '
    if($CustomBody){$taskArgs+='-WildlineCustomBody '}
    $taskArgs+=if($Capture){'-RenderOffscreen -windowed -ResX=1600 -ResY=900 -ForceRes -preferNvidia'}else{'-NullRHI -NoSound'}
    $taskProcess=Start-Process -FilePath $taskExe -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
    try{
        $taskDeadline=(Get-Date).AddMinutes(3)
        while(!$taskProcess.WaitForExit(1000)){if((Get-Date)-gt $taskDeadline){throw "Carry review timeout: $taskPreset"}}
        $taskText=Get-Content -LiteralPath $taskLog -Raw
        if($taskProcess.ExitCode -ne 0 -or $taskText -notmatch 'WILDLINE HUNTER CARRY: PASS' -or $taskText -match 'HUNTER CARRY FAIL|Fatal error'){throw "Carry review failed: $taskLog"}
        if($Capture){New-Item -ItemType Directory -Path $taskPreview -Force | Out-Null;foreach($taskName in @('HunterCarrySide','HunterAimFull','HunterAimGrip','HunterLoweredAgain')){
            $taskShot=Join-Path $taskProfile "Saved/Screenshots/Windows/$taskName.png"
            if(!(Test-Path -LiteralPath $taskShot) -or (Get-Item -LiteralPath $taskShot).LastWriteTime -lt $taskProcess.StartTime){throw "Missing or stale capture: $taskShot"}
            Copy-Item -LiteralPath $taskShot -Destination (Join-Path $taskPreview "$taskPreset-$taskName.png")
        }}
        Write-Output "$taskPreset carry review: PASS"
    }finally{if(!$taskProcess.HasExited){Stop-Process -Id $taskProcess.Id}}
}
Write-Output "Evidence: $taskEvidence"
if($Capture){Write-Output "Previews: $taskPreview"}
