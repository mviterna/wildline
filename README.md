# WILDLINE â€” Unreal Engine

Native Windows co-op hunting prototype built in Unreal Engine 5.8.3. Play **Classic** in autumn Blackwood or snowy Frostwood, **Farm Raid** at the homestead, or **King of the Hill** in the neighborhood. Choose a hunter/farmer or squirrel. The browser version is preserved in `BROWSER-PROTOTYPE.md` and `Launch Browser Prototype.cmd`.

## Launch

Double-click **Launch Game.cmd** or the **WILDLINE** desktop shortcut. Both launch the newest successfully packaged Windows build under `native/Build`, including neighborhood review builds. Completion records validate the packaged files so incomplete or failed rebuilds are skipped. If no completed package is available, the launcher falls back to engine play. **Open Unreal Editor.cmd** opens the editable project at `native/Unreal/Wildline/Wildline.uproject`.

**Build Windows Game.cmd** and **Build Beta Installer.cmd** publish the game after successful packaging, before building the installer, and automatically create or repair the **WILDLINE** desktop shortcut. For a separate review build, use `powershell -NoProfile -ExecutionPolicy Bypass -File native/LaunchNative.ps1 -Build -ArchiveRoot native/Build/YourReview -SkipInstaller`; it publishes to the same desktop launcher automatically. For a custom archive made directly with Unreal's packaging tools, run `powershell -NoProfile -ExecutionPolicy Bypass -File native/RegisterPackagedBuild.ps1 -PackageRoot native/Build/YourReview/Windows` after packaging succeeds. The launcher selects by completion time, including content-only builds whose EXE timestamp has not changed.

For Fab asset downloads, select **Wildline** in the Epic launcher's **Add to Project** dialog. Its project search folder is `C:/_git/wildline/native/Unreal`, registered under `[Launcher]` as `CreatedProjectPaths` in `%LOCALAPPDATA%/EpicGamesLauncher/Saved/Config/WindowsEditor/GameUserSettings.ini`. If an older asset pack hides the UE 5.8 project, check **Show all projects**, select **Wildline**, then choose the newest asset version available in **Select Version**. This selects the asset package version; it does not change WILDLINE's engine version. Asset compatibility still needs checking after import.

The game opens with a 6.6-second 2D squirrel short: an eight-frame running cycle
toward an acorn, interrupted by a comic shot and backward tumble before he reaches
it. The nut stays on the ground throughout; there is no pickup animation.
A one-second pause precedes the **hot_turd**
studio production ident. The logo combines turdmonkey's tabby and HoTsHoT's black
rubber duck avatars with a flaming poo emoji, an animated reveal, drifting embers
and a fade into the menu. The logo stays on screen for 3.07 seconds, 60% longer
than the earlier version. A lighthearted
woodland bluegrass instrumental fades in and loops over the menu. **Space**,
**Esc** or the **Skip** button goes straight to camp. The intro runs once per
application session, including across map changes. There is no replay menu item.

The landing menu uses the supplied **Squirrel Hunters** poster, with the complete
portrait fitted beside the expedition controls. **Match Rules / Bots / Settings**
opens the existing workshop. Music has its own saved volume slider under **Your
Settings**, eases down by 40% from the studio music level when the menu appears,
and fades out when entering gameplay. The editable 2D animation is
in `WildlineIntro.cpp`; presentation asset sources and
reimport instructions are in `ArtSource/Presentation/PROVENANCE.md`. Use
`native/VerifyPresentationPackage.ps1 -Capture` to verify the packaged sequence,
duration/skip, cleanup and gameplay entry, and capture the studio logo and menu.

## Send a Windows beta to a friend

Double-click **Build Windows Game.cmd** or **Build Beta Installer.cmd**. Both
rebuild/cook the current native game and produce a single
`Wildline-<version>-Setup.exe` in `native/Build/Beta`,
alongside a SHA-256 checksum, `BetaReadme.txt`, and the small online installer
`WildlineSetup.exe`. The full setup EXE embeds the runtimes and testing guide.
This is a Development beta with diagnostic logs.

Every Windows packaged build includes this installer; installer failure fails
the build. Installed shortcuts open the updater launcher, which checks the
signed beta feed at [wildline-downloads](https://github.com/mviterna/wildline-downloads).
Updates download in verified parts and use the same prerequisite-checking setup.
Different game versions have separate folders; setup verifies the new payload
before switching the active version and retaining the previous one. Re-running
the same full setup repairs its packaged trees. Saves and portraits are preserved.
The Start menu includes an offline shortcut. The development **WILDLINE** shortcut
continues to select local packages through `LaunchNative.ps1`.

Run **Publish Windows Beta.cmd** to build locally, verify the installers, and
publish a beta to the public download repository. Use an account without a
registered beta install for these lifecycle checks. Normal build commands never
upload. The permanent friend download is
[WildlineSetup.exe](https://github.com/mviterna/wildline-downloads/releases/download/beta/WildlineSetup.exe).
See [the update workflow](native/Launcher/README.md) for signing-key recovery,
release checks, rollback, and online installation verification.

Setup checks 64-bit Intel/AMD Windows, Windows updates, DirectX 12 availability
and free disk space. It compares installed Visual C++ x64 and GameInput DLL
versions against Unreal's minimums, installs bundled Microsoft-signed runtimes
when needed, stops on prerequisite failure and handles required restarts.
It installs for the current Windows account with Start menu shortcuts, an optional
desktop shortcut and a Windows uninstaller. It requests administrator permission
only to install missing runtimes. Your friend does not need Unreal, Epic Launcher
or developer tools.
GPU Shader Model 6 support is checked by the game at launch; use current drivers.

The build machine needs Unreal 5.8, its C++ toolchain and
[Inno Setup 6.3+](https://jrsoftware.org/isdl.php). For a package you just built,
run `powershell -NoProfile -ExecutionPolicy Bypass -File native/BuildBeta.ps1
-SkipGameBuild`. Optional `-EngineRoot` and `-CompilerPath` select local tools.
The staging pass excludes PDBs, caches, saved settings/portraits and packaging
manifests. `BetaBuild.json` records the version, source commit, working-tree
status, minimum runtime versions and payload hashes. Local setup/staging outputs
are ignored by Git.

Beta setup is unsigned, so Windows may show an unknown-publisher/SmartScreen
prompt. The bundled Microsoft runtime installers retain their Microsoft signatures.
Prerequisite logs are retained under
`%LOCALAPPDATA%\Wildline\InstallerLogs`. Game logs/crashes/settings live
under `%LOCALAPPDATA%\Wildline\Saved`; uninstall preserves personal data.
For internet co-op, the host still needs UDP 7777 forwarding or a shared VPN.

`native/VerifyBetaInstaller.ps1 -SetupPath <setup.exe>` verifies installation into
a path with spaces, all payload hashes, exclusion of saved/debug files, shortcuts,
the installed bootstrap and all three modes/intro, automatic upgrade discovery,
obsolete file cleanup, save preservation, same-version repair and uninstall.
Add `-PreviousSetupPath <older-setup.exe>` to test upgrading an older build.
Run it in a Windows account without an existing WILDLINE Beta installation.
Missing-runtime installation and reboot behavior also need clean-machine testing;
the local verification machine already has the required Microsoft runtimes.

## Controls

| Action | Hunter | Squirrel |
|---|---|---|
| Move / look | WASD / mouse | WASD / mouse |
| Sprint | Shift | Shift |
| Jump | Space | Space: long leap |
| Camera | V: first / third person | V: first / third person |
| Fire / scope | Left / right mouse | â€” |
| Reload | R | â€” |
| Thermal scope | T | â€” |
| Grab / release trunk | â€” | E near a tree |
| Climb / orbit trunk | â€” | W/S up/down; A/D around |
| Launch from trunk | â€” | Space in facing direction |
| Crouch | C | â€” |
| Camp / settings | Esc | Esc |
| Minimap: reserve / nearby view | M | M |
| Restart after a result | F5 | F5 |

Both roles start in **first person**. Mouse Y starts **inverted**. Uncheck **Invert mouse Y** at camp to change it. Inversion, separate normal/scope sensitivity sliders, sound volume, blood, mode, weapon, each mode's complete match rules and bot roster, and graphics settings persist between sessions. The co-op host controls the match rules and each bot's difficulty.

**Your Settings → Tree Leaves** controls canopy fullness from **0** (bare branches)
to **100** (all authored leaf cards). Each step progressively reveals whole leaf
cards, including needles, on reserve trees, neighborhood oaks and lodge scenery.
Changes apply immediately and save separately for autumn (default 72) and winter
(default 0). Trunks, branches, climbing collision and tree placement stay fixed.
Squirrels can leap while holding W and Shift; running leaps retain their momentum
and allow a brief grace period when uneven ground momentarily loses floor contact.

Graphics always use the high-quality configuration with 100% render resolution and dynamic resolution disabled. **Display resolution** is the only graphics option at camp; it applies immediately and persists between sessions. Existing lower quality presets are upgraded automatically.

## Create your hunter

Choose **Create / Edit Hunter** at camp. Start with **Custom hunter**, **Travis**, **Ethan**, **Greyson**, or **Mark**, then customize field clothing, cap, body slimness, skin tone and face width. The four named hunters use their supplied photographs on simple curved heads with no modeled facial features underneath. Hair, facial-hair and nose controls are disabled for those photo heads. Travis, Ethan and Greyson start with camouflage and blaze-orange torso panels; Mark starts in woodland olive at adult height. Face materials preserve detail in shade, the portrait uses soft face-level lighting, and the rifle pose keeps the head upright. Custom hunters also offer male/female bodies, hair color and nose shape. Full-body and face close-up views update live, with buttons to turn the preview. **Save Hunter** keeps the appearance for your expeditions; **Cancel** keeps the previous saved hunter. Previously saved Travis 2 profiles now load Travis.

**Choose PNG / JPEG** opens the native Windows image picker. Use a clear front-facing portrait, then align and crop it against the live face close-up. The image supplies photographic appearance on the selected 3D head; a single photo does not reconstruct an entire head. Portraits are resized locally and saved as a compact JPEG in the game's `Saved/Profiles/HunterFace.jpg`. Your appearance and face texture are shared with players in that co-op session, including late joiners. No external photo-processing service is used.

Ethan is approximately **71.4 cm** tall (30% shorter than his previous 102 cm), and Greyson is
approximately **110.4 cm** tall (20% shorter than his previous 138 cm), measured without a cap.
Their collision, eye level and equipment scale with the body; locomotion uses
shorter strides. Ethan's hands and feet are reduced by another 28% and 25%
relative to his body; Greyson's by 18% and 16%. Fingers and toes follow the
smaller proportions, including in the creator preview, with foot placement
adjusted for the smaller boots. Existing saved selections automatically update.

## Native co-op

Choose a role, then **Host Co-op Expedition**. Other players run the Windows game, choose either role, enter the host's IP address and select **Join Expedition**. Same-machine testing uses `127.0.0.1`; friends on the same network use the host's LAN address. Up to eight human players can join. Configure exact bot counts on both teams; bots are added to human players, and human joins or role changes retain the existing bots.

The native game uses Unreal's listen-server networking on **UDP port 7777**. Allow the game on your private network when Windows prompts. Internet play requires routing that port to the host or a suitable VPN. Matchmaking, accounts and automatic NAT traversal are not implemented.

## Game modes and weapons

At camp, select **Classic / Blackwood**, **Farm Raid / Peanut Heist**, or **King of the Hill / Bird Feeder** and a role, then **Enter Waiting Room** or **Host Co-op Expedition**. The co-op host chooses the map, match rules and bot roster. Joined players enter the waiting room for their chosen role.

Hunters gather in a log lodge with a recessed stone fireplace, exposed beams,
a woven rug, weathered picnic table, rocking chairs, bookcase, enamel cookware,
antique lanterns, scanned firewood and four mounted squirrel trophies on carved
oak shields: grey, fox, red and a black-coated eastern grey, with intact noses,
different coats, proportions and poses. Five open sash windows look into a
3D woodland clearing, with oak sills, gathered green curtains and brass rods.
A field noticeboard, wall clock, peg rail and specimen shelf add detail.
Walk to either weapon rack
and press **E** to equip the sporting rifle or pump-action shotgun. The selected
weapon saves locally and carries into the round; ammunition stays separate.
Weapon selection is no longer on the menu, and cannot change during a round.

Squirrels gather inside **The Acorn Hollow**, a continuous sculpted hollow tree
with scanned bark, roots, moss, forest leaf litter, deadwood, a woven acorn
basket, detailed acorn cupules and antique lanterns. Squirrels retain their unarmed forage,
climb and leap abilities. Both waiting rooms have a glowing **READY portal**.
Walk onto it when ready; step off to cancel. All connected humans must enter
a room and stand on their portal before the timer and configured bots start.
Late joiners choose equipment in their room, then enter the active round via
their portal. **Esc / Resume** returns to the current room or round. **F5** after
a result returns everyone to their waiting room for the next round.

Waiting-room players cannot fire, activate thermal, collect objectives or take
damage. The rooms use 22 downloaded CC0 Poly Haven models, six full 4K PBR
texture sets and original bevelled/sculpted architecture. Opaque detailed meshes
use Nanite; local lights add soft shadows and contact detail. Sources, author
metadata and file hashes are in `ArtSource/Lobby/Photoreal/sources.json`. Run
`native/VerifyLobbyPackage.ps1 -Capture` after building to check all three modes,
weapon interaction, portal readiness, late entry, restarts and host/client
replication, and capture both interiors.

**Classic** preserves the woodland hunt: hunters need eight kills, while squirrels win with twelve stored acorns or by surviving six minutes.

The camp **Match Workshop** has three tabs: **Match Rules**, **Bot Roster**, and **Your Settings**. Changes save automatically and separately for Classic, Farm Raid and King of the Hill. Entering with changed rules starts a fresh match; F5 after a result applies the saved rules to the next round. Co-op clients see the host's replicated rules and roster, with match controls disabled.

- **Bots:** exact defender and squirrel counts, **0–8 per team**, in addition to human players. Every defender has its own sporting rifle or pump-action shotgun and Easy/Medium/Hard AI. Every squirrel has its own difficulty and uses unarmed forage/climb abilities. Squirrel difficulty changes danger awareness and reaction delay; course bots pause according to difficulty between jumps. Inactive slots keep their settings when you reduce a count. Human joins/leaves keep active bot state.
- **AI behavior (Bot Roster tab):** sight, reaction speed, hunter accuracy, pursuit aggression, and squirrel danger awareness, each on a **0–100** scale (0 weakest, 50 balanced, 100 maximum), personal space (60–400 cm), escape tree time limit (2–12 seconds), and woodland escape climbing on/off. These controls save per mode and replicate from the host. At 100 accuracy, AI rifle shots are pinpoint and instant, while shotgun aim is exact with its normal pellet pattern; cover still blocks both. Maximum reaction speed reaches the weapon's legal firing cadence, maximum aggression pursues at 580 cm/s and searches for 20 seconds, and maximum sight reaches three times the bot difficulty's normal range. Neighborhood course climbs remain available because they are required by the course.
- Hunters patrol food areas and course approaches, pursue visible squirrels, search their last seen position, reload during quiet periods, and approach shared targets from different sides. Squirrels reserve separate food destinations, avoid occupied escape trees, and leave stalled climbs with a re-entry cooldown. Both teams follow walkable slopes, hop low obstructions where possible, and try another direction when movement stalls or circles without reaching the goal; unreachable food is temporarily avoided. Course squirrels queue for an occupied entry tree and retry the other authored route after a blocked entry or repeated failed landing.
- **Victory/session:** human player limit (1–8), round duration (1–30 minutes), squirrel objective and defender **kills** goals (1–30), and which side wins when time expires (mode default, defenders or squirrels).
- **Survival:** squirrel health (10–200), respawn delay (1–30 seconds), spawn damage protection (0–10 seconds), food carrying capacity (1–12), movement/climb speed (50–150%) and unlimited stamina. Course launch strength stays fixed so the authored gaps remain reachable.
- **Weapons/hazards:** reload duration (25–200%), infinite ammunition without magazine consumption, rifle thermal permission, thermal duration (5–60 seconds) and cooldown (0–300 seconds), and livestock damage in Farm Raid.
- **Minimap:** an **Allow minimap** checkbox, enforced for all players, plus Full/Nearby/Limited intel independent of bot AI. M only changes zoom when the map is allowed.
- **Schemes:** Standard, Casual, Hardcore and Playground populate a complete editable ruleset and roster for the selected mode. Choosing a scheme replaces that mode's current configuration. Custom edits remain saved for that mode.

Counts of zero allow co-op without AI or solo practice. A match needs opponents for kills and enough food for the objective; carrying capacity and livestock controls are disabled in modes where they do not apply. The original health, ammo, thermal and match timings described below are the Standard scheme's values.

**King of the Hill** counts repeated completed squirrel trips to a cedar hopper bird feeder. Choose **Maple Crescent** or **Cedar Backyards** under Match Rules / Bird Feeder Neighborhood, then enter a fresh waiting room to load that layout. Each has six houses mixing Colonial/Cape Cod architecture and the licensed suburban kit, porches, fenced yards, street trees, sidewalks, utility poles and garden furniture. Two organic approaches replace the mirrored platform ladders: climb the utility pole with **E**, launch onto the telephone line, cross an oak limb and cedar fence, or cross a raised planter, potting bench and garden fence. Both end with a real sprung trampoline: press **Space** on its fabric bed for the higher launch to the feeder. Land on every checkpoint in order; falling to the lawn resets progress. The smooth feeder pole and baffle cannot be climbed. A feeder visit scores once; return to a yard entrance before another trip. The HUD and minimap follow the selected configuration, and the host controls the shared match rules.

The neighborhood integrates 11 downloaded CC0 models, six 4K PBR surface sets and burnedhrum's licensed CC-BY suburban house kit with original feeder, trampoline and fence geometry. Both layouts also use selected Epic City Sample cars, pickup and van, plus SilverTm City Park benches, bins, lamps and a drinking fountain. Selected meshes live under `Content/Wildline/Fab`, with their source materials and texture dependencies retained. Editable placement and route data live in `ArtSource/Neighborhood/Configs`; their cooked Unreal DataAssets are assigned to each map. [Neighborhood sources and acquisition blockers](native/Unreal/Wildline/ArtSource/Neighborhood/PROVENANCE.md) record licenses and rebuild commands. Run `native/VerifyNeighborhood.ps1 -Editor -Capture` to check all four physical routes and capture both layouts. The desktop icon and `Launch Game.cmd` select the newest successful package.

Sound effects cover rifle/shotgun reports, bolt/pump action, reloads, boots and paws, jumping/landing, trunk scratching, solid impacts, damage, pickups, deposits, thermal activation, course checkpoints, feeder trips and match results, with occasional ambient birds. Nearby actions use spatial attenuation; objective feedback for the local player is clear. The camp **Sound effects volume** slider includes mute. The synthesized sounds are original project audio and require no external downloads.

**Farm Raid** pits farmers against squirrels at a walkable farmhouse. Squirrels sneak through the open doors or low wall vents, walk over floor-level peanuts in the pantry, carry up to four, and deliver twelve to the den. Peanuts are shared supplies: each one can be stolen once, and carried peanuts lost on death, role changes, or departure return to the pantry. Deposited peanuts remain removed. Farmers win with eight squirrel deaths, including livestock hazards, or by defending until the six-minute timer expires. Restarting restores the supplies.

Corn rows conceal slow squirrels from defender AI beyond nine metres; sprinting gives away movement. The farm has a covered porch, pantry shelving, kitchen furniture, a barn, silo, crop fields, fences, troughs, hay bales, an orchard and forest approaches. Four textured cows and two horses graze around the yard. Nearby squirrels trigger an audible and visible **0.9-second warning** before a kick. Move away to dodge: cows deal 18 damage and horses 22 against a squirrel's 40 health, with a 2.5-second recovery between attacks. Livestock cannot kick through solid walls or reach squirrels high in trees.

The **pump-action shotgun** uses the requested textured Sketchfab model by SuperMopsek. It holds two shells for gameplay balance, fires twelve pellets with a four-degree spread, and has damage falloff over a maximum 45 metres. Right mouse raises it and tightens the spread; **R** performs a 2.8-second reload. Its wooden fore-end pumps after firing and during reload, the support hand follows the fore-end, and spent shells eject beside the receiver. The rifle retains its five-round magazine, ballistic projectile and optical scope. Only the rifle has thermal sight. **T** activates it for **20 seconds**, followed by a **120-second cooldown after it ends**. Changing to the shotgun immediately ends thermal and retains the rifle cooldown.

Both weapons now produce a brief muzzle flash, a local light burst and drifting smoke. Solid hits produce dust and chips on wood/stone/ground, sparks on metal, and individual impact marks that fade after 25 seconds. Shotgun dust clouds are grouped to keep a twelve-pellet shot readable. The co-op server sends accepted shot and impact events to each player; the cosmetic effects have no gameplay collision, and the world retains at most 96 impact marks.

The Farm Raid minimap outlines the house, barn and crop rows and marks livestock; startled animals flash red. **Full intel reveals every remaining peanut, the den and every farmer**, alongside teammates. Nearby and Limited intel retain the Classic visibility rules. **M** switches between the 140-metre farm overview and a nearby 70-metre view. Squirrels have a health bar and collection/deposit feedback.

[Farm source credits](native/Unreal/Wildline/ArtSource/Farm/PROVENANCE.md) record the downloaded livestock, photographic textures, and attributed Sketchfab farmhouse and shotgun. The farmhouse preserves Raphael Escamilla's siding, roof, porch and textures, with an added playable ground floor, pantry furniture, front/rear openings and squirrel ramps. `Scripts/convert_sketchfab.py` rebuilds the adapted FBX files from the retained downloads; `Scripts/import_sketchfab.py` imports their PBR materials and faithful static triangle collision.

## Hunting and objectives

Hunter weapons follow the animated shoulder pocket, with both wrists solved
against the same rifle/shotgun pose. Vertical aim turns the chest, neck and head;
movement uses 63 retargeted Epic Game Animation Sample captures for directional
walk/run/crouch, starts/stops, standing turns and jump/landing transitions.
Directional blends share foot-contact phase, gait follows actual travel speed,
terrain IK plants stance feet through heel contact, and boot sounds follow gait.
Free animation candidates
and download settings are recorded in
`native/Unreal/Wildline/ArtSource/Hunters/UPGRADE_CANDIDATES.md`.

- Six-minute rounds. Hunters share a goal of eight squirrel kills. Squirrels win by depositing twelve acorns at their den, or surviving until time expires.
- The sporting rifle has a walnut-style stock, scoped steel receiver, trigger guard and a separate bolt that lifts, pulls back, returns and locks after firing. It fires lethal ballistic rounds with gravity, collision and a minimum 0.85-second firing interval. Tree trunks, log walls and terrain stop bullets. Five-round magazine; reload takes 2.2 seconds. Optical and thermal scopes have an outlined crosshair, mil marks and red center point.
- Hits kill squirrels and create blood droplets and ground effects. Blood can be disabled in settings. Dead squirrel players respawn after seven seconds and lose carried acorns.
- Each hunter's thermal scope runs for **20 seconds** and recharges for **120 seconds after expiry** (140 seconds between activations). Zoom remains independent while thermal is active: aim or release aim to enter or leave the scope without cancelling thermal, including third-person views. Living squirrels, including their fur, appear hot white against a darker grayscale scene in all five maps; solid geometry still blocks the view. `native/VerifyThermal.ps1` checks the packaged camera effect and captures normal, hot, covered, dead and expired views on every map.
- Squirrels can grab and climb trunks, orbit around them and launch between trees. Movement and long jumps use Unreal character physics and stamina.
- Walk over acorns to collect up to four, then return to the den. Thirty larger, capped acorns sit in clear forage patches; twelve are within thirty metres of camp. Nearby acorns have small gold hints when visible. Pickups and deposits show brief feedback, and a full bag prompts you to return home. Collected bodies and caps disappear locally. Acorns and deposits require a ground-level approach.
- **Hunter bot difficulty:** Easy has slower pursuit, a 2.5-second reaction and less accurate shots every 6.5 seconds. Medium balances pursuit, a 1.4-second reaction and shots every 3.8 seconds. Hard has faster pursuit, a 0.65-second reaction and precise shots every 1.65 seconds. All bots require line of sight, search a last known position briefly and steer around solid scenery.
- The north-up minimap shows terrain relief, trails, trees, your heading and teammates. Press **M** to switch between the whole reserve and a nearby 70-metre view. In the reserve view, **Full intel shows every remaining acorn, the den and all hunters**. Nearby intel shows acorns within 45 metres and visible hunters within 25 metres. Limited intel shows acorns within 22 metres and hides hunters. The den remains marked at every difficulty; zoomed maps keep the den and revealed hunters at the edge when off-screen.
- The server owns damage, cooldowns, climbing, pickups and shared scores. Standard Unreal character movement supplies client prediction and server correction.

## Visual scope

Rocks have cooked convex collision that blocks characters, bullets and cameras. The native reserve has a collidable split-rail perimeter backed by wooded slopes and surrounding ridges. A boundary notice appears near the edge, and outward velocity is removed at the reserve limit. Root collars are buried on slopes, including the pine asset's long below-ground root geometry. The native reserve uses licensed BlendSwap oak and pine geometry, curved grass clumps, clover, weeds and four photographic fern shapes. Small fallen leaves and weathered hollow logs sit on a scanned leaf-litter floor. Vegetation grows in irregular patches around clear paths, with saplings beneath varied crowns, subtle wind, two-sided leaf shading and four mesh LODs. A physical sky, woodland haze, Lumen lighting and virtual shadow maps light the reserve. All added forest geometry and shipped textures use CC0 sources; [forest source credits](native/Unreal/Wildline/ArtSource/Forest/PROVENANCE.md) record each asset and conversion. The hunters use detailed CC0 MakeHuman anatomy, textured padded jackets, boots and configurable caps on the engine's locomotion skeleton. The sporting rifle is original project geometry. Hollow logs have twelve convex collision wall segments, retaining open tunnels. [Hunter source credits](native/Unreal/Wildline/ArtSource/Hunters/PROVENANCE.md) document the new assets and portrait workflow. The squirrel uses WildMesh3D's textured skeletal model, sized to **48.9 cm nose to tail**, with tapered silhouette fur, matte coat shading and reflective eyes.

Third-person climbing keeps the camera outside the bark. Every gameplay oak and pine, including neighborhood yard trees, has collision on its actual wooden limbs. Press **E** near a trunk or branch to grip it; **W/S** climb vertical bark or move along a flatter limb in the look direction, and **A/D** move sideways. **Space** launches from either surface; **E** releases the grip. At a fork, look towards the limb and keep moving forward for a smooth assisted handoff onto its actual bark. Occupied branches shake loose small clusters of leaves that flutter down for hunters to spot; bare Frostwood trees shed snow instead. These disturbances replicate to nearby players. Foliage has no collision. The squirrel minimap shows the selected launch heading. The squirrel has travel-driven walking/bounding gaits, four-paw IK, slope alignment, breathing, head/ear movement, blinking and a balancing segmented tail. Jump poses tuck the limbs and extend on descent. Its 24 cm collision capsule and roughly 13 cm eye height fit the animal rather than the old enlarged placeholder. Ground jumps travel roughly 3.7 m on level ground and tree launches roughly 6.9 m; elevation and obstacles change that distance. Corpses fall from trees. First person, third person, configurable inversion and co-op remain available.

Choose **Frostwood / leafless winter** under Match Rules / Classic Reserve before entering a fresh waiting room. It preserves Classic's acorn collection, den deposits, perimeter and victory rules, with bare trees, frost on upward bark surfaces, granular snow, cold sunlight and winter haze. Blackwood uses approximately 28% fewer canopy leaf cards and fewer scattered ground leaves. The hunter lodge looks onto a deep wooded clearing and rising forest ridge. Acorn Hollow has mounted hunter busts, keepsakes, a lookout perch and a small raid-planning table. `native/VerifyWorldUpgrade.ps1 -Capture` checks limbs and trunks across all five maps and captures these additions from the latest package.

This is a playable native prototype moving toward realism. Cinematic fur, captured/authored animal animation, finished landscape composition and recorded environmental audio still need production work. Photo faces use adjustable texture projection; higher-end head reconstruction, authored reload/bolt-hand interaction and more refined character art remain production work. Hunter locomotion now uses captured movement and transitions, with both hands solved to the shouldered stock and closed finger poses. Aiming raises the stock and replicates to co-op players; firing adds recoil and the shotgun support hand follows its pump.

## Development

### Clone from GitHub

Install [Git LFS](https://git-lfs.com/) before cloning. Unreal content, editable
art sources, textures, previews and audio are stored in LFS:

```powershell
git lfs install
git clone https://github.com/mviterna/wildline.git
cd wildline
git lfs pull
```

Use Unreal Engine 5.8.3 and the Windows C++ toolchain to build the native project.
The engine installation, packaged game, build caches, local test output and
authoring downloads are excluded from the repository. The launch/build scripts
use the local engine installation; adjust its path for another computer.

This repository contains third-party assets with different licenses and a
personal hunter likeness. Keep access private. The asset credits below and
the `ArtSource` provenance files identify the applicable licenses; inclusion in
this repository does not grant unrestricted redistribution of those assets.

- **Build Windows Game.cmd** compiles, cooks and packages the game using the installed Unreal 5.8 engine and C++ toolchain, then creates the installer in `native/Build/Beta`.
- **Test Unreal Game.cmd** runs the native automation suites followed by Classic, Farm Raid and King of the Hill live-world checks covering lethal projectiles, climbing, long launches, reload, thermal timing, squirrel anatomy/scale, fur, paw articulation, body motion, forest assets, log/tree collision, hunter appearance, portrait decoding, visible bolt cycling, planted roots, boundary collision and containment, scaled rock traces and capsule sweeps, difficulty/map visibility, forage pickup/deposit validation, rifle grip across body profiles and scoped/sideways movement, directional animation assets, full-orbit climbing visibility on every tree species at low/middle/upper heights and camera recovery after launch.
- `native/Unreal/Wildline/Scripts/build_content.py` rebuilds editable materials and the Blackwood map using Unreal's Python commandlet. It expects the photographed source textures in `ArtSource`.
- `native/VerifyForagePackage.ps1 -Capture` verifies the packaged rock collision, pickups/deposits, late-join difficulty replication and captures the camp, Easy map, zoom and pickup feedback for review.
- `native/Unreal/Wildline/Scripts/polish_foraging.py` cooks stone collision and refreshes the saved map with clear forage patches and larger capped acorns.
- `native/Unreal/Wildline/Scripts/polish_reserve.py` refreshes the saved reserve and hair shading after C++ changes. `-WildlinePolishReview` captures hunter grip, actual third-person climbing/orbit and reserve boundary views.
- `native/VerifyHunterAnimations.ps1` checks captured hunter movement at 30, 60 and 120 simulation fps, including crouch ledge falls, shoulder/grip contact and stance-foot stability. Add `-Capture` to record the same sequence at high graphics quality. Source FBX retarget/export and import scripts are in `native/Unreal/Wildline/Scripts`; measured timing and stride metadata are retained with the hunter art sources.
- Diagnostic logs and native screenshots are under `native` and the project's `Saved` directory. Generated build folders are excluded from Git.
- `native/VerifyConfigPackage.ps1 -Capture` checks custom rules in all three modes, exact bot counts and individual loadouts, pickup capacity, health, respawn/protection, movement, ammo/reload/thermal permissions, real menu save/reload and co-op rules/roster replication using isolated test profiles. It captures all three workshop tabs and advanced rules. Use `-Editor` to verify the editor build.
- `native/VerifyAI.ps1 -Capture` checks all five maps with three patrolling hunters and eight squirrels: travel with no visible enemies, separate food goals, sustained food/course progress, sight blocked by cover, remembered search positions, escape tree deadlines, and real menu save/reload for all eight AI controls. It captures the AI controls and roster. Use `-Editor` before packaging.
- `native/VerifyGraphicsPackage.ps1` checks migration from saved low settings to fixed high quality, live resolution changes, persistence and restart using a separate test profile, and captures the graphics menu.

## TODO: integrate the new weapon assets

The assets supplied in `C:/Work/GameAssets` are now stored under
[`native/Unreal/Wildline/ArtSource/Weapons`](native/Unreal/Wildline/ArtSource/Weapons/README.md).
They include M64 Kryptek-style and blue/red Rascal rifles, Pardner .410 and
Stevens 940D shotguns, two scopes, textures, previews and editable Blender
sources. **Adding these weapons to the game remains TODO.**

- [ ] Import meshes and materials and add weapon/scope selection.
- [ ] Fit each stock to the hunter's shoulder and both hands in movement and aim.
- [ ] Add weapon-specific firing, action and reload animations, including the
  supplied shotguns' break-action handling.
- [ ] Verify sights, ammo, weapon switching and solo/co-op replication.

The [weapon asset inventory and integration checklist](native/Unreal/Wildline/ArtSource/Weapons/README.md)
records all seven exports, source conventions and remaining work.

## Asset credits

- Poly Haven **Bark Brown 01**, **Forest Ground 01**, **Pine Tree 01**, **Forest Leaves 02** and **Grass Medium 01** photographs and normal/alpha maps, CC0. BlendSwap trees, ground-cover geometry and ferns, plus ambientCG leaf photographs, are recorded in the forest provenance file above.
- Epic Games Unreal Engine template mannequin skeleton and animations, copied from the installed engine templates. These remain subject to Epic's applicable engine/content license.
- Epic's free Game Animation Sample supplies the hunter's captured locomotion and transitions. Selected clips are retargeted through the sample's UEFN-to-UE5 rig onto the existing hunter skeleton; [hunter source credits](native/Unreal/Wildline/ArtSource/Hunters/PROVENANCE.md) retain source and licensing details.
- WildMesh3D **Rigged Squirrel**, supplied through the publisher's [free-model page](https://www.wildmesh3d.com/free-models). Source, dimensions and integration notes are in `native/Unreal/Wildline/ArtSource/Squirrel/PROVENANCE.md`.
- MakeHuman CC0 anatomy, skins, hair, eyes, padded clothing and boots; generated with MPFB. Hunter provenance documents the source licenses.
- Original gameplay code, sporting rifle/cap geometry, generated forest geometry, squirrel fur geometry/native animation, UI and procedural gameplay audio are included in this project.
- Browser-specific asset and font credits are preserved in `BROWSER-PROTOTYPE.md`.

Farm package verification: run `powershell -NoProfile -ExecutionPolicy Bypass -File native/VerifyFarmPackage.ps1 -Capture`. It checks Classic regression, both weapon choices, walkable doors and squirrel vents, shared peanut theft/delivery, livestock warnings and damage, autonomous squirrel deliveries, and late-joining client replication of Farm Raid, shotgun ammo, pantry stock, carried peanuts, livestock, difficulty and hunter appearance. Rendered packaged views are copied into `native/Preview/Farm*.png`.

Weapon effects verification: `native/VerifyWeaponFXPackage.ps1 -Capture` runs the gameplay and co-op checks (including multicast muzzle and twelve-pellet impact events) and captures the ready weapon, flash, smoke, impact marks and pump reload. `Scripts/build_weapon_fx.py` rebuilds the editable smoke, flash, spark, chip and decal materials.

King of the Hill verification: `native/VerifyModesPackage.ps1 -Capture` checks the packaged course, configured scoring, real physics traversal, rifle-only thermal, Classic/Farm regression, host/client match-rule replication, checkpoint/trip replication, multicast sounds and the human player limit. It captures the mode settings, squirrel view, paired course and neighborhood. `Scripts/build_neighborhood.py` rebuilds the separate editable map and lawn material using the existing licensed assets.

`native/VerifyAIBalance.ps1 -Capture` verifies the 0–100 controls and maximum strength, 35/35 exposed shots across rifle/shotgun ranges and difficulty tiers (including a continuously moving target), cover blocking, and saved settings. Use `-Editor` for the editor module.
