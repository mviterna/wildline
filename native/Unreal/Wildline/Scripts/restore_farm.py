"""Restore the procedural Farmstead after the initial neighborhood generation check."""
import unreal as ue
levels=ue.get_editor_subsystem(ue.LevelEditorSubsystem)
actors=ue.get_editor_subsystem(ue.EditorActorSubsystem)
levels.load_level('/Game/Maps/Farmstead')
for actor in actors.get_all_level_actors():
    if actor.get_class().get_name()=='WildlineWorld':
        actor.set_editor_property('bFarm',True);actor.set_editor_property('bResidential',False)
        actor.set_actor_label('Farm Raid / playable homestead');actor.rebuild()
    elif actor.get_class().get_name()=='PlayerStart':
        actor.set_actor_location(ue.Vector(1250,750,320),False,False)
    elif actor.get_class().get_name()=='PointLight':actors.destroy_actor(actor)
for y in [-250,250]:
    light=actors.spawn_actor_from_class(ue.PointLight,ue.Vector(2350,y,460));light.set_actor_label('Pantry warm lamp')
    light.point_light_component.set_editor_property('intensity',50000.)
    light.point_light_component.set_editor_property('attenuation_radius',1100.)
    light.point_light_component.set_editor_property('light_color',ue.Color(255,215,166,255))
    light.point_light_component.set_mobility(ue.ComponentMobility.MOVABLE)
levels.save_current_level();ue.log('FARM RESTORED');ue.SystemLibrary.quit_editor()
