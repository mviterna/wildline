# Hunter assets and portrait workflow

Created and integrated on 2026-10-04.

On 2026-10-05 the runtime hunter animation proxy was updated to blend adjacent
movement headings, advance idle and airborne clips, share vertical aim across
the upper body, and keep the weapon stock and both wrists on one evaluated
shoulder pose. Free asset acquisition candidates and login/download instructions
are in `UPGRADE_CANDIDATES.md`.

The user's downloaded [Epic Game Animation Sample](https://www.fab.com/listings/880e319a-a59e-4ed2-b268-b32dac7fa016)
now supplies 63 captured animations: eight directions each of walk, run and
crouch; directional walk/run starts and stops; standing and crouched idles;
left/right turns; takeoff, fall and landing. These Epic assets retain their
applicable Epic content license. [Epic's official sample announcement](https://www.unrealengine.com/blog/game-animation-sample?lang=en-US)
allows the animations to be retargeted and used in Unreal projects with or
without the sample's animation blueprints. They are free assets, not CC0 assets.

`export_hunter_motion.py` runs the sample's UEFN-to-UE5 IK retargeter before
exporting animation-only FBX. `import_hunter_motion.py` imports the selected
clips onto the existing hunter mannequin skeleton. The editable FBX sources,
original asset paths, measured stride lengths and synchronized gait offsets are
retained in `GameAnimationSample/`. Capsule movement remains authoritative;
root translation is locked for the in-place runtime clips. No sample gameplay,
motion-matching databases or unrelated project assets are migrated.

The native animation proxy synchronizes left-foot contact across directional
clips and advances gait by the distance actually travelled. It layers the
existing rifle-ready upper body in component space, retains chest displacement
from the capture, solves terrain contact and stance-foot planting, and adds
captured crouch, start/stop, turn and jump/landing transitions. Footstep audio
follows gait contact. The gun and both wrists share the evaluated shoulder
transform, including aiming, recoil and the shotgun fore-end movement.
Forward-sector indices wrap even when double-to-float rounding produces an
angle of eight sectors; otherwise all blend weights could decay to zero after
a turn and expose the standing reference pose. Crouched ledge falls retain the
compact captured squat, and the standing landing overlay respects crouch.

On 2026-10-08 the user selected the photo-superimposed Travis variant and discarded the detailed MetaHuman version. The former Travis 2 is now Travis, and Ethan is a separate photo-based selectable hunter. Both retain the existing MakeHuman body, outfit, boots, cap, slim-body morph and Manny rig. See `TRAVIS.md`.

- Human anatomy, skin, eyes, eyebrows, hair, work clothing and boots come from the [MakeHuman system asset pack](https://static.makehumancommunity.org/assets/assetpacks/makehuman_system_assets.html), explicitly listed as **CC0**. The basemesh and anatomical targets come from [MPFB](https://github.com/makehumancommunity/mpfb2); its core generated assets are CC0, while the authoring software has its own GPL license. Only generated geometry and textures are used by the game. Copyright notices in the source asset files identify Data Collection AB, Joel Palmius and Jonas Hauquier.
- Two adult human variants are generated with MPFB's anatomical system, dressed in field clothing and boots, with an original curved field cap. Skin, hair, eye and clothing textures are retained in `Textures`. Their original CC0 source pack is retained in the ignored authoring download directory.
- Epic's installed Manny reference mesh supplies the existing locomotion skeleton and skin-weight transfer. Those engine template assets retain Epic's applicable license. The generated hunters use the existing mannequin animation blueprint and skeleton.
- `PortraitFixture.png` is the CC0 MakeHuman young Caucasian male skin thumbnail. It is a development test image, not an uploaded user photo or a shipped default face photo.
- The sporting bolt-action rifle and curved cap are original project geometry. The rifle is about 110 cm long, with a walnut-style stock, steel receiver, enclosed magazine, trigger guard, optical scope and separate bolt/handle geometry. The visible bolt lifts, slides back, returns and locks after firing.

Authoring scripts: `generate_hunters.py`, `generate_bolt_rifle.py`, `export_hunter_reference.py`, `import_hunter_assets.py`, and `finalize_hunter_content.py`. Downloaded blend files and embedded scripts are not executed. MPFB is deliberately used as a source-controlled authoring dependency with Blender auto-execution disabled. Exported FBX and generated `.blend` files are editable source assets.

The creator accepts local PNG/JPEG portraits, decodes and resizes them in the native game, and maps the photograph onto the front of a detailed 3D head with a feathered face mask. Crop/zoom and alignment controls are previewed live; face width and nose depth use mesh morph targets. This transfers photographic appearance onto the selected head; it is not photogrammetry or automatic reconstruction of a complete head from one image.

Photos stay in memory while editing. Saving the hunter stores a compact JPEG in the game's `Saved/Profiles/HunterFace.jpg` and persists the appearance settings. The compact portrait and appearance are replicated to players in that co-op session, including late joiners. No photo is sent to an external reconstruction service. Cancelling the creator keeps the previously saved hunter and portrait.

UE 5.8 Interchange's runtime skeletal buffers place the portrait projection at UV0 and the original diffuse atlas at UV1, although exported source FBX retains the opposite order. The materials use the verified runtime order. Cloth and boots currently use mesh normals: their original normal textures remain in the source folder, but need tangents rebuilt against diffuse UV1 before being enabled again.

Logs use twelve original convex collision wall segments, leaving their hollow tunnel open. The engine's legacy FBX importer imports the named UCX hulls; the Interchange path in this engine build skipped them.

The four built-in photo presets use unchanged user-supplied images:
`Textures/travis_reference.png` comes from
`codex-clipboard-beb6573a-6c4f-41d8-aa30-ca42f8a949fe.png`, and
`Textures/ethan_reference.png` comes from
`codex-clipboard-b9766ea3-2551-4f5d-adfd-51417ac71a17.png`, and
`Textures/greyson_reference.png` comes from
`codex-clipboard-0ae6cb0e-1d1f-4209-bf4f-a8c7e808fa7e.png` (added 2026-10-08).
`Textures/mark_reference.png` is an unchanged copy of the user's
`Downloads/mark.png` (added 2026-10-17; SHA-256
`395e59433a816b2cedd8d485bc5dd2f942a9e2bc359b3b61f2041ec00843fef7`).
These are personal reference photos, not CC0 assets or AI-generated textures.
Original simple head and ear geometry carries each image without modeled
facial features. Both face UV channels intentionally carry the same coordinates.
The discarded MetaHuman head, textures, materials and authoring path are removed.

The face materials retain the source photo color and add a small
exposure-compensated fill so eyes and expressions remain readable in shade.
They retain default-lit shading. The creator uses two soft lights closer to
face level. No portrait has been retouched or sent to an external service.
The rifle layer retains the locomotion neck/head direction and a connected
neck-to-head chain, so the ADS capture no longer bows and rolls every hunter's
face into the stock. Vertical aiming still moves the chest, neck and head.

Acorn Hollow's mounted Travis, Ethan and Greyson busts are static derivatives of
these existing player models. `prepare_hunter_trophies.py` crops below the collar
and retains the original face UVs, cap, skin and cloth materials. No additional
portrait source or license is introduced. `import_world_upgrade.py` imports the
busts into `Content/Wildline/Hunters/Meshes`; the room supplies oak shields,
brass nameplates, warm display lighting and the squirrels' rivalry captions.
