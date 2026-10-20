"""Original wearable meshes and matching studio thumbnails. No external assets.
Blender --background --python generate_headwear.py [-- --no-render]
Coordinates: metres, +X forward, +Y right, band at Z=0.
"""
import bpy, math, json, re, sys
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'ArtSource/Hunters/Headwear'
for folder in ['Export','Thumbnails/Studio']: (OUT/folder).mkdir(parents=True,exist_ok=True)
header=(ROOT/'Source/Wildline/WildlineHeadwear.h').read_text()
styles=[dict(id=i,slug=s,name=n,funny=f=='true',height=float(h)) for i,(s,n,f,h) in enumerate(re.findall(r'\{TEXT\("(.*?)"\),TEXT\("(.*?)"\),(true|false),([\d.]+)\}',header))]
assert len(styles)==37
bpy.ops.wm.read_factory_settings(use_empty=True)
MATS={}
def material(name,color,rough=.6,metal=0):
    m=bpy.data.materials.new('M_Hat_'+name);m.use_nodes=True
    p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*color,1);p.inputs['Roughness'].default_value=rough;p.inputs['Metallic'].default_value=metal
    MATS[name]=dict(asset=m.name,color=list(color),roughness=rough,metallic=metal)
    return m
olive=material('Olive',(.12,.18,.07));orange=material('Blaze',(.95,.21,.018));brown=material('Leather',(.18,.075,.026),.42)
dark=material('Charcoal',(.028,.035,.041));navy=material('Navy',(.025,.065,.16));cream=material('Canvas',(.66,.58,.38));white=material('Ivory',(.88,.9,.84));red=material('Crimson',(.65,.035,.025));yellow=material('SafetyYellow',(.96,.58,.018),.34)
green=material('DinosaurGreen',(.15,.4,.12));lime=material('FrogGreen',(.3,.64,.10));blue=material('Ocean',(.065,.28,.48));silver=material('Steel',(.34,.4,.43),.28,.72);gold=material('Brass',(.74,.45,.075),.28,.7);black=material('Black',(.008,.012,.018),.35)
purple=material('Wizard',(.16,.045,.34));pink=material('Pink',(.95,.22,.34));fur=material('Fur',(.27,.20,.12));mint=material('Mint',(.28,.77,.58));acorn=material('Acorn',(.39,.17,.045));spot=material('Camo',(.045,.075,.028))
def finish(o,m,bevel=0):
    o.data.materials.append(m)
    for p in o.data.polygons:p.use_smooth=True
    if bevel:
        mod=o.modifiers.new('Tailored rounded edge','BEVEL');mod.width=bevel;mod.segments=3
        bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=mod.name)
    return o
def ell(p,scale,m):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=32,ring_count=20,location=p);o=bpy.context.object;o.scale=scale;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);return finish(o,m)
def box(p,size,m,bevel=.004):
    bpy.ops.mesh.primitive_cube_add(size=1,location=p);o=bpy.context.object;o.scale=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);return finish(o,m,bevel)
def cyl(p,r,h,m,r2=None):
    bpy.ops.mesh.primitive_cone_add(vertices=64,radius1=r,radius2=r if r2 is None else r2,depth=h,location=p);return finish(bpy.context.object,m,.002)
def tube(points,r,m):
    c=bpy.data.curves.new('Sewn trim','CURVE');c.dimensions='3D';c.bevel_depth=r;c.bevel_resolution=2;s=c.splines.new('POLY');s.points.add(len(points)-1)
    for dst,p in zip(s.points,points):dst.co=(*p,1)
    o=bpy.data.objects.new('Raised trim',c);bpy.context.collection.objects.link(o);o.data.materials.append(m);bpy.context.view_layer.objects.active=o;o.select_set(True);bpy.ops.object.convert(target='MESH');o.select_set(False);return o
def ring(z,rx,ry,r,m):return tube([(rx*math.cos(a),ry*math.sin(a),z) for a in [i*math.tau/64 for i in range(65)]],r,m)
def lathe(profile,m,rx=.104,ry=.094):
    vs=[];fs=[];N=64
    for z,r in profile:
        for j in range(N):a=j/N*math.tau;vs.append((rx*r*math.cos(a),ry*r*math.sin(a),z))
    for k in range(len(profile)-1):
        for j in range(N):fs.append((k*N+j,k*N+(j+1)%N,(k+1)*N+(j+1)%N,(k+1)*N+j))
    mesh=bpy.data.meshes.new('Open wearable shell');mesh.from_pydata(vs,[],fs);o=bpy.data.objects.new('Crown shell',mesh);bpy.context.collection.objects.link(o);finish(o,m)
    mod=o.modifiers.new('Fabric shell thickness','SOLIDIFY');mod.thickness=.003;bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=mod.name);return o
def dome(m,h=.11,rx=.104,ry=.094):return lathe([(math.sin(i/16*math.pi/2)*h,max(.002,math.cos(i/16*math.pi/2))) for i in range(17)],m,rx,ry)
def brim(m,rx=.16,ry=.15,curve=0):
    vs=[];fs=[];N=80
    for t in [0,.5,1]:
        for j in range(N):
            a=j/N*math.tau;x=(.094+(rx-.094)*t)*math.cos(a);y=(.084+(ry-.084)*t)*math.sin(a);vs.append((x,y,-.008+curve*t*math.sin(a)**2))
    for k in range(2):
        for j in range(N):fs.append((k*N+j,k*N+(j+1)%N,(k+1)*N+(j+1)%N,(k+1)*N+j))
    mesh=bpy.data.meshes.new('Shaped brim');mesh.from_pydata(vs,[],fs);o=bpy.data.objects.new('Hat brim',mesh);bpy.context.collection.objects.link(o);finish(o,m);mod=o.modifiers.new('Brim thickness','SOLIDIFY');mod.thickness=.004;bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=mod.name)
def bill(m):
    o=ell((.10,0,.002),(.105,.104,.009),m);return o
def cap(m):
    dome(m);bill(m);ring(.005,.105,.095,.003,m);ell((0,0,.113),(.009,.009,.005),m)
    for a in [0,math.pi/2,math.pi,3*math.pi/2]:tube([(.105*math.cos(t)*math.cos(a),.095*math.cos(t)*math.sin(a),.112*math.sin(t)) for t in [i/20*math.pi/2 for i in range(21)]],.0012,cream)
def band(m,z=.028):lathe([(z-.014,1.018),(z+.014,1.018)],m)
def eye(p,r=.015):
    ell(p,(r,r,r),white);ell((p[0]+r*.85,p[1],p[2]),(r*.32,r*.63,r*.63),black);ell((p[0]+r*.96,p[1]-.003,p[2]+r*.25),(r*.13,r*.17,r*.17),white)
def horns(m):
    for side in [-1,1]:tube([(0,side*.08,.10),(-.025,side*.14,.13),(-.015,side*.19,.20),(.005,side*.20,.25)],.018,m)
def teeth(x,ys,z):
    for y in ys:
        o=cyl((x,y,z),.006,.021,white,r2=.001);o.rotation_euler.y=math.pi
def make(slug):
    if slug=='None':
        dome(dark,.12);return
    if slug in ['FieldCap','BlazeCap','CamoCap','Trucker','Propeller']:
        cap({'FieldCap':olive,'BlazeCap':orange,'CamoCap':olive,'Trucker':blue,'Propeller':red}[slug])
        if slug=='CamoCap':
            for i in range(16):
                a=i*2.399;z=.015+(i%5)*.015;r=math.sqrt(max(0,1-(z/.11)**2));ell((.105*r*math.cos(a),.095*r*math.sin(a),z),(.019,.011,.012),spot if i%2 else brown)
        if slug=='Trucker':lathe([(.012,1.01),(.055,.88),(.085,.60)],white)
        if slug=='Propeller':
            cyl((0,0,.145),.006,.07,silver);box((0,0,.185),(.24,.023,.005),mint);box((0,0,.185),(.023,.24,.005),yellow);ell((0,0,.19),(.015,.015,.01),blue)
    elif slug in ['Beanie','WatchCap']:
        m=orange if slug=='Beanie' else navy;dome(m,.14);band(m,.018)
        for i in range(40):
            a=i/40*math.tau;tube([(.105*math.cos(t)*math.cos(a),.095*math.cos(t)*math.sin(a),.14*math.sin(t)) for t in [k/14*math.pi/2 for k in range(15)]],.0017,m)
        if slug=='Beanie':ell((0,0,.151),(.025,.025,.025),cream)
    elif slug=='Trapper':
        dome(brown);band(fur,.025)
        for s in [-1,1]:ell((-.025,s*.103,-.025),(.069,.02,.08),fur);tube([(-.01,s*.11,-.04),(.02,s*.06,-.12)],.003,brown)
    elif slug in ['Boonie','Cowboy','Ranger','Fedora','Pith','FlatCap']:
        m={'Boonie':olive,'Cowboy':brown,'Ranger':olive,'Fedora':dark,'Pith':cream,'FlatCap':brown}[slug]
        if slug=='FlatCap':dome(m,.075,.13,.098);bill(m)
        else:
            brim(m,.18 if slug=='Cowboy' else .16,.15,.035 if slug=='Cowboy' else 0)
            if slug in ['Cowboy','Fedora']:lathe([(0,1),(.08,.99),(.125,.85),(.137,.70),(.125,.35),(.125,.001)],m)
            elif slug=='Ranger':lathe([(0,1),(.10,.9),(.17,.30),(.15,.001)],m)
            else:dome(m,.12 if slug=='Boonie' else .15)
            band(brown if m!=brown else dark)
        if slug=='Boonie':
            for i in range(8):a=i*math.tau/8;ell((.106*math.cos(a),.096*math.sin(a),.048),(.006,.006,.006),black)
        if slug in ['Ranger','Cowboy']:ell((.106,0,.045),(.004,.019,.023),gold)
    elif slug in ['HardHat','Foreman','Firefighter']:
        m=yellow if slug=='HardHat' else white if slug=='Foreman' else red;dome(m,.135,.112,.105);brim(m,.146,.13)
        for y in [-.045,0,.045]:
            k=math.sqrt(1-(y/.105)**2);tube([(.112*k*math.cos(t),y,.003+.135*k*math.sin(t)) for t in [i/25*math.pi for i in range(26)]],.004,m)
        if slug=='Firefighter':box((.112,0,.075),(.012,.065,.07),gold);box((.12,0,.075),(.004,.035,.046),dark);box((-.13,0,-.008),(.10,.24,.009),red)
    elif slug=='Astronaut':
        dome(white,.18,.14,.13);ring(.0,.14,.13,.015,silver);box((.113,0,.075),(.022,.17,.10),gold,.018)
        for s in [-1,1]:ell((0,s*.135,.07),(.045,.018,.045),dark);ell((.002,s*.15,.07),(.025,.009,.025),silver)
    elif slug in ['Chicken','Duck']:
        m=white if slug=='Chicken' else yellow;dome(m,.16,.115,.105);ell((.075,0,.13),(.08,.078,.075),m)
        if slug=='Chicken':
            for i in range(4):ell((-.065+i*.036,0,.17+i%2*.018),(.028,.018,.045),red)
            o=cyl((.16,0,.12),.033,.075,yellow,r2=0);o.rotation_euler.y=math.pi/2
            for s in [-1,1]:ell((.12,s*.021,.055),(.02,.013,.035),red)
        else:ell((.16,0,.095),(.067,.045,.015),orange)
        for s in [-1,1]:eye((.112,s*.061,.15),.014)
    elif slug in ['TRex','Triceratops']:
        dome(green,.155,.12,.105);ell((.115,0,.075),(.11,.078,.048),green)
        for s in [-1,1]:ell((.07,s*.077,.12),(.05,.035,.045),green);eye((.10,s*.077,.13),.02)
        if slug=='TRex':
            for x in [.08,.12,.16]:teeth(x,[-.058,.058],.031)
            for x,z in [(-.085,.12),(-.05,.16),(0,.18)]:cyl((x,0,z),.02,.045,cream,r2=0)
        else:
            o=ell((-.065,0,.12),(.03,.16,.11),olive);o.rotation_euler.y=-.25
            for s in [-1,1]:
                tube([(.06,s*.06,.12),(.11,s*.085,.19),(.18,s*.09,.23)],.014,cream)
            cyl((.19,0,.12),.014,.07,cream,r2=0)
    elif slug=='Shark':
        dome(blue,.15,.12,.10);ell((.11,0,.095),(.10,.09,.05),blue);ell((.12,0,.057),(.10,.077,.015),white)
        for s in [-1,1]:eye((.13,s*.07,.13),.012)
        for x in [.10,.14,.18]:teeth(x,[-.06,.06],.055)
        o=cyl((-.045,0,.18),.065,.12,blue,r2=0);o.scale.y=.25;o.rotation_euler.y=-.3
    elif slug=='Frog':
        dome(lime,.11);brim(lime,.13,.12)
        for s in [-1,1]:ell((.055,s*.07,.13),(.036,.035,.043),lime);eye((.08,s*.07,.145),.02)
        tube([(.11,-.05,.055),(.12,0,.04),(.11,.05,.055)],.0025,dark)
    elif slug=='Viking':dome(silver,.13);band(brown,.015);horns(cream);tube([(0,-.11,.02),(0,0,.14),(0,.11,.02)],.009,gold)
    elif slug=='Pirate':
        dome(dark,.09);brim(dark,.20,.17,.10);band(gold,.012);ell((.13,0,.06),(.006,.025,.028),white)
        for s in [-1,1]:tube([(.13,-s*.035,.035),(.135,s*.035,.085)],.004,white)
    elif slug=='TopHat':brim(dark,.16,.14);lathe([(0,1),(.23,.94),(.24,1),(.24,.001)],dark);band(red,.04);box((.109,0,.04),(.006,.04,.03),gold)
    elif slug=='Wizard':
        brim(purple,.19,.17);lathe([(0,1),(.13,.76),(.25,.43),(.33,.17),(.37,.001)],purple)
        for i in range(9):
            a=i*2.399;z=.04+(i%4)*.045;r=.104*(1-z/.37);ell((r*math.cos(a),r*.9*math.sin(a),z),(.007,.007,.007),gold)
    elif slug=='Crown':
        lathe([(0,1),(.08,1)],gold);ring(.006,.107,.097,.008,gold)
        for i in range(8):a=i/8*math.tau;cyl((.101*math.cos(a),.091*math.sin(a),.12),.019,.11,gold,r2=.002);ell((.101*math.cos(a),.091*math.sin(a),.177),(.009,.009,.009),red)
        for i in range(8):a=i/8*math.tau;ell((.106*math.cos(a),.096*math.sin(a),.043),(.012,.012,.016),blue)
    elif slug=='Chef':
        lathe([(0,1),(.11,1),(.12,.95)],white)
        for i in range(9):a=i/9*math.tau;ell((.06*math.cos(a),.06*math.sin(a),.20),(.07,.07,.10),white)
        band(white,.02)
    elif slug=='TrafficCone':
        box((0,0,0),(.25,.23,.017),orange);lathe([(0,1),(.35,.035),(.35,.001)],orange,.115,.105)
        for z in [.12,.23]:r=1-z/.35;lathe([(z,r+.01),(z+.035,r-.09)],white,.115,.105)
    elif slug=='UFO':
        dome(silver,.11);ell((0,0,.09),(.21,.19,.025),silver);dome(mint,.19,.085,.075)
        for i in range(12):a=i*math.tau/12;ell((.175*math.cos(a),.16*math.sin(a),.09),(.013,.013,.012),yellow if i%2 else pink)
    elif slug=='Pumpkin':
        dome(orange,.17,.13,.12)
        for i in range(10):a=i*math.tau/10;ell((.064*math.cos(a),.06*math.sin(a),.09),(.07,.065,.095),orange)
        cyl((0,0,.20),.015,.065,olive,r2=.009)
        for s in [-1,1]:ell((.132,s*.046,.09),(.008,.018,.022),dark)
        tube([(.14,-.045,.05),(.142,0,.03),(.14,.045,.05)],.004,dark)
    elif slug=='Acorn':
        dome(acorn,.22,.11,.10);lathe([(0,1.15),(.07,1.15),(.12,.85)],brown)
        for i in range(18):
            a=i*math.tau/18;tube([(.121*math.cos(a),.11*math.sin(a),.015),(.125*math.cos(a+.10),.114*math.sin(a+.10),.06),(.09*math.cos(a+.2),.08*math.sin(a+.2),.12)],.003,cream)
        tube([(0,0,.19),(.02,0,.25),(.04,0,.27)],.012,brown)
    elif slug=='Mushroom':
        dome(red,.17,.21,.19);brim(white,.205,.185)
        for i in range(16):a=i*2.399;r=.035+(i%4)*.038;z=.17*math.sqrt(max(0,1-(r/.21)**2));ell((r*math.cos(a),r*.9*math.sin(a),z+.002),(.023,.02,.004),white)
    elif slug=='Antlers':
        cap(brown)
        for s in [-1,1]:
            main=[(-.02,s*.075,.10),(-.04,s*.13,.18),(-.07,s*.18,.26),(-.12,s*.20,.35)];tube(main,.012,cream)
            for k in [1,2]:p=main[k];tube([p,(p[0]+.05,p[1]+s*.025,p[2]+.07)],.008,cream)
    else:raise ValueError(slug)

scene=bpy.context.scene;scene.world=bpy.data.worlds.new('Headwear studio');scene.render.engine='CYCLES';scene.cycles.samples=12;scene.cycles.use_denoising=True
scene.render.resolution_x=256;scene.render.resolution_y=256;scene.render.resolution_percentage=100;scene.render.image_settings.file_format='PNG';scene.world.color=(.12,.17,.15)
scene.view_settings.view_transform='AgX';scene.view_settings.exposure=-2
for position,power,size in [((.6,-.5,.8),65,.55),((-.3,.5,.5),45,.45),((-.5,-.3,.3),35,.4)]:
    bpy.ops.object.light_add(type='AREA',location=position);l=bpy.context.object;l.data.energy=power;l.data.shape='DISK';l.data.size=size;l.rotation_euler=(Vector((0,0,.08))-l.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(.60,-.72,.45));cam=bpy.context.object;scene.camera=cam;cam.data.type='ORTHO';cam.data.ortho_scale=.54
report=[]
for style in styles:
    for o in list(bpy.data.objects):
        if o.type=='MESH':bpy.data.objects.remove(o,do_unlink=True)
    make(style['slug']);bpy.ops.object.select_all(action='DESELECT');parts=[o for o in bpy.data.objects if o.type=='MESH']
    for o in parts:o.select_set(True)
    bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();obj=bpy.context.object;obj.name='SM_Hat_'+style['slug'];scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    if style['id']>0:bpy.ops.export_scene.fbx(filepath=str(OUT/'Export'/(obj.name+'.fbx')),use_selection=True,object_types={'MESH'},add_leaf_bones=False,bake_anim=False,axis_forward='-Y',axis_up='Z',path_mode='STRIP')
    zmin=min((obj.matrix_world@v.co).z for v in obj.data.vertices);zmax=max((obj.matrix_world@v.co).z for v in obj.data.vertices)
    centre=Vector((.015,0,(zmin+zmax)/2));cam.rotation_euler=(centre-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=max(.46,zmax-zmin+.13)
    scene.render.filepath=str(OUT/'Thumbnails/Studio'/('T_Hat_'+style['slug']+'.png'))
    if '--no-render' not in sys.argv:bpy.ops.render.render(write_still=True)
    report.append({**style,'vertices':len(obj.data.vertices),'triangles':sum(len(p.vertices)-2 for p in obj.data.polygons),'zmin':zmin,'zmax':zmax})
    print('HEADWEAR_COMPLETE',style['slug'],flush=True)
(OUT/'styles.json').write_text(json.dumps(report,indent=2));(OUT/'materials.json').write_text(json.dumps(MATS,indent=2))
(OUT/'PROVENANCE.md').write_text('# Original WILDLINE headwear\n\nAll 36 wearable designs, meshes and matching studio thumbnails are original procedural artwork created for WILDLINE. No third-party models or textures are used. Rebuild using `Scripts/generate_headwear.py` in Blender.\n')
print('HEADWEAR_LIBRARY_COMPLETE',len(report),flush=True)
