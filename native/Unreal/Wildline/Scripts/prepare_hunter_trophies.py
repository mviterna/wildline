"""Static trophy busts retain the existing selectable hunters' face UVs."""
from pathlib import Path
import bpy,bmesh
root=Path(__file__).resolve().parent.parent/'ArtSource/Hunters'
for person in ['Travis','Ethan','Greyson']:
    bpy.ops.wm.open_mainfile(filepath=str(root/f'SK_Hunter{person}.blend'),use_scripts=False)
    obj=bpy.data.objects[f'SK_Hunter{person}']
    # Bake the unposed basis, then crop below the neck and centre the bust.
    if obj.data.shape_keys:
        obj.shape_key_clear()
    obj.modifiers.clear();obj.parent=None
    bm=bmesh.new();bm.from_mesh(obj.data)
    bmesh.ops.delete(bm,geom=[v for v in bm.verts if v.co.z<1.48],context='VERTS')
    for v in bm.verts:v.co.z-=1.48
    bm.to_mesh(obj.data);bm.free();obj.name=f'SM_HunterTrophy{person}'
    bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
    bpy.ops.export_scene.fbx(filepath=str(root/f'Export/{obj.name}.fbx'),use_selection=True,object_types={'MESH'},bake_anim=False,axis_forward='-Y',axis_up='Z',path_mode='STRIP')
    print('HUNTER TROPHY EXPORTED '+person,flush=True)
