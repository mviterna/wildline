param([Parameter(Mandatory=$true)][string]$Version, [Parameter(Mandatory=$true)][string]$ToolsRoot)
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
if ($Version -notmatch '^\d+\.\d+\.\d+-beta\.\d{8}\.\d{6}$') { throw 'Invalid candidate version.' }
if (!$env:GITHUB_ACTIONS -or $env:RUNNER_ENVIRONMENT -ne 'github-hosted') { throw 'Release lifecycle validation requires a disposable GitHub-hosted Windows VM.' }
$taskRoot = Join-Path $PSScriptRoot 'Build/ReleaseValidation'
New-Item -ItemType Directory $taskRoot -Force | Out-Null
New-Item -ItemType Directory (Join-Path $PSScriptRoot 'Build/Beta') -Force | Out-Null
$taskTools = (Resolve-Path -LiteralPath $ToolsRoot).Path
$taskLauncher = & (Join-Path $PSScriptRoot 'Launcher/BuildLauncher.ps1')
[Reflection.Assembly]::LoadFrom($taskLauncher) | Out-Null
$taskBase = "https://github.com/mviterna/wildline-downloads/releases/download/v$Version"
$taskManifestPath = Join-Path $taskRoot 'latest.json'
& curl.exe --fail --location --silent --show-error --retry 4 --output $taskManifestPath "$taskBase/latest.json"
if ($LASTEXITCODE) { throw 'Candidate metadata download failed.' }
$taskManifest = [Wildline.Updates.Core]::ReadManifest([IO.File]::ReadAllText($taskManifestPath), [Wildline.Updates.Core]::PublicKey)
if ($taskManifest.version -ne $Version) { throw 'Candidate manifest version mismatch.' }
$taskDownloads = foreach ($taskPart in $taskManifest.parts) {
    $taskArguments = '--fail --location --silent --show-error --retry 4 --output "' + (Join-Path $taskRoot $taskPart.name) + '" "' + "$taskBase/" + $taskPart.name + '"'
    Start-Process -FilePath curl.exe -ArgumentList $taskArguments -WindowStyle Hidden -PassThru -RedirectStandardError (Join-Path $taskRoot ($taskPart.name + '.download.log'))
}
foreach ($taskDownload in $taskDownloads) {
    if (!$taskDownload.WaitForExit(1200000)) { throw 'Candidate part download timed out.' }
    $taskDownload.Refresh()
    if ($taskDownload.ExitCode) { throw 'Candidate part download failed; inspect download logs.' }
}
[Wildline.Updates.Core]::Assemble($taskManifest, $taskRoot, [Action[string,int]] { param($message,$percent) })
Write-Output 'PASS signed public metadata, all part hashes, byte-exact public installer assembly.'
# Only disposable candidate downloads inside this VM are removed to recover disk space.
foreach ($taskPart in $taskManifest.parts) { Remove-Item -LiteralPath (Join-Path $taskRoot $taskPart.name) }
$taskBootstrap = Join-Path $taskRoot 'WildlineSetup.exe'
& curl.exe --fail --location --silent --show-error --retry 4 --output $taskBootstrap "$taskBase/WildlineSetup.exe"
if ($LASTEXITCODE) { throw 'Candidate bootstrap download failed.' }
$taskExpected = Get-Content (Join-Path $taskTools 'ExpectedInstaller.json') -Raw | ConvertFrom-Json
if ($taskExpected.version -ne $Version -or (Get-FileHash $taskBootstrap -Algorithm SHA256).Hash -ne $taskExpected.bootstrapSha256 -or $taskManifest.installerSha256 -ne $taskExpected.sha256) { throw 'Downloaded installers differ from the locally tested candidate.' }
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'VerifyBetaInstaller.ps1') -SetupPath (Join-Path $taskRoot 'Setup.exe') -BootstrapPath $taskBootstrap
if ($LASTEXITCODE) { throw 'Full installer lifecycle verification failed.' }
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'Launcher/VerifyVersionedSetup.ps1') -CompilerPath (Join-Path $taskTools 'InnoSetup/ISCC.exe') -RedistPath (Join-Path $taskTools 'Redist') -RuntimeMetadataPath (Join-Path $taskTools 'RuntimeMetadata.json')
if ($LASTEXITCODE) { throw 'Versioned setup activation/corruption/rollback verification failed.' }
$taskReceipt = Get-Content (Join-Path $PSScriptRoot 'Build/Beta/InstallerVerification.json') -Raw | ConvertFrom-Json
$taskReceipt | Add-Member -NotePropertyName versionedSetupPassed -NotePropertyValue $true
$taskReceipt | Add-Member -NotePropertyName runner -NotePropertyValue 'github-hosted-windows'
$taskReceipt | Add-Member -NotePropertyName runUrl -NotePropertyValue "https://github.com/$env:GITHUB_REPOSITORY/actions/runs/$env:GITHUB_RUN_ID"
$taskReceipt | ConvertTo-Json | Set-Content (Join-Path $taskRoot 'result.json') -Encoding UTF8
Write-Output 'PASS clean Windows release validation.'
