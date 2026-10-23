from pathlib import Path
from PIL import Image, ImageDraw
from bsp import BSP

ROOT=Path(__file__).resolve().parents[2]
b=BSP(ROOT/'valve/maps/rapidcore.bsp')
vertices=b.records(3,'<3f')
edges=b.records(12,'<2H')
surfedges=[r[0] for r in b.records(13,'<i')]
faces=b.records(7,'<Hhihh4Bi')
im=Image.new('RGB',(1260,1220),'#111720')
d=ImageDraw.Draw(im)
def point(v): return ((v[0]+1900)*.38+20,(1150-v[1])*.38+20)
for x in range(-1800,1300,200):
    d.line([point((x,-1800)),point((x,1150))],fill='#273241')
    d.text(point((x,1150)),str(x),fill='white')
for y in range(-1800,1200,200):
    d.line([point((-1900,y)),point((1300,y))],fill='#273241')
    d.text(point((-1900,y)),str(y),fill='white')
for f in faces:
    poly=[vertices[edges[abs(se)][0 if se>=0 else 1]] for se in surfedges[f[2]:f[2]+f[3]]]
    if abs(b.planes[f[0]][2])>.9 and -410<poly[0][2]<-190:
        d.polygon([point(v) for v in poly],fill='#34424c',outline='#536773')
for f in faces:
    poly=[vertices[edges[abs(se)][0 if se>=0 else 1]] for se in surfedges[f[2]:f[2]+f[3]]]
    hits=[]
    for a,c in zip(poly,poly[1:]+poly[:1]):
        if (a[2]<-240)!=(c[2]<-240):
            t=(-240-a[2])/(c[2]-a[2]); hits.append(tuple(a[i]+(c[i]-a[i])*t for i in range(3)))
    if len(hits)==2: d.line([point(v) for v in hits],fill='#c5d9e6',width=2)
for i,e in enumerate(b.entities):
    if e.get('classname') in ('info_player_deathmatch','info_player_start'):
        v=tuple(map(float,e['origin'].split())); px,py=point(v)
        d.ellipse((px-5,py-5,px+5,py+5),fill='orange')
        d.text((px+7,py),e['origin'],fill='yellow')
im.save(ROOT/'Cremation/tools/rapidcore-layout.png')
