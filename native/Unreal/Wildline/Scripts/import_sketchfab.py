"""Import the adapted, attributed Sketchfab models with their PBR materials."""
import unreal as ue, shutil
from pathlib import Path
root=Path(ue.Paths.project_dir()).resolve();src=root/'ArtSource/Farm/Sketchfab';base='/Game/Wildline/Farm'
assets=ue.AssetToolsHelpers.get_asset_tools();eal=ue.EditorAssetLibrary;mel=ue.MaterialEditingLibrary
helper=(root/'Scripts/import_forest.py').read_text();exec(helper[helper.index('def node('):helper.index('def wind(')])
def tex(file,name,normal=False,linear=False):
    task=ue.AssetImportTask();task.filename=str(file);task.destination_path=base+'/Textures';task.destination_name=name;task.automated=True;task.replace_existing=True;task.save=True;assets.import_asset_tasks([task]);t=eal.load_asset(base+'/Textures/'+name);assert t,file
    t.set_editor_property('max_texture_size',2048)
    if normal:t.set_editor_property('compression_settings',ue.TextureCompressionSettings.TC_NORMALMAP);t.set_editor_property('flip_green_channel',True)
    t.set_editor_property('srgb',not (normal or linear));eal.save_loaded_asset(t,only_if_is_dirty=False);return t
for name,folder,prefix,channels in [
    ('F_SketchHouse1','Farmhouse','house_1',{'Color':'albedo.jpeg','Normal':'normal.png','Roughness':'roughness.jpeg','Metallic':'metallic.jpeg','AO':'AO.jpeg'}),
    ('F_SketchHouse2','Farmhouse','House_2',{'Color':'albedo.jpeg','Normal':'normal.png','Roughness':'roughness.jpeg','Metallic':'metallic.jpeg','AO':'AO.jpeg'}),
    ('F_SketchShotgun','Shotgun','shotgun_l_MainMat.006',{'Color':'BaseColor.png','Normal':'Normal.png','Roughness':'Roughness.png','Metallic':'Metallic.png','AO':'AO.png'})]:
    m=material(name);mel.delete_all_material_expressions(m);m.set_editor_property('two_sided',name!='F_SketchShotgun')
    for channel,file in channels.items():
        t=tex(src/folder/'textures'/(prefix+'_'+file),'T_'+name[2:]+'_'+channel,channel=='Normal',channel not in ['Color','Normal']);n=sample(m,t)
        prop(n,{'Color':ue.MaterialProperty.MP_BASE_COLOR,'Normal':ue.MaterialProperty.MP_NORMAL,'Roughness':ue.MaterialProperty.MP_ROUGHNESS,'Metallic':ue.MaterialProperty.MP_METALLIC,'AO':ue.MaterialProperty.MP_AMBIENT_OCCLUSION}[channel],'RGB' if channel in ['Color','Normal'] else 'R')
    finish(m)
ue.SystemLibrary.execute_console_command(ue.EditorLevelLibrary.get_editor_world(),'Interchange.FeatureFlags.Import.FBX 0')
for name in ['SM_Farmhouse','SM_Shotgun','SM_ShotgunAction']:
    # Reimporting the former procedural models retains their section/material
    # overrides. Recreate these three adapted meshes from their source FBX so
    # floorboards, furniture and weapon PBR channels use the source slot layout.
    old=root/'Content/Wildline/Farm/Meshes'/(name+'.uasset');backup=src/'ImportBackup';backup.mkdir(exist_ok=True)
    if old.exists() and not (backup/old.name).exists():shutil.copy2(old,backup/old.name)
    if eal.does_asset_exist(base+'/Meshes/'+name):assert eal.delete_asset(base+'/Meshes/'+name),name
    opts=ue.FbxImportUI();opts.automated_import_should_detect_type=False;opts.import_as_skeletal=False;opts.import_materials=False;opts.import_textures=False;opts.mesh_type_to_import=ue.FBXImportType.FBXIT_STATIC_MESH
    opts.static_mesh_import_data.combine_meshes=True;opts.static_mesh_import_data.auto_generate_collision=False;opts.static_mesh_import_data.normal_import_method=ue.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS
    task=ue.AssetImportTask();task.factory=ue.FbxFactory();task.filename=str(root/'ArtSource/Farm/Export'/(name+'.fbx'));task.destination_path=base+'/Meshes';task.destination_name=name;task.options=opts;task.automated=True;task.replace_existing=True;task.replace_existing_settings=True;task.save=True;assets.import_asset_tasks([task]);mesh=eal.load_asset(base+'/Meshes/'+name);assert mesh,name
    for i,slot in enumerate(mesh.static_materials):
        key=str(slot.material_slot_name).split('.')[0];mat=eal.load_asset(('/Game/Wildline/Hunters/Materials/' if key.startswith('R_') else base+'/Materials/')+key);assert mat,(name,key);mesh.set_material(i,mat)
    ue.log('SKETCHFAB SLOTS '+name+' '+str([str(s.material_slot_name) for s in mesh.static_materials]))
    if name=='SM_Farmhouse':
        body=mesh.get_editor_property('body_setup');body.set_editor_property('collision_trace_flag',ue.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE);body.set_editor_property('double_sided_geometry',True)
    eal.save_loaded_asset(mesh,only_if_is_dirty=False)
levels=ue.get_editor_subsystem(ue.LevelEditorSubsystem);actors=ue.get_editor_subsystem(ue.EditorActorSubsystem);levels.load_level('/Game/Maps/Farmstead')
for actor in actors.get_all_level_actors():
    if actor.get_class().get_name()=='WildlineWorld':actor.rebuild()
levels.save_current_level();eal.save_directory(base,only_if_is_dirty=False,recursive=True);ue.log('SKETCHFAB CONTENT READY');ue.SystemLibrary.quit_editor()
