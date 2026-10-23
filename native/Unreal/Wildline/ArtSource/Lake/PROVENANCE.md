# Taylorsville Lake

Boats, rods, water and dock surfaces are original procedural WILDLINE artwork. The lake lodge doorway derives from the existing WILDLINE lodge generator and reuses its retained CC0 scanned materials; see ArtSource/Lobby/PROVENANCE.md. Existing forest rocks, logs and ferns retain their original credits. Map thumbnails are captured directly from the game.
`M_LakeWater` reuses the retained CityPark `T_Water_N` normal texture; that texture remains under the existing Epic/Fab CityPark license. The dock, boat seats and lakebed reuse the retained CC0 lodge wood and forest litter PBR textures under their existing credits.

## Textured largemouth bass

"Largemouth bass" by faerbogdan99, licensed under Creative Commons Attribution 4.0 (CC BY 4.0): https://sketchfab.com/3d-models/largemouth-bass-87a588ddfe904fc8a578082fcc5ebb66

License: https://creativecommons.org/licenses/by/4.0/

The original GLB was downloaded through Sketchfab's authorized Download 3D Model control on 2026-10-22. Fish/sketchfab-source.json retains the source metadata. WILDLINE extracts the original GLB's embedded maps into Fish/Embedded (the separate Sketchfab texture files use a different atlas), reorients the mesh to +X, rescales it to 55 cm, imports its PBR maps with the normal green channel adapted for Unreal, adds shader tail motion, and varies instance size. No endorsement by the creator is implied. Export/SM_PhotorealBass.fbx is the editable integration export. The former primitive SM_LakeFish is superseded and is not spawned.

## Refined spinning rod

Scripts/refine_lake_assets.py authors cork rings, reel seat, graphite blank, line guides, thread bindings, a wound spool, and a separate animated reel crank. These meshes are original WILDLINE artwork.
