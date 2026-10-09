param(
    [Parameter(Mandatory)][string]$PackageRoot,
    [DateTimeOffset]$CompletedAtUtc = [DateTimeOffset]::UtcNow
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'PackagedBuilds.ps1')
$PackageRoot = [IO.Path]::GetFullPath($PackageRoot).TrimEnd('\', '/')
$buildPrefix = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'Build')).TrimEnd('\', '/') + '\'
if (!$PackageRoot.StartsWith($buildPrefix, [StringComparison]::OrdinalIgnoreCase) -or (Split-Path $PackageRoot -Leaf) -ne 'Windows') {
    throw 'Register a Windows archive inside native/Build.'
}
if ($PackageRoot.Substring($buildPrefix.Length) -match '(^|[\\/])(InstallerStage|InstallerVerification|Beta)([\\/]|$)') {
    throw 'Installer staging/output directories cannot be registered as developer builds.'
}
Publish-WildlineBuild -PackageRoot $PackageRoot -CompletedAtUtc $CompletedAtUtc
