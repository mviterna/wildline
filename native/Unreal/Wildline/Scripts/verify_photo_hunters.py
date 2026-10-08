"""Persisted asset checks for the selected photo hunter approach."""
import unreal as ue
eal=ue.EditorAssetLibrary
base='/Game/Wildline/Hunters/'
for name in ['SK_HunterMale','SK_HunterFemale','SK_HunterTravis','SK_HunterEthan','SK_HunterGreyson']:
    mesh=eal.load_asset(base+'Meshes/'+name);assert mesh
    photo=name in ['SK_HunterTravis','SK_HunterEthan','SK_HunterGreyson']
    morphs=[m.get_name() for m in mesh.get_editor_property('morph_targets')]
    expected=['FaceWidth','SlimBuild'] if photo else ['FaceWidth','NoseDepth','SlimBuild']
    assert all(m in morphs for m in expected),(name,morphs)
    paths=[s.material_interface.get_path_name() for s in mesh.get_editor_property('materials')]
    assert all(p.startswith(base+'Materials/') for p in paths),(name,paths)
    for s in mesh.get_editor_property('materials'):
        assert s.material_interface.get_editor_property('used_with_morph_targets'),(name,s.material_interface.get_path_name())
    if photo:
        person=name.removeprefix('SK_Hunter')
        assert any('M_'+person+'Photo.' in p for p in paths),(name,paths)
        assert not any(n in p for n in ['Eyes','Teeth','Beard','Brows','Occlusion','Head'] for p in paths),(name,paths)
        assert eal.load_asset(base+'Textures/T_'+person+'Face').srgb
        assert mesh.skeleton.get_path_name()=='/Game/Characters/Mannequins/Meshes/SK_Mannequin.SK_Mannequin'
        pipelines=[p for p in mesh.get_editor_property('asset_import_data').get_pipelines() if isinstance(p,ue.InterchangeGenericAssetsPipeline)]
        assert pipelines,(name,'missing authored Interchange settings')
        for p in pipelines:
            common=p.get_editor_property('common_meshes_properties')
            assert common.get_editor_property('vertex_color_import_option')==ue.InterchangeVertexColorImportOption.IVCIO_REPLACE
            assert not common.get_editor_property('recompute_normals')
    ue.log('HUNTER PERSISTED ASSET PASS '+name+' '+str(morphs)+' '+str(paths))
for retired in ['Meshes/SK_HunterTravis2','Materials/M_TravisHead','Materials/M_TravisEyes','Materials/M_TravisTeeth','Textures/T_TravisMH_Basecolor','Textures/T_TravisMH_Normal']:
    assert not eal.does_asset_exist(base+retired),retired
ue.log('PHOTO HUNTERS ASSET VERIFICATION PASS')
ue.SystemLibrary.quit_editor()
