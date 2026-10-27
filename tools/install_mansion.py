"""Install the supplied Mansion/sky archives without running archive content."""
from pathlib import Path
import collections
import functools
import hashlib
import heapq
import json
import math
import shutil
import struct
from bsp import BSP

MOD = Path(__file__).resolve().parents[1]
ROOT = MOD.parent
SOURCE = MOD / 'assets-source/mansion'


def install():
    current=MOD/'mansion-placement.json'
    if current.exists() and json.loads(current.read_text()).get('geometry_revision')=='reactor':
        from build_mansion_reactor import install_map
        install_map(MOD/'assets-source/mansion-reactor/cs_mansion_reactor.bsp')
        return
    b = BSP(SOURCE / 'maps/cs_mansion.bsp')
    b.solids += [int(e['model'][1:]) for e in b.entities if e['classname'] in ('func_button', 'func_pushable')]
    b.solids = [m for m in b.solids if m == 0 or b.models[m][3] >= 0]
    original = b.entities
    tt = [e for e in original if e['classname'] == 'info_player_deathmatch']
    ct = [e for e in original if e['classname'] == 'info_player_start']
    assert len(tt) == len(ct) == 10
    entities = [dict(e) for e in original if e['classname'] not in
                ('info_player_start', 'info_player_deathmatch', 'hostage_entity', 'trigger_camera')]
    world = entities[0]
    mountain_sky = False
    world['skyname'] = 'city'
    world['wad'] = 'mansion_textures.wad'
    world.pop('mapteams', None)

    # Bundle exactly the original texture data, including animation companions.
    texture_lump = b.lump(2)
    names = set()
    for i in range(struct.unpack_from('<i', texture_lump)[0]):
        offset = struct.unpack_from('<i', texture_lump, 4 + 4*i)[0]
        if offset >= 0:
            names.add(texture_lump[offset:offset+16].split(b'\0')[0].lower())
    textures = {}
    for wad in (ROOT/'valve/halflife.wad', ROOT/'valve/liquids.wad'):
        raw = wad.read_bytes()
        magic, count, directory = struct.unpack_from('<4sii', raw)
        assert magic == b'WAD3'
        for i in range(count):
            pos, size, unpacked, kind, compression, pad, name = struct.unpack_from('<iiiBBH16s', raw, directory+32*i)
            key = name.split(b'\0')[0].lower()
            if key in names or (key[:1] in (b'+', b'-') and any(n[2:] == key[2:] for n in names if n[:1] == key[:1])):
                assert compression == 0
                textures[key] = (raw[pos:pos+size], kind, name)
    assert names <= textures.keys(), names - textures.keys()
    data = bytearray(b'WAD3' + bytes(8))
    directory = []
    for payload, kind, name in textures.values():
        directory.append(struct.pack('<iiiBBH16s', len(data), len(payload), len(payload), kind, 0, 0, name))
        data.extend(payload)
    struct.pack_into('<ii', data, 4, len(directory), len(data))
    data.extend(b''.join(directory))
    (MOD/'mansion_textures.wad').write_bytes(data)
    (MOD/'gfx/env').mkdir(parents=True, exist_ok=True)
    for face in ('bk', 'dn', 'ft', 'lf', 'rt', 'up'):
        shutil.copy2(SOURCE/f'valve_addon/gfx/env/city{face}.tga', MOD/f'gfx/env/city{face}.tga')

    loadout = json.loads((MOD/'placement.json').read_text())['loadout']
    for i, spawn in enumerate(tt):
        # Ravenholm's single-player selector expects unnamed starts. Retain
        # the native multiplayer class at the same TT point for compatibility.
        player = dict(spawn, classname='info_player_start')
        entities.append(player)
        entities.append(dict(spawn))
    for i, spawn in enumerate(ct):
        entities.append(dict(classname='info_target', targetname=f'mansion_ct_{i:02}',
                             origin=spawn['origin'], angles='0 180 0'))
    entities.append(dict(classname='info_target', targetname='mansion_wave_state', origin='0 0 0'))

    # Human hull plus the extra Quake-zombie height. Routes follow real floors,
    # check steps and swept volume, and finish at the main door's front porch.
    def floor(x, y):
        last = None
        for z in range(160, -80, -4):
            if b.clear((x, y, z), 1):
                last = z
            elif last is not None:
                lo, hi = z, last
                for _ in range(10):
                    mid = (lo+hi)/2
                    if b.clear((x, y, mid), 1): hi = mid
                    else: lo = mid
                feet = hi-36
                # On stairs the expanded hull rests on the next tread while
                # its center is still above the previous tread.
                if any(b.contents((x, y, feet-drop)) == -2 for drop in range(2, 20, 2)):
                    return round(feet, 2)
                return None
        return None

    grid = {}
    for x in range(32, 1377, 16):
        for y in range(-1120, 449, 16):
            z = floor(x, y)
            if z is not None and all(b.clear((x+dx, y+dy, z+h), 1)
                                    for dx, dy in ((0,0),(-2,0),(2,0),(0,-2),(0,2)) for h in (36.1,68.1)):
                grid[x, y] = (x, y, z+.1)

    @functools.lru_cache(maxsize=None)
    def segment(a, c):
        if abs(a[2]-c[2]) > 18.1:
            return False
        steps = max(1, math.ceil(math.dist(a, c)/8))
        for i in range(steps+1):
            p = tuple(a[k]+(c[k]-a[k])*i/steps for k in range(3))
            # Step-up traversal uses the raised hull, as WalkMove does.
            z = max(a[2], c[2])
            if not all(b.clear((p[0]+dx, p[1]+dy, z+h), 1)
                       for dx, dy in ((0,0),(-2,0),(2,0),(0,-2),(0,2)) for h in (36.1, 68.1)):
                return False
            if not any(b.contents((p[0],p[1],z-drop)) == -2 for drop in range(2,22,2)):
                return False
        return True

    goal = (64, -256)
    assert goal in grid
    routes = []
    for index, spawn in enumerate(ct):
        x, y, _ = map(float, spawn['origin'].split())
        feet = b.floor(x, y, top=160, bottom=-80)
        start = (x, y, feet+.1)
        candidates = sorted(grid, key=lambda k: math.dist(start, grid[k]))
        anchor = next(k for k in candidates if segment(start, grid[k]))
        costs = {anchor: 0}
        previous = {}
        queue = [(math.dist(grid[anchor], grid[goal]), anchor)]
        while queue:
            _, node = heapq.heappop(queue)
            if node == goal:
                break
            for dx, dy in ((16, 0), (-16, 0), (0, 16), (0, -16)):
                nxt = (node[0]+dx, node[1]+dy)
                if nxt not in grid or not segment(grid[node], grid[nxt]):
                    continue
                cost = costs[node] + math.dist(grid[node], grid[nxt])
                if cost < costs.get(nxt, float('inf')):
                    costs[nxt] = cost
                    previous[nxt] = node
                    heapq.heappush(queue, (cost + math.dist(grid[nxt], grid[goal]), nxt))
        if goal not in costs:
            (MOD/'diagnostics/mansion-grid.json').write_text(json.dumps(dict(points=list(grid.values()), reached=[grid[k] for k in costs])))
            raise AssertionError(f'No ground route for CT {index}; grid diagnostic saved')
        nodes = [goal]
        while nodes[-1] != anchor:
            nodes.append(previous[nodes[-1]])
        points = [start] + [grid[k] for k in reversed(nodes)]
        # Greedy swept-hull smoothing, with continuous ground support checks.
        reduced = [points[0]]
        cursor = 0
        while cursor < len(points)-1:
            nxt = next(j for j in range(len(points)-1,cursor,-1) if segment(points[cursor],points[j]))
            reduced.append(points[nxt])
            cursor = nxt
        routes.append(reduced)
        for j, point in enumerate(reduced):
            entity = dict(classname='info_target', targetname=f'mansion_route_{index:02}_{j:02}',
                          origin=' '.join(map(str, point)))
            if j+1 < len(reduced):
                entity['target'] = f'mansion_route_{index:02}_{j+1:02}'
            entities.append(entity)
    # Native combat navigation can also use nodes along the approach routes.
    seen = set()
    for route in routes:
        for a, c in zip(route, route[1:]):
            for i in range(max(1, math.ceil(math.dist(a, c)/128))):
                t = i/max(1, math.ceil(math.dist(a, c)/128))
                p = tuple(round(a[k]+(c[k]-a[k])*t, 2) for k in range(3))
                key = tuple(round(v/48) for v in p)
                if key not in seen:
                    seen.add(key)
                    entities.append(dict(classname='info_node', origin=f'{p[0]} {p[1]} {p[2]+8}'))
    b.write(MOD/'maps/cs_mansion.bsp', entities)
    installed = BSP(MOD/'maps/cs_mansion.bsp')
    assert all(installed.lump(i) == b.lump(i) for i in range(1, 15))
    manifest = dict(map='cs_mansion', sky=world['skyname'], tt_spawns=tt, ct_spawns=ct, routes=routes,
                    waves=[dict(normal=20), dict(normal=10, quake=10),
                           dict(normal=10, quake=10, fast=10, poison=10),
                           dict(normal=5, quake=5, fast=5, poison=5, cultist=20)],
                    loadout=loadout, textures=len(textures), entities=len(entities),
                    geometry_unchanged=True)
    (MOD/'mansion-placement.json').write_text(json.dumps(manifest, indent=2)+'\n')
    if mountain_sky:
        from install_mountain_sky import install as install_sky
        install_sky(update_map=False)
    from install_menu import install_wave_maps
    install_wave_maps()
    for config in ('cremation.cfg', 'liblist.gam'):
        path = MOD/config
        path.write_text(path.read_text().replace('rapidcore', 'cs_mansion'))
    print(f'Installed Mansion: {len(entities)} entities, {len(textures)} textures, 10 TT / 10 CT, 10 checked routes.')


if __name__ == '__main__':
    install()
