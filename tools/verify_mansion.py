"""Check installed Mansion geometry, spawns, routes, textures, sky and defaults."""
from pathlib import Path
import collections
import hashlib
import json
import math
import struct
from bsp import BSP

MOD=Path(__file__).resolve().parents[1]
source=BSP(MOD/'assets-source/mansion/maps/cs_mansion.bsp')
bsp=BSP(MOD/'maps/cs_mansion.bsp')
placement=json.loads((MOD/'mansion-placement.json').read_text())
if placement.get('geometry_revision')=='reactor':
    assert bsp.contents((1000,300,-200))==-1
    assert bsp.contents((500,-288,16))==-2
    assert bsp.contents((500,-288,-200))==-1
    for x0,y0,x1,y1 in placement['reactor'].get('islands',[placement['reactor']['island']]):
        assert bsp.contents(((x0+x1)/2,(y0+y1)/2,-200))==-2
    assert bsp.contents((1000,300,-425))==-2
else:
    assert all(source.lump(i)==bsp.lump(i) for i in range(1,15))
entities=bsp.entities
trims=[e for e in entities if e.get('targetname','').startswith('reactor_trim_')]
assert len(trims)==7 and all(e['classname']=='func_illusionary' for e in trims)
for x0,y0,x1,y1 in placement['reactor']['safe_rects']:
    for x,y in [(x0-16.25,(y0+y1)/2),(x1+16.25,(y0+y1)/2),((x0+x1)/2,y0-16.25),((x0+x1)/2,y1+16.25)]:
        for feet in (-380,-384,-390,-399):
            assert bsp.clear((x,y,feet+36),1), (x,y,feet)
nodes=[e for e in entities if e.get('classname')=='info_node']
if placement.get('reactor',{}).get('diagonal_houses'):
    assert 1<len(nodes)<=96, len(nodes)
    for y,z in [(-824,32),(-808,40),(-792,48),(-776,56)]:
        assert bsp.contents((-324,y,z-.1),0)==-2
        assert bsp.contents((-324,y,z+.1),0)==-1
tt=[e for e in source.entities if e['classname']=='info_player_deathmatch']
ct=placement['ct_spawns'] if placement.get('reactor',{}).get('four_fronts') else [e for e in source.entities if e['classname']=='info_player_start']
players=[e for e in entities if e['classname']=='info_player_start']
assert [e['origin'] for e in players]==[e['origin'] for e in tt]
assert [e.get('angle','0') for e in players]==[e.get('angle','0') for e in tt]
assert all(not e.get('targetname') for e in players)
assert [e['origin'] for e in entities if e['classname']=='info_player_deathmatch']==[e['origin'] for e in tt]
markers={e['targetname']:e for e in entities if 'targetname' in e}
if 'portal' in placement.get('reactor',{}):
    assert 'reactor_pickup_1' not in markers
    assert list(map(float,markers['reactor_pickup_0']['origin'].split()))==placement['reactor']['stations'][0]
    assert list(map(float,markers['reactor_return_portal']['origin'].split()))==placement['reactor']['portal']
    destination=tuple(map(float,markers['reactor_return_exit']['origin'].split()))
    assert list(destination)==placement['reactor']['portal_exit'] and bsp.contents(destination,1)==-1
if placement.get('reactor',{}).get('four_fronts'):
    assert len(ct)==40
    assert len(placement['reactor']['islands'])==2 and len(placement['reactor']['bridges'])==4
    assert 'reactor_powerup_0' in markers and 'reactor_powerup_jump_pad' in markers and 'reactor_powerup_portal' in markers
    counts=collections.Counter(spawn['sector'] for spawn in ct);assert counts=={0:10,1:10,2:10,3:10}
    for spawn in ct:
        p=tuple(map(float,spawn['origin'].split()));assert bsp.contents(p,1)==-1
    layout=placement['reactor'];cx,cy=layout['center']
    diagonal=layout.get('diagonal_houses',False)
    a,b=layout['islands'];assert b==[2*cx-a[2],2*cy-a[3] if diagonal else a[1],2*cx-a[0],2*cy-a[1] if diagonal else a[3]]
    for rect in layout['spawn_hubs']:
        x0,y0,x1,y1=rect;assert sorted([x1-x0,y1-y0])==([768,2560] if diagonal else [512,512])
        assert abs((x0+x1)/2-cx)+abs((y0+y1)/2-cy)==2048
    for rect in layout['bridges']:
        x0,y0,x1,y1=rect;assert sorted([x1-x0,y1-y0])==[256,832 if diagonal else 960]
        for i in range(41):
            p=(x0+(x1-x0)*i/40,(y0+y1)/2,64) if x1-x0>y1-y0 else ((x0+x1)/2,y0+(y1-y0)*i/40,64)
            assert bsp.contents(p,1)==-1
            assert bsp.contents((p[0],p[1],16),0)==-2
    hx,hy=(a[0]+a[2])/2,(a[1]+a[3])/2
    for x in (hx-64,hx,hx+64):
        for y in (hy-40,hy,hy+40):
            for z in (32,64,128,160,176):
                assert bsp.contents((x,y,z),0)==bsp.contents((2*cx-x,2*cy-y if diagonal else y,z),0)
    if diagonal:
        assert len(layout['hub_walls'])==20 and layout['hub_opening']==320
        for x0,y0,x1,y1 in layout['hub_walls']:
            assert bsp.contents(((x0+x1)/2,(y0+y1)/2,120),0)==-2
        for dx,dy in ((0,1),(1,0),(0,-1),(-1,0)):
            for offset in range(1584,1857,16):
                assert bsp.contents((cx+dx*offset,cy+dy*offset,64),1)==-1
        for route in layout['routes']:
            for start,end in zip(route[:2],route[1:3]):
                steps=max(1,math.ceil(math.dist(start,end)/32))
                for step in range(steps+1):
                    p=[start[k]+(end[k]-start[k])*step/steps for k in range(3)]
                    assert bsp.contents((p[0],p[1],p[2]+36),1)==-1
                    assert bsp.contents((p[0],p[1],16),0)==-2
        for key,dest in [('pad','destination'),('powerup_pad','powerup_destination')]:
            p,q=layout[key],layout[dest]
            assert p[0]-cx==p[1]-cy and q[0]-cx==q[1]-cy
            assert bsp.contents((p[0],p[1],20),0)==-2
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
assert bsp.entities[0]['skyname']==sky
if placement.get('geometry_revision')!='reactor':assert 'mansion_textures.wad' in bsp.entities[0].get('wad','')
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
    if off>=0:
        if placement.get('geometry_revision')=='reactor':assert struct.unpack_from('<i',textures,off+24)[0]>0
        else:assert textures[off:off+16].split(b'\0')[0].lower() in names
assert not any(line.strip().lower() == 'map cs_mansion'
               for line in (MOD/'cremation.cfg').read_text().splitlines())
for key in ('startmap','trainmap'):
    assert f'{key} "cs_mansion"' in (MOD/'liblist.gam').read_text()
print(f'PASS: validated geometry, 10 exact TT starts, {len(ct)} CT markers/routes, symmetric hubs/bridges/houses, no ground weapons, textures, original sky and defaults.')
