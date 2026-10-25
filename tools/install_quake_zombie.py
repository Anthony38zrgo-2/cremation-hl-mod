"""Install isolated SGClassic zombie models and sounds from the user's Quake PAK."""
from pathlib import Path
import argparse, hashlib, json, struct, zipfile, shutil, math

MOD = Path(__file__).resolve().parents[1]
DEFAULT_ZIP = Path.home() / 'Downloads/slipgateclassic_beta11.zip'
DEFAULT_PAK = MOD.parent.parent / 'Quake/id1/PAK0.PAK'
MODELS = ('zombie', 'zom_gib', 'h_zombie', 'gib_1', 'gib_2', 'gib_3')
SOUNDS = ('z_idle', 'z_idle1', 'z_shot1', 'z_gib', 'z_pain', 'z_pain1',
          'z_fall', 'z_miss', 'z_hit', 'idle_w2')

def install(archive=DEFAULT_ZIP, pak=DEFAULT_PAK):
    manifest = {}
    def write(relative, data, source):
        dest = MOD / relative
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_bytes(data)
        manifest[relative] = dict(source=source, sha256=hashlib.sha256(data).hexdigest(), bytes=len(data))
    with zipfile.ZipFile(archive) as z:
        for name in MODELS:
            member = f'SGClassic/models/{name}.mdl'
            remaster = MOD / 'assets-source/quake-zombie-remaster/goldsrc/zombie.mdl'
            data = remaster.read_bytes() if name == 'zombie' and remaster.exists() else z.read(member)
            assert data[:4] == b'IDST' and struct.unpack_from('<i', data, 4)[0] == 10
            source = 'Quake/rerelease/id1/pak0.pak:progs/zombie.md5mesh + zombie.md5anim; scale=2.0' if name == 'zombie' and remaster.exists() else f'{Path(archive).name}:{member}'
            write(f'models/cremation/quake/{name}.mdl', data, source)
    with Path(pak).open('rb') as f:
        magic, offset, size = struct.unpack('<4sii', f.read(12))
        assert magic == b'PACK' and size % 64 == 0
        f.seek(offset)
        entries = {name.split(b'\0')[0].decode(): (start, length)
                   for name, start, length in struct.iter_unpack('<56sii', f.read(size))}
        for name in SOUNDS:
            member = f'sound/zombie/{name}.wav'
            start, length = entries[member]
            f.seek(start)
            data = f.read(length)
            assert data[:4] == b'RIFF' and data[8:12] == b'WAVE'
            write(f'sound/cremation/quake/zombie/{name}.wav', data, f'Quake/id1/PAK0.PAK:{member}')
    (MOD / 'quake-zombie-assets.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'Installed {len(MODELS)} Quake models and {len(SOUNDS)} sounds in private Cremation paths.')

def main_zombie_class(index):
    """Zero-based main horde slot: alternate Quake and normal, ten of each."""
    assert 0 <= index < 20
    return 'monster_quake_zombie' if index % 2 == 0 else 'monster_zombie'

def replace_map():
    from bsp import BSP
    path=MOD/'maps/rapidcore.bsp';bsp=BSP(path)
    main=[e for e in bsp.entities if e.get('classname') in ('monster_zombie','monster_quake_zombie')]
    assert len(main)==20
    backup=MOD/'backups/before-purple-zombies';backup.mkdir(parents=True,exist_ok=True)
    for name in ('maps/rapidcore.bsp','placement.json','models/zombie.mdl','models/cremation/charred/zombie.mdl'):
        dest=backup/Path(name).as_posix().replace('/','_')
        if not dest.exists():shutil.copy2(MOD/name,dest)
    changes=0
    for e in main:
        index=int(e['targetname'].rsplit('_',1)[1])-1
        desired=main_zombie_class(index)
        if e['classname']!=desired:e['classname']=desired;changes+=1
    if changes:bsp.write(path,bsp.entities)
    report_path=MOD/'placement.json';report=json.loads(report_path.read_text())
    for e in report['zombies']:
        if e['id']<=20:e['classname']=main_zombie_class(e['id']-1)
    report_path.write_text(json.dumps(report,indent=2)+'\n')
    print(f'Main horde: 10 normal + 10 Quake zombies; {changes} entity replacements. Fast/poison unchanged.')

def adjust_tall_patrols():
    from bsp import BSP
    path=MOD/'maps/rapidcore.bsp';bsp=BSP(path)
    report_path=MOD/'quake-zombie-patrols.json'
    changes=json.loads(report_path.read_text()) if report_path.exists() else {}
    new_changes={}
    def clear_route(a,b):
        steps=max(1,math.ceil(math.dist(a,b)/4))
        for i in range(steps+1):
            p=[a[k]+(b[k]-a[k])*i/steps for k in range(3)]
            if not bsp.clear((p[0],p[1],p[2]+36),1):return False
            for dx,dy in [(0,0),(-16,-16),(-16,16),(16,-16),(16,16)]:
                if not bsp.line_clear((p[0]+dx,p[1]+dy,p[2]+18),(p[0]+dx,p[1]+dy,p[2]+104)):return False
        return True
    for zombie in bsp.entities:
        if zombie.get('classname')!='monster_quake_zombie':continue
        origin=tuple(map(float,zombie['origin'].split()))
        marks=[e for e in bsp.entities if e.get('netname')==zombie['targetname']]
        goal=next(e for e in marks if e['targetname'].endswith('_0'))
        old=tuple(map(float,goal['origin'].split()))
        if clear_route(origin,old):continue
        candidate=None
        for radius in (96,64,128):
            for angle in range(0,360,30):
                x=origin[0]+radius*math.cos(math.radians(angle));y=origin[1]+radius*math.sin(math.radians(angle))
                floor=bsp.floor(x,y,top=128)
                if floor is None or abs(floor+1-origin[2])>8:continue
                point=(x,y,floor+1)
                if clear_route(origin,point):candidate=point;break
            if candidate:break
        assert candidate,('No tall patrol',zombie['targetname'])
        new=' '.join(f'{v:g}' for v in candidate)
        new_changes[goal['targetname']]=dict(before=changes.get(goal['targetname'],{}).get('before',goal['origin']),after=new)
        goal['origin']=new
    if new_changes:
        changes.update(new_changes);bsp.write(path,bsp.entities)
        placements=MOD/'placement.json';placement=json.loads(placements.read_text())
        for e in placement['zombies']:
            key=f"cremation_patrol_{e['id']:02}_0"
            if key in changes:e['patrol'][1]=list(map(float,changes[key]['after'].split()))
        placements.write_text(json.dumps(placement,indent=2)+'\n')
    report_path.write_text(json.dumps(changes,indent=2)+'\n')
    print(f'Tall-zombie patrols checked; {len(new_changes)} waypoint corrections installed.')

if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--zip', type=Path, default=DEFAULT_ZIP)
    p.add_argument('--pak', type=Path, default=DEFAULT_PAK)
    a = p.parse_args()
    install(a.zip, a.pak)
    replace_map()
    adjust_tall_patrols()
