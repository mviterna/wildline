"""Save Nanite material permutations before cooking the free lodge assets."""
import unreal as ue
eal=ue.EditorAssetLibrary;mel=ue.MaterialEditingLibrary
base='/Game/Wildline/Lobby/Photoreal/Materials'
count=0
for path in eal.list_assets(base,recursive=False):
    material=eal.load_asset(path)
    if isinstance(material,ue.Material) and material.get_editor_property('blend_mode')!=ue.BlendMode.BLEND_TRANSLUCENT:
        mel.set_material_usage(material,ue.MaterialUsage.MATUSAGE_NANITE)
        mel.recompile_material(material)
        eal.save_loaded_asset(material,only_if_is_dirty=False)
        count+=1
ue.log('LODGE NANITE MATERIALS SAVED '+str(count))
ue.SystemLibrary.quit_editor()
