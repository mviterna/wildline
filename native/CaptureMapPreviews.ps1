param([string]$EngineRoot='D:/EpicEngine/UE_5.8',[string[]]$Maps=@('TaylorsvilleLake','Blackwood','Frostwood','Farmstead','Neighborhood','NeighborhoodCedar'))
$ErrorActionPreference='Stop'
$taskProject=Join-Path $PSScriptRoot 'Unreal/Wildline/Wildline.uproject'
$taskOutput=Join-Path $PSScriptRoot 'Unreal/Wildline/ArtSource/Lake/Previews'
New-Item -ItemType Directory -Path $taskOutput -Force | Out-Null
foreach($taskMap in $Maps){
    if($taskMap -notin @('TaylorsvilleLake','Blackwood','Frostwood','Farmstead','Neighborhood','NeighborhoodCedar')){throw "Unknown preview map: $taskMap"}
    $taskLog=Join-Path $PSScriptRoot "lake-preview-$taskMap.log"
    $taskProfile=Join-Path $PSScriptRoot "Unreal/Wildline/Saved/LakeVerification/Preview-$taskMap"
    $taskArgs="`"$taskProject`" /Game/Maps/$taskMap -game -WildlineMapPreview -WildlineProfileName=MapPreview -WildlineNoIntro -unattended -NoSound -UserDir=`"$taskProfile`" -abslog=`"$taskLog`" -RenderOffscreen -windowed -ResX=1280 -ResY=720 -ForceRes -preferNvidia"
    $taskStarted=Get-Date
    $taskProcess=Start-Process -FilePath (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
    try{
        while(!$taskProcess.WaitForExit(1000)){if((Get-Date)-$taskStarted -gt [TimeSpan]::FromMinutes(4)){throw "Map capture timed out: $taskMap"}}
        if($taskProcess.ExitCode -ne 0){throw "Map capture failed: $taskMap. See $taskLog"}
        $taskImage=Get-Item -LiteralPath (Join-Path $taskOutput "$taskMap.png")
        if($taskImage.LastWriteTime -lt $taskStarted -or $taskImage.Length -lt 20000){throw "Missing or stale map preview: $taskMap"}
        Write-Output "Captured $taskMap"
    }finally{if(!$taskProcess.HasExited){Stop-Process -Id $taskProcess.Id}}
}
& (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') $taskProject "-ExecutePythonScript=$(Join-Path $PSScriptRoot 'Unreal/Wildline/Scripts/import_map_previews.py')" -unattended -NullRHI -NoSound "-abslog=$(Join-Path $PSScriptRoot 'lake-previews-import.log')"
if($LASTEXITCODE -ne 0){throw 'Map preview import failed.'}
if((Get-Content -LiteralPath (Join-Path $PSScriptRoot 'lake-previews-import.log') -Raw) -notmatch 'MAP PREVIEWS IMPORT PASS'){throw 'Map preview import did not confirm success.'}
