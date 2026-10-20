# Waiting room assets

## Map views, fire glow and hunting paintings, 2026-10-19

Two 1839 oil-on-panel paintings by John Frederick Herring are mounted in original
modelled gilt frames: *Foxhunting: Clearing a Ditch* (B2001.2.175) and
*Foxhunting: Encouraging Hounds* (B2001.2.176). Images come directly from the
Yale Center for British Art's IIIF image service. Both per-image records state
CC0 1.0 and permit commercial reuse. Credit: Yale Center for British Art,
Paul Mellon Collection. `Paintings/sources.json` retains source/catalog URLs,
image dimensions, exact rights statements and SHA-256 hashes; each complete
museum manifest is retained beside the original JPEG. No paid assets or accounts
were used. The complete images retain their aspect ratios and are not mirrored
or cropped into invented compositions.

Reproduce with `download_lodge_paintings.py`, Blender
`generate_lodge_paintings.py`, then the Unreal `import_lodge_atmosphere.py`
commandlet. The frame geometry is original; its oak backing uses the previously
retained CC0 timber maps. The fire halo is original procedural shader math,
supported by warm flickering point light and interior bloom.
The lit canvas materials include a diffuse fill to preserve image detail under
the room's fixed exposure, with separate brass picture-light fixtures.

`WildlineLodgeExterior.cpp` derives the view from the loaded map on each peer.
Frostwood uses the map's snow material, bare winter trees with frosted bark,
and snow on the exterior window ledges. Farmstead and both neighborhood layouts
reuse their actual meshes, materials and placements as decorative copies outside
the lodge. These copies exclude the room's bounds, have no collision, and only
render from a camera inside the lodge. Match actors and terrain are not moved.
Farm copies use conventional instancing with complete buffers registered once;
this avoids a reproduced Nanite instance-culling GPU fault on duplicated farm
components. Empty copy layers are discarded. The playable farm keeps Nanite.
`VerifyLodgeAtmosphere.ps1 -Capture` checks and captures all five maps.

## Hunter lounge upgrade, 2026-10-19

Five additional free CC0 Poly Haven models are integrated at 4K: `sofa_03`
(leather sofa), `ArmChair_01`, `gothic_coffee_table`, `vintage_cabinet_01` and
`throw_pillows_01`. The existing outdoor picnic furniture is superseded by a
coherent indoor lounge. All original downloaded assets remain in the project.
`Photoreal/sources.json` retains the provider metadata, license URLs and verified
MD5 hashes for each file. No account or purchase was needed.

The original bound/fringed wool rug is generated geometry, using the free CC0
`dirty_carpet` 4K color, normal and roughness maps. Its green border, contrasting
woven diamonds and individual cotton fringe strands are original mesh detail.
The floor/timber color factors and interior-only window lighting are authored
for this room. Imported source geometry, UVs and material slots are preserved;
Nanite is enabled for opaque furniture. Glass retains translucent rendering.

To reproduce the additions without reimporting the earlier rooms, run
`download_lobby_assets.py --upgrade-only`, then Blender with
`convert_lobby_assets.py -- --upgrade-only` and `generate_lodge_upgrade.py`.
Finally run the Unreal `import_lobby_assets.py` commandlet with
`-LodgeUpgradeOnly`, `finalize_lodge_upgrade.py` to persist the Nanite shader
permutations, and `create_lodge_fire.py` for the original procedural animated
hearth material. The fire uses shader math with no third-party imagery.
Asset search/download tooling is Powered by Poly Haven
(https://polyhaven.com); it is development tooling rather than a shipped API client.

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
actual 3D geometry with view parallax, shared by all map waiting rooms.
Exterior scenery is visible only to a local camera inside the lodge, so the
elevated waiting-room clearing cannot appear in the outdoor match sky.
Room collision stays continuous behind the windows. To refresh only this
work, convert with `--lodge-only`, regenerate lodge details, then run
`import_lobby_assets.py -LodgeDetailsOnly` and `import_lodge_trophies.py`.

The stone fire-pit source is retained as an alternate hearth detail; the built
interior uses the authored recessed fireplace. Provider thumbnails or example
renders are not imported or redistributed as game art.

Acorn Hollow's three hunter trophies are static neck-and-head derivatives of
the existing selectable Travis, Ethan and Greyson meshes, retaining the supplied
reference photographs and face UVs. `Scripts/prepare_hunter_trophies.py` crops the
unposed models; `Scripts/import_world_upgrade.py` assigns their existing hunter
materials. No new third-party portrait imagery was acquired. The trophy shields,
nameplates, keepsake shelf and lookout perch are authored in the lobby builder;
the bowls, books, moss, stumps and other scanned objects retain the licenses above.

The lodge's exterior clearing now extends 140 m in each direction, with a ring
of rising wooded ridges and 144 trees at several depths. These are derivatives
of the existing licensed reserve trees. Exterior components appear only while
the local camera is inside the lodge, keeping the elevated scenery out of matches.

Bounded room grades explicitly set exposure method, compensation and physical
camera behavior so lantern lighting remains consistent across the outdoor maps.
