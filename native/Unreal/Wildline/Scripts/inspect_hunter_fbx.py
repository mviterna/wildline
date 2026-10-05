import bpy
from pathlib import Path
root=Path(__file__).resolve().parent.parent/'ArtSource/Hunters'
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(root/'Export/SK_HunterTravis.fbx'))
for o in bpy.context.selected_objects:
    if o.type=='MESH':print('FBX_SHAPES',o.name,[k.name for k in o.data.shape_keys.key_blocks] if o.data.shape_keys else [],'UV',[(u.name,u.active_render) for u in o.data.uv_layers])
