"""Author the Knobs State Forest map using retained WILDLINE forest assets."""
from pathlib import Path
import unreal as ue

root = Path(ue.Paths.project_dir()).resolve()
assets = ue.AssetToolsHelpers.get_asset_tools()
eal = ue.EditorAssetLibrary
mel = ue.MaterialEditingLibrary
base = '/Game/Wildline/Knobs'
helpers = (root / 'Scripts/import_forest.py').read_text()
exec(helpers[helpers.index('def node('):helpers.index('for name,dry in')])

for name, color in [
    ('M_KnobsFloor', (.32, .25, .12)),
    ('M_KnobsDirt', (.22, .13, .055)), ('M_KnobsGravel', (.32, .29, .23)),
    ('M_KnobsMeadow', (.28, .32, .105)), ('M_KnobsSign', (.028, .065, .036)),
    ('M_KnobsRed', (.65, .035, .018)), ('M_KnobsBlue', (.025, .24, .85)),
    ('M_KnobsOrange', (.95, .25, .015)), ('M_KnobsYellow', (.95, .72, .025)),
    ('M_KnobsWater', (.08, .16, .14)),
]:
    if '-WildlineKnobsLettersOnly' in ue.SystemLibrary.get_command_line():
        continue
    m = material(name)
    mel.delete_all_material_expressions(m)
    m.set_editor_property('two_sided', True)
    p = node(m, ue.MaterialExpressionWorldPosition)
    if name in ('M_KnobsFloor', 'M_KnobsDirt', 'M_KnobsGravel', 'M_KnobsMeadow'):
        uv = custom(m, 'return P.xy/150;', {'P': p}, ue.CustomMaterialOutputType.CMOT_FLOAT2)
        source = sample(m, eal.load_asset('/Game/Wildline/Forest/Textures/T_Litter'), uv)
        code = 'float grain=dot(C,float3(.3,.59,.11)); float macro=.82+.18*sin(P.x*.0012)*cos(P.y*.0015); return float3(%s)*lerp(.52,1.45,grain)*macro;' % ','.join(map(str, color))
        prop(custom(m, code, {'C': (source, 'RGB'), 'P': p}), ue.MaterialProperty.MP_BASE_COLOR)
        normal = sample(m, eal.load_asset('/Game/Wildline/Forest/Textures/T_LitterNormal'), uv)
        prop(normal, ue.MaterialProperty.MP_NORMAL, 'RGB')
        if name != 'M_KnobsFloor':
            m.set_editor_property('blend_mode', ue.BlendMode.BLEND_MASKED)
            uv0 = node(m, ue.MaterialExpressionTextureCoordinate)
            edge = custom(m, 'float edge=min(U.x,1-U.x); float noise=sin(P.x*.043)*sin(P.y*.039)*.045; return smoothstep(.005,.10,edge+noise);', {'U': uv0, 'P': p}, ue.CustomMaterialOutputType.CMOT_FLOAT1)
            prop(edge, ue.MaterialProperty.MP_OPACITY_MASK)
    else:
        prop(node(m, ue.MaterialExpressionConstant3Vector, constant=ue.LinearColor(*color)), ue.MaterialProperty.MP_BASE_COLOR)
    constant(m, .19 if name == 'M_KnobsWater' else .92, ue.MaterialProperty.MP_ROUGHNESS)
    if name == 'M_KnobsWater':
        t = node(m, ue.MaterialExpressionTime)
        uv = custom(m, 'return P.xy/360+float2(T*.022,T*.004);', {'P': p, 'T': t}, ue.CustomMaterialOutputType.CMOT_FLOAT2)
        normal = sample(m, eal.load_asset('/Game/CityPark/Textures/BaseTextures/T_Water_N'), uv)
        prop(normal, ue.MaterialProperty.MP_NORMAL, 'RGB')
    mel.set_material_usage(m, ue.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    mel.recompile_material(m)
    assert eal.save_loaded_asset(m, only_if_is_dirty=False)

m = material('M_KnobsLetters')
mel.delete_all_material_expressions(m)
m.set_editor_property('shading_model', ue.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property('blend_mode', ue.BlendMode.BLEND_MASKED)
m.set_editor_property('two_sided', True)
font = eal.load_asset('/Engine/EngineFonts/RobotoDistanceField')
assert font
glyph = node(m, ue.MaterialExpressionFontSampleParameter, font=font, parameter_name='Font')
# RobotoDistanceField stores its signed distance in R, matching Unreal's default text material.
prop(custom(m, 'return lerp(-90.0,90.0,D);', {'D': (glyph, 'R')}, ue.CustomMaterialOutputType.CMOT_FLOAT1), ue.MaterialProperty.MP_OPACITY_MASK)
text_color = node(m, ue.MaterialExpressionVertexColor)
gain = node(m, ue.MaterialExpressionMultiply, const_b=400)
connect(text_color, gain, 'A')
assert mel.connect_material_property(gain, '', ue.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(m)
assert eal.save_loaded_asset(m, only_if_is_dirty=False)

if any(flag in ue.SystemLibrary.get_command_line() for flag in ['-WildlineKnobsMaterialsOnly', '-WildlineKnobsLettersOnly']):
    ue.log('KNOBS MATERIALS IMPORT PASS')
else:
    levels = ue.get_editor_subsystem(ue.LevelEditorSubsystem)
    actors = ue.get_editor_subsystem(ue.EditorActorSubsystem)
    path = '/Game/Maps/KnobsStateForest'
    assert levels.load_level(path) if eal.does_asset_exist(path) else levels.new_level_from_template(path, '/Game/Maps/Blackwood')
    found = 0
    for a in actors.get_all_level_actors():
        if a.get_class().get_name() == 'WildlineWorld':
            for key in ['bLake', 'bWinter', 'bFarm', 'bResidential']:
                a.set_editor_property(key, False)
            a.set_editor_property('bKnobs', True)
            a.set_actor_label('Knobs State Forest / Crooked Creek and Hay Ridge')
            a.rebuild()
            found += 1
        if isinstance(a, ue.ExponentialHeightFog):
            a.component.set_editor_property('fog_density', .008)
        if isinstance(a, ue.DirectionalLight) and a.get_actor_label() == 'Early autumn sun':
            a.set_actor_rotation(ue.Rotator(pitch=-42, yaw=-48), False)
            a.light_component.set_intensity(12000)
    assert found == 1, found
    assert levels.save_current_level()
    ue.log('KNOBS STATE FOREST IMPORT PASS')
ue.SystemLibrary.quit_editor()
