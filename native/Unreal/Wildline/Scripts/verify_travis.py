import unreal as ue
eal=ue.EditorAssetLibrary
for name in ['SK_HunterMale','SK_HunterFemale','SK_HunterTravis']:
    mesh=eal.load_asset('/Game/Wildline/Hunters/Meshes/'+name);assert mesh
    morphs=[m.get_name() for m in mesh.get_editor_property('morph_targets')]
    assert all(m in morphs for m in ['FaceWidth','NoseDepth','SlimBuild']),(name,morphs)
    ue.log('HUNTER MORPHS '+name+' '+str(morphs))
    paths=[s.material_interface.get_path_name() for s in mesh.get_editor_property('materials')]
    assert all(p.startswith('/Game/Wildline/Hunters/Materials/') for p in paths),(name,paths)
    for s in mesh.get_editor_property('materials'):
        assert s.material_interface.get_editor_property('used_with_morph_targets'),(name,s.material_interface.get_path_name(),'missing morph shader usage')
    ue.log('HUNTER PERSISTED MATERIALS PASS '+name+' '+str(paths))
for name in ['T_travis_face','T_travis_face_shaved']:assert eal.load_asset('/Game/Wildline/Hunters/Textures/'+name)
ue.log('TRAVIS ASSET VERIFICATION PASS');ue.SystemLibrary.quit_editor()
