"""Original Taylorsville Lake boats, fish, fishing rods, and lodge doorway.
Run with Blender --background --python Scripts/generate_lake_assets.py.
"""
from pathlib import Path
import bpy, math, json
from mathutils import Vector
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'ArtSource/Lake/Export'
OUT.mkdir(parents=True,exist_ok=True)
manifest=[]
def reset(): bpy.ops.wm.read_factory_settings(use_empty=True)
def mat(name,color):
    m=bpy.data.materials.new(name);m.diffuse_color=(*color,1);return m
def box(name,p,size,m,bevel=.01):
    bpy.ops.mesh.primitive_cube_add(size=1,location=p);o=bpy.context.object;o.name=name;o.dimensions=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(m)
    if bevel: mod=o.modifiers.new('Rounded worked edges','BEVEL');mod.width=bevel;mod.segments=3;o.modifiers.new('Weighted normals','WEIGHTED_NORMAL')
    return o
def mesh(name,verts,faces,m):
    data=bpy.data.meshes.new(name);data.from_pydata(verts,[],faces);data.update();o=bpy.data.objects.new(name,data);bpy.context.collection.objects.link(o);o.data.materials.append(m);return o
def rod(name,a,b,r,m,r2=None):
    d=Vector(b)-Vector(a);bpy.ops.mesh.primitive_cone_add(vertices=16,radius1=r,radius2=r if r2 is None else r2,depth=d.length,location=(Vector(a)+Vector(b))/2);o=bpy.context.object;o.name=name;o.rotation_euler=d.to_track_quat('Z','Y').to_euler();o.data.materials.append(m);return o
def export(name):
    objects=[o for o in bpy.context.scene.objects if o.type=='MESH'];bpy.ops.object.select_all(action='DESELECT')
    for o in objects:o.select_set(True)
    bpy.context.view_layer.objects.active=objects[0];bpy.ops.object.convert(target='MESH');bpy.context.scene.unit_settings.system='METRIC'
    bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')),use_selection=True,object_types={'MESH'},bake_anim=False,axis_forward='-Y',axis_up='Z',apply_unit_scale=True,path_mode='STRIP')
    manifest.append({'name':name,'source':'Original WILDLINE procedural artwork','triangles':sum(sum(len(p.vertices)-2 for p in o.data.polygons) for o in objects)})

reset();green=mat('M_BoatGreen',(.12,.22,.16));wood=mat('M_DockWood',(.35,.23,.11));metal=mat('M_LakeMetal',(.15,.17,.16))
# Open clinker hull with a pointed bow, full interior, transom and gunwales.
sections=[(-2,.02),(-1.75,.48),(-1.2,.66),(-.4,.72),(.5,.7),(1.35,.6),(1.8,.53)]
for side in [-1,1]:
    for layer in range(5):
        v=[]
        for x,width in sections:
            for level in [layer/5,(layer+1)/5]:v.append((x,side*width*(.55+.45*level),.05+level*.5))
        faces=[(i*2,i*2+1,i*2+3,i*2+2) for i in range(len(sections)-1)]
        o=mesh('Overlapping hull strake',v,faces,green);s=o.modifiers.new('Timber hull thickness','SOLIDIFY');s.thickness=.024;b=o.modifiers.new('Soft plank edge','BEVEL');b.width=.007;b.segments=2
    for (x,w),(xx,ww) in zip(sections,sections[1:]):rod('Oak gunwale',(x,side*w,.57),(xx,side*ww,.57),.03,wood)
box('Stern transom',(1.8,0,.29),(.06,1.04,.5),green)
for i in range(9):box('Interior floorboards',(-.05,(i-4)*.105,.12),(3.45,.10,.035),wood,.006)
for x in [-1,.15,1.15]:box('Bench seat',(x,0,.43),(.30,1.18,.07),wood);box('Bench support',(x,0,.26),(.20,.07,.31),green)
for side in [-1,1]:
    rod('Oar shaft',(-1.45,side*.28,.49),(1.25,side*.50,.53),.018,wood)
    o=box('Oar blade',(1.48,side*.52,.53),(.57,.16,.025),wood,.03);o.rotation_euler.z=side*.06
    rod('Oarlock',(.35,side*.71,.54),(.35,side*.71,.65),.018,metal)
for x in [-1.45,1.25]:
    for side in [-1,1]:rod('Brass fitting',(x,side*.61,.55),(x,side*.61,.62),.022,metal)
export('SM_FishingBoat')

reset();carbon=mat('M_LakeMetal',(.08,.10,.10));cork=mat('M_DockWood',(.5,.36,.18));line=mat('M_Bobber',(.8,.13,.04))
rod('Cork handle',(0,0,0),(.35,0,0),.018,cork)
rod('Tapered graphite blank',(.35,0,0),(2.2,0,.06),.009,carbon,.002)
for x in [.55,.85,1.15,1.45,1.75,2.05]:
    bpy.ops.mesh.primitive_torus_add(major_radius=.012 if x<1.4 else .007,minor_radius=.002,major_segments=16,minor_segments=6,location=(x,0,.02));o=bpy.context.object;o.name='Rod line guide';o.rotation_euler.y=math.pi/2;o.data.materials.append(carbon)
rod('Spinning reel spool',(.24,-.055,-.04),(.24,.055,-.04),.045,carbon)
rod('Reel stem',(.24,0,0),(.24,0,-.04),.008,carbon)
rod('Reel handle',(.24,.07,-.04),(.28,.09,-.085),.007,cork)
export('SM_FishingRod')

reset();scales=mat('M_FishScales',(.22,.30,.18));fin=mat('M_FishFin',(.19,.26,.13));eye=mat('M_FishEye',(.005,.006,.003))
bpy.ops.mesh.primitive_uv_sphere_add(segments=32,ring_count=16);body=bpy.context.object;body.name='Bass body';body.scale=(.23,.052,.085);body.data.materials.append(scales);bpy.ops.object.shade_smooth()
mesh('Forked tail',[(-.19,0,.015),(-.34,-.005,.095),(-.30,0,.005),(-.34,-.005,-.075),(-.19,0,-.015)],[(0,1,2,3,4)],fin)
mesh('Dorsal fin',[(-.16,0,.045),(-.12,0,.12),(-.07,0,.10),(-.03,0,.135),(.02,0,.10),(.10,0,.10),(.14,0,.045)],[(0,1,2,3,4,5,6)],fin)
for side in [-1,1]:
    mesh('Pectoral fin',[(.08,side*.04,.005),(-.035,side*.10,-.055),(.01,side*.045,-.015)],[(0,1,2)],fin)
    bpy.ops.mesh.primitive_uv_sphere_add(segments=12,ring_count=8,radius=.012,location=(.165,side*.039,.027));bpy.context.object.data.materials.append(eye)
rod('Bass mouth',(.217,0,-.003),(.232,0,-.003),.018,fin)
export('SM_LakeFish')

# Rebuild the existing UV-mapped lodge with a floor-level door in the east
# window aperture. All other windows and the retained scanned materials match.
helper=(ROOT/'Scripts/convert_lobby_assets.py').read_text();helper=helper[:helper.index('import sys\nif')]
helper=helper.replace("y_gaps=[(-2.35,-.15)] if side==1 and 1.55<z+.155 and z-.155<3.55 else []","y_gaps=[(-2.35,-.15)] if side==1 and z-.155<3.55 else []")
helper=helper.replace('for boundary,below in [(1.55,True),(3.55,False)]:','for boundary,below in ([(0,True),(3.55,False)] if axis==\'Y\' and side==1 else [(1.55,True),(3.55,False)]):')
helper=helper.replace("export('SM_LodgeArchitecture',objects,False)","export('SM_LakeLodgeArchitecture',objects,False)")
scope={'__file__':str(ROOT/'Scripts/convert_lobby_assets.py')};exec(compile(helper,'lake_lodge_authoring','exec'),scope);scope['OUT']=OUT;scope['architecture'](True)
(OUT.parent/'manifest.json').write_text(json.dumps(manifest+scope['REPORT'],indent=2))
# Preserve the retained third-party bass attribution when regenerating props.
if not (OUT.parent/'PROVENANCE.md').exists():(OUT.parent/'PROVENANCE.md').write_text('# Taylorsville Lake\n\nOriginal WILDLINE boats, rods, dock and lake lodge. Retain the forest, lobby and CityPark material credits and add the bass creator attribution before packaging.\n')
print('LAKE ART READY',flush=True)
