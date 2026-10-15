"""Import full canopies and gate complete leaf cards by their stable vertex rank."""
from pathlib import Path
import unreal as ue, json
root = Path(ue.Paths.project_dir()).resolve()
assets = ue.AssetToolsHelpers.get_asset_tools(); eal = ue.EditorAssetLibrary
mel = ue.MaterialEditingLibrary
forest = '/Game/Wildline/Forest'
for name in ['M_Leaves', 'M_LivingLeaf', 'M_Needles']:
    m = eal.load_asset(forest + '/Materials/' + name)
    original = mel.get_material_property_input_node(m,ue.MaterialProperty.MP_OPACITY_MASK)
    assert original is not None, name
    if original.get_editor_property('desc') == 'Tree leaf density mask':
        continue
    original_output = mel.get_material_property_input_node_output_name(m,ue.MaterialProperty.MP_OPACITY_MASK)
    def expression(cls, label):
        n = mel.create_material_expression(m, cls)
        n.set_editor_property('desc', label); return n
    density = expression(ue.MaterialExpressionScalarParameter, 'Tree leaf density')
    density.set_editor_property('parameter_name','LeafDensity'); density.set_editor_property('default_value', 1.)
    color = expression(ue.MaterialExpressionVertexColor, 'Tree leaf card rank')
    mask = expression(ue.MaterialExpressionCustom, 'Tree leaf density mask')
    mask.set_editor_property('code', 'return Alpha * (Density >= 1 || (Density > 0 && Rank < Density) ? 1 : 0);')
    mask.set_editor_property('output_type', ue.CustomMaterialOutputType.CMOT_FLOAT1)
    inputs = []
    for key in ['Alpha','Density','Rank']:
        i = ue.CustomInput(); i.set_editor_property('input_name',key); inputs.append(i)
    mask.set_editor_property('inputs',inputs)
    assert mel.connect_material_expressions(original, original_output, mask, 'Alpha')
    assert mel.connect_material_expressions(density,'',mask,'Density')
    assert mel.connect_material_expressions(color,'G',mask,'Rank')
    assert mel.connect_material_property(mask,'',ue.MaterialProperty.MP_OPACITY_MASK)
    mel.recompile_material(m); assert eal.save_loaded_asset(m, only_if_is_dirty=False), name
mapping = {'F_Bark':'M_TreeBark','F_Leaf':'M_Leaves','F_LivingLeaf':'M_LivingLeaf','F_Needles':'M_Needles'}
for row in json.loads((root / 'ArtSource/Forest/Density/manifest.json').read_text()):
    name = row['mesh']; opt = ue.FbxImportUI()
    opt.automated_import_should_detect_type=False; opt.import_as_skeletal=False
    opt.mesh_type_to_import=ue.FBXImportType.FBXIT_STATIC_MESH
    opt.import_materials=False; opt.import_textures=False; opt.import_animations=False
    opt.static_mesh_import_data.combine_meshes=True; opt.static_mesh_import_data.auto_generate_collision=False
    opt.static_mesh_import_data.vertex_color_import_option=ue.VertexColorImportOption.REPLACE
    opt.static_mesh_import_data.normal_import_method=ue.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS
    t=ue.AssetImportTask(); t.filename=str(root/'ArtSource/Forest/Density'/(name+'.fbx'))
    t.destination_path=forest+'/Meshes'; t.destination_name=name
    t.automated=True; t.replace_existing=True; t.save=True; t.options=opt
    assets.import_asset_tasks([t]); mesh=eal.load_asset(forest+'/Meshes/'+name); assert mesh, name
    for i,slot in enumerate(mesh.get_editor_property('static_materials')):
        key=str(slot.material_slot_name); material=next((v for k,v in mapping.items() if key==k or key.startswith(k+'_')), None)
        assert material, key; mesh.set_material(i,eal.load_asset(forest+'/Materials/'+material))
    ns=mesh.get_editor_property('nanite_settings'); ns.set_editor_property('enabled',False); mesh.set_editor_property('nanite_settings',ns)
    # Match the forest's existing distance reductions. Disconnected cards retain
    # their rank while distant canopies use fewer triangles.
    reductions=[]
    for fraction,screen in [(1.,1.),(.7,.25),(.4,.08),(.18,.025)]:
        r=ue.EditorScriptingMeshReductionSettings(); r.percent_triangles=fraction; r.screen_size=screen; reductions.append(r)
    lod=ue.EditorScriptingMeshReductionOptions(); lod.auto_compute_lod_screen_size=False; lod.reduction_settings=reductions
    subsystem=ue.get_editor_subsystem(ue.StaticMeshEditorSubsystem); subsystem.set_lods(mesh,lod)
    assert subsystem.get_number_verts(mesh,0)>1000, name
    assert eal.save_loaded_asset(mesh,only_if_is_dirty=False), name
    ue.log('TREE DENSITY IMPORTED '+name)
ue.log('TREE DENSITY IMPORT: PASS')
