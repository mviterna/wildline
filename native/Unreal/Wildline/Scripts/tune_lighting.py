import unreal as ue
levels=ue.get_editor_subsystem(ue.LevelEditorSubsystem)
levels.load_level('/Game/Maps/Blackwood')
for actor in ue.get_editor_subsystem(ue.EditorActorSubsystem).get_all_level_actors():
    if isinstance(actor, ue.DirectionalLight):
        ue.log(f'SUN intensity={actor.light_component.get_editor_property("intensity")} rotation={actor.get_actor_rotation()}')
        actor.light_component.set_editor_property('forward_shading_priority',1 if actor.get_actor_label()=='Early autumn sun' else 0)
levels.save_current_level()
