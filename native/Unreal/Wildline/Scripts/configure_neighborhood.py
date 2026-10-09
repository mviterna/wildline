"""Generate two editable neighborhood layouts from measured asset dimensions.

Positions and landing surfaces are centimetres, meshes retain real-world scale.
Re-run after conversion, then import_neighborhood_assets.py in Unreal.
"""
import json, math
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent/'ArtSource/Neighborhood'
BASE='/Game/Wildline/Residential/Meshes/'

def make(key,name,feeder,utility,garden):
    props=[];models={r['name']:r for r in json.loads((ROOT/'model-index.json').read_text())}
    mounts=json.loads((ROOT/'hose-mounts.json').read_text())
    def add(mesh,p,scale=(1,1,1),yaw=0,solid=True,size=None,path=None,pitch=0):
        if size:scale=[size[i]/(models[mesh]['dimensions_m'][i]*100) for i in range(3)]
        props.append({'mesh':path or BASE+mesh+'.'+mesh,'position':p,'scale':scale,'yaw':yaw,'pitch':pitch,'collision':solid})
    add('SM_AsphaltStreet',(0,-4300,213))
    for side in [-1,1]:
        add('SM_ConcreteWalk',(0,-4300+side*720,218))
        for x in range(-5400,5401,200):add('SM_Curb',(x,-4300+side*568,211))
    # Six lots vary in architecture, paint, porches, yard dressing and tree canopy.
    for row in range(2):
        for lot in range(-1,2):
            x=lot*3100;y=-2700 if row==0 else 2900;yaw=180 if row==0 else 0
            house_mesh=['SM_KitGardenHouse','SM_KitTwoStoryHouse','SM_KitCottage'][lot+1] if row==1 else ('SM_CapeCodHouse' if lot%2 else 'SM_ColonialHouse')
            add(house_mesh,(x,y,210),yaw=yaw)
            add('SM_KitMailbox',(x-930,y-800 if row==0 else y-680,210),yaw=yaw)
            if row==1:
                add('SM_KitGarage',(x+970,y,210),yaw=yaw)
                add('SM_KitLounge',(x+730,y-950,210),yaw=lot*20)
            # The kit pool is an in-ground basin: its rim, not its bottom, meets
            # the lawn. Runtime terrain excavation follows this mesh's bounds.
            if row==1 and lot==-1:add('SM_KitPool',(x+950,y+1000,212-models['SM_KitPool']['dimensions_m'][2]*100))
            if row==1 and lot==1:add('SM_KitSwing',(x+950,y-650,210))
            add('SM_Driveway',(x+850,y-750 if row==0 else y-1500,212))
            for j in range(-6,7):
                if row==0 and abs(x+j*210)<1650:continue
                add('SM_PrivacyFence',(x+j*210,-1450 if row==0 else 1650,210),(.98,1,.80))
            add('SM_metal_trash_can',(x+900,y+650,210),yaw=23)
            # Contacts are measured against the exported wall triangles. Reel
            # backing overlaps the facade by 1 cm, despite different footprints.
            wall=mounts[house_mesh]['rear']['point_cm']
            depth=models['SM_garden_hose_wall_mounted_01']['dimensions_m'][1]*50-1
            add('SM_garden_hose_wall_mounted_01',(x-wall[0] if row==0 else x+wall[0],y-wall[1]+depth if row==0 else y+wall[1]-depth,275),yaw=0 if row==0 else 180,solid=False)
            add('SM_planter_pot_clay',(x-440,y+560 if row==0 else y-560,210))
            add('SM_garden_gnome',(x-720,y+610 if row==0 else y-610,210),solid=False)
            add('SM_painted_wooden_bench',(x+1150,850 if row==0 else 1350,210),yaw=lot*13)
            tree='/Game/Wildline/Forest/Meshes/SM_Oak'+('A' if lot!=0 else 'B')
            add('',(x-1250,y+450 if row==0 else y-400,210),(.45,.45,.45),yaw=lot*37,path=tree+'.'+tree.rsplit('/',1)[1])
    for i in range(-2,3):add('SM_UtilityPole02',(i*2400,-3440,210))
    for x in [-4200,4100]:add('SM_fire_hydrant',(x,-3530,210))
    for x,y in [(-2350,550),(2650,450)]:
        add('SM_outdoor_table_chair_set_01_table',(x,y,210))
        for side in [-1,1]:add('SM_plastic_monobloc_chair_01',(x,y+side*150,210),yaw=0 if side==1 else 180)
        add('SM_garden_sprinkler_01',(x+340,y+90,210),solid=False)
        add('SM_planter_box_01',(x+450,y-300,210))
    add('SM_CedarBirdFeeder',(feeder[0],feeder[1],feeder[2]-240))
    entry=utility['entry_trunk'];add('SM_UtilityPole01',entry,size=(110,70,630))
    # A real overhead service cable, with a small collision radius for paws.
    # Its first half is the checkpoint; the rest continues to the tree.
    t=utility['targets'][0]
    def cable_span(a,b):
        dx=b[0]-a[0];dy=b[1]-a[1];dz=b[2]-a[2];flat=math.hypot(dx,dy);length=math.hypot(flat,dz)
        add('SM_ServiceCable',((a[0]+b[0])/2,(a[1]+b[1])/2,(a[2]+b[2])/2),((length+20)/300,1,1),yaw=math.degrees(math.atan2(dy,dx)),pitch=math.degrees(math.atan2(dz,flat)))
    cable_span((entry[0],entry[1],t[2]),t);cable_span(t,utility['targets'][1])
    b=utility['targets'][1];add('SM_CourseBranch',(b[0],b[1],b[2]-3))
    tree='/Game/Wildline/Forest/Meshes/SM_OakA'
    add('',(b[0]-95,b[1]+15,210),(.34,.34,.34),yaw=180,path=tree+'.SM_OakA',solid=False)
    for route in [utility,garden]:
        f=route['targets'][2];add('SM_PrivacyFence',(f[0],f[1],210),(1,1,(f[2]-210)/190),yaw=-20 if route is utility else 18)
        tr=route['targets'][3];add('SM_BackyardTrampoline',(tr[0],tr[1],tr[2]-90))
    p=garden['targets'][0];add('SM_planter_box_01',(p[0],p[1],210),size=(125,105,p[2]-210))
    add('SM_PlanterSoil',p)
    b=garden['targets'][1];add('SM_PottingBench',(b[0],b[1],b[2]-90),yaw=12)
    (ROOT/'Configs').mkdir(exist_ok=True)
    data={'id':key,'display_name':name,'feeder':feeder,'routes':[utility,garden],'props':props}
    (ROOT/'Configs'/(key+'.json')).write_text(json.dumps(data,indent=2))
    print('CONFIG',key,len(props),'props',flush=True)

make('MapleCrescent','Maple Crescent / utility easement',[120,0,450],
    {'name':'Over the easement','start':[-1530,-400,210],'entry_trunk':[-1370,-400,210],'entry_climb_top':825,'entry_radius':15,'targets':[[-1050,-400,810],[-790,-310,750],[-510,-190,390],[-220,-145,300]],'labels':['TELEPHONE LINE','OAK LIMB','CEDAR FENCE','TRAMPOLINE'],'trampoline_launch_z':620},
    {'name':'Through the garden','start':[-1170,430,210],'entry_trunk':[0,0,0],'entry_climb_top':0,'entry_radius':15,'targets':[[-875,430,255],[-595,370,300],[-335,300,345],[-130,205,300]],'labels':['RAISED PLANTER','POTTING BENCH','GARDEN FENCE','TRAMPOLINE'],'trampoline_launch_z':620})
make('CedarBackyards','Cedar Backyards / staggered gardens',[200,80,450],
    {'name':'Around the old oak','start':[-1400,-620,210],'entry_trunk':[-1250,-620,210],'entry_climb_top':825,'entry_radius':15,'targets':[[-970,-620,810],[-705,-420,750],[-425,-260,390],[-120,-100,300]],'labels':['TELEPHONE LINE','OAK LIMB','PRIVACY FENCE','TRAMPOLINE'],'trampoline_launch_z':620},
    {'name':'Across the neighbors','start':[-1030,690,210],'entry_trunk':[0,0,0],'entry_climb_top':0,'entry_radius':15,'targets':[[-730,690,255],[-475,590,300],[-240,455,345],[-50,275,300]],'labels':['RAISED PLANTER','POTTING BENCH','GARDEN FENCE','TRAMPOLINE'],'trampoline_launch_z':620})
