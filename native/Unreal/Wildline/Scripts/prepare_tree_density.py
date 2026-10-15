"""Assign stable ranks to whole leaf cards, preserving source wind weights."""
from pathlib import Path
import bpy, bmesh, json, random
root = Path(__file__).resolve().parent.parent / 'ArtSource/Forest'
out = root / 'Density'
out.mkdir(exist_ok=True)
report = []
for species in ['OakA', 'OakB', 'OakC', 'Pine']:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(root / 'Export' / ('SM_' + species + '.fbx')))
    obj = next(o for o in bpy.context.selected_objects if o.type == 'MESH')
    obj.name = 'SM_' + species + '_Density'
    bm = bmesh.new(); bm.from_mesh(obj.data); bm.faces.ensure_lookup_table()
    leaf_slots = {i for i, m in enumerate(obj.data.materials) if 'Bark' not in m.name}
    unseen = {f for f in bm.faces if f.material_index in leaf_slots}
    groups = []
    while unseen:
        seed = min(unseen, key=lambda f: f.index); unseen.remove(seed)
        group = {seed}; todo = [seed]
        while todo:
            for edge in todo.pop().edges:
                for other in edge.link_faces:
                    if other in unseen:
                        unseen.remove(other); group.add(other); todo.append(other)
        groups.append(group)
    random.Random(9241 + len(species)).shuffle(groups)
    source_colors = bm.loops.layers.float_color.active or bm.loops.layers.color.active
    colors = bm.loops.layers.float_color.new('LeafCardRank')
    for face in bm.faces:
        for loop in face.loops:
            loop[colors] = tuple(loop[source_colors]) if source_colors else (1,1,1,1)
    for i, group in enumerate(groups):
        rank = (i + .5) / len(groups)
        for face in group:
            for loop in face.loops:
                old = loop[colors]
                loop[colors] = (old[0], rank, old[2], 1)
    bm.to_mesh(obj.data); bm.free(); obj.data.update()
    # FBX exports the active render layer first.
    obj.data.color_attributes.active_color = obj.data.color_attributes['LeafCardRank']
    obj.data.color_attributes.render_color_index = list(obj.data.color_attributes.keys()).index('LeafCardRank')
    bpy.ops.object.select_all(action='DESELECT'); obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.context.scene.unit_settings.system = 'METRIC'; bpy.context.scene.unit_settings.scale_length = 1
    bpy.ops.export_scene.fbx(filepath=str(out / (obj.name + '.fbx')), use_selection=True,
        object_types={'MESH'}, mesh_smooth_type='FACE', bake_anim=False,
        axis_forward='-Y', axis_up='Z', apply_unit_scale=True, path_mode='STRIP', colors_type='LINEAR', prioritize_active_color=True)
    report.append({'mesh': obj.name, 'leaf_cards': len(groups), 'faces': len(obj.data.polygons),
                   'visible_cards': {str(p): sum((i+.5)/len(groups) < p/100 for i in range(len(groups))) for p in [0, 1, 25, 50, 72, 75, 99, 100]}})
    print('TREE DENSITY READY ' + json.dumps(report[-1]), flush=True)
(out / 'manifest.json').write_text(json.dumps(report, indent=2))
