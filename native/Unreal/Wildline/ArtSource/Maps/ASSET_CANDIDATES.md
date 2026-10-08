# New map asset candidates: free assets only

Reviewed 2026-10-07 for three planned maps: a squirrel den "waiting room" shown
before a match, a hunting lodge, and a suburban neighborhood in the spirit of
Hitman's Whittleton Creek. Paid assets, paid trials, NonCommercial licenses and
free-download mirrors of paid packs are excluded. Nothing has been downloaded
or imported. Adding a candidate here does not integrate it.

**Previews:** open `native/Preview/MapAssetCandidates.html` in a browser. It
holds 228 assets, with tabs per map, source/type filters, search and a
hero-pick toggle. The thumbnails load from the source sites, so the page needs
an internet connection.

## Sources, licenses and logins

| Source | License | Login | Notes |
|---|---|---|---|
| [Poly Haven](https://polyhaven.com) | CC0 | None | Scanned models, 8K–16K textures, 16K+ HDRIs. |
| [Fab](https://www.fab.com) (Megascans, Megaplants, Epic, sellers) | Fab Standard License, $0 for Personal and Professional | The user's Epic/Fab account | Add the listings to the Fab library, then add them to the UE 5.8 project. |
| [Sketchfab](https://sketchfab.com) | CC-BY 4.0 (one CC0, one Sketchfab Free Standard) | Sketchfab account | Download the original-format scans. Credit CC-BY authors in the game credits. |

The agent did not create or use any accounts. Signing in to Fab and Sketchfab
is up to the user, as with the earlier Sketchfab farm downloads.

**Free filter warning.** Fab's "free" filter also returns Megascans that are
free only as *UEFN – Reference only*. For a UE project those cost $1.99–$2.99
(Personal) or $19.99–$29.99 (Professional). Every Fab item listed here was
checked on its listing for a $0 Professional price. The following were
excluded on that basis: Mossy Stump, Lichen Stump, Mossy Tree Log, Fallen Log,
Eroded Pine Log, Chipped Bark, Broken Tree Bark, Gilled Mushroom, Forest
Terrain, Firewood, Burnt Firewood and one of the two Megascans mailboxes.

**Engine versions.** Megaplants are Nanite procedural vegetation for UE 5.7+
and use the experimental Procedural Vegetation Editor. Several older free packs
(Protofactor, Dekogon, SilverTm, Sierra Division) predate UE 5.8. Expect to
migrate them through a separate project, as was done with the Game Animation
Sample.

## Squirrel den: waiting room

Build the room at squirrel scale inside a scanned hollow stump or root mass.
Macro scans of nuts, mushrooms, bark and moss become the furniture and clutter.

| Hero pick | Source | License | Role |
|---|---|---|---|
| [Big Hollow Tree Stump 2](https://sketchfab.com/3d-models/af9fcaf4228e419fa60ebe647ca3f155) | Sketchfab, b_nealie | CC-BY | Den shell: its hollow interior becomes the room. |
| [Uprooted fallen hornbeam tree](https://sketchfab.com/3d-models/870b595eda444b4ea23ab9f1db629942) | Sketchfab, matousekfoto | CC-BY | Root mass and soil for the ceiling. |
| [Ancient Temple Jungle Roots](https://www.fab.com/listings/d30849f6-13cd-4e6f-9664-d7db13b5555f) | Fab, Megascans | Fab Standard | Root lattice for the walls. |
| [Megaplants: English Oak](https://www.fab.com/listings/83642c38-7661-4df1-8629-0422e1898d26) | Fab, Megaplants | Fab Standard | Host tree and canopy outside the den. |
| [Hazelnut](https://www.fab.com/listings/c1ed0911-5f5e-4feb-950a-ad0427cd5b1b) / [Walnut](https://www.fab.com/listings/33d2ca51-dced-4078-b679-99eae42575d1) | Fab, Megascans | Fab Standard | Snack piles and seating. |
| [Acorn](https://sketchfab.com/3d-models/84ad040b23644329aa142e502a1d4b14) (Artec scan) / [Red oak acorn](https://sketchfab.com/3d-models/94bf085f6b5344afb18e45eb8fca5e85) | Sketchfab | CC-BY / CC0 | Hero acorns. |

Also strong: Megascans stumps, logs, branches, bolete and red mushrooms, and
leaf-litter, forest-floor and moss materials. From Sketchfab: a shelf-fungus
scan (seating or shelves at squirrel scale), pine cones and a hazelnut twig.
From Poly Haven: 8K root clusters, stumps and bark debris, and the
`forest_cave` HDRI for lighting. Project Nature's temperate ferns, ground
foliage and spruce packs, and Sierra Division's Procedural Nature Pack, cover
the forest outside.

## Hunting lodge

| Hero pick | Source | License | Role |
|---|---|---|---|
| [Modular Rural Cabins](https://www.fab.com/listings/508fe84a-4976-4cfe-9a40-c2b9533da601) | Fab, Maarten Hof | Fab Standard ("free forever", ★4.8, 78 ratings) | Cabin shell kit: 156 meshes with furniture and log piles. |
| [Log Cabin](https://sketchfab.com/3d-models/2657189dbdff49fdb20ba33d6ebd5c52) | Sketchfab, Kless | CC-BY | Log exterior with a stone foundation and chimney. |
| [Saloon Interior](https://www.fab.com/listings/57991a62-f98f-4c6a-9b94-a74ec12f242e) | Fab, Megascans | Fab Standard | Scanned wood interior project: great room or bar. |
| [Cigar Room Environment](https://www.fab.com/listings/4da78da6-44b3-4adf-8883-219fe17b44d4) | Fab, Hane Studios | CC-BY | Leather chairs, panelling and fireplace mood. |
| [Large Antler Chandelier](https://sketchfab.com/3d-models/84641f3024424a9983bd14146e5df4cb) | Sketchfab, GetDeadEntertainment | CC-BY | Signature light. |
| [Mounted Deer](https://sketchfab.com/3d-models/cc7dfa9255384129814b563f27b9722d) | Sketchfab, Andre Bond Sr. | CC-BY | Photoscanned whitetail mount (876k faces; decimate). |
| [Decorative Antlers Wall Mount](https://www.fab.com/listings/ccc26e12-6542-4133-a2d6-19b710bc30bc) | Fab, Megascans | Fab Standard | Trophy wall. |
| [Animal Variety Pack](https://www.fab.com/listings/2dd7964c-a601-4264-a53d-465dcae1644c) | Fab, Protofactor | Fab Standard | Rigged stag, doe, wolf, fox and crow (4K). Pose them as taxidermy. |

Also strong: Dekogon's Old West vols. 1, 2, 4 and 5 (furniture, tools,
supplies, town props) and Megascans' fireplace, old stove, candles, blanket
chests, leather luggage, barrels, books, smoked deer leg and animal skulls.
Megascans also has wood floor, log wall, leather, wool and antique-rug
materials. From Sketchfab: the Oatlands Park fireplace (a museum scan said to
come from Henry VIII's hunting lodge), a bear trap, a duck decoy, an elk antler,
snowshoes and a scanned rocking chair. The Sketchfab bear-skin rug is mid
quality. Poly Haven covers rocking and arm chairs, chandeliers, lanterns,
clocks, a fire pit, a bookshelf, stone and plank textures, and the `cabin` and
`fireplace` HDRIs.

## Neighborhood (Whittleton Creek)

| Hero pick | Source | License | Role |
|---|---|---|---|
| [City Sample Vehicles](https://www.fab.com/listings/2909157b-ddfa-4cef-a925-69dc2467021f) | Fab, Epic | Fab Standard | 13 drivable AAA cars from The Matrix Awakens. |
| [MetaRoad Free](https://www.fab.com/listings/a425c08b-d82a-4b42-ab94-101c3eb23f28) | Fab, udrive | Fab Standard | Procedural roads, sidewalks and curbs. |
| [Megaplants: Norway Maple Forest](https://www.fab.com/listings/e789ad04-cf79-4973-adb0-85c794d77144) | Fab, Megaplants | Fab Standard | Nanite street trees. |
| [City Park Environment Collection](https://www.fab.com/listings/11cc2abb-126c-4452-9fe4-6f2381d96544) | Fab, SilverTm | Fab Standard (★4.5, 243 ratings) | Lawns, trees, paths and benches. |
| [Modular House / Garage / Shed / Pool Kit](https://sketchfab.com/3d-models/7dfa7d64895d4b1394162401366e806a) | Sketchfab, burnedhrum | CC-BY | 76-piece frame-house kit with interiors. |
| [Cozy Cottage](https://sketchfab.com/3d-models/72c05612ebbc4f5f8eac26d047d5d69c) | Sketchfab, SINNIK | CC-BY | Sided cottage with interior, basement, attic, garage and yard. |
| [Suburban House](https://sketchfab.com/3d-models/efcc220c62f641d29a1480e8b221c7b3) | Sketchfab, Bogdan Strielecki | CC-BY | Clean archviz house with dormers. |

**Gap.** No free AAA American suburban *house* kit was found. The AAA-grade
suburban house packs on Fab (PropHaus Residential Houses, Suburb Neighborhood
House Pack and similar) are paid, and the free copies on mirror sites are
unlicensed. The free houses above are archviz or mid-quality models. They need
optimization and a material pass using Poly Haven siding (`weathered_plank_siding`),
shingles (`roof_07`) and Megascans lawn and asphalt.

Also strong: Megascans curbs, sidewalk corners, fence posts, a mailbox,
hydrangea, elderberry, lawn, asphalt and concrete materials, and a fireplace
grill. Greenleaf Vision's free wind-animated shrubs. Poly Haven's 8K hydrant,
street lamps, utility poles, manhole cover, trash can, garden props, siding,
shingle, driveway and grass textures, and the `suburban_garden`,
`pretville_street` and `stuttgart_suburbs` HDRIs. From Sketchfab: an American
fire hydrant, a US mailbox, a gas grill, a basketball hoop, a 1970s lawn mower,
a scanned trash can and a well-made shed. The Vehicle Variety Packs (American
cars, pickups, RVs) are mid quality and older-generation; the Epic City Sample
Buildings kit is AAA but urban.

## Rejected during visual review

Each preview was inspected. These were cut for quality or fit: Sketchfab
"Antlers" (low quality), "Glenwood" ranch house and "Cape Cod Street"
(block-out level), "Red House" (an apartment interior), "Suburban Street Lamp";
Fab "Soul: Cave" (mobile-optimized), Freeway Props (highway, not suburban),
City Park LITE (duplicate), American Road (covered by MetaRoad), Modern House
Pack (wrong style) and MeikWModels street-prop packs (low quality).

## Next steps

1. Sign in to Fab and add the hero Fab listings to the library. For UE-only
   packs, create a matching-version project and migrate the selected content
   into Wildline.
2. Sign in to Sketchfab and download the hero scans in their original format.
   Record author, license and URL in a `PROVENANCE.md` per map, as the Farm and
   Forest folders do.
3. Plan retopology or decimation for the raw scans: the bear trap, duck decoy,
   rocking chair and mounted deer are 0.9–2.9M faces.
