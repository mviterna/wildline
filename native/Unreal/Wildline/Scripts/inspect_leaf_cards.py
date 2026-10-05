import bpy, json
from pathlib import Path
root=Path(__file__).resolve().parent.parent/'ArtSource/Forest'
bpy.ops.wm.open_mainfile(filepath=str(root/'Broadleaf/trees_grass_system.blend'),use_scripts=False)
for name in ['oaktree1','oaktree2','oaktree3']:
    obj=bpy.data.objects[name]
    for i,mat in enumerate(obj.data.materials):
        ps=[p for p in obj.data.polygons if p.material_index==i]
        print('CARD_DATA',name,mat.name,len(ps),[(len(p.vertices),[tuple(round(x,3) for x in obj.data.uv_layers.active.data[l].uv) for l in p.loop_indices]) for p in ps[:2]])
