"""Import corrected trophy variants and coat materials without changing players."""
from pathlib import Path
import unreal as ue
base='/Game/Wildline/Lobby'
src=Path(ue.Paths.project_dir()).resolve()/'ArtSource/Lobby'
eal=ue.EditorAssetLibrary;mel=ue.MaterialEditingLibrary
assets=ue.AssetToolsHelpers.get_asset_tools()
ue.SystemLibrary.execute_console_command(ue.EditorLevelLibrary.get_editor_world(),'Interchange.FeatureFlags.Import.FBX 0')
def node(m,cls,**props):
    n=mel.create_material_expression(m,cls)
    for k,v in props.items():n.set_editor_property(k,v)
    return n
coats={
    'Grey':(.50,.54,.58),
    'Fox':(.95,.43,.14),
    'Red':(.64,.19,.055),
    'Black':(.038,.046,.052),
}
name='M_TrophyNose'
m=eal.load_asset(base+'/'+name) if eal.does_asset_exist(base+'/'+name) else assets.create_asset(name,base,ue.Material,ue.MaterialFactoryNew())
mel.delete_all_material_expressions(m)
mel.connect_material_property(node(m,ue.MaterialExpressionConstant3Vector,constant=ue.LinearColor(.016,.011,.009,1)),'',ue.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(node(m,ue.MaterialExpressionConstant,r=.38),'',ue.MaterialProperty.MP_ROUGHNESS)
mel.recompile_material(m);eal.save_loaded_asset(m,only_if_is_dirty=False)
for species,tint in coats.items():
    name='M_Trophy'+species
    m=eal.load_asset(base+'/'+name) if eal.does_asset_exist(base+'/'+name) else assets.create_asset(name,base,ue.Material,ue.MaterialFactoryNew())
    mel.delete_all_material_expressions(m)
    tex=node(m,ue.MaterialExpressionTextureSample,texture=eal.load_asset('/Game/Wildline/Squirrel/Squirrel'))
    custom=node(m,ue.MaterialExpressionCustom,output_type=ue.CustomMaterialOutputType.CMOT_FLOAT3)
    inp=ue.CustomInput();inp.set_editor_property('input_name','C');custom.set_editor_property('inputs',[inp])
    # The source hair variation remains visible. Bright eye rings/muzzle stay
    # pale on the three usual coats; the melanistic grey has charcoal markings.
    pale=(.17,.19,.21) if species=='Black' else (.76,.71,.60)
    custom.set_editor_property('code',
        'float l=dot(C,float3(.299,.587,.114));'
        'float pale=smoothstep(.39,.69,l);'
        f'return lerp(float3({tint[0]},{tint[1]},{tint[2]})*(.26+l*1.8),'
        f'float3({pale[0]},{pale[1]},{pale[2]})*(.6+l),pale);')
    mel.connect_material_expressions(tex,'RGB',custom,'C');mel.connect_material_property(custom,'',ue.MaterialProperty.MP_BASE_COLOR)
    for value,prop in [(.9,ue.MaterialProperty.MP_ROUGHNESS),(.22,ue.MaterialProperty.MP_SPECULAR)]:
        mel.connect_material_property(node(m,ue.MaterialExpressionConstant,r=value),'',prop)
    mel.recompile_material(m);eal.save_loaded_asset(m,only_if_is_dirty=False)

for file,species in [('SquirrelTrophy','Grey'),('SquirrelTrophyFox','Fox'),('SquirrelTrophyRed','Red'),('SquirrelTrophyBlack','Black')]:
    options=ue.FbxImportUI();options.automated_import_should_detect_type=False
    options.import_as_skeletal=False;options.import_materials=False;options.import_textures=False;options.import_animations=False
    options.mesh_type_to_import=ue.FBXImportType.FBXIT_STATIC_MESH
    options.static_mesh_import_data.combine_meshes=True;options.static_mesh_import_data.auto_generate_collision=False
    options.static_mesh_import_data.normal_import_method=ue.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS
    task=ue.AssetImportTask();task.factory=ue.FbxFactory();task.filename=str(src/(file+'.fbx'))
    task.destination_path=base;task.destination_name='SM_'+file;task.automated=True
    task.replace_existing=True;task.replace_existing_settings=True;task.options=options;task.save=True
    assets.import_asset_tasks([task]);mesh=eal.load_asset(base+'/SM_'+file);assert mesh,file
    for i,slot in enumerate(mesh.static_materials):
        key=str(slot.material_slot_name)
        path=base+'/M_TrophyNose' if 'TrophyNose' in key else '/Game/Wildline/Squirrel/M_SquirrelCornea' if 'Eye' in key else base+'/M_Trophy'+species
        material=eal.load_asset(path);assert material,path;mesh.set_material(i,material)
    eal.save_loaded_asset(mesh,only_if_is_dirty=False)
    ue.log('LODGE TROPHY READY '+file+' '+str(mesh.get_bounds().box_extent))
ue.SystemLibrary.quit_editor()
