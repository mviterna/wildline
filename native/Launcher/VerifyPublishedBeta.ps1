# Real download/install/startup check. Requires no existing registered beta.
param([string]$ReceiptPath = "$PSScriptRoot/../Build/Beta/LatestInstaller.json", [string]$LauncherOverride)
$ErrorActionPreference = 'Stop'
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
Write-Output "Online check: starting public launcher; logs in $taskTest"
Start-Process -FilePath $taskLauncher -WindowStyle Hidden | Out-Null
$taskDeadline = (Get-Date).AddMinutes(20)
$taskMarker = Join-Path $taskInstall 'current.version'
while ((Get-Date) -lt $taskDeadline) {
    if ((Test-Path $taskMarker) -and (Get-Content $taskMarker -Raw).Trim() -eq $taskReceipt.version) { break }
    Start-Sleep -Seconds 2
}
if (!(Test-Path $taskMarker) -or (Get-Content $taskMarker -Raw).Trim() -ne $taskReceipt.version) { throw 'Public update did not install and activate.' }
$taskGame = Wait-Game
$taskFirstPid = $taskGame.Id
Write-Output "PASS public bootstrap -> signed feed -> download parts -> setup helper -> responding game ($taskFirstPid)."
$taskCache = Join-Path $env:LOCALAPPDATA ('Wildline/Updates/' + $taskReceipt.version)
if ((Test-Path (Join-Path $taskCache 'Setup.exe')) -or @(Get-ChildItem $taskCache -Filter '*.part*' -File).Count) { throw 'Successful update kept its large download cache.' }
$taskInstallLogTime = (Get-Item (Join-Path $taskCache 'install.log')).LastWriteTimeUtc
$taskVerify = Start-Process -FilePath $taskLauncher -ArgumentList '--verify-only' -WindowStyle Hidden -PassThru -Wait
if ($taskVerify.ExitCode) { throw 'Installed public payload verification failed.' }
Close-TestGame
Start-Process -FilePath $taskLauncher -WindowStyle Hidden | Out-Null
$taskGame = Wait-Game
if ((Get-Item (Join-Path $taskCache 'install.log')).LastWriteTimeUtc -ne $taskInstallLogTime) { throw 'Current-version launch unnecessarily reinstalled the game.' }
Write-Output "PASS current-version launch ($($taskGame.Id))."
Close-TestGame
Start-Process -FilePath $taskLauncher -ArgumentList '--offline' -WindowStyle Hidden | Out-Null
$taskGame = Wait-Game
Write-Output "PASS explicit offline launch ($($taskGame.Id))."
Close-TestGame
$taskUninstaller = Start-Process -FilePath (Join-Path $taskInstall 'unins000.exe') -ArgumentList ('/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /LOG="' + $taskTest + '\uninstall.log"') -WindowStyle Hidden -PassThru -Wait
if ($taskUninstaller.ExitCode -or (Test-Path $taskRegistry)) { throw 'Online installation did not uninstall cleanly.' }
[ordered]@{ passed = $true; version = $taskReceipt.version; bootstrapSha256 = $taskReceipt.bootstrapSha256; checkedAtUtc = [DateTimeOffset]::UtcNow.ToString('o'); logs = $taskTest; firstGamePid = $taskFirstPid } |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $taskTest 'result.json') -Encoding UTF8
Write-Output "PASS published beta download, install, startup, current-version launch, offline launch, and uninstall. Report: $taskTest/result.json"
