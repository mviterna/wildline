import bpy,json
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parent.parent/'ArtSource/Neighborhood'
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=str(root/'ModularHouseKitGLTF/scene.gltf'))
rows=[]
for o in bpy.context.scene.objects:
    if o.type!='EMPTY' or not o.name.startswith(('SM_','Object','SK_','Door')):continue
    parts=[p for p in o.children_recursive if p.type=='MESH']
    if not parts:continue
    vs=[p.matrix_world@Vector(v) for p in parts for v in p.bound_box]
    lo=[min(v[i] for v in vs) for i in range(3)];hi=[max(v[i] for v in vs) for i in range(3)]
    rows.append({'name':o.name,'parts':[p.name for p in parts],'low':lo,'high':hi})
(root/'house-kit-inspection.json').write_text(json.dumps(rows,indent=2))
for r in rows:print('KIT',r['name'],[round(x,2) for x in r['low']],[round(x,2) for x in r['high']],flush=True)
bpy.ops.wm.save_as_mainfile(filepath=str(root/'ModularHouseKit.blend'))
