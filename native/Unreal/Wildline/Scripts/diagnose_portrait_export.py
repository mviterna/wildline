"""Read-only export of the actual imported portrait UV channels."""
import unreal as ue
from pathlib import Path
root=Path(ue.Paths.project_dir()).resolve()
out=root.parents[1]/'AssetDownloads/Hunters/Diagnostics'
out.mkdir(parents=True,exist_ok=True)
task=ue.AssetExportTask()
task.object=ue.EditorAssetLibrary.load_asset('/Game/Wildline/Hunters/Meshes/SK_HunterFemale')
task.filename=str(out/'SK_Female_FromUE.fbx')
task.automated=True;task.prompt=False;task.replace_identical=True
task.exporter=ue.SkeletalMeshExporterFBX()
assert ue.Exporter.run_asset_export_task(task)
ue.log('PORTRAIT UV EXPORT '+task.filename)
ue.SystemLibrary.quit_editor()
