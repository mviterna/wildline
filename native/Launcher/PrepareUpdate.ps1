param(
    [string]$ReceiptPath = "$PSScriptRoot/../Build/Beta/LatestInstaller.json",
    [string]$KeyPath = "$env:LOCALAPPDATA/WildlinePublisher/update-key.dpapi",
    [ValidateRange(1,1024)][int]$PartSizeMB = 1024
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Security
$taskReceipt = Get-Content -LiteralPath $ReceiptPath -Raw | ConvertFrom-Json
if ($taskReceipt.version -notmatch '^\d+\.\d+\.\d+-beta\.\d{8}\.\d{6}$') { throw 'Invalid installer version.' }
if ((Get-FileHash -LiteralPath $taskReceipt.setupPath -Algorithm SHA256).Hash -ne $taskReceipt.sha256) { throw 'Installer changed since build completion.' }
if ((Get-Item -LiteralPath $taskReceipt.setupPath).Length -ne $taskReceipt.size) { throw 'Installer size changed.' }
if ($taskReceipt.bootstrapSha256 -and (Get-FileHash -LiteralPath $taskReceipt.bootstrapPath -Algorithm SHA256).Hash -ne $taskReceipt.bootstrapSha256) { throw 'Bootstrap installer changed since build completion.' }
$taskRsa = New-Object Security.Cryptography.RSACryptoServiceProvider
$taskRsa.PersistKeyInCsp = $false
try {
    $taskPrivate = [Security.Cryptography.ProtectedData]::Unprotect([IO.File]::ReadAllBytes($KeyPath), $null, 'CurrentUser')
    $taskRsa.FromXmlString([Text.Encoding]::UTF8.GetString($taskPrivate))
    if ($taskRsa.ToXmlString($false) -cne [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'UpdatePublicKey.xml'))) { throw 'Signing key does not match the launcher trust key.' }
    $taskOutput = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot "../Build/Updates/$($taskReceipt.version)"))
    New-Item -ItemType Directory -Path $taskOutput -Force | Out-Null
    $taskParts = @()
    $taskInput = [IO.File]::OpenRead($taskReceipt.setupPath)
    try {
        $taskBuffer = New-Object byte[] (1MB)
        $taskIndex = 0
        while ($taskInput.Position -lt $taskInput.Length) {
            $taskName = 'Wildline-{0}-Setup.exe.part{1:000}' -f $taskReceipt.version, $taskIndex
            $taskPath = Join-Path $taskOutput $taskName
            $taskStream = [IO.File]::Create($taskPath)
            try {
                $taskRemaining = [long]$PartSizeMB * 1MB
                while ($taskRemaining -gt 0 -and ($taskRead = $taskInput.Read($taskBuffer, 0, [int][Math]::Min($taskBuffer.Length, $taskRemaining))) -gt 0) {
                    $taskStream.Write($taskBuffer, 0, $taskRead); $taskRemaining -= $taskRead
                }
            } finally { $taskStream.Dispose() }
            $taskParts += [ordered]@{ name = $taskName; size = (Get-Item $taskPath).Length; sha256 = (Get-FileHash $taskPath -Algorithm SHA256).Hash.ToLowerInvariant() }
            $taskIndex++
        }
    } finally { $taskInput.Dispose() }
    $taskManifest = [ordered]@{
        schemaVersion = 1; channel = 'beta'; version = $taskReceipt.version; sequence = $taskReceipt.sequence
        tag = "v$($taskReceipt.version)"; installerSize = $taskReceipt.size
        installerSha256 = $taskReceipt.sha256.ToLowerInvariant(); parts = @($taskParts)
    }
    $taskBytes = [Text.Encoding]::UTF8.GetBytes(($taskManifest | ConvertTo-Json -Depth 6 -Compress))
    $taskSignature = $taskRsa.SignData($taskBytes, [Security.Cryptography.CryptoConfig]::MapNameToOID('SHA256'))
    $taskEnvelope = [ordered]@{ payload = [Convert]::ToBase64String($taskBytes); signature = [Convert]::ToBase64String($taskSignature) }
    $taskEnvelope | ConvertTo-Json -Compress | Set-Content -LiteralPath (Join-Path $taskOutput 'latest.json') -Encoding UTF8
    Copy-Item -LiteralPath $taskReceipt.bootstrapPath -Destination (Join-Path $taskOutput 'WildlineSetup.exe') -Force
} finally { $taskRsa.Dispose(); if ($taskPrivate) { [Array]::Clear($taskPrivate,0,$taskPrivate.Length) } }
Write-Output $taskOutput
