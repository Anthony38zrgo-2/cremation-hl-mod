"""Original Vostok animated props, with separate solid map collision."""
from pathlib import Path
import hashlib
import json
import shutil
from map_brushes import box

PROPS = [
    dict(name='jeep', model='jeep-russian', origin=(-864,64,24), yaw=0, rate=10),
    dict(name='tree_spawn', model='tree_noleaves2', origin=(-864,-624,24), yaw=0, rate=1),
    dict(name='tree_a_north', model='tree_noleaves2', origin=(-448,344,24), yaw=90, rate=1),
    dict(name='tree_a_east', model='tree_noleaves2', origin=(336,-640,24), yaw=120, rate=1.3),
]

def add_cover(entities, mod, entity):
    manifest=[]
    for prop in PROPS:
        source=mod.parent/'czero/models/props'/f"{prop['model']}.mdl"
        target=mod/'models/cremation/vostok'/source.name
        target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(source,target)
        origin=prop['origin']; x,y,z=origin
        entities.append(entity(dict(classname='info_target',targetname='vostok_cover_'+prop['name'],
            model=target.relative_to(mod).as_posix(),origin=' '.join(map(str,origin)),
            angles=f"0 {prop['yaw']} 0",framerate=str(prop['rate']))))
        # NULL faces are invisible; solid BSP hulls still stop movement and shots.
        # Canopies remain permeable. The jeep has a low chassis and a cabin.
        boxes=[((-12,-12,0),(12,12,208))] if prop['model'].startswith('tree') else [
            ((-36,-84,18),(36,84,55)),((-32,-36,55),(32,30,78))]
        brushes=[box((x+a[0],y+a[1],z+a[2]),(x+b[0],y+b[1],z+b[2]),'NULL') for a,b in boxes]
        entities.append(entity(dict(classname='func_wall',targetname='vostok_collision_'+prop['name']),brushes))
        manifest.append(dict(prop,source=str(source),installed=target.relative_to(mod).as_posix(),
            sha256=hashlib.sha256(target.read_bytes()).hexdigest(),collision_boxes=boxes))
    folder=mod/'assets-source/vostok-cover';folder.mkdir(parents=True,exist_ok=True)
    (folder/'installed.json').write_text(json.dumps(manifest,indent=2)+'\n')

