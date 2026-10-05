param([switch]$Capture)
$ErrorActionPreference='Stop'
$taskGame=Join-Path $PSScriptRoot 'Build/Windows/Wildline/Binaries/Win64/Wildline.exe'
if(!(Test-Path -LiteralPath $taskGame)){throw 'Build the Windows game first.'}
$taskProcesses=@()
function Start-FarmProbe($taskFlags,$taskLog){
    $taskArgs=$taskFlags+' -unattended -NoSound -abslog="'+(Join-Path $PSScriptRoot $taskLog).Replace('\','/')+'"'
    Start-Process -FilePath $taskGame -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
}
try {
    $taskProcesses+=Start-FarmProbe '/Game/Maps/Blackwood -WildlineSmoke -NullRHI' 'farm-classic-package.log'
    $taskProcesses+=Start-FarmProbe '/Game/Maps/Farmstead -WildlineFarmSmoke -NullRHI' 'farm-package-smoke.log'
    $taskProcesses+=Start-FarmProbe '/Game/Maps/Farmstead?listen -port=7782 -WildlineAutoStart -WildlineNetProbe -NullRHI' 'farm-net-host.log'
    $taskProcesses+=Start-FarmProbe '/Game/Maps/Farmstead -WildlineFarmBotProbe -NullRHI' 'farm-package-bots.log'
    if($Capture){$taskProcesses+=Start-FarmProbe '/Game/Maps/Farmstead -WildlineFarmReview -RenderOffscreen -windowed -ResX=1600 -ResY=900 -ForceRes -preferNvidia' 'farm-package-review.log'}
    Start-Sleep -Seconds 22
    $taskProcesses+=Start-FarmProbe '127.0.0.1:7782 -WildlineAutoStart -WildlineSquirrel -WildlineNetProbe -NullRHI' 'farm-net-client.log'
    foreach($taskProcess in $taskProcesses){
        if(!$taskProcess.WaitForExit(60000) -and !$taskProcess.WaitForExit(60000)){throw "Native verification timed out: $($taskProcess.Id)"}
        if($taskProcess.ExitCode -ne 0){throw "Native verification failed: $($taskProcess.Id), exit $($taskProcess.ExitCode)"}
    }
    foreach($taskCheck in @(
        @('farm-classic-package.log','WILDLINE SMOKE RESULT: PASS'),
        @('farm-package-smoke.log','WILDLINE FARM SMOKE: PASS'),
        @('farm-package-bots.log','WILDLINE FARM BOTS: PASS'),
        @('farm-net-host.log','WILDLINE NETWORK Host: PASS'),
        @('farm-net-client.log','WILDLINE NETWORK Client: PASS.+appearance=1 aim=1 difficulty=2')
    )){
        if((Get-Content (Join-Path $PSScriptRoot $taskCheck[0]) -Raw) -notmatch $taskCheck[1]){throw "Missing pass marker: $($taskCheck[0])"}
    }
    foreach($taskSide in @('host','client')){if((Get-Content (Join-Path $PSScriptRoot "farm-net-$taskSide.log") -Raw) -notmatch 'WILDLINE FARM NETWORK: PASS'){throw "Farm replication failed on $taskSide"}}
    if($Capture){
        $taskReview=Get-Content (Join-Path $PSScriptRoot 'farm-package-review.log') -Raw
        if($taskReview -match 'LogMaterial: Error|LogShaderCompilers: Error|Failed to compile Material|Fatal error'){throw 'Farm render review reported shader or material errors.'}
        foreach($taskName in @('FarmMenu.png','FarmShotgun.png','Farmhouse.png','FarmOverview.png','FarmPantry.png','FarmLivestock.png')){
            $taskShot=Join-Path $PSScriptRoot "Build/Windows/Wildline/Saved/Screenshots/Windows/$taskName"
            if(!(Test-Path -LiteralPath $taskShot)){throw "Missing packaged screenshot: $taskName"}
            Copy-Item -LiteralPath $taskShot -Destination (Join-Path $PSScriptRoot "Preview/$taskName") -Force
        }
    }
    Write-Output 'Packaged Classic, Farm Raid, shared pantry, livestock damage, shotgun, autonomous deliveries and late-join replication: PASS'
} finally {
    foreach($taskProcess in $taskProcesses){if(!$taskProcess.HasExited){Stop-Process -Id $taskProcess.Id -ErrorAction SilentlyContinue}}
}
