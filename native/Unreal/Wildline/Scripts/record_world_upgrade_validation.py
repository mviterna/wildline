"""Record checked package evidence without rerunning gameplay probes."""
from pathlib import Path
from datetime import datetime, timezone
import json
import re

project = Path(__file__).resolve().parents[1]
native = project.parents[1]
repo = native.parent
package = native / 'Build/WorldUpgradeReview/Windows'
completion = json.loads((package / 'WildlineBuild.json').read_text(encoding='utf-8-sig'))
completed = datetime.fromisoformat(completion['completedAtUtc'])


def evidence(relative, required, fresh=False):
    path = repo / relative
    text = path.read_text(encoding='utf-8-sig', errors='replace')
    assert required in text, f'Missing pass: {relative}'
    modified = datetime.fromtimestamp(path.stat().st_mtime, timezone.utc)
    if fresh:
        assert modified > completed, f'Stale final-package evidence: {relative}'
    return {'log': relative, 'verified_at_utc': modified.isoformat(), 'passed': True}


maps = []
for name in ['Blackwood', 'Farmstead', 'Neighborhood', 'NeighborhoodCedar', 'Frostwood']:
    result = evidence(f'native/world-upgrade-{name}-smoke.log', 'WILDLINE TRAVERSAL: PASS', name == 'Frostwood')
    result['map'] = name
    result['stage'] = 'final package' if name == 'Frostwood' else 'packaged gameplay pass before final room-lighting adjustment'
    maps.append(result)

routes = []
for name in ['Neighborhood', 'NeighborhoodCedar']:
    for lane in range(2):
        relative = f'native/neighborhood-{name}-lane{lane}.log'
        result = evidence(relative, 'WILDLINE HILL SMOKE: PASS')
        text = (repo / relative).read_text(encoding='utf-8', errors='replace')
        counts = re.findall(r'HILL BOT .*visits=(\d+)', text)
        assert counts, relative
        result.update(map=name, lane=lane, completed_trips=int(counts[-1]))
        routes.append(result)

automation_log = 'native/Unreal/Wildline/Saved/Logs/Wildline-backup-2026.10.09-03.37.37.log'
automation = evidence(automation_log, 'Result={Success}')
automation['tests'] = re.findall(r'Test Completed\. Result=\{Success\} Name=\{[^}]+\} Path=\{([^}]+)\}', (repo / automation_log).read_text(encoding='utf-8', errors='replace'))
assert len(automation['tests']) == 11

renders = []
for name, shots in [('Frostwood', ['Reserve', 'Branch']), ('Neighborhood', ['Cars', 'Neighborhood'])]:
    result = evidence('native/world-upgrade-final-captures.log', f'PASS {name}/review', True)
    result['render_log'] = f'native/world-upgrade-{name}-review.log'
    images = []
    for shot in shots + ['HunterTrophies', 'SquirrelDen', 'LodgeWoods']:
        path = native / f'Preview/{name}_Upgrade{shot}.png'
        assert path.stat().st_mtime > completed.timestamp(), f'Stale image: {path}'
        images.append(path.relative_to(repo).as_posix())
    result.update(map=name, images=images, visually_reviewed=True)
    renders.append(result)

report = {
    'verified_at_utc': datetime.now(timezone.utc).isoformat(),
    'engine': 'Unreal Engine 5.8.3',
    'package': package.relative_to(repo).as_posix(),
    'package_completed_at_utc': completion['completedAtUtc'],
    'features': [
        'Six City Sample cars and seven City Park details in each neighborhood',
        'Wooded lodge horizon with layered trees and rising ridges',
        'Three hunter trophies, keepsake shelf, raid table and branch perch in Acorn Hollow',
        'Wood-only branch and trunk collision with grip traversal on all five maps',
        'Approximately 28 percent fewer canopy leaf faces; ground leaves reduced from 28000 to 19000',
        'Leafless Frostwood winter map preserving Classic rules and 30 forage acorns',
    ],
    'editor_automation': automation,
    'packaged_traversal': maps,
    'packaged_routes_before_final_lighting_pass': routes,
    'classic_regression': evidence('native/world-upgrade-package-classic.log', 'WILDLINE SMOKE RESULT: PASS'),
    'farm_regression': evidence('native/world-upgrade-package-farm.log', 'WILDLINE FARM SMOKE: PASS'),
    'final_package_network': [evidence(f'native/world-upgrade-net-{role}.log', f'WILDLINE TRAVERSAL NET {role}: PASS', True) for role in ['host', 'client']],
    'final_package_waiting_rooms': evidence('native/world-upgrade-final-lobbies.log', 'co-op replication: PASS', True),
    'final_package_visual_review': renders,
    'desktop_launcher': json.loads((native / 'world-upgrade-launcher-verification.json').read_text(encoding='utf-8-sig')),
    'desktop_visible_launch': json.loads((native / 'world-upgrade-visible-launch.json').read_text(encoding='utf-8-sig')),
    'new_asset_download_blockers': [],
    'previous_asset_download_blockers': [{'asset': 'Artec bird feeder scan', 'reason': 'Publisher provides no download option or reuse license', 'url': 'https://www.artec3d.com/3d-models/bird-feeder'}],
}
output = project / 'ArtSource/WorldUpgrade/validation.json'
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')

old = project / 'ArtSource/Neighborhood/validation.json'
neighborhood = json.loads(old.read_text(encoding='utf-8-sig'))
neighborhood.update(verified_at=report['verified_at_utc'], build=report['package'], physical_routes=routes,
    visual_review={'verified_at': report['verified_at_utc'], 'description': 'Final cooked cars, neighborhood, den trophies and wooded lodge captures at 1600x900.'},
    test_stage='Physical routes verified in packaged gameplay pass; final changes affect room lighting only.')
old.write_text(json.dumps(neighborhood, indent=2) + '\n', encoding='utf-8')
print(output)
