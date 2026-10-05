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

Travis is a named, customizable hunter preset derived from these same human
assets, with original receding hair edits, skinned vest/goatee shells and a
slim-body morph. Its facial appearance is AI assisted using the user's supplied
photos, not a third-party stock face. Both bearded and clean-shaven projection
textures are shipped for the editable goatee setting. See `TRAVIS.md` for source
files, authoring steps and the limits of the likeness workflow.

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
