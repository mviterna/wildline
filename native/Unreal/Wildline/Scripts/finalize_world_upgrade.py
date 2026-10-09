"""Cap selected Fab dependency textures and validate rebuilt map assets."""
from pathlib import Path
import unreal as ue,json
root=Path(ue.Paths.project_dir()).resolve();eal=ue.EditorAssetLibrary
registry=ue.AssetRegistryHelpers.get_asset_registry()
options=ue.AssetRegistryDependencyOptions(include_hard_package_references=True,include_soft_package_references=False,include_searchable_names=False,include_soft_management_references=False,include_hard_management_references=False)
pending=eal.list_assets('/Game/Wildline/Fab/Meshes',recursive=True,include_folder=False);seen=set();changed=[]
while pending:
    path=str(pending.pop()).split('.')[0]
    if path in seen:continue
    seen.add(path)
    for dependency in registry.get_dependencies(path,options):
        d=str(dependency)
        if d.startswith(('/Game/CitySampleVehicles/','/Game/CityPark/')):pending.append(d)
    if path.startswith(('/Game/CitySampleVehicles/','/Game/CityPark/')):
        asset=eal.load_asset(path)
        if isinstance(asset,ue.Texture2D):
            asset.set_editor_property('max_texture_size',2048);eal.save_loaded_asset(asset,only_if_is_dirty=False);changed.append(path)
assert changed,'No selected Fab texture dependencies found'
(root/'ArtSource/Neighborhood/fab-texture-budget.json').write_text(json.dumps({'max_texture_size':2048,'textures':sorted(changed)},indent=2))
ue.log('FAB TEXTURE BUDGET READY '+str(len(changed)))
levels=ue.get_editor_subsystem(ue.LevelEditorSubsystem);actors=ue.get_editor_subsystem(ue.EditorActorSubsystem)
for name in ['Blackwood','Farmstead','Neighborhood','NeighborhoodCedar','Frostwood']:
    assert levels.load_level('/Game/Maps/'+name)
    world=next(a for a in actors.get_all_level_actors() if a.get_class().get_name()=='WildlineWorld')
    wood=[c for c in world.get_components_by_class(ue.HierarchicalInstancedStaticMeshComponent) if c.get_name().startswith('Climbable wood')]
    assert len(wood)==4
    assert all(c.get_editor_property('static_mesh') and c.get_instance_count()>0 for c in wood)
    if name.startswith('Neighborhood'):
        props=world.get_editor_property('neighborhood_config').get_editor_property('props')
        assert len([p for p in props if '/Game/Wildline/Fab/' in p.mesh])==13
    assert world.get_editor_property('bWinter')==(name=='Frostwood')
    ue.log('WORLD UPGRADE ASSET VALIDATION '+name)
ue.log('WORLD UPGRADE FINALIZE COMPLETE');ue.SystemLibrary.quit_editor()
