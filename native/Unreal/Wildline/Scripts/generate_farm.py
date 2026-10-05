"""Original playable farm architecture, shotgun, and CC0 textured livestock rigs."""
import bpy, math, random
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parent.parent/'ArtSource/Farm'
out=root/'Export';out.mkdir(exist_ok=True)
random.seed(7401)
parts=[];hulls=[]
def reset():
    global parts,hulls
    bpy.ops.wm.read_factory_settings(use_empty=True);parts=[];hulls=[]
def material(name):return bpy.data.materials.get(name) or bpy.data.materials.new(name)
def finish(o,mat):
    o.data.materials.append(material(mat));parts.append(o);return o
def box(p,s,mat='F_Wood',collision=False,angle=0):
    bpy.ops.mesh.primitive_cube_add(size=1,location=p);o=bpy.context.object;o.scale=s;o.rotation_euler[1]=angle
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    # World-aligned physical UVs keep photographic planks and brick readable.
    uv=o.data.uv_layers.active
    for face in o.data.polygons:
        n=face.normal;axis=max(range(3),key=lambda i:abs(n[i]));indices=[i for i in range(3) if i!=axis]
        for li in face.loop_indices:
            v=o.data.vertices[o.data.loops[li].vertex_index].co
            uv.data[li].uv=(v[indices[0]]/2,v[indices[1]]/2)
    finish(o,mat)
    if collision:
        c=o.copy();c.data=o.data.copy();bpy.context.collection.objects.link(c);hulls.append(c)
    return o
def cyl(p,r,h,mat='F_Metal',rotation=None):
    bpy.ops.mesh.primitive_cylinder_add(vertices=32,radius=r,depth=h,location=p);o=bpy.context.object
    if rotation:o.rotation_euler=rotation
    for f in o.data.polygons:f.use_smooth=len(f.vertices)==4
    return finish(o,mat)
def sphere(p,s,mat):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=20,ring_count=12,radius=1,location=p);o=bpy.context.object;o.scale=s
    for f in o.data.polygons:f.use_smooth=True
    return finish(o,mat)
def export(name):
    bpy.ops.object.select_all(action='DESELECT')
    for o in parts:o.select_set(True)
    bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=bpy.context.object;o.name=name
    bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    for i,c in enumerate(hulls):c.name=f'UCX_{name}_{i:03d}';c.select_set(True)
    bpy.ops.export_scene.fbx(filepath=str(out/(name+'.fbx')),use_selection=True,object_types={'MESH'},bake_anim=False,axis_forward='-Y',axis_up='Z',path_mode='STRIP')
reset()
# Every wall has individual hulls. Doors and low squirrel vents remain open.
box((0,0,.025),(12,10,.05),'F_Wood',True)
for x in [-6,6]:
    for y in [-2.85,2.85]:box((x,y,1.6),(.22,4.3,3.2),'F_Brick',True)
    box((x,0,2.85),(.22,1.4,.7),'F_Brick',True)
for y in [-5,5]:
    for x in [-4,4]:box((x,y,1.6),(4,.22,3.2),'F_Brick',True)
    # Center windows over a low, wide vent: only squirrels fit under the sill.
    box((0,y,.72),(4,.22,.6),'F_Brick',True)
    box((0,y,2.75),(4,.22,.9),'F_Brick',True)
    for x in [-1.92,0,1.92]:box((x,y,1.65),(.12,.25,1.3),'F_Trim',True)
    for x in [-1,1]:box((x,y,1.66),(1.7,.035,1.22),'F_Glass')
    for z in [.41,1.01,2.30]:box((0,y,z),(4.15,.30,.08),'F_Trim')
for x in [-6.13,6.13]:
    for y in [-.76,.76]:box((x,y,1.28),(.14,.16,2.56),'F_Trim',True)
    box((x,0,2.55),(.14,1.65,.15),'F_Trim',True)
    # Door panels are visibly held open alongside each entrance.
    box((x-.4,.87,1.22),(.78,.07,2.42),'F_Wood',True)
for x in [-5.98,5.98]:
    for y in [-5.03,5.03]:box((x,y,1.6),(.28,.28,3.3),'F_Trim',True)
slope=math.atan2(2.2,6.4)
for x in [-3.2,3.2]:box((x,0,4.3),(math.sqrt(6.4**2+2.2**2),11.1,.16),'F_Roof',True, -slope if x<0 else slope)
for y in [-5.15,5.15]:
    # Gable timber infill follows the sloping roof.
    for i in range(24):
        x=-5.75+i*.5;h=2.2*(1-abs(x)/6.4)
        box((x,y,3.25+h/2),(.48,.12,h),'F_Wood',True)
    box((0,y,3.2),(12.8,.16,.17),'F_Trim')
box((4,2.5,4.95),(.85,.85,2),'F_Brick',True);box((4,2.5,6),(.99,.99,.12),'F_Trim')
# Covered porch with an uninterrupted ground-level entry.
box((-7.45,0,.025),(2.8,4.5,.05),'F_Wood',True)
box((-7.45,0,2.85),(3,4.8,.16),'F_Roof',True, -.08)
for y in [-2.12,2.12]:box((-8.7,y,1.38),(.16,.16,2.75),'F_Trim',True)
# Interior pantry, kitchen and furniture leave the center aisle clear.
for y in [-4.4,4.4]:
    box((3.7,y,.48),(3.7,.85,.85),'F_Trim',True);box((3.7,y,.94),(3.9,.96,.08),'F_Wood',True)
    for x in [2.2,3.5,4.8]:box((x,y-.46 if y<0 else y+.46,.49),(1.1,.05,.7),'F_Wood')
box((-3.7,3.0,.77),(2.2,1.3,.10),'F_Wood',True)
for x in [-4.6,-2.8]:
    for y in [2.5,3.5]:box((x,y,.4),(.12,.12,.78),'F_Trim',True)
box((4.9,-3.2,1),(1.25,1.2,1.9),'F_Trim',True)
for z in [.35,.95,1.55,1.95]:box((4.8,-3.2,z),(1.3,1.25,.08),'F_Wood')
for x in [3.9,4.6]:
    for y in [3.9,4.35]:cyl((x,y,1.18),.14,.39,'F_Jar')
export('SM_Farmhouse')
reset()
# Classic red barn with open double doors and loft slats.
box((0,0,.04),(10,8,.08),'F_Wood',True)
for x in [-5,5]:
    for y in [-2.85,2.85]:box((x,y,2.1),(.20,2.3,4.2),'F_RedWood',True)
    box((x,0,3.9),(.20,3.4,.6),'F_RedWood',True)
for y in [-4,4]:box((0,y,2.1),(10,.2,4.2),'F_RedWood',True)
for x in [-5,5]:
    for y in [-4,4]:box((x,y,2.1),(.22,.25,4.3),'F_Trim',True)
for y in [-4.12,4.12]:box((0,y,4.2),(10.5,.18,.17),'F_Trim')
for x in [-2.6,2.6]:box((x,0,5.32),(5.72,8.7,.18),'F_Roof',True,-.41 if x<0 else .41)
for y in [-4.08,4.08]:
    for i in range(20):
        x=-4.75+i*.5;h=2.1*(1-abs(x)/5.2);box((x,y,4.2+h/2),(.49,.12,h),'F_RedWood',True)
export('SM_Barn')
reset();cyl((0,0,3.5),2,7);sphere((0,0,7),(2,2,1.1),'F_Metal')
for z in [.5,1.8,3.1,4.4,5.7,7]:
    bpy.ops.mesh.primitive_torus_add(major_radius=2.02,minor_radius=.028,location=(0,0,z));finish(bpy.context.object,'F_Steel')
for z in [i*.35 for i in range(20)]:box((-2.10,0,z+.35),(.08,.6,.035),'F_Steel')
for y in [-.3,.3]:box((-2.12,y,3.5),(.065,.065,7),'F_Steel')
box((0,0,3.5),(2.8,2.8,7),'F_Metal',True);export('SM_Silo')
reset();box((0,0,.48),(1.4,.9,.95),'F_Hay',True)
for x in [-.45,.45]:box((x,0,.49),(.045,.93,.98),'F_Rope')
export('SM_HayBale')
reset();box((0,0,.22),(.68,.52,.42),'F_Wood',True)
for y in [-.28,.28]:
    for z in [.07,.22,.38]:box((0,y,z),(.74,.035,.1),'F_Trim')
export('SM_Crate')
reset()
for x in [-1,1]:box((x,0,.65),(.15,.15,1.3),'F_Wood',True)
for z in [.50,1.02]:box((0,0,z),(2.2,.11,.12),'F_Wood',True)
export('SM_Fence')
reset()
box((0,0,.26),(2,.85,.06),'F_Metal',True)
for y in [-.46,.46]:box((0,y,.45),(2,.08,.45),'F_Metal',True)
for x in [-1,1]:box((x,0,.45),(.08,.85,.45),'F_Metal',True)
box((0,0,.51),(1.89,.77,.015),'F_Water')
for x in [-.75,.75]:box((x,0,.13),(.12,.6,.26),'F_Steel',True)
export('SM_Trough')
reset();cyl((0,0,1),.025,2,'F_Stalk')
for i in range(8):
    a=i*2.4;z=.35+i*.18;direction=Vector((math.cos(a),math.sin(a),.28))
    verts=[];faces=[]
    for j in range(9):
        t=j/8;w=math.sin(math.pi*t)*.055
        c=direction*(t*.84)+Vector((0,0,z-.42*t*t))
        side=Vector((-math.sin(a),math.cos(a),0))*w
        verts.extend([tuple(c-side),tuple(c+Vector((0,0,.013*math.sin(math.pi*t)))),tuple(c+side)])
    for j in range(8):
        for k in range(2):faces.append((j*3+k,j*3+k+1,(j+1)*3+k+1,(j+1)*3+k))
    m=bpy.data.meshes.new('Curved corn leaf');m.from_pydata(verts,[],faces);o=bpy.data.objects.new('Corn leaf',m);bpy.context.collection.objects.link(o)
    for f in m.polygons:f.use_smooth=True
    finish(o,'F_Corn')
for i in range(5):sphere((math.sin(i*1.4)*.08,math.cos(i*1.4)*.08,2+i*.04),(.025,.025,.11),'F_Hay')
sphere((.08,0,1.05),(.065,.055,.22),'F_Hay');export('SM_Corn')
reset();box((0,0,0),(31,14,.01),'F_Soil');export('SM_Field')
reset()
# One continuous shell with a narrow waist, tapered ends and subtle surface ribs.
verts=[];faces=[];rings=33;steps=24
for j in range(rings):
    t=-1+j*2/(rings-1);profile=math.sqrt(max(.0001,1-t*t))*(.65+.5*math.sin(math.pi*abs(t)))
    for k in range(steps):
        a=k*math.tau/steps;ripple=1+.025*math.sin(a*12)*math.sin(j*.9)
        verts.append((t*.025,math.cos(a)*.0085*profile*ripple,.0096+math.sin(a)*.009*profile*ripple))
for j in range(rings-1):
    for k in range(steps):faces.append((j*steps+k,j*steps+(k+1)%steps,(j+1)*steps+(k+1)%steps,(j+1)*steps+k))
faces.extend([tuple(reversed(range(steps))),tuple(range((rings-1)*steps,rings*steps))])
m=bpy.data.meshes.new('Continuous peanut shell');m.from_pydata(verts,[],faces);o=bpy.data.objects.new('Peanut',m);bpy.context.collection.objects.link(o)
uv=m.uv_layers.new(name='ShellUV')
for poly in m.polygons:
    poly.use_smooth=True
    for li in poly.loop_indices:
        idx=m.loops[li].vertex_index;uv.data[li].uv=(idx//steps/(rings-1),(idx%steps)/steps)
finish(o,'F_Peanut');export('SM_Peanut')
reset()
# Shared sporter stock and grip proportions retain the existing hunter hand IK.
rifle=(root.parent/'../Scripts/generate_bolt_rifle.py').resolve().read_text()
stock=rifle[rifle.index('parts=[]'):rifle.index("cylinder('Tapered barrel'")]
farm_finish=finish
exec(stock)
finish=farm_finish
# That helper keeps (object, action) pairs.
parts=[o for o,b in parts];stockparts=list(parts);parts=[]
for y in [-.014,.014]:
    cyl((.35,y,.02),.013,.68,'R_Metal',(0,math.pi/2,0));cyl((.693,y,.02),.0105,.005,'R_Black',(0,math.pi/2,0))
box((.30,0,.038),(.72,.009,.008),'R_Steel')
box((.003,0,.019),(.14,.070,.06),'R_Steel')
box((.17,0,-.012),(.27,.048,.040),'R_Wood')
sphere((.65,0,.048),(.006,.005,.006),'R_Steel')
actionparts=list(parts);parts=stockparts
box((-.075,0,-.047),(.010,.005,.027),'R_Steel')
box((-.067,0,-.066),(.085,.027,.007),'R_Metal')
for x in [-.11,-.03]:box((x,0,-.052),(.006,.027,.032),'R_Metal')
box((-.06,0,.053),(.06,.015,.007),'R_Steel')
export('SM_Shotgun');parts=actionparts;hulls=[];export('SM_ShotgunAction')

def animal(species,file,height,angle):
    reset();bpy.ops.wm.obj_import(filepath=str(file))
    objs=[o for o in bpy.context.scene.objects if o.type=='MESH']
    for o in objs:
        bpy.context.view_layer.objects.active=o;o.select_set(True);bpy.ops.object.transform_apply(location=True,rotation=True,scale=True);o.select_set(False)
    verts=[v.co for o in objs for v in o.data.vertices];lo=min(v.z for v in verts);hi=max(v.z for v in verts);scale=height/(hi-lo)
    for o in objs:
        for v in o.data.vertices:
            x,y,z=v.co;v.co=(math.cos(angle)*x-math.sin(angle)*y,math.sin(angle)*x+math.cos(angle)*y,z-lo);v.co*=scale
        for slot in o.material_slots:
            key=slot.material.name.lower();slot.material=material('F_'+species+('Eye' if 'eye' in key else 'Hair' if 'hair' in key else 'Skin'))
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:o.select_set(True)
    bpy.context.view_layer.objects.active=objs[0];bpy.ops.object.join();mesh=bpy.context.object;mesh.name='SK_'+species
    vs=[v.co for v in mesh.data.vertices];xmin=min(v.x for v in vs);xmax=max(v.x for v in vs)
    # Fit individual legs from actual source geometry, preserving the photographic UVs.
    legs={};legheight=height*.46
    for suffix,sx,sy in [('FL',1,1),('FR',1,-1),('BL',-1,1),('BR',-1,-1)]:
        points=[v for v in vs if v.z<height*.22 and (v.x>0)==(sx>0) and (v.y>0)==(sy>0)]
        legs[suffix]=Vector((sum(v.x for v in points)/len(points),sum(v.y for v in points)/len(points),0)) if points else Vector((sx*height*.45,sy*height*.17,0))
    armdata=bpy.data.armatures.new(species+' grazing rig');arm=bpy.data.objects.new('FarmRig',armdata);bpy.context.collection.objects.link(arm)
    bpy.context.view_layer.objects.active=arm;mesh.select_set(False);arm.select_set(True);bpy.ops.object.mode_set(mode='EDIT')
    def bone(name,head,tail,parent=None):
        b=armdata.edit_bones.new(name);b.head=head;b.tail=tail
        if parent:b.parent=armdata.edit_bones[parent]
    bone('Root',(0,0,0),(0,0,.15));bone('Body',(0,0,height*.62),(0,0,height*.77),'Root')
    bone('Head',(xmax*.55,0,height*.70),(xmax*.55,0,height*.88),'Body');bone('Tail',(xmin*.78,0,height*.65),(xmin*.78,0,height*.86),'Body')
    for suffix,p in legs.items():
        bone('Leg'+suffix,p+Vector((0,0,legheight)),p+Vector((0,0,legheight*.57)),'Body')
        bone('Knee'+suffix,p+Vector((0,0,legheight*.57)),p+Vector((0,0,.08)),'Leg'+suffix)
    bpy.ops.object.mode_set(mode='OBJECT')
    groups={b.name:mesh.vertex_groups.new(name=b.name) for b in armdata.bones}
    for v in mesh.data.vertices:
        p=v.co
        if p.z<legheight:
            suffix=min(legs,key=lambda s:(p-legs[s]).to_2d().length);blend=max(0,min(1,(p.z-legheight*.30)/(legheight*.22)))
            groups['Leg'+suffix].add([v.index],blend,'REPLACE');groups['Knee'+suffix].add([v.index],1-blend,'REPLACE')
        elif p.x>xmax*.43:groups['Head'].add([v.index],1,'REPLACE')
        elif p.x<xmin*.82:groups['Tail'].add([v.index],1,'REPLACE')
        else:groups['Body'].add([v.index],1,'REPLACE')
    mod=mesh.modifiers.new('Farm grazing animation','ARMATURE');mod.object=arm;mesh.parent=arm
    bpy.ops.object.select_all(action='DESELECT');mesh.select_set(True);arm.select_set(True)
    bpy.ops.export_scene.fbx(filepath=str(out/(mesh.name+'.fbx')),use_selection=True,object_types={'MESH','ARMATURE'},add_leaf_bones=False,bake_anim=False,axis_forward='-Y',axis_up='Z',path_mode='STRIP')
animal('Cow',root/'cow/cow/cowTM08New00RTime02.obj',1.45,-math.pi/2)
animal('Horse',root/'horse/horse/LD_HorseRtime02.obj',1.75,math.pi/2)
print('FARM_ASSETS_EXPORTED')
