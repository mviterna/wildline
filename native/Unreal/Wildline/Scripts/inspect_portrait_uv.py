import bpy,json
from pathlib import Path
root=Path(__file__).resolve().parent.parent
sources=[root/'ArtSource/Hunters/Export/SK_HunterFemale.fbx',root.parents[1]/'AssetDownloads/Hunters/Diagnostics/SK_Female_FromUE.fbx']
for source in sources:
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    bpy.ops.import_scene.fbx(filepath=str(source))
    mesh=next(o for o in bpy.context.selected_objects if o.type=='MESH' and len(o.data.vertices)>1000)
    skin=[p for p in mesh.data.polygons if 'skin' in mesh.data.materials[p.material_index].name.lower()]
    crown=max((mesh.matrix_world@mesh.data.vertices[i].co).z for p in skin for i in p.vertices)
    samples=[]
    for offset in [.04,.08,.12,.16,.20]:
        points=[]
        for p in skin:
            for li,vi in zip(p.loop_indices,p.vertices):
                co=mesh.matrix_world@mesh.data.vertices[vi].co
                if abs(co.x)<.02 and abs(co.z-(crown-offset))<.009:
                    points.append((co.y,tuple(round(v,4) for v in co),[tuple(round(v,4) for v in uv.data[li].uv) for uv in mesh.data.uv_layers]))
        points.sort(key=lambda x:x[0]);samples.append({'belowCrown':offset,'front':points[:1]})
    print('PORTRAIT_UV '+json.dumps({'source':source.name,'crown':crown,'uvs':[u.name for u in mesh.data.uv_layers],'samples':samples}))
