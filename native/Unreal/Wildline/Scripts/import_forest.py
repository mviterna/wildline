"""Import licensed foliage and rebuild the native reserve map."""
from pathlib import Path
import json
import unreal as ue
root=Path(ue.Paths.project_dir()).resolve();source=root/'ArtSource/Forest'
assets=ue.AssetToolsHelpers.get_asset_tools();eal=ue.EditorAssetLibrary;mel=ue.MaterialEditingLibrary
base='/Game/Wildline/Forest'
def texture(path,name,normal=False,linear=False):
    t=ue.AssetImportTask();t.filename=str(source/path);t.destination_path=base+'/Textures';t.destination_name=name;t.automated=True;t.replace_existing=True;t.save=True
    assets.import_asset_tasks([t]);tex=eal.load_asset(base+'/Textures/'+name)
    if normal:tex.compression_settings=ue.TextureCompressionSettings.TC_NORMALMAP;tex.srgb=False;tex.flip_green_channel=True
    if linear:tex.srgb=False
    eal.save_loaded_asset(tex);return tex
leaf=texture('Export/LeafCluster.png','T_LeafCluster')
single=texture('Leaves/LeafSet001_2K-JPG_Color.jpg','T_Leaf')
single_a=texture('Leaves/LeafSet001_2K-JPG_Opacity.jpg','T_LeafMask',linear=True)
single_n=texture('Leaves/LeafSet001_2K-JPG_NormalGL.jpg','T_LeafNormal',True)
fern=texture('Export/Nature_Fern_01_DiffuseMask.png','T_Fern')
photograss=texture('PhotoGrass/textures/grass_medium_01_diff_1k.jpg','T_PhotoGrass')
photograss_a=texture('PhotoGrass/textures/grass_medium_01_alpha_1k.png','T_PhotoGrassMask',linear=True)
photograss_n=texture('PhotoGrass/textures/grass_medium_01_nor_gl_1k.png','T_PhotoGrassNormal',True)
litter=texture('LeafLitter-color.jpg','T_Litter');litter_n=texture('LeafLitter-normal.jpg','T_LitterNormal',True);litter_r=texture('LeafLitter-rough.jpg','T_LitterRough',linear=True)
bark=eal.load_asset('/Game/Wildline/Textures/T_Bark');bark_n=eal.load_asset('/Game/Wildline/Textures/T_Bark_N');pine=eal.load_asset('/Game/Wildline/Textures/T_Pine');pine_a=eal.load_asset('/Game/Wildline/Textures/T_Pine_Alpha');pine_n=eal.load_asset('/Game/Wildline/Textures/T_Pine_N');ground=eal.load_asset('/Game/Wildline/Textures/T_Ground')
def node(m,cls,**props):
    n=mel.create_material_expression(m,cls)
    for k,v in props.items():n.set_editor_property(k,v)
    return n
def connect(a,b,pin='',output=''):
    if not mel.connect_material_expressions(a,output,b,pin):raise RuntimeError(f'Material connection failed: {a.get_class().get_name()}.{output} -> {pin}')
def prop(n,p,output=''):mel.connect_material_property(n,output,p)
def constant(m,v,p):prop(node(m,ue.MaterialExpressionConstant,r=v),p)
def sample(m,tex,uv=None):
    n=node(m,ue.MaterialExpressionTextureSample,texture=tex)
    if tex.compression_settings==ue.TextureCompressionSettings.TC_NORMALMAP:n.set_editor_property('sampler_type',ue.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    elif not tex.srgb:n.set_editor_property('sampler_type',ue.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    if uv:connect(uv,n,'UVs')
    return n
def custom(m,code,inputs,output=ue.CustomMaterialOutputType.CMOT_FLOAT3):
    n=node(m,ue.MaterialExpressionCustom,code=code,output_type=output)
    definitions=[]
    for name,value in inputs.items():
        i=ue.CustomInput();i.set_editor_property('input_name',name);definitions.append(i)
    n.set_editor_property('inputs',definitions)
    for name,value in inputs.items():
        v,out=value if isinstance(value,tuple) else (value,'');connect(v,n,name,out)
    return n
def material(name):
    path=base+'/Materials/'+name
    return eal.load_asset(path) if eal.does_asset_exist(path) else assets.create_asset(name,base+'/Materials',ue.Material,ue.MaterialFactoryNew())
def finish(m):
    mel.set_material_usage(m,ue.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES);mel.recompile_material(m);eal.save_loaded_asset(m)
def wind(m,amp):
    p=node(m,ue.MaterialExpressionWorldPosition);t=node(m,ue.MaterialExpressionTime);vc=node(m,ue.MaterialExpressionVertexColor)
    w=custom(m,f'return float3(sin(T*1.35+P.x*.003+P.y*.002)*{amp},cos(T*1.1+P.y*.003)*{amp}*.6,0)*V.r*V.r;',{'P':p,'T':t,'V':vc})
    prop(w,ue.MaterialProperty.MP_WORLD_POSITION_OFFSET)
def foliage(m):
    m.set_editor_property('two_sided',True);m.set_editor_property('shading_model',ue.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
    constant(m,.83,ue.MaterialProperty.MP_ROUGHNESS);constant(m,.18,ue.MaterialProperty.MP_SPECULAR)

for name,dry in [('M_Grass',False),('M_DryGrass',True)]:
    m=material(name);foliage(m);vc=node(m,ue.MaterialExpressionVertexColor);rnd=node(m,ue.MaterialExpressionPerInstanceRandom)
    code='return lerp(float3(.09,.055,.022),float3(.34,.23,.10),V.r)*lerp(.72,1.05,R);' if dry else 'return lerp(float3(.035,.085,.012),float3(.18,.29,.045),V.r)*lerp(.72,1.06,R)*lerp(.86,1.08,V.g);'
    c=custom(m,code,{'V':vc,'R':rnd});prop(c,ue.MaterialProperty.MP_BASE_COLOR)
    prop(custom(m,'return C*.55;',{'C':c}),ue.MaterialProperty.MP_SUBSURFACE_COLOR);wind(m,1.7);finish(m)
for name,tex in [('M_Leaves',leaf),('M_Fern',fern)]:
    m=material(name);foliage(m);m.set_editor_property('blend_mode',ue.BlendMode.BLEND_MASKED);m.set_editor_property('opacity_mask_clip_value',.35)
    c=sample(m,tex);rnd=node(m,ue.MaterialExpressionPerInstanceRandom)
    shade=custom(m,'return C*float3(.72,.92,.55)*lerp(.78,1.08,R);',{'C':(c,'RGB'),'R':rnd})
    prop(shade,ue.MaterialProperty.MP_BASE_COLOR);prop(c,ue.MaterialProperty.MP_OPACITY_MASK,'A')
    prop(custom(m,'return C*.65;',{'C':shade}),ue.MaterialProperty.MP_SUBSURFACE_COLOR);wind(m,1.1 if name=='M_Leaves' else 2.);finish(m)
m=material('M_PhotoGrass');foliage(m);m.set_editor_property('blend_mode',ue.BlendMode.BLEND_MASKED);m.set_editor_property('opacity_mask_clip_value',.3)
c=sample(m,photograss);rnd=node(m,ue.MaterialExpressionPerInstanceRandom)
shade=custom(m,'return C*lerp(.70,1.02,R);',{'C':(c,'RGB'),'R':rnd});prop(shade,ue.MaterialProperty.MP_BASE_COLOR)
prop(sample(m,photograss_a),ue.MaterialProperty.MP_OPACITY_MASK,'R');prop(sample(m,photograss_n),ue.MaterialProperty.MP_NORMAL,'RGB');prop(custom(m,'return C*.6;',{'C':shade}),ue.MaterialProperty.MP_SUBSURFACE_COLOR);wind(m,1.2);finish(m)
m=material('M_LivingLeaf');foliage(m);m.set_editor_property('blend_mode',ue.BlendMode.BLEND_MASKED);m.set_editor_property('opacity_mask_clip_value',.3)
uv0=node(m,ue.MaterialExpressionTextureCoordinate);uv=custom(m,'return U*float2(.195,.44)+float2(.05,.53);',{'U':uv0},ue.CustomMaterialOutputType.CMOT_FLOAT2)
c=sample(m,single,uv);prop(c,ue.MaterialProperty.MP_BASE_COLOR,'RGB');prop(sample(m,single_a,uv),ue.MaterialProperty.MP_OPACITY_MASK,'R');prop(sample(m,single_n,uv),ue.MaterialProperty.MP_NORMAL,'RGB');prop(custom(m,'return C*.6;',{'C':(c,'RGB')}),ue.MaterialProperty.MP_SUBSURFACE_COLOR);wind(m,1.1);finish(m)
m=material('M_FallenLeaf');m.set_editor_property('two_sided',True);m.set_editor_property('blend_mode',ue.BlendMode.BLEND_MASKED);m.set_editor_property('opacity_mask_clip_value',.4)
c=sample(m,single);prop(custom(m,'float g=dot(C,float3(.3,.59,.11));return lerp(C,float3(g*.72,g*.43,g*.17),.86);',{'C':(c,'RGB')}),ue.MaterialProperty.MP_BASE_COLOR)
prop(sample(m,single_a),ue.MaterialProperty.MP_OPACITY_MASK,'R');prop(sample(m,single_n),ue.MaterialProperty.MP_NORMAL,'RGB');constant(m,.98,ue.MaterialProperty.MP_ROUGHNESS);finish(m)
m=material('M_Needles');foliage(m);m.set_editor_property('blend_mode',ue.BlendMode.BLEND_MASKED);m.set_editor_property('opacity_mask_clip_value',.3)
uv=node(m,ue.MaterialExpressionTextureCoordinate,u_tiling=.17,v_tiling=.285);uv2=node(m,ue.MaterialExpressionAdd,const_b=.045);connect(uv,uv2,'A')
# Vector offset selects the photographed needle shoot from the original CC0 atlas.
off=node(m,ue.MaterialExpressionConstant2Vector,r=.045,g=.045);connect(off,uv2,'B')
c=sample(m,pine,uv2);shade=custom(m,'return C*float3(.65,.85,.45);',{'C':(c,'RGB')});prop(shade,ue.MaterialProperty.MP_BASE_COLOR)
prop(sample(m,pine_a,uv2),ue.MaterialProperty.MP_OPACITY_MASK,'R');prop(sample(m,pine_n,uv2),ue.MaterialProperty.MP_NORMAL,'RGB');prop(custom(m,'return C*.35;',{'C':shade}),ue.MaterialProperty.MP_SUBSURFACE_COLOR);wind(m,.7);finish(m)
m=material('M_TreeBark');p=node(m,ue.MaterialExpressionWorldPosition);uv=custom(m,'return float2(P.x*.010+P.y*.007,P.z*.004);',{'P':p},ue.CustomMaterialOutputType.CMOT_FLOAT2)
prop(sample(m,bark,uv),ue.MaterialProperty.MP_BASE_COLOR,'RGB');prop(sample(m,bark_n,uv),ue.MaterialProperty.MP_NORMAL,'RGB');constant(m,.94,ue.MaterialProperty.MP_ROUGHNESS);finish(m)
m=material('M_LeafLitter');c=sample(m,litter);d=sample(m,ground);pos=node(m,ue.MaterialExpressionWorldPosition)
color=custom(m,'float path=1-smoothstep(75,190,abs(P.y-700-sin(P.x*.0007)*210));float macro=.88+.09*sin(P.x*.0013)*cos(P.y*.0011);return lerp(C,D*.8,path*.78)*macro;',{'C':(c,'RGB'),'D':(d,'RGB'),'P':pos})
prop(color,ue.MaterialProperty.MP_BASE_COLOR);prop(sample(m,litter_n),ue.MaterialProperty.MP_NORMAL,'RGB');prop(sample(m,litter_r),ue.MaterialProperty.MP_ROUGHNESS,'R');constant(m,.15,ue.MaterialProperty.MP_SPECULAR);finish(m)
m=material('M_Stone');p=node(m,ue.MaterialExpressionWorldPosition);c=sample(m,bark)
prop(custom(m,'float g=dot(C,float3(.3,.59,.11));float moss=smoothstep(.3,.85,sin(P.x*.012)*cos(P.y*.014));return lerp(float3(g*.85,g*.86,g*.8),float3(.09,.13,.035),moss*.5);',{'C':(c,'RGB'),'P':p}),ue.MaterialProperty.MP_BASE_COLOR)
prop(sample(m,bark_n),ue.MaterialProperty.MP_NORMAL,'RGB');constant(m,.95,ue.MaterialProperty.MP_ROUGHNESS);finish(m)
m=material('M_Heartwood');c=sample(m,bark)
prop(custom(m,'float g=dot(C,float3(.3,.59,.11));return float3(g*.37,g*.24,g*.12);',{'C':(c,'RGB')}),ue.MaterialProperty.MP_BASE_COLOR);constant(m,.99,ue.MaterialProperty.MP_ROUGHNESS);finish(m)

mapping={'F_PhotoGrass':'M_PhotoGrass','F_LivingLeaf':'M_LivingLeaf','F_Grass':'M_Grass','F_DryGrass':'M_DryGrass','F_Fern':'M_Fern','F_Bark':'M_TreeBark','F_Leaf':'M_Leaves','F_Needles':'M_Needles','F_FallenLeaf':'M_FallenLeaf','F_Stone':'M_Stone','F_Heartwood':'M_Heartwood'}
report=[]
meshsub=ue.get_editor_subsystem(ue.StaticMeshEditorSubsystem)
refresh='ForestRefreshOnly' in ue.SystemLibrary.get_command_line()
photo_refresh='ForestPhotoRefresh' in ue.SystemLibrary.get_command_line()
for file in sorted((source/'Export').glob('*.fbx')):
    if file.stem=='SM_Birch':continue
    opts=ue.FbxImportUI();opts.automated_import_should_detect_type=False;opts.import_as_skeletal=False;opts.mesh_type_to_import=ue.FBXImportType.FBXIT_STATIC_MESH;opts.import_materials=False;opts.import_textures=False;opts.import_animations=False
    opts.static_mesh_import_data.normal_import_method=ue.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS;opts.static_mesh_import_data.combine_meshes=True;opts.static_mesh_import_data.auto_generate_collision=False;opts.static_mesh_import_data.vertex_color_import_option=ue.VertexColorImportOption.REPLACE
    task=ue.AssetImportTask();task.filename=str(file);task.destination_path=base+'/Meshes';task.destination_name=file.stem;task.automated=True;task.replace_existing=True;task.save=True;task.options=opts
    changed=file.stem in (('SM_GrassA','SM_GrassB') if photo_refresh else ('SM_Log','SM_OakA','SM_OakB','SM_OakC','SM_GrassA','SM_GrassB'))
    if not refresh or changed or not eal.does_asset_exist(base+'/Meshes/'+file.stem):assets.import_asset_tasks([task])
    mesh=eal.load_asset(base+'/Meshes/'+file.stem)
    if not mesh:raise RuntimeError('Missing mesh '+str(file))
    for i,slot in enumerate(mesh.get_editor_property('static_materials')):
        slotname=str(slot.material_slot_name);mat=next((v for k,v in mapping.items() if slotname==k or slotname.startswith(k+'_')),'M_Grass');mesh.set_material(i,eal.load_asset(base+'/Materials/'+mat))
    # Runtime LODs preserve dense near geometry with much less distant raster work.
    reductions=[]
    for fraction,screen in ([(1.,1.),(.78,.08),(.42,.025),(.18,.008)] if 'Grass' in file.stem or 'Clover' in file.stem else ([(1.,1.),(.85,.25),(.65,.08),(.35,.025)] if 'Oak' in file.stem or 'Pine' in file.stem else [(1.,1.),(.48,.25),(.17,.08),(.055,.025)])):
        reduction=ue.EditorScriptingMeshReductionSettings();reduction.percent_triangles=fraction;reduction.screen_size=screen;reductions.append(reduction)
    lod=ue.EditorScriptingMeshReductionOptions();lod.auto_compute_lod_screen_size=False;lod.reduction_settings=reductions
    if not refresh or changed or 'Grass' in file.stem or 'Clover' in file.stem:meshsub.set_lods(mesh,lod)
    if file.stem=='SM_Stone':
        assert meshsub.set_convex_decomposition_collisions(mesh,1,24,100000)
        assert meshsub.get_convex_collision_count(mesh)>0
    eal.save_loaded_asset(mesh)
    bounds=mesh.get_bounds();report.append({'name':file.stem,'extent_cm':[bounds.box_extent.x,bounds.box_extent.y,bounds.box_extent.z],'lods':meshsub.get_lod_count(mesh)})

levels=ue.get_editor_subsystem(ue.LevelEditorSubsystem);actors=ue.get_editor_subsystem(ue.EditorActorSubsystem);levels.load_level('/Game/Maps/Blackwood')
for a in actors.get_all_level_actors():
    if a.get_class().get_name()=='WildlineWorld':a.rebuild()
    if isinstance(a,ue.DirectionalLight):
        light=a.light_component
        if a.get_actor_label()=='Early autumn sun':
            a.set_actor_rotation(ue.Rotator(pitch=-38,yaw=-38,roll=0),False);light.set_intensity(12000);light.set_editor_property('light_source_angle',2.5)
        else:light.set_intensity(900)
    if isinstance(a,ue.SkyLight):a.light_component.set_intensity(1.15)
levels.save_current_level();eal.save_directory(base,only_if_is_dirty=False)
(source/'unreal-import.json').write_text(json.dumps(report,indent=2));ue.log('FOREST IMPORT COMPLETE '+json.dumps(report))

ue.SystemLibrary.quit_editor()
