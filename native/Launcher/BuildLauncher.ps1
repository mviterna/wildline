param([string]$OutputRoot = "$PSScriptRoot/../Build/Launcher")
$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Path $OutputRoot -Force | Out-Null
$taskCompiler = Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$taskOutput = [IO.Path]::GetFullPath((Join-Path $OutputRoot 'WildlineLauncher.exe'))
if (!(Test-Path "$PSScriptRoot/UpdatePublicKey.xml")) { throw 'Run InitializeUpdates.ps1 once before building the launcher.' }
$taskKey = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'UpdatePublicKey.xml'))
$taskSource = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'WildlineLauncher.cs'))
& $taskCompiler /nologo /target:winexe /platform:x64 /optimize+ "/out:$taskOutput" /r:System.Windows.Forms.dll /r:System.Drawing.dll /r:System.Web.Extensions.dll "/resource:$taskKey,UpdatePublicKey.xml" $taskSource | Write-Host
if ($LASTEXITCODE) { throw 'Launcher compilation failed.' }
$taskOutput
