"""Author open window casings, gathered curtains, a clock and a brass peg rail.

Blender --background --python generate_lodge_details.py. Reuses the retained
CC0 wood/fabric PBR maps; coordinates are metres and the window faces +X.
"""
import bpy, math, json
from pathlib import Path
helper=Path(__file__).with_name('convert_lobby_assets.py').read_text()
exec(helper[:helper.index('import sys\nif')])
REPORT=json.loads((ROOT/'model-index.json').read_text())
MATERIALS=json.loads((ROOT/'material-index.json').read_text())
names=['SM_LodgeWindowDressings','SM_LodgeClock','SM_LodgePegRail']
REPORT=[r for r in REPORT if r['name'] not in names]

def solid(name,color,roughness=.7,metallic=0):
    m=bpy.data.materials.new(name)
    MATERIALS[name]={'factor':[*color,1],'roughness':roughness,'metallic':metallic}
    return m

def cylinder(name,p,radius,depth,m,rotation=(0,0,0)):
    bpy.ops.mesh.primitive_cylinder_add(vertices=48,radius=radius,depth=depth,location=p,rotation=rotation)
    o=bpy.context.object;o.name=name;o.data.materials.append(m)
    bevel=o.modifiers.new('Soft brass edges','BEVEL');bevel.width=.004;bevel.segments=3
    return o

reset();oak=mat('M_WindowOak','dark_wooden_planks');MATERIALS['M_WindowOak']['factor']=[2.1,1.6,1.1,1]
cloth=mat('M_CurtainFabric','quatrefoil_jacquard_fabric')
MATERIALS['M_CurtainFabric']['factor']=[1.15,1.95,.95,1]
MATERIALS['M_CurtainFabric']['desaturate']=True;MATERIALS['M_CurtainFabric']['two_sided']=True
brass=solid('M_LodgeDetailBrass',(.43,.25,.07),.3,.75)
objects=[]
for y in [-1.13,1.13]:objects.append(box('Deep oak window jamb',(.025,y,1),(.28,.13,2.15),oak,.018))
for z in [-.01,2.01]:objects.append(box('Oak window header',(.025,0,z),(.28,2.39,.14),oak,.018))
objects.append(box('Rounded deep window sill',(.19,0,-.10),(.59,2.66,.12),oak,.025))
for y in [-.38,.38]:objects.append(box('Vertical sash glazing bar',(.09,y,1),(.075,.045,1.92),oak,.008))
objects.append(box('Horizontal sash glazing bar',(.09,0,1),(.075,2.18,.045),oak,.008))
# Leave the six panes open to actual 3D woodland, without an opaque sky picture.
objects.append(cylinder('Curtain brass rod',(.35,0,2.16),.018,3.15,brass,(math.pi/2,0,0)))
for side in [-1,1]:
    objects.append(cylinder('Curtain rod finial',(.35,side*1.59,2.16),.035,.045,brass,(math.pi/2,0,0)))
    rows,cols=64,64;vs=[];fs=[]
    for i in range(rows+1):
        t=i/rows;gather=math.exp(-((t-.39)/.16)**2)
        for j in range(cols+1):
            u=j/cols
            inner=.82+.25*gather-.18*t;outer=1.54
            y=side*(inner+(outer-inner)*u)
            x=.31+.064*math.sin(u*math.pi*14)*(1-.42*gather)+.06*math.sin(t*math.pi)
            z=-.25+2.35*t+.035*math.cos(u*math.pi*14)*(1-t)**3
            vs.append((x,y,z))
    for i in range(rows):
        for j in range(cols):
            a=i*(cols+1)+j;fs.append((a,a+1,a+cols+2,a+cols+1))
    mesh=bpy.data.meshes.new('Woven gathered curtain');mesh.from_pydata(vs,[],fs);mesh.materials.append(cloth);mesh.update()
    o=bpy.data.objects.new('Tied back forest green drape',mesh);bpy.context.collection.objects.link(o)
    uv=mesh.uv_layers.new(name='Fabric weave UV')
    for face in mesh.polygons:
        face.use_smooth=True
        for li in face.loop_indices:
            vi=mesh.loops[li].vertex_index;uv.data[li].uv=(vi%(cols+1)/cols,vi//(cols+1)/rows*2)
    objects.append(o)
    objects.append(box('Brass curtain tie',(.375,side*1.31,.69),(.075,.39,.055),brass,.02))
    for j in range(8):
        y=side*(.69+j*.12)
        bpy.ops.mesh.primitive_torus_add(major_radius=.026,minor_radius=.006,location=(.35,y,2.125),rotation=(math.pi/2,0,0),major_segments=20,minor_segments=8)
        o=bpy.context.object;o.data.materials.append(brass);objects.append(o)
export('SM_LodgeWindowDressings',objects,False)

reset();oak=mat('M_WindowOak','dark_wooden_planks');MATERIALS['M_WindowOak']['factor']=[2.1,1.6,1.1,1]
paper=solid('M_ClockIvory',(.76,.68,.48),.85);ink=solid('M_ClockInk',(.026,.032,.024),.7)
objects=[cylinder('Carved clock surround',(.035,0,0),.34,.07,oak,(0,math.pi/2,0)),cylinder('Ivory clock dial',(.079,0,0),.292,.014,paper,(0,math.pi/2,0))]
for i in range(60):
    a=i*math.pi/30;length=.036 if i%5==0 else .013
    o=box('Clock minute marker',(.09,math.sin(a)*(.276-length/2),math.cos(a)*(.276-length/2)),(.004,.007 if i%5 else .013,length),ink,.001)
    o.rotation_euler.x=-a;objects.append(o)
for angle,length,width in [(-math.pi/3,.17,.018),(math.pi/3,.235,.009)]:
    o=box('Clock hand',(.10,math.sin(angle)*length/2,math.cos(angle)*length/2),(.008,width,length),ink,.003)
    o.rotation_euler.x=-angle;objects.append(o)
objects.append(cylinder('Clock spindle',(.11,0,0),.018,.012,ink,(0,math.pi/2,0)))
export('SM_LodgeClock',objects,False)

reset();oak=mat('M_WindowOak','dark_wooden_planks');MATERIALS['M_WindowOak']['factor']=[2.1,1.6,1.1,1]
brass=solid('M_LodgeDetailBrass',(.43,.25,.07),.3,.75)
objects=[box('Weathered lodge peg rail',(.025,0,0),(.05,2.50,.19),oak,.025)]
for y in [-1,-.5,0,.5,1]:
    objects.extend([cylinder('Brass peg backplate',(.065,y,0),.045,.014,brass,(0,math.pi/2,0)),box('Bent brass coat hook',(.12,y,-.005),(.12,.022,.022),brass,.009),box('Upturned hook end',(.173,y,.025),(.022,.022,.08),brass,.009)])
export('SM_LodgePegRail',objects,False)
(ROOT/'model-index.json').write_text(json.dumps(REPORT,indent=2))
(ROOT/'material-index.json').write_text(json.dumps(MATERIALS,indent=2))
print('LODGE WINDOW AND WALL DETAILS EXPORTED',flush=True)
