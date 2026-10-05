import bpy,json
from pathlib import Path
root=Path(__file__).resolve().parent.parent/'ArtSource/Hunters'
bpy.ops.wm.open_mainfile(filepath=str(root/'HunterSource.blend'),use_scripts=False)
for o in bpy.data.objects:
    if o.type=='MESH':print('HUNTER_MESH',o.name,o.dimensions[:],len(o.data.vertices),[m.name for m in o.data.materials])
    if o.type=='ARMATURE':print('HUNTER_BONES',[(b.name,tuple(round(v,3) for v in o.matrix_world@b.head_local)) for b in o.data.bones][:30])
bpy.ops.import_scene.fbx(filepath=str(root/'MannyReference.fbx'))
for o in bpy.context.selected_objects:
    print('MANNY_OBJECT',o.name,o.type,o.dimensions[:])
    if o.type=='ARMATURE':print('MANNY_BONES',[(b.name,tuple(round(v,3) for v in o.matrix_world@b.head_local)) for b in o.data.bones][:35])
