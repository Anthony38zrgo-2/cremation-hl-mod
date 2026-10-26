"""Bake walkable Mansion floor/step graph from actual BSP collision hulls."""
from pathlib import Path
import sys,math,json
from bsp import BSP
MOD=Path(__file__).resolve().parents[1];b=BSP(MOD/'maps/cs_mansion.bsp')
interactive={int(e['model'][1:]) for e in b.entities if e.get('classname') in ('func_door','func_door_rotating','func_breakable') and e.get('model','').startswith('*')}
b.solids=[i for i in b.solids if i not in interactive]
def clear_margin(x,y,z):
 return all(b.clear((x+dx,y+dy,z),1) for dx,dy in ((-3,-3),(-3,3),(3,-3),(3,3)))
points=[];cells={}
heights=(24,32,40,48,56,64,72,80,88,96,104,112,120,128,136,144,152,160,168,176,184,192,200,208,216)
for x in range(-1152,449,32):
 for y in range(-1056,545,32):
  for z in heights:
   if not b.clear((x,y,z+36.1),1) or b.clear((x,y,z+35.5),1) or not clear_margin(x,y,z+36.1):continue
   cells.setdefault((x,y),[]).append(len(points));points.append((x,y,z+.1))
edges=[];starts=[]
for index,p in enumerate(points):
 start=len(edges)
 for dx in (-32,0,32):
  for dy in (-32,0,32):
   if not dx and not dy:continue
   for j in cells.get((p[0]+dx,p[1]+dy),[]):
    q=points[j]
    if abs(p[2]-q[2])>18:continue
    z=max(p[2],q[2])+36.1;count=8
    if any(not clear_margin(p[0]+(q[0]-p[0])*k/count,p[1]+(q[1]-p[1])*k/count,z) for k in range(count+1)):continue
    if any(b.clear((p[0]+dx*k/count,p[1]+dy*k/count,min(p[2],q[2])+35.5),1) for k in range(count+1)):continue
    edges.append(j)
 starts.append((start,len(edges)-start))
def nearest(p):return min(range(len(points)),key=lambda i:math.dist(points[i],p))
# Check that both doors can reach the original ground-floor hostage location.
plant=nearest((-304,-272,56))
for name,p in [('front',(96,-256,56)),('south',(-312,-736,56)),('upper',(-624,-176,192))]:
 seen={nearest(p)};pending=list(seen)
 while pending:
  n=pending.pop();a,c=starts[n]
  for j in edges[a:a+c]:
   if j not in seen:seen.add(j);pending.append(j)
 print(name,'node',nearest(p),'reachable',len(seen),'ground plant reachable',plant in seen)
assert plant in seen or name=='upper'
rows=','.join('{'+','.join(f'{v:.1f}f' for v in p)+','+str(a)+','+str(c)+'}' for p,(a,c) in zip(points,starts))
(MOD/'source/czero_navigation.h').write_text('#pragma once\nstruct CZNavNode{float x,y,z;int first,count;};\nstatic const CZNavNode czNav[]={'+rows+'};\nstatic const int czNavEdges[]={'+','.join(map(str,edges))+'};\n')
folder=MOD/'assets-source/czero-zombie';(folder/'navigation.json').write_text(json.dumps(dict(points=points,starts=starts,edges=edges),indent=2)+'\n')
print('Baked',len(points),'nodes,',len(edges),'walkable directed links; doors/windows are interactive.')
