param([switch]$Capture,[switch]$Editor,[switch]$CustomBody,[int[]]$Rates=@(30,60,120),[ValidateSet('Travis','Ethan','Greyson','Mark')][string[]]$Presets=@('Travis'))
$ErrorActionPreference='Stop'
$taskGame=Join-Path $PSScriptRoot 'Build/Windows/Wildline/Binaries/Win64/Wildline.exe'
if($Editor){$taskGame='D:/EpicEngine/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'}
if(!(Test-Path -LiteralPath $taskGame)){throw 'Build the Windows game first.'}
$taskResults=Join-Path $PSScriptRoot 'test-results/hunter-animations'
New-Item -ItemType Directory -Path $taskResults -Force | Out-Null
function Invoke-AnimationProbe($taskRate,$taskRender,$taskPreset){
    $taskName=if($taskRender){'capture'}else{"fps-$taskRate"}
    if($taskPreset -ne 'Travis'){$taskName=$taskPreset.ToLower()+'-'+$taskName}
    if($CustomBody){$taskName+='-custom-body'}
    if($Editor){$taskName+='-editor'}
    $taskFixture=Join-Path $taskResults $taskName
    $taskLog=Join-Path $taskResults "$taskName.log"
    $taskFlags=if($taskRender){'-WildlineHunterLocomotionReview -RenderOffscreen -windowed -ResX=1280 -ResY=720 -ForceRes -preferNvidia'}else{'-WildlineHunterAnimationProbe -NullRHI'}
    $taskArgs=$taskFlags+' -WildlineHunterPreset='+$taskPreset+' -UseFixedTimeStep -FPS='+$taskRate+' -unattended -NoSound -UserDir="'+$taskFixture.Replace('\','/')+'/" -abslog="'+$taskLog.Replace('\','/')+'"'
    if($CustomBody){$taskArgs+=' -WildlineCustomBody'}
    if($Editor){$taskArgs='"'+(Join-Path $PSScriptRoot 'Unreal/Wildline/Wildline.uproject')+'" -game '+$taskArgs}
    $taskProcess=Start-Process -FilePath $taskGame -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
    if(!$taskProcess.WaitForExit(600000)){throw "Hunter animation probe timed out: $taskName"}
    $taskText=Get-Content -LiteralPath $taskLog -Raw
    if($taskProcess.ExitCode -ne 0 -or $taskText -notmatch 'HUNTER ANIMATION RESULT: PASS'){throw "Hunter animation probe failed: $taskLog"}
    ($taskText -split "`r?`n" | Where-Object {$_ -match 'HUNTER ANIMATION RESULT:'}) | Write-Output
}
foreach($taskPreset in $Presets){
    foreach($taskRate in $Rates){Invoke-AnimationProbe $taskRate $false $taskPreset}
    if($Capture){Invoke-AnimationProbe 30 $true $taskPreset}
}
