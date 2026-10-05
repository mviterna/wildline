import unreal as ue, json
from pathlib import Path
report=[]
for path in ue.EditorAssetLibrary.list_assets('/Game/Wildline/Forest/Meshes'):
    mesh=ue.EditorAssetLibrary.load_asset(path)
    if isinstance(mesh,ue.StaticMesh):
        slots=[]
        for m in mesh.get_editor_property('static_materials'):
            slots.append({'slot':str(m.material_slot_name),'material':m.material_interface.get_path_name() if m.material_interface else None})
        report.append({'mesh':mesh.get_name(),'slots':slots})
(Path(ue.Paths.project_dir())/'ArtSource/Forest/material-slots.json').write_text(json.dumps(report,indent=2));ue.log('FOREST MATERIAL SLOTS '+json.dumps(report))
