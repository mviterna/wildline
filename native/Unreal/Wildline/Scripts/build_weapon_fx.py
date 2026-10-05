"""Editable native materials for smoke, muzzle flash, sparks and impact decals."""
import unreal as ue
from pathlib import Path
root=Path(ue.Paths.project_dir()).resolve()
assets=ue.AssetToolsHelpers.get_asset_tools();eal=ue.EditorAssetLibrary;mel=ue.MaterialEditingLibrary;base='/Game/Wildline/FX'
helper=(root/'Scripts/import_forest.py').read_text();exec(helper[helper.index('def node('):helper.index('def wind(')])
def parameter(m,name,value):return node(m,ue.MaterialExpressionScalarParameter,parameter_name=name,default_value=value)
for name in ['M_Smoke','M_Flash','M_Spark','M_Chip','M_Impact']:
    path=base+'/Materials/'+name
    # These paths deliberately match the runtime FX folder (no Materials child).
    path=base+'/'+name
    m=eal.load_asset(path) if eal.does_asset_exist(path) else assets.create_asset(name,base,ue.Material,ue.MaterialFactoryNew())
    mel.delete_all_material_expressions(m);uv=node(m,ue.MaterialExpressionTextureCoordinate)
    tint=node(m,ue.MaterialExpressionVectorParameter,parameter_name='Tint',default_value=ue.LinearColor(.35,.36,.37,1));opacity=parameter(m,'Opacity',1);seed=parameter(m,'Seed',0)
    m.set_editor_property('two_sided',True)
    if name=='M_Impact':
        m.set_editor_property('material_domain',ue.MaterialDomain.MD_DEFERRED_DECAL);m.set_editor_property('blend_mode',ue.BlendMode.BLEND_TRANSLUCENT)
        mask=custom(m,'float2 p=UV*2-1;float r=length(p);float ragged=.025*sin(atan2(p.y,p.x)*11);return saturate((.82-r+ragged)*7)*O;',{'UV':uv,'O':opacity},ue.CustomMaterialOutputType.CMOT_FLOAT1)
        color=custom(m,'float r=length(UV*2-1);return lerp(float3(.012,.009,.006),float3(.11,.078,.035),smoothstep(.2,.8,r));',{'UV':uv})
        prop(color,ue.MaterialProperty.MP_BASE_COLOR);prop(mask,ue.MaterialProperty.MP_OPACITY);constant(m,.94,ue.MaterialProperty.MP_ROUGHNESS)
    elif name=='M_Chip':prop(tint,ue.MaterialProperty.MP_BASE_COLOR);constant(m,.85,ue.MaterialProperty.MP_ROUGHNESS)
    elif name=='M_Spark':
        m.set_editor_property('blend_mode',ue.BlendMode.BLEND_ADDITIVE);m.set_editor_property('shading_model',ue.MaterialShadingModel.MSM_UNLIT);prop(tint,ue.MaterialProperty.MP_EMISSIVE_COLOR);prop(opacity,ue.MaterialProperty.MP_OPACITY)
    else:
        m.set_editor_property('blend_mode',ue.BlendMode.BLEND_ADDITIVE if name=='M_Flash' else ue.BlendMode.BLEND_TRANSLUCENT)
        m.set_editor_property('shading_model',ue.MaterialShadingModel.MSM_UNLIT if name=='M_Flash' else ue.MaterialShadingModel.MSM_DEFAULT_LIT)
        code='float2 p=UV*2-1;float r=length(p);float n=sin(p.x*11+S)*sin(p.y*13+S*.7);return pow(saturate(1-r),1.3)*(0.7+n*.22)*O;'
        mask=custom(m,code,{'UV':uv,'O':opacity,'S':seed},ue.CustomMaterialOutputType.CMOT_FLOAT1)
        prop(tint,ue.MaterialProperty.MP_EMISSIVE_COLOR if name=='M_Flash' else ue.MaterialProperty.MP_BASE_COLOR);prop(mask,ue.MaterialProperty.MP_OPACITY)
        if name=='M_Smoke':constant(m,1,ue.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(m);eal.save_loaded_asset(m,only_if_is_dirty=False)
ue.log('WEAPON FX MATERIALS READY');ue.SystemLibrary.quit_editor()
