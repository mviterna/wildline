"""Derive a mounted head from the already licensed squirrel, preserving UVs.
Run Blender -b 'ArtSource/Squirrel/Rigged Squirrel.blend' --python this script.
"""
from pathlib import Path
import bpy
import bmesh
import json
from mathutils import Vector

out=Path(__file__).resolve().parent.parent/'ArtSource/Lobby'
out.mkdir(parents=True,exist_ok=True)
if bpy.context.object and bpy.context.object.mode!='OBJECT':bpy.ops.object.mode_set(mode='OBJECT')
for img in bpy.data.images:
    if img.name.startswith('Squirrel'):
        img.filepath=str(out.parent/'Squirrel'/('Squirrel Eye.png' if 'Eye' in img.name else 'Squirrel.png'))
heads=[]
sources=[obj for obj in bpy.context.scene.objects if obj.type=='MESH']
for obj in sources:
    if obj.type!='MESH':continue
    print('TROPHY SOURCE',obj.name,'groups',[(g.name,g.index) for g in obj.vertex_groups])
    # FACIAL_Nose is separately weighted: omitting it cuts a hole in the snout.
    groups={g.index for g in obj.vertex_groups if any(k in g.name.lower() for k in ('head','neck','eye','jaw','nose','_ear'))}
    if not groups:continue
    copy=obj.copy();copy.data=obj.data.copy();bpy.context.collection.objects.link(copy)
    world=obj.matrix_world.copy();copy.parent=None;copy.matrix_world=world
    for mod in list(copy.modifiers):copy.modifiers.remove(mod)
    keep={v.index for v in copy.data.vertices if sum(g.weight for g in v.groups if g.group in groups)>.5}
    bm=bmesh.new();bm.from_mesh(copy.data);bm.verts.ensure_lookup_table()
    bmesh.ops.delete(bm,geom=[v for v in bm.verts if v.index not in keep],context='VERTS')
    bm.to_mesh(copy.data);bm.free()
    if len(copy.data.vertices)==0:bpy.data.objects.remove(copy,do_unlink=True);continue
    nose_group=next((g.index for g in copy.vertex_groups if 'nose' in g.name.lower()),None)
    if nose_group is not None:
        copy.data.materials.append(bpy.data.materials.get('TrophyNose') or bpy.data.materials.new('TrophyNose'))
        nose_slot=len(copy.data.materials)-1
        for face in copy.data.polygons:
            weights=[sum(g.weight for g in copy.data.vertices[vi].groups if g.group==nose_group) for vi in face.vertices]
            if sum(weights)/len(weights)>.65:face.material_index=nose_slot
    heads.append(copy)
assert heads,'No weighted squirrel head found'
# Include only original rigid eye meshes; never duplicate already extracted eyes.
for obj in sources:
    if 'eye' in obj.name.lower() and not obj.vertex_groups:
        copy=obj.copy();copy.data=obj.data.copy();bpy.context.collection.objects.link(copy);copy.parent=None
        for mod in list(copy.modifiers):copy.modifiers.remove(mod)
        heads.append(copy)
bpy.ops.object.select_all(action='DESELECT')
for obj in heads:obj.select_set(True)
bpy.context.view_layer.objects.active=heads[0]
bpy.ops.object.join()
head=bpy.context.object
bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
points=[v.co for v in head.data.vertices]
centre=(Vector(tuple(min(p[i] for p in points) for i in range(3)))+Vector(tuple(max(p[i] for p in points) for i in range(3))))*.5
for v in head.data.vertices:v.co-=centre
head.location=(0,0,0)
for face in head.data.polygons:face.use_smooth=True
sub=head.modifiers.new('Rounded trophy anatomy','SUBSURF');sub.levels=1
bpy.ops.object.modifier_apply(modifier=sub.name)
report=[]
for name,proportions in [('SquirrelTrophy',(1,1,1)),('SquirrelTrophyFox',(1.15,1.10,1.04)),('SquirrelTrophyRed',(.86,.92,.95)),('SquirrelTrophyBlack',(.96,1.02,1.05))]:
    bpy.ops.object.select_all(action='DESELECT')
    variant=head.copy();variant.data=head.data.copy();bpy.context.collection.objects.link(variant)
    variant.name=name;variant.select_set(True);bpy.context.view_layer.objects.active=variant
    for v in variant.data.vertices:
        for i in range(3):v.co[i]*=proportions[i]
    bpy.ops.export_scene.fbx(filepath=str(out/(name+'.fbx')),use_selection=True,object_types={'MESH'},mesh_smooth_type='FACE',bake_anim=False,axis_forward='-Z',axis_up='Y',apply_unit_scale=True)
    report.append({'name':name,'vertices':len(variant.data.vertices),'proportions':proportions,'nose_included':True})
    print('TROPHY EXPORTED',report[-1],flush=True)
(out/'trophy-variants.json').write_text(json.dumps(report,indent=2))
