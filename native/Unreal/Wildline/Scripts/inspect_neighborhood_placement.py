"""Read imported pivots and map actors without changing the layouts."""
import unreal as ue
import json
from pathlib import Path

root = Path(ue.Paths.project_dir()).resolve()
rows = []
for item in json.loads((root / 'ArtSource/Neighborhood/model-index.json').read_text()):
    mesh = ue.EditorAssetLibrary.load_asset('/Game/Wildline/Residential/Meshes/' + item['name'])
    b = mesh.get_bounds()
    row = {'name': item['name'], 'origin': [b.origin.x, b.origin.y, b.origin.z],
           'extent': [b.box_extent.x, b.box_extent.y, b.box_extent.z]}
    rows.append(row)
    ue.log('PLACEMENT BOUNDS ' + json.dumps(row))
levels = ue.get_editor_subsystem(ue.LevelEditorSubsystem)
actors = ue.get_editor_subsystem(ue.EditorActorSubsystem)
for path in ['/Game/Maps/Neighborhood', '/Game/Maps/NeighborhoodCedar']:
    levels.load_level(path)
    for actor in actors.get_all_level_actors():
        ue.log('PLACEMENT ACTOR ' + path + ' ' + actor.get_class().get_name() + ' ' + actor.get_actor_label() + ' ' + str(actor.get_actor_location()))
(root / 'Saved/NeighborhoodBounds.json').write_text(json.dumps(rows, indent=2))
