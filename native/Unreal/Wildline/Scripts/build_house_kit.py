"""Convert licensed burnedhrum kit into assembled, floor-centred presets.

Source: https://sketchfab.com/3d-models/7dfa7d64895d4b1394162401366e806a
CC BY 4.0, burnedhrum. Groups preserve the author's example assemblies.
"""
from pathlib import Path
builder=Path(__file__).with_name('build_neighborhood_assets.py')
exec(compile(builder.read_text().split("if '--soil-only' in sys.argv:")[0],str(builder),'exec'))
MATERIALS=json.loads((ROOT/'material-index.json').read_text())
REPORT=[r for r in json.loads((ROOT/'model-index.json').read_text()) if not r['name'].startswith('SM_Kit')]
reset();file=ROOT/'ModularHouseKitGLTF/scene.gltf';data=json.loads(file.read_text());bpy.ops.import_scene.gltf(filepath=str(file))
def image(t):return str((file.parent/data['images'][data['textures'][t['index']]['source']]['uri']).relative_to(ROOT)) if t else None
for i,s in enumerate(data['materials']):
    original=s['name'];matches=[m for m in bpy.data.materials if m.name==original or m.name.startswith(original+'.')];m=matches[0]
    for o in bpy.context.scene.objects:
        if o.type=='MESH':
            for slot in o.material_slots:
                if slot.material in matches:slot.material=m
    key='M_NH_HouseKit_'+str(i);m.name=key;p=s.get('pbrMetallicRoughness',{})
    MATERIALS[key]={'color':image(p.get('baseColorTexture')),'normal':image(s.get('normalTexture')),'arm':image(p.get('metallicRoughnessTexture')),'factor':p.get('baseColorFactor',[1,1,1,1]),'roughness':p.get('roughnessFactor',1),'metallic':p.get('metallicFactor',1),'alpha':s.get('alphaMode','OPAQUE'),'cutoff':s.get('alphaCutoff',.5),'two_sided':s.get('doubleSided',False),'source':'https://sketchfab.com/3d-models/7dfa7d64895d4b1394162401366e806a'}
groups={
    'SM_KitGardenHouse':['SM_Wall_House','SM_Roof_House','SM_Concrete_Floor_House'],
    'SM_KitTwoStoryHouse':['SM_2storyhouse','SM_Roof_Edge_3m001','SM_Concrete_Floor_2x003'],
    'SM_KitCottage':['SM_House_05_Walls','SM_Roof_Edge_4m003'],
    'SM_KitGarage':['SM_Garage_Walls','SM_Roof'],
    'SM_KitPool':['SM_Pool_Example'],
    'SM_KitMailbox':['SM_MailBox'],
    'SM_KitSwing':['SM_Swing'],
    'SM_KitLounge':['SM_Lounge']}
for name,parents in groups.items():
    objects=[]
    for parent in parents:
        o=bpy.data.objects.get(parent);assert o and o.type=='EMPTY',parent
        objects.extend(p for p in o.children_recursive if p.type=='MESH')
    export(name,objects,True)
(ROOT/'model-index.json').write_text(json.dumps(REPORT,indent=2));(ROOT/'material-index.json').write_text(json.dumps(MATERIALS,indent=2))
print('LICENSED HOUSE KIT READY',len(groups),'presets',flush=True)
