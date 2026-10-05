import unreal as ue
from pathlib import Path
root=Path(ue.Paths.project_dir()).resolve()/'ArtSource/Hunters'
root.mkdir(exist_ok=True)
t=ue.AssetExportTask();t.object=ue.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple');t.filename=str(root/'MannyReference.fbx');t.automated=True;t.prompt=False;t.exporter=ue.SkeletalMeshExporterFBX()
assert ue.Exporter.run_asset_export_task(t)
ue.log('HUNTER REFERENCE EXPORTED')
ue.SystemLibrary.quit_editor()
