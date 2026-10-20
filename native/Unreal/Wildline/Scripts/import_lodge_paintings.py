"""Import only the two retained museum paintings and their authored frames."""
from pathlib import Path
source=Path(__file__).with_name('import_lobby_assets.py').read_text()
exec(source[:source.index('materials=json.loads')])
materials=json.loads((SRC/'material-index.json').read_text())
for key,info in materials.items():
    if key.startswith('M_Painting'):
        m=material(key,info)
        if key in ['M_PaintingClearingDitch','M_PaintingEncouragingHounds']:
            # A small diffuse fill keeps the oil image legible under the fixed
            # interior exposure. The canvas still retains its lit PBR surface.
            fill=node(m,ue.MaterialExpressionMultiply);fill.set_editor_property('const_b',18)
            mel.connect_material_expressions(sample(m,texture(info['color'])),'RGB',fill,'A')
            connect(fill,ue.MaterialProperty.MP_EMISSIVE_COLOR)
            mel.recompile_material(m);eal.save_loaded_asset(m,only_if_is_dirty=False)
for entry in json.loads((SRC/'model-index.json').read_text()):
    name=entry['name']
    if not name.startswith('SM_Painting'):continue
    options=ue.FbxImportUI();options.automated_import_should_detect_type=False;options.import_as_skeletal=False;options.import_materials=False;options.import_textures=False;options.import_animations=False;options.mesh_type_to_import=ue.FBXImportType.FBXIT_STATIC_MESH
    options.static_mesh_import_data.combine_meshes=True;options.static_mesh_import_data.auto_generate_collision=False;options.static_mesh_import_data.normal_import_method=ue.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS
    task=ue.AssetImportTask();task.factory=ue.FbxFactory();task.filename=str(SRC/'Export'/(name+'.fbx'));task.destination_path=BASE+'/Meshes';task.destination_name=name;task.automated=True;task.replace_existing=True;task.options=options;task.save=True
    assets.import_asset_tasks([task]);mesh=eal.load_asset(BASE+'/Meshes/'+name);assert mesh
    for index,slot in enumerate(mesh.static_materials):mesh.set_material(index,eal.load_asset(BASE+'/Materials/'+str(slot.material_slot_name).split('.')[0]))
    settings=mesh.get_editor_property('nanite_settings');settings.set_editor_property('enabled',True);mesh.set_editor_property('nanite_settings',settings);eal.save_loaded_asset(mesh,only_if_is_dirty=False)
    ue.log('LODGE PAINTING IMPORTED '+name)
ue.SystemLibrary.quit_editor()
