import bpy
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parent.parent/'ArtSource/Hunters'
bpy.ops.wm.open_mainfile(filepath=str(root/'SK_HunterMale.blend'),use_scripts=False)
obj=bpy.data.objects['SK_HunterMale'];arm=obj.parent
print('RIG',arm.matrix_world,'POSE',arm.data.pose_position)
print('UV',[(u.name,u.active_render) for u in obj.data.uv_layers])
for point in [(.1,0,.5),(.13,0,.15),(.3,0,1.25),(.45,-.2,1),(.05,-.13,1.65),(0,0,1.25)]:
    v=min(obj.data.vertices,key=lambda v:(v.co-Vector(point)).length)
    print('WEIGHT',point,tuple(v.co),[(obj.vertex_groups[g.group].name,g.weight) for g in v.groups])
deps=bpy.context.evaluated_depsgraph_get();evaluated=obj.evaluated_get(deps)
print('DEFORM',[tuple(evaluated.data.vertices[i].co-obj.data.vertices[i].co) for i in [100,3000,5000,9000]])
print('POSE_BONES',[(b.name,tuple(b.matrix.translation),tuple(b.bone.matrix_local.translation)) for b in arm.pose.bones if b.name in ['head','spine_01','thigh_l','calf_l','upperarm_l','lowerarm_l','foot_l']])
