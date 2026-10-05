param([switch]$Capture)
$ErrorActionPreference='Stop'
$taskGame=Join-Path $PSScriptRoot 'Build/Windows/Wildline/Binaries/Win64/Wildline.exe'
if(!(Test-Path -LiteralPath $taskGame)){throw 'Build the Windows game first.'}
$taskProcesses=@()
function Start-ForageProbe($taskFlags,$taskLog){
    $taskArgs=$taskFlags+' -unattended -NoSound -abslog="'+(Join-Path $PSScriptRoot $taskLog).Replace('\','/')+'"'
    Start-Process -FilePath $taskGame -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
}
try {
    $taskProcesses+=Start-ForageProbe '-WildlineSmoke -NullRHI' 'forage-package-smoke.log'
    $taskProcesses+=Start-ForageProbe '/Game/Maps/Blackwood?listen -port=7781 -WildlineAutoStart -WildlineNetProbe -NullRHI' 'forage-net-host.log'
    if($Capture){$taskProcesses+=Start-ForageProbe '-WildlineForageReview -RenderOffscreen -windowed -ResX=1600 -ResY=900 -ForceRes -preferNvidia' 'forage-package-review.log'}
    Start-Sleep -Seconds 22
    $taskProcesses+=Start-ForageProbe '127.0.0.1:7781 -WildlineAutoStart -WildlineSquirrel -WildlineNetProbe -NullRHI' 'forage-net-client.log'
    foreach($taskProcess in $taskProcesses){
        if(!$taskProcess.WaitForExit(60000) -and !$taskProcess.WaitForExit(60000)){throw "Native verification timed out: $($taskProcess.Id)"}
        if($taskProcess.ExitCode -ne 0){throw "Native verification failed: $($taskProcess.Id), exit $($taskProcess.ExitCode)"}
    }
    $taskSmoke=Get-Content (Join-Path $PSScriptRoot 'forage-package-smoke.log') -Raw
    $taskHost=Get-Content (Join-Path $PSScriptRoot 'forage-net-host.log') -Raw
    $taskClient=Get-Content (Join-Path $PSScriptRoot 'forage-net-client.log') -Raw
    if($taskSmoke -notmatch 'WILDLINE SMOKE RESULT: PASS'){throw 'Smoke result marker missing.'}
    if($taskHost -notmatch 'WILDLINE NETWORK Host: PASS'){throw 'Host result marker missing.'}
    if($taskClient -notmatch 'WILDLINE NETWORK Client: PASS.+appearance=1 aim=1 difficulty=2'){throw 'Late-joining client difficulty or appearance did not replicate.'}
    if($Capture){
        $taskReview=Get-Content (Join-Path $PSScriptRoot 'forage-package-review.log') -Raw
        if($taskReview -match 'LogMaterial: Error|LogShaderCompilers: Error|Fatal error'){throw 'Render review reported material or shader errors.'}
        $taskShots=Join-Path $PSScriptRoot 'Build/Windows/Wildline/Saved/Screenshots/Windows'
        $taskPreview=Join-Path $PSScriptRoot 'Preview'
        New-Item -Path $taskPreview -ItemType Directory -Force | Out-Null
        foreach($taskName in @('ForageCamp.png','ForageEasyMap.png','ForageZoomMap.png','ForagePickup.png')){
            $taskShot=Join-Path $taskShots $taskName
            if(!(Test-Path -LiteralPath $taskShot)){throw "Missing packaged screenshot: $taskName"}
            Copy-Item -LiteralPath $taskShot -Destination (Join-Path $taskPreview $taskName) -Force
        }
    }
    Write-Output 'Packaged rock collision, forage pickups/deposits and late-join difficulty replication: PASS'
} finally {
    foreach($taskProcess in $taskProcesses){if(!$taskProcess.HasExited){Stop-Process -Id $taskProcess.Id -ErrorAction SilentlyContinue}}
}
