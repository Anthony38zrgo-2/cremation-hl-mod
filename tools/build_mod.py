from pathlib import Path
import shutil, json, hashlib, math, re
from bsp import BSP
from install_ricochet_assets import install as install_ricochet
from install_bounce_assets import install as install_bounce

ROOT=Path(__file__).resolve().parents[2]
MOD=ROOT/'Cremation'
DONOR=ROOT/'Half-Life2Classic_Ravenholm'
SPAWN=(432,-1601,-220)
SPAWN_YAW=155
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
    config_path.write_text(config_text)
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
    loadout=['item_suit','weapon_ricochet','weapon_shotgun','weapon_bounce_shotgun']+['ammo_buckshot']*3
    for i,classname in enumerate(loadout):
        es.append(dict(classname=classname,origin=gear_origin,angles=f'0 {SPAWN_YAW} 0',targetname=f'cremation_loadout_{i+1:02}'))
    placements=[]
    for i,(x,y) in enumerate(POSITIONS):
        floor_z=bsp.floor(x,y,top=128)
        assert floor_z is not None, (x,y,'no standing floor')
        origin=(x,y,floor_z+1)
        assert bsp.clear((x,y,floor_z+37),1), (origin,'blocked standing hull')
        distance=math.dist(SPAWN[:2],origin[:2])
        assert distance>1000, (origin,'too close')
        for eye in (32,64):
            assert not bsp.line_clear((SPAWN[0],SPAWN[1],SPAWN[2]+28),(x,y,floor_z+eye)),(origin,'visible from spawn')
        yaw=(i*137+35)%360
        es.append(dict(classname='monster_zombie',origin=vec(origin),angles=f'0 {yaw} 0',targetname=f'cremation_zombie_{i+1:02}',spawnflags='0'))
        placements.append(dict(id=i+1,classname='monster_zombie',origin=origin,distance_from_spawn=round(distance,1),direct_line_of_sight=False))
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
    report=dict(name='Cremation',map='rapidcore',spawn=dict(origin=SPAWN,angles=(0,SPAWN_YAW,0)),loadout=loadout,zombies=placements,navigation_nodes=len(nodes),original_map_sha256=hashlib.sha256(bsp.data).hexdigest(),runtime_source=DONOR.name,validation='Static geometry and files only; manual gameplay validation pending.')
    (MOD/'placement.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Built Cremation:',len(placements),'zombies;',len(nodes),'navigation nodes;',len(es),'entities')
    print('Closest zombie:',min(p['distance_from_spawn'] for p in placements),'units')

if __name__=='__main__': build()
