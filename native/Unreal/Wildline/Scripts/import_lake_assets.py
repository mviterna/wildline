"""Build lake materials, import original models, and author the cooked map."""
from pathlib import Path
import unreal as ue, json
root=Path(ue.Paths.project_dir()).resolve();assets=ue.AssetToolsHelpers.get_asset_tools();eal=ue.EditorAssetLibrary;mel=ue.MaterialEditingLibrary
base='/Game/Wildline/Lake';folder=root/'ArtSource/Lake'
helpers=(root/'Scripts/import_forest.py').read_text();exec(helpers[helpers.index('def node('):helpers.index('for name,dry in')])
def finish(m):
    mel.set_material_usage(m,ue.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES);mel.recompile_material(m);eal.save_loaded_asset(m,only_if_is_dirty=False)
def solid(name,color,rough=.65,metal=0):
    m=material(name);mel.delete_all_material_expressions(m);c=node(m,ue.MaterialExpressionConstant3Vector,constant=ue.LinearColor(*color));prop(c,ue.MaterialProperty.MP_BASE_COLOR);constant(m,rough,ue.MaterialProperty.MP_ROUGHNESS);constant(m,metal,ue.MaterialProperty.MP_METALLIC);finish(m);return m
for name,color,rough,metal in [('M_BoatGreen',(.085,.15,.10),.6,0),('M_LakeMetal',(.11,.13,.12),.28,.7),('M_Bobber',(.95,.16,.025),.28,0),('M_FishFin',(.18,.24,.10),.45,0),('M_FishEye',(.004,.006,.003),.09,0)]:solid(name,color,rough,metal)
m=material('M_FishScales');mel.delete_all_material_expressions(m);p=node(m,ue.MaterialExpressionWorldPosition)
prop(custom(m,'float grain=sin(P.x*2.1+sin(P.z*3.8))*sin(P.z*3.8);return lerp(float3(.22,.28,.11),float3(.46,.51,.31),grain*.18+.45);',{'P':p}),ue.MaterialProperty.MP_BASE_COLOR);constant(m,.30,ue.MaterialProperty.MP_ROUGHNESS);constant(m,.18,ue.MaterialProperty.MP_METALLIC);finish(m)
for name,c in [('M_DockWood',(.65,.57,.44)),('M_LakeBed',(.32,.39,.27)),('M_LakeTrail',(.58,.52,.38))]:
    m=material(name);mel.delete_all_material_expressions(m);p=node(m,ue.MaterialExpressionWorldPosition)
    uv=custom(m,'return P.xy/180;',{'P':p},ue.CustomMaterialOutputType.CMOT_FLOAT2)
    texture='/Game/Wildline/Lobby/Photoreal/Textures/T_wood_floor_worn_diff_4k' if name=='M_DockWood' else '/Game/Wildline/Forest/Textures/T_Litter'
    normal='/Game/Wildline/Lobby/Photoreal/Textures/T_wood_floor_worn_nor_gl_4k' if name=='M_DockWood' else '/Game/Wildline/Forest/Textures/T_LitterNormal'
    color=sample(m,eal.load_asset(texture),uv);prop(custom(m,f'return C*float3{c};',{'C':(color,'RGB')}),ue.MaterialProperty.MP_BASE_COLOR);prop(sample(m,eal.load_asset(normal),uv),ue.MaterialProperty.MP_NORMAL,'RGB');constant(m,.89,ue.MaterialProperty.MP_ROUGHNESS);finish(m)
m=material('M_LakeWater');mel.delete_all_material_expressions(m);m.set_editor_property('blend_mode',ue.BlendMode.BLEND_TRANSLUCENT);m.set_editor_property('two_sided',True)
m.set_editor_property('translucency_lighting_mode',next(getattr(ue.TranslucencyLightingMode,k) for k in dir(ue.TranslucencyLightingMode) if 'SURFACE' in k and 'PIXEL' in k))
p=node(m,ue.MaterialExpressionWorldPosition);t=node(m,ue.MaterialExpressionTime);fresnel=node(m,ue.MaterialExpressionFresnel,exponent=4.0,base_reflect_fraction=.035)
prop(custom(m,'return lerp(float3(.012,.065,.055),float3(.07,.17,.16),F);',{'F':fresnel}),ue.MaterialProperty.MP_BASE_COLOR)
water_normal=eal.load_asset('/Game/CityPark/Textures/BaseTextures/T_Water_N');assert water_normal
uv_a=custom(m,'return P.xy/850+float2(T*.010,T*.006);',{'P':p,'T':t},ue.CustomMaterialOutputType.CMOT_FLOAT2)
uv_b=custom(m,'return float2(-P.y,P.x)/1330+float2(-T*.007,T*.009);',{'P':p,'T':t},ue.CustomMaterialOutputType.CMOT_FLOAT2)
n_a=sample(m,water_normal,uv_a);n_b=sample(m,water_normal,uv_b)
prop(custom(m,'return normalize(float3((A.xy+B.xy)*.23,1));',{'A':(n_a,'RGB'),'B':(n_b,'RGB')}),ue.MaterialProperty.MP_NORMAL)
prop(custom(m,'return float3(0,0,sin(P.x*.009+T*1.2)*sin(P.y*.013+T*.8)*1.7);',{'P':p,'T':t}),ue.MaterialProperty.MP_WORLD_POSITION_OFFSET)
fade=node(m,ue.MaterialExpressionDepthFade,fade_distance_default=90,opacity_default=.92);prop(fade,ue.MaterialProperty.MP_OPACITY);constant(m,.18,ue.MaterialProperty.MP_ROUGHNESS);constant(m,.85,ue.MaterialProperty.MP_SPECULAR);constant(m,1.015,ue.MaterialProperty.MP_REFRACTION);finish(m)

meshes=ue.get_editor_subsystem(ue.StaticMeshEditorSubsystem)
for row in json.loads((folder/'manifest.json').read_text()):
    name=row['name'];file=folder/'Export'/(name+'.fbx');opt=ue.FbxImportUI();opt.automated_import_should_detect_type=False;opt.import_as_skeletal=False;opt.mesh_type_to_import=ue.FBXImportType.FBXIT_STATIC_MESH;opt.import_materials=False;opt.import_textures=False;opt.import_animations=False;opt.static_mesh_import_data.combine_meshes=True;opt.static_mesh_import_data.auto_generate_collision=False
    task=ue.AssetImportTask();task.filename=str(file);task.destination_path=base+'/Meshes';task.destination_name=name;task.automated=True;task.replace_existing=True;task.save=True;task.options=opt;assets.import_asset_tasks([task]);mesh=eal.load_asset(base+'/Meshes/'+name);assert mesh,name
    for i,slot in enumerate(mesh.get_editor_property('static_materials')):
        key=str(slot.material_slot_name).split('.')[0];path=base+'/Materials/'+key
        if not eal.does_asset_exist(path):path='/Game/Wildline/Lobby/Photoreal/Materials/'+key
        material_asset=eal.load_asset(path);assert material_asset,(name,key,path);mesh.set_material(i,material_asset)
    if name=='SM_FishingBoat':mesh.get_editor_property('body_setup').set_editor_property('collision_trace_flag',ue.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    eal.save_loaded_asset(mesh,only_if_is_dirty=False)
levels=ue.get_editor_subsystem(ue.LevelEditorSubsystem);actors=ue.get_editor_subsystem(ue.EditorActorSubsystem)
path='/Game/Maps/TaylorsvilleLake'
assert levels.load_level(path) if eal.does_asset_exist(path) else levels.new_level_from_template(path,'/Game/Maps/Blackwood')
for a in actors.get_all_level_actors():
    if a.get_class().get_name()=='WildlineWorld':
        a.set_editor_property('bLake',True);a.set_editor_property('bWinter',False);a.set_editor_property('bFarm',False);a.set_editor_property('bResidential',False);a.set_actor_label('Taylorsville Lake / lakeshore hunt');a.rebuild()
    if isinstance(a,ue.ExponentialHeightFog):a.component.set_editor_property('fog_density',.009)
assert levels.save_current_level()
ue.log('TAYLORSVILLE LAKE IMPORT PASS')
ue.SystemLibrary.quit_editor()
