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

exec(compile((Path(__file__).parent/'build_photo_hunters.py').read_text(), 'build_photo_hunters.py', 'exec'))
