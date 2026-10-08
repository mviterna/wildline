param([switch]$Capture,[int[]]$Rates=@(30,60,120),[ValidateSet('Travis','Ethan','Greyson')][string[]]$Presets=@('Travis'))
$ErrorActionPreference='Stop'
$taskGame=Join-Path $PSScriptRoot 'Build/Windows/Wildline/Binaries/Win64/Wildline.exe'
if(!(Test-Path -LiteralPath $taskGame)){throw 'Build the Windows game first.'}
$taskResults=Join-Path $PSScriptRoot 'test-results/hunter-animations'
New-Item -ItemType Directory -Path $taskResults -Force | Out-Null
function Invoke-AnimationProbe($taskRate,$taskRender,$taskPreset){
    $taskName=if($taskRender){'capture'}else{"fps-$taskRate"}
    if($taskPreset -ne 'Travis'){$taskName=$taskPreset.ToLower()+'-'+$taskName}
    $taskFixture=Join-Path $taskResults $taskName
    $taskLog=Join-Path $taskResults "$taskName.log"
    $taskFlags=if($taskRender){'-WildlineHunterLocomotionReview -RenderOffscreen -windowed -ResX=1280 -ResY=720 -ForceRes -preferNvidia'}else{'-WildlineHunterAnimationProbe -NullRHI'}
    $taskArgs=$taskFlags+' -WildlineHunterPreset='+$taskPreset+' -UseFixedTimeStep -FPS='+$taskRate+' -unattended -NoSound -UserDir="'+$taskFixture.Replace('\','/')+'/" -abslog="'+$taskLog.Replace('\','/')+'"'
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
