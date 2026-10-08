"""Author detailed acorn cupules and a bevelled stone ready arch.

These original fixtures complement the downloaded CC0 set. Material surfaces
reuse the full 4K licensed bark, timber and masonry maps from that set.
"""
import bpy, math, json
from pathlib import Path
helper=Path(__file__).with_name('convert_lobby_assets.py').read_text()
exec(helper[:helper.index('import sys\nif')])
REPORT=json.loads((ROOT/'model-index.json').read_text());MATERIALS=json.loads((ROOT/'material-index.json').read_text())
REPORT=[r for r in REPORT if r['name'] not in ['SM_AcornDetailed','SM_ReadyArch']]

reset();body=mat('M_AcornShell','dark_wooden_planks');MATERIALS['M_AcornShell']['factor']=[1.35,.78,.34,1]
cap=mat('M_AcornCupule','jolcham_oak_bark_01')
objects=[]
def surface(name,rows,cols,position,m):
    verts=[position(i/rows,j/cols) for i in range(rows+1) for j in range(cols)]
    faces=[(i*cols+j,i*cols+(j+1)%cols,(i+1)*cols+(j+1)%cols,(i+1)*cols+j) for i in range(rows) for j in range(cols)]
    mesh=bpy.data.meshes.new(name);mesh.from_pydata(verts,[],faces);mesh.materials.append(m);mesh.update();o=bpy.data.objects.new(name,mesh);bpy.context.collection.objects.link(o)
    uv=mesh.uv_layers.new(name='Axial grain UV')
    for f in mesh.polygons:
        f.use_smooth=True
        for li in f.loop_indices:
            vi=mesh.loops[li].vertex_index;j=vi%cols
            if j==0 and any(mesh.loops[k].vertex_index%cols==cols-1 for k in f.loop_indices):j=cols
            uv.data[li].uv=(j/cols,vi//cols/rows)
    return o
def nut(t,u):
    a=u*2*math.pi;r=.0174*max(math.sin(math.pi*t),.001)**.45*(.86+.22*t)
    r*=1+.013*math.sin(a*18+t*13)+.008*math.sin(a*37-t*11)
    return (r*math.cos(a),r*math.sin(a),.045*t)
def cup(t,u):
    a=u*2*math.pi;phi=t*math.pi*.48
    # Raised overlapping scales are actual geometry, including the wavy rim.
    ridges=max(0,math.cos(a*32+t*math.pi*11))*max(0,math.cos(t*math.pi*24))
    r=(.0187+.00065*ridges)*math.sin(phi)
    return (r*math.cos(a),r*math.sin(a),.0475+.007*math.cos(phi)+.00035*ridges)
objects=[surface('Acorn ribbed skin',64,128,nut,body),surface('Overlapping oak cupule scales',56,192,cup,cap)]
objects.append(box('Irregular stalk',(.002,0,.059),(.0028,.0026,.010),cap,.001))
export('SM_AcornDetailed',objects)

reset();stone=mat('M_HearthStone','rustic_stone_wall');timber=mat('M_LodgeTimber','dark_wooden_planks')
objects=[];radius=1.25;height=2.85;thick=.12;n=96
verts=[]
for depth in [-.10,.10]:
    for r in [radius-thick,radius+thick]:
        for i in range(n+1):
            a=i/n*math.pi;verts.append((depth,r*math.cos(a),height-radius+r*math.sin(a)))
faces=[];stride=n+1
for i in range(n):
    faces.extend([(i,i+1,stride+i+1,stride+i),
                  (2*stride+i,3*stride+i,3*stride+i+1,2*stride+i+1),
                  (i,2*stride+i,2*stride+i+1,i+1),
                  (stride+i,stride+i+1,3*stride+i+1,3*stride+i)])
for i in [0,n]:faces.append((i,stride+i,3*stride+i,2*stride+i))
mesh=bpy.data.meshes.new('Continuous carved portal arch');mesh.from_pydata(verts,[],faces);mesh.materials.append(stone);mesh.update();o=bpy.data.objects.new('Carved stone arch',mesh);bpy.context.collection.objects.link(o)
bpy.context.view_layer.objects.active=o;o.select_set(True);bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project();bpy.ops.object.mode_set(mode='OBJECT')
mod=o.modifiers.new('Carved edge bevel','BEVEL');mod.width=.016;mod.segments=3;objects=[o]
for side in [-1,1]:objects.append(box('Stone arch foot',(.0,side*radius,(height-radius)/2),(.2,thick*2,height-radius),stone,.016))
export('SM_ReadyArch',objects,False)
(ROOT/'model-index.json').write_text(json.dumps(REPORT,indent=2));(ROOT/'material-index.json').write_text(json.dumps(MATERIALS,indent=2))
print('LOBBY DETAIL FIXTURES COMPLETE',flush=True)
