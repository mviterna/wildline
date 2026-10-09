"""Unreal commandlet: import neighborhood art and cook both layout DataAssets.

The proven lobby importer supplies FBX/PBR handling; only its source/destination
are changed. Then both maps are rebuilt from the corresponding config assets.
"""
import unreal as ue, json
from pathlib import Path
ROOT=Path(ue.Paths.project_dir()).resolve();SRC=ROOT/'ArtSource/Neighborhood'
BASE='/Game/Wildline/Residential'
source=(ROOT/'Scripts/import_lobby_assets.py').read_text()
assert "SRC=ROOT/'ArtSource/Lobby'" in source and "BASE='/Game/Wildline/Lobby/Photoreal'" in source
source=source.replace("SRC=ROOT/'ArtSource/Lobby'","SRC=ROOT/'ArtSource/Neighborhood'").replace("BASE='/Game/Wildline/Lobby/Photoreal'","BASE='/Game/Wildline/Residential'")
source=source.replace('ue.SystemLibrary.quit_editor()','').replace('LOBBY HIGH DETAIL','NEIGHBORHOOD HIGH DETAIL')
source=source.replace('eal.save_loaded_asset(m,only_if_is_dirty=False);return m','assert eal.save_loaded_asset(m,only_if_is_dirty=False);return m')
source=source.replace('eal.save_loaded_asset(mesh,only_if_is_dirty=False)','assert eal.save_loaded_asset(mesh,only_if_is_dirty=False)')
paint_shader='''
    if info.get('paint') and color:
        pigment=node(m,ue.MaterialExpressionConstant3Vector);pigment.set_editor_property('constant',ue.LinearColor(*info['paint'],1))
        mix=node(m,ue.MaterialExpressionLinearInterpolate);mel.connect_material_expressions(sample(m,color),'RGB',mix,'A');mel.connect_material_expressions(pigment,'',mix,'B');mel.connect_material_expressions(const(m,.88),'',mix,'Alpha');connect(mix,ue.MaterialProperty.MP_BASE_COLOR)
'''
source=source.replace("    normal=texture(info.get('normal'),normal=True)",paint_shader+"    normal=texture(info.get('normal'),normal=True)")
reset_stale_slots='''
    old_mesh=eal.load_asset(BASE+'/Meshes/'+name) if eal.does_asset_exist(BASE+'/Meshes/'+name) else None
    if old_mesh and any(str(s.material_slot_name).split('.')[0] not in materials for s in old_mesh.static_materials):
        # Only invalid meshes from an interrupted neighborhood import. FBX
        # reimport retains obsolete slot names unless the failed asset is reset.
        assert eal.delete_asset(BASE+'/Meshes/'+name)
'''
source=source.replace('    options=ue.FbxImportUI()',reset_stale_slots+'    options=ue.FbxImportUI()')
source=source.replace("nanite.set_editor_property('enabled',not any(materials[k].get('alpha')=='BLEND' for k in slot_names))", "nanite.set_editor_property('enabled',name not in ['SM_CedarBirdFeeder','SM_BackyardTrampoline','SM_PrivacyFence','SM_CourseBranch','SM_ServiceCable','SM_PottingBench','SM_PlanterSoil'] and not any(materials[k].get('alpha')=='BLEND' for k in slot_names))")
if 'NeighborhoodLawnOnly' in ue.SystemLibrary.get_command_line():
    source=source.replace("lodge_details_only='LodgeDetailsOnly'","lodge_details_only='NeighborhoodLawnOnly'")
    source=source.replace("lodge_materials=['M_LodgeTimber','M_LodgeFloor','M_HearthStone','M_WindowOak','M_CurtainFabric','M_LodgeDetailBrass','M_ClockIvory','M_ClockInk']","lodge_materials=['M_NeighborhoodLawn']")
if 'NeighborhoodSoilOnly' in ue.SystemLibrary.get_command_line():
    source=source.replace("lodge_details_only='LodgeDetailsOnly'","lodge_details_only='NeighborhoodSoilOnly'")
    source=source.replace("lodge_meshes=['SM_LodgeArchitecture','SM_LodgeWindowDressings','SM_LodgeClock','SM_LodgePegRail']","lodge_meshes=['SM_PlanterSoil']")
    source=source.replace("lodge_materials=['M_LodgeTimber','M_LodgeFloor','M_HearthStone','M_WindowOak','M_CurtainFabric','M_LodgeDetailBrass','M_ClockIvory','M_ClockInk']","lodge_materials=['M_PlanterSoil']")
if 'NeighborhoodHouseKitOnly' in ue.SystemLibrary.get_command_line():
    source=source.replace("lodge_details_only='LodgeDetailsOnly'","lodge_details_only='NeighborhoodHouseKitOnly'")
    source=source.replace("lodge_meshes=['SM_LodgeArchitecture','SM_LodgeWindowDressings','SM_LodgeClock','SM_LodgePegRail']","lodge_meshes=[r['name'] for r in json.loads((SRC/'model-index.json').read_text()) if r['name'].startswith('SM_Kit')]")
    source=source.replace("lodge_materials=['M_LodgeTimber','M_LodgeFloor','M_HearthStone','M_WindowOak','M_CurtainFabric','M_LodgeDetailBrass','M_ClockIvory','M_ClockInk']","lodge_materials=['M_NH_HouseKit_'+str(i) for i in range(5)]")
if 'NeighborhoodArchitectureOnly' in ue.SystemLibrary.get_command_line():
    source=source.replace("lodge_details_only='LodgeDetailsOnly'","lodge_details_only='NeighborhoodArchitectureOnly'")
    source=source.replace("lodge_meshes=['SM_LodgeArchitecture','SM_LodgeWindowDressings','SM_LodgeClock','SM_LodgePegRail']","lodge_meshes=['SM_ColonialHouse','SM_CapeCodHouse','SM_CourseBranch']")
    source=source.replace("lodge_materials=['M_LodgeTimber','M_LodgeFloor','M_HearthStone','M_WindowOak','M_CurtainFabric','M_LodgeDetailBrass','M_ClockIvory','M_ClockInk']","lodge_materials=['M_HouseBlue','M_HouseCream','M_HouseTrim','M_HouseShingles','M_HouseFoundation','M_WindowReflection','M_HouseDoor','M_HouseBrick','M_DoorBrass','M_CourseBark']")
if 'NeighborhoodConfigOnly' not in ue.SystemLibrary.get_command_line():
    if 'NeighborhoodMaterialsOnly' in ue.SystemLibrary.get_command_line():
        source=source.replace("for item in json.loads((SRC/'model-index.json').read_text()):","for item in []:")
    exec(compile(source,str(ROOT/'Scripts/import_lobby_assets.py'),'exec'),{})
eal=ue.EditorAssetLibrary;assets=ue.AssetToolsHelpers.get_asset_tools();levels=ue.get_editor_subsystem(ue.LevelEditorSubsystem);actors=ue.get_editor_subsystem(ue.EditorActorSubsystem)
if 'NeighborhoodSoilOnly' in ue.SystemLibrary.get_command_line():
    for name in ['SM_BackyardTrampoline','SM_PrivacyFence','SM_CourseBranch','SM_ServiceCable','SM_PottingBench']:
        mesh=eal.load_asset(BASE+'/Meshes/'+name);ns=mesh.get_editor_property('nanite_settings');ns.set_editor_property('enabled',False);mesh.set_editor_property('nanite_settings',ns);assert eal.save_loaded_asset(mesh,only_if_is_dirty=False)
for file in ([] if 'NeighborhoodMaterialsOnly' in ue.SystemLibrary.get_command_line() else sorted((SRC/'Configs').glob('*.json'))):
    data=json.loads(file.read_text());name='DA_'+data['id'];folder=BASE+'/Configs'
    factory=ue.DataAssetFactory();factory.set_editor_property('data_asset_class',ue.WildlineNeighborhoodConfig)
    config=eal.load_asset(folder+'/'+name) if eal.does_asset_exist(folder+'/'+name) else assets.create_asset(name,folder,ue.WildlineNeighborhoodConfig,factory)
    assert config,name
    config.set_editor_property('display_name',data['display_name']);config.set_editor_property('feeder',ue.Vector(*data['feeder']))
    routes=[]
    for r in data['routes']:
        route=ue.WildlineNeighborhoodRoute();route.set_editor_property('name',r['name']);route.set_editor_property('start',ue.Vector(*r['start']));route.set_editor_property('targets',[ue.Vector(*p) for p in r['targets']]);route.set_editor_property('labels',r['labels']);route.set_editor_property('entry_trunk',ue.Vector(*r['entry_trunk']));route.set_editor_property('entry_climb_top',r['entry_climb_top']);route.set_editor_property('entry_radius',r['entry_radius']);route.set_editor_property('trampoline_launch_z',r['trampoline_launch_z']);routes.append(route)
    config.set_editor_property('routes',routes);props=[]
    for p in data['props']:
        prop=ue.WildlineNeighborhoodProp();prop.set_editor_property('mesh',p['mesh']);prop.set_editor_property('transform',ue.Transform(location=ue.Vector(*p['position']),rotation=ue.Rotator(pitch=p.get('pitch',0),yaw=p['yaw'],roll=0),scale=ue.Vector(*p['scale'])));prop.set_editor_property('collision',p['collision']);props.append(prop)
    config.set_editor_property('props',props);eal.save_loaded_asset(config,only_if_is_dirty=False)
    map_path='/Game/Maps/Neighborhood' if data['id']=='MapleCrescent' else '/Game/Maps/NeighborhoodCedar'
    if not eal.does_asset_exist(map_path):assert eal.duplicate_asset('/Game/Maps/Neighborhood',map_path)
    levels.load_level(map_path)
    for actor in actors.get_all_level_actors():
        if actor.get_class().get_name()=='WildlineWorld':
            actor.set_editor_property('neighborhood_config',config);actor.set_editor_property('bResidential',True);actor.set_editor_property('bFarm',False);actor.set_actor_label(data['display_name']);actor.rebuild()
        elif actor.get_class().get_name()=='DirectionalLight':actor.set_actor_rotation(ue.Rotator(pitch=-38,yaw=-24,roll=0),False)
    levels.save_current_level();ue.log('NEIGHBORHOOD LAYOUT READY '+data['id'])
ue.SystemLibrary.quit_editor()
