# Photo hunters: Travis, Ethan, Greyson and Mark

Updated 2026-10-08. The user selected the photo-superimposed approach and discarded
the detailed MetaHuman variant. The former Travis 2 is now named **Travis**.
**Ethan** is a separate selectable hunter using the newly supplied portrait.
**Greyson** is the third named hunter, added with the supplied portrait on the same date.

All three heads carry an unchanged source photograph on a 544-vertex, 512-quad curved
shell. There are no modeled eyes, nose, mouth, teeth, eyebrows or beard beneath
the image. Simple ears, scalp and neck complete each head. All use the existing
hunter body, Manny skeleton, animations, clothing and build morph. Clothing, cap,
skin tone and face width remain editable. Hair, beard and nose-depth controls are
disabled for these photo heads because those features belong to the photo.

## Sources

- `Textures/travis_reference.png`: original supplied smiling Travis photo.
- `Textures/ethan_reference.png`: original supplied Ethan photo.
- `SK_HunterTravis.blend`, `Export/SK_HunterTravis.fbx`: photo Travis.
- `SK_HunterEthan.blend`, `Export/SK_HunterEthan.fbx`: photo Ethan.
- `Textures/greyson_reference.png`: original supplied Greyson photo.
- `SK_HunterGreyson.blend`, `Export/SK_HunterGreyson.fbx`: photo Greyson.
- `travis-model.json`, `ethan-model.json`, `greyson-model.json`: geometry reports.
- `travis-unreal.json`, `ethan-unreal.json`, `greyson-unreal.json`: persisted import reports.

Photos are user-provided reference material, not CC0 or generated textures.
No image is uploaded to an external reconstruction service. Built-in photo
materials are cooked into the game and need no custom portrait upload.

## Rebuild and check

1. Run `Scripts/build_photo_hunters.py` in Blender with auto-execution disabled.
2. Run `Scripts/import_photo_hunters.py` through Unreal's Python executor.
3. Run `Scripts/verify_photo_hunters.py` in a fresh editor session.
4. Compile and package. `-WildlinePhotoHuntersReview` captures both named hunters.
   `-WildlineTravisReview` captures Travis's full body, customized appearance and
   gameplay. `-WildlineGreysonReview` captures Greyson's face, turns and customized
   full body. These review modes do not change the saved profile.
5. `Wildline.Hunters.PhotoPresets` checks selection, edits, reset and migration.
   `-WildlineSmoke` checks cooked meshes, materials, morphs and rifle grips.

`render_travis_source.py -- --ethan` renders Ethan's actual source geometry;
`-- --greyson` renders Greyson; without either option it renders Travis. Both photo UV channels intentionally use
the same coordinates. The explicit Interchange importer replaces vertex masks
and preserves authored normals. The only vertex-color layer is `PhotoMask`.

Preset ID 1 is Travis. Retired ID 2 migrates to Travis so previously saved
Travis 2 selections retain their identity and appearance edits. Ethan uses ID 3.
Greyson uses ID 4. Mark uses ID 5, adult height and proportions, and the supplied
`Textures/mark_reference.png`. His source files are `SK_HunterMark.blend`,
`Export/SK_HunterMark.fbx`, `mark-model.json` and `mark-unreal.json`.
The visible options are Custom hunter, Travis, Ethan, Greyson, Mark. MetaHuman Creator is no
longer required. Existing body and animation credits remain in `PROVENANCE.md`.

`native/VerifyHunterFaces.ps1 -Capture` checks the cooked presets and captures
all four face close-ups, Mark's turn, full body, cap/clothing customization and
in-world appearance. These checks use isolated profiles and preserve player
settings. `native/VerifyHunterAnimations.ps1 -Presets Mark -Rates 60` checks
Mark's live movement, stance, feet and weapon grips.

Preset default height (uncapped): Travis and Mark use the 174.5 cm source height, Ethan is
71.4 cm (30% shorter than the previous 102 cm), and Greyson is 110.4 cm (20% shorter
than the previous 138 cm). `FWildlineHunterProfile::HeightScale()` scales the
complete actor, including collision and equipment. The original editable rig
and animations remain at their authored size; gait and foot placement convert
between rig and world distances. `-WildlineHunterAnimationProbe` with
`-WildlineHunterPreset=Ethan` or `Greyson` verifies actual movement, crouch,
jumps, foot planting and both weapon grips. `-WildlineHunterHeightReview`
captures all three with height-aware full-body framing and both short heads
in face close-up, without changing the saved profile.
Runtime hand/foot scales relative to the body are .72/.75 for Ethan and .82/.84
for Greyson. The animation scales the wrist/ankle bones and their descendants,
preserving finger and toe articulation; ankle targets account for smaller soles.
The creator evaluates the same proportions on its unarmed idle pose.
Run `native/VerifyHunterAnimations.ps1 -Presets Ethan,Greyson -Rates 30,60`
to check both heights with isolated standard settings at those frame rates.

### Editable body settings

Named presets initialize height, hands and feet; `DimensionsVersion` migrates
older saves once. Height (60-230 cm), width (70-150%), depth (70-160%), hands
and feet (50-150%) and head/face size (60-150%) are saved/replicated profile
fields. Weight/build (0-150%) reuses `SlimBuild`, including negative morph
weights for heavier bodies. Defaults preserve all existing shapes.
`FWildlineHunterProxy::ApplyBodyShape` reshapes the rig before terrain and
weapon IK; separate head/hand/foot subtrees retain independent proportions.
Weapons stay uniformly scaled to height. Creator controls share the game rig,
show measured values, and frame the selected height and face size.
`VerifyHunterBodies.ps1 -Editor -Capture` supports iteration before packaging;
the default invocation verifies the final packaged build in isolated profiles.
