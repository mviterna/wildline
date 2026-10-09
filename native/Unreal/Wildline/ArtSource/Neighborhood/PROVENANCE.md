# Bird-feeder neighborhood art

Prepared 2026-10-08. Art direction: a lived-in American residential block inspired
by Whittleton Creek's quiet streets, mature trees and private backyards. No Hitman
models, textures or game files are used.

## Downloaded and integrated

`Photoreal/sources.json` records the author metadata, original URLs, resolutions,
file sizes and expected MD5 for every downloaded file. All 17 sources are
[Poly Haven CC0](https://polyhaven.com/license), usable in a commercial game.
The source geometry and 4K albedo, normal and roughness/metallic maps are retained.

- Models: modular electricity poles; painted wooden bench; metal trash can;
  fire hydrant; wall-mounted garden hose; garden sprinkler; planter box; clay
  planter; plastic garden chair; outdoor table/chair set; garden gnome.
- PBR surfaces: weathered plank siding; roof shingles; aerial asphalt; concrete
  pavement; grass path; peeling painted wood.
- Modular poles are exported as three assembled presets. Table and chairs are
  separate meshes. No parts-library display arrangement is placed into the game.

[Modular House l Garage l Shed l Pool l Kit](https://sketchfab.com/3d-models/7dfa7d64895d4b1394162401366e806a)
by [burnedhrum](https://sketchfab.com/burnedhrum), licensed under
[Creative Commons Attribution 4.0](https://creativecommons.org/licenses/by/4.0/).
Downloaded original FBX and PBR glTF archives on 2026-10-08. Source archives,
textures and the editable Blender conversion are retained. Modifications:
assembled the author's example groups into three house presets and a garage;
separated the pool, mailbox, swing and lounge; centered floor pivots and converted
materials for Unreal. Both layouts use these assets alongside the original
Colonial and Cape Cod architecture. Creator attribution must accompany builds.

Original project geometry supplies a cedar hopper feeder, galvanized pole and
baffle, seed scatter, sprung backyard trampoline, capped cedar fence, oak limb,
service cable, potting bench, Colonial and Cape Cod houses, street, walks,
driveways and curbs. Houses include physical clapboard courses, closed gables,
porches, railings, steps, shutters, divided windows, gutters and downspouts.
These are project-authored substitutes, not acquired premium environment packs.
The mature oak/grass meshes reuse the project's existing licensed forest assets;
the oak limb also reuses the existing CC0 `jolcham_oak_bark_01` PBR source from
`ArtSource/Lobby/Photoreal`.

`model-index.json`, `material-index.json` and `unreal-import.json` record the
exports, material assignments, Nanite flags and imported dimensions. FBX collision
follows static triangles, preserving trampoline legs, feeder openings and fences.
Gameplay landing meshes use full triangle collision with Nanite disabled; the
planter receives a soil surface. Placement rotations use explicit yaw in Unreal.
Hose reel positions are measured from exported wall triangles by
`measure_hose_mounts.py`; `hose-mounts.json` preserves the contact points.
The kit pool is seated by its top coping, with a matching terrain depression
and terrain vertices at the basin edges so lawn cannot cross the pool interior.
The example kit pool's open ends are completed with a solid concrete liner;
the pool lot is placed clear of driveways, chairs and house foundations.
The lawn albedo is graded green while retaining the source surface detail,
normal map and roughness.

## Configurations

- `Configs/MapleCrescent.json` -> `DA_MapleCrescent` -> `/Game/Maps/Neighborhood`.
- `Configs/CedarBackyards.json` -> `DA_CedarBackyards` -> `/Game/Maps/NeighborhoodCedar`.

Both configs contain prop transforms, two start locations, landing heights,
checkpoint labels, utility-pole climb entry and trampoline launch strength.
They are editable Unreal DataAssets, assigned to the map's WildlineWorld actor.
The host chooses the neighborhood in Match Rules; clients travel to the same map.
Layout choice is saved with the King of the Hill match configuration.

## Acquisition blockers

| Candidate | Verified blocker | Intended use |
|---|---|---|
| [Artec bird feeder scan](https://sketchfab.com/3d-models/414d27fb511d4ffd91abcc686f1af17d) / [publisher page](https://www.artec3d.com/3d-models/bird-feeder) | Publisher's Sketchfab page has no download option or reuse license. No viewer geometry was extracted. | Potential alternate scanned hero feeder. |

## Fab acquisitions

[City Sample Vehicles](https://www.fab.com/listings/2909157b-ddfa-4cef-a925-69dc2467021f)
by Epic Games downloaded successfully through the authorized Epic account on
2026-10-08. Launcher delivery and verification completed successfully; all 1,405
files are installed under `Content/CitySampleVehicles` (about 7.4 GiB). Selected
the newest available asset package, UE 5.3, through **Show all projects**; the
WILDLINE project remains on UE 5.8. Listing specifies UE-only use. Intended for
parked cars and pickups. Two sedan types, a pickup and a van now dress the
curbs and driveways of both neighborhood layouts. The selected complete static
LOD meshes retain Epic materials and texture dependencies.

[City Park Environment Collection](https://www.fab.com/listings/11cc2abb-126c-4452-9fe4-6f2381d96544)
by SilverTm downloaded and verified successfully through the same Fab account on
2026-10-08. Its UE 5.7 source project contains 1,123 files (about 4.07 GiB), saved
under `native/AssetDownloads/Fab/CityParkEnvironment` with
`CityParkEnvironmentCollec.uproject`. Fab delivers this pack via **Create Project**.
The original source project is retained outside tracked game content. Its park
content has been migrated into WILDLINE; selected benches, trash bins, street
lamps and a drinking fountain dress both neighborhood layouts.

`Scripts/import_world_upgrade.py` duplicates selected meshes into
`/Game/Wildline/Fab/Meshes`, ensuring that string-based configuration references
are included by the game's existing cook rules. Source materials and textures
remain referenced dependencies. Measured bounds align each prop's ground contact
and physical scale; `fab-placement.json` records those measurements. Placement is
also saved in both editable JSON configs and cooked layout DataAssets. Reimporting
the neighborhood configs retains the Fab dressing. No paid purchases were made.

Vehicle virtual textures are enabled in the project renderer settings. Selected
Fab dependency textures are capped at 2048 pixels without changing their source
images; `fab-texture-budget.json` records the affected assets. The new solid
course oak is oriented at 180 degrees to keep the wire approach and fence descent
open. This orientation is saved in both layout configs and their generator.

The launcher-only delivery blocker was resolved by registering
`C:/_git/wildline/native/Unreal` as a `CreatedProjectPaths` entry in the launcher's
active `WindowsEditor/GameUserSettings.ini`, restarting the launcher and repairing
the `.uproject` file association. Its **My Projects** list now shows WILDLINE.

Registration was attempted with the user-authorized email, birth date and terms
approval. Passwords are not stored in source, configs, manifests or credits.
The supplied existing Epic account subsequently signed in successfully, without
an email code, and enabled the two free Fab acquisitions and Sketchfab download.
The rejection screenshot is local and excluded from Git:
`native/Unreal/Wildline/Saved/AccountRegistration/EpicRegistrationBlocked.jpg`.
No purchases or paid subscriptions were made.

## Validation

Validation on 2026-10-08: the neighborhood asset/layout automation passes, and
the separate Windows Development package passes all four physical route probes.
Over 75 simulated seconds, Maple's utility/garden routes completed 5/8 feeder
trips and Cedar's completed 2/7. The probes also check ordered landings, lawn
reset, feeder camping, restart, trampoline reach and match scoring.
`validation.json` records the results. Run `native/VerifyNeighborhood.ps1`
with `-GamePath` pointing to the review package's Win64 executable to reproduce.

## Rebuild

1. Run `Scripts/download_neighborhood_assets.py` with Python.
2. Run Blender in background with `Scripts/build_neighborhood_assets.py`.
3. Retain the licensed house-kit archive; run Blender with
   `Scripts/build_house_kit.py`, then `Scripts/configure_neighborhood.py` with Python.
4. Build WildlineEditor; run Unreal's Python commandlet with
   `Scripts/import_neighborhood_assets.py` (NullRHI is suitable for import).
5. Generate seasonal trees and hunter busts with the Blender preparation scripts,
   then run `Scripts/import_world_upgrade.py` in the full unattended Unreal editor
   using `-ExecutePythonScript`. Run `Scripts/fix_world_material_usage.py` to
   persist material usage flags, and `Scripts/rotate_course_oaks.py` to restore
   the verified solid-oak orientation when upgrading older configs.
6. Run `native/VerifyNeighborhood.ps1 -Editor` for all four physical routes and
   `native/VerifyWorldUpgrade.ps1 -Editor -Net -Capture` for limbs and presentation.
7. Build through `native/LaunchNative.ps1 -Build` to publish the newest successful
   Windows package through the existing desktop icon.
