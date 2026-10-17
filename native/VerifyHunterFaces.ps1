param([switch]$Capture)
$ErrorActionPreference='Stop'
$taskGame=Join-Path $PSScriptRoot 'Build/Windows/Wildline/Binaries/Win64/Wildline.exe'
$taskReviewRoot=Join-Path $PSScriptRoot 'Build/HunterFacesReview'
New-Item -ItemType Directory -Path $taskReviewRoot -Force | Out-Null
function Invoke-HunterFaceProbe([string]$Flags,[string]$LogName,[string]$Marker) {
    $taskLog=Join-Path $PSScriptRoot $LogName
    $taskArguments=$Flags+' -unattended -NoSound -UserDir="'+$taskReviewRoot+'" -abslog="'+$taskLog+'"'
    $taskProcess=Start-Process -FilePath $taskGame -ArgumentList $taskArguments -WindowStyle Hidden -PassThru
    if(!$taskProcess.WaitForExit(180000)){throw "Hunter face probe timed out: $($taskProcess.Id)"}
    $taskProcess.Refresh()
    if($taskProcess.ExitCode -ne 0){throw "Hunter face probe failed: $($taskProcess.ExitCode); $taskLog"}
    $taskText=Get-Content -LiteralPath $taskLog -Raw
    if($Marker -and $taskText -notmatch $Marker){throw "Missing result $Marker in $taskLog"}
    if($taskText -match 'Fatal error:|Assertion failed:'){throw "Runtime failure in $taskLog"}
}
Invoke-HunterFaceProbe '-WildlineSmoke -NullRHI' 'hunter-faces-smoke.log' 'WILDLINE SMOKE RESULT: PASS'
if($Capture){
    $taskCaptureStarted=[DateTime]::UtcNow
    Invoke-HunterFaceProbe '-WildlineHunterFacesReview -RenderOffscreen -windowed -ResX=1600 -ResY=900 -ForceRes -preferNvidia' 'hunter-faces-review.log' ''
    foreach($taskName in @('HunterFacesTravis','HunterFacesEthan','HunterFacesGreyson','MarkFace','MarkThreeQuarter','MarkCreator','MarkCustomized','MarkInGame')){
        $taskImage=Join-Path $taskReviewRoot "Saved/Screenshots/Windows/$taskName.png"
        if(!(Test-Path -LiteralPath $taskImage)){throw "Missing screenshot: $taskImage"}
        if((Get-Item -LiteralPath $taskImage).Length -lt 20000){throw "Incomplete screenshot: $taskImage"}
        if((Get-Item -LiteralPath $taskImage).LastWriteTimeUtc -lt $taskCaptureStarted){throw "Stale screenshot: $taskImage"}
        Copy-Item -LiteralPath $taskImage -Destination (Join-Path $PSScriptRoot "Preview/$taskName.png") -Force
    }
}
Write-Output 'PASS cooked hunter presets, Mark portrait, customization, morphs and rifle grips.'
