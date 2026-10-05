import unreal as ue

for body in ['Male', 'Female', 'Travis']:
    mesh = ue.load_asset('/Game/Wildline/Hunters/Meshes/SK_Hunter' + body)
    sk = mesh.get_editor_property('skeleton')
    modes = sk.get_editor_property('bone_tree')
    ue.log('HUNTER BODY SKELETON ' + body + ' ' + sk.get_path_name() + ' ' + str([n.get_editor_property('translation_retargeting_mode') for n in modes[:8]]))

s = ue.load_asset('/Game/Characters/Mannequins/Meshes/SK_Mannequin')
tree = s.get_editor_property('bone_tree')
for i, node in enumerate(tree):
    ue.log('HUNTER RETARGET MODE ' + str(i) + ' ' + str(node.get_editor_property('translation_retargeting_mode')))
ue.log('HUNTER RETARGET LIBRARY ' + str([x for x in dir(ue.AnimationLibrary) if 'retarget' in x.lower()]))
a = ue.load_asset('/Game/Wildline/Hunters/Animations/A_Hunter_Crouch_Fwd')
for prop in ['retarget_source', 'retarget_source_asset', 'bone_compression_settings', 'additive_anim_type']:
    ue.log('HUNTER ANIMATION PROPERTY ' + prop + ' ' + str(a.get_editor_property(prop)))
for mode in [ue.AnimDataEvalType.RAW, ue.AnimDataEvalType.COMPRESSED]:
    opt = ue.AnimPoseEvaluationOptions()
    opt.evaluation_type = mode
    opt.should_retarget = True
    pose = ue.AnimPoseExtensions.get_anim_pose_at_time(a, 1.455, opt)
    ue.log('HUNTER EVALUATION ' + str(mode) + ' ' + str(ue.AnimPoseExtensions.get_bone_pose(pose, 'pelvis', ue.AnimPoseSpaces.WORLD)))
