"""Build the editable King of the Hill neighborhood using existing licensed assets."""
import unreal as ue

eal=ue.EditorAssetLibrary
mel=ue.MaterialEditingLibrary
assets=ue.AssetToolsHelpers.get_asset_tools()
base='/Game/Wildline/Residential'
eal.make_directory(base)
path=base+'/M_Lawn'
m=eal.load_asset(path) if eal.does_asset_exist(path) else assets.create_asset('M_Lawn',base,ue.Material,ue.MaterialFactoryNew())
mel.delete_all_material_expressions(m)
world=mel.create_material_expression(m,ue.MaterialExpressionWorldPosition)
color=mel.create_material_expression(m,ue.MaterialExpressionCustom)
color.set_editor_property('code','float2 q=P.xy*.025;float2 i=floor(q),f=frac(q);f=f*f*(3-2*f);float4 h=frac(sin(float4(dot(i,float2(12.9898,78.233)),dot(i+float2(1,0),float2(12.9898,78.233)),dot(i+float2(0,1),float2(12.9898,78.233)),dot(i+1,float2(12.9898,78.233))))*43758.5453);float n=lerp(lerp(h.x,h.y,f.x),lerp(h.z,h.w,f.x),f.y);return lerp(float3(.055,.095,.016),float3(.11,.16,.035),n);')
color.set_editor_property('output_type',ue.CustomMaterialOutputType.CMOT_FLOAT3)
inp=ue.CustomInput();inp.set_editor_property('input_name','P');color.set_editor_property('inputs',[inp])
mel.connect_material_expressions(world,'',color,'P');mel.connect_material_property(color,'',ue.MaterialProperty.MP_BASE_COLOR)
rough=mel.create_material_expression(m,ue.MaterialExpressionConstant);rough.set_editor_property('r',.95)
mel.connect_material_property(rough,'',ue.MaterialProperty.MP_ROUGHNESS);mel.recompile_material(m);eal.save_loaded_asset(m,only_if_is_dirty=False)
levels=ue.get_editor_subsystem(ue.LevelEditorSubsystem)
actors=ue.get_editor_subsystem(ue.EditorActorSubsystem)
if not eal.does_asset_exist('/Game/Maps/Neighborhood'):
    assert eal.duplicate_asset('/Game/Maps/Farmstead','/Game/Maps/Neighborhood')
levels.load_level('/Game/Maps/Neighborhood')
for actor in actors.get_all_level_actors():
    if actor.get_class().get_name()=='WildlineWorld':
        actor.set_editor_property('bFarm',False);actor.set_editor_property('bResidential',True)
        actor.set_actor_label('King of the Hill / two backyard courses');actor.rebuild()
    elif actor.get_class().get_name()=='PlayerStart':
        actor.set_actor_location(ue.Vector(1500,-1600,320),False,False)
    elif actor.get_class().get_name()=='PointLight':actors.destroy_actor(actor)
levels.save_current_level();eal.save_directory(base,only_if_is_dirty=False,recursive=True)
ue.log('NEIGHBORHOOD CONTENT READY');ue.SystemLibrary.quit_editor()
