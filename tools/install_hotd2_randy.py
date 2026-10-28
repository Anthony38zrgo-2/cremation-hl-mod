"""Install Randy's native fast-zombie actor and regional body pieces."""
from pathlib import Path
import json, shutil

MOD = Path(__file__).resolve().parents[1]
SOURCE = MOD / 'assets-source/hotd2-randy/goldsrc'
DEST = MOD / 'models/cremation/hotd2/randy'

def install():
    DEST.mkdir(parents=True, exist_ok=True)
    for name in ('randy.mdl', 'randy_charred.mdl', *(p.name for p in sorted(SOURCE.glob('part_*.mdl')))):
        source = SOURCE / name
        if not source.is_file(): raise FileNotFoundError(f'Run tools/convert_hotd2_randy.py: {source}')
        shutil.copy2(source, DEST / name)
    print('Installed Randy actor, charred actor and regional body parts.')

def replace_map():
    from bsp import BSP
    path=MOD/'maps/rapidcore.bsp';bsp=BSP(path)
    backup=MOD/'backups/before-hotd2-randy';backup.mkdir(parents=True,exist_ok=True)
    for item in (path,MOD/'placement.json'):
        if item.exists() and not (backup/item.name).exists():shutil.copy2(item,backup/item.name)
    changed=0
    for entity in bsp.entities:
        if entity.get('classname') in ('monster_fastzombie','monster_ken') and entity.get('targetname','').startswith('cremation_zombie_') and int(entity['targetname'].rsplit('_',1)[1])>20:entity['classname']='monster_randy';changed+=1
    if changed:bsp.write(path,bsp.entities)
    report=MOD/'placement.json'
    if report.exists():
        data=json.loads(report.read_text())
        for entity in data.get('zombies',[]):
            if entity.get('classname') in ('monster_fastzombie','monster_ken') and entity.get('id',0)>20:entity['classname']='monster_randy'
        report.write_text(json.dumps(data,indent=2)+'\n')
    print(f'Replaced {changed} fast-zombie slots with Randy in Rapidcore.')

if __name__ == '__main__':
    install()
    replace_map()
