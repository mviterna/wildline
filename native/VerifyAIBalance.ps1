param([switch]$Editor,[switch]$Capture,[string]$GamePath)
$ErrorActionPreference='Stop'
if(!$GamePath){
    if($Editor){$GamePath='D:/EpicEngine/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'}
    else{. (Join-Path $PSScriptRoot 'PackagedBuilds.ps1');$taskBuild=Get-LatestWildlineBuild -BuildRoot (Join-Path $PSScriptRoot 'Build');$GamePath=Join-Path $taskBuild.PackageRoot 'Wildline/Binaries/Win64/Wildline.exe'}
}
$taskProfile=Join-Path $PSScriptRoot 'Unreal/Wildline/Saved/AIBalanceVerification'
$taskLog=Join-Path $PSScriptRoot 'ai-balance.log'
$taskArgs=if($Editor){'"'+(Join-Path $PSScriptRoot 'Unreal/Wildline/Wildline.uproject')+'" -game '}else{''}
$taskArgs+="/Game/Maps/Blackwood -WildlineAIBalanceReview -WildlineNoIntro -unattended -NoSound -UserDir=`"$taskProfile`" -abslog=`"$taskLog`" "
$taskArgs+=if($Capture){'-RenderOffscreen -windowed -ResX=1600 -ResY=900 -ForceRes -preferNvidia'}else{'-NullRHI -UseFixedTimeStep -FPS=60'}
$taskProcess=Start-Process -FilePath $GamePath -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
try{
    while(!$taskProcess.WaitForExit(1000)){if((Get-Date)-$taskProcess.StartTime -gt [TimeSpan]::FromMinutes(5)){throw 'AI balance check timed out'}}
    $taskText=Get-Content -LiteralPath $taskLog -Raw
    if($taskProcess.ExitCode -ne 0 -or $taskText -notmatch 'WILDLINE AI BALANCE: PASS exposedShots=35 hits=35' -or $taskText -match 'AI BALANCE FAIL'){throw "AI balance check failed. See $taskLog"}
    if($Capture){
        $taskShot=@('Saved/Screenshots/Windows/AIStrength100.png','Wildline/Saved/Screenshots/Windows/AIStrength100.png') | ForEach-Object {Join-Path $taskProfile $_} | Where-Object {Test-Path -LiteralPath $_} | ForEach-Object {Get-Item -LiteralPath $_} | Where-Object {$_.LastWriteTime -ge $taskProcess.StartTime} | Select-Object -First 1
        if(!$taskShot){throw 'Missing fresh 0-100 settings capture'}
        Copy-Item -LiteralPath $taskShot.FullName -Destination (Join-Path $PSScriptRoot 'Preview/AIStrength100.png') -Force
    }
    @{game=$GamePath;gameSha256=(Get-FileHash -LiteralPath $GamePath -Algorithm SHA256).Hash;editor=[bool]$Editor;verifiedAt=(Get-Date).ToString('o');exposedShots=35;hits=35;coverBlocksShots=$true;controls='0-100';checks=@($taskText -split "`n" | Where-Object {$_ -match 'AI BALANCE|AI ACCURACY'})} | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $PSScriptRoot 'ai-balance-verification.json')
    Write-Output 'PASS: 0-100 controls, maximum difficulty, 35/35 exposed hits including continuous movement, and cover blocking'
}finally{if(!$taskProcess.HasExited){Stop-Process -Id $taskProcess.Id}}
