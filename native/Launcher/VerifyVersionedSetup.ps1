# Small real Inno Setup fixtures exercise activation failure and rollback without
# downloading or installing several copies of the full game.
$ErrorActionPreference = 'Stop'
$taskRegistry = 'HKCU:/Software/Microsoft/Windows/CurrentVersion/Uninstall/{5BA860F0-CE7A-47E1-887D-EC02489DB723}_is1'
if (Test-Path $taskRegistry) { throw 'A beta is installed; use a clean Windows account for versioned setup checks.' }
$taskNative = Split-Path $PSScriptRoot
$taskRoot = Join-Path $taskNative ('Build/VersionedSetupVerification/' + [guid]::NewGuid().ToString('N'))
$taskInstall = [IO.Path]::GetFullPath((Join-Path $taskRoot 'Installed Game'))
New-Item -ItemType Directory -Path $taskRoot -Force | Out-Null
$taskCompiler = Join-Path $taskNative 'Tools/InnoSetup/ISCC.exe'
$taskLauncher = [IO.Path]::GetFullPath((Join-Path $taskNative 'Build/Launcher/WildlineLauncher.exe'))
$taskEngineHeader = Get-Content 'D:/EpicEngine/UE_5.8/Engine/Source/Runtime/ApplicationCore/Internal/Windows/WindowsRedistributableValidation.h' -Raw
function Get-FixtureRuntime([string]$Name) {
    $taskMatch = [regex]::Match($taskEngineHeader, "$Name\s*=\s*\{\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+)\s*\}")
    if (!$taskMatch.Success) { throw "Engine minimum missing: $Name" }
    return (($taskMatch.Groups | Select-Object -Skip 1 | ForEach-Object Value) -join '.')
}
[Reflection.Assembly]::LoadFrom($taskLauncher) | Out-Null
function New-FixtureSetup([string]$Version, [int]$Sequence, [switch]$Corrupt) {
    $taskFixture = [IO.Path]::GetFullPath((Join-Path $taskRoot $Version))
    $taskPayload = Join-Path $taskFixture 'Payload'
    $taskNames = @('Wildline.exe','Wildline/Binaries/Win64/Wildline.exe','Engine/Build/InstalledProjectBuild.txt','BetaReadme.txt')
    foreach ($taskName in $taskNames) {
        $taskPath = Join-Path $taskPayload $taskName
        New-Item -ItemType Directory -Path (Split-Path $taskPath) -Force | Out-Null
        [IO.File]::WriteAllText($taskPath, $Version)
    }
    $taskFiles = @($taskNames | ForEach-Object { [ordered]@{ path = $_; sha256 = (Get-FileHash (Join-Path $taskPayload $_) -Algorithm SHA256).Hash } })
    [ordered]@{ version = $Version; sequence = $Sequence; files = $taskFiles } | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $taskPayload 'BetaBuild.json') -Encoding UTF8
    if ($Corrupt) { [IO.File]::WriteAllText((Join-Path $taskPayload 'Wildline.exe'),'invalid payload') }
    Copy-Item (Join-Path $taskNative 'Installer/BetaReadme.txt') $taskFixture
    Copy-Item (Join-Path $taskNative 'Installer/Wildline.iss') $taskFixture
    $taskDefines = @{ BetaVersion=$Version; PayloadPath=$taskPayload; OutputPath=$taskFixture; LauncherPath=$taskLauncher; RedistPath='D:\EpicEngine\UE_5.8\Engine\Extras\Redist\en-us'; VCVersion=(Get-FixtureRuntime 'MinRedistVersion'); GameInputVersion=(Get-FixtureRuntime 'MinGameInputRedistVersion') }
    $taskDefines.GetEnumerator() | ForEach-Object { '#define ' + $_.Key + ' "' + $_.Value + '"' } | Set-Content (Join-Path $taskFixture 'Generated.iss') -Encoding UTF8
    & $taskCompiler (Join-Path $taskFixture 'Wildline.iss') *> (Join-Path $taskFixture 'compile.log')
    if ($LASTEXITCODE) { throw "Fixture compilation failed: $taskFixture" }
    return (Join-Path $taskFixture "Wildline-$Version-Setup.exe")
}
function Install-Fixture([string]$Setup, [string]$Name) {
    $taskProcess = Start-Process -FilePath $Setup -ArgumentList ('/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /DIR="' + $taskInstall + '" /LOG="' + $taskRoot + '\' + $Name + '.log"') -WindowStyle Hidden -PassThru -Wait
    return $taskProcess.ExitCode
}
$taskA = '0.0.0-beta.20000101.000001'; $taskB = '0.0.0-beta.20000101.000002'; $taskC = '0.0.0-beta.20000101.000003'
try {
    $taskSetupA = New-FixtureSetup $taskA 1
    if ((Install-Fixture $taskSetupA 'first') -ne 0) { throw 'First fixture install failed.' }
    if ([Wildline.Updates.Core]::ActiveVersion($taskInstall) -ne $taskA) { throw 'First fixture did not activate.' }
    $taskSetupB = New-FixtureSetup $taskB 2
    if ((Install-Fixture $taskSetupB 'upgrade') -ne 0) { throw 'Second fixture install failed.' }
    if ([Wildline.Updates.Core]::ActiveVersion($taskInstall) -ne $taskB) { throw 'Second fixture did not activate.' }
    [Wildline.Updates.Core]::VerifyPayload([Wildline.Updates.Core]::VersionRoot($taskInstall,$taskA),$taskA)
    Write-Output 'PASS real setup activates new version and retains verified previous payload.'
    $taskSetupC = New-FixtureSetup $taskC 3 -Corrupt
    $taskBadExit = Install-Fixture $taskSetupC 'corrupt'
    if ($taskBadExit -eq 0 -or [Wildline.Updates.Core]::ActiveVersion($taskInstall) -ne $taskB) { throw 'Corrupt setup did not fail while retaining the current version.' }
    Write-Output 'PASS real setup rejects a corrupt payload without changing the selected version.'
    [Wildline.Updates.Core]::Rollback($taskInstall)
    if ([Wildline.Updates.Core]::ActiveVersion($taskInstall) -ne $taskA) { throw 'Rollback did not restore the first version.' }
    Write-Output 'PASS rollback selects and verifies the retained previous installation.'
} finally {
    $taskUninstall = Join-Path $taskInstall 'unins000.exe'
    if (Test-Path $taskUninstall) {
        $taskProcess = Start-Process -FilePath $taskUninstall -ArgumentList '/VERYSILENT /SUPPRESSMSGBOXES /NORESTART' -WindowStyle Hidden -PassThru -Wait
        if ($taskProcess.ExitCode) { throw 'Fixture uninstall failed.' }
    }
}
if (Test-Path $taskRegistry) { throw 'Fixture uninstall left a registered installation.' }
Write-Output "PASS versioned setup integration. Logs: $taskRoot"
