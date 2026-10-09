"""Import the requested concrete steps as non-looping spatial sound waves."""
from pathlib import Path
import unreal as ue

root = Path(ue.Paths.project_dir()).resolve()
tools = ue.AssetToolsHelpers.get_asset_tools()
for index in range(1, 5):
    name = f'SW_Footstep{index:02}'
    task = ue.AssetImportTask()
    task.filename = str(root / f'ArtSource/Audio/Footstep{index:02}.wav')
    task.destination_path = '/Game/Wildline/Audio'
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = True
    tools.import_asset_tasks([task])
    asset = ue.EditorAssetLibrary.load_asset(f'/Game/Wildline/Audio/{name}')
    assert isinstance(asset, ue.SoundWave), name
    asset.set_editor_property('looping', False)
    asset.set_editor_property('sound_group', ue.SoundGroup.SOUNDGROUP_EFFECTS)
    asset.set_editor_property('volume', 1.0)
    assert ue.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
    ue.log('FOOTSTEP IMPORT PASS ' + name)
