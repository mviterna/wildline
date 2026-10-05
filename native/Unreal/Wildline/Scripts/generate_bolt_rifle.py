"""Original 110 cm sporting bolt-action rifle, with a separate cycling bolt."""
import bpy,math
from pathlib import Path
from mathutils import Vector
bpy.ops.wm.read_factory_settings(use_empty=True)
root=Path(__file__).resolve().parent.parent/'ArtSource/Hunters/Export';root.mkdir(exist_ok=True)
parts=[]
def mat(name):return bpy.data.materials.get(name) or bpy.data.materials.new(name)
def finish(obj,name,material,bolt=False):
    obj.name=name;obj.data.materials.clear();obj.data.materials.append(mat(material))
    for p in obj.data.polygons:p.use_smooth=True
    parts.append((obj,bolt));return obj
def cylinder(name,x0,x1,r,z,material,y=0,bolt=False):
    bpy.ops.mesh.primitive_cylinder_add(vertices=48,radius=r,depth=x1-x0,location=((x0+x1)*.5,y,z),rotation=(0,math.pi*.5,0));return finish(bpy.context.object,name,material,bolt)
def cube(name,loc,scale,material,bevel=.004,bolt=False):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.scale=scale;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    mod=o.modifiers.new('Machined edges','BEVEL');mod.width=bevel;mod.segments=3;bpy.ops.object.modifier_apply(modifier=mod.name);return finish(o,name,material,bolt)
sections=[(-.45,.025,-.04,.063),(-.40,.025,-.04,.063),(-.27,.024,-.025,.05),(-.20,.022,-.008,.029),(-.15,.025,-.027,.046),(-.095,.021,-.026,.044),(-.06,.022,-.014,.032),(.05,.026,-.013,.027),(.19,.026,-.018,.022),(.29,.019,-.016,.018)]
verts=[];faces=[]
for x,w,z,h in sections:
    for i in range(24):
        a=i/24*math.tau;verts.append((x,w*math.cos(a),z+h*math.sin(a)))
for s in range(len(sections)-1):
    for i in range(24):faces.append((s*24+i,s*24+(i+1)%24,(s+1)*24+(i+1)%24,(s+1)*24+i))
faces.extend([tuple(reversed(range(24))),tuple(range((len(sections)-1)*24,len(sections)*24))])
mesh=bpy.data.meshes.new('Walnut sporting stock');mesh.from_pydata(verts,[],faces);obj=bpy.data.objects.new('Stock',mesh);bpy.context.collection.objects.link(obj);finish(obj,'Walnut stock','R_Wood')
sub=obj.modifiers.new('Stock contour','SUBSURF');sub.levels=2;bpy.context.view_layer.objects.active=obj;bpy.ops.object.modifier_apply(modifier=sub.name)
cube('Recoil pad',(-.456,0,-.04),(.017,.055,.13),'R_Rubber')
cylinder('Tapered barrel',.16,.65,.0105,.017,'R_Metal');cylinder('Barrel shoulder',.12,.20,.016,.017,'R_Metal')
cylinder('Receiver',-.12,.15,.021,.017,'R_Metal')
cube('Ejection port',(.012,.019,.026),(.084,.004,.026),'R_Black',.002)
cube('Magazine floorplate',(.005,0,-.040),(.135,.038,.008),'R_Metal',.003)
cube('Trigger',(-.073,0,-.052),(.008,.006,.026),'R_Steel',.002)
# Swept trigger guard (oval tube).
vs=[];fs=[]
for i in range(48):
    a=i/48*math.tau
    for j in range(8):
        b=j/8*math.tau;vs.append((-.066+(.054+.002*math.cos(b))*math.cos(a),.002*math.sin(b),-.05+(.024+.002*math.cos(b))*math.sin(a)))
for i in range(48):
    for j in range(8):fs.append((i*8+j,i*8+(j+1)%8,((i+1)%48)*8+(j+1)%8,((i+1)%48)*8+j))
m=bpy.data.meshes.new('Guard');m.from_pydata(vs,[],fs);o=bpy.data.objects.new('Guard',m);bpy.context.collection.objects.link(o);finish(o,'Trigger guard','R_Metal')
for x in [-.065,.09]:
    cube('Scope mount',(x,0,.046),(.018,.033,.017),'R_Metal',.002)
    cylinder('Scope ring',x-.008,x+.008,.0195,.078,'R_Metal')
cylinder('Scope tube',-.19,.19,.015,.078,'R_Metal');cylinder('Eyepiece',-.23,-.16,.023,.078,'R_Rubber');cylinder('Objective bell',.14,.24,.024,.078,'R_Metal')
cylinder('Objective glass',.241,.244,.0215,.078,'R_Glass');cylinder('Eye glass',-.232,-.230,.020,.078,'R_Glass')
cube('Elevation turret',(.013,0,.097),(.035,.032,.029),'R_Metal',.005)
cylinder('Bolt body',-.135,.02,.013,.021,'R_Steel',bolt=True)
cube('Bolt shroud',(-.15,0,.021),(.04,.032,.028),'R_Metal',.004,True)
start=Vector((-.105,.013,.023));end=Vector((-.10,.057,-.012));delta=end-start
bpy.ops.mesh.primitive_cylinder_add(vertices=24,radius=.004,depth=delta.length,location=(start+end)/2);o=bpy.context.object;o.rotation_euler=delta.to_track_quat('Z','Y').to_euler();finish(o,'Bent bolt handle','R_Steel',True)
bpy.ops.mesh.primitive_uv_sphere_add(segments=32,ring_count=16,radius=.012,location=end);finish(bpy.context.object,'Bolt knob','R_Metal',True)
for bolt,name in [(False,'SM_BoltRifle'),(True,'SM_RifleBolt')]:
    bpy.ops.object.select_all(action='DESELECT');selected=[o for o,b in parts if b==bolt]
    for o in selected:o.select_set(True)
    bpy.context.view_layer.objects.active=selected[0];bpy.ops.object.join();o=selected[0];o.name=name;bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.export_scene.fbx(filepath=str(root/(name+'.fbx')),use_selection=True,object_types={'MESH'},bake_anim=False,axis_forward='-Y',axis_up='Z',path_mode='STRIP')
bpy.ops.wm.save_as_mainfile(filepath=str(root.parent/'BoltRifle.blend'))
print('SPORTING_RIFLE_EXPORTED')
