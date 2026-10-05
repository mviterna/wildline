"""Author Travis and editable body morphs on the game's existing hunter rig."""
import bpy, math, json
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parent.parent/'ArtSource/Hunters'

def slim(co):
    c=co.copy();x,y,z=c
    if .22<z<1.50:
        # Shrink around limb centers, preserving the animation skeleton.
        if z<.98:
            anchor=math.copysign(.12,x);c.x=anchor+(x-anchor)*.78
        elif abs(x)>.22:
            c.y=y*.82
        else:c.x=x*.84
        c.y=y*.82
    return c

def morphs(obj):
    if not obj.data.shape_keys:obj.shape_key_add(name='Basis')
    k=obj.data.shape_keys.key_blocks.get('SlimBuild') or obj.shape_key_add(name='SlimBuild')
    for i,v in enumerate(obj.data.vertices):k.data[i].co=slim(v.co)

def export(obj,name):
    obj.data.uv_layers.active_index=0;obj.data.uv_layers[0].active_render=True
    bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);obj.parent.select_set(True);bpy.context.view_layer.objects.active=obj.parent
    bpy.ops.export_scene.fbx(filepath=str(root/'Export'/(name+'.fbx')),use_selection=True,object_types={'MESH','ARMATURE'},add_leaf_bones=False,bake_anim=False,axis_forward='-Y',axis_up='Z',path_mode='STRIP')
    bpy.ops.wm.save_as_mainfile(filepath=str(root/(name+'.blend')))

for name in ['SK_HunterMale','SK_HunterFemale']:
    bpy.ops.wm.open_mainfile(filepath=str(root/(name+'.blend')),use_scripts=False)
    obj=bpy.data.objects[name];morphs(obj);export(obj,name)

bpy.ops.wm.open_mainfile(filepath=str(root/'SK_HunterMale.blend'),use_scripts=False)
obj=bpy.data.objects['SK_HunterMale'];obj.name='SK_HunterTravis'
hair=obj.data.materials.find('H_Hair');skin=obj.data.materials.find('H_Skin');cloth=obj.data.materials.find('H_Cloth')
# Receding hairline: retain short side/back hair and remove the forward crown.
remove=[]
for p in obj.data.polygons:
    c=sum((obj.data.vertices[i].co for i in p.vertices),Vector())/len(p.vertices)
    if p.material_index==hair and (c.y<-.055 and c.z>1.65 or abs(c.x)<.042 and c.z>1.705):remove.append(p.index)
# Use a mesh copy to remove polygons without changing retained shape-key indices.
import bmesh
bm=bmesh.new();bm.from_mesh(obj.data);bm.faces.ensure_lookup_table()
bmesh.ops.delete(bm,geom=[bm.faces[i] for i in remove],context='FACES_ONLY');bm.to_mesh(obj.data);bm.free()

def shell(name,slot,predicate,offset):
    selected=[p for p in obj.data.polygons if predicate(p)]
    ids=sorted({i for p in selected for i in p.vertices});mapping={v:i for i,v in enumerate(ids)}
    me=bpy.data.meshes.new(name);me.from_pydata([obj.data.vertices[i].co+obj.data.vertices[i].normal*offset for i in ids],[],[[mapping[i] for i in p.vertices] for p in selected]);me.update()
    part=bpy.data.objects.new(name,me);bpy.context.collection.objects.link(part);me.materials.append(bpy.data.materials.get(slot) or bpy.data.materials.new(slot))
    for g in obj.vertex_groups:part.vertex_groups.new(name=g.name)
    for vi,i in enumerate(ids):
        for g in obj.data.vertices[i].groups:part.vertex_groups[g.group].add([vi],g.weight,'REPLACE')
    part.parent=obj.parent;part.matrix_parent_inverse=obj.matrix_parent_inverse.copy()
    for p in me.polygons:p.use_smooth=True
    part.shape_key_add(name='Basis');morphs(part)
    for key in ['FaceWidth','NoseDepth']:
        k=part.shape_key_add(name=key)
        src=obj.data.shape_keys.key_blocks[key]
        for vi,i in enumerate(ids):k.data[vi].co+=src.data[i].co-obj.data.vertices[i].co
    return part

def center(p):return sum((obj.data.vertices[i].co for i in p.vertices),Vector())/len(p.vertices)
def beard(p):
    x,y,z=center(p)
    return p.material_index==skin and y<-.08 and ((abs(x)<.036 and 1.512<z<1.573) or (.026<abs(x)<.053 and 1.535<z<1.596) or (abs(x)<.042 and 1.599<z<1.614))
def vest(p):
    x,y,z=center(p)
    return p.material_index==cloth and 1.13<z<1.46 and abs(x)<.185 and not (y<-.03 and abs(x)<.014)
parts=[shell('Travis goatee','H_Beard',beard,.0008),shell('Travis hunting vest','H_Vest',vest,.005)]
# Calibrated second UV channel for the bundled facial projection.
uv=obj.data.uv_layers.get('PhotoFace');mask=obj.data.color_attributes.get('PhotoMask')
for p in obj.data.polygons:
    for li,vi in zip(p.loop_indices,p.vertices):
        v=obj.data.vertices[vi];x,y,z=v.co
        uv.data[li].uv=(.5+x/.185,(z-1.505)/.235)
        a=max(0,min(1,(-v.normal.y-.08)*1.6))*max(0,min(1,(z-1.51)*65))*max(0,min(1,(.086-abs(x))*85)) if p.material_index==skin else 0
        garment=1 if p.material_index==cloth and 1.115<z<1.46 and abs(x)<.21 else 0
        mask.data[li].color=(a,garment,0,1)
bpy.ops.object.select_all(action='DESELECT');obj.select_set(True)
for p in parts:p.select_set(True)
bpy.context.view_layer.objects.active=obj;bpy.ops.object.join()
obj['preset']='Travis';obj['identity']='User supplied likeness; AI assisted facial projection, hand shaped game mesh'
export(obj,'SK_HunterTravis')
report={'mesh':'SK_HunterTravis','vertices':len(obj.data.vertices),'polygons':len(obj.data.polygons),'morphs':[k.name for k in obj.data.shape_keys.key_blocks],'slots':[m.name for m in obj.data.materials]}
(root/'travis-model.json').write_text(json.dumps(report,indent=2));print('TRAVIS_EXPORTED',json.dumps(report))
