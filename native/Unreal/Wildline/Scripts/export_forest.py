"""Convert CC0 BlendSwap geometry into meter-scaled Unreal FBX assets.
Downloaded blend scripts are disabled; third-party tree textures are excluded.
"""
import bpy, math, random, json
from pathlib import Path
from mathutils import Vector, Matrix
root=Path(__file__).resolve().parent.parent/'ArtSource/Forest'
out=root/'Export';out.mkdir(exist_ok=True)
rng=random.Random(84672);manifest=[]
def open_file(path):
    bpy.ops.wm.open_mainfile(filepath=str(root/path),use_scripts=False)
    if bpy.context.object and bpy.context.object.mode!='OBJECT':bpy.ops.object.mode_set(mode='OBJECT')
def mesh_object(name,verts,faces,uvs=None,material='F_Grass'):
    mesh=bpy.data.meshes.new(name);mesh.from_pydata(verts,[],faces);mesh.update()
    obj=bpy.data.objects.new(name,mesh);bpy.context.collection.objects.link(obj)
    mesh.materials.append(bpy.data.materials.get(material) or bpy.data.materials.new(material))
    for p in mesh.polygons:p.use_smooth=True
    if uvs:
        layer=mesh.uv_layers.new(name='UVMap')
        for p in mesh.polygons:
            for li,vi in zip(p.loop_indices,p.vertices):layer.data[li].uv=uvs[vi]
    return obj
def export(obj,name):
    if not obj.data.color_attributes:
        colors=obj.data.color_attributes.new(name='Growth',type='FLOAT_COLOR',domain='CORNER')
        maxz=max(v.co.z for v in obj.data.vertices)
        for poly in obj.data.polygons:
            for li,vi in zip(poly.loop_indices,poly.vertices):
                v=obj.data.vertices[vi].co
                colors.data[li].color=(max(0,min(1,v.z/max(maxz,.01))),.5+.5*math.sin(v.x*81+v.y*57),1,1)
    bpy.ops.object.select_all(action='DESELECT');obj.hide_set(False);obj.hide_viewport=False;obj.select_set(True);bpy.context.view_layer.objects.active=obj
    bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=1
    bpy.ops.export_scene.fbx(filepath=str(out/(name+'.fbx')),use_selection=True,object_types={'MESH'},use_mesh_modifiers=True,mesh_smooth_type='FACE',bake_anim=False,axis_forward='-Y',axis_up='Z',apply_unit_scale=True,path_mode='STRIP')
    vs=[obj.matrix_world@v.co for v in obj.data.vertices]
    lo=[min(v[k] for v in vs) for k in range(3)];hi=[max(v[k] for v in vs) for k in range(3)]
    manifest.append({'name':name,'height_cm':(hi[2]-lo[2])*100,'triangles':sum(len(p.vertices)-2 for p in obj.data.polygons),'materials':[m.name for m in obj.data.materials]})
def clone_scaled(obj,name,height):
    # Bake source transforms but remove display-grid translation; set root to ground.
    verts=[obj.matrix_world.to_3x3()@v.co for v in obj.data.vertices]
    low=min(v.z for v in verts);h=max(v.z for v in verts)-low;s=height/h
    verts=[Vector((v.x*s,v.y*s,(v.z-low)*s)) for v in verts]
    res=mesh_object(name,verts,[list(p.vertices) for p in obj.data.polygons],material='F_Grass')
    if obj.data.uv_layers:
        layer=res.data.uv_layers.new(name='UVMap')
        for p,op in zip(res.data.polygons,obj.data.polygons):
            for li,oli in zip(p.loop_indices,op.loop_indices):layer.data[li].uv=obj.data.uv_layers.active.data[oli].uv
    return res

open_file(Path('GrassField/Realistic Grass Field.blend'))
for src,name,h in [('grass_1','SM_GrassA',.23),('grass_3','SM_GrassB',.30),('grass_14','SM_GrassC',.16),('grass_9.001','SM_DryGrass',.26),('clovers_3','SM_Clover',.10),('clovers_5','SM_CloverWide',.09),('weed','SM_Weed',.50),('weed.006','SM_WeedB',.40)]:
    obj=clone_scaled(bpy.data.objects[src],name,h)
    if 'Dry' in name:obj.data.materials[0]=bpy.data.materials.new('F_DryGrass')
    export(obj,name)

open_file(Path('Ferns/BSwap_FernModels_01.blend'))
for image in bpy.data.images:
    if image.packed_file:
        image.filepath_raw=str(out/(image.name.replace('.tga','.png')));image.file_format='PNG';image.save()
for i in range(1,5):
    obj=clone_scaled(bpy.data.objects[f'Fern_{i:02}'],f'SM_Fern{i}',.60 if i==1 else .38)
    obj.data.materials[0]=bpy.data.materials.new('F_Fern');export(obj,f'SM_Fern{i}')

open_file(Path('Broadleaf/trees_grass_system.blend'))
for src,name in [('oaktree1','SM_OakA'),('oaktree3','SM_OakB'),('oaktree2','SM_OakC')]:
    orig=bpy.data.objects[src];matrix=orig.matrix_world.to_3x3()
    vv=[matrix@v.co for v in orig.data.vertices];minz=min(v.z for v in vv);height=max(v.z for v in vv)-minz;s=24/height
    vv=[Vector((v.x*s,v.y*s,(v.z-minz)*s)) for v in vv]
    # Locate the trunk root independently of the crown's asymmetric bounds.
    roots=[v for v in vv if v.z<.12];cx=sum(v.x for v in roots)/len(roots);cy=sum(v.y for v in roots)/len(roots)
    vv=[v-Vector((cx,cy,0)) for v in vv]
    barkv=[];barkf=[];barkuv=[];leafv=[];leaff=[];leafuv=[];singlev=[];singlef=[];singleuv=[]
    handled=set()
    for p in orig.data.polygons:
        mat=orig.data.materials[p.material_index].name.lower()
        if 'leaves' not in mat and 'branches' not in mat:
            start=len(barkv)
            for vi,li in zip(p.vertices,p.loop_indices):barkv.append(vv[vi]);barkuv.append(tuple(orig.data.uv_layers.active.data[li].uv) if orig.data.uv_layers else (0,0))
            barkf.append(tuple(range(start,len(barkv))));continue
        # The source leaves have zero UVs (Blender used generated coordinates).
        # Give every leaf and branch card its own complete photographic mask.
        targetv,targetf,targetuv=(singlev,singlef,singleuv) if 'leaves' in mat else (leafv,leaff,leafuv)
        start=len(targetv)
        for j,vi in enumerate(p.vertices):
            targetv.append(vv[vi]);targetuv.append([(0,0),(1,0),(1,1),(0,1)][j%4])
        targetf.append(tuple(range(start,len(targetv))))
    wood=mesh_object(name+'_Wood',barkv,barkf,barkuv,'F_Bark')
    bpy.ops.object.select_all(action='DESELECT');wood.select_set(True);bpy.context.view_layer.objects.active=wood;bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.mesh.remove_doubles(threshold=.00005);bpy.ops.object.mode_set(mode='OBJECT')
    for poly in wood.data.polygons:poly.use_smooth=True
    leaves=mesh_object(name+'_Leaves',leafv,leaff,leafuv,'F_Leaf')
    singles=mesh_object(name+'_IndividualLeaves',singlev,singlef,singleuv,'F_LivingLeaf')
    bpy.ops.object.select_all(action='DESELECT');wood.select_set(True);leaves.select_set(True);singles.select_set(True);bpy.context.view_layer.objects.active=wood;bpy.ops.object.join()
    export(wood,name)
    ring=[v for v in barkv if v.z<.12];manifest[-1]['radius_cm']=max((math.hypot(v.x,v.y) for v in ring),default=.5)*100

open_file(Path('Pine/Kiefer_neu.blend'))
wood=bpy.data.objects['tree.001'];points=[wood.matrix_world@v.co for v in wood.data.vertices];minz=min(v.z for v in points);s=24/(max(v.z for v in points)-minz)
wood=clone_scaled(wood,'PineWood',24);wood.data.materials[0]=bpy.data.materials.new('F_Bark')
# Curve-free needle clusters sampled from the author's original canopy emitter.
emitter=bpy.data.objects['leafs.001'];vv=[];ff=[];uv=[]
for p in list(emitter.data.polygons)[::60]:
    pos=sum((emitter.matrix_world@emitter.data.vertices[i].co for i in p.vertices),Vector())/len(p.vertices)
    pos=Vector((pos.x*s,pos.y*s,(pos.z-minz)*s))
    for n in range(2):
        angle=rng.random()*math.tau;axis=Vector((math.cos(angle),math.sin(angle),rng.uniform(-.5,.5))).normalized();side=axis.cross(Vector((0,0,1))).normalized()
        start=len(vv);length=rng.uniform(.22,.42);width=length*.60
        for x,y in [(-1,-1),(1,-1),(1,1),(-1,1)]:vv.append(pos+axis*x*length*.5+side*y*width*.5)
        ff.append((start,start+1,start+2,start+3));uv.extend([(0,0),(1,0),(1,1),(0,1)])
leaf=mesh_object('PineNeedles',vv,ff,uv,'F_Needles')
bpy.ops.object.select_all(action='DESELECT');wood.select_set(True);leaf.select_set(True);bpy.context.view_layer.objects.active=wood;bpy.ops.object.join();export(wood,'SM_Pine')
ring=[v.co for v in wood.data.vertices if .2<v.co.z<.8];manifest[-1]['radius_cm']=max((math.hypot(v.x,v.y) for v in ring),default=.5)*100

# Fallen branches and small stones use authored geometry rather than engine spheres.
open_file(Path('GrassField/Realistic Grass Field.blend'))
stone=clone_scaled(bpy.data.objects['rock.002'],'SM_Stone',.40)
stone.data.materials[0]=bpy.data.materials.new('F_Stone')
sub=stone.modifiers.new('Worn edges','SUBSURF');sub.levels=2;bpy.context.view_layer.objects.active=stone;stone.select_set(True);bpy.ops.object.modifier_apply(modifier=sub.name);export(stone,'SM_Stone')
(out/'manifest.json').write_text(json.dumps(manifest,indent=2));print('FOREST_EXPORT_DONE '+json.dumps(manifest))
