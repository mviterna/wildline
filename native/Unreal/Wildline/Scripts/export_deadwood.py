"""An original hollow fallen trunk, using separately licensed bark in Unreal."""
import bpy, math, random
from pathlib import Path
bpy.ops.wm.read_factory_settings(use_empty=True)
r=random.Random(79);verts=[];faces=[];mats=[];uv=[];N=32;segments=14
for inner in [False,True]:
    for s in range(segments+1):
        x=-1.65+s/segments*3.3
        for a in range(N):
            theta=a/N*math.tau;radius=(.255 if inner else .40)*(1-.16*s/segments)*(1+.05*math.sin(a*3.1+s*.7))
            verts.append((x,radius*math.cos(theta),radius*math.sin(theta)+.025*math.sin(s*.6)));uv.append((a/N,s/segments))
    start=inner*(segments+1)*N
    for s in range(segments):
        for a in range(N):
            b=(a+1)%N;face=(start+s*N+a,start+s*N+b,start+(s+1)*N+b,start+(s+1)*N+a)
            faces.append(tuple(reversed(face)) if inner else face);mats.append(1 if inner else 0)
for s in [0,segments]:
    for a in range(N):
        b=(a+1)%N;offset=(segments+1)*N
        face=(s*N+a,s*N+b,offset+s*N+b,offset+s*N+a);faces.append(face if s==0 else tuple(reversed(face)));mats.append(1)
mesh=bpy.data.meshes.new('Weathered hollow log');mesh.from_pydata(verts,[],faces);mesh.materials.append(bpy.data.materials.new('F_Bark'));mesh.materials.append(bpy.data.materials.new('F_Heartwood'));layer=mesh.uv_layers.new()
for p,mi in zip(mesh.polygons,mats):
    p.material_index=mi;p.use_smooth=True
    for li,vi in zip(p.loop_indices,p.vertices):layer.data[li].uv=uv[vi]
obj=bpy.data.objects.new('SM_Log',mesh);bpy.context.collection.objects.link(obj);obj.select_set(True);bpy.context.view_layer.objects.active=obj
# Twelve convex wall segments give walking and bullets real collision while keeping
# the central tunnel open. UCX geometry is imported as collision, never rendered.
for i in range(12):
    points=[]
    for x,shrink in [(-1.65,1),(1.65,.84)]:
        for radius in [.245,.41]:
            for a in [i/12*math.tau,(i+1)/12*math.tau]:points.append((x,radius*shrink*math.cos(a),radius*shrink*math.sin(a)))
    cm=bpy.data.meshes.new('Log collision wall');cm.from_pydata(points,[],[(0,1,3,2),(4,6,7,5),(0,4,5,1),(2,3,7,6),(0,2,6,4),(1,5,7,3)])
    co=bpy.data.objects.new(f'UCX_SM_Log_{i:02}',cm);bpy.context.collection.objects.link(co);co.select_set(True)
bpy.ops.export_scene.fbx(filepath=str(Path(__file__).resolve().parent.parent/'ArtSource/Forest/Export/SM_Log.fbx'),use_selection=True,object_types={'MESH'},bake_anim=False,axis_forward='-Y',axis_up='Z',path_mode='STRIP')
