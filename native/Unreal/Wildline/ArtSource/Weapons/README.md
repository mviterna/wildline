# Weapon assets — integration TODO

Copied from the user's `C:/Work/GameAssets` folder on 2026-10-05. These assets
are retained in the repository for future integration. They are not imported
into Unreal Content or selected by the running game yet.

`Rifles/` preserves the supplied directory layout: exported FBX meshes at its
root, texture maps in `Textures/`, reference renders in `Previews/`, and editable
Blender files and authoring scripts in `src/`. Binary files use Git LFS. The
existing ignore rule excludes the redundant `.blend1` backup from Git.

| Asset | Export | Status |
|---|---|---|
| M64 Kryptek-style rifle | `Rifles/SM_Rifle_M64_Kryptek.fbx` | TODO |
| Blue Rascal rifle | `Rifles/SM_Rifle_Rascal_Blue.fbx` | TODO |
| Red Rascal rifle | `Rifles/SM_Rifle_Rascal_Red.fbx` | TODO |
| Pardner .410 shotgun | `Rifles/SM_Shotgun_Pardner_410.fbx` | TODO |
| Stevens 940D shotgun | `Rifles/SM_Shotgun_Stevens_940D.fbx` | TODO |
| 3–9x40 scope | `Rifles/SM_Scope_3-9x40.fbx` | TODO |
| HS219 Hydra scope | `Rifles/SM_Scope_HS219_Hydra.fbx` | TODO |

## TODO: add to Wildline

- [ ] Import the seven FBX files and their materials into `/Game/Wildline/Weapons`;
  check scale, orientation, normals, collision, texture references and LODs.
- [ ] Create shotgun PBR materials using base color as sRGB, normal maps as
  DirectX tangent-space normals, and linear ORM maps with R = ambient occlusion,
  G = roughness and B = metallic.
- [ ] Add weapon/scope selection and weapon-specific definitions for stock,
  trigger hand, support hand, muzzle, action and optic attachment transforms.
- [ ] Fit shoulder contact and both hand targets to each stock across male,
  female and slim hunter profiles, including crouch and vertical aim.
- [ ] Add appropriate firing, action cycling and reload animations for each
  weapon. The supplied shotgun script describes single-shot break-action guns;
  they need their own opening/loading/closing sequence and gameplay state.
- [ ] Align scope eye position, sight picture and muzzle effects; verify
  first-person and third-person views and existing thermal-sight rules.
- [ ] Verify switching, ammo, reload, recoil and replicated weapon/scope choices
  in solo, co-op and late-join tests, then capture the finished handling.

## Source notes

The supplied Python scripts describe procedural Blender geometry and materials.
`make_camo.py` generates a Kryptek-Highlander-style texture. This records the
supplied asset names and source descriptions without assigning a new license;
no separate license file was included in the provided folder.

The scripts describe metres in Blender, centimetre FBX export, +X towards the
muzzle, +Z up, Blender -Y towards the weapon's right side, and an origin at the
trigger. Confirm those conventions on import. The shotgun material bakes use
4096-pixel Wood and Metal texture sets. Keep the provided scripts as authoring
sources; copying them into the repository does not run them.

`ASSET_MANIFEST.json` records each included source file's relative path, byte
size and SHA-256 checksum. All copied files were compared with the originals.

On 2026-10-07 the Pardner .410 was rebuilt here from a revised
`build_shotgun.py`. Its frame, trigger guard, forend and stock profile now
match photographs of an NEF Pardner SB1: a longer frame with the guard under it,
a straight comb and a shallow semi-pistol grip instead of a deep knob. Its
manifest entries describe the rebuilt files, which no longer match the copies
in `C:/Work/GameAssets`. The script needs Blender 5 (it uses 5.x shader socket
names); the Blender 4.5 under `native/AssetDownloads` fails on it. Rebuild with
`blender -b -P build_shotgun.py -- <Rifles dir> pardner`.
