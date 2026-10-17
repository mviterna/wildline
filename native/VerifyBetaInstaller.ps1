param([Parameter(Mandatory=$true)][string]$SetupPath,[string]$PreviousSetupPath,[string]$BootstrapPath)
$ErrorActionPreference = 'Stop'
$taskSetup = (Resolve-Path -LiteralPath $SetupPath).Path
if (!$BootstrapPath) { $BootstrapPath = Join-Path (Split-Path $taskSetup) 'WildlineSetup.exe' }
$BootstrapPath = (Resolve-Path -LiteralPath $BootstrapPath).Path
$taskInitialSetup = $taskSetup
if ($PreviousSetupPath) { $taskInitialSetup = (Resolve-Path -LiteralPath $PreviousSetupPath).Path }
# Refuse to redirect an existing beta installation while exercising the installer.
$taskUninstallKey = 'HKCU:/Software/Microsoft/Windows/CurrentVersion/Uninstall/{5BA860F0-CE7A-47E1-887D-EC02489DB723}_is1'
if (Test-Path $taskUninstallKey) { throw 'An installed WILDLINE Beta already exists. Use a clean Windows account for installer verification.' }
$taskTest = Join-Path $PSScriptRoot ('Build/InstallerVerification/' + [guid]::NewGuid().ToString('N'))
$taskInstall = Join-Path $taskTest 'Installed Game'
New-Item -ItemType Directory -Path $taskTest -Force | Out-Null
function Invoke-SetupProbe([string]$Executable, [string]$Arguments) {
    $taskProcess = Start-Process -FilePath $Executable -ArgumentList $Arguments -WindowStyle Hidden -PassThru
    if (!$taskProcess.WaitForExit(600000)) { throw "Installer operation timed out; inspect process $($taskProcess.Id)." }
    $taskProcess.Refresh()
    if ($taskProcess.ExitCode -ne 0) { throw "Installer operation failed: $($taskProcess.ExitCode). Logs: $taskTest" }
}
$taskUninstaller = Join-Path $taskInstall 'unins000.exe'
function Assert-InstalledPayload {
    $script:taskPayloadRoot = $taskInstall
    if (Test-Path (Join-Path $taskInstall 'current.version')) {
        $taskVersion = (Get-Content (Join-Path $taskInstall 'current.version') -Raw).Trim()
        if ($taskVersion -notmatch '^\d+\.\d+\.\d+-beta\.\d{8}\.\d{6}$') { throw 'Invalid active version.' }
        $script:taskPayloadRoot = Join-Path $taskInstall "versions/$taskVersion"
    }
    $taskInfo = Get-Content -LiteralPath (Join-Path $taskPayloadRoot 'BetaBuild.json') -Raw | ConvertFrom-Json
    if (!(Test-Path -LiteralPath (Join-Path $taskPayloadRoot 'Engine/Build/InstalledProjectBuild.txt'))) { throw 'Installed-project marker is missing; personal saves could go inside the install directory.' }
    foreach ($taskEntry in $taskInfo.files) {
        $taskFile = Join-Path $taskPayloadRoot $taskEntry.path
        if ((Get-FileHash -LiteralPath $taskFile -Algorithm SHA256).Hash -ne $taskEntry.sha256) { throw "Installed payload hash mismatch: $($taskEntry.path)" }
        if ($taskEntry.path -match '(?i)(^|\\)Saved(\\|$)|\.pdb$|^Manifest_') { throw "Private/debug file in installer: $($taskEntry.path)" }
    }
    if ((Get-ItemProperty -LiteralPath $taskUninstallKey).DisplayVersion -ne $taskInfo.version) { throw 'Registered version differs from the installed build.' }
}
try {
    Invoke-SetupProbe $taskInitialSetup "/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /DIR=`"$taskInstall`" /LOG=`"$taskTest/install.log`""
    Assert-InstalledPayload
    $taskWasLegacy = $taskPayloadRoot -eq $taskInstall
    $taskInitialGuide = [IO.File]::ReadAllBytes((Join-Path $taskPayloadRoot 'BetaReadme.txt'))
    # Simulate removed files in each immutable package tree, plus an existing
    # personal save. The update must clean the former and preserve the latter.
    $taskStaleFiles = @('Engine/obsolete-runtime.dll', 'Wildline/Binaries/Win64/obsolete-game.dll', 'Wildline/Content/Paks/obsolete-build.pak', 'Wildline/Config/obsolete-build.ini')
    foreach ($taskRelative in $taskStaleFiles) {
        $taskStaleFile = Join-Path $taskPayloadRoot $taskRelative
        New-Item -ItemType Directory -Path (Split-Path $taskStaleFile) -Force | Out-Null
        Set-Content -LiteralPath $taskStaleFile -Value 'obsolete build'
    }
    $taskSave = Join-Path $taskInstall 'Wildline/Saved/Profiles/upgrade-test.txt'
    New-Item -ItemType Directory -Path (Split-Path $taskSave) -Force | Out-Null
    Set-Content -LiteralPath $taskSave -Value 'preserve this profile'
    $taskSaveHash = (Get-FileHash -LiteralPath $taskSave).Hash
    $taskRepairFile = Join-Path $taskPayloadRoot 'BetaReadme.txt'
    Set-Content -LiteralPath $taskRepairFile -Value 'outdated payload'
    (Get-Item -LiteralPath $taskRepairFile).LastWriteTime = (Get-Date).AddYears(1)
    # No /DIR: discover the previous custom install path automatically.
    Invoke-SetupProbe $taskSetup "/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /LOG=`"$taskTest/upgrade.log`""
    Assert-InstalledPayload
    foreach ($taskRelative in $taskStaleFiles) {
        if (Test-Path -LiteralPath (Join-Path $taskPayloadRoot $taskRelative)) { throw "Upgrade kept obsolete file: $taskRelative" }
    }
    if ((Get-FileHash -LiteralPath $taskSave).Hash -ne $taskSaveHash) { throw 'Upgrade changed personal data.' }
    if ($taskWasLegacy) {
        # Restore the intentionally damaged legacy guide, then verify that the
        # real old package can be selected and the new package selected again.
        [IO.File]::WriteAllBytes((Join-Path $taskInstall 'BetaReadme.txt'), $taskInitialGuide)
        [Reflection.Assembly]::LoadFrom((Join-Path $PSScriptRoot 'Build/Launcher/WildlineLauncher.exe')) | Out-Null
        $taskNewVersion = [Wildline.Updates.Core]::ActiveVersion($taskInstall)
        [Wildline.Updates.Core]::Rollback($taskInstall)
        if ([Wildline.Updates.Core]::GameRoot($taskInstall) -ne $taskInstall) { throw 'Legacy rollback did not select the old package.' }
        [Wildline.Updates.Core]::Activate($taskInstall, $taskNewVersion)
        Write-Output 'PASS legacy migration, verified rollback, and reactivation of the new package.'
    }
    $taskShortcut = Join-Path $env:APPDATA 'Microsoft/Windows/Start Menu/Programs/WILDLINE Beta/WILDLINE Beta.lnk'
    $taskLink = (New-Object -ComObject WScript.Shell).CreateShortcut($taskShortcut)
    if ($taskLink.TargetPath -ne (Join-Path $taskInstall 'WildlineLauncher.exe') -or $taskLink.WorkingDirectory -ne $taskInstall) { throw 'Start menu shortcut does not point to the updater launcher.' }
    $taskLauncherProbe = Start-Process -FilePath (Join-Path $taskInstall 'WildlineLauncher.exe') -ArgumentList '--verify-only' -WindowStyle Hidden -PassThru -Wait
    if ($taskLauncherProbe.ExitCode) { throw 'Installed launcher payload verification failed.' }
    $taskLauncherHash = (Get-FileHash (Join-Path $taskInstall 'WildlineLauncher.exe') -Algorithm SHA256).Hash
    $taskGame = Join-Path $taskPayloadRoot 'Wildline.exe'
    foreach ($taskProbe in @(
        @('classic', '/Game/Maps/Blackwood -WildlineSmoke -NullRHI -NoSound', 'WILDLINE SMOKE RESULT: PASS'),
        @('farm', '/Game/Maps/Farmstead -WildlineFarmSmoke -NullRHI -NoSound', 'WILDLINE FARM SMOKE: PASS'),
        @('hill', '/Game/Maps/Neighborhood -WildlineHillSmoke -NullRHI -NoSound', 'WILDLINE HILL SMOKE: PASS'),
        @('intro', '/Game/Maps/Blackwood -WildlineIntroSmoke -NullRHI -NoSound', 'WILDLINE INTRO SMOKE: PASS')
    )) {
        $taskLog = Join-Path $taskTest ($taskProbe[0] + '.log')
        $taskProfile = Join-Path $taskTest ('Profiles/' + $taskProbe[0])
        $taskProcess = Start-Process -FilePath $taskGame -WorkingDirectory $taskPayloadRoot -ArgumentList ($taskProbe[1] + " -unattended -UserDir=`"$taskProfile`" -abslog=`"$taskLog`"") -WindowStyle Hidden -PassThru
        # The Unreal bootstrap exits before the child game; wait for its pass marker.
        $taskDeadline = (Get-Date).AddSeconds(60)
        while ((Get-Date) -lt $taskDeadline) {
            if ((Test-Path -LiteralPath $taskLog) -and (Get-Content -LiteralPath $taskLog -Raw) -match $taskProbe[2]) { break }
            Start-Sleep -Milliseconds 500
        }
        if (!(Test-Path -LiteralPath $taskLog) -or (Get-Content -LiteralPath $taskLog -Raw) -notmatch $taskProbe[2]) { throw "Installed game probe failed: $($taskProbe[0]). Logs: $taskTest" }
        # Allow the self-terminating probe to finish before updating its binaries.
        $taskChildren = Get-Process Wildline -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq (Join-Path $taskPayloadRoot 'Wildline/Binaries/Win64/Wildline.exe') }
        foreach ($taskChild in $taskChildren) {
            if (!$taskChild.WaitForExit(10000)) { throw "Game probe did not exit: $($taskChild.Id)" }
        }
    }
    $taskRepairFile = Join-Path $taskPayloadRoot 'BetaReadme.txt'
    Set-Content -LiteralPath $taskRepairFile -Value 'same-version repair needed'
    # Repair must still clean obsolete content if the bootstrap was deleted.
    Remove-Item -LiteralPath $taskGame
    $taskRepairStaleFile = Join-Path $taskPayloadRoot 'Wildline/Content/Paks/obsolete-repair.pak'
    Set-Content -LiteralPath $taskRepairStaleFile -Value 'obsolete repair payload'
    $taskOtherInstall = Join-Path $taskTest 'Must Not Create Another Install'
    Invoke-SetupProbe $taskSetup "/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /DIR=`"$taskOtherInstall`" /LOG=`"$taskTest/reinstall.log`""
    Assert-InstalledPayload
    if (Test-Path -LiteralPath $taskRepairStaleFile) { throw 'Repair kept obsolete content when the game EXE was missing.' }
    if (Test-Path -LiteralPath $taskOtherInstall) { throw 'Reinstall created a second installation instead of replacing the previous one.' }
    if ((Get-FileHash -LiteralPath $taskSave).Hash -ne $taskSaveHash) { throw 'Reinstall changed personal data.' }
    Invoke-SetupProbe $taskUninstaller "/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /LOG=`"$taskTest/uninstall.log`""
    if ((Test-Path -LiteralPath $taskGame) -or (Test-Path $taskUninstallKey) -or (Test-Path -LiteralPath $taskShortcut)) { throw 'Uninstall left registered game files or shortcuts.' }
    if ((Get-FileHash -LiteralPath $taskSave).Hash -ne $taskSaveHash) { throw 'Uninstall changed personal data.' }
    Invoke-SetupProbe $BootstrapPath "/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /DIR=`"$taskInstall`" /LOG=`"$taskTest/bootstrap.log`""
    if ((Get-FileHash (Join-Path $taskInstall 'WildlineLauncher.exe') -Algorithm SHA256).Hash -ne $taskLauncherHash) { throw 'Bootstrap and full installer contain different launchers.' }
    $taskLink = (New-Object -ComObject WScript.Shell).CreateShortcut($taskShortcut)
    if ($taskLink.TargetPath -ne (Join-Path $taskInstall 'WildlineLauncher.exe')) { throw 'Bootstrap shortcut bypasses the launcher.' }
    Invoke-SetupProbe $taskUninstaller "/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /LOG=`"$taskTest/bootstrap-uninstall.log`""
    if (Test-Path $taskUninstallKey) { throw 'Bootstrap uninstall left a registered installation.' }
    if ((Get-FileHash -LiteralPath $taskSave).Hash -ne $taskSaveHash) { throw 'Bootstrap uninstall changed personal data.' }
    [ordered]@{ passed = $true; sha256 = (Get-FileHash $taskSetup -Algorithm SHA256).Hash; bootstrapSha256 = (Get-FileHash $BootstrapPath -Algorithm SHA256).Hash; checkedAtUtc = [DateTimeOffset]::UtcNow.ToString('o'); logs = $taskTest } |
        ConvertTo-Json | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'Build/Beta/InstallerVerification.json') -Encoding UTF8
    Write-Output "PASS: install in path with spaces, upgrade detection, overwrite/repair, obsolete file cleanup, saved data preservation, all payload hashes, launcher verification, shortcuts, three modes + intro, same-version reinstall, single install path, full and online bootstrap uninstall. Logs: $taskTest"
} finally {
    if ((Test-Path $taskUninstallKey) -or (Test-Path -LiteralPath (Join-Path $taskInstall 'Wildline.exe'))) {
        Write-Warning "Verification left an installation at $taskInstall. Inspect its logs before uninstalling."
    }
}
