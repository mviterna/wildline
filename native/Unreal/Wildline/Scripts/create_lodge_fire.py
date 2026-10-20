"""Original animated hearth shader; no third-party flame imagery or textures."""
import unreal as ue
eal=ue.EditorAssetLibrary;mel=ue.MaterialEditingLibrary
base='/Game/Wildline/Lobby/Photoreal/Materials';name='M_LodgeFire'
m=eal.load_asset(base+'/'+name) if eal.does_asset_exist(base+'/'+name) else ue.AssetToolsHelpers.get_asset_tools().create_asset(name,base,ue.Material,ue.MaterialFactoryNew())
mel.delete_all_material_expressions(m)
m.set_editor_property('blend_mode',ue.BlendMode.BLEND_ADDITIVE)
m.set_editor_property('shading_model',ue.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property('two_sided',True)
uv=mel.create_material_expression(m,ue.MaterialExpressionTextureCoordinate)
time=mel.create_material_expression(m,ue.MaterialExpressionTime)
flame=mel.create_material_expression(m,ue.MaterialExpressionCustom)
inputs=[]
for name in ['UV','T']:
    i=ue.CustomInput();i.set_editor_property('input_name',name);inputs.append(i)
flame.set_editor_property('inputs',inputs)
flame.set_editor_property('output_type',ue.CustomMaterialOutputType.CMOT_FLOAT4)
flame.set_editor_property('code',r'''
float2 p=float2(UV.x,1-UV.y);
float drift=sin(p.y*11-T*3.5)*.025+sin(p.y*23+T*2.7)*.018;
float x=p.x+drift*p.y;
float wave=sin(x*31+T*4.1)*.06+sin(x*67-T*3.2)*.035+sin(x*109+T*6.7)*.018;
float height=.25+.48*pow(saturate(1-abs(x-.5)*2),.65)+wave;
float tongue=sin(x*43-T*4+p.y*9)*.035;
float a=smoothstep(height+.07,height-.06,p.y+tongue)*smoothstep(0,.07,p.y)*smoothstep(.5,.37,abs(x-.5));
float heat=saturate(1-p.y/max(height,.01));
float3 color=lerp(float3(1,.12,.012),float3(1,.69,.15),heat);
return float4(color*250,a);
''')
mel.connect_material_expressions(uv,'',flame,'UV');mel.connect_material_expressions(time,'',flame,'T')
rgb=mel.create_material_expression(m,ue.MaterialExpressionComponentMask);rgb.set_editor_property('r',True);rgb.set_editor_property('g',True);rgb.set_editor_property('b',True);rgb.set_editor_property('a',False)
alpha=mel.create_material_expression(m,ue.MaterialExpressionComponentMask);alpha.set_editor_property('r',False);alpha.set_editor_property('g',False);alpha.set_editor_property('b',False);alpha.set_editor_property('a',True)
mel.connect_material_expressions(flame,'',rgb,'');mel.connect_material_expressions(flame,'',alpha,'')
mel.connect_material_property(rgb,'',ue.MaterialProperty.MP_EMISSIVE_COLOR);mel.connect_material_property(alpha,'',ue.MaterialProperty.MP_OPACITY)
mel.recompile_material(m);eal.save_loaded_asset(m,only_if_is_dirty=False)
ue.log('LODGE FIRE MATERIAL SAVED')
# A soft, fully faded additive halo remains visible even with bloom disabled.
glow_name='M_LodgeFireGlow'
g=eal.load_asset(base+'/'+glow_name) if eal.does_asset_exist(base+'/'+glow_name) else ue.AssetToolsHelpers.get_asset_tools().create_asset(glow_name,base,ue.Material,ue.MaterialFactoryNew())
mel.delete_all_material_expressions(g);g.set_editor_property('blend_mode',ue.BlendMode.BLEND_ADDITIVE);g.set_editor_property('shading_model',ue.MaterialShadingModel.MSM_UNLIT);g.set_editor_property('two_sided',True)
uv=mel.create_material_expression(g,ue.MaterialExpressionTextureCoordinate)
halo=mel.create_material_expression(g,ue.MaterialExpressionCustom);i=ue.CustomInput();i.set_editor_property('input_name','UV');halo.set_editor_property('inputs',[i]);halo.set_editor_property('output_type',ue.CustomMaterialOutputType.CMOT_FLOAT1)
halo.set_editor_property('code','float2 p=(UV-.5)*2; float r=dot(p,p); return exp(-r*4.5)*pow(saturate(1-r),2)*.34;')
mel.connect_material_expressions(uv,'',halo,'UV');mel.connect_material_property(halo,'',ue.MaterialProperty.MP_OPACITY)
color=mel.create_material_expression(g,ue.MaterialExpressionConstant3Vector);color.set_editor_property('constant',ue.LinearColor(105,31,4,1));mel.connect_material_property(color,'',ue.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(g);eal.save_loaded_asset(g,only_if_is_dirty=False)
ue.log('LODGE FIRE GLOW MATERIAL SAVED');ue.SystemLibrary.quit_editor()
