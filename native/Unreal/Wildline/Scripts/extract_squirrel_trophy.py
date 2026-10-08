"""Derive a mounted head from the already licensed squirrel, preserving UVs.
Run Blender -b 'ArtSource/Squirrel/Rigged Squirrel.blend' --python this script.
"""
from pathlib import Path
import bpy
import bmesh
from mathutils import Vector

out=Path(__file__).resolve().parent.parent/'ArtSource/Lobby'
out.mkdir(parents=True,exist_ok=True)
if bpy.context.object and bpy.context.object.mode!='OBJECT':bpy.ops.object.mode_set(mode='OBJECT')
for img in bpy.data.images:
    if img.name.startswith('Squirrel'):
        img.filepath=str(out.parent/'Squirrel'/('Squirrel Eye.png' if 'Eye' in img.name else 'Squirrel.png'))
heads=[]
for obj in list(bpy.context.scene.objects):
    if obj.type!='MESH':continue
    print('TROPHY SOURCE',obj.name,'groups',[(g.name,g.index) for g in obj.vertex_groups])
    groups={g.index for g in obj.vertex_groups if any(k in g.name.lower() for k in ('head','neck','eye','jaw')) or '_ear' in g.name.lower()}
    if not groups:continue
    copy=obj.copy();copy.data=obj.data.copy();bpy.context.collection.objects.link(copy)
    copy.parent=None
    for mod in list(copy.modifiers):copy.modifiers.remove(mod)
    keep={v.index for v in copy.data.vertices if sum(g.weight for g in v.groups if g.group in groups)>.5}
    bm=bmesh.new();bm.from_mesh(copy.data);bm.verts.ensure_lookup_table()
    bmesh.ops.delete(bm,geom=[v for v in bm.verts if v.index not in keep],context='VERTS')
    bm.to_mesh(copy.data);bm.free()
    if len(copy.data.vertices)==0:bpy.data.objects.remove(copy,do_unlink=True);continue
    heads.append(copy)
assert heads,'No weighted squirrel head found'
# Include eye meshes which may have rigid parenting rather than skin groups.
for obj in list(bpy.context.scene.objects):
    if obj.type=='MESH' and 'eye' in obj.name.lower() and obj not in heads:
        copy=obj.copy();copy.data=obj.data.copy();bpy.context.collection.objects.link(copy);copy.parent=None
        for mod in list(copy.modifiers):copy.modifiers.remove(mod)
        heads.append(copy)
bpy.ops.object.select_all(action='DESELECT')
for obj in heads:obj.select_set(True)
bpy.context.view_layer.objects.active=heads[0]
bpy.ops.object.join()
head=bpy.context.object
points=[v.co for v in head.data.vertices]
centre=(Vector(tuple(min(p[i] for p in points) for i in range(3)))+Vector(tuple(max(p[i] for p in points) for i in range(3))))*.5
for v in head.data.vertices:v.co-=centre
head.location=(0,0,0)
for face in head.data.polygons:face.use_smooth=True
sub=head.modifiers.new('Rounded trophy anatomy','SUBSURF');sub.levels=1
bpy.ops.object.modifier_apply(modifier=sub.name)
bpy.ops.export_scene.fbx(filepath=str(out/'SquirrelTrophy.fbx'),use_selection=True,object_types={'MESH'},mesh_smooth_type='FACE',bake_anim=False,axis_forward='-Z',axis_up='Y',apply_unit_scale=True)
print('TROPHY EXPORTED',len(head.data.vertices),str(out/'SquirrelTrophy.fbx'))
