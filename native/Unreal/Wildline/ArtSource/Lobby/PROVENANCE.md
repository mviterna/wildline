# Waiting room assets

Upgraded 2026-10-08. The rooms use 22 downloaded Poly Haven models and six
4K scanned/PBR texture sets, licensed [CC0](https://polyhaven.com/license).
Source files, author metadata, URLs, checksums and resolutions are retained
in `Photoreal/sources.json`. No credentials are included in these files.

The model set includes a weathered picnic table, rocking chair, bookcase,
antique lanterns/chandelier, axe, bucket, woven basket, books, enamel cookware,
wooden bowl, display stand, picture frame, brass drinking vessels, stone fire
pit, tree stump, deadwood, root clusters, pine roots, moss and bark debris.
`model-index.json` records converted geometry, dimensions and material slots;
`material-index.json` records the source PBR map connections.

The authored lodge architecture has individual bevelled oak boards,
irregular log profiles, ceiling joists and recessed stone masonry. The hollow
tree uses continuous sculpted bark with scanned roots, moss and leaf litter.
These use licensed 4K timber, oak bark, masonry, fabric and forest-floor maps.
The carved squirrel shields, detailed acorn skin/cupules and stone portal
arch are original meshes, also surfaced with these licensed PBR maps.
These are created by `convert_lobby_assets.py` and `generate_lobby_fixtures.py`.

Four mounted squirrel heads retain the project's licensed source squirrel
UVs, coat detail and eyes. `extract_squirrel_trophy.py` selects its head/ear/neck
and facial nose groups. Grey, fox, American red and melanistic eastern grey
variants have authored proportions, coat materials, sizes and mounted poses;
they are adaptations of this source mesh, not separate scanned species.
`import_lodge_trophies.py` imports these variants with dedicated dark nose and
glass-eye materials, without modifying the playable squirrel.
The attribution/license remain in `ArtSource/Squirrel/PROVENANCE.md`.
The existing rifle and shotgun retain their respective source credits.
The preceding primitive deer/fox heads and bead portal arches are superseded.

Reproduction order:

1. Run `Scripts/download_lobby_assets.py` with Python. Each downloaded file is
   checked against its public API MD5 and retained with its source metadata.
2. Run `convert_lobby_assets.py`, then `generate_lobby_fixtures.py` and
   `generate_lodge_details.py`, with Blender
   in background mode. Source glTF UVs and material assignments are preserved.
3. Run `import_lobby_assets.py` through Unreal's Python commandlet. It connects
   base color, OpenGL normals (green inverted for Unreal), roughness/metal/AO
   maps and alpha masks. Textures retain 4K resolution; opaque mesh geometry
   uses Nanite. Glass uses conventional translucent rendering.
4. Run `extract_squirrel_trophy.py` against the retained source squirrel blend,
   then `import_lodge_trophies.py` through Unreal. Run
   `enhance_portal_materials.py` to create the ready-pad materials.
5. Build/package, then run `native/VerifyLobbyPackage.ps1 -Capture`.

`WildlineLobbyInteriors.cpp` arranges the assets. Cheap invisible room collision
is separate from the detailed visual architecture. Decorative geometry is built
locally on each peer; weapons, readiness and round entry remain server-owned.

The lodge has five true apertures cut through the logs. Original oak sash
casings, sills, gathered damask curtains, brass rods/rings/ties, a marked analog
clock and a brass peg rail are authored by `generate_lodge_details.py`. These
reuse the retained CC0 timber/fabric maps; the curtain pigment is desaturated
and recolored green. `WildlineLodgeDetails.cpp` arranges these with a specimen
shelf and pinned field notes. The outside clearing reuses the project's existing
licensed forest trees and leaf-litter materials, plus CC0 deadwood/moss. It is
actual 3D geometry with view parallax, shared by all three map waiting rooms.
Exterior scenery is visible only to a local camera inside the lodge, so the
elevated waiting-room clearing cannot appear in the outdoor match sky.
Room collision stays continuous behind the windows. To refresh only this
work, convert with `--lodge-only`, regenerate lodge details, then run
`import_lobby_assets.py -LodgeDetailsOnly` and `import_lodge_trophies.py`.

The stone fire-pit source is retained as an alternate hearth detail; the built
interior uses the authored recessed fireplace. Provider thumbnails or example
renders are not imported or redistributed as game art.
