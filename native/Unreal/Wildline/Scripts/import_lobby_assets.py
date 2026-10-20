"""Import full-resolution CC0 lobby models and PBR maps into Unreal.

Use UnrealEditor-Cmd -run=pythonscript -script=<this file>. Reproducible sources
are in ArtSource/Lobby; generated content stays local to the waiting rooms.
"""
import unreal as ue, json, re
from pathlib import Path
ROOT=Path(ue.Paths.project_dir()).resolve();SRC=ROOT/'ArtSource/Lobby'
BASE='/Game/Wildline/Lobby/Photoreal'
assets=ue.AssetToolsHelpers.get_asset_tools();eal=ue.EditorAssetLibrary;mel=ue.MaterialEditingLibrary
meshsub=ue.get_editor_subsystem(ue.StaticMeshEditorSubsystem)
ue.SystemLibrary.execute_console_command(ue.EditorLevelLibrary.get_editor_world(),'Interchange.FeatureFlags.Import.FBX 0')
cache={};report=[]
details_only='LobbyDetailsOnly' in ue.SystemLibrary.get_command_line()
architecture_only='LobbyArchitectureOnly' in ue.SystemLibrary.get_command_line()
lodge_details_only='LodgeDetailsOnly' in ue.SystemLibrary.get_command_line()
lodge_shell_only='LodgeShellOnly' in ue.SystemLibrary.get_command_line()
upgrade_only='LodgeUpgradeOnly' in ue.SystemLibrary.get_command_line()
upgrade_ids=['sofa_03','ArmChair_01','gothic_coffee_table','vintage_cabinet_01','throw_pillows_01']
lodge_meshes=['SM_LodgeArchitecture','SM_LodgeWindowDressings','SM_LodgeClock','SM_LodgePegRail']
lodge_materials=['M_LodgeTimber','M_LodgeFloor','M_HearthStone','M_WindowOak','M_CurtainFabric','M_LodgeDetailBrass','M_ClockIvory','M_ClockInk']

def node(m,kind):return mel.create_material_expression(m,kind)
def connect(n,p,out=''):assert mel.connect_material_property(n,out,p)
def const(m,value):
    c=node(m,ue.MaterialExpressionConstant);c.set_editor_property('r',value);return c
def texture(file,normal=False,linear=False):
    if not file:return None
    key=(file,normal,linear)
    if key in cache:return cache[key]
    path=SRC/file;assert path.exists(),path
    name='T_'+re.sub(r'[^a-zA-Z0-9_]','_',path.stem)+('_Linear' if linear else '')
    if (details_only or architecture_only or lodge_details_only) and eal.does_asset_exist(BASE+'/Textures/'+name):
        tex=eal.load_asset(BASE+'/Textures/'+name);cache[key]=tex;return tex
    t=ue.AssetImportTask();t.filename=str(path);t.destination_path=BASE+'/Textures';t.destination_name=name;t.automated=True;t.replace_existing=True;t.save=True
    assets.import_asset_tasks([t]);tex=eal.load_asset(t.destination_path+'/'+name);assert tex,name
    tex.set_editor_property('max_texture_size',4096);tex.set_editor_property('srgb',not(normal or linear))
    if normal:tex.set_editor_property('compression_settings',ue.TextureCompressionSettings.TC_NORMALMAP);tex.set_editor_property('flip_green_channel',True)
    elif linear:tex.set_editor_property('compression_settings',ue.TextureCompressionSettings.TC_MASKS)
    eal.save_loaded_asset(tex,only_if_is_dirty=False);cache[key]=tex;return tex

def sample(m,tex):
    n=node(m,ue.MaterialExpressionTextureSample);n.set_editor_property('texture',tex)
    if tex.compression_settings==ue.TextureCompressionSettings.TC_NORMALMAP:n.set_editor_property('sampler_type',ue.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    elif not tex.srgb:n.set_editor_property('sampler_type',ue.MaterialSamplerType.SAMPLERTYPE_MASKS)
    return n

def material(name,info):
    path=BASE+'/Materials/'+name
    m=eal.load_asset(path) if eal.does_asset_exist(path) else assets.create_asset(name,BASE+'/Materials',ue.Material,ue.MaterialFactoryNew())
    mel.delete_all_material_expressions(m);m.set_editor_property('two_sided',info.get('two_sided',False))
    c=node(m,ue.MaterialExpressionConstant3Vector);c.set_editor_property('constant',ue.LinearColor(*info.get('factor',[1,1,1,1])[:3],1))
    color=texture(info.get('color'));color_node=sample(m,color) if color else c
    if color:
        pigment=color_node;output='RGB'
        if info.get('desaturate'):
            pigment=node(m,ue.MaterialExpressionDesaturation);mel.connect_material_expressions(color_node,'RGB',pigment,'');mel.connect_material_expressions(const(m,1),'',pigment,'Fraction');output=''
        mul=node(m,ue.MaterialExpressionMultiply);mel.connect_material_expressions(pigment,output,mul,'A');mel.connect_material_expressions(c,'',mul,'B');connect(mul,ue.MaterialProperty.MP_BASE_COLOR)
    else:connect(c,ue.MaterialProperty.MP_BASE_COLOR)
    normal=texture(info.get('normal'),normal=True)
    if normal:connect(sample(m,normal),ue.MaterialProperty.MP_NORMAL,'RGB')
    arm=texture(info.get('arm'),linear=True)
    if arm:
        n=sample(m,arm);connect(n,ue.MaterialProperty.MP_AMBIENT_OCCLUSION,'R')
        for channel,prop,factor in [('G',ue.MaterialProperty.MP_ROUGHNESS,info.get('roughness',1)),('B',ue.MaterialProperty.MP_METALLIC,info.get('metallic',1))]:
            mul=node(m,ue.MaterialExpressionMultiply);mul.set_editor_property('const_b',factor);mel.connect_material_expressions(n,channel,mul,'A');connect(mul,prop)
    else:
        rough=texture(info.get('rough_map'),linear=True)
        connect(sample(m,rough) if rough else const(m,info.get('roughness',.8)),ue.MaterialProperty.MP_ROUGHNESS,'R' if rough else '')
        connect(const(m,info.get('metallic',0)),ue.MaterialProperty.MP_METALLIC)
    if info.get('alpha')=='MASK' and color:
        m.set_editor_property('blend_mode',ue.BlendMode.BLEND_MASKED);m.set_editor_property('opacity_mask_clip_value',info.get('cutoff',.5));connect(color_node,ue.MaterialProperty.MP_OPACITY_MASK,'A')
    elif info.get('alpha')=='BLEND':
        m.set_editor_property('blend_mode',ue.BlendMode.BLEND_TRANSLUCENT)
        connect(const(m,min(info.get('factor',[1,1,1,.3])[3],.35)),ue.MaterialProperty.MP_OPACITY)
    else:m.set_editor_property('blend_mode',ue.BlendMode.BLEND_OPAQUE)
    if info.get('alpha')!='BLEND':mel.set_material_usage(m,ue.MaterialUsage.MATUSAGE_NANITE)
    mel.recompile_material(m);eal.save_loaded_asset(m,only_if_is_dirty=False);return m

materials=json.loads((SRC/'material-index.json').read_text())
for name,info in materials.items():
    if upgrade_only:
        if any(name.startswith('M_PH_'+n+'_') for n in upgrade_ids) or name in ['M_LodgeRugWeave','M_LodgeRugBorder','M_LodgeRugFringe','M_LodgeFloor','M_LodgeTimber']:material(name,info)
        continue
    if lodge_shell_only:continue
    if lodge_details_only:
        if name in lodge_materials:material(name,info)
        continue
    if (not details_only and not architecture_only) or (details_only and name in ['M_AcornShell','M_AcornCupule']) or (architecture_only and not name.startswith('M_PH_')):material(name,info)
for item in json.loads((SRC/'model-index.json').read_text()):
    name=item['name'];file=SRC/'Export'/(name+'.fbx')
    if upgrade_only and name not in ['SM_'+n for n in upgrade_ids]+['SM_LodgeHeritageRug']:continue
    if lodge_shell_only and name!='SM_LodgeArchitecture':continue
    if lodge_details_only and name not in lodge_meshes:continue
    if details_only and name not in ['SM_AcornDetailed','SM_ReadyArch']:continue
    if architecture_only and name.startswith('SM_') and name not in ['SM_LodgeArchitecture','SM_HollowArchitecture','SM_LodgeRug','SM_TrophyPlaque','SM_AcornDetailed','SM_ReadyArch']:continue
    options=ue.FbxImportUI();options.automated_import_should_detect_type=False;options.import_as_skeletal=False;options.import_materials=False;options.import_textures=False;options.import_animations=False;options.mesh_type_to_import=ue.FBXImportType.FBXIT_STATIC_MESH
    options.static_mesh_import_data.combine_meshes=True;options.static_mesh_import_data.auto_generate_collision=False;options.static_mesh_import_data.normal_import_method=ue.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS
    task=ue.AssetImportTask();task.factory=ue.FbxFactory();task.filename=str(file);task.destination_path=BASE+'/Meshes';task.destination_name=name;task.automated=True;task.replace_existing=True;task.replace_existing_settings=True;task.options=options;task.save=True
    assets.import_asset_tasks([task]);mesh=eal.load_asset(BASE+'/Meshes/'+name);assert mesh,name
    slot_names=[]
    for index,slot in enumerate(mesh.static_materials):
        key=str(slot.material_slot_name).split('.')[0];assert key in materials,(name,key)
        mesh.set_material(index,eal.load_asset(BASE+'/Materials/'+key));slot_names.append(key)
    # Nanite keeps the authored surface geometry on opaque scanned assets. Glass
    # props retain conventional rendering, since translucent sections need it.
    nanite=mesh.get_editor_property('nanite_settings');nanite.set_editor_property('enabled',not any(materials[k].get('alpha')=='BLEND' for k in slot_names));mesh.set_editor_property('nanite_settings',nanite)
    # Architecture uses separate cheap invisible walls; props use static triangle
    # collision so hollow baskets/rocking chairs do not become solid convex boxes.
    body=mesh.get_editor_property('body_setup');body.set_editor_property('collision_trace_flag',ue.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE);body.set_editor_property('double_sided_geometry',True)
    eal.save_loaded_asset(mesh,only_if_is_dirty=False)
    bounds=mesh.get_bounds();report.append({'name':name,'slots':slot_names,'nanite':nanite.enabled,'extent_cm':[bounds.box_extent.x,bounds.box_extent.y,bounds.box_extent.z]})
    ue.log('LOBBY HIGH DETAIL IMPORT '+name+' '+str(report[-1]))
if details_only or architecture_only or lodge_details_only or lodge_shell_only or upgrade_only:
    old=json.loads((SRC/'unreal-import.json').read_text());old=[r for r in old if r['name'] not in [n['name'] for n in report]];report=old+report
(SRC/'unreal-import.json').write_text(json.dumps(report,indent=2))
ue.log('LOBBY HIGH DETAIL IMPORT COMPLETE '+str(len(report))+' meshes; '+str(len(cache))+' texture imports')
ue.SystemLibrary.quit_editor()
