"""Run in Epic's Game Animation Sample with the Python/Editor Scripting plugins.

Bake the supplied UEFN-to-Manny IK retargeter, export selected captured motions,
and retain root travel and a common left-foot gait phase for runtime timing.
"""
import json
import math
import statistics
from pathlib import Path
import unreal as ue

root = Path(__file__).resolve().parent.parent
out = root / 'ArtSource/Hunters/GameAnimationSample'
(out / 'Export').mkdir(parents=True, exist_ok=True)
eal = ue.EditorAssetLibrary
lib = ue.AnimationLibrary
selected = []
directions = ['F', 'FR', 'RR', 'BR', 'B', 'BL', 'LL', 'FL']
labels = ['Fwd', 'FwdRight', 'Right', 'BwdRight', 'Bwd', 'BwdLeft', 'Left', 'FwdLeft']
for folder, prefix, label in [('Walk', 'M_Neutral_Walk', 'Walk'), ('Run', 'M_Neutral_Run', 'Run'), ('Crouch', 'M_Neutral_Crouch', 'Crouch')]:
    for d, l in zip(directions, labels):
        selected.append((folder, prefix + '_Loop_' + d, 'A_Hunter_' + label + '_' + l, True))
for folder, name, label in [
    ('Idle', 'M_Neutral_Stand_Idle_Loop', 'Idle'),
    ('Idle', 'M_Neutral_Crouch_Idle_Loop', 'CrouchIdle'),
    ('Idle', 'M_Neutral_Stand_Turn_090_L', 'TurnLeft'),
    ('Idle', 'M_Neutral_Stand_Turn_090_R', 'TurnRight'),
    ('Jump', 'M_Neutral_Jump_F_Start_Stand_Lfoot', 'JumpStart'),
    ('Jump', 'M_Neutral_Jump_Loop_Fall', 'Fall'),
    ('Jump', 'M_Neutral_Jump_F_Land_Stand_Light_Lfoot', 'Land'),
]:
    selected.append((folder, name, 'A_Hunter_' + label, False))
for folder in ['Walk', 'Run']:
    for action in ['Start', 'Stop']:
        for d, l in zip(directions, labels):
            selected.append((folder, 'M_Neutral_' + folder + '_' + action + '_' + d + '_Lfoot',
                             'A_Hunter_' + folder + action + '_' + l, False))

assets = [ue.load_asset('/Game/Characters/UEFN_Mannequin/Animations/' + f + '/' + n) for f, n, _, _ in selected]
assert all(assets), [x for x, a in zip(selected, assets) if not a]
inputs = ue.IKRetargetBatchOperationInputs()
inputs.assets_to_retarget = [eal.find_asset_data(a.get_path_name()) for a in assets]
inputs.source_mesh = ue.load_asset('/Game/Characters/UEFN_Mannequin/Meshes/SKM_UEFN_Mannequin')
inputs.target_mesh = ue.load_asset('/Game/Characters/UE5_Mannequins/Meshes/SKM_Manny_Simple')
inputs.ik_retarget_asset = ue.load_asset('/Game/Characters/UE5_Mannequins/Rigs/RTG_UEFN_to_UE5_Mannequin')
inputs.target_path = '/Game/WildlineRetarget'
inputs.include_referenced_assets = False
inputs.overwrite_existing_files = True
results = ue.IKRetargetBatchOperation.run_batch_retarget(inputs)
assert len(results) == len(selected), (len(results), len(selected))

opts = ue.AnimPoseEvaluationOptions()
opts.evaluation_type = ue.AnimDataEvalType.RAW
opts.extract_root_motion = False
opts.incorporate_root_motion_into_pose = True
report = []
for folder, source, name, loop in selected:
    a = ue.load_asset('/Game/WildlineRetarget/' + source)
    assert a, source
    a.set_editor_property('force_root_lock', False)
    a.set_editor_property('enable_root_motion', False)
    length = a.sequence_length
    first = lib.get_bone_pose_for_time(a, 'root', 0, False)
    last = lib.get_bone_pose_for_time(a, 'root', length, False)
    delta = last.translation - first.translation
    distance = math.hypot(delta.x, delta.y)
    metadata = dict(source='/Game/Characters/UEFN_Mannequin/Animations/' + folder + '/' + source,
                    name=name, length=length, distance=distance, root_delta=[delta.x, delta.y, delta.z])
    if loop:
        count = round(length * 60)
        # Use the left foot's fore/aft motion relative to the moving root. All
        # direction clips start at the same gait event, preventing phase mixing.
        signal = []
        axis = delta / max(distance, 1)
        for i in range(count + 1):
            pose = ue.AnimPoseExtensions.get_anim_pose_at_time(a, i * length / count, opts)
            foot = ue.AnimPoseExtensions.get_bone_pose(pose, 'foot_l', ue.AnimPoseSpaces.WORLD).translation
            r = ue.AnimPoseExtensions.get_bone_pose(pose, 'root', ue.AnimPoseSpaces.WORLD).translation
            v = foot - r
            signal.append(v.x * axis.x + v.y * axis.y)
        peaks = [i for i in range(2, count - 2) if signal[i] == max(signal[i-2:i+3])]
        clean = []
        for i in peaks:
            if not clean or i - clean[-1] > 20:
                clean.append(i)
            elif signal[i] > signal[clean[-1]]:
                clean[-1] = i
        assert len(clean) >= 2, (source, clean)
        period = statistics.median([b-a for a, b in zip(clean, clean[1:])]) * length / count
        offset = clean[0] * length / count
        metadata.update(period=period, offset=offset, stride=distance * period / length, peaks=clean)
    export_opts = ue.FbxExportOption()
    export_opts.set_editor_property('export_preview_mesh', False)
    export_opts.set_editor_property('force_front_x_axis', False)
    task = ue.AssetExportTask()
    task.object = a
    task.filename = str(out / 'Export' / (name + '.fbx'))
    task.automated = True
    task.prompt = False
    task.replace_identical = True
    task.options = export_opts
    task.exporter = ue.AnimSequenceExporterFBX()
    assert ue.Exporter.run_asset_export_task(task), name
    report.append(metadata)
    ue.log('HUNTER CAPTURE ' + json.dumps(metadata))
(out / 'motions.json').write_text(json.dumps(report, indent=2))
ue.log('HUNTER CAPTURE EXPORT COMPLETE ' + str(len(report)))
