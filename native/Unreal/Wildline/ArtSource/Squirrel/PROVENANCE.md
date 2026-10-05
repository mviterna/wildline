# Squirrel source and integration

The rig, mesh and original colour/eye atlases are from WildMesh3D's publicly offered **Rigged Squirrel** in its free animal-model pack, downloaded on 2026-10-04.

- Publisher: https://www.wildmesh3d.com/free-models
- Publisher archive: `50+ Static Animals.zip`; selected nested archive: `Rigged Squirrel.zip`.
- The FBX and Blender source files and original PNG atlases are retained here. The publisher's free-model page is the source of permission to use the asset in projects; these third-party files are not relabelled as original WILDLINE work or CC0.

The FBX uses centimetres with +Y forward. The imported rest bounds are 42.505 cm nose to tail, 7.617 cm wide and 14.575 cm high. Runtime uniform scale 1.15 gives **48.881 cm total length**, approximately **23.4 cm head/body** and **23.6 cm tail**. The curled, animated tail changes the visible bounding box, not the anatomical scale. This target is within eastern grey squirrel dimensions reported in the University of Wisconsin–Stevens Point species account (total length 380–525 mm, tail 150–250 mm): https://www3.uwsp.edu/biology/VertebrateCollection/Pages/Vertebrates/Mammals%20of%20Wisconsin/Sciurus%20carolinensis/Sciurus%20carolinensis.pdf

WILDLINE adds native skeletal gait, paw IK, breathing, head/ear motion, blinking, articulated tail balance, jump tucking and climbing poses. The free FBX contains a rig, rather than an authored animation library. Bone-attached tapered fur geometry samples the original colour atlas; matte coat micro-relief and reflective eye materials are authored in Unreal. The integration does not include a cinematic groom or captured animal animation.

`Squirrel Smooth.fbx` is a WILDLINE derivative exported from the original Blender file with two subdivision levels and smooth face normals. It keeps the source rig, weights and UV atlas. `Scripts/smooth_squirrel.py` reproduces this processing with Blender 4.5.3. The original files remain intact. The FBX unit conversion appears as a 100× parent transform in the skeleton; the animation correctly converts centimetre-scale body motion into that parent's local space.

`Scripts/import_squirrel.py` imports the smoothed FBX when present, otherwise the original. Then `Scripts/tune_squirrel.py` preserves the smooth imported normals and prepares CPU data, materials and animated bounds. Run these after compiling the Wildline editor module. `rig-info.json` records the rig's bone positions and render bounds (including any animation bounds extensions already on the asset); the original unextended dimensions are given above.
