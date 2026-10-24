"""Generate deterministic, animated additive GoldSrc plasma sprite layers."""
from pathlib import Path
import math,struct,json,hashlib
MOD=Path(__file__).resolve().parents[1]
OUT=MOD/'sprites/cremation'
def build_assets():
 OUT.mkdir(parents=True,exist_ok=True);report={}
 for kind,width,height in [('body',64,80),('core',48,48),('halo',96,96),('beam_glow',32,32),('beam_hot',16,16)]:
  frames=16
  data=bytearray(struct.pack('<4i f 3i f i',0x50534449,2,2,1,math.hypot(width,height)/2,width,height,frames,0.,0))
  data.extend(struct.pack('<H',256));palette=[]
  for i in range(256):
   f=i/255
   if kind in ('core','beam_hot'):rgb=(round(255*f),round(255*f),round(190*f))
   elif kind in ('halo','beam_glow'):rgb=(round(25*f),round(230*f),round(8*f))
   else:rgb=(round(185*max(0,(f-.55)/.45)),round(255*f),round(35*f))
   palette.extend(rgb)
  data.extend(bytes(palette))
  for frame in range(frames):
   phase=math.tau*frame/frames;pixels=bytearray()
   for y in range(height):
    for x in range(width):
     u=(x+.5-width/2)/(width*.48);v=(y+.5-height/2)/(height*.48)
     r=math.hypot(u,v);angle=math.atan2(v,u)
     if kind in ('halo','beam_glow'):
      intensity=math.exp(-r*r*4.8)*max(0,1-r)*.65*(.94+.06*math.sin(phase))
     elif kind in ('core','beam_hot'):
      a=math.exp(-((u-.16*math.sin(phase))**2*22+(v+.14*math.cos(phase))**2*36))
      b=math.exp(-((u+.18*math.cos(phase))**2*28+(v-.12*math.sin(phase))**2*22))
      intensity=min(1,a+b*.75)*max(0,1-r*.4)
     else:
      boundary=1+.11*math.sin(angle*5+phase)+.07*math.sin(angle*9-phase*2)
      edge=max(0,1-r/boundary)
      swirl=.5+.5*math.sin(angle*3+phase*2-r*9)
      intensity=min(1,edge**1.5*1.8+edge*swirl*.22)
     pixels.append(max(0,min(255,round(intensity*255))))
   data.extend(struct.pack('<5i',0,-width//2,height//2,width,height));data.extend(pixels)
  path=OUT/f'link_plasma_{kind}.spr';path.write_bytes(data)
  report[kind]={'path':path.relative_to(MOD).as_posix(),'size':[width,height],'frames':frames,'mode':'additive billboard','sha256':hashlib.sha256(data).hexdigest()}
 (MOD/'link-plasma-assets.json').write_text(json.dumps({'generator':'tools/prepare_link_plasma_sprites.py','layers':report},indent=2)+'\n')
if __name__=='__main__':build_assets()
