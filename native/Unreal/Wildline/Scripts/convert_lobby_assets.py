"""Convert downloaded CC0 models; author UV-mapped, bevelled room architecture.

Blender --background --python convert_lobby_assets.py. Source glTF UVs, normals
and material slots are preserved. Export origins are floor-centred in metres.
"""
import bpy, bmesh, json, math, random
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parent.parent/'ArtSource/Lobby'
SRC=ROOT/'Photoreal';OUT=ROOT/'Export';OUT.mkdir(exist_ok=True)
REPORT=[];MATERIALS={};rng=random.Random(42177)

def reset():bpy.ops.wm.read_factory_settings(use_empty=True)
def export(name,objects,centre=True):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objects:o.select_set(True)
    bpy.context.view_layer.objects.active=objects[0]
    bpy.ops.object.convert(target='MESH')
    points=[o.matrix_world@v.co for o in objects for v in o.data.vertices]
    low=Vector([min(v[i] for v in points) for i in range(3)])
    high=Vector([max(v[i] for v in points) for i in range(3)])
    offset=Vector(((low.x+high.x)/2,(low.y+high.y)/2,low.z)) if centre else Vector((0,0,0))
    for o in objects:o.location-=offset
    bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
    bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')),use_selection=True,
        object_types={'MESH'},bake_anim=False,axis_forward='-Y',axis_up='Z',path_mode='STRIP')
    REPORT.append({'name':name,'dimensions_m':list(high-low),
                   'triangles':sum(sum(len(p.vertices)-2 for p in o.data.polygons) for o in objects),
                   'materials':sorted(set(m.name for o in objects for m in o.data.materials if m))})

def convert_sources():
    for entry in json.loads((SRC/'sources.json').read_text()):
        if entry['type']!='model':continue
        reset();asset=entry['id'];file=SRC/asset/(asset+'_4k.gltf');data=json.loads(file.read_text())
        bpy.ops.import_scene.gltf(filepath=str(file))
        def image_path(texture):
            if not texture:return None
            index=data['textures'][texture['index']]['source']
            return str((file.parent/data['images'][index]['uri']).relative_to(ROOT))
        for i,s in enumerate(data.get('materials',[])):
            original=s.get('name','Material_'+str(i));mat=bpy.data.materials.get(original)
            if mat is None:raise RuntimeError((asset,'Missing imported material',original))
            key='M_PH_'+asset+'_'+str(i);mat.name=key
            pbr=s.get('pbrMetallicRoughness',{})
            MATERIALS[key]={'color':image_path(pbr.get('baseColorTexture')),
                'normal':image_path(s.get('normalTexture')),'arm':image_path(pbr.get('metallicRoughnessTexture')),
                'ao':image_path(s.get('occlusionTexture')),'factor':pbr.get('baseColorFactor',[1,1,1,1]),
                'roughness':pbr.get('roughnessFactor',1),'metallic':pbr.get('metallicFactor',1),
                'alpha':s.get('alphaMode','OPAQUE'),'cutoff':s.get('alphaCutoff',.5),
                'two_sided':s.get('doubleSided',False),'source':entry['page']}
        meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
        # glTF collections may contain optional lower-detail variants. Import the
        # original package as authored; no reduction is applied to source surfaces.
        export('SM_'+asset,meshes)
        print('CONVERTED',asset,REPORT[-1],flush=True)

def mat(name,asset):
    m=bpy.data.materials.get(name) or bpy.data.materials.new(name)
    maps={}
    for channel in ['diff','nor_gl','rough','disp']:
        found=list((SRC/asset).glob('*_'+channel+'_4k.*'))
        if found:maps[channel]=str(found[0].relative_to(ROOT))
    MATERIALS[name]={'color':maps.get('diff'),'normal':maps.get('nor_gl'),
        'rough_map':maps.get('rough'),'factor':[1,1,1,1],'roughness':1,'metallic':0,
        'alpha':'OPAQUE','two_sided':False,'source':'https://polyhaven.com/a/'+asset}
    assert maps.get('diff') and maps.get('nor_gl') and maps.get('rough'),(asset,'Missing PBR maps')
    return m

def box(name,p,size,m,bevel=.025):
    bpy.ops.mesh.primitive_cube_add(size=1,location=p);o=bpy.context.object;o.name=name;o.dimensions=size
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    o.data.materials.append(m)
    uv=o.data.uv_layers.active
    for face in o.data.polygons:
        normal=face.normal;drop=max(range(3),key=lambda i:abs(normal[i]));axes=[i for i in range(3) if i!=drop]
        for li in face.loop_indices:
            point=o.data.vertices[o.data.loops[li].vertex_index].co+Vector(p)
            uv.data[li].uv=(point[axes[0]]/1.8,point[axes[1]]/1.8)
    if bevel:
        mod=o.modifiers.new('Rounded worn edges','BEVEL');mod.width=bevel;mod.segments=3
        mod=o.modifiers.new('Weighted normals','WEIGHTED_NORMAL')
    return o

def log(name,axis,position,length,radius,m,seed):
    rr=random.Random(seed);verts=[];faces=[];n=48;slices=38
    for z in range(slices+1):
        t=z/slices
        for j in range(n):
            a=2*math.pi*j/n;r=radius*(1+.018*math.sin(a*9+t*11)+.022*math.sin(a*17+t*23)+.035*math.sin(a*3+t*7+seed))
            x=r*math.cos(a);y=r*math.sin(a);h=(t-.5)*length
            v=(h,x,y) if axis=='X' else (x,h,y)
            verts.append(tuple(Vector(position)+Vector(v)))
    for z in range(slices):
        for j in range(n):faces.append((z*n+j,z*n+(j+1)%n,(z+1)*n+(j+1)%n,(z+1)*n+j))
    faces.extend([tuple(reversed(range(n))),tuple(slices*n+j for j in range(n))])
    mesh=bpy.data.meshes.new(name);mesh.from_pydata(verts,[],faces);mesh.materials.append(m);mesh.update()
    o=bpy.data.objects.new(name,mesh);bpy.context.collection.objects.link(o)
    uv=mesh.uv_layers.new(name='Physical grain UV')
    for face in mesh.polygons:
        face.use_smooth=len(face.vertices)==4
        for li in face.loop_indices:
            vi=mesh.loops[li].vertex_index;j=vi%n;z=vi//n
            # Grain follows the log; wrap seam is on the back of each wall.
            uv.data[li].uv=(z/slices*length/2,j/n*.65)
    return o

def architecture(lodge_only=False):
    reset();wood=mat('M_LodgeTimber','dark_wooden_planks');floor=mat('M_LodgeFloor','wood_floor_worn')
    stone=mat('M_HearthStone','rustic_stone_wall');bark=mat('M_HollowBark','jolcham_oak_bark_01');litter=mat('M_HollowFloor','forest_leaves_03');cloth=mat('M_LodgeFabric','quatrefoil_jacquard_fabric')
    objects=[]
    for y in range(56):
        for x in range(5):
            xx=-7+(x+.5)*2.8
            objects.append(box('Hand laid oak floorboard',(xx,-6.875+y*.25,-.03+rng.uniform(-.001,.001)),(2.795,.246,.06),floor,.005))
    for row in range(21):
        z=.12+row*.235
        for side in [-1,1]:
            # Cut actual window apertures, including the radius of each log.
            # FBX flips Y; both end walls use the same two opening positions.
            x_gaps=[(-5.15,-2.95),(3.45,5.65)] if 1.55<z+.155 and z-.155<3.55 else []
            y_gaps=[(-2.35,-.15)] if side==1 and 1.55<z+.155 and z-.155<3.55 else []
            for axis,gaps,fixed,half in [('X',x_gaps,side*7.04,7.2),('Y',y_gaps,side*7.04,7.125)]:
                cursor=-half
                for a,b in gaps+[(half,half)]:
                    if a>cursor:
                        mid=(cursor+a)/2
                        p=(mid,fixed,z) if axis=='X' else (fixed,mid,z)
                        objects.append(log('Window-cut interlocking log '+axis,axis,p,a-cursor,.155,wood,row*17+side))
                    cursor=b
                # Retain the part of a boundary log above/below the rectangular
                # opening, so daylight cannot leak around the oak trim.
                for a,b in gaps:
                    for boundary,below in [(1.55,True),(3.55,False)]:
                        if (below and z-.17>=boundary) or (not below and z+.17<=boundary):continue
                        mid=(a+b)/2;p=(mid,fixed,z) if axis=='X' else (fixed,mid,z)
                        fragment=log('Window lintel log edge',axis,p,b-a,.155,wood,row*17+side)
                        bm=bmesh.new();bm.from_mesh(fragment.data)
                        bmesh.ops.bisect_plane(bm,geom=list(bm.verts)+list(bm.edges)+list(bm.faces),dist=.000001,
                            plane_co=(0,0,boundary),plane_no=(0,0,1),clear_outer=below,clear_inner=not below)
                        bmesh.ops.holes_fill(bm,edges=[e for e in bm.edges if e.is_boundary])
                        bm.to_mesh(fragment.data);bm.free()
                        if len(fragment.data.vertices):objects.append(fragment)
                        else:bpy.data.objects.remove(fragment,do_unlink=True)
    for x in [-5.9,-3,0,3,5.9]:objects.append(box('Chamfered ceiling joist',(x,0,4.54),(.26,14.15,.36),wood,.04))
    objects.append(box('Ceiling panelling',(0,0,4.86),(14.35,14.35,.12),wood,.02))
    # Stone hearth built from irregular masonry, with actual recess and chimney.
    for row in range(9):
        z=.18+row*.30
        for col in range(5):
            x=-.3+col*.65
            if row<5 and col in [1,2,3]:continue
            objects.append(box('Hearth quarried masonry',(x,-6.66,z),(.64+rng.uniform(-.04,.03),.57,.295),stone,.025))
    objects.append(box('Slate hearth slab',(1,-6.14,.06),(3.85,1.70,.12),stone,.035))
    objects.append(box('Weathered mantel',(1,-6.28,1.80),(3.85,.84,.19),wood,.035))
    export('SM_LodgeArchitecture',objects,False)
    if lodge_only:return
    # A continuous sculpted hollow trunk replaces the old polygon of flat panels.
    reset();bark=mat('M_HollowBark','jolcham_oak_bark_01');litter=mat('M_HollowFloor','forest_leaves_03');wood=mat('M_LodgeTimber','dark_wooden_planks')
    verts=[];faces=[];n=256;slices=100
    for iz in range(slices+1):
        z=iz/slices*4.35
        for j in range(n):
            a=j/n*2*math.pi
            r=4.52+.10*math.sin(a*5+z*.65)+.07*math.sin(a*11-z*.7)+.016*math.sin(a*37+z*5)+.16*math.exp(-z*2)*math.sin(a*9)
            verts.append((r*math.cos(a),r*math.sin(a),z))
    for iz in range(slices):
        for j in range(n):faces.append((iz*n+j,(iz+1)*n+j,(iz+1)*n+(j+1)%n,iz*n+(j+1)%n))
    mesh=bpy.data.meshes.new('Sculpted continuous inner trunk');mesh.from_pydata(verts,[],faces);mesh.materials.append(bark);mesh.update()
    trunk=bpy.data.objects.new('Hollow oak heartwood',mesh);bpy.context.collection.objects.link(trunk)
    uv=mesh.uv_layers.new(name='Continuous bark UV')
    for face in mesh.polygons:
        face.use_smooth=True
        for li in face.loop_indices:
            vi=mesh.loops[li].vertex_index;j=vi%n;iz=vi//n
            jj=n if j==0 and any(mesh.loops[k].vertex_index%n==n-1 for k in face.loop_indices) else j
            uv.data[li].uv=(jj/n*7,iz/slices*2.2)
    objects=[trunk,box('Leaf litter floor',(0,0,-.06),(10,10,.12),litter,.03),box('Opaque trunk crown',(0,0,4.39),(10,10,.16),wood,.03)]
    export('SM_HollowArchitecture',objects,False)
    reset();cloth=mat('M_LodgeFabric','quatrefoil_jacquard_fabric')
    rug=box('Woven hearth rug',(0,0,.015),(4.2,3.25,.03),cloth,.012);export('SM_LodgeRug',[rug],False)
    # Carved taxidermy plaques with a shield silhouette and bevelled moulding.
    reset();wood=mat('M_LodgeTimber','dark_wooden_planks')
    outline=[(-.30,.5),(.30,.5),(.43,.31),(.40,-.08),(.28,-.35),(0,-.54),(-.28,-.35),(-.40,-.08),(-.43,.31)]
    vs=[(depth,y,z+.54) for depth in [0,.045] for y,z in outline];k=len(outline)
    fs=[tuple(reversed(range(k))),tuple(range(k,k*2))]+[(i,(i+1)%k,(i+1)%k+k,i+k) for i in range(k)]
    mesh=bpy.data.meshes.new('Carved oak shield');mesh.from_pydata(vs,[],fs);mesh.materials.append(wood);mesh.update();o=bpy.data.objects.new('Shield plaque',mesh);bpy.context.collection.objects.link(o)
    bpy.context.view_layer.objects.active=o;o.select_set(True);bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project();bpy.ops.object.mode_set(mode='OBJECT')
    mod=o.modifiers.new('Rounded moulding','BEVEL');mod.width=.012;mod.segments=4;export('SM_TrophyPlaque',[o],False)

import sys
if '--lodge-only' in sys.argv:
    MATERIALS=json.loads((ROOT/'material-index.json').read_text())
    REPORT=[r for r in json.loads((ROOT/'model-index.json').read_text()) if r['name']!='SM_LodgeArchitecture']
    architecture(lodge_only=True)
elif '--architecture-only' in sys.argv:
    MATERIALS=json.loads((ROOT/'material-index.json').read_text())
    REPORT=[r for r in json.loads((ROOT/'model-index.json').read_text()) if r['name'] not in ['SM_LodgeArchitecture','SM_HollowArchitecture','SM_LodgeRug','SM_TrophyPlaque']]
    architecture()
else:
    convert_sources();architecture()
(ROOT/'model-index.json').write_text(json.dumps(REPORT,indent=2))
(ROOT/'material-index.json').write_text(json.dumps(MATERIALS,indent=2))
print('LOBBY CONVERSION COMPLETE',len(REPORT),'meshes',len(MATERIALS),'materials',flush=True)
