"""Adapt the user-supplied CC BY Sketchfab downloads, retaining their UVs."""
import bpy, bmesh, math, json, xml.etree.ElementTree as ET
from pathlib import Path
from mathutils import Matrix, Vector
root=Path(__file__).resolve().parent.parent/'ArtSource/Farm'
src=root/'Sketchfab';out=root/'Export'
N={'c':'http://www.collada.org/2005/11/COLLADASchema'}
def bounds(o):
    return [[round(min(v.co[j] for v in o.data.vertices),4),round(max(v.co[j] for v in o.data.vertices),4)] for j in range(3)]
def dae():
    t=ET.parse(src/'Farmhouse/source/model/model/model.dae')
    geoms={g.get('id'):g for g in t.findall('.//c:geometry',N)}
    def visit(n,parent):
        e=n.find('c:matrix',N);m=Matrix.Identity(4)
        if e is not None:
            f=list(map(float,e.text.split()));m=Matrix([f[i:i+4] for i in range(0,16,4)])
        m=parent@m
        for instance in n.findall('c:instance_geometry',N):
            g=geoms[instance.get('url')[1:]];mesh=g.find('c:mesh',N);sources={}
            for s in mesh.findall('c:source',N):
                a=s.find('c:float_array',N);stride=int(s.find('c:technique_common/c:accessor',N).get('stride'))
                f=list(map(float,a.text.split()));sources[s.get('id')]=[f[i:i+stride] for i in range(0,len(f),stride)]
            p=sources[g.get('id')+'-positions'];uv=sources[g.get('id')+'-tex0'];faces=[]
            for tris in mesh.findall('c:triangles',N)+mesh.findall('c:polylist',N):
                assert all(i.get('offset')=='0' for i in tris.findall('c:input',N))
                ids=list(map(int,tris.find('c:p',N).text.split()))
                counts=tris.find('c:vcount',N)
                if counts is None:faces.extend([ids[i:i+3] for i in range(0,len(ids),3)])
                else:
                    at=0
                    for count in map(int,counts.text.split()):faces.append(ids[at:at+count]);at+=count
            v=[m@Vector(x) for x in p];v=[(x.x,-x.z,x.y) for x in v]
            data=bpy.data.meshes.new(g.get('id'));data.from_pydata(v,[],faces);data.update()
            o=bpy.data.objects.new(n.get('name'),data);bpy.context.collection.objects.link(o)
            layer=data.uv_layers.new(name='SourceUV')
            for loop in data.loops:layer.data[loop.index].uv=uv[loop.vertex_index]
            target=instance.find('.//c:instance_material',N).get('target')[1:]
            mat=bpy.data.materials.get(target) or bpy.data.materials.new(target);data.materials.append(mat)
        for child in n.findall('c:node',N):visit(child,m)
    for n in t.findall('c:library_visual_scenes/c:visual_scene/c:node',N):visit(n,Matrix.Identity(4))
def material_preview(mat, file):
    mat.use_nodes=True;n=mat.node_tree.nodes.new('ShaderNodeTexImage');n.image=bpy.data.images.load(str(file));mat.node_tree.links.new(n.outputs['Color'],mat.node_tree.nodes.get('Principled BSDF').inputs['Base Color'])
def review(file,p,target,ortho):
    scene=bpy.context.scene;scene.render.engine='BLENDER_EEVEE';scene.render.resolution_x=1200;scene.render.resolution_y=900;scene.render.resolution_percentage=100
    scene.world=bpy.data.worlds.new('Review sky');scene.world.color=(.24,.24,.24)
    bpy.ops.object.camera_add(location=p);c=bpy.context.object;c.rotation_euler=(Vector(target)-c.location).to_track_quat('-Z','Y').to_euler();c.data.type='ORTHO';c.data.ortho_scale=ortho;scene.camera=c
    bpy.ops.object.light_add(type='AREA',location=p);l=bpy.context.object;l.rotation_euler=c.rotation_euler;l.data.energy=1800;l.data.shape='DISK';l.data.size=10
    scene.render.filepath=str(src/file);bpy.ops.render.render(write_still=True)
def inspect_house():
    bpy.ops.wm.read_factory_settings(use_empty=True);dae()
    meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
    report={o.name:{'bounds':bounds(o),'material':o.data.materials[0].name,'faces':len(o.data.polygons)} for o in meshes}
    (src/'house_bounds.json').write_text(json.dumps(report,indent=2))
    print('HOUSE_TOTAL',[[min(v.co[j] for o in meshes for v in o.data.vertices),max(v.co[j] for o in meshes for v in o.data.vertices)] for j in range(3)])
    material_preview(bpy.data.materials['house_1'],src/'Farmhouse/textures/house_1_albedo.jpeg');material_preview(bpy.data.materials['House_2'],src/'Farmhouse/textures/House_2_albedo.jpeg')
    for o in meshes:
        for v in o.data.vertices:v.co*=.01
    bpy.ops.wm.save_as_mainfile(filepath=str(src/'Farmhouse/source/inspection.blend'))
    review('house_source.png',(24,-26,16),(-3,0,3),28)
def inspect_gun():
    bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(src/'Shotgun/source/shotgun.fbx'))
    o=next(o for o in bpy.context.scene.objects if o.type=='MESH')
    for v in o.data.vertices:
        p=o.matrix_world@v.co;v.co=(-p.y,p.x,p.z)
    o.matrix_world=Matrix.Identity(4)
    bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.00001);bm.to_mesh(o.data);bm.free()
    # Connected islands identify the stock, receiver, barrel and trigger without UV edits.
    bm=bmesh.new();bm.from_mesh(o.data);seen=set();parts=[]
    for v in bm.verts:
        if v in seen:continue
        todo=[v];island=[];seen.add(v)
        while todo:
            q=todo.pop();island.append(q)
            for e in q.link_edges:
                r=e.other_vert(q)
                if r not in seen:seen.add(r);todo.append(r)
        parts.append({'count':len(island),'bounds':[[round(min(v.co[j] for v in island),4),round(max(v.co[j] for v in island),4)] for j in range(3)]})
    bm.free();print('GUN_ISLANDS',json.dumps(parts))
    material_preview(o.data.materials[0],src/'Shotgun/textures/shotgun_l_MainMat.006_BaseColor.png')
    bpy.ops.wm.save_as_mainfile(filepath=str(src/'Shotgun/source/inspection.blend'))
    review('gun_source.png',(0,-16,5),(2,0,.5),12)
def export_mesh(objects,name):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objects:o.select_set(True)
    bpy.context.view_layer.objects.active=objects[0];bpy.ops.object.join();o=bpy.context.object;o.name=name
    bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.export_scene.fbx(filepath=str(out/(name+'.fbx')),use_selection=True,object_types={'MESH'},bake_anim=False,axis_forward='-Y',axis_up='Z',path_mode='STRIP')
def clip_opening(o,lo,hi):
    """Subtract a doorway volume from surface polygons, interpolating the original UVs."""
    def split(poly,axis,value,sign):
        inside=[];outside=[]
        for i,a in enumerate(poly):
            b=poly[(i+1)%len(poly)];da=(a[0][axis]-value)*sign;db=(b[0][axis]-value)*sign
            (inside if da>=0 else outside).append(a)
            if (da>=0)!=(db>=0):
                t=da/(da-db);q=(a[0].lerp(b[0],t),a[1].lerp(b[1],t));inside.append(q);outside.append(q)
        return inside,outside
    verts=[];faces=[];uvs=[]
    data=o.data;layer=data.uv_layers.active
    for p in data.polygons:
        pending=[(data.vertices[data.loops[i].vertex_index].co.copy(),layer.data[i].uv.copy()) for i in p.loop_indices]
        if any(max(a[0][j] for a in pending)<lo[j] or min(a[0][j] for a in pending)>hi[j] for j in range(3)):
            pieces=[pending]
        else:
            pieces=[]
            for j,v,sign in [(j,lo[j],1) for j in range(3)]+[(j,hi[j],-1) for j in range(3)]:
                if len(pending)<3:break
                pending,other=split(pending,j,v,sign)
                if len(other)>=3:pieces.append(other)
        for poly in pieces:
            at=len(verts);verts.extend([p[0] for p in poly]);uvs.extend([p[1] for p in poly]);faces.append(tuple(range(at,len(verts))))
    m=bpy.data.meshes.new(o.name+'_open');m.from_pydata(verts,[],faces);m.update();m.materials.append(data.materials[0]);uv=m.uv_layers.new(name='SourceUV')
    for loop in m.loops:uv.data[loop.index].uv=uvs[loop.vertex_index]
    o.data=m
def add_box(p,s,mat):
    bpy.ops.mesh.primitive_cube_add(size=1,location=p);o=bpy.context.object;o.scale=s;bpy.ops.object.transform_apply(location=True,rotation=False,scale=True)
    o.data.materials.append(bpy.data.materials.get(mat) or bpy.data.materials.new(mat))
    layer=o.data.uv_layers.active
    # Joining Blender meshes matches UV layers by name. Keep additions in the
    # authored channel so Unreal's channel 0 retains their floorboard mapping.
    layer.name='SourceUV'
    for face in o.data.polygons:
        axis=max(range(3),key=lambda j:abs(face.normal[j]));axes=[j for j in range(3) if j!=axis]
        for i in face.loop_indices:
            v=o.data.vertices[o.data.loops[i].vertex_index].co;layer.data[i].uv=(v[axes[0]]/2,v[axes[1]]/2)
    return o
def adapt_house():
    bpy.ops.wm.read_factory_settings(use_empty=True);dae()
    # The source is authored in centimetres despite its Collada metre tag.
    # Rotate its south porch to face the den, preserving the main 12 m footprint.
    scale=12/(8.789816-(-4.955));floor=.61
    for o in list(bpy.context.scene.objects):
        if o.type!='MESH':continue
        o.data.materials[0].name='F_SketchHouse1' if o.data.materials[0].name in ['house_1','F_SketchHouse1'] else 'F_SketchHouse2'
        for v in o.data.vertices:
            p=v.co*.01;v.co=((p.y-1.917408)*scale,-p.x*scale,(p.z+2.006389)*scale)
        # Main door, rear service door and a low south-wall squirrel opening.
        for lo,hi in [((-6.2,-.70,floor-.025),(-5.80,.70,floor+2.12)),((5.80,-.70,floor-.025),(6.20,.70,floor+2.12)),((-.4,-5.02,floor+.02),(1.2,-4.62,floor+.44))]:clip_opening(o,lo,hi)
    add_box((0,0,floor-.035),(11.85,9.48,.07),'F_Wood')
    add_box((0,0,floor+2.18),(11.85,9.48,.08),'F_Trim')
    # Door casings and visibly open timber panels match the original porch.
    for x in [-6,6]:
        for y in [-.745,.745]:add_box((x,y,floor+1.065),(.18,.09,2.13),'F_Trim')
        add_box((x,0,floor+2.165),(.18,1.58,.09),'F_Trim')
        add_box((x+.43 if x<0 else x-.43,.80,floor+1.04),(.86,.055,2.08),'F_Wood')
    for x in [-.445,1.245]:add_box((x,-4.81,floor+.23),(.07,.15,.51),'F_Trim')
    add_box((.4,-4.81,floor+.485),(1.76,.15,.07),'F_Trim')
    # A modest central board ramp crosses the raised porch steps for squirrels.
    for p,s,axis,angle in [((-9.60,0,.315),(2.34,1.35,.075),'Y',-math.atan2(.61,2.30)),((.4,-5.77,.32),(1.1,1.96,.065),'X',math.atan2(.61,1.94))]:
        ramp=add_box(p,s,'F_Wood');rotation=Matrix.Rotation(angle,3,axis);centre=Vector(p)
        for v in ramp.data.vertices:v.co=centre+rotation@(v.co-centre)
    # Pantry fixtures leave a broad centre aisle; sacks and spills are floor-level.
    for y in [-4.30,4.30]:
        add_box((3.4,y,floor+.45),(3.8,.65,.90),'F_Trim');add_box((3.4,y,floor+.925),(3.92,.72,.06),'F_Wood')
        for x in [2.15,3.40,4.65]:add_box((x,y+(.34 if y>0 else -.34),floor+.46),(1.10,.035,.77),'F_Wood')
    add_box((-3.4,3.05,floor+.78),(2.0,1.15,.08),'F_Wood')
    for x in [-4.22,-2.58]:
        for y in [2.64,3.46]:add_box((x,y,floor+.38),(.09,.09,.76),'F_Trim')
    add_box((4.75,-2.9,floor+.95),(.12,1.30,1.90),'F_Wood')
    for z in [.12,.70,1.30,1.88]:add_box((4.25,-2.9,floor+z),(1.08,1.3,.065),'F_Wood')
    for x in [2.4,3.2,4.0]:
        for y in [-4.3,4.3]:
            bpy.ops.mesh.primitive_uv_sphere_add(segments=16,ring_count=10,location=(x,y,floor+1.08));o=bpy.context.object;o.scale=(.15,.15,.22);o.data.materials.append(bpy.data.materials.new('F_Hay') if not bpy.data.materials.get('F_Hay') else bpy.data.materials['F_Hay']);bpy.ops.object.transform_apply(location=True,rotation=False,scale=True)
    export_mesh([o for o in bpy.context.scene.objects if o.type=='MESH'],'SM_Farmhouse')
    bpy.ops.wm.save_as_mainfile(filepath=str(src/'Farmhouse_Playable.blend'))
def adapt_gun():
    bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(src/'Shotgun/source/shotgun.fbx'))
    o=next(o for o in bpy.context.scene.objects if o.type=='MESH')
    for v in o.data.vertices:
        p=o.matrix_world@v.co;v.co=(-p.y*.105-.25,p.x*.105,p.z*.105-.111)
    o.matrix_world=Matrix.Identity(4);o.data.materials[0].name='F_SketchShotgun'
    bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.000001);seen=set();pump=[]
    for v in bm.verts:
        if v in seen:continue
        todo=[v];island=[];seen.add(v)
        while todo:
            q=todo.pop();island.append(q)
            for e in q.link_edges:
                r=e.other_vert(q)
                if r not in seen:seen.add(r);todo.append(r)
        # The isolated wooden fore-end, rather than the magazine or fixed barrel.
        if len(island)==86:pump=island
    assert pump,'Source pump island changed'
    bm.select_mode={'VERT'}
    for f in bm.faces:f.select=False
    for e in bm.edges:e.select=False
    for v in bm.verts:v.select=False
    for v in pump:v.select_set(True)
    bm.select_flush_mode()
    bm.to_mesh(o.data);bm.free();bpy.context.view_layer.objects.active=o;o.select_set(True)
    bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_mode(type='VERT');bpy.ops.mesh.separate(type='SELECTED');bpy.ops.object.mode_set(mode='OBJECT')
    objs=[a for a in bpy.context.scene.objects if a.type=='MESH'];action=next(a for a in objs if len(a.data.vertices)==86);body=next(a for a in objs if a!=action)
    export_mesh([body],'SM_Shotgun');export_mesh([action],'SM_ShotgunAction')
    bpy.ops.wm.save_as_mainfile(filepath=str(src/'Shotgun_Game.blend'))
    print('SKETCHFAB ADAPTATION READY: farmhouse floor 61 cm, front/rear open, side vent, shotgun muzzle (57.4,0,2) cm')
if __name__=='__main__':adapt_house();adapt_gun()
