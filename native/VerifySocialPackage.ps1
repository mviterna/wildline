param([switch]$Editor,[switch]$Capture,[string]$GamePath)
$ErrorActionPreference='Stop'
$taskProject=Join-Path $PSScriptRoot 'Unreal/Wildline/Wildline.uproject'
$taskGame=if($GamePath){$GamePath}else{Join-Path $PSScriptRoot 'Build/Windows/Wildline/Binaries/Win64/Wildline.exe'}
if($Editor){$taskGame='D:/EpicEngine/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'}
$taskStamp=[guid]::NewGuid().ToString('N')
$taskRoot=Join-Path $PSScriptRoot "Unreal/Wildline/Saved/SocialProbes/$taskStamp"
$taskProcesses=@()
function Start-SocialProbe($taskName,$taskFlags){
    $taskProfile=Join-Path $taskRoot $taskName
    New-Item -ItemType Directory -Path $taskProfile -Force | Out-Null
    $taskPrefix=if($Editor){'"'+$taskProject+'" -game '}else{''}
    $taskArgs=$taskPrefix+$taskFlags+' -WildlineNoIntro -unattended -UserDir="'+$taskProfile+'" -abslog="'+$taskProfile+'/probe.log"'
    Start-Process -FilePath $taskGame -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
}
try {
    $taskVisual=if($Capture){'-RenderOffscreen -windowed -ResX=1600 -ResY=900 -ForceRes -preferNvidia'}else{'-NullRHI -NoSound'}
    $taskProcesses+=Start-SocialProbe 'local' ('/Game/Maps/Blackwood -WildlineSocialReview -WildlineDiscoveryPort=7790 '+$taskVisual)
    foreach($taskProcess in $taskProcesses){if(!$taskProcess.WaitForExit(180000)){throw "Social review timed out: $($taskProcess.Id)"};if($taskProcess.ExitCode -ne 0){throw "Social review failed: $taskRoot"}}
    if((Get-Content (Join-Path $taskRoot 'local/probe.log') -Raw) -notmatch 'WILDLINE SOCIAL REVIEW: PASS'){throw 'Missing local review pass.'}
    $taskProcesses+=Start-SocialProbe 'host' '/Game/Maps/Blackwood?listen -port=7797 -WildlineSocialNetProbe -WildlineSocialHost -WildlineProfileName=RoomHost -WildlineDiscoveryPort=7787 -WildlineDiscoverySeed=127.0.0.1:7788 -NullRHI -NoSound'
    $taskHostLog=Join-Path $taskRoot 'host/probe.log'
    $taskDeadline=(Get-Date).AddSeconds(100)
    do { Start-Sleep -Milliseconds 500; $taskReady=(Test-Path $taskHostLog) -and ((Get-Content $taskHostLog -Raw) -match 'listening on port 7797');if((Get-Date) -gt $taskDeadline){throw 'Host startup timed out.'} } while(!$taskReady)
    $taskProcesses+=Start-SocialProbe 'client' '/Game/Maps/Blackwood -port=7798 -WildlineSocialNetProbe -WildlineProfileName=RoomClient -WildlineDiscoveryPort=7788 -WildlineDiscoverySeed=127.0.0.1:7787 -NullRHI -NoSound'
    foreach($taskProcess in $taskProcesses){if(!$taskProcess.WaitForExit(180000)){throw "Social probe timed out: $($taskProcess.Id)"};if($taskProcess.ExitCode -ne 0){throw "Social network failed: $taskRoot"}}
    foreach($taskName in @('host','client')){if((Get-Content (Join-Path $taskRoot "$taskName/probe.log") -Raw) -notmatch 'WILDLINE SOCIAL NETWORK: PASS'){throw "Missing $taskName network pass."}}
    if((Get-Content (Join-Path $taskRoot 'client/probe.log') -Raw) -notmatch 'WILDLINE DISCOVERY HOST LOCK: PASS'){throw 'Missing discovery / host lock pass.'}
    if($Capture){$taskPreview=Join-Path $PSScriptRoot "Preview/Social-$taskStamp";New-Item -ItemType Directory -Path $taskPreview -Force | Out-Null;foreach($taskName in @('ProfileFirstLaunch','ProfileRoom','HunterHeight145','HunterHeight205','HunterHeight230')){Copy-Item -LiteralPath (Join-Path $taskRoot "local/Saved/Screenshots/Windows/$taskName.png") -Destination (Join-Path $taskPreview "$taskName.png") -Force}}
    Write-Output "WILDLINE SOCIAL PACKAGE: PASS. Evidence: $taskRoot"
} finally {foreach($taskProcess in $taskProcesses){if(!$taskProcess.HasExited){Stop-Process -Id $taskProcess.Id -ErrorAction SilentlyContinue}}}
