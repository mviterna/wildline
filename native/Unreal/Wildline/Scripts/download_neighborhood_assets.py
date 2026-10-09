"""Acquire commercial-use CC0 neighborhood sources, with verified hashes.

No marketplace credentials are stored in the project. Public Poly Haven API only.
"""
import importlib.util
from pathlib import Path

path = Path(__file__).with_name('download_lobby_assets.py')
spec = importlib.util.spec_from_file_location('acquisition', path)
acquisition = importlib.util.module_from_spec(spec)
spec.loader.exec_module(acquisition)
acquisition.ROOT = path.parent.parent / 'ArtSource/Neighborhood/Photoreal'
acquisition.MODELS = [
    'modular_electricity_poles', 'painted_wooden_bench', 'metal_trash_can',
    'fire_hydrant', 'garden_hose_wall_mounted_01', 'garden_sprinkler_01',
    'planter_box_01', 'planter_pot_clay', 'plastic_monobloc_chair_01',
    'outdoor_table_chair_set_01', 'garden_gnome',
]
acquisition.TEXTURES = [
    'weathered_plank_siding', 'roof_07', 'aerial_asphalt_01',
    'concrete_pavement', 'grass_path_2', 'wood_peeling_paint_weathered',
]
if __name__ == '__main__':
    acquisition.main()
