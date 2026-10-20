# WILDLINE updates

Public downloads: https://github.com/mviterna/wildline-downloads

Permanent installer link: https://github.com/mviterna/wildline-downloads/releases/download/beta/WildlineSetup.exe

The public repository contains download instructions and compiled release assets only. The game and launcher sources remain in the private source repository.

## Local release

1. Build normally: `powershell -NoProfile -ExecutionPolicy Bypass -File native/LaunchNative.ps1 -Build`.
2. Verify the resulting full installer: `powershell -NoProfile -ExecutionPolicy Bypass -File native/VerifyBetaInstaller.ps1 -SetupPath native/Build/Beta/Wildline-<version>-Setup.exe`. Use a clean Windows account without an existing beta installation. This checks install, repair, saves, shortcuts, uninstall, and installed gameplay probes. A blocked installer is not a passing result.
3. Publish: `powershell -NoProfile -ExecutionPolicy Bypass -File native/PublishBeta.ps1`.

`PublishBeta.ps1 -Build` runs the local build, installer lifecycle verification, updater checks, and publication in one command. Use a clean Windows account without an existing beta installation. The publishing account needs authenticated `gh` access and the signing key. Normal builds never upload automatically.

Every normal build creates the full offline installer and the small online bootstrap in `native/Build/Beta`. `LatestInstaller.json` identifies the completed output. Publishing requires its exact SHA-256 to match the successful installer verification receipt, runs updater checks, splits the full installer into 1 GiB parts, and signs the manifest. Only explicit release assets are uploaded. All version files are uploaded to a draft release and checked before publishing it. The beta channel's signed `latest.json` advances last. A failed upload leaves the old feed selected; inspect a leftover draft before retrying. Published version assets must never be replaced.

For an interrupted unpublished draft, use `PublishBeta.ps1 -ResumeDraft`. It reuses only assets whose GitHub SHA-256 and size match, replaces incomplete uploads in that draft, and refuses to replace any published version or mismatched completed asset.

## Isolated Windows release verification

Preserve the publisher's existing beta installation and desktop shortcuts. `PublishBeta.ps1 -StageCandidate` uploads and verifies a version release without advancing the beta feed. The `Verify Windows release` Actions workflow can then validate that exact candidate on a disposable GitHub-hosted Windows VM. It downloads the signed public manifest and every part, verifies reconstruction, and runs the full installer lifecycle/game probes plus the small activation/corruption/rollback fixtures.

The workflow takes the candidate version and a published private fixture release tag (a draft is invisible to its read-only token). That private release contains `validation-tools.zip` with `InnoSetup/` (the compiler distribution), `Redist/` (the same VC++ and GameInput installers bundled in the candidate), `RuntimeMetadata.json` (the candidate's `visualCppMinimum` and `gameInputMinimum`), and `ExpectedInstaller.json` (its `LatestInstaller.json`). It contains no signing key. The VM uses only the committed public verification key.

After confirming the validation step succeeded for the intended source commit, decode its `WILDLINE_VERIFICATION_RESULT` Base64 line from the job logs and save the candidate receipt to local `native/Build/Beta/InstallerVerification.json`. Optional `upload_artifacts` also retains full logs when account storage is available. `PublishBeta.ps1 -ResumeCandidate` checks this exact-installer receipt and all immutable remote assets before advancing the beta feed. A failed validation leaves the previous update selected. VM lifecycle probes use NullRHI; graphical gameplay evidence comes from the separately tested package. After publication, the workflow's `live` phase exercises the real public bootstrap, launcher download/install, headless game smoke, current/offline launcher commands, and uninstall. It does not verify graphical windows; run `VerifyPublishedBeta.ps1` without `-Headless` on a clean Windows machine with a supported GPU for those checks.

## Trust and recovery

`InitializeUpdates.ps1` was run once. The private RSA signing key is stored outside Git at `%LOCALAPPDATA%/WildlinePublisher/update-key.dpapi`, encrypted with Windows DPAPI for the publishing account. Back up that file with a recoverable Windows profile; it is not portable to a different Windows account by itself. Do not delete it or rerun initialization. The committed XML file is the public verification key embedded in launchers. Losing the private key requires an explicit trust migration and new installers.

The launcher fetches the beta feed without GitHub credentials. It verifies the RSA/SHA-256 signature before accepting metadata, then verifies every part and the assembled installer. Version assets use a version-specific release tag. Installed build sequence prevents an older feed from automatically downgrading an installation. Completed parts are reused; an interrupted part restarts. The update helper rechecks the signed receipt and full installer after the launcher exits, then invokes the same prerequisite-checking Inno Setup installer.

Installed payloads live in `versions/<version>`. Setup hashes the new payload before atomically replacing `current.version`, retaining `previous.version` for rollback. An interrupted update to a different version leaves the previous selection and game files intact. A manual reinstall of the same version repairs that version in place. Older versions are retained until uninstall; disk usage grows across releases. AppData saves and any legacy `Wildline/Saved` directory are preserved. Legacy flat installations remain available on disk during migration. The development `WILDLINE.lnk` continues to use `LaunchNative.ps1` and locally completed packages; the tester `WILDLINE Beta` shortcut uses the updater.

Use the Start menu offline shortcut or `WildlineLauncher.exe --offline` to play without an update check. `--rollback` verifies, selects, and launches the previous version; a later normal launch can update again. Publisher checks are in `VerifyUpdates.ps1`. Launcher logs are in `%LOCALAPPDATA%/Wildline/LauncherLogs`; update logs are in `%LOCALAPPDATA%/Wildline/Updates/<version>/install.log`. Integrity signatures do not provide Windows Authenticode publisher identity; these beta executables remain unsigned unless a code-signing certificate is configured.

`VerifyVersionedSetup.ps1` uses small real installers to check successful version changes, rejection of corrupt payloads with a failing process exit code, and rollback. `VerifyPublishedBeta.ps1` downloads the public bootstrap, lets the real launcher download and install the game, checks responding game processes for normal/current/offline launches, then uninstalls the test copy. It uses the regular game profile for these startup checks and does not establish gameplay or clean-machine prerequisite behavior.
