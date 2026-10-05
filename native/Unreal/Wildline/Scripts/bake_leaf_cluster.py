"""Render a twig atlas from CC0 photographic leaves for original tree cards."""
import bpy, random, math
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parent.parent/'ArtSource/Forest'
bpy.ops.wm.read_factory_settings(use_empty=True)
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=1
scene.render.resolution_x=2048;scene.render.resolution_y=2048;scene.render.resolution_percentage=100
scene.render.film_transparent=True;scene.view_settings.view_transform='Standard'
color=bpy.data.images.load(str(root/'Leaves/LeafSet001_2K-JPG_Color.jpg'))
alpha=bpy.data.images.load(str(root/'Leaves/LeafSet001_2K-JPG_Opacity.jpg'));alpha.colorspace_settings.name='Non-Color'
mat=bpy.data.materials.new('CC0 photographic leaf');mat.use_nodes=True
nodes=mat.node_tree.nodes;nodes.clear();tex=nodes.new('ShaderNodeTexImage');tex.image=color;mask=nodes.new('ShaderNodeTexImage');mask.image=alpha
em=nodes.new('ShaderNodeEmission');trans=nodes.new('ShaderNodeBsdfTransparent');mix=nodes.new('ShaderNodeMixShader');output=nodes.new('ShaderNodeOutputMaterial');links=mat.node_tree.links
links.new(tex.outputs['Color'],em.inputs['Color']);links.new(mask.outputs['Color'],mix.inputs[0]);links.new(trans.outputs[0],mix.inputs[1]);links.new(em.outputs[0],mix.inputs[2]);links.new(mix.outputs[0],output.inputs[0])
box=[(.05,.03,.245,.47),(.36,.10,.53,.39),(.685,.065,.90,.48),(.055,.55,.25,.95),(.37,.54,.55,.97),(.685,.55,.90,.96)]
r=random.Random(41);verts=[];faces=[];uv=[]
for j in range(65):
    # An irregular twig spray with a gap around its silhouette, rather than a square hedge.
    y=r.uniform(-.41,.41);x=r.choice([-1,1])*(.045+(.41-abs(y))*.55)*r.uniform(.3,1.2)
    angle=r.uniform(-1.3,1.3)+(-.55 if x<0 else .55);length=r.uniform(.13,.20);width=length*.51
    u=Vector((math.cos(angle),math.sin(angle),0));v=Vector((-math.sin(angle),math.cos(angle),0));pos=Vector((x,y,j*.0001));n=len(verts)
    for a,b in [(-.5,-.5),(.5,-.5),(.5,.5),(-.5,.5)]:verts.append(pos+u*a*width+v*b*length)
    faces.append((n,n+1,n+2,n+3));x0,y0,x1,y1=box[r.randrange(6)];uv.extend([(x0,1-y1),(x1,1-y1),(x1,1-y0),(x0,1-y0)])
mesh=bpy.data.meshes.new('twig');mesh.from_pydata(verts,[],faces);mesh.materials.append(mat);layer=mesh.uv_layers.new()
for p in mesh.polygons:
    for li,vi in zip(p.loop_indices,p.vertices):layer.data[li].uv=uv[vi]
obj=bpy.data.objects.new('twig',mesh);scene.collection.objects.link(obj)
bpy.ops.object.camera_add(location=(0,0,2));cam=bpy.context.object;cam.data.type='ORTHO';cam.data.ortho_scale=1;cam.rotation_euler=(0,0,0);scene.camera=cam
scene.render.image_settings.file_format='PNG';scene.render.image_settings.color_mode='RGBA';scene.render.filepath=str(root/'Export/LeafCluster.png');bpy.ops.render.render(write_still=True)
# A curved individual fallen leaf, photographed UVs retained.
mesh=bpy.data.meshes.new('fallen leaf');mesh.from_pydata([(-.035,-.06,0),(.035,-.06,.008),(.035,.06,.003),(-.035,.06,.006)],[],[(0,1,2,3)]);layer=mesh.uv_layers.new()
for li,v in enumerate([(.05,.53),(.245,.53),(.245,.97),(.05,.97)]):layer.data[li].uv=v
obj=bpy.data.objects.new('SM_FallenLeaf',mesh);scene.collection.objects.link(obj);mesh.materials.append(bpy.data.materials.new('F_FallenLeaf'))
bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
bpy.ops.export_scene.fbx(filepath=str(root/'Export/SM_FallenLeaf.fbx'),use_selection=True,object_types={'MESH'},bake_anim=False,axis_forward='-Y',axis_up='Z',path_mode='STRIP')
