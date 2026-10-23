"""Import actual map captures into the map guide, without resaving map assets."""
from pathlib import Path
import unreal as ue
root=Path(ue.Paths.project_dir()).resolve();assets=ue.AssetToolsHelpers.get_asset_tools();eal=ue.EditorAssetLibrary
known = ['Blackwood','Frostwood','Farmstead','Neighborhood','NeighborhoodCedar','TaylorsvilleLake','KnobsStateForest']
selected = next((part.split('=', 1)[1] for part in ue.SystemLibrary.get_command_line().split() if part.startswith('-WildlinePreviewMaps=')), None)
names = selected.split(',') if selected else known
assert all(name in known for name in names), names
for name in names:
    file=root/'ArtSource/Lake/Previews'/(name+'.png');assert file.exists(),file
    t=ue.AssetImportTask();t.filename=str(file);t.destination_path='/Game/Wildline/Lake/Previews';t.destination_name='T_Map_'+name;t.automated=True;t.replace_existing=True;t.save=True;assets.import_asset_tasks([t]);tex=eal.load_asset(t.destination_path+'/'+t.destination_name);assert tex,name
    tex.set_editor_property('lod_group',ue.TextureGroup.TEXTUREGROUP_UI);tex.set_editor_property('max_texture_size',1024);eal.save_loaded_asset(tex,only_if_is_dirty=False)
ue.log('MAP PREVIEWS IMPORT PASS');ue.SystemLibrary.quit_editor()
