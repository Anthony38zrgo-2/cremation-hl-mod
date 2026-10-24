"""Replace GoldSrc gore and explosion paths with UT2004 source recordings."""
from pathlib import Path
import wave,struct,math,hashlib,json,shutil
MOD=Path(__file__).resolve().parents[1]
PLAYERS=MOD/'assets-source/ut2004_gib_explosions/PlayerSounds'
WEAPONS=MOD/'assets-source/ut2004_rocket/WeaponSounds'
BACKUP=MOD/'backups/before-ut2004-gib-explosion-sounds'

def mappings():
 out={'sound/common/bodysplat.wav':PLAYERS/'NewGibs.NewGib1.wav'}
 for i in range(1,5):out[f'sound/common/bodydrop{i}.wav']=PLAYERS/f'ImpactSounds.BodyHit{i}.wav'
 for i in range(1,8):out[f'sound/debris/flesh{i}.wav']=PLAYERS/f'Final.Giblets{(i-1)%5+1}.wav'
 for i in range(1,3):out[f'sound/debris/bustflesh{i}.wav']=PLAYERS/f'NewGibs.NewGib{i+1}.wav'
 for i in range(3,6):out[f'sound/weapons/explode{i}.wav']=WEAPONS/f'BaseImpactAndExplosions.BExplosion{i-2}.wav'
 out['sound/ricochet/blast.wav']=WEAPONS/'BaseImpactAndExplosions.BExplosion4.wav'
 for target in ['sound/weapons/mortarhit.wav','sound/weapons/energy_sing_explosion2.wav']:
  out[target]=WEAPONS/'BaseImpactAndExplosions.BExplosion5.wav'
 return out

def install():
 report={}
 for relative,source in mappings().items():
  target=MOD/relative;backup=BACKUP/relative
  if target.exists() and not backup.exists():
   backup.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(target,backup)
  with wave.open(str(source),'rb') as w:
   rate=w.getframerate();width=w.getsampwidth();channels=w.getnchannels();frames=w.getnframes();raw=w.readframes(frames)
  if width==2:values=list(struct.unpack('<'+'h'*(len(raw)//2),raw))
  elif width==1:values=[(v-128)*256 for v in raw]
  else:raise ValueError(f'Unsupported WAV: {source}')
  samples=[sum(values[i:i+channels])/channels for i in range(0,len(values),channels)]
  step=rate/22050;audio=[]
  for n in range(math.ceil(len(samples)/step)):
   position=min(n*step,len(samples)-1);i=int(position);alpha=position-i
   audio.append(round(samples[i]*(1-alpha)+samples[min(i+1,len(samples)-1)]*alpha))
  target.parent.mkdir(parents=True,exist_ok=True)
  with wave.open(str(target),'wb') as w:
   w.setparams((1,2,22050,0,'NONE','not compressed'));w.writeframes(struct.pack('<'+'h'*len(audio),*audio))
  report[relative]={'source':source.relative_to(MOD).as_posix(),'source_rate':rate,'source_bits':width*8,'duration_seconds':len(audio)/22050,'sha256':hashlib.sha256(target.read_bytes()).hexdigest(),'backup':backup.relative_to(MOD).as_posix() if backup.exists() else None}
 (MOD/'ut2004-gib-explosion-assets.json').write_text(json.dumps({'packages':['D:/games/ut2004/UnrealTournament2004/Sounds/PlayerSounds.uax','D:/games/ut2004/UnrealTournament2004/Sounds/WeaponSounds.uax'],'exporter':'UT2004 UCC.exe batchexport <package> sound wav <directory>','installed_format':'mono PCM16 22050 Hz','installed':report},indent=2)+'\n')
 manifest=MOD/'file-manifest.json';data=json.loads(manifest.read_text())
 for name,item in report.items():data[name]=item['sha256']
 manifest.write_text(json.dumps(data,indent=2)+'\n')
 print(f'Installed {len(report)} UT2004 gore/explosion sound replacements.')
if __name__=='__main__':install()
