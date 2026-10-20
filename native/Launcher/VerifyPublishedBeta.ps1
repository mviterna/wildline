# Real download/install/startup check. Requires no existing registered beta.
param([string]$ReceiptPath = "$PSScriptRoot/../Build/Beta/LatestInstaller.json", [string]$LauncherOverride, [switch]$Headless)
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$taskRegistry = 'HKCU:/Software/Microsoft/Windows/CurrentVersion/Uninstall/{5BA860F0-CE7A-47E1-887D-EC02489DB723}_is1'
if (Test-Path $taskRegistry) { throw 'Use a Windows account without an installed beta for the online installation check.' }
$taskReceipt = Get-Content $ReceiptPath -Raw | ConvertFrom-Json
$taskTest = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot ('../Build/OnlineInstallerVerification/' + [guid]::NewGuid().ToString('N'))))
$taskInstall = Join-Path $taskTest 'Installed Game'
New-Item -ItemType Directory -Path $taskTest -Force | Out-Null
$taskBootstrap = Join-Path $taskTest 'WildlineSetup.exe'
Invoke-WebRequest -UseBasicParsing 'https://github.com/mviterna/wildline-downloads/releases/download/beta/WildlineSetup.exe' -OutFile $taskBootstrap
if ((Get-FileHash $taskBootstrap -Algorithm SHA256).Hash -ne $taskReceipt.bootstrapSha256) { throw 'Public bootstrap differs from the local verified build.' }
$taskSetup = Start-Process -FilePath $taskBootstrap -ArgumentList ('/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /DIR="' + $taskInstall + '" /LOG="' + $taskTest + '\bootstrap.log"') -WindowStyle Hidden -PassThru -Wait
if ($taskSetup.ExitCode) { throw 'Public bootstrap failed to install.' }
$taskLauncher = Join-Path $taskInstall 'WildlineLauncher.exe'
if ($LauncherOverride) { Copy-Item -LiteralPath $LauncherOverride -Destination $taskLauncher -Force }
function Wait-Game {
    param([int]$Seconds = 180)
    $taskDeadline = (Get-Date).AddSeconds($Seconds)
    $taskGamePath = Join-Path $taskInstall ("versions/" + $taskReceipt.version + '/Wildline/Binaries/Win64/Wildline.exe')
    while ((Get-Date) -lt $taskDeadline) {
        $taskGame = @(Get-Process Wildline -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $taskGamePath -and $_.MainWindowHandle -ne 0 -and $_.Responding })
        if ($taskGame.Count) { return $taskGame[0] }
        Start-Sleep -Seconds 1
    }
    throw "Online check did not reach a responding game. Inspect $taskTest and %LOCALAPPDATA%/Wildline/Updates."
}
function Close-TestGame {
    $taskPrefix = $taskInstall.TrimEnd('\') + '\'
    foreach ($taskGame in @(Get-Process Wildline -ErrorAction SilentlyContinue | Where-Object { $_.Path -and $_.Path.StartsWith($taskPrefix,[StringComparison]::OrdinalIgnoreCase) } | Sort-Object { $_.Path -like '*\Binaries\Win64\*' } -Descending)) {
        if ($taskGame.HasExited) { continue }
        $taskGame.CloseMainWindow() | Out-Null
        if (!$taskGame.WaitForExit(15000)) { Stop-Process -Id $taskGame.Id }
    }
}
function Test-HeadlessPayload {
    $taskLog = Join-Path $taskTest 'public-game-smoke.log'
    $taskGameRoot = Join-Path $taskInstall ('versions/' + $taskReceipt.version)
    Start-Process -FilePath (Join-Path $taskGameRoot 'Wildline.exe') -WorkingDirectory $taskGameRoot -ArgumentList ('-NullRHI -NoSound -unattended -WildlineSmoke -WildlineProfileName=UpdaterQA -UserDir="' + $taskTest + '\Profile" -abslog="' + $taskLog + '"') -WindowStyle Hidden | Out-Null
    $taskDeadline = (Get-Date).AddSeconds(90)
    while ((Get-Date) -lt $taskDeadline) {
        if ((Test-Path $taskLog) -and (Get-Content $taskLog -Raw) -match 'WILDLINE SMOKE RESULT: PASS') { return }
        Start-Sleep -Milliseconds 500
    }
    throw 'Publicly installed headless game smoke failed.'
}
function Wait-HeadlessLauncher($Process) {
    if (!$Process.WaitForExit(180000)) { throw 'Public launcher did not finish.' }
    $Process.Refresh()
    if ($Process.ExitCode) { throw "Public launcher command failed: $($Process.ExitCode)" }
}
Write-Output "Online check: starting public launcher; logs in $taskTest"
Start-Process -FilePath $taskLauncher -WindowStyle Hidden | Out-Null
$taskDeadline = (Get-Date).AddMinutes(20)
$taskMarker = Join-Path $taskInstall 'current.version'
while ((Get-Date) -lt $taskDeadline) {
    if ((Test-Path $taskMarker) -and (Get-Content $taskMarker -Raw).Trim() -eq $taskReceipt.version) { break }
    Start-Sleep -Seconds 2
}
if (!(Test-Path $taskMarker) -or (Get-Content $taskMarker -Raw).Trim() -ne $taskReceipt.version) { throw 'Public update did not install and activate.' }
$taskCache = Join-Path $env:LOCALAPPDATA ('Wildline/Updates/' + $taskReceipt.version)
if ($Headless) {
    $taskCleanupDeadline = (Get-Date).AddMinutes(3)
    while ((Get-Date) -lt $taskCleanupDeadline) {
        if ((Test-Path (Join-Path $taskCache 'install.log')) -and !(Test-Path (Join-Path $taskCache 'Setup.exe')) -and !@(Get-ChildItem $taskCache -Filter '*.part*' -File -ErrorAction SilentlyContinue).Count) { break }
        Start-Sleep -Seconds 1
    }
    Close-TestGame
    Test-HeadlessPayload
    $taskFirstPid = $null
    Write-Output 'PASS public bootstrap -> signed feed -> download parts -> setup helper -> headless game smoke.'
} else {
    $taskGame = Wait-Game
    $taskFirstPid = $taskGame.Id
    Write-Output "PASS public bootstrap -> signed feed -> download parts -> setup helper -> responding game ($taskFirstPid)."
}
if ((Test-Path (Join-Path $taskCache 'Setup.exe')) -or @(Get-ChildItem $taskCache -Filter '*.part*' -File).Count) { throw 'Successful update kept its large download cache.' }
$taskInstallLogTime = (Get-Item (Join-Path $taskCache 'install.log')).LastWriteTimeUtc
$taskVerify = Start-Process -FilePath $taskLauncher -ArgumentList '--verify-only' -WindowStyle Hidden -PassThru -Wait
if ($taskVerify.ExitCode) { throw 'Installed public payload verification failed.' }
Close-TestGame
$taskCurrentLauncher = Start-Process -FilePath $taskLauncher -WindowStyle Hidden -PassThru
if ($Headless) { Wait-HeadlessLauncher $taskCurrentLauncher } else { $taskGame = Wait-Game }
if ((Get-Item (Join-Path $taskCache 'install.log')).LastWriteTimeUtc -ne $taskInstallLogTime) { throw 'Current-version launch unnecessarily reinstalled the game.' }
Write-Output 'PASS current-version launcher completes without reinstalling.'
Close-TestGame
$taskOfflineLauncher = Start-Process -FilePath $taskLauncher -ArgumentList '--offline' -WindowStyle Hidden -PassThru
if ($Headless) { Wait-HeadlessLauncher $taskOfflineLauncher } else { $taskGame = Wait-Game }
Write-Output 'PASS explicit offline launcher command.'
Close-TestGame
$taskUninstaller = Start-Process -FilePath (Join-Path $taskInstall 'unins000.exe') -ArgumentList ('/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /LOG="' + $taskTest + '\uninstall.log"') -WindowStyle Hidden -PassThru -Wait
if ($taskUninstaller.ExitCode -or (Test-Path $taskRegistry)) { throw 'Online installation did not uninstall cleanly.' }
[ordered]@{ passed = $true; version = $taskReceipt.version; bootstrapSha256 = $taskReceipt.bootstrapSha256; checkedAtUtc = [DateTimeOffset]::UtcNow.ToString('o'); logs = $taskTest; firstGamePid = $taskFirstPid; headless = [bool]$Headless; graphicalWindowVerified = !$Headless } |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $taskTest 'result.json') -Encoding UTF8
Write-Output "PASS published beta download, install, startup, current-version launch, offline launch, and uninstall. Report: $taskTest/result.json"
if ($Headless) { Write-Output 'Headless scope: game smoke passed with NullRHI; launcher commands exited successfully. Graphical windows and rendering were not verified.' }
