"""Install UT2004 Flak/Link recordings and a Link fire variant four semitones lower."""
from pathlib import Path
import wave,struct,math,json,hashlib
MOD=Path(__file__).resolve().parents[1]
FILES={
 'flak_fire.wav':('BaseFiringSounds.BFlakCannonFire.wav',0),
 'flak_altfire.wav':('BaseFiringSounds.BFlakCannonAltFire.wav',0),
 'flak_impact.wav':('BaseProjectileSounds.BFlakCannonProjectile.wav',0),
 'flak_equip.wav':('FlakCannon.SwitchToFlakCannon.wav',0),
 'link_fire.wav':('LinkGun.BLinkedFire.wav',0),
 'link_altfire.wav':('LinkGun.BLinkedFire.wav',-4),
 'link_equip.wav':('LinkGun.SwitchToLinkGun.wav',0),
}
def install():
 source=MOD/'assets-source/ut2004_rocket/WeaponSounds'
 target=MOD/'sound/weapons/ut2004';target.mkdir(parents=True,exist_ok=True)
 report={}
 for name,(original,semitones) in FILES.items():
  with wave.open(str(source/original),'rb') as w:
   rate=w.getframerate();width=w.getsampwidth();channels=w.getnchannels();raw=w.readframes(w.getnframes())
  if width==1:samples=[(v-128)*256 for v in raw]
  elif width==2:samples=list(struct.unpack('<'+'h'*(len(raw)//2),raw))
  else:raise ValueError(original)
  samples=[sum(samples[i:i+channels])/channels for i in range(0,len(samples),channels)]
  step=rate/22050*2**(semitones/12)
  output=[]
  for n in range(math.ceil(len(samples)/step)):
   position=min(n*step,len(samples)-1);i=int(position);alpha=position-i
   value=samples[i]*(1-alpha)+samples[min(i+1,len(samples)-1)]*alpha
   output.append(max(-32768,min(32767,round(value))))
  with wave.open(str(target/name),'wb') as w:
   w.setparams((1,2,22050,0,'NONE','not compressed'));w.writeframes(struct.pack('<'+'h'*len(output),*output))
  report[name]={'source':original,'semitones':semitones,'pitch_ratio':2**(semitones/12),'sha256':hashlib.sha256((target/name).read_bytes()).hexdigest()}
 (MOD/'ut2004-flak-link-assets.json').write_text(json.dumps({'package':'D:/games/ut2004/UnrealTournament2004/Sounds/WeaponSounds.uax','format':'mono PCM16 22050 Hz','pitch_method':'linear resampling; lowers pitch and lengthens playback','sounds':report},indent=2)+'\n')
if __name__=='__main__':install()
