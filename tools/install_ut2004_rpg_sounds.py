"""Install original UT2004 Rocket Launcher recordings as GoldSrc PCM WAVs.
Source recordings are exported with the user's UT2004 UCC batchexport tool.
"""
from pathlib import Path
import wave
import struct
import hashlib
import json

MOD=Path(__file__).resolve().parents[1]
FILES={
    'fire.wav':'BaseFiringSounds.BRocketLauncherFire.wav',
    'altfire.wav':'BaseFiringSounds.BRocketLauncherAltFire.wav',
    'load.wav':'BaseGunTech.BRocketLauncherLoad.wav',
    'plunger.wav':'RocketLauncher.RocketLauncherPlunger.wav',
    'flight.wav':'RocketLauncher.RocketLauncherProjectile.wav',
    'equip.wav':'RocketLauncher.SwitchToRocketLauncher.wav',
}
def install():
    source=MOD/'assets-source/ut2004_rocket/WeaponSounds'
    target=MOD/'sound/weapons/ut2004';target.mkdir(parents=True,exist_ok=True)
    report={}
    for name,original in FILES.items():
        with wave.open(str(source/original),'rb') as w:
            rate=w.getframerate();width=w.getsampwidth();channels=w.getnchannels();frames=w.getnframes();data=w.readframes(frames)
        if channels!=1 or rate!=22050 or width not in (1,2):raise ValueError(f'Unsupported source: {original}')
        if width==1:data=b''.join(struct.pack('<h',(v-128)*256) for v in data)
        with wave.open(str(target/name),'wb') as w:
            w.setparams((1,2,rate,0,'NONE','not compressed'));w.writeframes(data)
        report[name]={'source':original,'seconds':frames/rate,'rate':rate,'channels':channels,'source_sample_bits':width*8,'installed_sample_bits':16,'sha256':hashlib.sha256((target/name).read_bytes()).hexdigest()}
    (MOD/'ut2004-rpg-assets.json').write_text(json.dumps({'package':'D:/games/ut2004/UnrealTournament2004/Sounds/WeaponSounds.uax','exporter':'UT2004 System/UCC.exe batchexport WeaponSounds sound wav','sounds':report},indent=2)+'\n')
if __name__=='__main__':install()
