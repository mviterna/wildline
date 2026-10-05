"""Supplement BlendSwap plants with CC0 photographed Poly Haven grass blades."""
import bpy, math
from pathlib import Path
root=Path(__file__).resolve().parent.parent/'ArtSource/Forest'
bpy.ops.wm.open_mainfile(filepath=str(root/'PhotoGrass/grass.blend'),use_scripts=False)
for path in (root/'PhotoGrass/textures').glob('*.exr'):
    image=bpy.data.images.load(str(path));image.colorspace_settings.name='Non-Color';pixel=image.pixels[0];image.file_format='PNG';image.filepath_raw=str(path.with_suffix('.png'));image.save()
for original,name,height in [('grass_medium_01_mid_b_LOD1','SM_GrassA',.23),('grass_medium_01_mid_c_LOD1','SM_GrassB',.30)]:
    src=bpy.data.objects[original];obj=bpy.data.objects.new(name,src.data.copy());bpy.context.collection.objects.link(obj)
    coords=[src.matrix_world.to_3x3()@v.co for v in obj.data.vertices];low=min(v.z for v in coords);scale=height/(max(v.z for v in coords)-low)
    for v,p in zip(obj.data.vertices,coords):v.co=(p.x*scale,p.y*scale,(p.z-low)*scale)
    obj.data.materials.clear();obj.data.materials.append(bpy.data.materials.get('F_PhotoGrass') or bpy.data.materials.new('F_PhotoGrass'))
    colors=obj.data.color_attributes.new(name='Growth',type='FLOAT_COLOR',domain='CORNER')
    for p in obj.data.polygons:
        p.material_index=0;p.use_smooth=True
        for li,vi in zip(p.loop_indices,p.vertices):colors.data[li].color=(max(0,min(1,obj.data.vertices[vi].co.z/height)),.5,1,1)
    bpy.ops.object.select_all(action='DESELECT');obj.hide_set(False);obj.hide_viewport=False;obj.select_set(True);bpy.context.view_layer.objects.active=obj
    bpy.ops.export_scene.fbx(filepath=str(root/'Export'/(name+'.fbx')),use_selection=True,object_types={'MESH'},use_mesh_modifiers=False,mesh_smooth_type='FACE',bake_anim=False,axis_forward='-Y',axis_up='Z',apply_unit_scale=True,path_mode='STRIP')
    print('PHOTO_GRASS_EXPORT',name,len(obj.data.polygons),height)
