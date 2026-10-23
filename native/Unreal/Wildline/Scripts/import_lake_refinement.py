"""Import bass PBR maps and rod meshes; regenerate the expanded saved lake."""
from pathlib import Path
import unreal as ue
root=Path(ue.Paths.project_dir()).resolve();folder=root/'ArtSource/Lake';base='/Game/Wildline/Lake'
original=(root/'Scripts/import_lake_assets.py').read_text();exec(compile(original[:original.index('meshes=')],'lake_materials','exec'))
solid('M_ReelBrass',(.33,.26,.12),.27,.75);solid('M_LineSpool',(.63,.68,.54),.35,0)
maps={}
for file in (folder/'Fish/Embedded').glob('*.png'):
    key=file.stem
    task=ue.AssetImportTask();task.filename=str(file);task.destination_path=base+'/Textures';task.destination_name='T_Bass_'+key;task.automated=True;task.replace_existing=True;task.save=True;assets.import_asset_tasks([task]);tex=eal.load_asset(task.destination_path+'/'+task.destination_name);assert tex
    if key=='Normal':tex.set_editor_property('compression_settings',ue.TextureCompressionSettings.TC_NORMALMAP);tex.set_editor_property('flip_green_channel',True)
    if key!='Diffuse':tex.set_editor_property('srgb',False)
    tex.set_editor_property('max_texture_size',2048);eal.save_loaded_asset(tex,only_if_is_dirty=False);maps[key]=tex
assert len(maps)==4,maps
m=material('M_PhotorealBass');mel.delete_all_material_expressions(m);m.set_editor_property('two_sided',True)
prop(sample(m,maps['Diffuse']),ue.MaterialProperty.MP_BASE_COLOR,'RGB');prop(sample(m,maps['Normal']),ue.MaterialProperty.MP_NORMAL,'RGB');prop(sample(m,maps['Roughness']),ue.MaterialProperty.MP_ROUGHNESS,'R');constant(m,.07,ue.MaterialProperty.MP_METALLIC)
# Deform the tail along the mesh's local side axis; keep the head stable.
position=node(m,ue.MaterialExpressionWorldPosition);local=node(m,ue.MaterialExpressionTransformPosition,transform_source_type=ue.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_WORLD,transform_type=ue.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_LOCAL);assert mel.connect_material_expressions(position,'',local,'')
time=node(m,ue.MaterialExpressionTime)
offset=custom(m,'float tail=saturate((-P.x+4)/27);return float3(0,sin(T*6.2+P.x*.10)*tail*tail*3.8,0);',{'P':local,'T':time})
world=node(m,ue.MaterialExpressionTransform,transform_source_type=ue.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_LOCAL,transform_type=ue.MaterialVectorCoordTransform.TRANSFORM_WORLD);assert mel.connect_material_expressions(offset,'',world,'');prop(world,ue.MaterialProperty.MP_WORLD_POSITION_OFFSET);finish(m)
for name in ['SM_FishingRod','SM_FishingReelCrank','SM_PhotorealBass']:
    opt=ue.FbxImportUI();opt.automated_import_should_detect_type=False;opt.import_as_skeletal=False;opt.mesh_type_to_import=ue.FBXImportType.FBXIT_STATIC_MESH;opt.import_materials=False;opt.import_textures=False;opt.import_animations=False;opt.static_mesh_import_data.combine_meshes=True;opt.static_mesh_import_data.auto_generate_collision=False
    task=ue.AssetImportTask();task.filename=str(folder/'Export'/(name+'.fbx'));task.destination_path=base+'/Meshes';task.destination_name=name;task.automated=True;task.replace_existing=True;task.save=True;task.options=opt;assets.import_asset_tasks([task]);mesh=eal.load_asset(base+'/Meshes/'+name);assert mesh
    for i,slot in enumerate(mesh.get_editor_property('static_materials')):
        key=str(slot.material_slot_name).split('.')[0];mat=eal.load_asset(base+'/Materials/'+key);assert mat,key;mesh.set_material(i,mat)
    eal.save_loaded_asset(mesh,only_if_is_dirty=False)
levels=ue.get_editor_subsystem(ue.LevelEditorSubsystem);assert levels.load_level('/Game/Maps/TaylorsvilleLake')
for actor in ue.get_editor_subsystem(ue.EditorActorSubsystem).get_all_level_actors():
    if actor.get_class().get_name()=='WildlineWorld':actor.rebuild()
assert levels.save_current_level();ue.log('REFINED LAKE IMPORT PASS');ue.SystemLibrary.quit_editor()
