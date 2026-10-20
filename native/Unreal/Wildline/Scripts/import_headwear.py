"""Import original headwear, materials and rendered gallery thumbnails."""
import unreal as ue, json
from pathlib import Path
ROOT=Path(ue.Paths.project_dir()).resolve();SRC=ROOT/'ArtSource/Hunters/Headwear';BASE='/Game/Wildline/Hunters/Headwear'
assets=ue.AssetToolsHelpers.get_asset_tools();eal=ue.EditorAssetLibrary;mel=ue.MaterialEditingLibrary
ue.SystemLibrary.execute_console_command(ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world(),'Interchange.FeatureFlags.Import.FBX 0')
for folder in ['Meshes','Materials','Thumbnails']:eal.make_directory(BASE+'/'+folder)
for data in json.loads((SRC/'materials.json').read_text()).values():
    path=BASE+'/Materials/'+data['asset'];m=eal.load_asset(path) if eal.does_asset_exist(path) else None
    if not m:m=assets.create_asset(data['asset'],BASE+'/Materials',ue.Material,ue.MaterialFactoryNew())
    mel.delete_all_material_expressions(m)
    c=mel.create_material_expression(m,ue.MaterialExpressionConstant3Vector);c.constant=ue.LinearColor(*data['color'],1);mel.connect_material_property(c,'',ue.MaterialProperty.MP_BASE_COLOR)
    for value,prop in [(data['roughness'],ue.MaterialProperty.MP_ROUGHNESS),(data['metallic'],ue.MaterialProperty.MP_METALLIC)]:
        n=mel.create_material_expression(m,ue.MaterialExpressionConstant);n.r=value;mel.connect_material_property(n,'',prop)
    mel.recompile_material(m);eal.save_loaded_asset(m,only_if_is_dirty=False)
styles=json.loads((SRC/'styles.json').read_text())
for style in styles:
    slug=style['slug'];name='SM_Hat_'+slug
    if style['id']>0:
        options=ue.FbxImportUI();options.automated_import_should_detect_type=False;options.mesh_type_to_import=ue.FBXImportType.FBXIT_STATIC_MESH;options.import_as_skeletal=False;options.import_materials=False;options.import_textures=False;options.import_animations=False
        options.static_mesh_import_data.combine_meshes=True;options.static_mesh_import_data.auto_generate_collision=False;options.static_mesh_import_data.normal_import_method=ue.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS
        task=ue.AssetImportTask();task.factory=ue.FbxFactory();task.filename=str(SRC/'Export'/(name+'.fbx'));task.destination_path=BASE+'/Meshes';task.destination_name=name;task.options=options;task.automated=True;task.replace_existing=True;task.save=True
        assets.import_asset_tasks([task]);mesh=eal.load_asset(BASE+'/Meshes/'+name);assert mesh,name
        for i,slot in enumerate(mesh.static_materials):
            mat=eal.load_asset(BASE+'/Materials/'+str(slot.material_slot_name).split('.')[0]);assert mat,str(slot.material_slot_name);mesh.set_material(i,mat)
        eal.save_loaded_asset(mesh,only_if_is_dirty=False)
    name='T_Hat_'+slug;task=ue.AssetImportTask();task.filename=str(SRC/'Thumbnails/Studio'/(name+'.png'));task.destination_path=BASE+'/Thumbnails';task.destination_name=name;task.automated=True;task.replace_existing=True;task.save=True;assets.import_asset_tasks([task])
    tex=eal.load_asset(BASE+'/Thumbnails/'+name);assert tex,name;tex.set_editor_property('lod_group',ue.TextureGroup.TEXTUREGROUP_UI);tex.set_editor_property('mip_gen_settings',ue.TextureMipGenSettings.TMGS_NO_MIPMAPS);tex.set_editor_property('compression_settings',ue.TextureCompressionSettings.TC_EDITOR_ICON);eal.save_loaded_asset(tex,only_if_is_dirty=False)
    ue.log('HEADWEAR_IMPORTED '+slug)
ue.log('HEADWEAR_IMPORT_COMPLETE '+str(len(styles)));ue.SystemLibrary.quit_editor()
