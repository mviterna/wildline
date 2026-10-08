param([switch]$Capture)
$ErrorActionPreference='Stop'
$taskGame=Join-Path $PSScriptRoot 'Build/Windows/Wildline/Binaries/Win64/Wildline.exe'
if(!(Test-Path -LiteralPath $taskGame)){throw 'Build the Windows game first.'}
$taskProcesses=@()
function Start-ModesProbe($taskFlags,$taskLog){
    $taskProfile=Join-Path $PSScriptRoot ('Unreal/Wildline/Saved/ModesProbes/'+[IO.Path]::GetFileNameWithoutExtension($taskLog))
    $taskArgs=$taskFlags+' -unattended -UserDir="'+$taskProfile+'" -abslog="'+(Join-Path $PSScriptRoot $taskLog).Replace('\','/')+'"'
    Start-Process -FilePath $taskGame -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
}
try {
    $taskProcesses+=Start-ModesProbe '/Game/Maps/Blackwood -WildlineSmoke -NullRHI -NoSound' 'modes-classic-package.log'
    $taskProcesses+=Start-ModesProbe '/Game/Maps/Farmstead -WildlineFarmSmoke -NullRHI -NoSound' 'modes-farm-package.log'
    $taskProcesses+=Start-ModesProbe '/Game/Maps/Neighborhood -WildlineHillSmoke -NullRHI -NoSound' 'modes-hill-package.log'
    $taskProcesses+=Start-ModesProbe '/Game/Maps/Neighborhood?listen -port=7794 -WildlineAutoStart -WildlineHillNetProbe -NullRHI -NoSound' 'modes-hill-net-host.log'
    if($Capture){$taskProcesses+=Start-ModesProbe '/Game/Maps/Neighborhood -WildlineHillReview -RenderOffscreen -windowed -ResX=1600 -ResY=900 -ForceRes -preferNvidia' 'modes-hill-review.log'}
    Start-Sleep -Seconds 20
    $taskProcesses+=Start-ModesProbe '127.0.0.1:7794 -WildlineAutoStart -WildlineSquirrel -WildlineHillNetProbe -NullRHI -NoSound' 'modes-hill-net-client.log'
    Start-Sleep -Seconds 10
    $taskOverflow=Start-ModesProbe '127.0.0.1:7794 -NullRHI -NoSound' 'modes-hill-capacity.log'
    Start-Sleep -Seconds 8
    if(!$taskOverflow.HasExited){Stop-Process -Id $taskOverflow.Id}
    $taskCapacityLog=Get-Content (Join-Path $PSScriptRoot 'modes-hill-capacity.log') -Raw
    if($taskCapacityLog -notmatch 'Game (?:is )?full|Server (?:is )?full'){throw 'Configured human player limit was not enforced.'}
    foreach($taskProcess in $taskProcesses){
        if(!$taskProcess.WaitForExit(60000) -and !$taskProcess.WaitForExit(60000)){throw "Verification timed out: $($taskProcess.Id)"}
        if($taskProcess.ExitCode -ne 0){throw "Verification failed: $($taskProcess.Id), exit $($taskProcess.ExitCode)"}
    }
    foreach($taskCheck in @(
        @('modes-classic-package.log','WILDLINE SMOKE RESULT: PASS'),
        @('modes-farm-package.log','WILDLINE FARM SMOKE: PASS'),
        @('modes-hill-package.log','WILDLINE HILL SMOKE: PASS'),
        @('modes-hill-net-host.log','WILDLINE HILL NETWORK Host: PASS'),
        @('modes-hill-net-client.log','WILDLINE HILL NETWORK Client: PASS')
    )){if((Get-Content (Join-Path $PSScriptRoot $taskCheck[0]) -Raw) -notmatch $taskCheck[1]){throw "Missing pass marker: $($taskCheck[0])"}}
    if($Capture){
        $taskReview=Get-Content (Join-Path $PSScriptRoot 'modes-hill-review.log') -Raw
        if($taskReview -match 'LogMaterial: Error|LogShaderCompilers: Error|Failed to compile Material|Fatal error'){throw 'Render review reported material errors.'}
        foreach($taskName in @('HillMenu.png','HillSquirrel.png','HillCourse.png','HillNeighborhood.png','HillSettings.png')){
            $taskShot=Join-Path $PSScriptRoot "Unreal/Wildline/Saved/ModesProbes/modes-hill-review/Saved/Screenshots/Windows/$taskName"
            if(!(Test-Path -LiteralPath $taskShot)){throw "Missing screenshot: $taskName"}
            Copy-Item -LiteralPath $taskShot -Destination (Join-Path $PSScriptRoot "Preview/$taskName") -Force
        }
    }
    Write-Output 'Packaged Classic, Farm, King of the Hill, real jumps, thermal, multiplayer rules/progress/sounds and player limit: PASS'
} finally {
    foreach($taskProcess in $taskProcesses){if(!$taskProcess.HasExited){Stop-Process -Id $taskProcess.Id -ErrorAction SilentlyContinue}}
    if($taskOverflow -and !$taskOverflow.HasExited){Stop-Process -Id $taskOverflow.Id -ErrorAction SilentlyContinue}
}
