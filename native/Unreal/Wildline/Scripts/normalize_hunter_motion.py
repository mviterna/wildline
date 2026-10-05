"""Keep every captured in-place clip on the same capsule origin."""
import unreal as ue

paths = ue.EditorAssetLibrary.list_assets('/Game/Wildline/Hunters/Animations', recursive=True)
count = 0
for path in paths:
    clip = ue.load_asset(path)
    if not isinstance(clip, ue.AnimSequence):
        continue
    clip.set_editor_property('enable_root_motion', False)
    clip.set_editor_property('force_root_lock', True)
    clip.set_editor_property('root_motion_root_lock', ue.RootMotionRootLock.REF_POSE)
    ue.EditorAssetLibrary.save_loaded_asset(clip, only_if_is_dirty=False)
    count += 1
ue.log('HUNTER ORIGIN NORMALIZATION COMPLETE ' + str(count))
