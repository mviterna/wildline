"""Studio renders of the actual editable mesh (not a concept illustration)."""
import bpy, math, json, sys
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parent.parent/'ArtSource/Hunters'
name='SK_HunterMale' if '--baseline' in sys.argv else 'SK_HunterGreyson' if '--greyson' in sys.argv else 'SK_HunterEthan' if '--ethan' in sys.argv else 'SK_HunterTravis'
bpy.ops.wm.open_mainfile(filepath=str(root/(name+'.blend')),use_scripts=False)
obj=bpy.data.objects[name]
for o in list(bpy.data.objects):
    if o.type=='MESH' and o!=obj:o.hide_render=True
for mod in obj.modifiers:
    if mod.type=='ARMATURE':mod.show_render=False
# Strip inherited authoring scene settings and retain just the exported mesh.
scene=bpy.data.scenes.new('TravisStudio');scene.collection.objects.link(obj)
bpy.context.window.scene=scene
scene.world=bpy.data.worlds.new('TravisStudioWorld')
scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs['Color'].default_value=(.08,.08,.08,1)
scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.6
print('HEAD_EXTENTS')
for i,m in enumerate(obj.data.materials):
    vs={v for p in obj.data.polygons if p.material_index==i for v in p.vertices if obj.data.vertices[v].co.z>1.48}
    if vs:print(m.name,len(vs),[(min(obj.data.vertices[v].co[a] for v in vs),max(obj.data.vertices[v].co[a] for v in vs)) for a in range(3)])
tex={'H_Skin':'young_lightskinned_male_diffuse.png','H_Eyes':'brown_eye.png','H_Brows':'eyebrow001.png','H_Hair':'short01_diffuse.png','H_Cloth':'male_casualsuit05_diffuse.png','H_Boots':'shoes04_diffuse.png'}
for mat in obj.data.materials:
    if mat.get('travis_authored'):continue
    mat.use_nodes=True;nodes=mat.node_tree.nodes;nodes.clear();links=mat.node_tree.links
    out=nodes.new('ShaderNodeOutputMaterial');bs=nodes.new('ShaderNodeBsdfPrincipled');links.new(bs.outputs['BSDF'],out.inputs['Surface']);bs.inputs['Roughness'].default_value=.58
    if mat.name in tex:
        t=nodes.new('ShaderNodeTexImage');t.image=bpy.data.images.load(str(root/'Textures'/tex[mat.name]),check_existing=True)
        uv=nodes.new('ShaderNodeUVMap');uv.uv_map='UVMap';links.new(uv.outputs['UV'],t.inputs['Vector']);links.new(t.outputs['Color'],bs.inputs['Base Color'])
        if mat.name in ['H_Hair','H_Brows']:links.new(t.outputs['Alpha'],bs.inputs['Alpha'])
    else:bs.inputs['Alpha'].default_value=0
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=32
scene.render.resolution_x=800;scene.render.resolution_y=900;scene.render.resolution_percentage=100
scene.world.color=(.16,.16,.16)
scene.view_settings.view_transform='AgX'
def aim(o,p):o.rotation_euler=(Vector(p)-o.location).to_track_quat('-Z','Y').to_euler()
for loc,power,size in [((-.35,-.5,2.15),30,.45),((.45,-.25,1.9),14,.4),((.2,.35,2.0),22,.3)]:
    bpy.ops.object.light_add(type='AREA',location=loc);o=bpy.context.object;o.data.energy=power;o.data.shape='DISK';o.data.size=size;aim(o,(0,-.02,1.63))
bpy.ops.object.camera_add(location=(0,-.8,1.64));cam=bpy.context.object;scene.camera=cam;cam.data.type='ORTHO';cam.data.ortho_scale=.36
output=root.parents[3]/'Preview';output.mkdir(exist_ok=True)
for angle,label in [(0,'Front'),(40,'ThreeQuarter'),(90,'Profile')]:
    a=math.radians(angle);cam.location=(math.sin(a)*.8,-math.cos(a)*.8,1.64);aim(cam,(0,-.015,1.625))
    scene.render.filepath=str(output/(('TravisBaseline' if name.endswith('Male') else name.removeprefix('SK_Hunter')+'Photo')+label+'.png'));bpy.ops.render.render(write_still=True)
