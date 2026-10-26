"""Render installed four-front BSP as an annotated orthographic grid."""
from pathlib import Path
import json
import math
from PIL import Image,ImageDraw,ImageFont
from bsp import BSP

def render():
    mod=Path(__file__).resolve().parents[1];b=BSP(mod/'maps/cs_mansion.bsp')
    data=json.loads((mod/'mansion-placement.json').read_text());r=data['reactor']
    im=Image.new('RGB',(2180,1960),'#11161d');d=ImageDraw.Draw(im)
    font=lambda size:ImageFont.truetype('C:/Windows/Fonts/consola.ttf',size)
    def label(p,text,color='#dae5ee',size=20):d.text(p,text,font=font(size),fill=color)
    scale=.30;left,top=110,170;xmin,xmax,ymin,ymax=-3072,2560,-3072,2560
    def pt(p):return left+(p[0]-xmin)*scale,top+(ymax-p[1])*scale
    def rect(bounds,fill,outline=None):
        x0,y0,x1,y1=bounds;d.rectangle([pt((x0,y1)),pt((x1,y0))],fill=fill,outline=outline,width=2)
    label((110,28),'CREMATION / MANSION - CUATRO FRENTES',size=32)
    label((110,78),'BSP INSTALADO | VISTA SUPERIOR | +Y NORTE | +X ESTE',size=21)
    rect(r['arena'],'#183b29','#66cf83')
    for area in r['safe_rects']:rect(area,'#2c3943','#9cbbc9')
    for bridge in r['bridges']:rect(bridge,'#485661','#c4dae3')
    for x in range(xmin,xmax+1,128):
        d.line([pt((x,ymin)),pt((x,ymax))],fill='#40515a' if x%512==0 else '#283c40')
        if x%512==0:label((pt((x,ymax))[0]-24,132),str(x),size=16)
    for y in range(ymin,ymax+1,128):
        d.line([pt((xmin,y)),pt((xmax,y))],fill='#40515a' if y%512==0 else '#283c40')
        if y%512==0:label((24,pt((xmin,y))[1]-8),str(y),size=16)
    vertices=b.records(3,'<3f');edges=b.records(12,'<2H');surf=[a[0] for a in b.records(13,'<i')]
    faces=b.records(7,'<Hhihh4Bi');world=b.models[0]
    for f in faces[world[14]:world[14]+world[15]]:
        poly=[vertices[edges[abs(se)][0 if se>=0 else 1]] for se in surf[f[2]:f[2]+f[3]]]
        for z,color in [(80,'#deedf5'),(224,'#a48ec0')]:
            hits=[]
            for a,c in zip(poly,poly[1:]+poly[:1]):
                if (a[2]<z)!=(c[2]<z):
                    t=(z-a[2])/(c[2]-a[2]);hits.append(tuple(a[i]+(c[i]-a[i])*t for i in range(3)))
            if len(hits)==2:d.line([pt(v) for v in hits],fill=color,width=2)
    for spawn in data['tt_spawns']:
        x,y=pt(list(map(float,spawn['origin'].split())));d.ellipse((x-3,y-3,x+3,y+3),fill='#ffd263')
    for spawn in data['ct_spawns']:
        x,y=pt(list(map(float,spawn['origin'].split())));d.rectangle((x-4,y-4,x+4,y+4),fill='#50c6ff')
    for hub,name in zip(r['spawn_hubs'],['NORTE','ESTE','SUR','OESTE']):
        x,y=pt(((hub[0]+hub[2])/2,hub[3]+100));label((x-60,y),name,'#6fd4ff')
    for key in ('pad','powerup_pad'):
        x,y=pt(r[key]);d.rectangle((x-7,y-7,x+7,y+7),fill='#bc8aef');label((x-27,y+12),'SALTO','#d6b0ff',16)
        destination=r['powerup_destination' if key=='powerup_pad' else 'destination']
        tx,ty=pt(destination);length=math.hypot(tx-x,ty-y);ux,uy=(tx-x)/length,(ty-y)/length
        for step in range(16,int(length)-20,18):
            d.line([(x+ux*step,y+uy*step),(x+ux*(step+8),y+uy*(step+8))],fill='#c394ef',width=2)
        ax,ay=tx-ux*18,ty-uy*18
        d.polygon([(ax,ay),(ax-ux*13+uy*6,ay-uy*13-ux*6),(ax-ux*13-uy*6,ay-uy*13+ux*6)],fill='#c394ef')
    for key,color in [('stations','#ffce42'),('powerup_stations','#6ee9ff')]:
        for p in r[key]:
            x,y=pt(p);d.polygon([(x,y-7),(x+7,y),(x,y+7),(x-7,y)],fill=color)
    for key in ('portal','powerup_portal','portal_exit'):
        x,y=pt(r[key]);d.ellipse((x-7,y-7,x+7,y+7),outline='#65b8ff',width=3)
    x,y=pt((-352,-120));label((x-55,y),'MANSION',size=19)
    for p,text,color in [(r['destination'],'ARMAS NE','#ffce42'),(r['powerup_destination'],'POWERUPS SO','#6ee9ff')]:
        p=(p[0],p[1]-230)
        x,y=pt(p);label((x-55,y),text,color,18)
    rx=1850
    notes=[('REFERENCIAS','#ffffff'),('Menor: 128 u','#dae5ee'),('Mayor: 512 u','#dae5ee'),('Verde: pozo letal','#8ce49f'),('4 puentes: 256 u','#dae5ee'),('40 spawns: 10/lado','#6fd4ff'),('Recintos: 2560x768','#dae5ee'),('Muros: 192 u','#dae5ee'),('Entrada: 320 u','#dae5ee'),('Violeta: salto','#d6b0ff'),('Azul: regreso','#65b8ff'),('NE: armas del mod','#ffce42'),('SO: salud, bateria,','#6ee9ff'),('    salto largo','#6ee9ff'),('OLEADAS / POR LADO','#ffffff'),('1: 40 / 10','#dae5ee'),('2: 40 / 10','#dae5ee'),('3: 80 / 20','#dae5ee'),('4: 80 / 20','#dae5ee'),('Skybox: CITY original','#e2b8a4')]
    for i,(text,color) in enumerate(notes):label((rx,180+i*43),text,color,18)
    label((110,1900),'Plano de la geometria instalada. Marcas de aparicion y recompensas; rutas no superpuestas.',size=18)
    out=mod/'diagnostics/cs_mansion-vista-superior-cuadriculada.png';im.save(out);print(out)

if __name__=='__main__':render()
