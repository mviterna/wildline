"""Import seasonal wood, trophy busts, measured Fab dressing, and Frostwood."""
from pathlib import Path
import unreal as ue,json,math,re
root=Path(ue.Paths.project_dir()).resolve();assets=ue.AssetToolsHelpers.get_asset_tools();eal=ue.EditorAssetLibrary
mel=ue.MaterialEditingLibrary;meshes=ue.get_editor_subsystem(ue.StaticMeshEditorSubsystem)
forest='/Game/Wildline/Forest'
helpers=(root/'Scripts/import_forest.py').read_text();base=forest
exec(helpers[helpers.index('def node('):helpers.index('for name,dry in')])

def finish_material(m):
    mel.set_material_usage(m,ue.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES);mel.set_material_usage(m,ue.MaterialUsage.MATUSAGE_NANITE);mel.recompile_material(m);eal.save_loaded_asset(m,only_if_is_dirty=False)

# Fine granular snow with broad wind-sculpted ripples, no leaf-litter texture.
m=material('M_Snow');mel.delete_all_material_expressions(m)
p=node(m,ue.MaterialExpressionWorldPosition)
c=custom(m,'float r=sin(P.x*.023+sin(P.y*.011)*1.7)*.012+sin(P.y*.057)*.006;float grain=frac(sin(dot(floor(P.xy*1.6),float2(12.9898,78.233)))*43758.5453);return float3(.79,.85,.91)*(1+r)+(grain-.5)*.025;',{'P':p})
prop(c,ue.MaterialProperty.MP_BASE_COLOR);constant(m,.86,ue.MaterialProperty.MP_ROUGHNESS);constant(m,.24,ue.MaterialProperty.MP_SPECULAR)
prop(custom(m,'return normalize(float3(cos(P.x*.023+sin(P.y*.011)*1.7)*.045,cos(P.y*.057)*.025,1));',{'P':p}),ue.MaterialProperty.MP_NORMAL);finish_material(m)
for name,original in [('M_FrostBark','M_TreeBark'),('M_FrostStone','M_Stone')]:
    m=material(name);mel.delete_all_material_expressions(m)
    wood=eal.load_asset(forest+'/Materials/'+original)
    tex=eal.load_asset('/Game/Wildline/Textures/T_Bark');ntex=eal.load_asset('/Game/Wildline/Textures/T_Bark_N')
    p=node(m,ue.MaterialExpressionWorldPosition);n=node(m,ue.MaterialExpressionVertexNormalWS)
    uv=custom(m,'return float2(P.x*.010+P.y*.007,P.z*.004);',{'P':p},ue.CustomMaterialOutputType.CMOT_FLOAT2)
    s=sample(m,tex,uv)
    prop(custom(m,'float snow=smoothstep(.22,.8,N.z)*( .82+.10*sin(P.x*.026)*sin(P.y*.024));return lerp(C,float3(.78,.84,.90),snow);',{'C':(s,'RGB'),'N':n,'P':p}),ue.MaterialProperty.MP_BASE_COLOR)
    prop(sample(m,ntex,uv),ue.MaterialProperty.MP_NORMAL,'RGB');constant(m,.93,ue.MaterialProperty.MP_ROUGHNESS);finish_material(m)

def import_static(file,folder,mapping,complex_collision=False):
    opt=ue.FbxImportUI();opt.automated_import_should_detect_type=False;opt.import_as_skeletal=False;opt.mesh_type_to_import=ue.FBXImportType.FBXIT_STATIC_MESH
    opt.import_materials=False;opt.import_textures=False;opt.import_animations=False
    opt.static_mesh_import_data.combine_meshes=True;opt.static_mesh_import_data.auto_generate_collision=False
    opt.static_mesh_import_data.normal_import_method=ue.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS
    t=ue.AssetImportTask();t.filename=str(file);t.destination_path=folder;t.destination_name=file.stem;t.automated=True;t.replace_existing=True;t.save=True;t.options=opt
    assets.import_asset_tasks([t]);mesh=eal.load_asset(folder+'/'+file.stem);assert mesh,file
    for i,slot in enumerate(mesh.get_editor_property('static_materials')):
        key=str(slot.material_slot_name);path=next((v for k,v in mapping.items() if key==k or key.startswith(k+'_')),None)
        assert path,(file,key);mesh.set_material(i,eal.load_asset(path))
    ns=mesh.get_editor_property('nanite_settings');ns.set_editor_property('enabled',False);mesh.set_editor_property('nanite_settings',ns)
    if complex_collision:
        mesh.get_editor_property('body_setup').set_editor_property('collision_trace_flag',ue.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    else:
        reductions=[]
        for fraction,screen in [(1.,1.),(.7,.25),(.4,.08),(.18,.025)]:
            r=ue.EditorScriptingMeshReductionSettings();r.percent_triangles=fraction;r.screen_size=screen;reductions.append(r)
        lod=ue.EditorScriptingMeshReductionOptions();lod.auto_compute_lod_screen_size=False;lod.reduction_settings=reductions;meshes.set_lods(mesh,lod)
    eal.save_loaded_asset(mesh,only_if_is_dirty=False);return mesh

maps_only='WorldUpgradeMapsOnly' in ue.SystemLibrary.get_command_line()
placement_only=maps_only or 'WorldUpgradePlacementOnly' in ue.SystemLibrary.get_command_line()
for row in ([] if placement_only else json.loads((root/'ArtSource/Forest/Seasonal/manifest.json').read_text())):
    name=row['mesh'];mapping={'F_Bark':forest+'/Materials/'+('M_FrostBark' if name.endswith('_Bare') else 'M_TreeBark'),'F_Leaf':forest+'/Materials/M_Leaves','F_LivingLeaf':forest+'/Materials/M_LivingLeaf','F_Needles':forest+'/Materials/M_Needles'}
    import_static(root/'ArtSource/Forest/Seasonal'/(name+'.fbx'),forest+'/Meshes',mapping,name.endswith('_Wood'))
    ue.log('SEASONAL IMPORT '+name)
for person in ([] if placement_only else ['Travis','Ethan','Greyson']):
    mapping={'H_Skin':'M_Skin_male','H_Cloth':'M_TravisCamo','H_Boots':'M_Boots','H_Cap':'M_Cap','H_Vest':'M_TravisVest',f'H_{person}Photo':f'M_{person}Photo',f'H_{person}Skin':f'M_{person}Skin',f'H_{person}Hair':f'M_{person}Hair'}
    import_static(root/f'ArtSource/Hunters/Export/SM_HunterTrophy{person}.fbx','/Game/Wildline/Hunters/Meshes',{k:'/Game/Wildline/Hunters/Materials/'+v for k,v in mapping.items()})

report=[]
def object_path(p):return p+'.'+p.rsplit('/',1)[1]
cars=['/Game/CitySampleVehicles/vehicle02_Car/Mesh/SM_vehCar_vehicle02_LOD','/Game/CitySampleVehicles/vehicle03_Car/Mesh/SM_vehCar_vehicle03_LOD','/Game/CitySampleVehicles/vehicle04_Truck/Mesh/SM_vehTruck_vehicle04_LOD','/Game/CitySampleVehicles/vehicle01_Van/Mesh/SM_vehVan_vehicle01_LOD']
def measured_prop(path,position,yaw=0,length=None,solid=True):
    selected='/Game/Wildline/Fab/Meshes/'+path.rsplit('/',1)[1]
    if not eal.does_asset_exist(selected):assert eal.duplicate_asset(path,selected)
    path=selected
    mesh=eal.load_asset(path);assert mesh,path
    b=mesh.get_bounds();ext=b.box_extent;centre=b.origin
    scale=length/(max(ext.x,ext.y)*2) if length else 1
    a=math.radians(yaw);cx=centre.x*math.cos(a)-centre.y*math.sin(a);cy=centre.x*math.sin(a)+centre.y*math.cos(a)
    # Desired point is the ground contact at the centre, irrespective of pivot.
    pos=[position[0]-cx*scale,position[1]-cy*scale,position[2]-(centre.z-ext.z)*scale]
    prop=ue.WildlineNeighborhoodProp();prop.set_editor_property('mesh',object_path(path));prop.set_editor_property('transform',ue.Transform(location=ue.Vector(*pos),rotation=ue.Rotator(pitch=0,yaw=yaw,roll=0),scale=ue.Vector(scale,scale,scale)));prop.set_editor_property('collision',solid)
    if solid and meshes.get_simple_collision_count(mesh)==0:
        if 'veh' in path:assert meshes.add_simple_collisions(mesh,ue.ScriptingCollisionShapeType.BOX)>=0,path
        else:assert meshes.set_convex_decomposition_collisions(mesh,8,24,100000),path
        eal.save_loaded_asset(mesh,only_if_is_dirty=False)
    report.append({'mesh':path,'ground_contact':position,'scale':scale,'position':pos,'yaw':yaw,'collision':solid,'extent':[ext.x,ext.y,ext.z]});return prop

for key in ([] if maps_only else ['MapleCrescent','CedarBackyards']):
    first_report=len(report)
    config=eal.load_asset('/Game/Wildline/Residential/Configs/DA_'+key);assert config
    # Idempotent: keep authored routes, replace only this dressing layer.
    props=[p for p in config.get_editor_property('props') if not p.mesh.startswith(('/Game/City','/Game/Wildline/Fab/'))]
    props=[p for p in props if 'SM_painted_wooden_bench' not in p.mesh]
    for i,(x,y,yaw) in enumerate([(-4400,-3945,0),(-2100,-4650,180),(1400,-3945,0),(4300,-4650,180),(-2250,1400,90),(3850,-3450,90)]):
        props.append(measured_prop(cars[i%4],[x,y,215 if y<-3800 else 212],yaw,510 if i%4==2 else 455))
    for i,x in enumerate([-3000,2500]):
        props.append(measured_prop('/Game/CityPark/Meshes/Props/SM_Bench03',[x,1150,210],180,170))
        props.append(measured_prop('/Game/CityPark/Meshes/Props/SM_TrashCan02',[x+150,1150,210],0,45))
    for x in [-4900,5000]:props.append(measured_prop('/Game/CityPark/Meshes/Props/SM_LampPost01',[x,-3565,210],0,None,True))
    props.append(measured_prop('/Game/CityPark/Meshes/Props/SM_DrinkingFountain',[3300,1100,210],90,70))
    config.set_editor_property('props',props);eal.save_loaded_asset(config,only_if_is_dirty=False)
    source=root/'ArtSource/Neighborhood/Configs'/(key+'.json');data=json.loads(source.read_text())
    data['props']=[p for p in data['props'] if not p['mesh'].startswith(('/Game/City','/Game/Wildline/Fab/')) and 'SM_painted_wooden_bench' not in p['mesh']]
    data['props'] += [{'mesh':object_path(p['mesh']),'position':p['position'],'scale':[p['scale']]*3,'yaw':p['yaw'],'collision':p['collision']} for p in report[first_report:]]
    source.write_text(json.dumps(data,indent=2))
if report:(root/'ArtSource/Neighborhood/fab-placement.json').write_text(json.dumps(report,indent=2))

levels=ue.get_editor_subsystem(ue.LevelEditorSubsystem);actors=ue.get_editor_subsystem(ue.EditorActorSubsystem)
selected_map=re.search(r'-WorldUpgradeMap=(\w+)',ue.SystemLibrary.get_command_line())
for name in ([selected_map.group(1)] if selected_map else ['Blackwood','Farmstead','Neighborhood','NeighborhoodCedar','Frostwood']):
    if name=='Frostwood' and not eal.does_asset_exist('/Game/Maps/Frostwood'):
        assert levels.new_level_from_template('/Game/Maps/Frostwood','/Game/Maps/Blackwood')
    else:assert levels.load_level('/Game/Maps/'+name)
    for a in actors.get_all_level_actors():
        if a.get_class().get_name()=='WildlineWorld':
            a.set_editor_property('bWinter',name=='Frostwood');a.rebuild()
            if name=='Frostwood':
                a.set_actor_label('Frostwood / leafless Classic reserve')
                components=a.get_components_by_class(ue.HierarchicalInstancedStaticMeshComponent)
                assert sum(c.get_instance_count() for c in components if c.get_name().startswith('Climbable wood'))>100
                for c in components:
                    if c.get_name().startswith('Woodland species') and 4<=int(c.get_name().rsplit(' ',1)[-1])<17:assert c.get_instance_count()==0
        if name=='Frostwood' and isinstance(a,ue.DirectionalLight):
            a.set_actor_rotation(ue.Rotator(pitch=-24,yaw=-55,roll=0),False);a.light_component.set_light_color(ue.LinearColor(.79,.87,1));a.light_component.set_intensity(9000 if 'sun' in a.get_actor_label().lower() else 500)
        if name=='Frostwood' and isinstance(a,ue.SkyLight):a.light_component.set_intensity(.9)
        if name=='Frostwood' and isinstance(a,ue.ExponentialHeightFog):
            a.component.set_editor_property('fog_density',.012)
    assert levels.save_current_level();ue.log('WORLD UPGRADE MAP READY '+name)
ue.log('WORLD UPGRADE IMPORT COMPLETE')
exec(compile((root/'Scripts/finalize_world_upgrade.py').read_text(),'finalize_world_upgrade.py','exec'))
