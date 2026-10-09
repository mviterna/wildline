"""Persist Nanite usage on our material library and audit licensed vehicle slots."""
import unreal as ue
eal=ue.EditorAssetLibrary;mel=ue.MaterialEditingLibrary
count=0
for path in eal.list_assets('/Game/Wildline',recursive=True,include_folder=False):
    data=eal.find_asset_data(path)
    if str(data.asset_class_path.asset_name)!='Material':continue
    m=eal.load_asset(path)
    mel.set_material_usage(m,ue.MaterialUsage.MATUSAGE_NANITE)
    mel.set_material_usage(m,ue.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    eal.save_loaded_asset(m,only_if_is_dirty=False);count+=1
for path in eal.list_assets('/Game/Wildline/Fab/Meshes',recursive=True,include_folder=False):
    mesh=eal.load_asset(path)
    for i,slot in enumerate(mesh.get_editor_property('static_materials')):
        m=slot.material_interface
        ue.log('FAB MATERIAL '+mesh.get_name()+' '+str(i)+' '+str(m))
        if isinstance(m,ue.MaterialInstanceConstant):
            ue.log('FAB PARENT '+str(m.get_editor_property('parent')))
            for n in mel.get_texture_parameter_names(m):ue.log('FAB TEXTURE '+str(n)+' '+str(mel.get_material_instance_texture_parameter_value(m,n)))
ue.log('WORLD MATERIAL USAGE READY '+str(count));ue.SystemLibrary.quit_editor()
