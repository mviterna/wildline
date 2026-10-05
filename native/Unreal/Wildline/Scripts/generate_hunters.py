"""Build CC0 outdoor hunters on the existing Unreal locomotion skeleton."""
import bpy,sys,json,math,shutil
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parent.parent/'ArtSource/Hunters'
workspace=root.parents[4]
download=workspace/'native/AssetDownloads/Hunters'
sys.path.insert(0,str(download/'MPFB/mpfb2-master/src'))
bpy.utils.extension_path_user=lambda *args,**kwargs:str(download/'User')
import mpfb
mpfb.bl_info=mpfb.fake_bl_info
mpfb.get_preference=lambda name:{'mpfb_shelf_label':'MPFB','mpfb_user_data':str(download/'User'),'mpfb_enable_developer':False}.get(name)
mpfb.register()
from mpfb.services.humanservice import HumanService
from mpfb.services.targetservice import TargetService
from mpfb.services.assetservice import AssetService
from mpfb.services.exportservice import ExportService
from mpfb.services.objectservice import ObjectService
from mpfb.services.locationservice import LocationService
from mpfb.entities.objectproperties import HumanObjectProperties
LocationService._user_data=str(download/'System')
print('MPFB_READY',LocationService.get_user_data())
macro=TargetService.get_default_macro_info_dict();macro.update(gender=1.,age=.55,muscle=.60,weight=.52,height=.5,race={'african':0.,'asian':0.,'caucasian':1.})
from mathutils.kdtree import KDTree
out=root/'Export';out.mkdir(exist_ok=True);texdir=root/'Textures';texdir.mkdir(exist_ok=True);report=[]
for gender,name in [(1.,'SK_HunterMale'),(0.,'SK_HunterFemale')]:
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    macro['gender']=gender
    human=HumanService.create_human(macro_detail_dict=macro)
    HumanService.set_character_skin(AssetService.find_asset_absolute_path('young_caucasian_male.mhmat' if gender else 'young_caucasian_female.mhmat',asset_subdir='skins'),human,skin_type='GAMEENGINE')
    HumanService.add_builtin_rig(human,'game_engine')
    for subdir,fname,kind in [('eyes','low-poly.mhclo','Eyes'),('eyebrows','eyebrow001.mhclo','Eyebrows'),('hair','short01.mhclo','Hair'),('clothes','male_casualsuit05.mhclo','Clothes'),('clothes','shoes04.mhclo','Clothes')]:
        HumanService.add_mhclo_asset(AssetService.find_asset_absolute_path(fname,asset_subdir=subdir),human,asset_type=kind,material_type='GAMEENGINE')
    # Bake generated anatomy and clothing masks, removing all helper geometry.
    export_root=ExportService.create_character_copy(human,name_suffix='_export')
    body=ObjectService.find_object_of_type_amongst_nearest_relatives(export_root,'Basemesh')
    ExportService.bake_modifiers_remove_helpers(body,bake_masks=True,bake_subdiv=True,remove_helpers=True,also_proxy=True)
    # Macro proportions live in shape keys; the Basis crown is not the visible head.
    bpy.context.view_layer.update()
    evaluated_body=body.evaluated_get(bpy.context.evaluated_depsgraph_get())
    head_crown=max((evaluated_body.matrix_world@v.co).z for v in evaluated_body.data.vertices)
    print('EVALUATED_HEAD_CROWN',name,head_crown)
    parts=ObjectService.get_list_of_children(export_root)
    deps=bpy.context.evaluated_depsgraph_get();new=[]
    for part in parts:
        if part.type!='MESH':continue
        mesh=bpy.data.meshes.new_from_object(part.evaluated_get(deps),depsgraph=deps)
        obj=bpy.data.objects.new(part.name+'_Game',mesh);bpy.context.collection.objects.link(obj)
        for v in mesh.vertices:v.co=part.matrix_world@v.co
        key='H_Skin' if 'Basemesh'==ObjectService.get_object_type(part) else ('H_Cloth' if 'casualsuit' in part.name else 'H_Boots' if 'shoes' in part.name else 'H_Hair' if 'short01' in part.name else 'H_Brows' if 'eyebrow' in part.name else 'H_Eyes')
        original=part.data.materials[0]
        textures=[]
        if original.use_nodes:
            for node in original.node_tree.nodes:
                if node.type=='TEX_IMAGE' and node.image:
                    path=Path(bpy.path.abspath(node.image.filepath));dest=texdir/path.name
                    if path.is_file():shutil.copy2(path,dest);textures.append(path.name)
        mat=bpy.data.materials.get(key) or bpy.data.materials.new(key);mesh.materials.clear();mesh.materials.append(mat)
        for p in mesh.polygons:p.material_index=0;p.use_smooth=True
        # Additional UV channel and a frontal-face blending mask for runtime photos.
        uv=mesh.uv_layers.new(name='PhotoFace');vc=mesh.color_attributes.new(name='PhotoMask',type='FLOAT_COLOR',domain='CORNER')
        for p in mesh.polygons:
            for li,vi in zip(p.loop_indices,p.vertices):
                v=mesh.vertices[vi];uv.data[li].uv=(.5+v.co.x/.205,(v.co.z-(head_crown-.264))/.255)
                mask=max(0,min(1,(-v.normal.y-.10)*1.5))*max(0,min(1,(v.co.z-(head_crown-.234))*40)) if key=='H_Skin' else 0
                vc.data[li].color=(mask,0,0,1)
        mesh.uv_layers.active_index=0
        mesh.uv_layers[0].active_render=True
        new.append(obj);report.append({'hunter':name,'slot':key,'textures':textures})
    # An original field cap with curved brim, in meters.
    crown=max(v.co.z for o in new if o.data.materials[0].name=='H_Skin' for v in o.data.vertices)
    cap_bottom=crown-.049
    verts=[];faces=[]
    for row in range(9):
        a=row/8*math.pi*.5
        for j in range(32):
            t=j/32*math.tau;verts.append((.09*math.cos(a)*math.cos(t),-.047+.105*math.cos(a)*math.sin(t),cap_bottom+.075*math.sin(a)))
    for row in range(8):
        for j in range(32):faces.append((row*32+j,row*32+(j+1)%32,(row+1)*32+(j+1)%32,(row+1)*32+j))
    for j in range(13):
        a=math.pi+j/12*math.pi;n=len(verts);verts.extend([(.09*math.cos(a),-.047+.105*math.sin(a),cap_bottom+.001),(.13*math.cos(a),-.047+.195*math.sin(a),cap_bottom-.012)])
        if j:faces.append((n-2,n,n+1,n-1))
    cm=bpy.data.meshes.new('Field cap');cm.from_pydata(verts,[],faces);co=bpy.data.objects.new('Field cap',cm);bpy.context.collection.objects.link(co);cm.materials.append(bpy.data.materials.get('H_Cap') or bpy.data.materials.new('H_Cap'));new.append(co)
    for p in cm.polygons:p.use_smooth=True
    # The source human and Manny share closely matching A poses. Transfer skin
    # weights from the engine mesh, retaining its exact locomotion skeleton.
    bpy.ops.import_scene.fbx(filepath=str(root/'MannyReference.fbx'))
    arm=next(o for o in bpy.context.selected_objects if o.type=='ARMATURE')
    ref=next(o for o in bpy.context.selected_objects if o.type=='MESH' and 'LOD0' in o.name)
    tree=KDTree(len(ref.data.vertices))
    for v in ref.data.vertices:tree.insert(ref.matrix_world@v.co,v.index)
    tree.balance()
    for obj in new:
        obj.vertex_groups.clear()
        for vg in ref.vertex_groups:obj.vertex_groups.new(name=vg.name)
        # Evaluated meshes retain numeric deform layers from MPFB. Those indices
        # must be cleared before adding weights for a different skeleton.
        indices=list(range(len(obj.data.vertices)))
        for vg in obj.vertex_groups:vg.remove(indices)
        for v in obj.data.vertices:
            if obj.data.materials[0].name in ['H_Cap','H_Hair','H_Brows','H_Eyes']:
                obj.vertex_groups['head'].add([v.index],1.,'REPLACE')
            else:
                neighbors=tree.find_n(v.co,4);weights={};total=0
                for _,idx,distance in neighbors:
                    influence=1/max(.002,distance)**2;total+=influence
                    for g in ref.data.vertices[idx].groups:weights[g.group]=weights.get(g.group,0)+g.weight*influence
                for group,weight in weights.items():obj.vertex_groups[group].add([v.index],weight/total,'REPLACE')
        obj.parent=arm;obj.matrix_parent_inverse=arm.matrix_world.inverted()
        mod=obj.modifiers.new('Unreal locomotion','ARMATURE');mod.object=arm
    bpy.ops.object.select_all(action='DESELECT')
    for obj in new:obj.select_set(True)
    bpy.context.view_layer.objects.active=new[0];bpy.ops.object.join();hunter=new[0];hunter.name=name
    for v in hunter.data.vertices:
        assert abs(sum(g.weight for g in v.groups)-1)<.001,(name,v.index,[(hunter.vertex_groups[g.group].name,g.weight) for g in v.groups])
    hunter.data.uv_layers.active_index=0
    hunter.data.uv_layers[0].active_render=True
    hunter.shape_key_add(name='Basis')
    for key in ['FaceWidth','NoseDepth']:
        shape=hunter.shape_key_add(name=key)
        for i,v in enumerate(hunter.data.vertices):
            if v.co.z<head_crown-.234:continue
            if key=='FaceWidth':shape.data[i].co.x*=1.08
            elif abs(v.co.x)<.024 and head_crown-.174<v.co.z<head_crown-.104 and v.co.y<-.035:shape.data[i].co.y-=.012
    bpy.ops.object.select_all(action='DESELECT');hunter.select_set(True);arm.select_set(True);bpy.context.view_layer.objects.active=arm
    bpy.ops.export_scene.fbx(filepath=str(out/(name+'.fbx')),use_selection=True,object_types={'MESH','ARMATURE'},add_leaf_bones=False,bake_anim=False,axis_forward='-Y',axis_up='Z',path_mode='STRIP')
    bpy.ops.wm.save_as_mainfile(filepath=str(root/(name+'.blend')))
    print('HUNTER_EXPORTED',name,len(hunter.data.vertices),[m.name for m in hunter.data.materials])
(root/'materials.json').write_text(json.dumps(report,indent=2))
