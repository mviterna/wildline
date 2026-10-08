"""Compatibility entry point for photo hunter asset verification."""
import unreal as ue
from pathlib import Path
root=Path(ue.Paths.project_dir()).resolve()
exec(compile((root/'Scripts/verify_photo_hunters.py').read_text(),'verify_photo_hunters.py','exec'))
