"""Check installed Mansion geometry, spawns, routes, textures, sky and defaults."""
from pathlib import Path
import collections
import hashlib
import json
import struct
from bsp import BSP

MOD=Path(__file__).resolve().parents[1]
source=BSP(MOD/'assets-source/mansion/maps/cs_mansion.bsp')
bsp=BSP(MOD/'maps/cs_mansion.bsp')
assert all(source.lump(i)==bsp.lump(i) for i in range(1,15))
entities=bsp.entities
tt=[e for e in source.entities if e['classname']=='info_player_deathmatch']
ct=[e for e in source.entities if e['classname']=='info_player_start']
players=[e for e in entities if e['classname']=='info_player_start']
assert [e['origin'] for e in players]==[e['origin'] for e in tt]
assert [e.get('angle','0') for e in players]==[e.get('angle','0') for e in tt]
assert all(not e.get('targetname') for e in players)
assert [e['origin'] for e in entities if e['classname']=='info_player_deathmatch']==[e['origin'] for e in tt]
markers={e['targetname']:e for e in entities if 'targetname' in e}
for i,spawn in enumerate(ct):
    assert markers[f'mansion_ct_{i:02}']['origin']==spawn['origin']
    goal=markers[f'mansion_route_{i:02}_00']
    seen=set()
    while 'target' in goal:
        assert goal['targetname'] not in seen
        seen.add(goal['targetname'])
        goal=markers[goal['target']]
    assert tuple(map(float,goal['origin'].split()))==(64,-256,56.1)
assert not any(e['classname'].startswith('monster_') for e in entities)
assert not any(e['classname'] in ('hostage_entity','trigger_camera') for e in entities)
loadout=json.loads((MOD/'mansion-placement.json').read_text())['loadout']
assert not any(e['classname'].startswith('weapon_') or e.get('targetname','').startswith('mansion_loadout_') for e in entities)
sky=json.loads((MOD/'mansion-placement.json').read_text())['sky']
assert bsp.entities[0]['skyname']==sky and bsp.entities[0]['wad']=='mansion_textures.wad'
for face in ('bk','dn','ft','lf','rt','up'):
    assert (MOD/f'gfx/env/city{face}.tga').read_bytes()==(MOD/f'assets-source/mansion/valve_addon/gfx/env/city{face}.tga').read_bytes()
    assert (MOD/f'gfx/env/{sky}{face}.tga').is_file()
    if sky=='cremation_mountains':
        header=(MOD/f'gfx/env/{sky}{face}.tga').read_bytes()[:18]
        assert struct.unpack_from('<HHBB',header,12)==(256,256,24,0)
wad=(MOD/'mansion_textures.wad').read_bytes()
magic,n,offset=struct.unpack_from('<4sii',wad)
assert magic==b'WAD3'
names={struct.unpack_from('<iiiBBH16s',wad,offset+32*i)[-1].split(b'\0')[0].lower() for i in range(n)}
textures=bsp.lump(2)
for i in range(struct.unpack_from('<i',textures)[0]):
    off=struct.unpack_from('<i',textures,4+4*i)[0]
    if off>=0:assert textures[off:off+16].split(b'\0')[0].lower() in names
assert 'map cs_mansion' in (MOD/'cremation.cfg').read_text()
for key in ('startmap','trainmap'):
    assert f'{key} "cs_mansion"' in (MOD/'liblist.gam').read_text()
print('PASS: unchanged geometry, 10 exact TT starts, 10 exact CT markers/routes, no ground weapons, all textures, six exact sky faces and Mansion defaults.')
