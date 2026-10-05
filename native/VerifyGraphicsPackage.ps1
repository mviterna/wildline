$ErrorActionPreference='Stop'
$taskGame=Join-Path $PSScriptRoot 'Build/Windows/Wildline/Binaries/Win64/Wildline.exe'
if(!(Test-Path -LiteralPath $taskGame)){throw 'Build the Windows game first.'}
# Use a separate profile to exercise migration without altering the player's settings.
$taskProfile=Join-Path $PSScriptRoot 'test-results/graphics-profile'
$taskConfig=Join-Path $taskProfile 'Saved/Config/Windows'
New-Item -ItemType Directory -Path $taskConfig -Force | Out-Null
$taskFixture=@'
[/Script/Wildline.WildlineUserSettings]
Version=5
ResolutionSizeX=1600
ResolutionSizeY=900
LastUserConfirmedResolutionSizeX=1600
LastUserConfirmedResolutionSizeY=900
FullscreenMode=2
LastConfirmedFullscreenMode=2
bUseDynamicResolution=True

[ScalabilityGroups]
sg.ResolutionQuality=50
sg.ViewDistanceQuality=0
sg.AntiAliasingQuality=0
sg.PostProcessQuality=0
sg.ShadowQuality=0
sg.GlobalIlluminationQuality=0
sg.ReflectionQuality=0
sg.TextureQuality=0
sg.EffectsQuality=0
sg.FoliageQuality=0
sg.ShadingQuality=0
sg.LandscapeQuality=0
'@
Set-Content -LiteralPath (Join-Path $taskConfig 'GameUserSettings.ini') -Value $taskFixture -Encoding utf8
foreach($taskRun in @('migration','restart')){
    $taskLog=Join-Path $PSScriptRoot ('graphics-'+$taskRun+'.log')
    $taskArgs='-WildlineSettingsReview -unattended -NoSound -RenderOffscreen -windowed -ResX=1600 -ResY=900 -ForceRes -preferNvidia -UserDir="'+$taskProfile.Replace('\','/')+'/" -abslog="'+$taskLog.Replace('\','/')+'"'
    $taskProcess=Start-Process -FilePath $taskGame -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
    if(!$taskProcess.WaitForExit(60000)){throw "Graphics verification timed out: $taskRun ($($taskProcess.Id))"}
    if($taskProcess.ExitCode -ne 0){throw "Graphics verification failed: $taskRun, exit $($taskProcess.ExitCode)"}
    if((Get-Content -LiteralPath $taskLog -Raw) -notmatch 'WILDLINE SETTINGS RESULT: PASS'){throw "Graphics result marker missing: $taskRun"}
}
$taskSaved=Get-Content -LiteralPath (Join-Path $taskConfig 'GameUserSettings.ini') -Raw
if($taskSaved -match 'bUseDynamicResolution=True' -or $taskSaved -match 'sg\.\w+Quality=0(?:\r?\n|$)'){throw 'Saved low quality settings were not upgraded.'}
New-Item -ItemType Directory -Path (Join-Path $PSScriptRoot 'Preview') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $taskProfile 'Saved/Screenshots/Windows/HighQualitySettings.png') -Destination (Join-Path $PSScriptRoot 'Preview/HighQualitySettings.png') -Force
Copy-Item -LiteralPath (Join-Path $taskProfile 'Saved/Screenshots/Windows/HighQualitySettings720.png') -Destination (Join-Path $PSScriptRoot 'Preview/HighQualitySettings720.png') -Force
Write-Output 'High quality migration, live resolution changes, saved settings and restart: PASS'
