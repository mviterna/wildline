import bpy, json
from pathlib import Path
root=Path(__file__).resolve().parent.parent/'ArtSource/Hunters'
bpy.ops.wm.open_mainfile(filepath=str(root/'SK_HunterMale.blend'))
obj=bpy.data.objects['SK_HunterMale']
print('TRAVIS_SOURCE',obj.matrix_world,[(m.name,i) for i,m in enumerate(obj.data.materials)])
for i,m in enumerate(obj.data.materials):
    vs={vi for p in obj.data.polygons if p.material_index==i for vi in p.vertices}
    coords=[obj.data.vertices[v].co for v in vs]
    print('PART',m.name,len(vs),[(min(v[a] for v in coords),max(v[a] for v in coords)) for a in range(3)])
print('BONES', [(b.name,tuple(b.head_local)) for b in obj.parent.data.bones if any(s in b.name.lower() for s in ['head','spine','thigh','calf','upperarm'])])
