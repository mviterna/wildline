# Run once on the publishing Windows account. Never rotate the key implicitly.
param([string]$KeyPath = "$env:LOCALAPPDATA/WildlinePublisher/update-key.dpapi")
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Security
$taskPublicPath = Join-Path $PSScriptRoot 'UpdatePublicKey.xml'
if ((Test-Path $KeyPath) -or (Test-Path $taskPublicPath)) {
    throw 'An update key already exists. Preserve it: existing launchers trust this public key.'
}
$taskRsa = New-Object Security.Cryptography.RSACryptoServiceProvider(3072)
$taskRsa.PersistKeyInCsp = $false
try {
    New-Item -ItemType Directory -Path (Split-Path $KeyPath) -Force | Out-Null
    $taskProtected = [Security.Cryptography.ProtectedData]::Protect([Text.Encoding]::UTF8.GetBytes($taskRsa.ToXmlString($true)), $null, 'CurrentUser')
    [IO.File]::WriteAllBytes($KeyPath, $taskProtected)
    [IO.File]::WriteAllText($taskPublicPath, $taskRsa.ToXmlString($false))
} finally { $taskRsa.Dispose() }
Write-Output 'Update signing key created outside the repository, protected for this Windows account. Back it up securely with the Windows account recovery material.'
