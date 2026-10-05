"""Rebuild the saved reserve with grounded vegetation and a composed perimeter."""
import unreal as ue
from pathlib import Path
root=Path(ue.Paths.project_dir()).resolve()
assets=ue.AssetToolsHelpers.get_asset_tools();eal=ue.EditorAssetLibrary;mel=ue.MaterialEditingLibrary
base='/Game/Wildline/Hunters'
helper=(root/'Scripts/import_forest.py').read_text()
exec(helper[helper.index('def node('):helper.index('for name,dry in')])
hair=material('M_Hair');hair.set_editor_property('blend_mode',ue.BlendMode.BLEND_MASKED)
hair.set_editor_property('two_sided',True);hair.set_editor_property('opacity_mask_clip_value',.2)
uv=node(hair,ue.MaterialExpressionTextureCoordinate,coordinate_index=1)
c=sample(hair,eal.load_asset(base+'/Textures/T_short01_diffuse'),uv)
t=node(hair,ue.MaterialExpressionVectorParameter,parameter_name='HairTint',default_value=ue.LinearColor(.35,.23,.13,1))
cap=node(hair,ue.MaterialExpressionScalarParameter,parameter_name='CapVisible',default_value=0.)
crop=node(hair,ue.MaterialExpressionScalarParameter,parameter_name='CloseCrop',default_value=0.)
prop(custom(hair,'float g=dot(C,float3(.3,.59,.11));return T*lerp(.32,.9,saturate(g*6));',{'C':(c,'RGB'),'T':(t,'RGB')}),ue.MaterialProperty.MP_BASE_COLOR)
prop(custom(hair,'return lerp(A,1,H)*(1-C);',{'A':(c,'A'),'C':cap,'H':crop},ue.CustomMaterialOutputType.CMOT_FLOAT1),ue.MaterialProperty.MP_OPACITY_MASK)
constant(hair,.92,ue.MaterialProperty.MP_ROUGHNESS);constant(hair,.18,ue.MaterialProperty.MP_SPECULAR)
mel.set_material_usage(hair,ue.MaterialUsage.MATUSAGE_SKELETAL_MESH);mel.set_material_usage(hair,ue.MaterialUsage.MATUSAGE_MORPH_TARGETS)
mel.recompile_material(hair);eal.save_loaded_asset(hair)
levels=ue.get_editor_subsystem(ue.LevelEditorSubsystem)
levels.load_level('/Game/Maps/Blackwood')
actors=ue.get_editor_subsystem(ue.EditorActorSubsystem)
for actor in actors.get_all_level_actors():
    if actor.get_class().get_name()=='WildlineWorld':
        actor.rebuild()
        ue.log('POLISH: rebuilt roots, reserve fence and surrounding ridges')
assert levels.save_current_level()
ue.SystemLibrary.quit_editor()
