"""Unchanged reference photographs on featureless, rigged low-poly heads.

No modeled eyes, nose, mouth, teeth, eyebrows or beard under the photographs.
Both UV channels carry the same face coordinates. Source images are unedited.
"""
import bpy, bmesh, math, json, sys
from pathlib import Path
root=Path(__file__).resolve().parent.parent/'ArtSource/Hunters'

TRAVIS_ROWS=[(1.420,.055,-.067,528,140,183,235),(1.490,.053,-.080,528,140,183,235),
      (1.511,.035,-.113,526,145,186,232),(1.520,.050,-.121,514,126,179,243),
      (1.535,.065,-.125,480,81,166,282),(1.555,.078,-.127,431,44,157,308),
      (1.575,.083,-.130,382,23,148,325),(1.598,.087,-.132,327,11,154,340),
      (1.620,.088,-.132,275,3,150,349),(1.644,.087,-.128,214,2,137,350),
      (1.663,.086,-.119,165,2,148,350),(1.683,.085,-.109,116,2,157,347),
      (1.703,.079,-.094,68,3,164,339),(1.720,.066,-.072,27,14,172,323),
      (1.733,.047,-.043,6,42,177,295),(1.742,.018,-.014,2,136,181,223),
      (1.745,.001,-.003,2,179,181,183)]
ETHAN_ROWS=[(1.420,.055,-.067,594,170,266,381),(1.490,.053,-.080,594,170,266,381),
 (1.511,.040,-.110,567,174,268,357),(1.520,.054,-.119,552,144,268,387),
 (1.535,.070,-.125,517,99,264,424),(1.555,.084,-.129,475,64,260,457),
 (1.575,.093,-.132,431,43,255,485),(1.598,.097,-.133,378,30,250,506),
 (1.620,.097,-.132,324,28,246,516),(1.643,.096,-.128,259,32,242,518),
 (1.660,.093,-.119,213,44,246,509),(1.679,.088,-.108,158,59,250,488),
 (1.699,.078,-.094,104,89,254,456),(1.718,.065,-.072,50,130,263,417),
 (1.733,.047,-.043,15,177,270,370),(1.742,.018,-.014,3,230,274,320),
 (1.745,.001,-.003,2,272,274,276)]

GREYSON_ROWS=[(1.420,.055,-.067,384,112,158,211),(1.490,.053,-.080,384,112,158,211),
 (1.511,.035,-.113,374,121,157,193),(1.520,.050,-.121,363,96,154,214),
 (1.535,.065,-.125,346,75,151,233),(1.555,.078,-.127,325,60,149,246),
 (1.575,.083,-.130,300,43,145,251),(1.598,.087,-.132,274,35,141,252),
 (1.620,.088,-.132,247,27,138,253),(1.644,.087,-.128,219,23,135,257),
 (1.663,.086,-.119,194,20,132,263),(1.683,.085,-.109,155,12,132,267),
 (1.703,.079,-.094,110,7,131,268),(1.720,.066,-.072,66,18,130,254),
 (1.733,.047,-.043,30,44,128,233),(1.742,.018,-.014,7,94,127,167),
 (1.745,.001,-.003,4,125,127,129)]

def build(person,rows,image_width,image_height,skin_color,hair_color):
    slug=person.lower()
    bpy.ops.wm.open_mainfile(filepath=str(root/'SK_HunterMale.blend'),use_scripts=False)
    body=bpy.data.objects['SK_HunterMale'];body.name=f'SK_Hunter{person}';arm=body.parent
    names=[m.name for m in body.data.materials]
    bm=bmesh.new();bm.from_mesh(body.data)
    bmesh.ops.delete(bm,geom=[f for f in bm.faces if names[f.material_index] in ['H_Hair','H_Brows','H_Eyes','H_Beard'] or (names[f.material_index]=='H_Skin' and any(v.co.z>1.435 for v in f.verts))],context='FACES')
    bm.to_mesh(body.data);bm.free()
    def mat(name,color):
        m=bpy.data.materials.new(name);m.use_nodes=True;m['travis_authored']=True
        s=m.node_tree.nodes.get('Principled BSDF');s.inputs['Base Color'].default_value=(*color,1);s.inputs['Roughness'].default_value=.93;s.inputs['Specular IOR Level'].default_value=.1
        return m
    photo=mat(f'H_{person}Photo',(.6,.4,.3));ns=photo.node_tree.nodes;ls=photo.node_tree.links;s=ns.new('ShaderNodeEmission');ls.new(s.outputs[0],ns.get('Material Output').inputs['Surface'])
    t=ns.new('ShaderNodeTexImage');t.image=bpy.data.images.load(str(root/f'Textures/{slug}_reference.png'),check_existing=True);t.image.alpha_mode='NONE';t.interpolation='Linear'
    uv=ns.new('ShaderNodeUVMap');uv.uv_map='UVMap';ls.new(uv.outputs['UV'],t.inputs['Vector']);ls.new(t.outputs['Color'],s.inputs['Color'])
    skin=mat(f'H_{person}Skin',skin_color[:3]);hair=mat(f'H_{person}Hair',hair_color[:3])
    for m,color in [(skin,skin_color),(hair,hair_color)]:
        s=m.node_tree.nodes.new('ShaderNodeEmission');s.inputs['Color'].default_value=color;m.node_tree.links.new(s.outputs[0],m.node_tree.nodes.get('Material Output').inputs['Surface'])
    # z, half width, front y, source photo scanline, source left/centre/right.
    # The upper/bottom rings complete the cropped image with plain scalp/neck.

    verts=[];uvs=[];faces=[];slots=[];N=32
    for z,rx,fy,py,pl,pc,pr in rows:
        for j in range(N):
            a=(j/N)*2*math.pi;sin=math.sin(a);cos=math.cos(a)
            # Front is a continuous, gently curved surface with no face relief.
            x=rx*sin;edge=-.025
            y=fy+(edge-fy)*abs(sin)**2 if cos>=0 else edge+(.085 if z<1.70 else .065*rx/.079)*(-cos)
            if z>1.72:y*=rx/.066
            if z<1.510:y=(fy if cos>=0 else .06)*abs(cos)
            verts.append((x,y,z))
            px=pc+(pr-pc)*sin if sin>=0 else pc+(pc-pl)*sin
            uvs.append((px/image_width,1-py/image_height))
    for i in range(len(rows)-1):
        for j in range(N):
            nj=(j+1)%N;faces.append((i*N+j,i*N+nj,(i+1)*N+nj,(i+1)*N+j))
            a=(j+.5)/N*2*math.pi;front=math.cos(a)>0
            slots.append(0 if front and 2<=i<=13 else 2 if i>=13 or (not front and i>=10) else 1)
    mesh=bpy.data.meshes.new(f'{person}FeaturelessHead');mesh.from_pydata(verts,[],faces);mesh.update();head=bpy.data.objects.new(f'{person}FeaturelessHead',mesh);bpy.context.collection.objects.link(head)
    for m in [photo,skin,hair]:mesh.materials.append(m)
    mesh.uv_layers.new(name='UVMap');mesh.uv_layers.new(name='PhotoFace');mesh.color_attributes.new(name='PhotoMask',type='FLOAT_COLOR',domain='CORNER')
    for p,slot in zip(mesh.polygons,slots):
        p.material_index=slot;p.use_smooth=True
        for li,vi in zip(p.loop_indices,p.vertices):
            mesh.uv_layers['UVMap'].data[li].uv=uvs[vi];mesh.uv_layers['PhotoFace'].data[li].uv=uvs[vi]
            mesh.color_attributes['PhotoMask'].data[li].color=(1 if slot==0 else 0,0,0,1)
    # Small low-poly ears; no anatomical face features are added.
    parts=[head]
    for sign in [-1,1]:
        bpy.ops.mesh.primitive_uv_sphere_add(segments=10,ring_count=6,location=(sign*.087,-.012,1.624))
        ear=bpy.context.object;ear.name=f'{person}Ear';ear.scale=(.010,.016,.024);bpy.ops.object.transform_apply(location=True,rotation=True,scale=True);ear.data.materials.append(skin)
        for p in ear.data.polygons:p.use_smooth=True
        ear.data.uv_layers[0].name='UVMap';ear.data.uv_layers.new(name='PhotoFace');ear.data.color_attributes.new(name='PhotoMask',type='FLOAT_COLOR',domain='CORNER');parts.append(ear)
    for part in parts:
        part.parent=arm;part.matrix_parent_inverse=arm.matrix_world.inverted()
        for n in ['head','neck_01']:part.vertex_groups.new(name=n)
        for v in part.data.vertices:
            w=max(0,min(1,(v.co.z-1.49)/.07));part.vertex_groups['head'].add([v.index],w,'REPLACE')
            if w<1:part.vertex_groups['neck_01'].add([v.index],1-w,'REPLACE')
        mod=part.modifiers.new('Unreal locomotion','ARMATURE');mod.object=arm
        part.shape_key_add(name='Basis')
        for n in ['FaceWidth','NoseDepth','SlimBuild']:
            k=part.shape_key_add(name=n)
            if n=='FaceWidth':
                for i,v in enumerate(part.data.vertices):k.data[i].co.x*=1.08
        # NoseDepth intentionally has no displacement on this featureless shell.
    for p in body.data.polygons:
        for li in p.loop_indices:
            co=body.data.vertices[body.data.loops[li].vertex_index].co;c=body.data.color_attributes['PhotoMask'].data[li].color;c[0]=0;c[1]=1 if names[p.material_index]=='H_Cloth' and 1.115<co.z<1.46 and abs(co.x)<.21 else 0;body.data.color_attributes['PhotoMask'].data[li].color=c
    bpy.ops.object.select_all(action='DESELECT')
    for o in [body]+parts:o.select_set(True)
    bpy.context.view_layer.objects.active=body;bpy.ops.object.join();body['identity']=f'{person}: supplied photograph on a featureless low-poly head'
    body.data.color_attributes.active_color=body.data.color_attributes['PhotoMask'];body.data.color_attributes.render_color_index=list(body.data.color_attributes).index(body.data.color_attributes['PhotoMask'])
    body.data.uv_layers.active_index=0;body.data.uv_layers[0].active_render=True
    assert [a.name for a in body.data.color_attributes]==['PhotoMask']
    assert all(math.isfinite(c) for v in body.data.vertices for c in v.co)
    assert all(abs(sum(g.weight for g in v.groups)-1)<.002 for v in body.data.vertices)
    bpy.ops.object.select_all(action='DESELECT');body.select_set(True);arm.select_set(True);bpy.context.view_layer.objects.active=arm
    bpy.ops.export_scene.fbx(filepath=str(root/f'Export/SK_Hunter{person}.fbx'),use_selection=True,object_types={'MESH','ARMATURE'},add_leaf_bones=False,bake_anim=False,axis_forward='-Y',axis_up='Z',path_mode='STRIP')
    bpy.ops.wm.save_as_mainfile(filepath=str(root/f'SK_Hunter{person}.blend'))
    report={'mesh':body.name,'vertices':len(body.data.vertices),'polygons':len(body.data.polygons),'faceVertices':len(verts),'facePolygons':len(faces),'photoOverlay':True,'modeledFacialFeatures':False,'sourcePhoto':f'Textures/{slug}_reference.png','vertexColorLayers':[a.name for a in body.data.color_attributes],'morphs':[k.name for k in body.data.shape_keys.key_blocks]}
    (root/f'{slug}-model.json').write_text(json.dumps(report,indent=2));print(person.upper()+' PHOTO EXPORTED',json.dumps(report))


selected=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
if not selected or 'Travis' in selected:build('Travis',TRAVIS_ROWS,363,529,(.44,.29,.235,1),(.044,.032,.025,1))
if not selected or 'Ethan' in selected:build('Ethan',ETHAN_ROWS,540,595,(.30,.25,.24,1),(.025,.030,.042,1))
if not selected or 'Greyson' in selected:build('Greyson',GREYSON_ROWS,271,385,(.48,.34,.30,1),(.070,.056,.043,1))
