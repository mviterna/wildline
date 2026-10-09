"""Orient course oaks so their solid limbs leave the fence descent open."""
from pathlib import Path
import json,unreal as ue
root=Path(ue.Paths.project_dir()).resolve();eal=ue.EditorAssetLibrary
levels=ue.get_editor_subsystem(ue.LevelEditorSubsystem);actors=ue.get_editor_subsystem(ue.EditorActorSubsystem)
for key,name in [('MapleCrescent','Neighborhood'),('CedarBackyards','NeighborhoodCedar')]:
    file=root/'ArtSource/Neighborhood/Configs'/(key+'.json');data=json.loads(file.read_text())
    for p in data['props']:
        if 'SM_Oak' in p['mesh'] and not p['collision']:p['yaw']=180
    config=eal.load_asset('/Game/Wildline/Residential/Configs/DA_'+key);props=list(config.get_editor_property('props'))
    for p in props:
        if 'SM_Oak' in p.mesh and not p.collision:
            ue.log('COURSE OAK BEFORE '+str(p.transform))
            t=p.transform
            p.set_editor_property('transform',ue.Transform(location=t.translation,rotation=ue.Rotator(pitch=0,yaw=180,roll=0),scale=t.scale3d))
            ue.log('COURSE OAK AFTER '+str(p.transform))
    config.set_editor_property('props',props);eal.save_loaded_asset(config,only_if_is_dirty=False)
    for p in config.get_editor_property('props'):
        if 'SM_Oak' in p.mesh and not p.collision:
            assert abs((p.transform.rotation.rotator().yaw-180+180)%360-180)<.01
            ue.log('COURSE OAK SAVED '+str(p.transform))
    file.write_text(json.dumps(data,indent=2))
    assert levels.load_level('/Game/Maps/'+name)
    for a in actors.get_all_level_actors():
        if a.get_class().get_name()=='WildlineWorld':a.rebuild()
    assert levels.save_current_level();ue.log('COURSE OAK ROTATED '+name)
ue.SystemLibrary.quit_editor()
