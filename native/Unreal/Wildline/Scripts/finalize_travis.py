"""Compatibility entry point for rebuilding the two photo hunter assets."""
import unreal as ue
from pathlib import Path
root=Path(ue.Paths.project_dir()).resolve()
exec(compile((root/'Scripts/import_photo_hunters.py').read_text(),'import_photo_hunters.py','exec'))
