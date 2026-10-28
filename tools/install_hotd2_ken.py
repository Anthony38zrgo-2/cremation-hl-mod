"""Install Ken's David-family actor and regional body pieces."""
from pathlib import Path
import json, shutil

MOD = Path(__file__).resolve().parents[1]
SOURCE = MOD / 'assets-source/hotd2-ken/goldsrc'
DEST = MOD / 'models/cremation/hotd2/ken'

def install():
    DEST.mkdir(parents=True, exist_ok=True)
    for name in ('ken.mdl', 'ken_charred.mdl', *(p.name for p in sorted(SOURCE.glob('part_*.mdl')))):
        source = SOURCE / name
        if not source.is_file(): raise FileNotFoundError(f'Run tools/convert_hotd2_ken.py: {source}')
        shutil.copy2(source, DEST / name)
    print('Installed Ken actor, charred actor and regional body parts.')

if __name__ == '__main__': install()
