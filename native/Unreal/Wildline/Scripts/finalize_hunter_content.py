import unreal as ue
eal=ue.EditorAssetLibrary
for path in eal.list_assets('/Game/Wildline/Hunters/Materials',recursive=True):
    m=eal.load_asset(path)
    if isinstance(m,ue.Material):ue.MaterialEditingLibrary.set_material_usage(m,ue.MaterialUsage.MATUSAGE_SKELETAL_MESH);ue.MaterialEditingLibrary.recompile_material(m);eal.save_loaded_asset(m)
log=eal.load_asset('/Game/Wildline/Forest/Meshes/SM_Log');ue.log('LOG HULL COUNT '+str(ue.EditorStaticMeshLibrary.get_convex_collision_count(log)))
for name in ['SK_HunterMale','SK_HunterFemale']:
    mesh=eal.load_asset('/Game/Wildline/Hunters/Meshes/'+name);ue.log('HUNTER SKELETON '+name+' '+mesh.get_editor_property('skeleton').get_path_name())
levels=ue.get_editor_subsystem(ue.LevelEditorSubsystem);levels.load_level('/Game/Maps/Blackwood')
for actor in ue.get_editor_subsystem(ue.EditorActorSubsystem).get_all_level_actors():
    if actor.get_class().get_name()=='WildlineWorld':actor.rebuild()
levels.save_current_level();ue.log('HUNTER CONTENT FINALIZED');ue.SystemLibrary.quit_editor()
