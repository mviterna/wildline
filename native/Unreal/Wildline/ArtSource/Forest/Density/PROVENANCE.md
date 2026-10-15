# Configurable tree canopies

These FBX files derive from the existing licensed forest sources in `../Export`.
They retain the full authored bark, branches, leaf cards, UVs, normals and wind
weights. `Scripts/prepare_tree_density.py` assigns each connected leaf card a
stable rank in vertex color G; vertex R keeps the original wind weight.

`Scripts/import_tree_density.py` imports the meshes and adds a `LeafDensity`
parameter to the existing leaf and needle materials. Ranks below that parameter
remain visible. Zero uses the existing bare meshes; 100 shows all authored cards.
Tree placement, collision and climb profiles remain independent of this setting.

The deterministic rank/card counts are recorded in `manifest.json`. Regenerate
with Blender, then run the Unreal import script with no game/editor probes
reading these assets during import. Validate with `native/VerifyGameplayBugs.ps1
-Capture` after packaging through `native/LaunchNative.ps1 -Build`.
