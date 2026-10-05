import bpy,math
from pathlib import Path
root=Path(r'C:/Users/mvite/OneDrive/Documents/ChatGPT/PhotoRealistic Multiplater/native/Unreal/Wildline/ArtSource/Forest/Export')
for name in ['SM_OakA','SM_OakB','SM_OakC']:
 bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(root/(name+'.fbx')))
 o=next(o for o in bpy.context.selected_objects if o.type=='MESH');points=[o.matrix_world@v.co for v in o.data.vertices]
 zs=sorted(set(round(v.z,3) for v in points if v.z<4));print(name,zs[:40])
 for z in zs[:15]:
  ring=[math.hypot(v.x,v.y) for v in points if abs(v.z-z)<.001];print(z,len(ring),min(ring),max(ring))
