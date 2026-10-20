"""Import the poster, bluegrass loop, studio logo and 2D squirrel animation atlas.

Run prepare_presentation.py first, then UnrealEditor-Cmd -run=pythonscript
-script=<this file>. Assets are explicitly cooked under /Game/Wildline.
"""
from pathlib import Path
import unreal as ue

root = Path(ue.Paths.project_dir()).resolve()
tools = ue.AssetToolsHelpers.get_asset_tools()
for filename, name in [('MenuPoster.png', 'T_MenuPoster'),
                       ('MenuBluegrass.wav', 'SW_MenuBluegrass'),
                       ('MenuRap.wav', 'SW_MenuRap'),
                       ('StudioLogo.png', 'T_StudioLogo'),
                       ('IntroSquirrelAtlas.png', 'T_IntroSquirrelAtlas')]:
    task = ue.AssetImportTask()
    task.filename = str(root / 'ArtSource/Presentation' / filename)
    task.destination_path = '/Game/Wildline/Presentation'
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = True
    tools.import_asset_tasks([task])
    asset = ue.EditorAssetLibrary.load_asset(f'/Game/Wildline/Presentation/{name}')
    if not asset:
        raise RuntimeError(f'Presentation import failed: {filename}')
    if isinstance(asset, ue.Texture2D):
        asset.set_editor_property('compression_settings', ue.TextureCompressionSettings.TC_EDITOR_ICON)
        asset.set_editor_property('lod_group', ue.TextureGroup.TEXTUREGROUP_UI)
        asset.set_editor_property('mip_gen_settings', ue.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        asset.set_editor_property('srgb', True)
    else:
        asset.set_editor_property('looping', True)
        asset.set_editor_property('volume', 1.0)
        asset.set_editor_property('sound_group', ue.SoundGroup.SOUNDGROUP_MUSIC)
    ue.EditorAssetLibrary.save_loaded_asset(asset)
ue.log('WILDLINE PRESENTATION IMPORT: PASS / poster, bluegrass, studio logo and 2D squirrel atlas')
