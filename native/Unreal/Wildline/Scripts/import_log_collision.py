import unreal as ue
from pathlib import Path
root=Path(ue.Paths.project_dir()).resolve()
# The legacy FBX path imports UCX convex meshes consistently in this engine build.
ue.SystemLibrary.execute_console_command(ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world(),'Interchange.FeatureFlags.Import.FBX 0')
opts=ue.FbxImportUI();opts.automated_import_should_detect_type=False;opts.import_as_skeletal=False;opts.mesh_type_to_import=ue.FBXImportType.FBXIT_STATIC_MESH;opts.import_materials=False;opts.import_textures=False;opts.static_mesh_import_data.combine_meshes=True;opts.static_mesh_import_data.auto_generate_collision=False;opts.static_mesh_import_data.one_convex_hull_per_ucx=True
t=ue.AssetImportTask();t.filename=str(root/'ArtSource/Forest/Export/SM_Log.fbx');t.destination_path='/Game/Wildline/Forest/Meshes';t.destination_name='SM_Log';t.options=opts;t.factory=ue.FbxFactory();t.automated=True;t.replace_existing=True;t.save=True;ue.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
eal=ue.EditorAssetLibrary;log=eal.load_asset('/Game/Wildline/Forest/Meshes/SM_Log');log.set_material(0,eal.load_asset('/Game/Wildline/Forest/Materials/M_TreeBark'));log.set_material(1,eal.load_asset('/Game/Wildline/Forest/Materials/M_Heartwood'));eal.save_loaded_asset(log)
count=ue.EditorStaticMeshLibrary.get_convex_collision_count(log);ue.log('LOG COLLISION HULLS '+str(count));assert count>=12
ue.SystemLibrary.quit_editor()
