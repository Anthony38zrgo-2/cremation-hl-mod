"""Four symmetric horde fronts and a mirrored southwest powerup island."""
from pathlib import Path
import copy
import json
import re
import struct
import numpy as np
from map_brushes import Brush,Face,box,read_map,write_map
from build_mansion_reactor import MOD,ROOT,SOURCE,compile_map,install_map,entity
from update_reactor_return_portal import write_portal_sprite
from truth_exterior import exterior_materials,platform_material,add_garden,add_perimeter,add_pagoda_roofs

CENTER=(-352,-256)
CENTRAL=(-1184,-1088,480,576)
ARENA=(-2912,-2816,2208,2304)
ISLANDS=[(960,1088,1216,1280),(-1920,-1792,-1664,-1600)]
DIRECTIONS=[(0,1),(1,0),(0,-1),(-1,0)] # N,E,S,W; interleaved appearances.

def clipped_to(brush,cutter):
    result=brush
    for plane in cutter.faces:
        result=result.clipped(plane)
        if result is None:return None
    return result.compact()

def transformed(brush,rotation,translation):
    translation=np.array(translation,dtype=float)
    result=[]
    for face in brush.faces:
        normal=rotation@face.normal;distance=face.distance+normal@translation
        def axis(match):
            values=list(map(float,match.group(1).split()));uv=rotation@np.array(values[:3]);shift=values[3]-uv@translation
            return '[ '+' '.join(f'{v:.7f}' for v in [*uv,shift])+' ]'
        texture=re.sub(r'\[([^]]+)\]',axis,face.texture)
        result.append(Face(normal,float(distance),texture))
    return Brush(result)

def sprite_square(name,color):
    size=16;data=bytearray(struct.pack('<4i f 3i f i',0x50534449,2,2,1,12.,size,size,1,0.,0))
    data.extend(struct.pack('<H',256));data.extend(bytes(round(c*i/255) for i in range(256) for c in color))
    data.extend(struct.pack('<5i',0,-8,8,size,size));data.extend(bytes([255]*256))
    (MOD/f'sprites/cremation/{name}.spr').write_bytes(data)

def construct():
    original=read_map(SOURCE/'cs_mansion.map')
    house_clip=box((-736,-800,-176),(208,288,600),'NULL')
    world=entity(copy.deepcopy(original[0]['keys']))
    world['keys'].update(wad=';'.join(str(ROOT/p).replace('\\','/') for p in ['Cremation/mansion_textures.wad','Cremation/truth_textures.wad','valve/halflife.wad','valve/liquids.wad']),MaxRange='8192',skyname='city')
    for brush in original[0]['brushes']:
        names=[f.texture.split()[0].upper() for f in brush.faces]
        if 'SKY' in names or 'ORIGIN' in names or any(n.startswith('!') for n in names):continue
        kept=clipped_to(brush,house_clip)
        if kept:
            # Reconstructed foundation strips have NULL caps which become
            # exposed after removing the original yard. Give them real faces.
            for face in kept.faces:
                if face.texture.split()[0].upper()=='NULL' and kept.bounds[1][2]<=60:face.texture='crete3_wall02'
            world['brushes'].append(kept)
    # Remove the truncated original south stairs before rebuilding the full
    # approach; otherwise remnants protrude beside the new treads.
    stair_cut=box((-432,-848,24),(-224,-768,56),'crete3_wall02')
    world['brushes']=[piece for brush in world['brushes'] for piece in brush.subtract(stair_cut)]
    # The reward islands are open pavilions now: no original shed walls,
    # doors or intermediate roof beneath the new tiled roof and posts.
    entities=[world]
    from vostok_cover import add_cover
    add_cover(entities,MOD,entity)
    for old in original[1:]:
        keys=copy.deepcopy(old['keys']);cls=keys['classname']
        if old['brushes']:
            physical=[];origins=[]
            for brush in old['brushes']:
                if any(f.texture.split()[0].upper()=='ORIGIN' for f in brush.faces):origins.append(brush);continue
                kept=clipped_to(brush,house_clip)
                if kept:physical.append(kept)
            if physical:
                if origins:keys.pop('origin',None)
                entities.append(entity(keys,physical+origins))
            continue
        name=keys.get('targetname','')
        if cls in ('info_player_start','info_player_deathmatch','light_environment') or name=='mansion_wave_state':entities.append(entity(keys));continue
        if name.startswith(('mansion_route_','mansion_ct_','reactor_')) or cls=='info_node':continue
        if cls=='light' and 'origin' in keys:
            x,y,z=map(float,keys['origin'].split())
            if -736<x<208 and -800<y<288:entities.append(entity(keys))
    # Fully enclosed arena. The original outer walls no longer restrict expansion.
    x0,y0,x1,y1=ARENA;z0=-456;top=896
    # Space behind the brick perimeter for the second forest layer. The
    # playable outline, pit, hubs and navigation remain at their old limits.
    sx0,sy0,sx1,sy1=x0-192,y0-192,x1+192,y1+192
    for lo,hi in [((sx0-32,sy0-32,z0),(sx0,sy1+32,top+32)),((sx1,sy0-32,z0),(sx1+32,sy1+32,top+32)),((sx0,sy0-32,z0),(sx1,sy0,top+32)),((sx0,sy1,z0),(sx1,sy1+32,top+32)),((sx0,sy0,top),(sx1,sy1,top+32)),((sx0,sy0,z0-32),(sx1,sy1,z0))]:world['brushes'].append(box(lo,hi,'sky'))
    world['brushes'].append(box((x0,y0,-456),(x1,y1,-424),'c1a4_silo2b'))
    entities.append(entity(dict(classname='func_water',skin='-4',WaveHeight='1'),[box((x0,y0,-424),(x1,y1,-400),'!RADIO')]))
    hubs=[];bridges=[];walls=[]
    for dx,dy in DIRECTIONS:
        hx=CENTER[0]+2048*dx;hy=CENTER[1]+2048*dy
        def local_rect(u0,v0,u1,v1):
            corners=[(hx+dx*v+dy*u,hy+dy*v-dx*u) for u in (u0,u1) for v in (v0,v1)]
            return (min(p[0] for p in corners),min(p[1] for p in corners),max(p[0] for p in corners),max(p[1] for p in corners))
        hubs.append(local_rect(-1280,-384,1280,384))
        # Four walls, with a 320-unit opening in the side facing the bridge.
        walls.extend(local_rect(*r) for r in [(-1280,-384,-160,-360),(160,-384,1280,-360),(-1280,360,1280,384),(-1280,-360,-1256,360),(1256,-360,1280,360)])
        if dx:bridges.append((min(CENTER[0]+832*dx,hx-384*dx),CENTER[1]-128,max(CENTER[0]+832*dx,hx-384*dx),CENTER[1]+128))
        else:bridges.append((CENTER[0]-128,min(CENTER[1]+832*dy,hy-384*dy),CENTER[0]+128,max(CENTER[1]+832*dy,hy-384*dy)))
    safe=[CENTRAL,*hubs,*ISLANDS]
    for r in safe:
        platform=box((r[0],r[1],-456),(r[2],r[3],24),'crete3_wall02')
        top_texture='-0out_grss1' if r==CENTRAL else 'woods_plank_org' if r in ISLANDS else 'jpn_walk43'
        platform_material(platform,top_texture)
        world['brushes'].append(platform)
        # Decoration only: a solid one-unit lip catches monster hulls above
        # the lethal floor. func_illusionary keeps light/texture without support.
        trim=[]
        for lo,hi in [((r[0]-1,r[1]-1,-400),(r[2]+1,r[1]+1,-384)),((r[0]-1,r[3]-1,-400),(r[2]+1,r[3]+1,-384)),((r[0]-1,r[1],-400),(r[0]+1,r[3],-384)),((r[2]-1,r[1],-400),(r[2]+1,r[3],-384))]:trim.append(box(lo,hi,'SCROLLTOXIC'))
        entities.append(entity(dict(classname='func_illusionary',targetname=f'reactor_trim_{len(entities)}',skin='0',rendermode='0'),trim))
    for r in bridges:world['brushes'].append(box((r[0],r[1],0),(r[2],r[3],24),'woods_plank_org'))
    for r in walls:world['brushes'].append(box((r[0],r[1],24),(r[2],r[3],216),'outter_wall3'))
    # The front porch keeps its original height and has four 8-unit treads.
    for x,z in [(200,32),(184,40),(168,48),(152,56)]:world['brushes'].append(box((112,-312,24),(x,-200,z),'crete3_wall02'))
    # Restore a solid, textured approach to the south side door, outside the
    # original clipping boundary. Keep every rise within the native step height.
    for y,z in [(-832,32),(-816,40),(-800,48),(-784,56)]:world['brushes'].append(box((-384,y,24),(-264,-752,z),'crete3_wall02'))
    for px,py in ((448,544),(-1152,-1056)):world['brushes'].append(box((px-32,py-32,24),(px+32,py+32,28),'SCROLLTOXIC'))
    points={'reactor_jump_pad':[448,544,28],'reactor_jump_destination':[1088,1184,257],
            'reactor_powerup_jump_pad':[-1152,-1056,28],'reactor_powerup_jump_destination':[-1792,-1696,257],
            'reactor_pickup_0':[1136,1184,65],'reactor_powerup_0':[-1840,-1696,65],
            'reactor_return_portal':[1040,1184,72],'reactor_powerup_portal':[-1744,-1696,72],
            'reactor_return_exit':[-784,-272,61]}
    entities += [entity(dict(classname='info_target',targetname=name,origin=' '.join(map(str,p)))) for name,p in points.items()]
    for name in ('reactor_pickup_0','reactor_powerup_0','reactor_return_portal','reactor_powerup_portal'):
        x,y,_=points[name];entities.append(entity(dict(classname='light',origin=f'{x} {y} 88',_light='120 180 255 100' if x<0 else '255 200 55 100')))
    for x in range(x0+256,x1,512):
        for y in range(y0+256,y1,512):
            if not any(a<=x<=c and b<=y<=d for a,b,c,d in safe):entities.append(entity(dict(classname='light',origin=f'{x} {y} -320',_light='35 255 55 240')))
    spawn_points=[];route_list=[];node_points=set()
    for local in range(10):
        radial=-128 if local<5 else 128;tangent=(local%5-2)*400
        for sector,(dx,dy) in enumerate(DIRECTIONS):
            hx=CENTER[0]+2048*dx;hy=CENTER[1]+2048*dy
            x=hx+dx*radial+dy*tangent;y=hy+dy*radial-dx*tangent
            index=len(spawn_points);spawn_points.append(dict(origin=f'{x} {y} 64',angle=str((270-sector*90)%360),classname='info_player_start',sector=sector))
            entities.append(entity(dict(classname='info_target',targetname=f'mansion_ct_{index:02}',origin=f'{x} {y} 64',angles=f'0 {(270-sector*90)%360} 0')))
            inner=(CENTER[0]+800*dx,CENTER[1]+800*dy,24.1)
            funnel=(hx-256*dx+dy*tangent*.05,hy-256*dy-dx*tangent*.05,24.1)
            route=[(x,y,24.1),funnel,inner]
            if sector==0:route += [(-352,416,24.1),(208,416,24.1),(208,-256,24.1)]
            elif sector==1:route += [(256,-256,24.1),(208,-256,24.1)]
            elif sector==2:route += [(-352,-976,24.1),(208,-976,24.1),(208,-256,24.1)]
            else:route += [(-1056,-256,24.1),(-1056,416,24.1),(208,416,24.1),(208,-256,24.1)]
            route += [(176,-256,40.1),(160,-256,48.1),(64,-256,56.1)]
            route_list.append(route)
            for j,p in enumerate(route):
                keys=dict(classname='info_target',targetname=f'mansion_route_{index:02}_{j:02}',origin=' '.join(map(str,p)))
                if j+1<len(route):keys['target']=f'mansion_route_{index:02}_{j+1:02}'
                entities.append(entity(keys))
            for a,b in zip(route,route[1:]):
                steps=max(1,int(np.ceil(np.linalg.norm(np.array(b)-a)/128)))
                for k in range(steps):node_points.add(tuple(round(a[i]+(b[i]-a[i])*k/steps,2) for i in range(3)))
    # GoldSrc permits only 128 initial visible links per node. Shared routes
    # previously created 403 near-duplicate nodes and overflowed that pool.
    # Farthest-point sampling preserves coverage and guarantees <=95 links.
    candidates=sorted(node_points);selected=[candidates.pop(0)]
    while candidates and len(selected)<96:
        p=max(candidates,key=lambda p:min(sum((p[k]-q[k])**2 for k in range(3)) for q in selected))
        if min(sum((p[k]-q[k])**2 for k in range(3)) for q in selected)<192**2:break
        selected.append(p);candidates.remove(p)
    for p in selected:entities.append(entity(dict(classname='info_node',origin=f'{p[0]} {p[1]} {p[2]+8}')))
    exterior_materials(entities,MOD)
    add_garden(entities,MOD,entity)
    add_perimeter(entities,world,ARENA,entity)
    add_pagoda_roofs(entities,world,entity)
    path=SOURCE/'cs_mansion_reactor.map';write_map(path,entities)
    (SOURCE/'cs_mansion_reactor.rad').write_text('!RADIO 30 255 45 150\nSCROLLTOXIC 45 255 65 100\n')
    outline=[[x0,y0],[x1,y0],[x1,y1],[x0,y1],[x0,y0]]
    layout=dict(four_fronts=True,diagonal_houses=True,hub_walls=walls,hub_wall_height=192,hub_opening=320,center=CENTER,arena=ARENA,central=CENTRAL,depth=448,floor=-424,island=ISLANDS[0],islands=ISLANDS,regions=[outline],safe_rects=safe,
                bridges=bridges,bridge=bridges[1],spawn_hubs=hubs,spawn_points=spawn_points,routes=route_list,
                pad=points['reactor_jump_pad'],destination=points['reactor_jump_destination'],powerup_pad=points['reactor_powerup_jump_pad'],powerup_destination=points['reactor_powerup_jump_destination'],
                stations=[points['reactor_pickup_0']],powerup_stations=[points['reactor_powerup_0']],portal=points['reactor_return_portal'],powerup_portal=points['reactor_powerup_portal'],portal_exit=points['reactor_return_exit'])
    (SOURCE/'reactor-layout.json').write_text(json.dumps(layout,indent=2)+'\n')
    rows=lambda data:','.join('{'+','.join(f'{v:.5f}f' for v in row)+'}' for row in data)
    (MOD/'source/reactor_layout.h').write_text('#pragma once\nstatic const float reactorIslands[][4]={'+rows(ISLANDS)+'};\nstatic const float reactorSafeRects[][4]={'+rows(safe)+'};\nstatic const float reactorOutline[][2]={'+rows(outline[:-1])+'};\n')
    sprite_square('pickup_gold_square',(255,191,31));sprite_square('powerup_blue_square',(55,200,255));write_portal_sprite()
    print('Constructed diagonal NE/SW houses and jumps; four walled 2560 x 768 spawn hubs with 320-unit gates.')
    return path

if __name__=='__main__':install_map(compile_map(construct()))
