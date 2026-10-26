"""Truth materials and garden props, restricted to the Mansion exterior."""
import copy,hashlib,json,shutil
import numpy as np
from map_brushes import box,Face

PROPS=[
    ('maple_red','tree_japan_maple_red',(-896,304,24),0),
    ('maple_orange','tree_japan_maple_orange',(352,-832,24),15),
    ('bush_0','bush_japan_bush01',(-904,-440,39.6),0),
    ('bush_1','bush_japan_bush01',(-920,-792,39.6),70),
    ('bush_2','bush_japan_bush01',(-600,344,39.6),110),
    ('bush_3','bush_japan_bush01',(352,120,39.6),270),
    ('bush_4','bush_japan_bush01',(-72,-872,39.6),30),
    ('rock_0','rock_japan_rocks2',(-936,-120,24),0),
    ('rock_1','rock_japan_rocks2',(344,288,24),90),
]

def replace(face,name):
    parts=face.texture.split(maxsplit=1)
    face.texture=name+(' '+parts[1] if len(parts)>1 else '')
    if name=='rf_tile_1':
        # Roof donor UVs stretch a different texture. Use tile-sized repeats,
        # with ridges along Y and the rows running down the X slopes.
        face.texture='rf_tile_1 [ 0 1 0 0 ] [ 1 0 0 0 ] 0 0.5 0.5'

def exterior_materials(entities,mod):
    """Keep geometry/UVs and every interior material except requested doors."""
    changes=[]
    for ei,e in enumerate(entities):
        for bi,b in enumerate(e['brushes']):
            lo,hi=b.bounds
            main=lo[0]>=-736 and hi[0]<=208 and lo[1]>=-800 and hi[1]<=288
            little=any(lo[0]>=a and hi[0]<=c and lo[1]>=bb and hi[1]<=d
                       for a,bb,c,d in [(960,1088,1216,1280),(-1920,-1792,-1664,-1600)])
            chimney=main and lo[2]>=400 and hi[2]>=500 and -260<=lo[0]<=-168 and -620<=lo[1]<=-520
            door=e['keys'].get('classname','').startswith('func_door')
            for fi,f in enumerate(b.faces):
                old=f.texture.split()[0];new=None
                if old.lower() in ('null','origin','aaatrigger','clip','sky','black'):continue
                if door and (main or little):new='woods_plank_org'
                elif main:
                    # Original interior uses fifties_*, c1a1w1d, etc. Those
                    # material families are never substituted by this pass.
                    if chimney:new='WWD003'
                    elif old=='out_wall4a':new='brk_bld_yl_1'
                    elif old=='out_roof4':new='rf_tile_1'
                elif little:
                    if old=='out_roof4':new='rf_tile_1'
                    elif old in ('out_wall4a','crete3_wall02','c1a4_silo2b'):new='outter_wall3'
                    elif 'dr1' in old:new='woods_plank_org'
                if new:
                    changes.append(dict(entity=ei,brush=bi,face=fi,old=old,new=new))
                    replace(f,new)
    folder=mod/'assets-source/truth-exterior';folder.mkdir(parents=True,exist_ok=True)
    (folder/'material-changes.json').write_text(json.dumps(changes,indent=2)+'\n')

def platform_material(brush,top='jpn_walk43'):
    for f in brush.faces:
        f.texture=top if f.normal[2]<-.9 else 'WWD003'

def add_garden(entities,mod,entity):
    manifest=[]
    for name,model,origin,yaw in PROPS:
        source=mod.parent/'czero/models/props'/f'{model}.mdl'
        target=mod/'models/cremation/truth'/source.name;target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(source,target)
        entities.append(entity(dict(classname='info_target',targetname='vostok_cover_truth_'+name,
            model=target.relative_to(mod).as_posix(),origin=' '.join(map(str,origin)),angles=f'0 {yaw} 0',framerate='10')))
        # Low shrubs remain permeable: no invisible cages for the horde.
        # Tree trunks and rocks provide narrow, permanent solid cover.
        collision=[]
        if name.startswith('maple'):collision=[((-11,-11,0),(11,11,150))]
        elif name.startswith('rock'):collision=[((-36,-38,0),(36,38,55)),((-25,-27,55),(25,27,78))]
        if collision:
            x,y,z=origin
            brushes=[box((x+a[0],y+a[1],z+a[2]),(x+b[0],y+b[1],z+b[2]),'NULL') for a,b in collision]
            entities.append(entity(dict(classname='func_wall',targetname='truth_collision_'+name),brushes))
        manifest.append(dict(name=name,source=str(source),installed=target.relative_to(mod).as_posix(),
            origin=origin,yaw=yaw,framerate=10,sha256=hashlib.sha256(target.read_bytes()).hexdigest(),collision=collision))
    folder=mod/'assets-source/truth-exterior';folder.mkdir(parents=True,exist_ok=True)
    (folder/'props.json').write_text(json.dumps(manifest,indent=2)+'\n')

def add_perimeter(entities,world,arena,entity):
    x0,y0,x1,y1=arena
    # 64 units taller than hub walls (top=216), with the original transparent
    # Truth forest above. Keep all props inside the sky enclosure.
    rects=[(x0,y0,x0+32,y1),(x1-32,y0,x1,y1),(x0+32,y0,x1-32,y0+32),(x0+32,y1-32,x1-32,y1)]
    for i,(a,b,c,d) in enumerate(rects):
        world['brushes'].append(box((a,b,-424),(c,d,280),'brk_bld_yl_1'))
        foliage=box((a,b,280),(c,d,792),'NULL')
        for f in foliage.faces:
            if abs(f.normal[2])<.1:
                axis='0 1 0 0' if abs(f.normal[0])>.5 else '1 0 0 0'
                f.texture='{bam_trees03 [ '+axis+' ] [ 0 0 -1 792 ] 0 1 1'
        entities.append(entity(dict(classname='func_illusionary',targetname=f'truth_forest_{i}',rendermode='2',renderamt='255',skin='0'),[foliage]))
    # A separate rear ring sits 128 units farther away, 64 lower, with its
    # pattern shifted horizontally. Parallax and offset gaps give depth.
    rear=[(x0-128,y0-128,x0-124,y1+128),(x1+124,y0-128,x1+128,y1+128),
          (x0-124,y0-128,x1+124,y0-124),(x0-124,y1+124,x1+124,y1+128)]
    for i,(a,b,c,d) in enumerate(rear):
        foliage=box((a,b,216),(c,d,728),'NULL')
        for f in foliage.faces:
            if abs(f.normal[2])<.1:
                axis='0 1 0 256' if abs(f.normal[0])>.5 else '1 0 0 256'
                f.texture='{bam_trees03 [ '+axis+' ] [ 0 0 -1 728 ] 0 1 1'
        entities.append(entity(dict(classname='func_illusionary',targetname=f'truth_forest_rear_{i}',rendermode='2',renderamt='255',skin='0'),[foliage]))

def add_pagoda_roofs(entities,world,entity):
    # A solid tiled gable and lifted eaves cover each original flat roof.
    # The jump destination is raised to the new ridge; doors/items stay put.
    for cx,cy in [(1088,1184),(-1792,-1696)]:
        # Four slim red-brown corner posts and ornamental eave bands.
        for dx in (-106,106):
            for dy in (-74,74):
                world['brushes'].append(box((cx+dx-3,cy+dy-3,24),(cx+dx+3,cy+dy+3,190),'woods_plank_org'))
        for dy in (-78,74):world['brushes'].append(box((cx-112,cy+dy,178),(cx+112,cy+dy+4,190),'jpntrim1'))
        # Two convex wedges form the pitched roof, matching both islands.
        for sign in (-1,1):
            a=cx;b=cx+sign*120
            roof=box((min(a,b),cy-84,190),(max(a,b),cy+84,252),'outter_wall3')
            n=np.array([-.4*sign,0.,-1.]);distance=-252-.4*sign*cx
            roof=roof.clipped(Face(n/np.linalg.norm(n),distance/np.linalg.norm(n),'rf_tile_1')).compact()
            for f in roof.faces:
                if f.normal[2]<-.5:replace(f,'rf_tile_1')
            world['brushes'].append(roof)
            x=cx+sign*124
            world['brushes'].append(box((x-4,cy-88,202),(x+4,cy+88,214),'rf_tile_2'))

