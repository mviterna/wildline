param([switch]$Build, [switch]$ResumeDraft, [string]$Notes = 'WILDLINE Windows beta. Install once; future updates are checked automatically on launch.')
$ErrorActionPreference = 'Stop'
if ($Build) {
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'LaunchNative.ps1') -Build
    if ($LASTEXITCODE) { throw 'Local build failed; nothing was published.' }
}
$taskReceipt = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'Build/Beta/LatestInstaller.json') -Raw | ConvertFrom-Json
if ($Build) {
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'VerifyBetaInstaller.ps1') -SetupPath $taskReceipt.setupPath -BootstrapPath $taskReceipt.bootstrapPath
    if ($LASTEXITCODE) { throw 'Installer lifecycle verification failed; nothing was published.' }
}
$taskVerificationPath = Join-Path $PSScriptRoot 'Build/Beta/InstallerVerification.json'
if (!(Test-Path $taskVerificationPath)) { throw 'Run VerifyBetaInstaller.ps1 on this installer before publishing.' }
$taskVerification = Get-Content $taskVerificationPath -Raw | ConvertFrom-Json
if ($taskVerification.sha256 -ne $taskReceipt.sha256 -or $taskVerification.bootstrapSha256 -ne $taskReceipt.bootstrapSha256 -or !$taskVerification.passed) { throw 'These exact installers have not passed lifecycle verification.' }
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'Launcher/VerifyUpdates.ps1')
if ($LASTEXITCODE) { throw 'Updater checks failed; nothing was published.' }
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'Launcher/VerifyVersionedSetup.ps1')
if ($LASTEXITCODE) { throw 'Versioned setup integration failed; nothing was published.' }
$taskOutput = & (Join-Path $PSScriptRoot 'Launcher/PrepareUpdate.ps1')
$taskRepo = 'mviterna/wildline-downloads'
$taskTag = "v$($taskReceipt.version)"
function Invoke-GitHub {
    $Arguments = $args
    & gh @Arguments
    if ($LASTEXITCODE) { throw "GitHub operation failed: $($Arguments[0]) $($Arguments[1]). Feed was not advanced." }
}
# Do not replace an existing version. A retry must inspect/reuse its draft explicitly.
$taskExistingTags = @(& gh api --paginate "repos/$taskRepo/releases?per_page=100" --jq '.[].tag_name')
if ($LASTEXITCODE) { throw 'Cannot inspect existing releases.' }
if (($taskExistingTags -contains $taskTag) -and !$ResumeDraft) { throw "Release $taskTag already exists. Use -ResumeDraft only for an interrupted unpublished draft." }
$taskNotesPath = Join-Path $taskOutput 'release-notes.txt'
[IO.File]::WriteAllText($taskNotesPath, $Notes)
if ($taskExistingTags -notcontains $taskTag) {
    Invoke-GitHub @('release','create',$taskTag,'--repo',$taskRepo,'--draft','--prerelease','--title',"WILDLINE $($taskReceipt.version)",'--notes-file',$taskNotesPath)
}
# Drafts may not resolve through the tag endpoint until they are published.
$taskReleases = & gh api "repos/$taskRepo/releases?per_page=100" | ConvertFrom-Json
if ($LASTEXITCODE) { throw 'Cannot inspect the release draft.' }
$taskDrafts = @($taskReleases | Where-Object { $_.tag_name -eq $taskTag -and $_.draft })
if ($taskDrafts.Count -ne 1) { throw 'The version must be an unpublished draft; published assets are never replaced.' }
$taskDraft = $taskDrafts[0]
$taskAssets = @(Get-ChildItem $taskOutput -File | Where-Object { $_.Name -match '\.part\d{3}$|^latest\.json$|^WildlineSetup\.exe$' })
foreach ($taskAsset in $taskAssets) {
    $taskExpectedDigest = 'sha256:' + (Get-FileHash $taskAsset.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    $taskExisting = @($taskDraft.assets | Where-Object { $_.name -eq $taskAsset.Name })
    if ($taskExisting.Count) {
        if ($taskExisting.Count -eq 1 -and $taskExisting[0].state -eq 'uploaded' -and $taskExisting[0].digest -eq $taskExpectedDigest -and $taskExisting[0].size -eq $taskAsset.Length) { continue }
        if ($taskExisting.Count -eq 1 -and $taskExisting[0].state -ne 'uploaded') {
            Invoke-GitHub @('api','--method','DELETE',"repos/$taskRepo/releases/assets/$($taskExisting[0].id)")
        } else { throw "Existing draft asset differs: $($taskAsset.Name). Inspect it before retrying." }
    }
    Invoke-GitHub @('release','upload',$taskTag,$taskAsset.FullName,'--repo',$taskRepo)
}
$taskRemote = & gh api "repos/$taskRepo/releases/$($taskDraft.id)" | ConvertFrom-Json
if ($LASTEXITCODE) { throw 'Cannot verify uploaded release.' }
foreach ($taskAsset in $taskAssets) {
    $taskExpectedDigest = 'sha256:' + (Get-FileHash $taskAsset.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    $taskMatch = @($taskRemote.assets | Where-Object { $_.name -eq $taskAsset.Name -and $_.size -eq $taskAsset.Length -and $_.digest -eq $taskExpectedDigest })
    if ($taskMatch.Count -ne 1) { throw "Upload verification failed: $($taskAsset.Name)" }
}
Invoke-GitHub @('release','edit',$taskTag,'--repo',$taskRepo,'--draft=false')
if ($taskExistingTags -notcontains 'beta') {
    Invoke-GitHub @('release','create','beta','--repo',$taskRepo,'--prerelease','--title','WILDLINE beta downloads','--notes','Install WildlineSetup.exe once. It downloads the current verified beta and checks for future updates on launch.')
} else {
    Invoke-GitHub @('release','edit','beta','--repo',$taskRepo,'--title','WILDLINE beta downloads','--notes','Install WildlineSetup.exe once. It downloads the current verified beta and checks for future updates on launch.')
}
# The manifest is the commit point: upload it only after all version assets exist.
Invoke-GitHub @('release','upload','beta',(Join-Path $taskOutput 'WildlineSetup.exe'),'--clobber','--repo',$taskRepo)
Invoke-GitHub @('release','upload','beta',(Join-Path $taskOutput 'latest.json'),'--clobber','--repo',$taskRepo)
$taskLivePath = Join-Path $taskOutput 'live-latest.json'
Invoke-WebRequest -UseBasicParsing 'https://github.com/mviterna/wildline-downloads/releases/download/beta/latest.json' -OutFile $taskLivePath
if ((Get-FileHash $taskLivePath -Algorithm SHA256).Hash -ne (Get-FileHash (Join-Path $taskOutput 'latest.json') -Algorithm SHA256).Hash) { throw 'Public feed differs from the published manifest.' }
Write-Output 'Published: https://github.com/mviterna/wildline-downloads/releases/download/beta/WildlineSetup.exe'
