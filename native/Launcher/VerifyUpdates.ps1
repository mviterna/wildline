$ErrorActionPreference = 'Stop'
$taskOutput = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../Build/UpdaterVerification'))
New-Item -ItemType Directory -Path $taskOutput -Force | Out-Null
$taskExe = Join-Path $taskOutput 'UpdateTests.exe'
$taskCompiler = Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$taskSource = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'WildlineLauncher.cs'))
$taskTests = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'UpdateTests.cs'))
& $taskCompiler /nologo /target:exe /platform:x64 /main:UpdateTests "/out:$taskExe" /r:System.Windows.Forms.dll /r:System.Drawing.dll /r:System.Web.Extensions.dll $taskSource $taskTests
if ($LASTEXITCODE) { throw 'Updater check compilation failed.' }
& $taskExe $taskOutput
if ($LASTEXITCODE) { throw 'Updater verification failed.' }
# Exercise the PowerShell publisher and the shipped trust key across >1 part.
$taskInput = Join-Path $taskOutput 'publisher-fixture.dat'
$taskBytes = New-Object byte[] (1536KB)
$taskRandom = [Security.Cryptography.RandomNumberGenerator]::Create()
try { $taskRandom.GetBytes($taskBytes) } finally { $taskRandom.Dispose() }
[IO.File]::WriteAllBytes($taskInput, $taskBytes)
$taskReceipt = Join-Path $taskOutput 'fixture-receipt.json'
[ordered]@{ version = '0.0.0-beta.20000101.000000'; sequence = 1; setupPath = $taskInput; size = $taskBytes.Length; sha256 = (Get-FileHash $taskInput -Algorithm SHA256).Hash; bootstrapPath = $taskExe } |
    ConvertTo-Json | Set-Content -LiteralPath $taskReceipt -Encoding UTF8
$taskPartsRoot = & (Join-Path $PSScriptRoot 'PrepareUpdate.ps1') -ReceiptPath $taskReceipt -PartSizeMB 1
$taskLauncher = & (Join-Path $PSScriptRoot 'BuildLauncher.ps1')
[Reflection.Assembly]::LoadFrom($taskLauncher) | Out-Null
$taskManifest = [Wildline.Updates.Core]::ReadManifest([IO.File]::ReadAllText((Join-Path $taskPartsRoot 'latest.json')), [Wildline.Updates.Core]::PublicKey)
if ($taskManifest.parts.Length -ne 2) { throw 'Publisher did not split the fixture.' }
[Wildline.Updates.Core]::Assemble($taskManifest, $taskPartsRoot, [Action[string,int]] { param($message,$percent) })
if ((Get-FileHash (Join-Path $taskPartsRoot 'Setup.exe') -Algorithm SHA256).Hash -ne (Get-FileHash $taskInput -Algorithm SHA256).Hash) { throw 'Publisher round trip failed.' }
Write-Output 'PASS publisher signing, shipped public key, multiple parts and byte-exact reconstruction.'
