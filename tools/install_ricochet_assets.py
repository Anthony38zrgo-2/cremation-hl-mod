from pathlib import Path
import shutil, struct, json, hashlib
from PIL import Image, ImageDraw, ImageFont
ROOT=Path(__file__).resolve().parents[2]
MOD=ROOT/'Cremation'

def install():
    files={
        'dlls/hl_ravenholm.dll':'Half-Life2Classic_Ravenholm/dlls/hl.dll',
        'models/v_9mmhandgun.mdl':'ricochet/models/v_disc.mdl',
        'models/p_9mmhandgun.mdl':'ricochet/models/p_disc.mdl',
        'models/w_9mmhandgun.mdl':'ricochet/models/disc.mdl',
        'models/ricochet/disc.mdl':'ricochet/models/disc.mdl',
        'models/ricochet/disc_hard.mdl':'ricochet/models/disc_hard.mdl',
        'sprites/ricochet/lgtning.spr':'valve/sprites/lgtning.spr',
        'sprites/ricochet/discreturn.spr':'ricochet/sprites/discreturn.spr',
        'sound/ricochet/throw.wav':'ricochet/sound/weapons/cbar_miss1.wav',
        'sound/ricochet/altfire.wav':'ricochet/sound/weapons/altfire.wav',
        'sound/ricochet/freeze.wav':'valve/sound/weapons/electro5.wav',
        'sound/ricochet/blast.wav':'valve/sound/weapons/explode3.wav',
        'sound/ricochet/bounce.wav':'ricochet/sound/weapons/xbow_hit1.wav',
        'dlls/cremation.dll':'Cremation/source/build/cremation.dll',
    }
    for destination,source in files.items():
        target=MOD/destination;target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(ROOT/source,target)
    (MOD/'ricochet-assets.json').write_text(json.dumps(files,indent=2)+'\n')
    image=Image.new('L',(128,32));draw=ImageDraw.Draw(image)
    font=ImageFont.truetype('C:/Windows/Fonts/arialbd.ttf',17)
    draw.ellipse((3,8,27,24),outline=255,width=2);draw.ellipse((9,12,21,20),outline=255,width=1)
    draw.text((32,6),'RICOCHET',font=font,fill=255)
    header=struct.pack('<4siiifiiifi',b'IDSP',2,2,1,66,128,32,1,0,0)
    palette=bytes(v for i in range(256) for v in (i,i,i))
    frame=struct.pack('<5i',0,-64,16,128,32)+image.tobytes()
    (MOD/'sprites/cremation_ricochet.spr').write_bytes(header+struct.pack('<H',256)+palette+frame)
    sprite_text='10\n'
    for resolution in (320,640):
        sprite_text+=f'weapon {resolution} cremation_ricochet 0 0 128 32\nweapon_s {resolution} cremation_ricochet 0 0 128 32\n'
        sprite_text+=f'ammo {resolution} cremation_ricochet 2 7 28 20\n'
        sprite_text+=f'crosshair {resolution} crosshairs 24 0 24 24\nautoaim {resolution} crosshairs 0 72 24 24\n'
    for name in ('weapon_ricochet.txt','weapon_9mmhandgun.txt'):(MOD/'sprites'/name).write_text(sprite_text)
    print('Installed Ricochet models, sounds, HUD and Cremation DLL.')

if __name__=='__main__':install()
