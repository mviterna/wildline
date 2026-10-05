import unreal as ue
from pathlib import Path
root=Path(ue.Paths.project_dir()).resolve();base='/Game/Wildline/Farm';eal=ue.EditorAssetLibrary;mel=ue.MaterialEditingLibrary;assets=ue.AssetToolsHelpers.get_asset_tools()
helper=(root/'Scripts/import_forest.py').read_text();exec(helper[helper.index('def node('):helper.index('def wind(')])
m=eal.load_asset(base+'/Materials/F_Peanut');uv=node(m,ue.MaterialExpressionTextureCoordinate)
prop(custom(m,'float ribs=sin(P.y*75.398);float pits=sin(P.x*173+sin(P.y*53)*2)*sin(P.y*157);return float3(.32,.16,.055)*(.88+.08*ribs+.05*pits);',{'P':uv}),ue.MaterialProperty.MP_BASE_COLOR);constant(m,.96,ue.MaterialProperty.MP_ROUGHNESS);finish(m)
ue.SystemLibrary.execute_console_command(ue.EditorLevelLibrary.get_editor_world(),'Interchange.FeatureFlags.Import.FBX 0')
opts=ue.FbxImportUI();opts.automated_import_should_detect_type=False;opts.import_as_skeletal=False;opts.import_materials=False;opts.import_textures=False;opts.mesh_type_to_import=ue.FBXImportType.FBXIT_STATIC_MESH;opts.static_mesh_import_data.combine_meshes=True;opts.static_mesh_import_data.auto_generate_collision=False
task=ue.AssetImportTask();task.factory=ue.FbxFactory();task.filename=str(root/'ArtSource/Farm/Export/SM_Peanut.fbx');task.destination_path=base+'/Meshes';task.destination_name='SM_Peanut';task.options=opts;task.automated=True;task.replace_existing=True;task.save=True;assets.import_asset_tasks([task]);mesh=eal.load_asset(base+'/Meshes/SM_Peanut');mesh.set_material(0,m);eal.save_loaded_asset(mesh,only_if_is_dirty=False)
ue.log('PEANUT SHELL REFINEMENT READY');ue.SystemLibrary.quit_editor()
