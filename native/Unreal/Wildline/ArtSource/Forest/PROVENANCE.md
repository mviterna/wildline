# Forest asset sources

Retrieved 2026-10-04 through the user's authorized BlendSwap account. No credentials are saved in this project.

| Source | Creator | License | Use in Wildline |
|---|---|---|---|
| [Realistic Grass Field](https://blendswap.com/blend/17567) | CraigForster | CC0 1.0 | Authored grass, clover, weeds and stone meshes; meter-scaled and converted to FBX |
| [Pine Tree](https://blendswap.com/blend/22602) | Usch18 | CC0 1.0 geometry | Trunk and branches; sampled canopy geometry converted to photographed needle clusters |
| [Grass and trees system](https://blendswap.com/blend/16674) | piopis | CC0 1.0 geometry | Three authored oak silhouettes; replacement materials and photographic leaf cluster atlas |
| [4 Fern Models (Low Poly + LoD)](https://blendswap.com/blend/17143) | OliverMH | CC0 1.0 | Four curved fern meshes and original packed photographic alpha texture |
| [Leaf Set 001](https://ambientcg.com/a/LeafSet001) | ambientCG / Lennart Demes | CC0 1.0 | Photographic leaves, alpha and normal maps; a twig atlas rendered from these leaves; individual curved fallen leaves |
| [Forest Leaves 02](https://polyhaven.com/a/forest_leaves_02) | Rob Tuytel / Poly Haven | CC0 1.0 | Scanned forest floor color, normal and roughness |
| [Grass Medium 01](https://polyhaven.com/a/grass_medium_01) | Poly Haven contributors (see source page) | CC0 1.0 | Two curved grass clumps and photographic blade color, mask and normal atlas |

The original BlendSwap license HTML files are retained beside the source blends. CC0 permits commercial use, modification, and redistribution without attribution. Credits are retained for provenance. [ambientCG license](https://docs.ambientcg.com/license/), [Poly Haven license](https://polyhaven.com/license).

Third-party textures embedded in the pine and broadleaf files (including Textures.com, Flickr and other sites) are **not imported or used**. Their original downloaded blend files are source reference only. The shipped tree bark and needles use the separately sourced CC0 Poly Haven maps already documented in the project. No BlendSwap add-on or embedded script is executed.

`Scripts/export_forest.py` converts the geometry with Blender auto-execution disabled. It preserves the source files, scales vegetation to centimeters through FBX meter metadata, and paints growth weights for wind. `Scripts/bake_leaf_cluster.py` renders original leaf cards from the ambientCG atlas. `Scripts/import_forest.py` creates two-sided foliage materials, root-weighted wind, forest-floor PBR material and reduced distance LODs, then rebuilds Blackwood.

The broadleaf source uses generated coordinates for its individual leaves and zero bark UVs. The conversion gives leaf cards explicit photographic UVs; bark uses world-position mapping in Unreal. Branch cards receive a complete twig atlas. Near-distance LODs retain most leaf geometry so crowns stay full.

The native forest uses a fixed random seed so tree positions and climb collision match on every co-op client. Static plant instances are visual only; trunk collision and terrain remain authoritative movement surfaces.

After exporting the BlendSwap pack, run `Scripts/export_photo_grass.py` to produce the two main photographic grass variants in the same FBX paths. `Scripts/export_deadwood.py` supplies an original weathered hollow log, used for scattered deadwood and the animal den. Import with the full Unreal Editor (`-ExecutePythonScript`), because mesh LOD editing requires editor subsystems unavailable in a Python commandlet.

The polish pass measures stem cross sections from the same CC0 geometry (`trunk-profiles.json`); `WildlineTrunkProfiles.h` records continuous main-branch paths in Unreal coordinates. Pine planting buries its 4.6 m source root collar before scaling. Gameplay trees use segmented invisible collision along these paths. A separate deterministic seed places visual woodland beyond a collidable original split-rail fence, with rising terrain enclosing the playable reserve.
