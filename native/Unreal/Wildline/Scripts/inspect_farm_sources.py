import bpy,json
from pathlib import Path
from mathutils import Vector
root=Path(r'C:/Users/mvite/OneDrive/Documents/ChatGPT/PhotoRealistic Multiplater/native/Unreal/Wildline/ArtSource/Farm')
for species,file in [('cow',root/'cow/cow/cowTM08New00RTime02.obj'),('horse',root/'horse/horse/LD_HorseRtime02.obj')]:
    bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.wm.obj_import(filepath=str(file))
    for o in bpy.context.scene.objects:
        if o.type=='MESH':
            print('WORLD_BOUNDS',species,o.name,list(o.rotation_euler),[(min((o.matrix_world@v.co)[j] for v in o.data.vertices),max((o.matrix_world@v.co)[j] for v in o.data.vertices)) for j in range(3)])
