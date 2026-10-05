"""Run with UnrealEditor-Cmd -run=pythonscript -script=<this file>.

Imports photographed PBR textures, creates editable native materials and a lit reserve map.
The C++ world actor supplies deterministic geometry and collision during play.
"""
from pathlib import Path
import unreal as ue

root = Path(ue.Paths.project_dir()).resolve()
assets = ue.AssetToolsHelpers.get_asset_tools()
mel = ue.MaterialEditingLibrary
eal = ue.EditorAssetLibrary

def import_texture(filename, name, normal=False):
    task = ue.AssetImportTask()
    task.set_editor_property("filename", str(root / "ArtSource" / filename))
    task.set_editor_property("destination_path", "/Game/Wildline/Textures")
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    assets.import_asset_tasks([task])
    texture = eal.load_asset(f"/Game/Wildline/Textures/{name}")
    if normal:
        texture.set_editor_property("compression_settings", ue.TextureCompressionSettings.TC_NORMALMAP)
        texture.set_editor_property("srgb", False)
        texture.set_editor_property("flip_green_channel", True)
    eal.save_loaded_asset(texture)
    return texture

ground = import_texture("ground-color.jpg", "T_Ground")
ground_n = import_texture("ground-normal.jpg", "T_Ground_N", True)
bark = import_texture("bark-color.jpg", "T_Bark")
bark_n = import_texture("bark-normal.jpg", "T_Bark_N", True)
pine = import_texture("pine-color.jpg", "T_Pine")
pine_n = import_texture("pine-normal.jpg", "T_Pine_N", True)
pine_alpha = import_texture("pine-alpha.png", "T_Pine_Alpha")
pine_alpha.set_editor_property("srgb", False)
eal.save_loaded_asset(pine_alpha)

def material(name):
    path = f"/Game/Wildline/Materials/{name}"
    if eal.does_asset_exist(path):
        m = eal.load_asset(path)
        mel.delete_all_material_expressions(m)
    else:
        m = assets.create_asset(name, "/Game/Wildline/Materials", ue.Material, ue.MaterialFactoryNew())
    return m

def expr(m, cls, x=0, y=0, **properties):
    node = mel.create_material_expression(m, cls, x, y)
    for key, value in properties.items():
        node.set_editor_property(key, value)
    return node

def scalar(m, value, prop):
    mel.connect_material_property(expr(m, ue.MaterialExpressionConstant, r=value), "", prop)

def color(m, rgb, prop):
    mel.connect_material_property(expr(m, ue.MaterialExpressionConstant3Vector, constant=ue.LinearColor(*rgb)), "", prop)

def sample(m, texture, coords=None, normal=False):
    n = expr(m, ue.MaterialExpressionTextureSample, texture=texture)
    if normal:
        n.set_editor_property("sampler_type", ue.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    elif texture == pine_alpha:
        n.set_editor_property("sampler_type", ue.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    if coords:
        mel.connect_material_expressions(coords, "", n, "UVs")
    return n

def finish(m):
    mel.set_material_usage(m, ue.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)

m = material("M_Ground")
m.set_editor_property("two_sided", True)
mel.connect_material_property(sample(m, ground), "RGB", ue.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(sample(m, ground_n, normal=True), "RGB", ue.MaterialProperty.MP_NORMAL)
scalar(m, .97, ue.MaterialProperty.MP_ROUGHNESS)
finish(m)

m = material("M_Bark")
uv = expr(m, ue.MaterialExpressionTextureCoordinate, u_tiling=2., v_tiling=9.)
mel.connect_material_property(sample(m, bark, uv), "RGB", ue.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(sample(m, bark_n, uv, True), "RGB", ue.MaterialProperty.MP_NORMAL)
scalar(m, .94, ue.MaterialProperty.MP_ROUGHNESS)
finish(m)

for name, tint in [("M_Pine", (.73, .85, .65, 1.)), ("M_GroundCover", (.45, .64, .26, 1.))]:
    m = material(name)
    m.set_editor_property("two_sided", True)
    m.set_editor_property("blend_mode", ue.BlendMode.BLEND_MASKED)
    m.set_editor_property("opacity_mask_clip_value", .36)
    # Select a single photographed needle shoot from the source atlas by UV mapping.
    coordinates = expr(m, ue.MaterialExpressionTextureCoordinate, u_tiling=.17, v_tiling=.285)
    uv = expr(m, ue.MaterialExpressionAdd)
    offset = expr(m, ue.MaterialExpressionConstant2Vector, r=.045, g=.045)
    mel.connect_material_expressions(coordinates, "", uv, "A")
    mel.connect_material_expressions(offset, "", uv, "B")
    c = sample(m, pine, uv)
    multiply = expr(m, ue.MaterialExpressionMultiply)
    mel.connect_material_expressions(c, "RGB", multiply, "A")
    tint_node = expr(m, ue.MaterialExpressionConstant3Vector, constant=ue.LinearColor(*tint))
    mel.connect_material_expressions(tint_node, "", multiply, "B")
    mel.connect_material_property(multiply, "", ue.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(sample(m, pine_alpha, uv), "R", ue.MaterialProperty.MP_OPACITY_MASK)
    mel.connect_material_property(sample(m, pine_n, uv, True), "RGB", ue.MaterialProperty.MP_NORMAL)
    scalar(m, .92, ue.MaterialProperty.MP_ROUGHNESS)
    finish(m)

for name, rgb, rough in [
    ("M_Rock", (.23,.25,.22,1), .95),
    ("M_Belly", (.58,.47,.32,1), .99),
    ("M_Eye", (.009,.012,.006,1), .18),
    ("M_Blood", (.18,.006,.004,1), .28),
]:
    m = material(name)
    color(m, rgb, ue.MaterialProperty.MP_BASE_COLOR)
    scalar(m, rough, ue.MaterialProperty.MP_ROUGHNESS)
    finish(m)

m = material("M_Fur")
color(m, (.23,.11,.045,1), ue.MaterialProperty.MP_BASE_COLOR)
scalar(m, 1., ue.MaterialProperty.MP_ROUGHNESS)
uv = expr(m, ue.MaterialExpressionTextureCoordinate, u_tiling=4., v_tiling=9.)
mel.connect_material_property(sample(m, bark_n, uv, True), "RGB", ue.MaterialProperty.MP_NORMAL)
finish(m)

m = material("M_Heat")
color(m, (1.,.88,.63,1), ue.MaterialProperty.MP_BASE_COLOR)
color(m, (3.,2.7,1.8,1), ue.MaterialProperty.MP_EMISSIVE_COLOR)
finish(m)

levels = ue.get_editor_subsystem(ue.LevelEditorSubsystem)
actors = ue.get_editor_subsystem(ue.EditorActorSubsystem)
if eal.does_asset_exist("/Game/Maps/Blackwood"):
    levels.load_level("/Game/Maps/Blackwood")
    generated_labels = {
        "Blackwood / deterministic native forest", "Camp spawn", "Physical atmosphere",
        "Early autumn sun", "Sky fill / Lumen", "Low woodland haze", "Forest photographic grade", "Canopy ambient fill"
    }
    for old_actor in actors.get_all_level_actors():
        if old_actor.get_actor_label() in generated_labels:
            actors.destroy_actor(old_actor)
else:
    levels.new_level("/Game/Maps/Blackwood")

def spawn(cls, location=(0,0,0), rotation=(0,0,0), label=None):
    actor = actors.spawn_actor_from_class(cls, ue.Vector(*location), ue.Rotator(pitch=rotation[0], yaw=rotation[1], roll=rotation[2]))
    if label:
        actor.set_actor_label(label)
    return actor

forest = spawn(ue.load_class(None, "/Script/Wildline.WildlineWorld"), label="Blackwood / deterministic native forest")
if hasattr(forest, "rebuild"):
    forest.rebuild()
spawn(ue.PlayerStart, (0,750,420), (0,0,0), "Camp spawn")
spawn(ue.SkyAtmosphere, label="Physical atmosphere")
sun = spawn(ue.DirectionalLight, (0,0,6000), (-23,-38,0), "Early autumn sun")
sun.light_component.set_editor_property("intensity", 12000.)
sun.light_component.set_editor_property("light_color", ue.Color(255,225,177,255))
sun.light_component.set_editor_property("atmosphere_sun_light", True)
sun.light_component.set_editor_property("cast_shadows", True)
sun.light_component.set_editor_property("forward_shading_priority", 1)
sun.light_component.set_editor_property("volumetric_scattering_intensity", 1.25)
sun.light_component.set_mobility(ue.ComponentMobility.MOVABLE)

fill = spawn(ue.DirectionalLight, (0,0,7000), (-70,180,0), "Canopy ambient fill")
fill.light_component.set_editor_property("intensity", 3500.)
fill.light_component.set_editor_property("cast_shadows", False)
fill.light_component.set_editor_property("light_color", ue.Color(223,234,255,255))
fill.light_component.set_mobility(ue.ComponentMobility.MOVABLE)

sky = spawn(ue.SkyLight, (0,0,4000), label="Sky fill / Lumen")
sky.light_component.set_editor_property("intensity", .85)
sky.light_component.set_editor_property("real_time_capture", True)
sky.light_component.set_mobility(ue.ComponentMobility.MOVABLE)

fog = spawn(ue.ExponentialHeightFog, (0,0,0), label="Low woodland haze")
fog.component.set_editor_property("fog_density", .004)
fog.component.set_editor_property("fog_height_falloff", .27)
fog.component.set_editor_property("enable_volumetric_fog", True)
fog.component.set_editor_property("volumetric_fog_albedo", ue.Color(220,231,211,255))

volume = spawn(ue.PostProcessVolume, label="Forest photographic grade")
volume.set_editor_property("unbound", True)
settings = volume.get_editor_property("settings")
settings.set_editor_property("override_auto_exposure_method", True)
settings.set_editor_property("auto_exposure_method", ue.AutoExposureMethod.AEM_HISTOGRAM)
settings.set_editor_property("override_auto_exposure_min_brightness", True)
settings.set_editor_property("override_auto_exposure_max_brightness", True)
settings.set_editor_property("auto_exposure_min_brightness", 10.)
settings.set_editor_property("auto_exposure_max_brightness", 10.)
settings.set_editor_property("override_auto_exposure_bias", True)
settings.set_editor_property("auto_exposure_bias", 0.0)
settings.set_editor_property("override_auto_exposure_apply_physical_camera_exposure", True)
settings.set_editor_property("auto_exposure_apply_physical_camera_exposure", False)
settings.set_editor_property("override_bloom_intensity", True)
settings.set_editor_property("bloom_intensity", .22)
settings.set_editor_property("override_vignette_intensity", True)
settings.set_editor_property("vignette_intensity", .22)
settings.set_editor_property("override_motion_blur_amount", True)
settings.set_editor_property("motion_blur_amount", 0.)
volume.set_editor_property("settings", settings)

levels.save_current_level()
eal.save_directory("/Game/Wildline", only_if_is_dirty=False, recursive=True)
ue.log("WILDLINE CONTENT READY: /Game/Maps/Blackwood")
