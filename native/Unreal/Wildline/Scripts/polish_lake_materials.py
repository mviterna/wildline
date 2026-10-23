"""Refresh lake shading without rebuilding the map or reimporting meshes."""
from pathlib import Path
import unreal as ue
source=Path(ue.Paths.project_dir())/'Scripts/import_lake_assets.py'
exec(compile(source.read_text().split('\nmeshes=')[0],str(source),'exec'))
ue.log('LAKE MATERIALS PASS');ue.SystemLibrary.quit_editor()
