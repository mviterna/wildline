import json
from pathlib import Path
import unreal as ue

report = {}
for name, path in [
    ('rifle', '/Game/Characters/Mannequins/Anims/Rifle/MF_Rifle_Idle_ADS'),
    ('capture', '/Game/Wildline/Hunters/Animations/A_Hunter_Idle'),
    ('walk', '/Game/Wildline/Hunters/Animations/A_Hunter_Walk_Fwd'),
    ('crouch_idle', '/Game/Wildline/Hunters/Animations/A_Hunter_CrouchIdle'),
    ('crouch_walk', '/Game/Wildline/Hunters/Animations/A_Hunter_Crouch_Fwd'),
    ('stop', '/Game/Wildline/Hunters/Animations/A_Hunter_WalkStop_Fwd'),
]:
    a = ue.load_asset(path)
    a.set_editor_property('force_root_lock', False)
    pose = ue.AnimPoseExtensions.get_anim_pose_at_time(a, 0, ue.AnimPoseEvaluationOptions())
    report[name] = {}
    for bone in ['root', 'pelvis', 'spine_01', 'spine_02', 'clavicle_l', 'upperarm_l', 'head', 'foot_l']:
        report[name][bone] = {space: str(ue.AnimPoseExtensions.get_bone_pose(pose, bone, enum))
                              for space, enum in [('local', ue.AnimPoseSpaces.LOCAL), ('world', ue.AnimPoseSpaces.WORLD)]}
    ue.log('MOTION SPACE ' + name + ' ' + json.dumps(report[name]))
    if name in ['crouch_idle', 'crouch_walk']:
        for t in [0, .9, 1.1, 1.4, 1.5, 2, 3]:
            p = ue.AnimPoseExtensions.get_anim_pose_at_time(a, t, ue.AnimPoseEvaluationOptions())
            ue.log('CROUCH HEIGHT ' + name + ' ' + str(t) + ' ' + str(ue.AnimPoseExtensions.get_bone_pose(p, 'pelvis', ue.AnimPoseSpaces.WORLD).translation))
Path(__file__).resolve().parent.parent.joinpath('ArtSource/Hunters/GameAnimationSample/space-inspection.json').write_text(json.dumps(report, indent=2))
