"""Create original waiting-room materials, import the squirrel trophy, and save.
Run with UnrealEditor-Cmd -run=pythonscript -script=<this file>.
Room geometry remains editable in WildlineLobby.cpp; no third-party downloads.
"""
from pathlib import Path
import unreal as ue

base = '/Game/Wildline/Lobby'
eal, mel = ue.EditorAssetLibrary, ue.MaterialEditingLibrary
assets = ue.AssetToolsHelpers.get_asset_tools()
eal.make_directory(base)
palette = {
    'M_Wood': ((.30,.14,.052),.82,0),
    'M_DarkWood': ((.105,.044,.017),.9,0),
    'M_Stone': ((.25,.23,.21),.97,0),
    'M_Brass': ((.52,.30,.083),.35,.7),
    'M_Iron': ((.028,.031,.032),.58,.7),
    'M_Rug': ((.29,.042,.018),1,0),
    'M_Paper': ((.68,.58,.38),.97,0),
    'M_Fur': ((.29,.13,.04),.93,0),
    'M_Leaf': ((.16,.22,.052),.95,0),
    'M_Acorn': ((.44,.22,.065),.6,0),
    'M_PortalGreen': ((.12,.62,.28),.35,0),
    'M_PortalAmber': ((1,.35,.055),.4,0),
}
for name,(rgb,rough,metal) in palette.items():
    m = eal.load_asset(base+'/'+name) if eal.does_asset_exist(base+'/'+name) else assets.create_asset(name,base,ue.Material,ue.MaterialFactoryNew())
    mel.delete_all_material_expressions(m)
    color = mel.create_material_expression(m,ue.MaterialExpressionConstant3Vector)
    color.set_editor_property('constant',ue.LinearColor(*rgb,1))
    if name in ('M_Wood','M_DarkWood','M_Stone','M_Rug'):
        p = mel.create_material_expression(m,ue.MaterialExpressionWorldPosition)
        grain = mel.create_material_expression(m,ue.MaterialExpressionCustom)
        grain.set_editor_property('output_type',ue.CustomMaterialOutputType.CMOT_FLOAT3)
        ins=[]
        for key in ('P','C'):
            inp=ue.CustomInput();inp.set_editor_property('input_name',key);ins.append(inp)
        grain.set_editor_property('inputs',ins)
        if 'Wood' in name:
            code='float g=sin(P.y*.22+sin(P.x*.015)*3+sin(P.z*.09)*2);float fine=sin(P.y*2+sin(P.x*.022)*7);return C*(.82+.16*g+.045*fine);'
        elif name=='M_Stone':
            code='float g=frac(sin(dot(floor(P*.55),float3(12.989,78.233,36.72)))*43758.54);return C*(.65+.6*g);'
        else:
            code='float2 q=floor(P.xy/26);float stripe=fmod(abs(q.x)+abs(q.y),2);float weave=sin(P.x*4)*sin(P.y*4);return lerp(C,C*2.2,stripe*.55)*(1+weave*.05);'
        grain.set_editor_property('code',code)
        mel.connect_material_expressions(p,'',grain,'P');mel.connect_material_expressions(color,'',grain,'C');mel.connect_material_property(grain,'',ue.MaterialProperty.MP_BASE_COLOR)
    else:
        mel.connect_material_property(color,'',ue.MaterialProperty.MP_BASE_COLOR)
    for value,prop in ((rough,ue.MaterialProperty.MP_ROUGHNESS),(metal,ue.MaterialProperty.MP_METALLIC)):
        c=mel.create_material_expression(m,ue.MaterialExpressionConstant);c.set_editor_property('r',value);mel.connect_material_property(c,'',prop)
    if name.startswith('M_Portal'):
        mult=mel.create_material_expression(m,ue.MaterialExpressionMultiply);mult.set_editor_property('const_b',60)
        mel.connect_material_expressions(color,'',mult,'A');mel.connect_material_property(mult,'',ue.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(m);eal.save_loaded_asset(m,only_if_is_dirty=False)

name='M_Sign'
# Preserve the engine's distance-field font mask and font-page parameters.
# Recreate this generated material to keep repeated runs from stacking gain nodes.
if eal.does_asset_exist(base+'/'+name):
    assert eal.delete_asset(base+'/'+name)
assert eal.duplicate_asset('/Engine/EngineMaterials/DefaultTextMaterialOpaque',base+'/'+name)
m=eal.load_asset(base+'/'+name)
prop=ue.MaterialProperty.MP_EMISSIVE_COLOR
original=mel.get_material_property_input_node(m,prop)
if original is None:
    prop=ue.MaterialProperty.MP_BASE_COLOR
    original=mel.get_material_property_input_node(m,prop)
assert original is not None,'Default text material has no color input'
output=mel.get_material_property_input_node_output_name(m,prop)
mult=mel.create_material_expression(m,ue.MaterialExpressionMultiply);mult.set_editor_property('const_b',40)
assert mel.connect_material_expressions(original,output,mult,'A')
assert mel.connect_material_property(mult,'',ue.MaterialProperty.MP_EMISSIVE_COLOR)
m.set_editor_property('shading_model',ue.MaterialShadingModel.MSM_UNLIT)
mel.recompile_material(m);eal.save_loaded_asset(m,only_if_is_dirty=False)

source=Path(ue.Paths.project_dir()).resolve()/'ArtSource/Lobby/SquirrelTrophy.fbx'
if source.exists():
    options=ue.FbxImportUI();options.set_editor_property('import_as_skeletal',False);options.set_editor_property('import_materials',False);options.set_editor_property('import_textures',False)
    task=ue.AssetImportTask();task.set_editor_property('filename',str(source));task.set_editor_property('destination_path',base);task.set_editor_property('destination_name','SM_SquirrelTrophy');task.set_editor_property('automated',True);task.set_editor_property('replace_existing',True);task.set_editor_property('save',True);task.set_editor_property('options',options)
    assets.import_asset_tasks([task])
    mesh=eal.load_asset(base+'/SM_SquirrelTrophy')
    if mesh:
        for i,slot in enumerate(mesh.get_editor_property('static_materials')):
            material=eal.load_asset('/Game/Wildline/Squirrel/M_SquirrelEye' if 'Eye' in str(slot.material_slot_name) else '/Game/Wildline/Squirrel/M_SquirrelCoat')
            mesh.set_material(i,material)
        eal.save_loaded_asset(mesh,only_if_is_dirty=False)
eal.save_directory(base,only_if_is_dirty=False)
ue.log('WILDLINE LOBBY CONTENT READY')
