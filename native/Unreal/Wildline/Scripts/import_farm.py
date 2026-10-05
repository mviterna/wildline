"""Import CC0 textures/livestock and original collision-aware farm assets."""
import unreal as ue
from pathlib import Path
root=Path(ue.Paths.project_dir()).resolve();source=root/'ArtSource/Farm';base='/Game/Wildline/Farm'
assets=ue.AssetToolsHelpers.get_asset_tools();eal=ue.EditorAssetLibrary;mel=ue.MaterialEditingLibrary
helper=(root/'Scripts/import_forest.py').read_text()
exec(helper[helper.index('def node('):helper.index('def wind(')])
def texture(file,normal=False,linear=False):
    name='T_'+file.stem.replace('-','_');task=ue.AssetImportTask();task.filename=str(file);task.destination_path=base+'/Textures';task.destination_name=name;task.automated=True;task.replace_existing=True;task.save=True;assets.import_asset_tasks([task]);tex=eal.load_asset(base+'/Textures/'+name);assert tex,file
    if normal:tex.compression_settings=ue.TextureCompressionSettings.TC_NORMALMAP;tex.srgb=False;tex.flip_green_channel=True
    if linear:tex.srgb=False
    eal.save_loaded_asset(tex,only_if_is_dirty=False);return tex
def solid(name,rgb,rough=.85,metal=0):
    m=material(name);prop(node(m,ue.MaterialExpressionConstant3Vector,constant=ue.LinearColor(*rgb,1)),ue.MaterialProperty.MP_BASE_COLOR);constant(m,rough,ue.MaterialProperty.MP_ROUGHNESS);constant(m,metal,ue.MaterialProperty.MP_METALLIC);finish(m);return m
for name,tex in [('F_Wood','weathered_brown_planks'),('F_Brick','red_brick'),('F_Roof','roof_07'),('F_Soil','farm_soil')]:
    m=material(name);prop(sample(m,texture(source/(tex+'-diff.jpg'))),ue.MaterialProperty.MP_BASE_COLOR,'RGB');prop(sample(m,texture(source/(tex+'-nor_gl.jpg'),True)),ue.MaterialProperty.MP_NORMAL,'RGB');prop(sample(m,texture(source/(tex+'-rough.jpg'),linear=True)),ue.MaterialProperty.MP_ROUGHNESS,'R');finish(m)
m=material('F_RedWood');c=sample(m,eal.load_asset(base+'/Textures/T_weathered_brown_planks_diff'));prop(custom(m,'return C*float3(1.1,.17,.09);',{'C':(c,'RGB')}),ue.MaterialProperty.MP_BASE_COLOR);constant(m,.86,ue.MaterialProperty.MP_ROUGHNESS);finish(m)
for name,rgb,rough,metal in [('F_Trim',(.7,.66,.55),.88,0),('F_Metal',(.32,.35,.36),.48,.78),('F_Steel',(.16,.18,.19),.38,.9),('F_Hay',(.43,.29,.07),.97,0),('F_Rope',(.09,.06,.025),.95,0),('F_Jar',(.35,.24,.085),.4,0),('F_Stalk',(.16,.21,.025),.91,0),('F_Corn',(.07,.19,.018),.92,0),('F_Peanut',(.52,.30,.11),.9,0),('F_Water',(.035,.1,.1),.13,.3)]:solid(name,rgb,rough,metal)
m=solid('F_Glass',(.11,.19,.22),.08,.25);m.set_editor_property('blend_mode',ue.BlendMode.BLEND_TRANSLUCENT);m.set_editor_property('two_sided',True);constant(m,.20,ue.MaterialProperty.MP_OPACITY);finish(m)
m=eal.load_asset(base+'/Materials/F_Corn');m.set_editor_property('two_sided',True);mel.recompile_material(m);eal.save_loaded_asset(m)
m=eal.load_asset(base+'/Materials/F_Peanut');p=node(m,ue.MaterialExpressionTextureCoordinate);prop(custom(m,'float d=sin(P.x*100)*sin(P.y*75);return lerp(float3(.29,.14,.045),float3(.58,.36,.16),.6+d*.18);',{'P':p}),ue.MaterialProperty.MP_BASE_COLOR);finish(m)
for species,channel,file,norm in [
    ('Cow','Skin','cow/cow/colorOpacityCow.png','cow/cow/colorOpacityCowNorm.png'),('Cow','Eye','cow/cow/cowEye.png',''),
    ('Horse','Skin','horse/horse/HorseMain2k00.png','horse/horse/HorseMain2k00Norm00.png'),('Horse','Hair','horse/horse/Hair12Main2k.png','horse/horse/Hair12Main2kNorm.png'),('Horse','Eye','horse/horse/eye_texture.png','')]:
    m=material('F_'+species+channel);c=sample(m,texture(source/file));prop(c,ue.MaterialProperty.MP_BASE_COLOR,'RGB')
    if norm and (source/norm).exists():prop(sample(m,texture(source/norm,True)),ue.MaterialProperty.MP_NORMAL,'RGB')
    if channel=='Hair':m.set_editor_property('blend_mode',ue.BlendMode.BLEND_MASKED);m.set_editor_property('two_sided',True);prop(c,ue.MaterialProperty.MP_OPACITY_MASK,'A')
    constant(m,.78 if channel!='Eye' else .17,ue.MaterialProperty.MP_ROUGHNESS);mel.set_material_usage(m,ue.MaterialUsage.MATUSAGE_SKELETAL_MESH);finish(m)
for file in sorted((source/'Export').glob('*.fbx')):
    skeletal=file.stem.startswith('SK_');ue.SystemLibrary.execute_console_command(ue.EditorLevelLibrary.get_editor_world(),'Interchange.FeatureFlags.Import.FBX 0')
    opts=ue.FbxImportUI();opts.automated_import_should_detect_type=False;opts.import_materials=False;opts.import_textures=False;opts.import_animations=False;opts.import_as_skeletal=skeletal;opts.mesh_type_to_import=ue.FBXImportType.FBXIT_SKELETAL_MESH if skeletal else ue.FBXImportType.FBXIT_STATIC_MESH
    if skeletal:opts.skeletal_mesh_import_data.normal_import_method=ue.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS
    else:opts.static_mesh_import_data.combine_meshes=True;opts.static_mesh_import_data.auto_generate_collision=False;opts.static_mesh_import_data.one_convex_hull_per_ucx=True
    task=ue.AssetImportTask();task.factory=ue.FbxFactory();task.filename=str(file);task.destination_path=base+'/Meshes';task.destination_name=file.stem;task.options=opts;task.automated=True;task.replace_existing=True;task.replace_existing_settings=True;task.save=True;assets.import_asset_tasks([task]);mesh=eal.load_asset(base+'/Meshes/'+file.stem);assert mesh,file
    slots=mesh.get_editor_property('materials' if skeletal else 'static_materials')
    for i,slot in enumerate(slots):
        key=str(slot.material_slot_name).split('.')[0];path=('/Game/Wildline/Hunters/Materials/' if key.startswith('R_') else base+'/Materials/')+key
        mat=eal.load_asset(path);assert mat,(file,key,path)
        if skeletal:slot.set_editor_property('material_interface',mat);slots[i]=slot
        else:mesh.set_material(i,mat)
    if skeletal:mesh.set_editor_property('materials',slots)
    eal.save_loaded_asset(mesh)
levels=ue.get_editor_subsystem(ue.LevelEditorSubsystem);actors=ue.get_editor_subsystem(ue.EditorActorSubsystem)
if eal.does_asset_exist('/Game/Maps/Farmstead'):levels.load_level('/Game/Maps/Farmstead')
else:levels.new_level('/Game/Maps/Farmstead')
for actor in actors.get_all_level_actors():actors.destroy_actor(actor)
def spawn(cls,p=(0,0,0),r=(0,0,0),label=''):
    a=actors.spawn_actor_from_class(cls,ue.Vector(*p),ue.Rotator(pitch=r[0],yaw=r[1],roll=r[2]));a.set_actor_label(label);return a
farm=spawn(ue.load_class(None,'/Script/Wildline.WildlineWorld'),label='Farm Raid / playable homestead');farm.set_editor_property('bFarm',True);farm.rebuild()
spawn(ue.PlayerStart,(1250,750,320),label='Farm arrival');spawn(ue.SkyAtmosphere,label='Physical farm sky')
sun=spawn(ue.DirectionalLight,(0,0,6000),(-34,-38,0),'Late afternoon sun');sun.light_component.set_editor_property('intensity',14000.);sun.light_component.set_editor_property('light_color',ue.Color(255,231,194,255));sun.light_component.set_editor_property('atmosphere_sun_light',True);sun.light_component.set_mobility(ue.ComponentMobility.MOVABLE)
fill=spawn(ue.DirectionalLight,(0,0,7000),(-70,180,0),'Soft sky bounce');fill.light_component.set_editor_property('intensity',2100.);fill.light_component.set_editor_property('cast_shadows',False);fill.light_component.set_editor_property('light_color',ue.Color(219,233,255,255));fill.light_component.set_mobility(ue.ComponentMobility.MOVABLE)
sky=spawn(ue.SkyLight,label='Farm skylight');sky.light_component.set_editor_property('intensity',1.1);sky.light_component.set_editor_property('real_time_capture',True);sky.light_component.set_mobility(ue.ComponentMobility.MOVABLE)
fog=spawn(ue.ExponentialHeightFog,label='Distant farm haze');fog.component.set_editor_property('fog_density',.002);fog.component.set_editor_property('fog_height_falloff',.3)
vol=spawn(ue.PostProcessVolume,label='Farm photographic grade');vol.set_editor_property('unbound',True);s=vol.get_editor_property('settings')
for k,v in [('override_auto_exposure_min_brightness',True),('override_auto_exposure_max_brightness',True),('auto_exposure_min_brightness',10.),('auto_exposure_max_brightness',10.),('override_auto_exposure_apply_physical_camera_exposure',True),('auto_exposure_apply_physical_camera_exposure',False),('override_motion_blur_amount',True),('motion_blur_amount',0.),('override_bloom_intensity',True),('bloom_intensity',.18)]:s.set_editor_property(k,v)
vol.set_editor_property('settings',s)
# Warm local light helps a squirrel read the pantry under the roof.
for y in [-250,250]:
    light=spawn(ue.PointLight,(2350,y,460),label='Pantry warm lamp');light.point_light_component.set_editor_property('intensity',50000.);light.point_light_component.set_editor_property('attenuation_radius',1100.);light.point_light_component.set_editor_property('light_color',ue.Color(255,215,166,255));light.point_light_component.set_mobility(ue.ComponentMobility.MOVABLE)
levels.save_current_level();eal.save_directory(base,only_if_is_dirty=True,recursive=True);ue.log('FARM CONTENT READY');ue.SystemLibrary.quit_editor()
