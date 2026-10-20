"""Author a woven, bound and fringed lodge rug using retained CC0 PBR maps.

Run in Blender after convert_lobby_assets.py --upgrade-only.
"""
import bpy, math, json
from pathlib import Path
helper=Path(__file__).with_name('convert_lobby_assets.py').read_text()
exec(helper[:helper.index('import sys\nif')])
REPORT=json.loads((ROOT/'model-index.json').read_text())
REPORT=[r for r in REPORT if r['name']!='SM_LodgeHeritageRug']
MATERIALS=json.loads((ROOT/'material-index.json').read_text())
reset()
weave=mat('M_LodgeRugWeave','dirty_carpet')
MATERIALS['M_LodgeRugWeave'].update(factor=[1.05,.93,.73,1],desaturate=True)
border=mat('M_LodgeRugBorder','dirty_carpet')
MATERIALS['M_LodgeRugBorder'].update(factor=[.19,.34,.26,1],desaturate=True)
fringe=mat('M_LodgeRugFringe','dirty_carpet')
MATERIALS['M_LodgeRugFringe'].update(factor=[1.25,1.10,.79,1],desaturate=True)
objects=[box('Wool field',(0,0,.012),(4.6,3.6,.024),weave,.008)]
for side in [-1,1]:
    objects.append(box('Forest green bound edge',(side*2.20,0,.027),(.20,3.6,.012),border,.005))
    objects.append(box('Forest green bound edge',(0,side*1.70,.027),(4.6,.20,.012),border,.005))
    for x in [-1.98+i*.22 for i in range(19)]:
        o=box('Woven border diamond',(x,side*1.70,.037),(.075,.075,.004),fringe,.002);o.rotation_euler.z=math.pi/4;objects.append(o)
    for y in [-1.48+i*.22 for i in range(14)]:
        o=box('Woven border diamond',(side*2.20,y,.037),(.075,.075,.004),fringe,.002);o.rotation_euler.z=math.pi/4;objects.append(o)
    for i in range(216):
        x=-2.24+i*.0208
        for strand in range(2):
            o=box('Cotton fringe strand',(x+strand*.004,side*1.86,.009),(.003,.13,.005),fringe,.001)
            o.rotation_euler.z=math.sin(i*7.1+strand)*.13;objects.append(o)
export('SM_LodgeHeritageRug',objects,False)
# Tame the orange floor and lift dark timber while preserving scanned grain.
MATERIALS['M_LodgeFloor']['factor']=[.70,.67,.59,1]
MATERIALS['M_LodgeTimber']['factor']=[1.45,1.40,1.28,1]
(ROOT/'model-index.json').write_text(json.dumps(REPORT,indent=2))
(ROOT/'material-index.json').write_text(json.dumps(MATERIALS,indent=2))
print('LODGE UPGRADE RUG COMPLETE',flush=True)
