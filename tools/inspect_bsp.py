import struct, re, collections
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

def entities(path):
    data = path.read_bytes()
    offset, size = struct.unpack_from('<ii', data, 4)
    return [dict(re.findall(r'"([^"]*)"\s*"([^"]*)"', block))
            for block in re.findall(r'\{([^{}]*)\}', data[offset:offset+size].decode('latin1'))]

if __name__ == '__main__':
    for name in ['valve/maps/rapidcore.bsp', 'Half-Life2Classic_Ravenholm/maps/devtest.bsp']:
        es = entities(ROOT/name)
        print(name, collections.Counter(e.get('classname') for e in es))
        for e in es:
            if e.get('classname') in ('worldspawn','info_player_start','info_player_deathmatch','monster_zombie','monster_fastzombie','monster_poisionzombie'):
                print(e)
