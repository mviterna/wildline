"""Import the selectable photo hunters and their packaged materials."""
import unreal as ue,json,re
from pathlib import Path
root=Path(ue.Paths.project_dir()).resolve();base='/Game/Wildline/Hunters';eal=ue.EditorAssetLibrary;mel=ue.MaterialEditingLibrary;assets=ue.AssetToolsHelpers.get_asset_tools()
helper=(root/'Scripts/import_forest.py').read_text();exec(helper[helper.index('def node('):helper.index('for name,dry in')])
exec(compile((root/'Scripts/import_travis_mesh.py').read_text(),'import_travis_mesh.py','exec'))

def import_photo_hunter(person,skin_color,hair_color):
    slug=person.lower()
    t=ue.AssetImportTask();t.filename=str(root/f'ArtSource/Hunters/Textures/{slug}_reference.png');t.destination_path=base+'/Textures';t.destination_name=f'T_{person}Face';t.automated=True;t.replace_existing=True;t.save=True;assets.import_asset_tasks([t])
    tex=eal.load_asset(base+f'/Textures/T_{person}Face');assert tex;tex.set_editor_property('srgb',True);eal.save_loaded_asset(tex,only_if_is_dirty=False)
    def finish(m):
        mel.set_material_usage(m,ue.MaterialUsage.MATUSAGE_SKELETAL_MESH);mel.set_material_usage(m,ue.MaterialUsage.MATUSAGE_MORPH_TARGETS);mel.recompile_material(m);eal.save_loaded_asset(m,only_if_is_dirty=False)
    def fresh(n):
        m=material(n);mel.delete_all_material_expressions(m);m.set_editor_property('shading_model',ue.MaterialShadingModel.MSM_DEFAULT_LIT);return m
    m=fresh(f'M_{person}Photo')
    uv=node(m,ue.MaterialExpressionTextureCoordinate,coordinate_index=1);ref=sample(m,tex,uv)
    puv=node(m,ue.MaterialExpressionTextureCoordinate,coordinate_index=0)
    def scalar(n,v):return node(m,ue.MaterialExpressionScalarParameter,parameter_name=n,default_value=v)
    puv=custom(m,'return (U-.5)/max(.1,Z)+float2(.5+X,.5+Y);',{'U':puv,'Z':scalar('PhotoZoom',1),'X':scalar('PhotoX',0),'Y':scalar('PhotoY',0)},ue.CustomMaterialOutputType.CMOT_FLOAT2)
    photo=node(m,ue.MaterialExpressionTextureSampleParameter2D,parameter_name='FacePhoto',texture=tex);connect(puv,photo,'UVs')
    skin=node(m,ue.MaterialExpressionVectorParameter,parameter_name='SkinTint',default_value=ue.LinearColor(1,1,1,1))
    c=custom(m,'return lerp(C,P,H)*S*.52;',{'C':(ref,'RGB'),'P':(photo,'RGB'),'H':scalar('HasPhoto',0),'S':(skin,'RGB')});prop(c,ue.MaterialProperty.MP_BASE_COLOR);constant(m,1,ue.MaterialProperty.MP_ROUGHNESS);constant(m,0,ue.MaterialProperty.MP_SPECULAR);finish(m)
    for name,col in [(f'M_{person}Skin',skin_color),(f'M_{person}Hair',hair_color)]:
        m=fresh(name);c=node(m,ue.MaterialExpressionVectorParameter,parameter_name='Tint',default_value=ue.LinearColor(*col))
        tone=node(m,ue.MaterialExpressionVectorParameter,parameter_name='SkinTint',default_value=ue.LinearColor(1,1,1,1))
        prop(custom(m,'return C*T;',{'C':(c,'RGB'),'T':(tone,'RGB')}),ue.MaterialProperty.MP_BASE_COLOR);constant(m,1,ue.MaterialProperty.MP_ROUGHNESS);constant(m,0,ue.MaterialProperty.MP_SPECULAR);finish(m)
    ue.SystemLibrary.execute_console_command(ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world(),'Interchange.FeatureFlags.Import.FBX 1')
    import_travis_mesh(root,f'SK_Hunter{person}')
    mesh=eal.load_asset(base+f'/Meshes/SK_Hunter{person}');assert mesh
    mapping={'H_Skin':'M_Skin_male','H_Cloth':'M_TravisCamo','H_Boots':'M_Boots','H_Cap':'M_Cap','H_Vest':'M_TravisVest',f'H_{person}Photo':f'M_{person}Photo',f'H_{person}Skin':f'M_{person}Skin',f'H_{person}Hair':f'M_{person}Hair'}
    slots=mesh.get_editor_property('materials')
    for i,s in enumerate(slots):
        key=str(s.material_slot_name);name=next((v for k,v in mapping.items() if key==k or key.startswith(k+'_')),None);assert name,key
        s.material_interface=eal.load_asset(base+'/Materials/'+name);assert s.material_interface;slots[i]=s
    mesh.call_method('SetMaterials',(slots,));eal.save_loaded_asset(mesh,only_if_is_dirty=False)
    morphs=[m.get_name() for m in mesh.get_editor_property('morph_targets')];assert all(k in morphs for k in ['FaceWidth','SlimBuild']),morphs
    report={'skeleton':mesh.skeleton.get_path_name(),'morphs':morphs,'materials':{str(s.material_slot_name):s.material_interface.get_path_name() for s in mesh.get_editor_property('materials')},'photoOverlay':True,'photoAsset':tex.get_path_name(),'modeledFacialFeatures':False}
    (root/f'ArtSource/Hunters/{slug}-unreal.json').write_text(json.dumps(report,indent=2));ue.log(person.upper()+' PHOTO IMPORT PASS '+json.dumps(report))


old=eal.load_asset(base+'/Meshes/SK_HunterTravis')
if old and any('M_TravisHead' in s.material_interface.get_name() for s in old.get_editor_property('materials')):
    assert eal.delete_asset(base+'/Meshes/SK_HunterTravis')
selection=re.search(r'-PhotoHunters=(\w+)',ue.SystemLibrary.get_command_line())
selected=selection.group(1) if selection else None
assert selected in [None,'Travis','Ethan','Greyson'],selected
if not selected or selected=='Travis':import_photo_hunter('Travis',(.26,.16,.125,1),(.033,.024,.018,1))
if not selected or selected=='Ethan':import_photo_hunter('Ethan',(.16,.125,.12,1),(.015,.018,.025,1))
if not selected or selected=='Greyson':import_photo_hunter('Greyson',(.29,.20,.17,1),(.045,.035,.027,1))

# Retire the detailed variant and the old comparison names from cooked content.
retired=['Meshes/SK_HunterTravis2','Textures/T_Travis2Face','Textures/T_TravisMH_Basecolor','Textures/T_TravisMH_Normal']
retired += ['Materials/'+n for n in ['M_Travis2Photo','M_Travis2Skin','M_Travis2Hair','M_TravisHead','M_TravisEyes','M_TravisTeeth','M_TravisOcclusion','M_TravisCrop','M_TravisCropGray','M_TravisBrows','M_TravisBrowsGray','M_TravisBeardGray']]
for name in retired:
    if eal.does_asset_exist(base+'/'+name):assert eal.delete_asset(base+'/'+name),name
ue.log('PHOTO HUNTERS IMPORT COMPLETE')
ue.SystemLibrary.quit_editor()
