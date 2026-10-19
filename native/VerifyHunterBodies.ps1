param([switch]$Editor,[switch]$Capture)
$ErrorActionPreference='Stop'
$taskProject=Join-Path $PSScriptRoot 'Unreal/Wildline/Wildline.uproject'
$taskGame=if($Editor){'D:/EpicEngine/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'}else{Join-Path $PSScriptRoot 'Build/Windows/Wildline/Binaries/Win64/Wildline.exe'}
$taskKind=if($Editor){'editor'}else{'package'}
$taskRoot=Join-Path $PSScriptRoot "Build/HunterBodiesReview/$taskKind"
$taskStarted=[DateTime]::UtcNow
$taskProcesses=@()
function Start-BodyProbe([string]$Flags,[string]$Name){
    $taskPrefix=if($Editor){'"'+$taskProject+'" -game '}else{''}
    $taskArgs=$taskPrefix+$Flags+' -unattended -NoSound -UserDir="'+(Join-Path $taskRoot $Name)+'" -abslog="'+(Join-Path $PSScriptRoot "hunter-bodies-$taskKind-$Name.log")+'"'
    Start-Process -FilePath $taskGame -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
}
try{
    $taskRender=if($Capture){'-RenderOffscreen -windowed -ResX=1600 -ResY=900 -ForceRes -preferNvidia'}else{'-NullRHI'}
    $taskProcesses+=Start-BodyProbe ("/Game/Maps/Blackwood -WildlineHunterBodies $taskRender") 'creator'
    $taskProcesses+=Start-BodyProbe '/Game/Maps/Blackwood?listen -port=7806 -WildlineAutoStart -WildlineNetProbe -NullRHI' 'host'
    $taskHostLog=Join-Path $PSScriptRoot "hunter-bodies-$taskKind-host.log"
    $taskReadyBy=[DateTime]::UtcNow.AddSeconds(120)
    while([DateTime]::UtcNow -lt $taskReadyBy){
        if((Test-Path -LiteralPath $taskHostLog) -and (Get-Item -LiteralPath $taskHostLog).LastWriteTimeUtc -ge $taskStarted -and (Get-Content -LiteralPath $taskHostLog -Raw) -match 'GameNetDriver.*listening on port 7806'){break}
        Start-Sleep -Seconds 1
    }
    if([DateTime]::UtcNow -ge $taskReadyBy){throw 'Co-op host did not start.'}
    # Connect after profile edits to exercise late-join replication.
    Start-Sleep -Seconds 15
    $taskProcesses+=Start-BodyProbe '127.0.0.1:7806 -WildlineAutoStart -WildlineSquirrel -WildlineNetProbe -NullRHI' 'client'
    foreach($taskProcess in $taskProcesses){if(!$taskProcess.WaitForExit(180000)){throw "Body probe timed out: $($taskProcess.Id)"};$taskProcess.Refresh();if($taskProcess.ExitCode -ne 0){throw "Body probe failed: $($taskProcess.Id)"}}
    foreach($taskCheck in @(@('creator','WILDLINE HUNTER BODIES: PASS'),@('host','WILDLINE NETWORK Host: PASS'),@('client','WILDLINE NETWORK Client: PASS.+appearance=1'))){
        $taskLog=Join-Path $PSScriptRoot "hunter-bodies-$taskKind-$($taskCheck[0]).log"
        if((Get-Item -LiteralPath $taskLog).LastWriteTimeUtc -lt $taskStarted -or (Get-Content -LiteralPath $taskLog -Raw) -notmatch $taskCheck[1]){throw "Missing fresh pass marker: $taskLog"}
    }
    if($Capture){foreach($taskName in @('HunterBodyEthanDefault','HunterBodyEthanAdult','HunterBodyControls','HunterBodyFace','HunterBodyInGame','HunterBodyExtreme')){
        $taskImage=Join-Path $taskRoot "creator/Saved/Screenshots/Windows/$taskName.png"
        if(!(Test-Path -LiteralPath $taskImage) -or (Get-Item -LiteralPath $taskImage).LastWriteTimeUtc -lt $taskStarted -or (Get-Item -LiteralPath $taskImage).Length -lt 20000){throw "Missing fresh screenshot: $taskImage"}
        Copy-Item -LiteralPath $taskImage -Destination (Join-Path $PSScriptRoot "Preview/$taskName.png") -Force
    }}
    Write-Output 'PASS hunter body sliders, preset defaults/reset, adult Ethan, persistence, grips, extreme proportions and late-joining co-op dimensions.'
}finally{foreach($taskProcess in $taskProcesses){if(!$taskProcess.HasExited){Stop-Process -Id $taskProcess.Id -ErrorAction SilentlyContinue}}}
