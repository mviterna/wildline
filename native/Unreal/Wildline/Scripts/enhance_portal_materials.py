"""Build green and amber ready-pad materials with a radial light shader."""
import unreal as ue
eal=ue.EditorAssetLibrary;mel=ue.MaterialEditingLibrary
for name,tint in [('M_PortalGreen',(.13,.62,.37)),('M_PortalAmber',(.55,.31,.12))]:
    m=eal.load_asset('/Game/Wildline/Lobby/'+name);assert m,name
    mel.delete_all_material_expressions(m)
    local=mel.create_material_expression(m,ue.MaterialExpressionLocalPosition)
    time=mel.create_material_expression(m,ue.MaterialExpressionTime)
    color=mel.create_material_expression(m,ue.MaterialExpressionConstant3Vector);color.set_editor_property('constant',ue.LinearColor(*tint,1))
    wave=mel.create_material_expression(m,ue.MaterialExpressionCustom);wave.set_editor_property('output_type',ue.CustomMaterialOutputType.CMOT_FLOAT3)
    ins=[]
    for key in ['P','T','C']:
        i=ue.CustomInput();i.set_editor_property('input_name',key);ins.append(i)
    wave.set_editor_property('inputs',ins)
    wave.set_editor_property('code','float2 p=P.xy/50; float r=length(p); float a=atan2(p.y,p.x); float ring=pow(abs(sin(r*34-T*1.2)),18); float rim=smoothstep(.84,.96,r); float mist=.5+.5*sin(a*5+r*18-T*.7); float rune=pow(abs(sin(a*14+T*.08)),30)*smoothstep(.55,.70,r)*(1-smoothstep(.78,.84,r)); return C*(.04+ring*.65+rim*1.1+rune*.8+mist*.1)*9;')
    for n,out,key in [(local,'','P'),(time,'','T'),(color,'','C')]:assert mel.connect_material_expressions(n,out,wave,key)
    mel.connect_material_property(wave,'',ue.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.connect_material_property(color,'',ue.MaterialProperty.MP_BASE_COLOR)
    rough=mel.create_material_expression(m,ue.MaterialExpressionConstant);rough.set_editor_property('r',.34);mel.connect_material_property(rough,'',ue.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(m);eal.save_loaded_asset(m,only_if_is_dirty=False)
ue.log('LOBBY PORTAL MATERIALS ENHANCED');ue.SystemLibrary.quit_editor()
