param([switch]$Editor,[switch]$Capture,[string]$GamePath,[switch]$SkipGallery,[switch]$SkipProfile)
$ErrorActionPreference='Stop'
if(!$SkipGallery -and !$Capture){throw 'Gallery validation requires -Capture for mesh rendering and scroll layout. Use -SkipGallery for headless profile-only checks.'}
$taskExe=if($Editor){'D:/EpicEngine/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'}elseif($GamePath){$GamePath}else{throw 'Pass GamePath or Editor.'}
$taskStamp=[guid]::NewGuid().ToString('N');$taskRoot=Join-Path $PSScriptRoot "Unreal/Wildline/Saved/HeadwearReview/$taskStamp";$taskPreview=Join-Path $PSScriptRoot "Preview/Headwear-$taskStamp"
function Invoke-HeadwearProbe([string]$Mode,[string]$Fixture,[bool]$Render){
    New-Item -ItemType Directory -Path $Fixture -Force | Out-Null
    $taskLog=Join-Path $taskRoot "$Mode.log"
    $taskArgs=if($Editor){'"'+(Join-Path $PSScriptRoot 'Unreal/Wildline/Wildline.uproject')+'" -game '}else{''}
    $taskArgs+='/Game/Maps/Blackwood -'+$Mode+' -WildlineNoIntro -WildlineDiscoveryPort=7792 -unattended -UserDir="'+$Fixture+'" -abslog="'+$taskLog+'" '
    if($Mode -eq 'WildlineHeadwearReview'){$taskArgs+='-WildlineProfileName=HeadwearReview '}
    $taskArgs+=if($Render){'-RenderOffscreen -windowed -ResX=1600 -ResY=900 -ForceRes -preferNvidia'}else{'-NullRHI -NoSound'}
    $taskProcess=Start-Process -FilePath $taskExe -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
    try{
        $taskDeadline=(Get-Date).AddMinutes(3)
        while(!$taskProcess.WaitForExit(1000)){if((Get-Date)-gt $taskDeadline){throw "$Mode timed out"}}
        $taskText=Get-Content -LiteralPath $taskLog -Raw
        if($taskProcess.ExitCode -ne 0 -or $taskText -notmatch 'WILDLINE HEADWEAR REVIEW: PASS' -or $taskText -match 'HEADWEAR FAIL|Fatal error'){throw "Headwear/profile probe failed: $taskLog"}
        Write-Output "$Mode : PASS"
    }finally{if(!$taskProcess.HasExited){Stop-Process -Id $taskProcess.Id}}
}
if(!$SkipProfile){
    $taskFixture=Join-Path $taskRoot 'ProfileSession';Invoke-HeadwearProbe 'WildlineProfileWriteProbe' $taskFixture $false
    $taskStable=Join-Path $taskFixture 'Profiles/LocalPlayer.ini'
    if(!(Test-Path -LiteralPath $taskStable) -or (Get-Content $taskStable -Raw) -notmatch 'Name=Hat Tester'){throw 'Stable player profile was not saved.'}
    # Only retire disposable QA settings, simulating a new package directory.
    $taskConfig=Join-Path $taskFixture 'Saved/Config'
    if(Test-Path $taskConfig){Get-ChildItem -LiteralPath $taskConfig -Filter GameUserSettings.ini -File -Recurse | ForEach-Object {Move-Item -LiteralPath $_.FullName -Destination ($_.FullName+'.previous-build')}}
    Invoke-HeadwearProbe 'WildlineProfileReadProbe' $taskFixture $false
}
if(!$SkipGallery){
    $taskGallery=Join-Path $taskRoot 'Gallery';Invoke-HeadwearProbe 'WildlineHeadwearReview' $taskGallery ([bool]$Capture)
    if($Capture){New-Item -ItemType Directory -Path $taskPreview -Force | Out-Null;foreach($taskName in @('HeadwearChicken','HeadwearDinosaur','HeadwearConstruction','HeadwearWizard','HeadwearInGame')){
        $taskShot=Join-Path $taskGallery "Saved/Screenshots/Windows/$taskName.png";if(!(Test-Path $taskShot)){throw "Missing screenshot: $taskShot"};Copy-Item -LiteralPath $taskShot -Destination (Join-Path $taskPreview "$taskName.png")
    }}
}
Write-Output "Evidence: $taskRoot"
if($Capture){Write-Output "Previews: $taskPreview"}
