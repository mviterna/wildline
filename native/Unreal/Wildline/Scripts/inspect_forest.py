import bpy, json, sys
from pathlib import Path
source=Path(sys.argv[sys.argv.index('--')+1])
reports=[]
for file in source.rglob('*.blend'):
    bpy.ops.wm.open_mainfile(filepath=str(file),use_scripts=False)
    report={'file':str(file),'unit':bpy.context.scene.unit_settings.scale_length,'objects':[],'images':[]}
    for o in bpy.data.objects:
        report['objects'].append({'name':o.name,'type':o.type,'dimensions':list(o.dimensions),'location':list(o.location),'vertices':len(o.data.vertices) if o.type=='MESH' else 0,'polygons':len(o.data.polygons) if o.type=='MESH' else 0,'materials':[m.name for m in o.data.materials] if hasattr(o.data,'materials') else [],'modifiers':[(m.name,m.type) for m in o.modifiers],'particles':[{'name':p.settings.name,'object':p.settings.instance_object.name if p.settings.instance_object else None,'count':p.settings.count} for p in o.particle_systems]})
    report['images']=[{'name':i.name,'size':list(i.size),'packed':bool(i.packed_file),'path':i.filepath} for i in bpy.data.images]
    reports.append(report)
(source/'inspection.json').write_text(json.dumps(reports,indent=2))
print('FOREST_INSPECTION '+json.dumps(reports))
