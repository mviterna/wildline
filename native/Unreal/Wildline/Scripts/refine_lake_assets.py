"""Prepare the attributed textured bass and a detailed spinning rod for Unreal."""
from pathlib import Path
import bpy, math, json, sys
from mathutils import Vector, Matrix
root=Path(__file__).resolve().parent.parent
folder=root/'ArtSource/Lake';out=folder/'Export'
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=str(folder/'Fish/source/base_basic_pbr(9).glb'))
fish=[o for o in bpy.context.scene.objects if o.type=='MESH'][0]
embedded=folder/'Fish/Embedded';embedded.mkdir(parents=True,exist_ok=True)
for node in fish.data.materials[0].node_tree.nodes:
    if node.type!='TEX_IMAGE':continue
    im=node.image
    if 'diffuse' in im.name.lower() or 'normal' in im.name.lower():
        key='Diffuse' if 'diffuse' in im.name.lower() else 'Normal'
        im.filepath_raw=str(embedded/(key+'.png'));im.file_format='PNG';im.save()
    else:
        import numpy as np
        pixels=np.empty(len(im.pixels),dtype=np.float32);im.pixels.foreach_get(pixels);pixels=pixels.reshape((-1,4))
        for key,index in [('Roughness',1),('Metallic',2)]:
            image=bpy.data.images.new(key,width=im.size[0],height=im.size[1]);image.colorspace_settings.name='Non-Color';channel=np.ones_like(pixels);channel[:,:3]=pixels[:,index,None];image.pixels.foreach_set(channel.ravel());image.filepath_raw=str(embedded/(key+'.png'));image.file_format='PNG';image.save()
    print('Embedded texture',im.name,tuple(im.size),flush=True)
if '--textures-only' in sys.argv:sys.exit(0)
bpy.ops.object.select_all(action='DESELECT');fish.select_set(True);bpy.context.view_layer.objects.active=fish
world=fish.matrix_world.copy()
for v in fish.data.vertices:v.co=world@v.co
fish.parent=None;fish.matrix_world=Matrix.Identity(4)
sizes=[max(v.co[i] for v in fish.data.vertices)-min(v.co[i] for v in fish.data.vertices) for i in range(3)]
thin,up,long=sorted(range(3),key=lambda i:sizes[i])
rows=[[int(i==axis) for i in range(3)] for axis in [long,thin,up]]
mapping=Matrix(rows)
if mapping.determinant()<0:rows[1]=[-v for v in rows[1]];mapping=Matrix(rows)
for v in fish.data.vertices:v.co=mapping@v.co
low=Vector([min(v.co[i] for v in fish.data.vertices) for i in range(3)])
high=Vector([max(v.co[i] for v in fish.data.vertices) for i in range(3)])
centre=(low+high)/2
for v in fish.data.vertices:
    v.co=(v.co-centre)*(.55/(high.x-low.x));v.co.x*=-1;v.co.y*=-1
fish.data.update();bpy.context.view_layer.update()
fish.location=(0,0,0);fish.name='Photoreal largemouth bass'
fish.data.materials[0].name='M_PhotorealBass'
for p in fish.data.polygons:p.use_smooth=True
bpy.ops.export_scene.fbx(filepath=str(out/'SM_PhotorealBass.fbx'),use_selection=True,object_types={'MESH'},bake_anim=False,axis_forward='-Y',axis_up='Z',apply_unit_scale=True,path_mode='STRIP')
# Inspect the retained diffuse/normal/roughness asset under neutral light.
bpy.ops.object.camera_add(location=(.22,-.86,.20));cam=bpy.context.object;cam.rotation_euler=(Vector((0,0,0))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=.72;bpy.context.scene.camera=cam
for p,power,size in [((.3,-.5,.7),90,1),((-.4,.3,.3),65,.7)]:
    bpy.ops.object.light_add(type='AREA',location=p);light=bpy.context.object;light.data.energy=power;light.data.shape='DISK';light.data.size=size;light.rotation_euler=(-light.location).to_track_quat('-Z','Y').to_euler()
scene=bpy.context.scene;scene.world=bpy.data.worlds.new('Neutral inspection');scene.render.engine='CYCLES';scene.cycles.samples=32;scene.render.resolution_x=1200;scene.render.resolution_y=700;scene.render.resolution_percentage=100;scene.world.color=(.18,.18,.18);scene.render.filepath=str(folder/'Fish/BassInspection.png');bpy.ops.render.render(write_still=True)

# Reuse the original rod authoring helpers, while retaining the original boat
# and cabin exports. Cork rings, thread wraps and a separate animated crank.
original=(root/'Scripts/generate_lake_assets.py').read_text();helpers=original[:original.index('reset();green=')]
scope={'__file__':str(root/'Scripts/generate_lake_assets.py')};exec(compile(helpers,'rod_helpers','exec'),scope)
reset,mat,rod,box,export=[scope[n] for n in ['reset','mat','rod','box','export']]
reset();carbon=mat('M_LakeMetal',(.05,.07,.07));cork=mat('M_DockWood',(.52,.35,.16));brass=mat('M_ReelBrass',(.36,.30,.13));spool=mat('M_LineSpool',(.72,.78,.65))
rod('Cork butt grip',(0,0,0),(.24,0,0),.019,cork)
rod('Reel seat',(.24,0,0),(.38,0,0),.016,carbon)
rod('Cork foregrip',(.38,0,0),(.48,0,0),.021,cork)
for x in [.01,.23,.25,.36,.39,.47]:rod('Grip ring',(x,0,0),(x+.006,0,0),.022,brass)
rod('Tapered graphite blank',(.48,0,0),(2.2,0,.06),.008,carbon,.0018)
for x in [.55,.85,1.15,1.45,1.75,2.05,2.19]:
    z=(x-.48)/1.72*.06
    rod('Guide foot',(x-.015,0,z),(x,0,z+.025),.002,brass)
    bpy.ops.mesh.primitive_torus_add(major_radius=.014 if x<1.4 else .007,minor_radius=.0018,major_segments=24,minor_segments=8,location=(x,0,z+.025));o=bpy.context.object;o.rotation_euler.y=math.pi/2;o.data.materials.append(brass)
    for wrap in range(4):rod('Thread binding',(x-.024+wrap*.004,0,z),(x-.022+wrap*.004,0,z),.009,brass)
rod('Reel stem',(.29,0,0),(.29,0,-.055),.006,carbon)
rod('Reel housing',(.29,-.032,-.065),(.29,.032,-.065),.033,carbon)
rod('Spool axle',(.29,0,-.065),(.38,0,-.065),.012,brass)
rod('Wound line spool',(.33,0,-.065),(.38,0,-.065),.032,spool)
for x in [.325,.38]:rod('Spool lip',(x,0,-.065),(x+.004,0,-.065),.039,brass)
for j in range(20):rod('Visible wound line',(.332+j*.002,0,-.065),(.333+j*.002,0,-.065),.033,spool)
export('SM_FishingRod')
reset();carbon=mat('M_LakeMetal',(.05,.07,.07));cork=mat('M_DockWood',(.52,.35,.16));brass=mat('M_ReelBrass',(.36,.30,.13))
rod('Crank arm',(0,0,0),(.04,0,-.045),.005,brass);rod('Crank knob',(.04,-.012,-.045),(.04,.025,-.045),.012,cork);export('SM_FishingReelCrank')
print('REFINED LAKE ASSETS READY',flush=True)
