import unreal as ue,json
from pathlib import Path
root=Path(ue.Paths.project_dir()).resolve();assets=ue.AssetToolsHelpers.get_asset_tools();eal=ue.EditorAssetLibrary
task=ue.AssetImportTask();task.filename=str(root/'ArtSource/Hunters/Textures/travis_face_shaved.png');task.destination_path='/Game/Wildline/Hunters/Textures';task.destination_name='T_travis_face_shaved';task.automated=True;task.replace_existing=True;task.save=True;assets.import_asset_tasks([task])
report={}
for name in ['SK_HunterMale','SK_HunterFemale','SK_HunterTravis']:
    mesh=eal.load_asset('/Game/Wildline/Hunters/Meshes/'+name);assert mesh
    mats=[]
    mapping={'H_Skin':'M_Skin_female' if 'Female' in name else 'M_Skin_male','H_Cloth':'M_TravisCamo' if 'Travis' in name else 'M_Cloth','H_Boots':'M_Boots','H_Hair':'M_Hair','H_Eyes':'M_Eyes','H_Brows':'M_Brows','H_Cap':'M_Cap','H_Beard':'M_TravisBeard','H_Vest':'M_TravisVest'}
    for source in mesh.get_editor_property('materials'):
        key=str(source.material_slot_name);material_name=next((v for k,v in mapping.items() if key==k or key.startswith(k+'_')),None);assert material_name,(name,key)
        m=eal.load_asset('/Game/Wildline/Hunters/Materials/'+material_name);assert m
        ue.MaterialEditingLibrary.set_material_usage(m,ue.MaterialUsage.MATUSAGE_SKELETAL_MESH);ue.MaterialEditingLibrary.set_material_usage(m,ue.MaterialUsage.MATUSAGE_MORPH_TARGETS);ue.MaterialEditingLibrary.recompile_material(m);eal.save_loaded_asset(m)
        s=ue.SkeletalMaterial();s.set_editor_property('material_interface',m);s.set_editor_property('material_slot_name',source.material_slot_name);mats.append(s)
    mesh.call_method('SetMaterials',(mats,));eal.save_loaded_asset(mesh,only_if_is_dirty=False)
    paths=[s.get_editor_property('material_interface').get_path_name() for s in mesh.get_editor_property('materials')]
    assert all(p.startswith('/Game/Wildline/Hunters/Materials/') for p in paths),(name,paths)
    report[name]={'skeleton':mesh.get_editor_property('skeleton').get_path_name(),'materials':paths}
(root/'ArtSource/Hunters/travis-unreal.json').write_text(json.dumps(report,indent=2));ue.log('TRAVIS CONTENT READY '+json.dumps(report));ue.SystemLibrary.quit_editor()
