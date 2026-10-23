from pathlib import Path
from bsp import BSP
import math
root=Path(__file__).resolve().parents[2]
b=BSP(root/'valve/maps/rapidcore.bsp')
spawn=(432,-1601,-192)
points=[(-160,800),(200,880),(500,700),(-550,700),
        (-1450,350),(-1450,-150),(-1250,0),(-1600,100),
        (-850,300),(-850,-200),(-800,-480),
        (-240,100),(180,100),(-240,-250),(180,-250),
        (960,0),(1100,-400),(900,-650),(580,-300),(600,300)]
for x,y in points:
    z=b.floor(x,y,top=128)
    print((x,y,z),'distance',round(math.dist((x,y),spawn[:2])), 'visible',b.line_clear(spawn,(x,y,z+64)) if z is not None else None)
