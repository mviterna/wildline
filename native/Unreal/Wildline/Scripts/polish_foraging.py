"""Cook solid stone hulls and refresh the saved reserve's forage patches."""
import unreal as ue

eal = ue.EditorAssetLibrary
meshsub = ue.get_editor_subsystem(ue.StaticMeshEditorSubsystem)
stone = eal.load_asset('/Game/Wildline/Forest/Meshes/SM_Stone')
assert stone, 'Stone mesh is missing'
assert meshsub.set_convex_decomposition_collisions(stone, 1, 24, 100000)
assert meshsub.get_convex_collision_count(stone) > 0
eal.save_loaded_asset(stone)
ue.log('FORAGE POLISH: cooked convex stone collision')

levels = ue.get_editor_subsystem(ue.LevelEditorSubsystem)
assert levels.load_level('/Game/Maps/Blackwood')
actors = ue.get_editor_subsystem(ue.EditorActorSubsystem)
for actor in actors.get_all_level_actors():
    if actor.get_class().get_name() == 'WildlineWorld':
        actor.rebuild()
        ue.log('FORAGE POLISH: rebuilt clear forage patches and collidable rocks')
assert levels.save_current_level()
ue.log('FORAGE POLISH: saved refreshed reserve successfully')
ue.SystemLibrary.quit_editor()
