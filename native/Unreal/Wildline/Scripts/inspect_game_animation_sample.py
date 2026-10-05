import json
from pathlib import Path
import unreal as ue

root = Path(__file__).resolve().parent.parent
report = {}
lib = getattr(ue, 'AnimationLibrary', None) or ue.AnimationBlueprintLibrary
for folder, name in [
    ('Walk', 'M_Neutral_Walk_Loop_F'),
    ('Walk', 'M_Neutral_Walk_Loop_RR'),
    ('Run', 'M_Neutral_Run_Loop_F'),
    ('Crouch', 'M_Neutral_Crouch_Loop_F'),
    ('Idle', 'M_Neutral_Stand_Idle_Loop'),
    ('Jump', 'M_Neutral_Jump_F_Start_Stand_Lfoot'),
]:
    a = ue.load_asset('/Game/Characters/UEFN_Mannequin/Animations/' + folder + '/' + name)
    assert a, name
    start = lib.get_bone_pose_for_time(a, 'root', 0, False)
    end = lib.get_bone_pose_for_time(a, 'root', a.sequence_length, False)
    delta = end.translation - start.translation
    report[name] = dict(length=a.sequence_length, root_delta=[delta.x, delta.y, delta.z],
                        root_start=str(start), skeleton=a.get_editor_property('skeleton').get_path_name())
rtg = ue.load_asset('/Game/Characters/UE5_Mannequins/Rigs/RTG_UEFN_to_UE5_Mannequin')
assert rtg
report['retargeter'] = str(rtg)
ue.log('GASP INSPECT ' + json.dumps(report))
out = root / 'ArtSource/Hunters/GameAnimationSample'
out.mkdir(parents=True, exist_ok=True)
(out / 'inspection.json').write_text(json.dumps(report, indent=2))
