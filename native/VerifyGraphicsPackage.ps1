param([switch]$Editor,[string]$GamePath)
$ErrorActionPreference='Stop'
$taskGame=if($GamePath){$GamePath}else{Join-Path $PSScriptRoot 'Build/Windows/Wildline/Binaries/Win64/Wildline.exe'}
$taskProject=Join-Path $PSScriptRoot 'Unreal/Wildline/Wildline.uproject'
if($Editor){$taskGame='D:/EpicEngine/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'}
$taskStamp=[guid]::NewGuid().ToString('N')
$taskProfile=Join-Path $PSScriptRoot "Unreal/Wildline/Saved/PolishProbes/$taskStamp"
New-Item -ItemType Directory -Path $taskProfile -Force | Out-Null
foreach($taskRun in @('review','reload')){
    $taskFlag=if($taskRun -eq 'reload'){'-WildlinePolishReload'}else{'-WildlinePlayerPolishReview'}
    $taskLog=Join-Path $taskProfile "$taskRun.log"
    $taskPrefix=if($Editor){'"'+$taskProject+'" -game '}else{''}
    $taskArgs=$taskPrefix+'/Game/Maps/Blackwood '+$taskFlag+' -WildlineNoIntro -WildlineProfileName=PolishReview -WildlineDiscoveryPort=7794 -unattended -NoSound -RenderOffscreen -windowed -ResX=1600 -ResY=900 -ForceRes -preferNvidia -UserDir="'+$taskProfile+'" -abslog="'+$taskLog+'"'
    $taskProcess=Start-Process -FilePath $taskGame -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
    try {if(!$taskProcess.WaitForExit(180000)){throw "Polish verification timed out: $taskRun ($($taskProcess.Id))"};$taskProcess.Refresh();if($taskProcess.ExitCode -ne 0){throw "Polish verification failed: $taskRun"}}
    finally {if(!$taskProcess.HasExited){Stop-Process -Id $taskProcess.Id}}
    $taskMarker=if($taskRun -eq 'reload'){'WILDLINE POLISH RELOAD: PASS'}else{'WILDLINE POLISH REVIEW: PASS'}
    if((Get-Content -LiteralPath $taskLog -Raw) -notmatch $taskMarker){throw "Missing polish pass marker: $taskLog"}
}
$taskPreview=Join-Path $PSScriptRoot "Preview/Polish-$taskStamp"
New-Item -ItemType Directory -Path $taskPreview -Force | Out-Null
$taskShots=Join-Path $taskProfile 'Saved/Screenshots/Windows'
foreach($taskName in @('GraphicsQuality','GraphicsPerformance','SharedMatchStats')){Copy-Item -LiteralPath (Join-Path $taskShots "$taskName.png") -Destination (Join-Path $taskPreview "$taskName.png")}
$taskFrames=@(Get-ChildItem -LiteralPath $taskShots -Filter 'SquirrelPolish_*.png')
if($taskFrames.Count -ne 150){throw "Expected 150 motion frames; got $($taskFrames.Count)"}
[ordered]@{passed=$true;checkedAtUtc=[DateTimeOffset]::UtcNow.ToString('o');profile=$taskProfile;preview=$taskPreview;motionFrames=$taskFrames.Count;editor=[bool]$Editor}|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $PSScriptRoot 'polish-verification.json') -Encoding UTF8
Write-Output "WILDLINE POLISH PACKAGE: PASS. Evidence: $taskProfile; previews: $taskPreview"
