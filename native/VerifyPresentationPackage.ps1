param([switch]$Capture,[switch]$Editor,[string]$GamePath)
$ErrorActionPreference='Stop'
$taskProject=Join-Path $PSScriptRoot 'Unreal/Wildline/Wildline.uproject'
$taskGame=if($GamePath){$GamePath}else{Join-Path $PSScriptRoot 'Build/Windows/Wildline/Binaries/Win64/Wildline.exe'}
if($Editor){$taskGame='D:/EpicEngine/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'}
if(!(Test-Path -LiteralPath $taskGame)){throw 'Build the game or editor first.'}
$taskProcesses=@()
function Start-PresentationProbe($taskFlags,$taskName){
    $taskProfile=Join-Path $PSScriptRoot "Unreal/Wildline/Saved/PresentationProbes/$taskName"
    $taskLog=Join-Path $PSScriptRoot "$taskName.log"
    $taskArgs=if($Editor){'"'+$taskProject+'" -game '}else{''}
    $taskPort=if($taskName -eq 'presentation-review'){7793}else{7792}
    $taskArgs+='/Game/Maps/Blackwood '+$taskFlags+' -WildlineProfileName=PresentationReview -WildlineDiscoveryPort='+$taskPort+' -unattended -UserDir="'+$taskProfile+'" -abslog="'+$taskLog+'"'
    Start-Process -FilePath $taskGame -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
}
try {
    $taskProcesses+=Start-PresentationProbe '-WildlineIntroSmoke -NullRHI -NoSound' 'presentation-smoke'
    if($Capture){$taskProcesses+=Start-PresentationProbe '-WildlineIntroReview -RenderOffscreen -windowed -ResX=1600 -ResY=900 -ForceRes -preferNvidia' 'presentation-review'}
    foreach($taskProcess in $taskProcesses){if(!$taskProcess.WaitForExit(60000)){throw "Presentation probe timed out: $($taskProcess.Id)"};if($taskProcess.ExitCode -ne 0){throw "Presentation probe failed: $($taskProcess.Id), exit $($taskProcess.ExitCode)"}}
    if((Get-Content -LiteralPath (Join-Path $PSScriptRoot 'presentation-smoke.log') -Raw) -notmatch 'WILDLINE INTRO SMOKE: PASS'){throw 'Missing presentation smoke pass marker.'}
    if($Capture){
        $taskShots=Join-Path $PSScriptRoot 'Unreal/Wildline/Saved/PresentationProbes/presentation-review/Saved/Screenshots/Windows'
        $taskPreview=Join-Path $PSScriptRoot ('Preview/Presentation-'+[guid]::NewGuid().ToString('N'));New-Item -ItemType Directory -Path $taskPreview -Force | Out-Null
        foreach($taskName in @('Intro2DNotice','Intro2DRun','Intro2DBeforeShot','Intro2DShot','Intro2DTumble','Intro2DPause','StudioLogo','StudioFade','PresentationMenu','PresentationSettings')){Copy-Item -LiteralPath (Join-Path $taskShots "$taskName.png") -Destination (Join-Path $taskPreview "$taskName.png") -Force}
        Write-Output "Presentation captures: $taskPreview"
    }
    Write-Output '2D squirrel run interrupted before nut contact, no pickup, pause, extended logo, completion, skip, menu poster, cooked soundtrack, 40% menu music fade and gameplay entry: PASS'
} finally {
    foreach($taskProcess in $taskProcesses){if(!$taskProcess.HasExited){Stop-Process -Id $taskProcess.Id -ErrorAction SilentlyContinue}}
}
