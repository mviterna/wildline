"""Render original hero meshes for visual QA, without altering their sources."""
import bpy, sys, math
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parent.parent/'ArtSource/Neighborhood'
OUT=ROOT.parents[3]/'Preview';OUT.mkdir(exist_ok=True)
for name,position,target,scale in [
    ('SM_CedarBirdFeeder',(3.8,-5,3.9),(0,0,1.50),4.6),
    ('SM_ColonialHouse',(16,-21,12),(0,-.5,3.3),18.5),
    ('SM_BackyardTrampoline',(3.5,-4,3.5),(0,0,.4),3.7),
    ('SM_KitTwoStoryHouse',(12,-15,9),(0,0,2.3),12),
]:
    bpy.ops.wm.open_mainfile(filepath=str(ROOT/(name+'.blend')))
    for o in bpy.context.scene.objects:o.hide_render=not o.select_get()
    scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=32;scene.render.resolution_x=1400;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
    scene.world=bpy.data.worlds.new('Outdoor preview');scene.world.use_nodes=True;scene.world.node_tree.nodes.get('Background').inputs['Color'].default_value=(.36,.43,.52,1);scene.world.node_tree.nodes.get('Background').inputs['Strength'].default_value=.65
    bpy.ops.mesh.primitive_plane_add(size=200);o=bpy.context.object;o.location.z=-.015;m=bpy.data.materials.new('Preview ground');m.diffuse_color=(.14,.19,.11,1);o.data.materials.append(m)
    bpy.ops.object.camera_add(location=position);c=bpy.context.object;c.rotation_euler=(Vector(target)-c.location).to_track_quat('-Z','Y').to_euler();c.data.type='ORTHO';c.data.ortho_scale=scale;scene.camera=c
    bpy.ops.object.light_add(type='SUN',location=(5,-3,12));l=bpy.context.object;l.rotation_euler=(math.radians(28),math.radians(-25),math.radians(-28));l.data.energy=2.3;l.data.angle=math.radians(8)
    bpy.ops.object.light_add(type='AREA',location=(-3,-4,7));l=bpy.context.object;l.rotation_euler=(Vector(target)-l.location).to_track_quat('-Z','Y').to_euler();l.data.energy=500;l.data.size=6
    scene.render.filepath=str(OUT/(name+'_Review.png'));bpy.ops.render.render(write_still=True)
