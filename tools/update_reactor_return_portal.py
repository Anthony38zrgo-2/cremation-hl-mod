"""Move the island reward near its door and replace the other with a return portal."""
from pathlib import Path
import json
import math
import re
import struct
from bsp import BSP

MOD=Path(__file__).resolve().parents[1]
SOURCE=MOD/'assets-source/mansion-reactor'
STATION=[912,-960,65]
PORTAL=[816,-960,72]
EXIT=[-784,-272,61]
POINTS={'reactor_pickup_0':STATION,'reactor_return_portal':PORTAL,'reactor_return_exit':EXIT}

def write_portal_sprite():
    width,height,frames=64,128,16
    data=bytearray(struct.pack('<4i f 3i f i',0x50534449,2,2,1,72.,width,height,frames,0.,0))
    data.extend(struct.pack('<H',256))
    data.extend(bytes(c for i in range(256) for c in (round(i*.08),round(i*.62),i)))
    for frame in range(frames):
        pixels=bytearray();phase=math.tau*frame/frames
        for y in range(height):
            for x in range(width):
                u=(x+.5-width/2)/(width*.41);v=(y+.5-height/2)/(height*.44)
                radius=math.hypot(u,v);angle=math.atan2(v,u)
                ring=max(0,1-abs(radius-1)/.16)*(.78+.22*math.sin(angle*4-phase))
                interior=max(0,1-radius)*(.08+.08*math.sin(radius*15-phase*2)**2)
                pixels.append(round(255*min(1,ring+interior)))
        data.extend(struct.pack('<5i',0,-width//2,height//2,width,height));data.extend(pixels)
    (MOD/'sprites/cremation/return_portal.spr').write_bytes(data)

def update():
    write_portal_sprite()
    for path in (MOD/'maps/cs_mansion.bsp',SOURCE/'cs_mansion_reactor.bsp'):
        bsp=BSP(path)
        entities=[e for e in bsp.entities if e.get('targetname') not in ('reactor_pickup_0','reactor_pickup_1','reactor_return_portal','reactor_return_exit')]
        entities += [dict(classname='info_target',targetname=name,origin=' '.join(map(str,point))) for name,point in POINTS.items()]
        bsp.write(path,entities)
        assert all(BSP(path).lump(i)==bsp.lump(i) for i in range(1,15))
        assert bsp.contents(tuple(EXIT),1)==-1
    path=SOURCE/'cs_mansion_reactor.map'
    text=path.read_text()
    text=re.sub(r'\{\s*"classname" "info_target"\s*"targetname" "(?:reactor_pickup_[01]|reactor_return_portal|reactor_return_exit)"\s*"origin" "[^"]*"\s*\}\s*','',text)
    text+='\n'.join('{\n"classname" "info_target"\n"targetname" "'+name+'"\n"origin" "'+' '.join(map(str,point))+'"\n}' for name,point in POINTS.items())+'\n'
    path.write_text(text)
    path=SOURCE/'reactor-layout.json';layout=json.loads(path.read_text());layout.update(stations=[STATION],portal=PORTAL,portal_exit=EXIT);path.write_text(json.dumps(layout,indent=2)+'\n')
    path=MOD/'mansion-placement.json';data=json.loads(path.read_text());data['reactor']=layout;data['entities']=len(BSP(MOD/'maps/cs_mansion.bsp').entities);path.write_text(json.dumps(data,indent=2)+'\n')
    print('Installed one reward near the small-house door and a return portal to the rear yard; BSP geometry unchanged.')

if __name__=='__main__':update()
