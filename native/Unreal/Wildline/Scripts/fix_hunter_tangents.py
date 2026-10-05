import unreal as u
for name in ['M_Cloth','M_Boots']:
    m=u.EditorAssetLibrary.load_asset('/Game/Wildline/Hunters/Materials/'+name)
    n=u.MaterialEditingLibrary.create_material_expression(m,u.MaterialExpressionConstant3Vector)
    n.set_editor_property('constant',u.LinearColor(0,0,1,1))
    u.MaterialEditingLibrary.connect_material_property(n,'',u.MaterialProperty.MP_NORMAL)
    u.MaterialEditingLibrary.recompile_material(m)
    u.EditorAssetLibrary.save_loaded_asset(m)
u.SystemLibrary.quit_editor()
