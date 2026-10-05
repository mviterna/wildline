"""Repair texture persistence and farm lighting after the first rendered review."""
import unreal as ue
from pathlib import Path
eal=ue.EditorAssetLibrary;mel=ue.MaterialEditingLibrary;assets=ue.AssetToolsHelpers.get_asset_tools();base='/Game/Wildline/Farm'
root=Path(ue.Paths.project_dir()).resolve()
for name in ['weathered_brown_planks','red_brick','roof_07','farm_soil']:
    for channel in ['rough','nor_gl']:
        tex=eal.load_asset(base+'/Textures/T_'+name+'_'+channel);tex.set_editor_property('srgb',False)
        if channel=='nor_gl':tex.set_editor_property('compression_settings',ue.TextureCompressionSettings.TC_NORMALMAP);tex.set_editor_property('flip_green_channel',True)
        eal.save_loaded_asset(tex,only_if_is_dirty=False)
for path in eal.list_assets(base+'/Textures',recursive=False):
    if 'Norm' in path:
        tex=eal.load_asset(path);tex.set_editor_property('srgb',False);tex.set_editor_property('flip_green_channel',True);eal.save_loaded_asset(tex,only_if_is_dirty=False)
for name in ['F_Wood','F_Brick','F_Roof','F_Soil']:mel.recompile_material(eal.load_asset(base+'/Materials/'+name));eal.save_asset(base+'/Materials/'+name,only_if_is_dirty=False)
helper=(root/'Scripts/import_forest.py').read_text();exec(helper[helper.index('def node('):helper.index('def wind(')])
m=eal.load_asset(base+'/Materials/F_Corn');m.set_editor_property('two_sided',True);m.set_editor_property('shading_model',ue.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
prop(node(m,ue.MaterialExpressionConstant3Vector,constant=ue.LinearColor(.12,.24,.025,1)),ue.MaterialProperty.MP_SUBSURFACE_COLOR)
p=node(m,ue.MaterialExpressionWorldPosition);t=node(m,ue.MaterialExpressionTime)
prop(custom(m,'float h=saturate((P.z-210)/230);return float3(sin(T*1.2+P.x*.005)*3,cos(T+P.y*.005)*2,0)*h*h;',{'P':p,'T':t}),ue.MaterialProperty.MP_WORLD_POSITION_OFFSET);finish(m)
ue.SystemLibrary.execute_console_command(ue.EditorLevelLibrary.get_editor_world(),'Interchange.FeatureFlags.Import.FBX 0')
opts=ue.FbxImportUI();opts.automated_import_should_detect_type=False;opts.import_as_skeletal=False;opts.import_materials=False;opts.import_textures=False;opts.mesh_type_to_import=ue.FBXImportType.FBXIT_STATIC_MESH;opts.static_mesh_import_data.combine_meshes=True;opts.static_mesh_import_data.auto_generate_collision=False
task=ue.AssetImportTask();task.factory=ue.FbxFactory();task.filename=str(root/'ArtSource/Farm/Export/SM_Corn.fbx');task.destination_path=base+'/Meshes';task.destination_name='SM_Corn';task.options=opts;task.automated=True;task.replace_existing=True;task.save=True;assets.import_asset_tasks([task]);mesh=eal.load_asset(base+'/Meshes/SM_Corn')
for i,slot in enumerate(mesh.static_materials):mesh.set_material(i,eal.load_asset(base+'/Materials/'+str(slot.material_slot_name).split('.')[0]))
settings=mesh.get_editor_property('nanite_settings');settings.set_editor_property('enabled',True);mesh.set_editor_property('nanite_settings',settings);eal.save_loaded_asset(mesh,only_if_is_dirty=False)
levels=ue.get_editor_subsystem(ue.LevelEditorSubsystem);actors=ue.get_editor_subsystem(ue.EditorActorSubsystem);levels.load_level('/Game/Maps/Farmstead')
for actor in actors.get_all_level_actors():
    label=actor.get_actor_label()
    if label=='Late afternoon sun':actor.light_component.set_editor_property('forward_shading_priority',1)
    if label=='Pantry warm lamp':actor.point_light_component.set_editor_property('intensity',50000.);actor.point_light_component.set_editor_property('attenuation_radius',1100.)
    if actor.get_class().get_name()=='WildlineWorld':actor.rebuild()
levels.save_current_level();eal.save_directory(base,only_if_is_dirty=False,recursive=True);ue.log('FARM POLISH READY');ue.SystemLibrary.quit_editor()
