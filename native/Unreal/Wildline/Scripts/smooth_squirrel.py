"""Run with portable Blender -b 'Rigged Squirrel.blend' --python this script.

Preserves the publisher's original source; exports a smoothed game FBX alongside it.
"""
import bpy
from pathlib import Path
root=Path(__file__).resolve().parent.parent
source=root/'ArtSource/Squirrel'
if bpy.context.object and bpy.context.object.mode!='OBJECT':bpy.ops.object.mode_set(mode='OBJECT')
for image in bpy.data.images:
    if image.name.startswith('Squirrel'):
        filename='Squirrel Eye.png' if 'Eye' in image.name else 'Squirrel.png'
        image.filepath=str(source/filename)
bpy.ops.object.select_all(action='DESELECT')
for obj in bpy.context.scene.objects:
    if obj.type=='ARMATURE':
        obj.data.pose_position='REST'
        obj.select_set(True)
        for pose in obj.pose.bones:
            pose.location=(0,0,0);pose.rotation_quaternion=(1,0,0,0);pose.scale=(1,1,1)
    if obj.type=='MESH':
        obj.select_set(True);bpy.context.view_layer.objects.active=obj
        if obj.data.has_custom_normals:bpy.ops.mesh.customdata_custom_splitnormals_clear()
        for polygon in obj.data.polygons:polygon.use_smooth=True
        # Subdivision adds rounded anatomical contours while preserving skin weights/UVs.
        mod=obj.modifiers.new('Wildline rounded anatomy','SUBSURF');mod.levels=2;mod.render_levels=2
        # Apply before exporting so FBX retains the armature and the actual dense skin.
        bpy.ops.object.modifier_apply(modifier=mod.name)
        if obj.data.has_custom_normals:bpy.ops.mesh.customdata_custom_splitnormals_clear()
        print('WILDLINE SMOOTH',obj.name,len(obj.data.vertices),len(obj.data.polygons))
bpy.ops.export_scene.fbx(filepath=str(source/'Squirrel Smooth.fbx'),use_selection=True,object_types={'MESH','ARMATURE'},use_mesh_modifiers=True,mesh_smooth_type='FACE',add_leaf_bones=False,bake_anim=False,axis_forward='-Z',axis_up='Y',apply_unit_scale=True,path_mode='COPY',embed_textures=True)
print('WILDLINE SMOOTH FBX EXPORTED',bpy.context.scene.unit_settings.scale_length)
