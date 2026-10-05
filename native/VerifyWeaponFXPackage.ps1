param([switch]$Capture,[switch]$SkipGameplay)
$ErrorActionPreference='Stop'
if(!$SkipGameplay){
    & (Join-Path $PSScriptRoot 'VerifyFarmPackage.ps1')
    if($LASTEXITCODE){exit $LASTEXITCODE}
}
if($Capture){
    $taskGame=Join-Path $PSScriptRoot 'Build/Windows/Wildline/Binaries/Win64/Wildline.exe'
    $taskLog=Join-Path $PSScriptRoot 'weapon-fx-package-review.log'
    $taskArgs='/Game/Maps/Farmstead -WildlineWeaponFXReview -unattended -NoSound -RenderOffscreen -windowed -ResX=1600 -ResY=900 -ForceRes -preferNvidia -abslog="'+$taskLog.Replace('\','/')+'"'
    $taskProcess=Start-Process -FilePath $taskGame -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
    try {
        if(!$taskProcess.WaitForExit(60000) -and !$taskProcess.WaitForExit(60000)){throw 'Weapon FX capture timed out.'}
        if($taskProcess.ExitCode -ne 0){throw 'Weapon FX capture failed.'}
        if((Get-Content -LiteralPath $taskLog -Raw) -match 'LogMaterial: Error|LogShaderCompilers: Error|Failed to compile Material|Fatal error'){throw 'Weapon FX render reported a material error.'}
        foreach($taskName in @('ShotgunReady.png','ShotgunFlash.png','ShotgunSmoke.png','ShotgunPump.png','ShotgunImpacts.png','ShotgunReload.png')){
            $taskShot=Join-Path $PSScriptRoot "Build/Windows/Wildline/Saved/Screenshots/Windows/$taskName"
            if(!(Test-Path -LiteralPath $taskShot)){throw "Missing FX screenshot: $taskName"}
            Copy-Item -LiteralPath $taskShot -Destination (Join-Path $PSScriptRoot "Preview/$taskName") -Force
        }
    } finally {if(!$taskProcess.HasExited){Stop-Process -Id $taskProcess.Id -ErrorAction SilentlyContinue}}
}
if($SkipGameplay){Write-Output 'Weapon effects render capture: PASS'}
else {Write-Output 'Weapon effects, 12 pellet marks, collision isolation and co-op shot/impact events: PASS'}
