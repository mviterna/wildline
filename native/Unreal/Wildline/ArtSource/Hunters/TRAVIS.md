# Travis hunter preset

Travis is an editable starting appearance in the native Unreal hunter creator.
Choose **Create Hunter → Starting preset → Travis**, customize the look, and
select **Save Hunter**. Reset selected preset restores the original Travis look;
Cancel leaves the previously saved hunter untouched. Existing saved hunters
continue to use their custom appearance.

The starting look has a slim male body, receding hair, salt-and-pepper goatee,
modern camouflage clothing, blaze-orange torso panels and hunting boots. It uses the
game's existing sporting rifle, locomotion skeleton and animations. Body type,
body slimness, clothing/vest color, hair color, cap, goatee, skin tone, face width,
nose shape and facial-photo alignment remain editable. A user portrait can
replace the bundled facial appearance. Selecting the female body uses the
existing female hunter mesh; selecting the male body again restores Travis's
head and field clothing while retaining edited settings.

Editable source: `SK_HunterTravis.blend`. Skeletal export:
`Export/SK_HunterTravis.fbx`. Imported game asset:
`/Game/Wildline/Hunters/Meshes/SK_HunterTravis`. Source and game meshes include
`FaceWidth`, `NoseDepth` and `SlimBuild` morphs and nine material slots. The
source also retains skinned goatee/vest shells attached to the same rig. Their
runtime materials hide the coarse shells; the beard comes from the facial
texture, and the smooth torso panels use the jacket's vertex colors.

Rebuild the source hunters with `generate_hunters.py`, then run
`generate_travis.py` using Blender with auto-execution disabled. In Unreal, run
`import_hunter_assets.py`, followed by `finalize_travis.py`. Compile C++ after
source changes. Skeletal imports use Interchange to preserve Blender's shape
channels; static collision meshes use the legacy FBX factory. The authoring
pipeline clears inherited deform layers before transferring normalized rig
weights, and keeps the original UV map first and facial projection second.
`Wildline.Hunters.TravisPreset` checks preset customization and
bounds; the existing live smoke test also checks cooked Travis geometry,
bundled face texture and edited body morphs. `-WildlineTravisReview` generates
full-body, close-up, customized and in-game screenshots without saving over
the user's profile.

The supplied photos were used as likeness references. The facial projection
textures were created using the built-in image tool, with prompts for a
faithful neutral frontal face under diffuse light and an aligned clean-shaven
variant. Files: `Textures/travis_face.png` and `Textures/travis_face_shaved.png`.
The head is an adapted game mesh with photographic appearance projection,
not a scanned reconstruction. Human/rig source licenses remain recorded in
`PROVENANCE.md`. The character concept image is a visual reference; the
actual runtime model uses the mesh, textures and materials described here.

The preset identifier and independent appearance settings use the existing
saved-profile and replicated co-op path. Bundled textures are cooked assets;
they require no photo file on another player's computer.
