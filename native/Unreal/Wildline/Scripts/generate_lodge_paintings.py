"""Frame the retained public-domain Herring paintings with original gilt mouldings."""
import bpy,json
from pathlib import Path
helper=Path(__file__).with_name('convert_lobby_assets.py').read_text()
exec(helper[:helper.index('import sys\nif')])
REPORT=json.loads((ROOT/'model-index.json').read_text());MATERIALS=json.loads((ROOT/'material-index.json').read_text())
sources=json.loads((ROOT/'Paintings/sources.json').read_text())
names=['SM_Painting'+s['key'] for s in sources];REPORT=[r for r in REPORT if r['name'] not in names]
for source in sources:
    reset();key=source['key'];w=1.58;h=w*source['height']/source['width']
    gold=bpy.data.materials.new('M_PaintingGilt');MATERIALS[gold.name]={'factor':[.48,.29,.085,1],'roughness':.30,'metallic':.82}
    wood=mat('M_PaintingBacking','dark_wooden_planks')
    objects=[box('Oak backing',(-.005,0,0),(.035,w+.23,h+.23),wood,.008)]
    for inset,thickness,depth in [(0,.055,.050),(.060,.025,.074),(.094,.016,.059),(.116,.020,.043)]:
        wy=w+.23-inset*2;hz=h+.23-inset*2
        for side in [-1,1]:
            objects.append(box('Mitred gilt moulding',(depth,side*(wy/2-thickness/2),0),(.034,thickness,hz),gold,.009))
            objects.append(box('Mitred gilt moulding',(depth,0,side*(hz/2-thickness/2)),(.034,wy,thickness),gold,.009))
    canvas=bpy.data.materials.new('M_Painting'+key)
    MATERIALS[canvas.name]={'color':'Paintings/'+key+'.jpg','factor':[1,1,1,1],'roughness':.88,'metallic':0,'source':source['source']}
    mesh=bpy.data.meshes.new('Oil painting on panel');mesh.from_pydata([(.045,-w/2,-h/2),(.045,w/2,-h/2),(.045,w/2,h/2),(.045,-w/2,h/2)],[],[(0,1,2,3)]);mesh.materials.append(canvas);mesh.update()
    uv=mesh.uv_layers.new(name='Complete museum image')
    for loop,coord in zip(uv.data,[(0,0),(1,0),(1,1),(0,1)]):loop.uv=coord
    panel=bpy.data.objects.new('Original oil on panel reproduction',mesh);bpy.context.collection.objects.link(panel);objects.append(panel)
    export('SM_Painting'+key,objects,False)
(ROOT/'model-index.json').write_text(json.dumps(REPORT,indent=2));(ROOT/'material-index.json').write_text(json.dumps(MATERIALS,indent=2))
print('LODGE PAINTING FRAMES READY',flush=True)
