"""Explicit Interchange settings shared by both photo hunter imports.

FbxImportUI does not reliably replace saved Interchange reimport settings.
Always import the authored vertex masks and normals from the current FBX.
"""
import unreal as ue,json
def import_travis_mesh(root,name):
    slug=name.removeprefix('SK_Hunter').lower()
    authored=json.loads((root/f'ArtSource/Hunters/{slug}-model.json').read_text())
    assert authored.get('vertexColorLayers')==['PhotoMask'],'Export the single authored mask layer first'
    pipe=ue.InterchangeGenericAssetsPipeline()
    pipe.set_editor_property('reimport_strategy',ue.ReimportStrategyFlags.APPLY_PIPELINE_PROPERTIES)
    pipe.set_editor_property('asset_name',name)
    common=pipe.get_editor_property('common_meshes_properties')
    common.set_editor_property('vertex_color_import_option',ue.InterchangeVertexColorImportOption.IVCIO_REPLACE)
    common.set_editor_property('recompute_normals',False)
    common.set_editor_property('recompute_tangents',True)
    skeletal=pipe.get_editor_property('common_skeletal_meshes_and_animations_properties')
    skeletal.set_editor_property('try_auto_select_skeleton',False)
    skeletal.set_editor_property('skeleton',ue.EditorAssetLibrary.load_asset('/Game/Characters/Mannequins/Meshes/SK_Mannequin'))
    mesh=pipe.get_editor_property('mesh_pipeline')
    mesh.set_editor_property('import_static_meshes',False)
    mesh.set_editor_property('import_skeletal_meshes',True)
    mesh.set_editor_property('import_morph_targets',True)
    mesh.set_editor_property('create_physics_asset',False)
    pipe.get_editor_property('animation_pipeline').set_editor_property('import_animations',False)
    materials=pipe.get_editor_property('material_pipeline');materials.set_editor_property('import_materials',False)
    materials.get_editor_property('texture_pipeline').set_editor_property('import_textures',False)
    params=ue.ImportAssetParameters();params.is_automated=True;params.replace_existing=True
    params.override_pipelines=[ue.SoftObjectPath(pipe.get_path_name())]
    manager=ue.InterchangeManager.get_interchange_manager_scripted()
    source=ue.InterchangeManager.create_source_data(str(root/'ArtSource/Hunters/Export'/(name+'.fbx')))
    result=manager.import_asset('/Game/Wildline/Hunters/Meshes',source,params)
    assert result,('Interchange import failed',name)
