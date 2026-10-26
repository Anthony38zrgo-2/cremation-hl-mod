"""Rebuild Mansion's marked courtyard as real radioactive pit/island brushes."""
from pathlib import Path
import json
import math
import shutil
import struct
import subprocess
import numpy as np
from shapely.geometry import Polygon, box as shape_box
from shapely.ops import unary_union, triangulate
from map_brushes import read_map, write_map, box, prism
from bsp import BSP
from update_reactor_return_portal import STATION,PORTAL,EXIT,write_portal_sprite

MOD=Path(__file__).resolve().parents[1]
ROOT=MOD.parent
SOURCE=MOD/'assets-source/mansion-reactor'
TOOLS=MOD/'tools/map-tools/sdhlt/sdhlt-v1.3.1/tools/Win64'
FLOOR=-424
ISLAND=(736,-1056,992,-864) # 32-unit edge around the small house.
NORTH=[(232,144),(528,144),(880,0),(880,-160),(1152,-160),(1152,448),(232,448)]
SOUTH=[(232,-1184),(1152,-1184),(1152,-416),(880,-416),(880,-560),(528,-704),(232,-704)]

def entity(keys,brushes=()):return dict(keys=keys,brushes=list(brushes))
def rect(x0,y0,x1,y1):return [(x0,y0),(x1,y0),(x1,y1),(x0,y1)]

def construct():
    current=SOURCE/'reactor-layout.json'
    if current.exists() and json.loads(current.read_text()).get('four_fronts'):
        from build_mansion_cross import construct as construct_cross
        return construct_cross()
    entities=read_map(SOURCE/'cs_mansion.map')
    # Connect the marked north/south excavations and remove the central pool.
    # A solid central bridge retains the CT-to-mansion route above the pit.
    region=unary_union([Polygon(NORTH),Polygon(SOUTH),shape_box(528,-704,1152,144),shape_box(304,-496,688,-64)])
    island=shape_box(*ISLAND)
    cut_region=region.difference(island)
    triangles=[]
    for tri in triangulate(cut_region):
        if cut_region.covers(tri.representative_point()) and abs(tri.intersection(cut_region).area-tri.area)<.1:
            triangles.append(list(tri.exterior.coords)[:-1])
    assert abs(sum(Polygon(t).area for t in triangles)-cut_region.area)<.1
    cutters=[prism(t,FLOOR-32,400,'c1a3wall05') for t in triangles]
    # Clear the complete old pool rim and any remaining wall fragments from
    # the bridge's standing volume before adding the new solid bridge slab.
    cutters.append(box((224,-368,24),(1216,-208,400),'c1a3wall05'))
    cut_bounds=[c.bounds for c in cutters]
    removed=0
    for e in entities:
        output=[]
        for brush in e['brushes']:
            names=[f.texture.split()[0].upper() for f in brush.faces]
            if e['keys']['classname']=='worldspawn' and ('ORIGIN' in names or any(n.startswith('!') for n in names)):
                continue
            pieces=[brush]
            for cutter,(lo,hi) in zip(cutters,cut_bounds):
                following=[]
                for piece in pieces:
                    p0,p1=piece.bounds
                    if np.any(p1<=lo+.001) or np.any(p0>=hi-.001):following.append(piece);continue
                    result=piece.subtract(cutter)
                    if len(result)!=1 or result[0] is not piece:removed+=1
                    following.extend(result)
                pieces=following
                if not pieces:break
            output.extend(pieces)
        e['brushes']=output
    # Drop brush entities that excavation removed completely; point lights
    # buried in the former obstacles are replaced with reactor lighting.
    original=read_map(SOURCE/'cs_mansion.map')
    brush_names={e['keys'].get('targetname','') for e in original if e['brushes']}
    kept=[]
    for e,old in zip(entities,original):
        if old['brushes'] and not e['brushes'] and e['keys']['classname']!='worldspawn':continue
        cls=e['keys']['classname']
        if cls=='info_node':continue
        if cls=='info_target' and e['keys'].get('targetname','').startswith('mansion_route_'):continue
        if cls=='light' and 'origin' in e['keys']:
            p=list(map(float,e['keys']['origin'].split()))
            if region.contains(__import__('shapely').geometry.Point(p[:2])):continue
        kept.append(e)
    entities=kept
    for e in entities:
        if any(any(f.texture.split()[0].upper()=='ORIGIN' for f in b.faces) for b in e['brushes']):
            e['keys'].pop('origin',None)
    world=entities[0]
    world['keys']['wad']=';'.join(str(ROOT/p).replace('\\','/') for p in ['Cremation/mansion_textures.wad','valve/halflife.wad','valve/liquids.wad'])
    # Concrete pit floor plus native animated radioactive liquid at the bottom.
    for tri in triangles:
        world['brushes'].append(prism(tri,FLOOR-32,FLOOR,'c1a4_silo2b'))
        entities.append(entity(dict(classname='func_water',skin='-4',WaveHeight='1',rendermode='0',renderamt='255'),[prism(tri,FLOOR,FLOOR+24,'!RADIO')]))
    # Retaining walls seal the excavated volume independently of old brush data.
    for loop in [list(region.exterior.coords),list(island.exterior.coords)]:
        is_island=loop==list(island.exterior.coords)
        for a,c in zip(loop,loop[1:]):
            p=np.array(a);q=np.array(c);v=q-p;n=np.array([-v[1],v[0]])/np.linalg.norm(v)
            # Exterior ring is CCW; island ring is also CCW. Extend walls into
            # solid ground outside the pit, or into the island, respectively.
            mid=(p+q)/2
            from shapely.geometry import Point
            if region.contains(Point(*(mid+n*4))):n=-n
            if is_island:
                n=np.array([-v[1],v[0]])/np.linalg.norm(v)
            polygon=[p,q,q+n*24,p+n*24]
            wall=prism(polygon,FLOOR-32,24,'c1a3wall05')
            world['brushes'].append(wall)
            world['brushes'].append(prism(polygon,FLOOR+24,FLOOR+40,'SCROLLTOXIC'))
    world['brushes'].append(box((224,-368,0),(1216,-208,24),'crete3_wall02'))
    # Island floor and jump platform are actual collidable map geometry.
    world['brushes'].append(box((ISLAND[0],ISLAND[1],FLOOR-32),(ISLAND[2],ISLAND[3],24),'crete3_wall02'))
    world['brushes'].append(box((184,-1112,24),(248,-1048,28),'SCROLLTOXIC'))
    entities += [entity(dict(classname='info_target',targetname='reactor_jump_pad',origin='216 -1080 28')),
                 entity(dict(classname='info_target',targetname='reactor_jump_destination',origin='864 -960 205')),
                 entity(dict(classname='info_target',targetname='reactor_pickup_0',origin=' '.join(map(str,STATION)))),
                 entity(dict(classname='info_target',targetname='reactor_return_portal',origin=' '.join(map(str,PORTAL)))),
                 entity(dict(classname='info_target',targetname='reactor_return_exit',origin=' '.join(map(str,EXIT))))]
    # Compile-time green lights illuminate the floor and the sides of the shaft.
    from shapely.geometry import Point
    for x in range(320,1153,192):
        for y in range(-1088,449,192):
            if cut_region.contains(Point(x,y)):
                entities.append(entity(dict(classname='light',origin=f'{x} {y} {FLOOR+96}',_light='35 255 55 250')))
    entities.append(entity(dict(classname='light',origin='216 -1080 88',_light='100 255 80 170')))
    for x in (816,912):entities.append(entity(dict(classname='light',origin=f'{x} -960 88',_light='255 200 55 110')))
    for i,spawn in enumerate(json.loads((MOD/'mansion-placement.json').read_text())['ct_spawns']):
        x,y,z=map(float,spawn['origin'].split())
        points=[(x,y,24.1),(1280,y,24.1),(1280,-288,24.1),(1088,-288,24.1),(256,-288,24.1),(208,-256,24.1),(176,-256,40.1),(160,-256,48.1),(64,-256,56.1)]
        for j,p in enumerate(points):
            keys=dict(classname='info_target',targetname=f'mansion_route_{i:02}_{j:02}',origin=' '.join(map(str,p)))
            if j<len(points)-1:keys['target']=f'mansion_route_{i:02}_{j+1:02}'
            entities.append(entity(keys))
    for x in range(256,1281,128):entities.append(entity(dict(classname='info_node',origin=f'{x} -288 32')))
    for x in (-640,-448,-256,-64):
        for y in (-640,-256,64):entities.append(entity(dict(classname='info_node',origin=f'{x} {y} 64')))
    path=SOURCE/'cs_mansion_reactor.map'
    write_map(path,entities)
    (SOURCE/'cs_mansion_reactor.rad').write_text('!RADIO 30 255 45 150\nSCROLLTOXIC 45 255 65 100\n')
    (SOURCE/'reactor-layout.json').write_text(json.dumps(dict(depth=448,floor=FLOOR,island=ISLAND,regions=[list(region.exterior.coords)],triangles=triangles,pad=[216,-1080,28],destination=[864,-960,205],stations=[STATION],portal=PORTAL,portal_exit=EXIT,bridge=[224,-368,1216,-208],brush_cuts=removed),indent=2)+'\n')
    outline=list(region.exterior.coords)[:-1]
    (MOD/'source/reactor_layout.h').write_text('#pragma once\nstatic const float reactorIslands[][4]={{'+','.join(f'{v}.f' for v in ISLAND)+'}};\nstatic const float reactorSafeRects[][4]={{'+','.join(f'{v}.f' for v in ISLAND)+'}};\nstatic const float reactorOutline[][2]={'+','.join('{'+f'{x:.5f}f,{y:.5f}f'+'}' for x,y in outline)+'};\n')
    print('Constructed',path.name,'brushes',sum(len(e['brushes']) for e in entities),'cuts',removed)
    # Tiny golden square sprite, a code-defined particle primitive.
    size=16
    data=bytearray(struct.pack('<4i f 3i f i',0x50534449,2,2,1,12.,size,size,1,0.,0))
    data.extend(struct.pack('<H',256))
    data.extend(bytes(c for i in range(256) for c in (i,round(i*.75),round(i*.12))))
    data.extend(struct.pack('<5i',0,-8,8,size,size));data.extend(bytes([255]*(size*size)))
    (MOD/'sprites/cremation/pickup_gold_square.spr').write_bytes(data)
    write_portal_sprite()
    return path

def compile_map(path):
    layout=SOURCE/'reactor-layout.json'
    rad=['-bounce','2','-extra']
    if layout.exists() and json.loads(layout.read_text()).get('four_fronts'):rad+=['-chop','128','-texchop','256']
    for tool,args in [('sdHLCSG_x64.exe',['-nowadtextures','-cliptype','precise']),('sdHLBSP_x64.exe',[]),('sdHLVIS_x64.exe',['-fast']),('sdHLRAD_x64.exe',rad)]:
        log=SOURCE/(tool+'.log')
        with log.open('wb') as output:
            result=subprocess.run([str(TOOLS/tool),'-console','0','-threads','4',*args,str(path)],cwd=SOURCE,stdout=output,stderr=subprocess.STDOUT)
        if result.returncode:
            print(log.read_text(errors='replace')[-6000:]);raise RuntimeError(f'{tool} failed')
        print(tool,'PASS')
    output=path.with_suffix('.bsp')
    if (MOD/'assets-source/truth-exterior').is_dir():
        from preserve_mansion_interior import preserve
        preserve(output)
    return output

def install_map(output):
    bsp=BSP(output)
    assert bsp.contents((1000,300,-200))==-1
    assert bsp.contents((500,-288,16))==-2
    assert bsp.contents((500,-288,-200))==-1
    layout=json.loads((SOURCE/'reactor-layout.json').read_text())
    for x0,y0,x1,y1 in layout.get('islands',[layout['island']]):
        assert bsp.contents(((x0+x1)/2,(y0+y1)/2,-200))==-2
    assert bsp.contents((1000,300,FLOOR-1))==-2
    shutil.copy2(output,MOD/'maps/cs_mansion.bsp')
    path=MOD/'mansion-placement.json';data=json.loads(path.read_text())
    data['geometry_revision']='reactor';data['reactor']=json.loads((SOURCE/'reactor-layout.json').read_text())
    if data['reactor'].get('four_fronts'):data['ct_spawns']=data['reactor']['spawn_points']
    data['waves']=[dict(normal=40),dict(normal=20,quake=20),dict(normal=20,quake=20,fast=20,poison=20),dict(normal=10,quake=10,fast=10,poison=10,cultist=40)]
    for wave in data['waves']:wave['czero']=1
    data['special_enemy']=dict(classname='monster_czero_zombie',per_wave=1,spawn='random_unoccupied_ct',weapon='Leviathan Axe; Glock after planting',model_folder='0',smoke_per_spawn=1,smoke_trigger='four_zombies_near_mansion',flash_per_spawn=1,bomb_seconds=35,plant_seconds=3,defuse_seconds=5,infinite_glock_reserve=True,radar_hidden=True)
    data['entities']=len(bsp.entities);data['geometry_unchanged']=False
    routes=[]
    for i in range(len(data['ct_spawns'])):
        nodes=sorted((e for e in bsp.entities if e.get('targetname','').startswith(f'mansion_route_{i:02}_')),key=lambda e:e['targetname'])
        routes.append([list(map(float,e['origin'].split())) for e in nodes])
    data['routes']=routes
    path.write_text(json.dumps(data,indent=2)+'\n')
    print('Installed radioactive Mansion with four fronts and two reward islands.' if data['reactor'].get('four_fronts') else 'Installed radioactive Mansion, with central bridge and isolated reward house.')

if __name__=='__main__':
    output=compile_map(construct())
    install_map(output)
