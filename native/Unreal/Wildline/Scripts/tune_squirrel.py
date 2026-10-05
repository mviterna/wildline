"""Create PBR fur and eye materials for the imported, centimetre-scale rig."""
import unreal as ue
eal=ue.EditorAssetLibrary
mel=ue.MaterialEditingLibrary
assets=ue.AssetToolsHelpers.get_asset_tools()
folder='/Game/Wildline/Squirrel'
def material(name):
    path=folder+'/'+name
    m=eal.load_asset(path) if eal.does_asset_exist(path) else assets.create_asset(name,folder,ue.Material,ue.MaterialFactoryNew())
    # Retain previous unused nodes when retuning: UE 5.8 can root custom nodes
    # during shader compilation, making DeleteAllMaterialExpressions unsafe.
    mel.set_material_usage(m,ue.MaterialUsage.MATUSAGE_SKELETAL_MESH)
    return m
def node(m,cls,**kwargs):
    n=mel.create_material_expression(m,cls)
    for k,v in kwargs.items():n.set_editor_property(k,v)
    return n
def scalar(m,value,prop):mel.connect_material_property(node(m,ue.MaterialExpressionConstant,r=value),'',prop)
def texture(m,name):return node(m,ue.MaterialExpressionTextureSample,texture=eal.load_asset(folder+'/'+name))
fur=material('M_SquirrelCoat')
c=texture(fur,'Squirrel')
# Retain the publisher's varied grey/brown guard hair and pale belly.
desat=node(fur,ue.MaterialExpressionDesaturation)
mel.connect_material_expressions(c,'RGB',desat,'')
fraction=node(fur,ue.MaterialExpressionConstant,r=.28)
mel.connect_material_expressions(fraction,'',desat,'Fraction')
mel.connect_material_property(desat,'',ue.MaterialProperty.MP_BASE_COLOR)
scalar(fur,.89,ue.MaterialProperty.MP_ROUGHNESS)
scalar(fur,.24,ue.MaterialProperty.MP_SPECULAR)
# Derive only fine micro relief from the same UV photograph, never a bark normal.
uv=node(fur,ue.MaterialExpressionTextureCoordinate)
offset=node(fur,ue.MaterialExpressionAdd)
step=node(fur,ue.MaterialExpressionConstant2Vector,r=1/2048,g=0)
mel.connect_material_expressions(uv,'',offset,'A');mel.connect_material_expressions(step,'',offset,'B')
cx=texture(fur,'Squirrel');mel.connect_material_expressions(offset,'',cx,'UVs')
offsety=node(fur,ue.MaterialExpressionAdd)
stepy=node(fur,ue.MaterialExpressionConstant2Vector,r=0,g=1/2048)
mel.connect_material_expressions(uv,'',offsety,'A');mel.connect_material_expressions(stepy,'',offsety,'B')
cy=texture(fur,'Squirrel');mel.connect_material_expressions(offsety,'',cy,'UVs')
custom=node(fur,ue.MaterialExpressionCustom,code='float3 w=float3(0.299,0.587,0.114); return normalize(float3((dot(C,w)-dot(X,w))*1.6,(dot(C,w)-dot(Y,w))*1.6,1));',output_type=ue.CustomMaterialOutputType.CMOT_FLOAT3)
inputs=[]
for name in ['C','X','Y']:
    entry=ue.CustomInput();entry.set_editor_property('input_name',name);inputs.append(entry)
custom.set_editor_property('inputs',inputs)
for tex,pin in [(c,'C'),(cx,'X'),(cy,'Y')]:mel.connect_material_expressions(tex,'RGB',custom,pin)
mel.connect_material_property(custom,'',ue.MaterialProperty.MP_NORMAL)
mel.recompile_material(fur);eal.save_loaded_asset(fur)
eye=material('M_SquirrelCornea')
mel.connect_material_property(texture(eye,'Squirrel_Eye'),'RGB',ue.MaterialProperty.MP_BASE_COLOR)
scalar(eye,.10,ue.MaterialProperty.MP_ROUGHNESS);scalar(eye,.65,ue.MaterialProperty.MP_SPECULAR)
mel.recompile_material(eye);eal.save_loaded_asset(eye)
mesh=eal.load_asset(folder+'/SK_Squirrel')
ue.WildlineSquirrelAnim.prepare_mesh(mesh)
slots=list(mesh.get_editor_property('materials'))
for slot in slots:slot.set_editor_property('material_interface',eye if str(slot.material_slot_name)=='Eye' else fur)
mesh.set_editor_property('materials',slots)
# The tail curls above its imported flat rest bounds. Avoid culling it mid-bound.
mesh.set_editor_property('positive_bounds_extension',ue.Vector(4,8,28))
mesh.set_editor_property('negative_bounds_extension',ue.Vector(4,8,8))
eal.save_loaded_asset(mesh)
strands=material('M_SquirrelStrands');strands.set_editor_property('two_sided',True)
mel.connect_material_property(texture(strands,'Squirrel'),'RGB',ue.MaterialProperty.MP_BASE_COLOR)
scalar(strands,.92,ue.MaterialProperty.MP_ROUGHNESS);scalar(strands,.22,ue.MaterialProperty.MP_SPECULAR)
mel.recompile_material(strands);eal.save_loaded_asset(strands)
# Keep the superseded first material cookable too; it is no longer assigned.
if eal.does_asset_exist(folder+'/M_SquirrelFur'):
    old=eal.load_asset(folder+'/M_SquirrelFur')
    mel.connect_material_property(texture(old,'Squirrel'),'RGB',ue.MaterialProperty.MP_BASE_COLOR)
    normal=node(old,ue.MaterialExpressionConstant3Vector,constant=ue.LinearColor(0,0,1,1))
    mel.connect_material_property(normal,'',ue.MaterialProperty.MP_NORMAL)
    mel.recompile_material(old);eal.save_loaded_asset(old)
ue.log('SQUIRREL PBR MATERIALS AND ANIMATION BOUNDS SAVED')
