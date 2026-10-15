param([string]$GamePath,[string[]]$Maps=@('Blackwood','Frostwood','Farmstead','Neighborhood','NeighborhoodCedar'))
$ErrorActionPreference='Stop'
if(!$GamePath){. (Join-Path $PSScriptRoot 'PackagedBuilds.ps1');$GamePath=(Get-LatestWildlineBuild -BuildRoot (Join-Path $PSScriptRoot 'Build')).GamePath}
if(!(Test-Path -LiteralPath $GamePath)){throw 'Build the game first.'}
foreach($taskMap in $Maps){
    $taskProfile=Join-Path $PSScriptRoot "Unreal/Wildline/Saved/ThermalVerification/$taskMap"
    $taskLog=Join-Path $PSScriptRoot "thermal-$taskMap.log"
    $taskArgs="/Game/Maps/$taskMap -WildlineThermalReview -WildlineNoIntro -unattended -NoSound -UserDir=`"$taskProfile`" -abslog=`"$taskLog`" -RenderOffscreen -windowed -ResX=1600 -ResY=900 -ForceRes -preferNvidia"
    $taskProcess=Start-Process -FilePath $GamePath -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
    try{
        while(!$taskProcess.WaitForExit(1000)){if((Get-Date)-$taskProcess.StartTime -gt [TimeSpan]::FromMinutes(5)){throw "Thermal probe timed out: $taskMap"}}
        $taskText=Get-Content -LiteralPath $taskLog -Raw
        if($taskProcess.ExitCode -ne 0 -or $taskText -notmatch 'WILDLINE THERMAL REVIEW: PASS'){throw "Thermal probe failed: $taskMap. See $taskLog"}
        if($taskText -match 'LogMaterial: Error|LogShaderCompilers: Error|Fatal error'){throw "Render diagnostics failed: $taskMap"}
        foreach($taskShot in @('ThermalNormal','ThermalUnscoped','ThermalHot','ThermalCover','ThermalDead','ThermalExpired')){
            $taskSource=@("Saved/Screenshots/Windows/$taskShot.png","Wildline/Saved/Screenshots/Windows/$taskShot.png") | ForEach-Object {Join-Path $taskProfile $_} | Where-Object {Test-Path -LiteralPath $_} | ForEach-Object {Get-Item -LiteralPath $_} | Where-Object {$_.LastWriteTime -ge $taskProcess.StartTime} | Select-Object -First 1
            if(!$taskSource){throw "Missing fresh capture: $taskMap/$taskShot"}
            Copy-Item -LiteralPath $taskSource.FullName -Destination (Join-Path $PSScriptRoot "Preview/${taskMap}_$taskShot.png") -Force
        }
        Write-Output "PASS $taskMap thermal activation, squirrel body/fur, cover capture, death, expiration, cooldown and match rules"
    }finally{if(!$taskProcess.HasExited){Stop-Process -Id $taskProcess.Id -ErrorAction SilentlyContinue}}
}
