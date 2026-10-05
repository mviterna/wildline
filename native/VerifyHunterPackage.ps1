param([switch]$Capture)
$ErrorActionPreference='Stop'
$taskGame=Join-Path $PSScriptRoot 'Build/Windows/Wildline/Binaries/Win64/Wildline.exe'
if(!(Test-Path -LiteralPath $taskGame)){throw 'Build the Windows game first.'}
function Start-Probe($taskFlags,$taskLog){
    $taskArgs=$taskFlags+' -unattended -NoSound -abslog="'+(Join-Path $PSScriptRoot $taskLog).Replace('\','/')+'"'
    Start-Process -FilePath $taskGame -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
}
$taskSmoke=Start-Probe '-WildlineSmoke -NullRHI' 'hunter-package-smoke.log'
$taskHost=Start-Probe '/Game/Maps/Blackwood?listen -port=7779 -WildlineAutoStart -WildlineNetProbe -NullRHI' 'hunter-net-host.log'
if($Capture){
    $taskFixture=(Join-Path $PSScriptRoot 'Unreal/Wildline/ArtSource/Hunters/PortraitFixture.png').Replace('\','/')
    $taskReview=Start-Probe ('-WildlineHunterReview -WildlineHunterPhoto="'+$taskFixture+'" -RenderOffscreen -windowed -ResX=1600 -ResY=900 -ForceRes -preferNvidia') 'hunter-package-review.log'
}
# Join after the host has changed its appearance at 12 seconds, exercising late joins.
Start-Sleep -Seconds 22
$taskClient=Start-Probe '127.0.0.1:7779 -WildlineAutoStart -WildlineSquirrel -WildlineNetProbe -NullRHI' 'hunter-net-client.log'
foreach($taskProcess in @($taskSmoke,$taskClient,$taskHost,$taskReview)){
    if($null -eq $taskProcess){continue}
    if(!$taskProcess.WaitForExit(120000)){throw "Native verification timed out: $($taskProcess.Id)"}
    if($taskProcess.ExitCode -ne 0){throw "Native verification failed: $($taskProcess.Id), exit $($taskProcess.ExitCode)"}
}
$taskSmokeText=Get-Content (Join-Path $PSScriptRoot 'hunter-package-smoke.log') -Raw
$taskHostText=Get-Content (Join-Path $PSScriptRoot 'hunter-net-host.log') -Raw
$taskClientText=Get-Content (Join-Path $PSScriptRoot 'hunter-net-client.log') -Raw
if($taskSmokeText -notmatch 'WILDLINE SMOKE RESULT: PASS'){throw 'Smoke result marker missing.'}
if($taskHostText -notmatch 'WILDLINE NETWORK Host: PASS'){throw 'Host result marker missing.'}
if($taskClientText -notmatch 'WILDLINE NETWORK Client: PASS.+appearance=1'){throw 'Late-joining client appearance did not replicate.'}
Write-Output 'Native smoke and late-join hunter/squirrel co-op: PASS'
