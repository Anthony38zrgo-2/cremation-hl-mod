from pathlib import Path
import shutil, json, hashlib, math, re
from bsp import BSP
from install_ricochet_assets import install as install_ricochet
from install_bounce_assets import install as install_bounce
from install_tfc_assets import install as install_tfc
from install_horde_assets import install as install_horde
from install_hecu_assets import install as install_hecu
from install_additional_weapons import install as install_additional_weapons

ROOT=Path(__file__).resolve().parents[2]
MOD=ROOT/'Cremation'
DONOR=ROOT/'Half-Life2Classic_Ravenholm'
SPAWN=(432,-1601,-220)
SPAWN_YAW=155
LOADOUT=['item_suit','weapon_ricochet','weapon_shotgun','weapon_bounce_shotgun',
         'weapon_pipewrench','weapon_nailgun','weapon_handgrenade','weapon_rpg',
         'ammo_9mmAR']+['ammo_buckshot']*3
# All are regular Ravenholm zombies. No fast/poison variants or extra waves.
POSITIONS=[(-160,800),(200,880),(500,700),(-550,700),
           (-1450,350),(-1450,-150),(-1250,0),(-1350,200),
           (-850,300),(-850,-200),(-800,-480),
           (-240,100),(180,100),(-240,-300),(180,-300),
           (960,0),(1100,-400),(900,-650),(480,-300),(480,100)]

def prepare():
    for directory in ('dlls','cl_dlls','models','sound','sprites','events'):
        shutil.copytree(DONOR/directory,MOD/directory,dirs_exist_ok=True)
    for file in ('skill.cfg','titles.txt'):
        shutil.copy2(DONOR/file,MOD/file)
    install_ricochet()
    install_bounce()
    install_tfc()
    install_horde()
    install_hecu()
    install_additional_weapons()
    for file in ('config.cfg','default.cfg'):
        if not (MOD/file).exists(): shutil.copy2(ROOT/'valve'/file,MOD/file)
    # Preserve the user's existing controls, including their auxiliary configs.
    for file in ('userconfig.cfg','controller.cfg','violence.cfg'):
        if (ROOT/'valve'/file).exists() and not (MOD/file).exists():
            shutil.copy2(ROOT/'valve'/file,MOD/file)
    # autoexec may run before config.cfg. Keep the saved per-mod value consistent.
    config_path=MOD/'config.cfg'
    config_text=config_path.read_text()
    config_text=re.sub(r'(?m)^cl_lw\s+[^\r\n]*','cl_lw "0"',config_text)
    for key,command in [('b','cremation_build'),('n','cremation_dismantle'),('j','cremation_repair')]:
        if not re.search(r'(?im)^bind\s+"?'+key+r'"?\s',config_text):config_text+=f'\nbind "{key}" "{command}"\n'
    config_path.write_text(config_text)
    (MOD/'engineer.cfg').write_text('''// Run once to assign the engineer actions.\nbind b cremation_build\nbind n cremation_dismantle\nbind j cremation_repair\n''')
    defaults=MOD/'default.cfg'
    default_text=defaults.read_text()
    for key,command in [('b','cremation_build'),('n','cremation_dismantle'),('j','cremation_repair')]:
        if not re.search(r'(?im)^bind\s+"?'+key+r'"?\s',default_text):default_text+=f'\nbind "{key}" "{command}"\n'
    defaults.write_text(default_text)
    (MOD/'maps').mkdir(exist_ok=True)
    (MOD/'liblist.gam').write_text('''game "Cremation"
developer "Cremation"
startmap "rapidcore"
trainmap "rapidcore"
gamedll "dlls/cremation.dll"
type "singleplayer_only"
secure "1"
''')
    (MOD/'cremation.cfg').write_text('''// Start a fresh Cremation session.
deathmatch 0
coop 0
maxplayers 1
skill 1
sv_cheats 0
hud_draw 1
cl_lw 0
map rapidcore
''')
    # Server-authoritative disc attacks must not run the inherited pistol prediction.
    (MOD/'autoexec.cfg').write_text('// Cremation Ricochet runs on the local game server.\ncl_lw 0\n')

def vec(p): return ' '.join(f'{v:g}' for v in p)

def build():
    prepare()
    bsp=BSP(ROOT/'valve/maps/rapidcore.bsp')
    es=[dict(e) for e in bsp.entities
        if e.get('classname') not in ('info_player_deathmatch','info_player_start','item_longjump')
        and not e.get('classname','').startswith(('weapon_','ammo_'))]
    for e in es:
        if e.get('classname')=='worldspawn': e['message']='Cremation'; e['chaptertitle']='CREMATION'
    floor=bsp.floor(SPAWN[0],SPAWN[1])
    assert floor is not None and abs(SPAWN[2]-floor-36)<.1
    assert bsp.clear(SPAWN,1)
    es.append(dict(classname='info_player_start',origin=vec(SPAWN),angles=f'0 {SPAWN_YAW} 0'))
    # Real touch pickups overlap the standing player's hull at the spawn point.
    # They work in single-player without console cheats or multiplayer equip hooks.
    gear_origin=vec((SPAWN[0],SPAWN[1],floor+8))
    loadout=LOADOUT
    for i,classname in enumerate(loadout):
        es.append(dict(classname=classname,origin=gear_origin,angles=f'0 {SPAWN_YAW} 0',targetname=f'cremation_loadout_{i+1:02}'))
    # Six shell boxes in separate safe standing areas across the map.
    shell_pickups=[]
    for n,(x,y) in enumerate([POSITIONS[i] for i in (0,4,8,11,15,18)]):
        z=bsp.floor(x,y,top=128)
        assert z is not None and bsp.clear((x,y,z+37),1)
        origin=(x,y,z+8)
        es.append(dict(classname='ammo_buckshot',origin=vec(origin),targetname=f'cremation_shells_{n+1:02}'))
        shell_pickups.append(origin)
        for dx,dy in ((32,0),(-32,0),(0,32),(0,-32),(0,0)):
            if bsp.clear((x+dx,y+dy,z+8),0):
                es.append(dict(classname='item_battery',origin=vec((x+dx,y+dy,z+8)),targetname=f'cremation_metal_{n+1:02}'))
                break
    positions=list(POSITIONS)
    # Pick 20 extra reachable standing areas, preserving the original exclusion zone.
    for x in range(-1632,1185,112):
        for y in range(-1568,1009,112):
            if len(positions)>=40:break
            z=bsp.floor(x,y,top=128)
            if z is None or math.dist(SPAWN[:2],(x,y))<=1000:continue
            if any(math.dist((x,y),p)<112 for p in positions):continue
            if not bsp.clear((x,y,z+37),1):continue
            if any(bsp.line_clear((SPAWN[0],SPAWN[1],SPAWN[2]+28),(x,y,z+h)) for h in (1,33,65)):continue
            positions.append((x,y))
    assert len(positions)==40,len(positions)
    placements=[]
    for i,(x,y) in enumerate(positions):
        floor_z=bsp.floor(x,y,top=128)
        assert floor_z is not None, (x,y,'no standing floor')
        origin=(x,y,floor_z+1)
        assert bsp.clear((x,y,floor_z+37),1), (origin,'blocked standing hull')
        distance=math.dist(SPAWN[:2],origin[:2])
        assert distance>1000, (origin,'too close')
        for eye in (32,64):
            assert not bsp.line_clear((SPAWN[0],SPAWN[1],SPAWN[2]+28),(x,y,floor_z+eye)),(origin,'visible from spawn')
        yaw=(i*137+35)%360
        classname='monster_zombie' if i<20 else ('monster_fastzombie' if i%2==0 else 'monster_poisionzombie')
        target=f'cremation_zombie_{i+1:02}'
        es.append(dict(classname=classname,origin=vec(origin),angles=f'0 {yaw} 0',targetname=target,spawnflags='0'))
        # Native route following toward the spawn starts one second after spawn.
        # No NoInterrupt flag: seeing/being attacked by player/turret can break it.
        patrol=None
        for radius in ((64,96,128) if i in (9,10,38) else (128,96,64)):
            for angle in (range(180,540,45) if i in (9,10,38) else range(0,360,45)):
                a=math.radians(angle+i*17)
                qx,qy=x+math.cos(a)*radius,y+math.sin(a)*radius
                qz=bsp.floor(qx,qy,top=128)
                if qz is None or abs(qz-floor_z)>8:continue
                # Native route building includes other monster hulls. Do not
                # start a patrol through another zombie's initial footprint.
                blocked=False
                for ox,oy in positions:
                    if (ox,oy)==(x,y):continue
                    t=max(0,min(1,((ox-x)*(qx-x)+(oy-y)*(qy-y))/(radius*radius)))
                    if math.dist((ox,oy),(x+(qx-x)*t,y+(qy-y)*t))<56:
                        blocked=True;break
                if blocked:continue
                goal=(qx,qy,qz+1)
                steps=math.ceil(radius/8)
                if all(bsp.clear((x+(qx-x)*t/steps,y+(qy-y)*t/steps,origin[2]+(goal[2]-origin[2])*t/steps+36),1)
                       and not bsp.clear((x+(qx-x)*t/steps,y+(qy-y)*t/steps,origin[2]+(goal[2]-origin[2])*t/steps-3),0) for t in range(steps+1)):
                    patrol=goal;break
            if patrol:break
        assert patrol,(i,origin,'no physical patrol')
        if i==9:
            es[-1]['angles']=f'0 {math.degrees(math.atan2(patrol[1]-y,patrol[0]-x)):g} 0'
        # Passive waypoints never possess monsters or override native combat.
        first=f'cremation_patrol_{i+1:02}_0';second=f'cremation_patrol_{i+1:02}_1'
        for mark,next_mark,point in ((first,second,patrol),(second,first,origin)):
            es.append(dict(classname='info_target',origin=vec(point),targetname=mark,target=next_mark,netname=target))
        placements.append(dict(patrol=[origin,patrol],id=i+1,classname=classname,origin=origin,distance_from_spawn=round(distance,1),direct_line_of_sight=False))
    # Twelve armed cultists, with four supplied heads and native HECU weapons.
    hecu=[]
    occupied=[tuple(p['origin']) for p in placements]
    for x in range(-1600,1185,96):
        for y in range(-1408,1009,96):
            if len(hecu)==12:break
            z=bsp.floor(x,y,top=128)
            if z is None or math.dist(SPAWN[:2],(x,y))<1050:continue
            origin=(x,y,z+1)
            if not bsp.clear((x,y,z+37),1):continue
            if any(math.dist(origin,p)<144 for p in occupied):continue
            if any(bsp.line_clear((SPAWN[0],SPAWN[1],SPAWN[2]+28),(x,y,z+h)) for h in (32,64)):continue
            i=len(hecu)
            weapons=(1,8,3,10,5,12)[i%6]
            e=dict(classname='monster_human_grunt',origin=vec(origin),angles=f'0 {(i*137)%360} 0',
                   targetname=f'cremation_hecu_{i+1:02}',weapons=str(weapons),spawnflags='32',netname=f'cultist_squad_{i//4}')
            es.append(e);occupied.append(origin)
            hecu.append(dict(origin=origin,weapons=weapons,head=i%4,targetname=e['targetname']))
    assert len(hecu)==12,hecu
    # A ground-node network allows the inherited Ravenholm AI to route around corners.
    nodes=[]
    for x in range(-1760,1201,128):
        for y in range(-1664,1057,128):
            z=bsp.floor(x,y,top=128,bottom=-448)
            if z is None: continue
            origin=(x,y,z+8)
            nodes.append(origin)
            es.append(dict(classname='info_node',origin=vec(origin)))
    for origin in [SPAWN]+[tuple(p['origin']) for p in placements]:
        if all(math.dist(origin,p)>64 for p in nodes):
            es.append(dict(classname='info_node',origin=vec(origin)))
            nodes.append(origin)
    bsp.write(MOD/'maps/rapidcore.bsp',es)
    report=dict(hecu=hecu,shell_pickups=shell_pickups,name='Cremation',map='rapidcore',spawn=dict(origin=SPAWN,angles=(0,SPAWN_YAW,0)),loadout=loadout,zombies=placements,navigation_nodes=len(nodes),original_map_sha256=hashlib.sha256(bsp.data).hexdigest(),runtime_source=DONOR.name,validation='Static geometry and files only; manual gameplay validation pending.')
    (MOD/'placement.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Built Cremation:',len(placements),'zombies;',len(nodes),'navigation nodes;',len(es),'entities')
    print('Closest zombie:',min(p['distance_from_spawn'] for p in placements),'units')

if __name__=='__main__': build()
