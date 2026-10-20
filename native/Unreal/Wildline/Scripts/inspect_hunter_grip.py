import json
from pathlib import Path
import unreal as ue

clip = ue.load_asset('/Game/Characters/Mannequins/Anims/Rifle/MF_Rifle_Idle_ADS')
pose = ue.AnimPoseExtensions.get_anim_pose_at_time(clip, 0, ue.AnimPoseEvaluationOptions())
report = {}
for side in ['r', 'l']:
    hand = ue.AnimPoseExtensions.get_bone_pose(pose, 'hand_' + side, ue.AnimPoseSpaces.WORLD)
    report[side] = {'hand': str(hand)}
    for name in ['index_01', 'index_02', 'index_03', 'middle_01', 'middle_02', 'middle_03', 'pinky_01', 'thumb_01', 'thumb_02', 'thumb_03']:
        bone = ue.AnimPoseExtensions.get_bone_pose(pose, name + '_' + side, ue.AnimPoseSpaces.WORLD)
        report[side][name] = str(ue.MathLibrary.inverse_transform_location(hand, bone.translation))
Path(__file__).resolve().parent.parent.joinpath('Saved/hunter-grip-inspection.json').write_text(json.dumps(report, indent=2))
ue.log('HUNTER GRIP INSPECTED')
