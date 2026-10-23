# Lake asset authoring

The saved map is `/Game/Maps/TaylorsvilleLake`. Runtime lake scenery, fishing,
and swimming are in `WildlineLake.cpp` and `WildlineWaterGameplay.cpp`.

1. `Scripts/generate_lake_assets.py` runs in Blender to author the original
   boat, lodge doorway and base props. It preserves the retained attribution.
2. `Scripts/refine_lake_assets.py` runs in Blender to prepare the retained
   `Fish/source/base_basic_pbr(9).glb` bass, extract its embedded PBR maps, and
   author the detailed rod and separate crank. The exported bass is 55 cm long,
   faces +X and retains its original UVs. `-- --textures-only` extracts just maps.
3. `Scripts/import_lake_assets.py` initializes base assets for a fresh project.
   `Scripts/import_lake_refinement.py` imports the refined meshes and original
   embedded texture atlas, builds the tail shader and rebuilds the saved terrain.
   Run editor scripts with UnrealEditor-Cmd and `-ExecutePythonScript=...`.
4. `native/CaptureMapPreviews.ps1 -Maps TaylorsvilleLake` refreshes the game capture
   used by the Map tab. `native/LaunchNative.ps1 -Build` cooks and packages it.
5. `native/VerifyLakePackage.ps1 -Capture -Network` verifies gameplay, station
   reservations, input/casting replication, water rules and rendered assets.

Use `PROVENANCE.md` for the required bass attribution and retained material
licenses. The separate Sketchfab `Fish/textures` files use a different atlas;
the original GLB mesh requires `Fish/Embedded` maps.
