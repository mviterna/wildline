"""Create the glow and import the two framed museum paintings in one editor run."""
from pathlib import Path
folder=Path(__file__).resolve().parent
exec((folder/'create_lodge_fire.py').read_text().replace('ue.SystemLibrary.quit_editor()','None'))
exec((folder/'import_lodge_paintings.py').read_text())
