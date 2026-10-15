"""Build a white-hot camera filter without replacing squirrel surface materials."""
import unreal as ue

path = "/Game/Wildline/Materials/M_ThermalVision"
eal = ue.EditorAssetLibrary
mel = ue.MaterialEditingLibrary
material = eal.load_asset(path) if eal.does_asset_exist(path) else ue.AssetToolsHelpers.get_asset_tools().create_asset(
    "M_ThermalVision", "/Game/Wildline/Materials", ue.Material, ue.MaterialFactoryNew())
mel.delete_all_material_expressions(material)
material.set_editor_property("material_domain", ue.MaterialDomain.MD_POST_PROCESS)
material.set_editor_property("blendable_location", ue.BlendableLocation.BL_SCENE_COLOR_AFTER_TONEMAPPING)

output = mel.create_material_expression(material, ue.MaterialExpressionCustom, 0, 0)
output.set_editor_property("output_type", ue.CustomMaterialOutputType.CMOT_FLOAT3)
output.set_editor_property("description", "Visible living squirrels are white-hot; solid cover blocks heat")
output.set_editor_property("code", """
float cold = 0.06 + saturate(dot(Scene.rgb, float3(0.2126, 0.7152, 0.0722))) * 0.32;
float hot = (abs(Stencil.r - 1.0) < 0.1 && HeatDepth.r <= Depth.r + 2.0) ? 1.0 : 0.0;
return lerp(float3(cold, cold, cold), float3(1.0, 1.0, 1.0), hot);
""")
names = ["Scene", "Depth", "HeatDepth", "Stencil"]
inputs = []
for name in names:
    entry = ue.CustomInput()
    entry.set_editor_property("input_name", name)
    inputs.append(entry)
output.set_editor_property("inputs", inputs)
ids = [ue.SceneTextureId.PPI_POST_PROCESS_INPUT0, ue.SceneTextureId.PPI_SCENE_DEPTH,
       ue.SceneTextureId.PPI_CUSTOM_DEPTH, ue.SceneTextureId.PPI_CUSTOM_STENCIL]
for i, (name, texture_id) in enumerate(zip(names, ids)):
    texture = mel.create_material_expression(material, ue.MaterialExpressionSceneTexture, -400, i * 200)
    texture.set_editor_property("scene_texture_id", texture_id)
    mel.connect_material_expressions(texture, "Color", output, name)
mel.connect_material_property(output, "", ue.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(material)
eal.save_loaded_asset(material)
ue.log("WILDLINE THERMAL MATERIAL: SAVED")
