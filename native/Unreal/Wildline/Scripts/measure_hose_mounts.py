"""Blender: find wall contacts in exported house triangles, in local metres."""
from pathlib import Path
import json
import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree

root = Path(__file__).resolve().parents[1] / 'ArtSource/Neighborhood'
mounts = {}
for name in ['SM_ColonialHouse', 'SM_CapeCodHouse', 'SM_KitGardenHouse', 'SM_KitTwoStoryHouse', 'SM_KitCottage']:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(root / 'Export' / (name + '.fbx')))
    vertices, faces = [], []
    for obj in bpy.context.scene.objects:
        if obj.type != 'MESH':
            continue
        offset = len(vertices)
        vertices.extend(obj.matrix_world @ v.co for v in obj.data.vertices)
        faces.extend([offset + i for i in face.vertices] for face in obj.data.polygons)
    bvh = BVHTree.FromPolygons(vertices, faces)
    contacts = {}
    candidates = []
    for index in range(-30, 31):
        x = index / 10.
        # FBX import maps Blender +Y to Unreal -Y. Both rows mount on this
        # rear wall, with the street-facing row rotated 180 degrees in Unreal.
        point, normal, _, _ = bvh.ray_cast(Vector((x, 12., .9285)), Vector((0, -1, 0)), 12.)
        if point is not None and normal.y > .85:
            candidates.append((point, normal))
    assert candidates, name
    # Choose the outer facade, rather than an interior partition visible through
    # an open door or window in the modular kit.
    point, normal = max(candidates, key=lambda contact: (round(contact[0].y, 2), -abs(contact[0].x - 2)))
    contacts['rear'] = {'point_cm': [point.x * 100, -point.y * 100, point.z * 100], 'normal': [normal.x, -normal.y, normal.z]}
    print('HOSE WALL', name, tuple(point), tuple(normal), flush=True)
    mounts[name] = contacts
(root / 'hose-mounts.json').write_text(json.dumps(mounts, indent=2))
