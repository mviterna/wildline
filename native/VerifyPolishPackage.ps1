param([switch]$Capture)
$ErrorActionPreference='Stop'
$taskGame=Join-Path $PSScriptRoot 'Build/Windows/Wildline/Binaries/Win64/Wildline.exe'
if(!(Test-Path -LiteralPath $taskGame)){throw 'Build the Windows game first.'}
$taskProcesses=@()
function Start-Probe($taskFlags,$taskLog){
    $taskArgs=$taskFlags+' -unattended -NoSound -abslog="'+(Join-Path $PSScriptRoot $taskLog).Replace('\','/')+'"'
    Start-Process -FilePath $taskGame -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
}
try {
    $taskProcesses+=Start-Probe '-WildlineSmoke -NullRHI' 'polish-package-smoke.log'
    $taskProcesses+=Start-Probe '/Game/Maps/Blackwood?listen -port=7779 -WildlineAutoStart -WildlineNetProbe -NullRHI' 'polish-net-host.log'
    if($Capture){$taskProcesses+=Start-Probe '-WildlinePolishReview -RenderOffscreen -windowed -ResX=1600 -ResY=900 -ForceRes -preferNvidia' 'polish-package-review.log'}
    # Exercise a late join after the host raises the rifle and edits appearance.
    Start-Sleep -Seconds 22
    $taskProcesses+=Start-Probe '127.0.0.1:7779 -WildlineAutoStart -WildlineSquirrel -WildlineNetProbe -NullRHI' 'polish-net-client.log'
    foreach($taskProcess in $taskProcesses){
        if(!$taskProcess.WaitForExit(60000) -and !$taskProcess.WaitForExit(60000)){throw "Native verification timed out: $($taskProcess.Id)"}
        if($taskProcess.ExitCode -ne 0){throw "Native verification failed: $($taskProcess.Id), exit $($taskProcess.ExitCode)"}
    }
    $taskSmokeText=Get-Content (Join-Path $PSScriptRoot 'polish-package-smoke.log') -Raw
    $taskHostText=Get-Content (Join-Path $PSScriptRoot 'polish-net-host.log') -Raw
    $taskClientText=Get-Content (Join-Path $PSScriptRoot 'polish-net-client.log') -Raw
    if($taskSmokeText -notmatch 'WILDLINE SMOKE RESULT: PASS'){throw 'Smoke result marker missing.'}
    if($taskHostText -notmatch 'WILDLINE NETWORK Host: PASS'){throw 'Host result marker missing.'}
    if($taskClientText -notmatch 'WILDLINE NETWORK Client: PASS.+appearance=1 aim=1'){throw 'Late-joining client appearance or aim did not replicate.'}
    if($Capture){
        $taskReviewText=Get-Content (Join-Path $PSScriptRoot 'polish-package-review.log') -Raw
        if($taskReviewText -match 'LogMaterial: Error|LogShaderCompilers: Error|Fatal error'){throw 'Render review reported material or shader errors.'}
        $taskShots=Join-Path $PSScriptRoot 'Build/Windows/Wildline/Saved/Screenshots/Windows'
        $taskPreview=Join-Path $PSScriptRoot 'Preview'
        New-Item -Path $taskPreview -ItemType Directory -Force | Out-Null
        foreach($taskName in @('PolishHunterGrip.png','PolishSquirrelClimb.png','PolishSquirrelOrbit.png','PolishReserveBoundary.png')){
            $taskShot=Join-Path $taskShots $taskName
            if(!(Test-Path -LiteralPath $taskShot)){throw "Missing packaged screenshot: $taskName"}
            Copy-Item -LiteralPath $taskShot -Destination (Join-Path $taskPreview $taskName) -Force
        }
    }
    Write-Output 'Packaged live-world checks, late-join appearance and aiming replication: PASS'
} finally {
    foreach($taskProcess in $taskProcesses){if(!$taskProcess.HasExited){Stop-Process -Id $taskProcess.Id -ErrorAction SilentlyContinue}}
}
