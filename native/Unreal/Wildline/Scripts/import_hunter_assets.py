import unreal as ue,json
from pathlib import Path
root=Path(ue.Paths.project_dir()).resolve();source=root/'ArtSource/Hunters';base='/Game/Wildline/Hunters'
assets=ue.AssetToolsHelpers.get_asset_tools();eal=ue.EditorAssetLibrary;mel=ue.MaterialEditingLibrary
helper=(root/'Scripts/import_forest.py').read_text()
exec(helper[helper.index('def node('):helper.index("for name,dry in")])
# UE 5.8 Interchange sorts skeletal render UV streams by name: PhotoFace precedes
# UVMap, although the source MeshDescription/FBX export retains the other order.
base_sample=sample
def sample(m,tex,uv=None):
    return base_sample(m,tex,uv or node(m,ue.MaterialExpressionTextureCoordinate,coordinate_index=1))
def finish(m):
    mel.set_material_usage(m,ue.MaterialUsage.MATUSAGE_SKELETAL_MESH)
    mel.set_material_usage(m,ue.MaterialUsage.MATUSAGE_MORPH_TARGETS)
    mel.set_material_usage(m,ue.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    mel.recompile_material(m);eal.save_loaded_asset(m)
def texture(file,normal=False):
    name='T_'+file.stem.replace('.','_');t=ue.AssetImportTask();t.filename=str(file);t.destination_path=base+'/Textures';t.destination_name=name;t.automated=True;t.replace_existing=True;t.save=True;assets.import_asset_tasks([t]);tex=eal.load_asset(base+'/Textures/'+name)
    if normal:tex.compression_settings=ue.TextureCompressionSettings.TC_NORMALMAP;tex.srgb=False;tex.flip_green_channel=True
    eal.save_loaded_asset(tex);return tex
textures={p.name:texture(p,'normal' in p.name) for p in (source/'Textures').glob('*.png')}
def scalar(m,name,value):return node(m,ue.MaterialExpressionScalarParameter,parameter_name=name,default_value=value)
def tint(m,name,color):return node(m,ue.MaterialExpressionVectorParameter,parameter_name=name,default_value=ue.LinearColor(*color))
for sex in ['male','female']:
    m=material('M_Skin_'+sex);m.set_editor_property('shading_model',ue.MaterialShadingModel.MSM_SUBSURFACE)
    c=sample(m,textures['young_lightskinned_'+sex+'_diffuse.png']);skin=tint(m,'SkinTint',(1,1,1,1));vc=node(m,ue.MaterialExpressionVertexColor)
    puv=node(m,ue.MaterialExpressionTextureCoordinate,coordinate_index=0);zoom=scalar(m,'PhotoZoom',1.6);px=scalar(m,'PhotoX',0);py=scalar(m,'PhotoY',0)
    uv=custom(m,'return (U-.5)/max(.1,Z)+float2(.5+X,.5+Y);',{'U':puv,'Z':zoom,'X':px,'Y':py},ue.CustomMaterialOutputType.CMOT_FLOAT2)
    photo=node(m,ue.MaterialExpressionTextureSampleParameter2D,parameter_name='FacePhoto',texture=textures['young_lightskinned_'+sex+'_diffuse.png'],sampler_source=ue.SamplerSourceMode.SSM_CLAMP_WORLD_GROUP_SETTINGS);connect(uv,photo,'UVs')
    has=scalar(m,'HasPhoto',0)
    color=custom(m,'return lerp(C,P,saturate(V.r)*H)*S;',{'C':(c,'RGB'),'S':(skin,'RGB'),'P':(photo,'RGB'),'V':vc,'H':has});prop(color,ue.MaterialProperty.MP_BASE_COLOR)
    constant(m,.58,ue.MaterialProperty.MP_ROUGHNESS);constant(m,.3,ue.MaterialProperty.MP_SPECULAR);prop(custom(m,'return C*.35;',{'C':color}),ue.MaterialProperty.MP_SUBSURFACE_COLOR);finish(m)
for name,file,normal,masked,param in [('M_Cloth','male_casualsuit05_diffuse.png','male_casualsuit05_normal.png',False,'OutfitTint'),('M_Boots','shoes04_diffuse.png','shoes04_normal.png',False,''),('M_Hair','short01_diffuse.png','',True,'HairTint'),('M_Brows','eyebrow001.png','',True,'HairTint'),('M_Eyes','brown_eye.png','',False,'')]:
    m=material(name);c=sample(m,textures[file]);color=c
    if param:
        v=tint(m,param,(.48,.57,.37,1) if param=='OutfitTint' else (.32,.23,.14,1));code='float g=dot(C,float3(.3,.59,.11));return saturate(.10+g*2)*T;' if param=='OutfitTint' else 'float g=dot(C,float3(.3,.59,.11));return lerp(C,g.xxx,.7)*T;';color=custom(m,code,{'C':(c,'RGB'),'T':(v,'RGB')})
    prop(color,ue.MaterialProperty.MP_BASE_COLOR,'RGB' if color==c else '')
    # Render tangents are built against the planar UV0 portrait stream. Use the
    # mesh normals until tangents can be rebuilt against diffuse UV1.
    if normal:prop(node(m,ue.MaterialExpressionConstant3Vector,constant=ue.LinearColor(0,0,1,1)),ue.MaterialProperty.MP_NORMAL)
    if masked:
        m.set_editor_property('blend_mode',ue.BlendMode.BLEND_MASKED);m.set_editor_property('two_sided',True)
        if name=='M_Hair':prop(custom(m,'return A*(1-C);',{'A':(c,'A'),'C':scalar(m,'CapVisible',0)},ue.CustomMaterialOutputType.CMOT_FLOAT1),ue.MaterialProperty.MP_OPACITY_MASK)
        else:prop(c,ue.MaterialProperty.MP_OPACITY_MASK,'A')
    constant(m,.78 if name!='M_Eyes' else .15,ue.MaterialProperty.MP_ROUGHNESS);finish(m)
m=material('M_Cap');m.set_editor_property('blend_mode',ue.BlendMode.BLEND_MASKED);m.set_editor_property('two_sided',True);prop(custom(m,'return T*.42;',{'T':(tint(m,'OutfitTint',(.2,.28,.1,1)),'RGB')}),ue.MaterialProperty.MP_BASE_COLOR);prop(scalar(m,'CapVisible',1),ue.MaterialProperty.MP_OPACITY_MASK);constant(m,.86,ue.MaterialProperty.MP_ROUGHNESS);finish(m)
m=material('M_TravisCamo');p=node(m,ue.MaterialExpressionTextureCoordinate,coordinate_index=0);vc=node(m,ue.MaterialExpressionVertexColor);t=tint(m,'OutfitTint',(1,.12,.008,1))
c=custom(m,'float3 q=float3(P.x*40,P.y*40,0);float n=sin(q.x+sin(q.y*.7)*3)*cos(q.y*.8+sin(q.x)*2);float f=sin(q.x*3+q.y*2);float3 c=n>.3?float3(.15,.18,.09):n<-.3?float3(.028,.04,.022):f>0?float3(.28,.23,.13):float3(.08,.095,.048);return lerp(c,T,saturate(V.g));',{'P':p,'V':vc,'T':(t,'RGB')});prop(c,ue.MaterialProperty.MP_BASE_COLOR);constant(m,.88,ue.MaterialProperty.MP_ROUGHNESS);finish(m)
m=material('M_TravisVest');m.set_editor_property('blend_mode',ue.BlendMode.BLEND_MASKED);prop(tint(m,'OutfitTint',(1,.12,.008,1)),ue.MaterialProperty.MP_BASE_COLOR,'RGB');constant(m,0,ue.MaterialProperty.MP_OPACITY_MASK);constant(m,.87,ue.MaterialProperty.MP_ROUGHNESS);finish(m)
# Facial hair comes from the calibrated face projection; hide coarse shell cards.
m=material('M_TravisBeard');m.set_editor_property('blend_mode',ue.BlendMode.BLEND_MASKED);prop(tint(m,'HairTint',(.62,.61,.58,1)),ue.MaterialProperty.MP_BASE_COLOR,'RGB');constant(m,0,ue.MaterialProperty.MP_OPACITY_MASK);constant(m,.94,ue.MaterialProperty.MP_ROUGHNESS);finish(m)
for name,color,metal,rough in [('R_Metal',(.028,.031,.034,1),.87,.34),('R_Steel',(.42,.44,.46,1),.92,.28),('R_Rubber',(.016,.018,.019,1),0,.96),('R_Black',(.003,.004,.004,1),.4,.35),('R_Glass',(.018,.065,.043,1),.65,.08)]:
    m=material(name);prop(tint(m,'Tint',color),ue.MaterialProperty.MP_BASE_COLOR,'RGB');constant(m,metal,ue.MaterialProperty.MP_METALLIC);constant(m,rough,ue.MaterialProperty.MP_ROUGHNESS);finish(m)
m=material('R_Wood');p=node(m,ue.MaterialExpressionWorldPosition);local=node(m,ue.MaterialExpressionTransformPosition,transform_source_type=ue.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_WORLD,transform_type=ue.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_LOCAL);connect(p,local,'');color=custom(m,'float g=sin(P.x*.13+sin(P.z*.5)*2+sin(P.y*.3))* .5+.5;float fine=sin(P.x*3.4+sin(P.z*.8))*.07;return lerp(float3(.065,.021,.007),float3(.30,.12,.04),saturate(g*.6+.2+fine));',{'P':local});prop(color,ue.MaterialProperty.MP_BASE_COLOR);constant(m,.44,ue.MaterialProperty.MP_ROUGHNESS);finish(m)
m=material('M_PreviewBackdrop');m.set_editor_property('shading_model',ue.MaterialShadingModel.MSM_UNLIT);m.set_editor_property('two_sided',True);prop(tint(m,'Backdrop',(.012,.021,.016,1)),ue.MaterialProperty.MP_EMISSIVE_COLOR,'RGB');finish(m)
skeleton=eal.load_asset('/Game/Characters/Mannequins/Meshes/SK_Mannequin')
ue.SystemLibrary.execute_console_command(ue.EditorLevelLibrary.get_editor_world(),'Interchange.FeatureFlags.Import.FBX 0')
report=[]
for file in (source/'Export').glob('*.fbx'):
    opts=ue.FbxImportUI();opts.automated_import_should_detect_type=False;opts.import_materials=False;opts.import_textures=False;opts.import_animations=False
    skeletal=file.stem.startswith('SK_');opts.import_as_skeletal=skeletal;opts.mesh_type_to_import=ue.FBXImportType.FBXIT_SKELETAL_MESH if skeletal else ue.FBXImportType.FBXIT_STATIC_MESH
    if skeletal:
        opts.skeleton=skeleton;opts.import_mesh=True;opts.skeletal_mesh_import_data.set_editor_property('import_morph_targets',True);opts.skeletal_mesh_import_data.set_editor_property('import_content_type',ue.FBXImportContentType.FBXICT_ALL);opts.skeletal_mesh_import_data.vertex_color_import_option=ue.VertexColorImportOption.REPLACE;opts.skeletal_mesh_import_data.normal_import_method=ue.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS
        ue.log('HUNTER MORPH IMPORT REQUEST '+file.stem+' '+str(opts.skeletal_mesh_import_data.get_editor_property('import_morph_targets')))
    else:opts.static_mesh_import_data.combine_meshes=True;opts.static_mesh_import_data.auto_generate_collision=False
    # Interchange preserves Blender's shape channels; the legacy importer is
    # still required for static meshes with the custom UCX log collision.
    ue.SystemLibrary.execute_console_command(ue.EditorLevelLibrary.get_editor_world(),'Interchange.FeatureFlags.Import.FBX '+('1' if skeletal else '0'))
    task=ue.AssetImportTask()
    if not skeletal:task.factory=ue.FbxFactory()
    task.filename=str(file);task.destination_path=base+'/Meshes';task.destination_name=file.stem;task.options=opts;task.automated=True;task.replace_existing=True;task.replace_existing_settings=True;task.save=True;assets.import_asset_tasks([task])
    mesh=eal.load_asset(base+'/Meshes/'+file.stem);assert mesh
    if skeletal:ue.log('HUNTER MORPH IMPORT RESULT '+file.stem+' '+str([m.get_name() for m in mesh.get_editor_property('morph_targets')]))
    mapping={'H_Skin':'M_Skin_female' if 'Female' in file.stem else 'M_Skin_male','H_Cloth':'M_TravisCamo' if 'Travis' in file.stem else 'M_Cloth','H_Boots':'M_Boots','H_Hair':'M_Hair','H_Eyes':'M_Eyes','H_Brows':'M_Brows','H_Cap':'M_Cap','H_Beard':'M_TravisBeard','H_Vest':'M_TravisVest'}
    slots=mesh.get_editor_property('materials' if skeletal else 'static_materials')
    for i,slot in enumerate(slots):
        key=str(slot.material_slot_name);name=next((v for k,v in mapping.items() if key==k or key.startswith(k+'_')),key.split('.')[0])
        material_asset=eal.load_asset(base+'/Materials/'+name)
        assert material_asset,(file,key,name)
        ue.MaterialEditingLibrary.set_material_usage(material_asset,ue.MaterialUsage.MATUSAGE_SKELETAL_MESH) if skeletal else None
        if skeletal:
            slot.set_editor_property('material_interface',material_asset)
            slots[i]=slot
        else:mesh.set_material(i,material_asset)
    if skeletal:mesh.call_method('SetMaterials',(slots,))
    eal.save_loaded_asset(mesh);report.append({'mesh':file.stem,'slots':[str(s.material_slot_name) for s in slots]})
# Use the legacy FBX importer for custom convex collision hulls.
ue.SystemLibrary.execute_console_command(ue.EditorLevelLibrary.get_editor_world(),'Interchange.FeatureFlags.Import.FBX 0')
# Reimport just the log, retaining its twelve UCX hulls and hollow center.
opts=ue.FbxImportUI();opts.automated_import_should_detect_type=False;opts.import_as_skeletal=False;opts.mesh_type_to_import=ue.FBXImportType.FBXIT_STATIC_MESH;opts.import_materials=False;opts.import_textures=False;opts.static_mesh_import_data.combine_meshes=True;opts.static_mesh_import_data.auto_generate_collision=False;opts.static_mesh_import_data.one_convex_hull_per_ucx=True
t=ue.AssetImportTask();t.factory=ue.FbxFactory();t.filename=str(root/'ArtSource/Forest/Export/SM_Log.fbx');t.destination_path='/Game/Wildline/Forest/Meshes';t.destination_name='SM_Log';t.options=opts;t.automated=True;t.replace_existing=True;t.save=True;assets.import_asset_tasks([t])
log=eal.load_asset('/Game/Wildline/Forest/Meshes/SM_Log');log.set_material(0,eal.load_asset('/Game/Wildline/Forest/Materials/M_TreeBark'));log.set_material(1,eal.load_asset('/Game/Wildline/Forest/Materials/M_Heartwood'));eal.save_loaded_asset(log)
eal.save_directory(base,only_if_is_dirty=True)
(source/'unreal-import.json').write_text(json.dumps(report,indent=2));ue.log('HUNTER IMPORT COMPLETE '+json.dumps(report));ue.SystemLibrary.quit_editor()
