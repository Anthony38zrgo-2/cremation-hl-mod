"""Bake walkable Mansion floor/step graph from actual BSP collision hulls."""
from pathlib import Path
import sys,math,json
from bsp import BSP
MOD=Path(__file__).resolve().parents[1];b=BSP(MOD/'maps/cs_mansion.bsp')
interactive={int(e['model'][1:]) for e in b.entities if e.get('classname') in ('func_door','func_door_rotating','func_breakable') and e.get('model','').startswith('*')}
stair_ramp=next((e for e in b.entities if e.get('targetname')=='mansion_czero_stair_ramp'),None)
if stair_ramp and stair_ramp.get('model','').startswith('*'):interactive.add(int(stair_ramp['model'][1:]))
b.solids=[i for i in b.solids if i not in interactive]
def walk_floor(x,y,top=240,bottom=24):
 if stair_ramp and abs(x+224)<.1 and -224<=y<=-16:return 64+(y+224)*(128/208)
 return b.floor(x,y,top=top,bottom=bottom)
def clear_margin(x,y,z):
 return all(b.clear((x+dx,y+dy,z),1) for dx,dy in ((-3,-3),(-3,3),(3,-3),(3,3)))
points=[];cells={}
heights=(24,32,40,48,56,64,72,80,88,96,104,112,120,128,136,144,152,160,168,176,184,192,200,208,216)
for x in range(-1152,449,32):
 for y in range(-1056,545,32):
  ramp_sample=bool(stair_ramp and x==-224 and -224<=y<=-16)
  sample_heights=[walk_floor(x,y)] if ramp_sample else heights
  for z in sample_heights:
   if not b.clear((x,y,z+36.1),1) or (not ramp_sample and b.clear((x,y,z+35.5),1)) or not clear_margin(x,y,z+36.1):continue
   cells.setdefault((x,y),[]).append(len(points));points.append((x,y,z+.1))
# The mansion's side staircase rises in 16-unit steps every 24 units. The
# regular 32-unit grid skips two treads and disconnects the upper floor from
# the ground navigation component. Add checked 8-unit samples along the real
# stair flight, then link them to the adjacent regular-grid landings.
for y in range(-224,-15,8):
 z=walk_floor(-224,y,top=240,bottom=24)
 if z is None:continue
 point=(-224.0,float(y),float(z)+.1)
 if not b.clear((point[0],point[1],point[2]+36.1),1):continue
 if not clear_margin(point[0],point[1],point[2]+36.1):continue
 if any(abs(p[0]-point[0])<.1 and abs(p[1]-point[1])<.1 and abs(p[2]-point[2])<1 for p in points):continue
 cells.setdefault((point[0],point[1]),[]).append(len(points));points.append(point)
if stair_ramp:
 landing=(-224.0,8.0,float(b.floor(-224,8,top=240,bottom=24) or 192)+.1)
 center=(landing[0],landing[1],landing[2]+36.1)
 if b.clear(center,1) and clear_margin(*center):cells.setdefault((landing[0],landing[1]),[]).append(len(points));points.append(landing)
edges=[];starts=[]
for index,p in enumerate(points):
 start=len(edges)
 offsets={(dx,dy) for dx in (-32,0,32) for dy in (-32,0,32) if dx or dy}
 if -256<=p[0]<=-192 and -240<=p[1]<=16:
  offsets.update((dx,dy) for dx in range(-16,17,8) for dy in range(-32,33,8) if dx or dy)
 for dx,dy in offsets:
   if not dx and not dy:continue
   for j in cells.get((p[0]+dx,p[1]+dy),[]):
    q=points[j]
    if abs(p[2]-q[2])>18:continue
    z=max(p[2],q[2])+36.1;count=8
    if any(not clear_margin(p[0]+(q[0]-p[0])*k/count,p[1]+(q[1]-p[1])*k/count,z) for k in range(count+1)):continue
    ramp_edge=bool(stair_ramp and abs(p[0]+224)<.1 and abs(q[0]+224)<.1 and p[1]>=-224 and q[1]>=-224 and p[1]<=8 and q[1]<=8)
    if not ramp_edge and any(b.clear((p[0]+dx*k/count,p[1]+dy*k/count,min(p[2],q[2])+35.5),1) for k in range(count+1)):continue
    edges.append(j)
 starts.append((start,len(edges)-start))
def nearest(p):return min(range(len(points)),key=lambda i:math.dist(points[i],p))
def component(p):
 seen={nearest(p)};pending=list(seen)
 while pending:
  n=pending.pop();a,c=starts[n]
  for j in edges[a:a+c]:
   if j not in seen:seen.add(j);pending.append(j)
 return seen
# Confirm both the outside south entrance and the bottom of the real side
# staircase connect to the upper hostage-zone goal in the baked walk graph.
stair_bottom=nearest((-224,-224,56));stair_top=nearest((-224,-16,192))
plant_upper=nearest((-624,-176,192));south_entry=nearest((-312,-736,56))
stairs_component=component(points[stair_bottom]);south_component=component(points[south_entry])
print('stairs nodes',stair_bottom,points[stair_bottom],stair_top,points[stair_top],'upper',plant_upper,points[plant_upper],
      'connected',stair_top in stairs_component,plant_upper in stairs_component,'south',plant_upper in south_component)
print('stair links',[(i,points[i],starts[i][1]) for i,p in enumerate(points) if abs(p[0]+224)<.1 and -224<=p[1]<=8])
assert stair_top in stairs_component and plant_upper in stairs_component, 'side staircase does not reach the upper hostage zone'
assert plant_upper in south_component, 'south breakable entrance does not reach upper hostage zone'
print('side stairs',stair_bottom,'->',stair_top,'-> hostage',plant_upper,'reachable',len(stairs_component))
print('south door to upper hostage zone reachable',len(south_component))
rows=','.join('{'+','.join(f'{v:.1f}f' for v in p)+','+str(a)+','+str(c)+'}' for p,(a,c) in zip(points,starts))
(MOD/'source/czero_navigation.h').write_text('#pragma once\nstruct CZNavNode{float x,y,z;int first,count;};\nstatic const CZNavNode czNav[]={'+rows+'};\nstatic const int czNavEdges[]={'+','.join(map(str,edges))+'};\n')
folder=MOD/'assets-source/czero-zombie';(folder/'navigation.json').write_text(json.dumps(dict(points=points,starts=starts,edges=edges),indent=2)+'\n')
print('Baked',len(points),'nodes,',len(edges),'walkable directed links; doors/windows are interactive; side stairs connected.')
