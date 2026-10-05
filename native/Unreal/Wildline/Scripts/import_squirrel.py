"""Import the publisher's free rigged squirrel FBX and inspect its physical rig."""
from pathlib import Path
import json
import unreal as ue

root = Path(ue.Paths.project_dir()).resolve()
source = root / 'ArtSource/Squirrel'
assets = ue.AssetToolsHelpers.get_asset_tools()
options = ue.FbxImportUI()
options.set_editor_property('automated_import_should_detect_type', False)
options.set_editor_property('import_as_skeletal', True)
options.set_editor_property('mesh_type_to_import', ue.FBXImportType.FBXIT_SKELETAL_MESH)
options.set_editor_property('import_mesh', True)
options.set_editor_property('import_animations', True)
options.set_editor_property('import_materials', True)
options.set_editor_property('import_textures', True)
options.set_editor_property('create_physics_asset', True)
task = ue.AssetImportTask()
task.set_editor_property('filename', str(source / ('Squirrel Smooth.fbx' if (source/'Squirrel Smooth.fbx').exists() else 'Rigged Squirrel.fbx')))
task.set_editor_property('destination_path', '/Game/Wildline/Squirrel')
task.set_editor_property('destination_name', 'SK_Squirrel')
task.set_editor_property('automated', True)
task.set_editor_property('replace_existing', True)
task.set_editor_property('save', True)
task.set_editor_property('options', options)
assets.import_asset_tasks([task])
ue.log('SQUIRREL IMPORTED: '+str(task.get_editor_property('imported_object_paths')))
paths = [str(p) for p in ue.EditorAssetLibrary.list_assets('/Game/Wildline/Squirrel', recursive=True, include_folder=False)]
result = {'assets': paths}
for path in paths:
    asset = ue.EditorAssetLibrary.load_asset(path)
    if isinstance(asset, ue.SkeletalMesh):
        actor=ue.get_editor_subsystem(ue.EditorActorSubsystem).spawn_actor_from_class(ue.SkeletalMeshActor,ue.Vector(0,0,0))
        component=actor.skeletal_mesh_component
        component.set_skeletal_mesh_asset(asset)
        origin,extent=actor.get_actor_bounds(False)
        result['mesh']=path
        result['bounds_origin']=[origin.x,origin.y,origin.z]
        result['bounds_extent']=[extent.x,extent.y,extent.z]
        result['bones']={str(name):[component.get_socket_location(name).x,component.get_socket_location(name).y,component.get_socket_location(name).z] for name in component.get_all_socket_names()}
        result['materials']=[str(m.material_slot_name) for m in asset.get_editor_property('materials')]
        ue.get_editor_subsystem(ue.EditorActorSubsystem).destroy_actor(actor)
ue.EditorAssetLibrary.save_directory('/Game/Wildline/Squirrel',only_if_is_dirty=False)
(root/'ArtSource/Squirrel/rig-info.json').write_text(json.dumps(result,indent=2))
ue.log('SQUIRREL RIG INFO: '+json.dumps(result))
