"""Orthographic BSP plan for annotating changes; not an editor-app screenshot."""
from pathlib import Path
import json
import math
from PIL import Image, ImageDraw, ImageFont
from bsp import BSP

MOD=Path(__file__).resolve().parents[1]
b=BSP(MOD/'maps/cs_mansion.bsp')
placement=json.loads((MOD/'mansion-placement.json').read_text())
vertices=b.records(3,'<3f')
edges=b.records(12,'<2H')
surfedges=[r[0] for r in b.records(13,'<i')]
faces=b.records(7,'<Hhihh4Bi')
W,H=2220,1660
im=Image.new('RGB',(W,H),'#11161d')
d=ImageDraw.Draw(im)
font_path=Path('C:/Windows/Fonts/consola.ttf')
def font(size):return ImageFont.truetype(str(font_path),size)
small,normal,heading=font(17),font(21),font(32)
scale=.65
xmin,xmax,ymin,ymax=-1024,1536,-1280,768
left,top=105,180
def point(v):return (left+(v[0]-xmin)*scale,top+(ymax-v[1])*scale)
def label(x,y,text,fill='#d6e0e9',f=normal):d.text((x,y),text,font=f,fill=fill)

label(105,30,'CS_MANSION / CREMATION',f=heading)
label(105,77,'VISTA SUPERIOR ORTOGRAFICA DEL BSP  |  +Y ARRIBA  |  +X DERECHA',f=normal)

world=b.models[0]
polygons=[]
for idx,f in enumerate(faces):
    if not (world[14]<=idx<world[14]+world[15]):continue
    poly=[vertices[edges[abs(se)][0 if se>=0 else 1]] for se in surfedges[f[2]:f[2]+f[3]]]
    polygons.append(poly)
    z=sum(v[2] for v in poly)/len(poly)
    nz=b.planes[f[0]][2]*(-1 if f[1] else 1)
    if nz>.9 and -140<z<90:
        color='#202d36' if z>=20 else '#142b38'
        d.polygon([point(v) for v in poly],fill=color)

# Grid references and exact engine coordinates remain readable over floors.
for x in range(xmin,xmax+1,64):
    major=x%256==0
    d.line([point((x,ymin)),point((x,ymax))],fill='#3a4854' if major else '#27333d',width=1)
    if major:
        px,py=point((x,ymax));label(px-24,top-35,str(x),f=small)
for y in range(ymin,ymax+1,64):
    major=y%256==0
    d.line([point((xmin,y)),point((xmax,y))],fill='#3a4854' if major else '#27333d',width=1)
    if major:
        px,py=point((xmin,y));label(19,py-9,str(y),f=small)
for x in range(10):
    px,_=point((xmin+x*256+128,ymax));label(px-7,top-65,chr(65+x),fill='#6fb3c8')
for y in range(8):
    _,py=point((xmin,ymax-y*256-128));label(76,py-10,str(y+1),fill='#6fb3c8')

def slice_lines(z):
    lines=[]
    for poly in polygons:
        hits=[]
        for a,c in zip(poly,poly[1:]+poly[:1]):
            if (a[2]<z)!=(c[2]<z):
                t=(z-a[2])/(c[2]-a[2]);hits.append(tuple(a[i]+(c[i]-a[i])*t for i in range(3)))
        if len(hits)==2:lines.append(hits)
    return lines

for a,c in slice_lines(80):d.line([point(a),point(c)],fill='#c0d6df',width=3)

# Upper-floor partitions are deliberately dashed to distinguish their level.
for a,c in slice_lines(224):
    if max(a[0],c[0])>1 or min(a[0],c[0])<-740:continue
    pa,pc=point(a),point(c);length=math.dist(pa,pc)
    if length<2:continue
    for dist in range(0,int(length),13):
        t0=dist/length;t1=min(1,(dist+6)/length)
        d.line([(pa[0]+(pc[0]-pa[0])*t0,pa[1]+(pc[1]-pa[1])*t0),
                (pa[0]+(pc[0]-pa[0])*t1,pa[1]+(pc[1]-pa[1])*t1)],fill='#887ba9',width=2)

for i,spawn in enumerate(placement['tt_spawns']):
    x,y,z=map(float,spawn['origin'].split());px,py=point((x,y))
    d.ellipse((px-8,py-8,px+8,py+8),fill='#f1b94d',outline='#171b20',width=2)
    label(px+12,py-10,f'TT{i+1}',fill='#f1c86e',f=small)
for i,spawn in enumerate(placement['ct_spawns']):
    x,y,z=map(float,spawn['origin'].split());px,py=point((x,y))
    d.rectangle((px-7,py-7,px+7,py+7),fill='#47b7f1',outline='#11161d',width=2)
    label(px+11,py-10,f'CT{i+1}',fill='#6fcaff',f=small)

door=point((0,-256))
d.ellipse((door[0]-11,door[1]-11,door[0]+11,door[1]+11),outline='#72e0a0',width=3)
d.line([door,(door[0]+70,door[1]-30)],fill='#72e0a0',width=2)
label(door[0]+76,door[1]-52,'PUERTA',fill='#72e0a0',f=small)
label(door[0]+76,door[1]-31,'PRINCIPAL',fill='#72e0a0',f=small)

rx=1840
label(rx,150,'REFERENCIAS',f=heading)
label(rx,205,'Cuadricula menor: 64 u',f=small)
label(rx,236,'Cuadricula mayor: 256 u',f=small)
label(rx,267,'Celdas: A1 ... J8',f=small)
label(rx,330,'TT1-10 / jugador',fill='#f1c86e')
label(rx,362,'Puntos en planta alta',f=small)
label(rx,415,'CT1-10 / enemigos',fill='#6fcaff')
label(rx,447,'Puntos en el exterior',f=small)
label(rx,500,'Contorno claro:',fill='#c0d6df')
label(rx,532,'planta baja, corte Z=80',f=small)
label(rx,585,'Trazo violeta:',fill='#b09acb')
label(rx,617,'planta alta, corte Z=224',f=small)
label(rx,690,'ORIENTACION',f=normal)
cx,cy=1980,800
d.line((cx,cy+55,cx,cy-55),fill='#afc7d4',width=3)
d.polygon([(cx,cy-65),(cx-7,cy-48),(cx+7,cy-48)],fill='#afc7d4')
label(cx-12,cy-96,'+Y',f=small)
d.line((cx-55,cy,cx+55,cy),fill='#afc7d4',width=3)
d.polygon([(cx+65,cy),(cx+48,cy-7),(cx+48,cy+7)],fill='#afc7d4')
label(cx+75,cy-10,'+X',f=small)
label(rx,940,'Puedes marcar encima',f=small)
label(rx,969,'o indicar una celda',f=small)
label(rx,998,'y coordenadas X/Y.',f=small)
label(rx,1080,'Skybox activa: CITY',fill='#e2b8a4',f=small)
label(rx,1109,'Original del RAR',f=small)

label(105,1590,'Plano generado de la geometria instalada. Sin rutas de oleadas superpuestas para facilitar anotaciones.',f=small)
out=MOD/'diagnostics/cs_mansion-vista-superior-cuadriculada.png'
im.save(out)
print(out)
