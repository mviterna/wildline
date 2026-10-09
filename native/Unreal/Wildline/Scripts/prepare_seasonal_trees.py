"""Retain source bark/limbs, thin leaf cards, and export leafless winter trees.

The collision meshes contain wood only, so foliage never becomes a platform.
Original licensed forest FBX files remain unchanged.
"""
from pathlib import Path
import bpy, bmesh, json, random

root = Path(__file__).resolve().parent.parent / 'ArtSource/Forest'
source = root / 'Export'
out = root / 'Seasonal'
out.mkdir(exist_ok=True)
report = []

def export(obj, name):
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.context.scene.unit_settings.system = 'METRIC'
    bpy.context.scene.unit_settings.scale_length = 1
    bpy.ops.export_scene.fbx(filepath=str(out / (name + '.fbx')), use_selection=True,
        object_types={'MESH'}, mesh_smooth_type='FACE', bake_anim=False,
        axis_forward='-Y', axis_up='Z', apply_unit_scale=True, path_mode='STRIP')

for species in ['OakA', 'OakB', 'OakC', 'Pine']:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(source / ('SM_' + species + '.fbx')))
    original = next(o for o in bpy.context.selected_objects if o.type == 'MESH')
    original.name = 'Source_' + species
    leaves = {i for i, m in enumerate(original.data.materials) if 'Bark' not in m.name}
    for variant in ['Wood', 'Bare', 'Thin']:
        obj = original.copy()
        obj.data = original.data.copy()
        bpy.context.collection.objects.link(obj)
        obj.name = 'SM_' + species + '_' + variant
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bm.faces.ensure_lookup_table()
        discard = set()
        foliage_faces = {f for f in bm.faces if f.material_index in leaves}
        if variant in ['Wood', 'Bare']:
            discard = foliage_faces
        else:
            # Connected cards stay intact; never remove just half of a leaf quad.
            unseen = set(foliage_faces)
            rng = random.Random(8251 + len(species))
            while unseen:
                seed = min(unseen, key=lambda f: f.index)
                unseen.remove(seed)
                group = {seed}
                todo = [seed]
                while todo:
                    face = todo.pop()
                    for edge in face.edges:
                        for neighbor in edge.link_faces:
                            if neighbor in unseen:
                                unseen.remove(neighbor)
                                group.add(neighbor)
                                todo.append(neighbor)
                if rng.random() < .28:
                    discard.update(group)
        before = len(foliage_faces)
        bmesh.ops.delete(bm, geom=list(discard), context='FACES')
        bm.to_mesh(obj.data)
        bm.free()
        obj.data.update()
        export(obj, obj.name)
        report.append({'mesh': obj.name, 'source': original.name,
            'original_foliage_faces': before, 'removed_foliage_faces': len(discard),
            'faces': len(obj.data.polygons), 'materials': [m.name for m in obj.data.materials]})
        bpy.data.objects.remove(obj, do_unlink=True)
    print('SEASONAL TREE DONE ' + species, flush=True)
(out / 'manifest.json').write_text(json.dumps(report, indent=2))
print('SEASONAL TREES READY ' + json.dumps(report), flush=True)
