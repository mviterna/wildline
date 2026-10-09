"""Blender: preserve downloaded PBR models; author suburban hero meshes.

Run after download_neighborhood_assets.py. Original sources and .blend files stay
editable. Geometry is exported in metres with a floor-centred origin, no baking
of lighting into albedo. No Hitman content is used.
"""
import bpy, bmesh, json, math, random, sys, os
from pathlib import Path
from mathutils import Vector

ROOT=Path(__file__).resolve().parent.parent/'ArtSource/Neighborhood'
SRC=ROOT/'Photoreal';OUT=ROOT/'Export';OUT.mkdir(parents=True,exist_ok=True)
MATERIALS={};REPORT=[];rng=random.Random(18085)

def reset():bpy.ops.wm.read_factory_settings(use_empty=True)
def bounds(objects):
    p=[o.matrix_world@Vector(v) for o in objects for v in o.bound_box]
    return Vector([min(v[i] for v in p) for i in range(3)]),Vector([max(v[i] for v in p) for i in range(3)])
def export(name,objects,centre=False,anchor=None):
    # Work in world metres before centering; glTF kit meshes inherit assembly
    # rotations and translations from their parent empties.
    for o in objects:
        world=o.matrix_world.copy();o.parent=None;o.matrix_world=world
    bpy.context.view_layer.update()
    bpy.ops.object.select_all(action='DESELECT')
    for o in objects:o.select_set(True)
    bpy.context.view_layer.objects.active=objects[0];bpy.ops.object.convert(target='MESH')
    low,high=bounds(objects)
    offset=Vector(anchor) if anchor is not None else Vector(((low.x+high.x)/2,(low.y+high.y)/2,low.z)) if centre else Vector((0,0,0))
    for o in objects:o.location-=offset
    bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/(name+'.blend')))
    bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')),use_selection=True,object_types={'MESH'},bake_anim=False,axis_forward='-Y',axis_up='Z',path_mode='STRIP')
    REPORT.append({'name':name,'dimensions_m':list(high-low),'triangles':sum(sum(len(p.vertices)-2 for p in o.data.polygons) for o in objects),'materials':sorted({m.name for o in objects for m in o.data.materials if m})})
    print('EXPORTED',name,REPORT[-1]['triangles'],flush=True)

def convert():
    for entry in json.loads((SRC/'sources.json').read_text()):
        if entry['type']!='model':continue
        if '--poles-only' in sys.argv and entry['id']!='modular_electricity_poles':continue
        reset();aid=entry['id'];file=SRC/aid/(aid+'_4k.gltf');data=json.loads(file.read_text());bpy.ops.import_scene.gltf(filepath=str(file))
        def image(t):return str((file.parent/data['images'][data['textures'][t['index']]['source']]['uri']).relative_to(ROOT)) if t else None
        for i,s in enumerate(data.get('materials',[])):
            original=s.get('name','Material_'+str(i))
            matches=[m for m in bpy.data.materials if m.name==original or m.name.startswith(original+'.')]
            if not matches:continue # unused alternate glTF finish
            m=matches[0]
            # Blender's glTF variants extension can make .001 copies. Share the
            # same source finish so FBX slot names remain stable after sanitizing.
            for o in bpy.context.scene.objects:
                if o.type=='MESH':
                    for slot in o.material_slots:
                        if slot.material in matches:slot.material=m
            key='M_NH_'+aid+'_'+str(i);m.name=key;p=s.get('pbrMetallicRoughness',{})
            MATERIALS[key]={'color':image(p.get('baseColorTexture')),'normal':image(s.get('normalTexture')),'arm':image(p.get('metallicRoughnessTexture')),'factor':p.get('baseColorFactor',[1,1,1,1]),'roughness':p.get('roughnessFactor',1),'metallic':p.get('metallicFactor',1),'alpha':s.get('alphaMode','OPAQUE'),'cutoff':s.get('alphaCutoff',.5),'two_sided':s.get('doubleSided',False),'source':entry['page']}
        objects=[o for o in bpy.context.scene.objects if o.type=='MESH']
        if aid=='modular_electricity_poles':
            # The download is a parts library, not one giant utility pole.
            for preset in ['01','02','03']:
                chosen=[o for o in objects if o.name.startswith('preset_'+preset+'_')]
                assert chosen,preset
                stem=next(o for o in chosen if o.name=='preset_'+preset+'_pole');lo,hi=bounds([stem])
                export('SM_UtilityPole'+preset,chosen,True,((lo.x+hi.x)/2,(lo.y+hi.y)/2,lo.z))
        elif aid=='outdoor_table_chair_set_01':
            for o in objects:export('SM_'+o.name,[o],True)
        else:export('SM_'+aid,objects,True)

def material(name,asset=None,factor=(1,1,1,1),roughness=.65,metallic=0):
    m=bpy.data.materials.get(name) or bpy.data.materials.new(name);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=factor;p.inputs['Roughness'].default_value=roughness;p.inputs['Metallic'].default_value=metallic
    info={'factor':list(factor),'roughness':roughness,'metallic':metallic,'alpha':'OPAQUE'}
    if asset:
        texture_folder=SRC/asset
        if not texture_folder.exists():texture_folder=ROOT.parent/'Lobby/Photoreal'/asset
        for channel,key in [('diff','color'),('nor_gl','normal'),('rough','rough_map')]:
            f=next(texture_folder.glob('*_'+channel+'_4k.*'));info[key]=os.path.relpath(f,ROOT)
            if key=='color':
                n=m.node_tree.nodes.new('ShaderNodeTexImage');n.image=bpy.data.images.load(str(f));m.node_tree.links.new(n.outputs['Color'],p.inputs['Base Color'])
            elif key=='normal':
                n=m.node_tree.nodes.new('ShaderNodeTexImage');n.image=bpy.data.images.load(str(f));n.image.colorspace_settings.name='Non-Color';normal=m.node_tree.nodes.new('ShaderNodeNormalMap');m.node_tree.links.new(n.outputs['Color'],normal.inputs['Color']);m.node_tree.links.new(normal.outputs['Normal'],p.inputs['Normal'])
        info['source']='https://polyhaven.com/a/'+asset
    if name in ['M_HouseBlue','M_HouseCream']:
        info['paint']=list(factor[:3]);info['factor']=[1,1,1,1]
        # Painted clapboard retains photographic surface detail under the paint.
        base=next(n for n in m.node_tree.nodes if n.type=='TEX_IMAGE' and n.image and '_diff_' in n.image.name)
        mix=m.node_tree.nodes.new('ShaderNodeMixRGB');mix.blend_type='MIX';mix.inputs[0].default_value=.88;mix.inputs[2].default_value=factor;m.node_tree.links.new(base.outputs['Color'],mix.inputs[1]);m.node_tree.links.new(mix.outputs['Color'],p.inputs['Base Color'])
    MATERIALS[name]=info;return m
def box(p,s,m,bevel=.008,name='Bevelled component'):
    bpy.ops.mesh.primitive_cube_add(size=1,location=p);o=bpy.context.object;o.name=name;o.dimensions=s;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(m)
    uv=o.data.uv_layers.active
    for face in o.data.polygons:
        axes=[i for i in range(3) if i!=max(range(3),key=lambda i:abs(face.normal[i]))]
        for li in face.loop_indices:
            v=o.data.vertices[o.data.loops[li].vertex_index].co+Vector(p);uv.data[li].uv=(v[axes[0]],v[axes[1]])
    if bevel:
        b=o.modifiers.new('Soft physical edges','BEVEL');b.width=bevel;b.segments=3;o.modifiers.new('Weighted normals','WEIGHTED_NORMAL')
    return o
def cylinder(p,r,h,m,vertices=48):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices,radius=r,depth=h,location=p);o=bpy.context.object;o.data.materials.append(m)
    for f in o.data.polygons:f.use_smooth=len(f.vertices)==4
    b=o.modifiers.new('Rolled edge','BEVEL');b.width=min(.006,r*.12);b.segments=3;return o
def tube(points,r,m):
    c=bpy.data.curves.new('Formed tube','CURVE');c.dimensions='3D';c.bevel_depth=r;c.bevel_resolution=3;c.resolution_u=12;s=c.splines.new('POLY');s.points.add(len(points)-1)
    for p,v in zip(s.points,points):p.co=(*v,1)
    o=bpy.data.objects.new('Formed tube',c);bpy.context.collection.objects.link(o);o.data.materials.append(m);return o
def ring(p,r,t,m):
    bpy.ops.mesh.primitive_torus_add(major_segments=96,minor_segments=12,location=p,major_radius=r,minor_radius=t);o=bpy.context.object;o.data.materials.append(m)
    for f in o.data.polygons:f.use_smooth=True
    return o
def meshes():return [o for o in bpy.context.scene.objects if o.type in ('MESH','CURVE')]

def feeder():
    reset();cedar=material('M_FeederCedar','weathered_plank_siding');metal=material('M_Galvanized',factor=(.38,.40,.42,1),roughness=.32,metallic=.9);roof=material('M_FeederRoof',factor=(.15,.19,.17,1),roughness=.37,metallic=.75);glass=material('M_FeederHopper',factor=(.80,.86,.87,.20),roughness=.08);seed=material('M_BirdSeed',factor=(.47,.34,.12,1),roughness=.85)
    MATERIALS['M_FeederHopper']['alpha']='BLEND';glass.node_tree.nodes.get('Principled BSDF').inputs['Transmission Weight'].default_value=.95
    # A plausible 75 cm cedar hopper feeder, tray exactly 240 cm above the lawn.
    cylinder((0,0,1.18),.045,2.36,metal);ring((0,0,.07),.09,.012,metal)
    bpy.ops.mesh.primitive_cone_add(vertices=96,radius1=.39,radius2=.06,depth=.28,location=(0,0,1.34));bpy.context.object.data.materials.append(metal)
    for face in bpy.context.object.data.polygons:face.use_smooth=len(face.vertices)==4
    for z in [1.21,1.48]:ring((0,0,z),.39 if z<1.3 else .06,.007,metal)
    box((0,0,2.365),(.80,.68,.07),cedar,.01,'Cedar seed tray')
    for side in [-1,1]:
        box((side*.385,0,2.425),(.026,.68,.085),cedar,.006);box((0,side*.325,2.425),(.80,.026,.085),cedar,.006)
    for x in [-.26,.26]:
        for y in [-.21,.21]:box((x,y,2.62),(.045,.045,.43),cedar,.005)
    for y in [-.21,.21]:box((0,y,2.66),(.49,.008,.31),glass,.002,'Reflective seed hopper pane')
    for side in [-1,1]:
        o=box((side*.18,0,2.88),(.44,.81,.027),roof,.009,'Folded metal roof');o.rotation_euler.y=side*math.radians(27)
        tube([(side*.36,-.405,2.785),(side*.36,.405,2.785)],.007,metal)
    tube([(0,-.41,2.98),(0,.41,2.98)],.011,roof)
    for i in range(220):
        x=rng.uniform(-.33,.33);y=rng.uniform(-.27,.27)
        bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1,radius=.007,location=(x,y,2.408+rng.uniform(0,.005)));o=bpy.context.object;o.scale=(.75,.6,1.6);o.rotation_euler=(rng.random()*3,rng.random()*3,rng.random()*3);o.data.materials.append(seed)
    for x in [-.385,.385]:
        for y in [-.27,.27]:cylinder((x,y,2.473),.004,.005,metal,12)
    export('SM_CedarBirdFeeder',meshes())

def trampoline():
    reset();metal=material('M_Galvanized',factor=(.38,.40,.42,1),roughness=.32,metallic=.9);pad=material('M_TrampolinePad',factor=(.018,.13,.14,1),roughness=.67);mat=material('M_TrampolineMat',factor=(.019,.022,.023,1),roughness=.87)
    # A compact 2.4 m backyard trampoline; spring ring and separate fabric bed.
    cylinder((0,0,.885),.95,.025,mat,96);ring((0,0,.87),1.12,.09,pad);ring((0,0,.81),1.17,.026,metal)
    for i in range(8):
        a=i*math.tau/8;x=math.cos(a);y=math.sin(a);tube([(x*1.15,y*1.15,.81),(x*1.15,y*1.15,.10),(x*.90,y*.90,.07)],.024,metal)
        cylinder((x*.90,y*.90,.045),.036,.07,mat)
    for i in range(64):
        a=i*math.tau/64
        pts=[((.96+t*.18)*math.cos(a),(.96+t*.18)*math.sin(a),.856+.009*math.sin(t*math.tau*7)) for t in [j/35 for j in range(36)]];tube(pts,.003,metal)
    export('SM_BackyardTrampoline',meshes())

def fence():
    reset();wood=material('M_PrivacyCedar','wood_peeling_paint_weathered',(.75,.72,.65,1));metal=material('M_Galvanized',factor=(.38,.40,.42,1),roughness=.32,metallic=.9)
    for x in [-1.0,1.0]:
        box((x,0,.92),(.095,.095,1.84),wood,.009);box((x,0,1.86),(.14,.14,.06),wood,.012)
    for z in [.30,1.35]:box((0,.06,z),(2.08,.055,.095),wood,.006)
    for i in range(18):
        x=-.94+i*.11;box((x,-.025,.90+rng.uniform(-.004,.004)),(.106,.025,1.8),wood,.003)
        for z in [.30,1.35]:
            o=cylinder((x,-.041,z),.003,.003,metal,12);o.rotation_euler.x=math.pi/2
    # Continuous cap is a believable walkable fence top, not an obstacle platform.
    box((0,0,1.875),(2.1,.14,.05),wood,.007);export('SM_PrivacyFence',meshes())

def branch():
    reset();bark=material('M_CourseBark','jolcham_oak_bark_01',roughness=.9)
    # A broad horizontal limb at 5.4 m, with sculpted ridges and small forks.
    for i in range(4):
        tube([(-1.15,0,-.13),(-.65,.035,-.06),(0,0,-.12),(.48,.025,-.20),(.82,.07,-.36)],.15-i*.014,bark)
    tube([(-.6,0,-.06),(-.44,.18,-.02),(-.18,.30,.02)],.035,bark)
    export('SM_CourseBranch',meshes())

def course_details():
    reset();metal=material('M_ServiceCable',factor=(.014,.017,.019,1),roughness=.70)
    # 3 m service cable. A thick insulation sheath gives a real landing surface.
    tube([(-1.5,0,-.02),(1.5,0,-.02)],.02,metal)
    export('SM_ServiceCable',meshes())
    reset();soil=material('M_PlanterSoil',factor=(.09,.055,.025,1),roughness=1)
    export('SM_PlanterSoil',[box((0,0,-.025),(1.13,.93,.05),soil,.006)])
    reset();wood=material('M_FeederCedar','weathered_plank_siding');metal=material('M_Galvanized',factor=(.38,.40,.42,1),roughness=.32,metallic=.9)
    for x in [-.70,.70]:
        for y in [-.29,.29]:box((x,y,.41),(.065,.065,.82),wood,.007)
    for i in range(7):box((0,-.315+i*.105,.855),(1.6,.100,.09),wood,.006)
    for i in range(5):box((0,-.21+i*.105,.19),(1.48,.10,.045),wood,.005)
    for x in [-.65,.65]:
        for y in [-.28,.28]:cylinder((x,y,.902),.004,.005,metal,12)
    export('SM_PottingBench',meshes())

def house(cape=False):
    reset();siding=material('M_HouseBlue' if cape else 'M_HouseCream','weathered_plank_siding',(.57,.68,.76,1) if cape else (.94,.91,.79,1));trim=material('M_HouseTrim',factor=(.85,.86,.81,1),roughness=.73);roof=material('M_HouseShingles','roof_07');foundation=material('M_HouseFoundation','concrete_pavement');glass=material('M_WindowReflection',factor=(.09,.16,.19,1),roughness=.16,metallic=.48);door=material('M_HouseDoor',factor=(.12,.18,.17,1),roughness=.63);brick=material('M_HouseBrick',factor=(.34,.14,.09,1),roughness=.9)
    eave=3.35 if cape else 5.65;peak=eave+2.0
    box((0,0,.18),(11.2,8.4,.36),foundation,.025)
    box((0,0,eave/2),(11,8.1,eave),siding,.01)
    # Fine physical clapboard shadow courses instead of texture-only box walls.
    for j in range(int(eave/.16)):
        z=.34+j*.16
        for side in [-1,1]:box((0,side*4.06,z),(11.0,.026,.014),siding,.001)
    for x in [-5.47,5.47]:
        for y in [-4.07,4.07]:box((x,y,eave/2),(.12,.10,eave),trim,.006)
    angle=math.atan2(2,4.25)
    for side in [-1,1]:
        o=box((0,side*2.17,eave+.96),(11.65,4.87,.09),roof,.012);o.rotation_euler.x=-side*angle
        box((0,side*4.36,eave-.04),(11.6,.11,.16),trim,.006)
        for x in [-5.65,5.65]:tube([(x,side*4.4,eave),(x,side*4.44,.2),(x,side*4.7,.16)],.045,trim)
    for x in [-5.5,5.5]:
        vs=[(x+d,y,z) for d in [0,.06 if x>0 else -.06] for y,z in [(-4.1,eave),(4.1,eave),(0,peak)]];mesh=bpy.data.meshes.new('Closed gable');mesh.from_pydata(vs,[],[(0,1,2),(5,4,3),(0,3,4,1),(1,4,5,2),(2,5,3,0)]);mesh.materials.append(siding)
        bm=bmesh.new();bm.from_mesh(mesh);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(mesh);bm.free();o=bpy.data.objects.new('Clapboard gable',mesh);bpy.context.collection.objects.link(o)
        uv=mesh.uv_layers.new();
        for li,l in enumerate(mesh.loops):v=mesh.vertices[l.vertex_index].co;uv.data[li].uv=(v.y,v.z)
    def window(x,y,z):
        box((x,y,z),(1.38,.11,1.68),trim,.01);box((x,y-.065,z),(1.17,.025,1.46),glass,.002)
        for xx in [-.39,0,.39]:box((x+xx,y-.085,z),(.027,.024,1.44),trim,.002)
        for zz in [-.24,.24]:box((x,y-.085,z+zz),(1.17,.024,.027),trim,.002)
        box((x,y-.14,z-.88),(1.55,.28,.075),trim,.007)
        for side in [-1,1]:
            box((x+side*.87,y-.055,z),(.30,.07,1.68),door,.005)
            for slat in range(12):box((x+side*.87,y-.10,z-.72+slat*.13),(.25,.025,.025),door,.001)
    for x in [-3.45,3.45]:
        window(x,-4.10,1.85)
        if not cape:window(x,-4.10,4.5)
    if not cape:window(0,-4.10,4.5)
    from mathutils import Matrix
    for side in [-1,1]:
        for yy in [-2.0,2.0]:
            for zz in ([1.85] if cape else [1.85,4.5]):
                old=set(bpy.context.scene.objects);window(0,0,zz);rotation=Matrix.Rotation(side*math.pi/2,4,'Z')
                for o in set(bpy.context.scene.objects)-old:o.location=rotation@o.location+Vector((side*5.55,yy,0));o.rotation_euler.z+=side*math.pi/2
    box((0,-4.12,1.43),(1.05,.12,2.35),door,.015)
    for z in [.66,1.34,2.02]:box((0,-4.20,z),(.75,.02,.42),trim,.008)
    cylinder((.34,-4.23,1.4),.025,.045,material('M_DoorBrass',factor=(.48,.32,.08,1),roughness=.25,metallic=.8))
    # Front porch, railings, turned posts and narrow entry steps.
    for j in range(18):box((0,-4.10-j*.10,.46),(6.8,.096,.07),trim,.004)
    for x in [-3.18,-1.50,1.50,3.18]:
        box((x,-5.77,1.95),(.12,.12,3.0),trim,.012)
        for z in [.6,3.22]:box((x,-5.77,z),(.21,.21,.10),trim,.01)
    box((0,-5.18,3.4),(7.05,2.40,.13),roof,.01)
    for side in [-1,1]:
        box((side*2.30,-5.82,1.28),(1.72,.08,.085),trim,.006)
        for j in range(12):box((side*(1.55+j*.14),-5.82,.91),(.04,.04,.68),trim,.004)
    for i in range(3):box((0,-6.03-i*.28,.36-i*.12),(1.9,.32,.12),foundation,.018)
    box((3,1.8,eave+1.2),(.75,.85,2.9),brick,.012)
    for j in range(14):box((3,1.8,eave-.2+j*.18),(.78,.88,.012),trim,.001)
    export('SM_CapeCodHouse' if cape else 'SM_ColonialHouse',meshes())

def paving():
    for name,asset,size in [('SM_AsphaltStreet','aerial_asphalt_01',(110,11,.06)),('SM_ConcreteWalk','concrete_pavement',(110,2.2,.12)),('SM_Driveway','concrete_pavement',(3.8,18,.06))]:
        reset();m=material('M_Asphalt' if 'Asphalt' in name else 'M_Sidewalk',asset);export(name,[box((0,0,-size[2]/2),size,m,.006)])
    reset();m=material('M_Sidewalk','concrete_pavement');export('SM_Curb',[box((0,0,.08),(2,.20,.16),m,.02)])
    material('M_NeighborhoodLawn','grass_path_2',factor=(.24,.52,.14,1))

if '--soil-only' in sys.argv:
    MATERIALS=json.loads((ROOT/'material-index.json').read_text());REPORT=[r for r in json.loads((ROOT/'model-index.json').read_text()) if r['name'] not in ['SM_ServiceCable','SM_PlanterSoil','SM_PottingBench']];course_details()
elif '--houses-only' in sys.argv:
    MATERIALS=json.loads((ROOT/'material-index.json').read_text());REPORT=[r for r in json.loads((ROOT/'model-index.json').read_text()) if r['name'] not in ['SM_ColonialHouse','SM_CapeCodHouse','SM_CourseBranch']];house(False);house(True);branch()
elif '--poles-only' in sys.argv:
    MATERIALS=json.loads((ROOT/'material-index.json').read_text());REPORT=[r for r in json.loads((ROOT/'model-index.json').read_text()) if not r['name'].startswith('SM_UtilityPole')];convert()
else:
    convert();feeder();trampoline();fence();branch();course_details();house(False);house(True);paving()
(ROOT/'model-index.json').write_text(json.dumps(REPORT,indent=2));(ROOT/'material-index.json').write_text(json.dumps(MATERIALS,indent=2))
print('NEIGHBORHOOD CONVERSION COMPLETE',len(REPORT),'meshes',len(MATERIALS),'materials',flush=True)
