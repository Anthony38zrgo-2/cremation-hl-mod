"""Install UT classic recordings from locally exported packages; keep runtime paths stable."""
from pathlib import Path
import wave,struct,math,json,hashlib,shutil
MOD=Path(__file__).resolve().parents[1]
SOURCE=MOD/'assets-source/ut99_sounds'
FILES={
'weapons/ut2004/link_fire.wav':('Botpack/PulseGun.PulseFire.wav',0),
'weapons/ut2004/link_altfire.wav':('Botpack/PulseGun.PulseBolt.wav',-4),
'weapons/ut2004/link_equip.wav':('Botpack/PulseGun.PulsePickup.wav',0),
'weapons/ut2004/flak_fire.wav':('UnrealShare/flak.shot1.wav',0),
'weapons/ut2004/flak_altfire.wav':('UnrealShare/flak.Explode1.wav',0),
'weapons/ut2004/flak_impact.wav':('UnrealI/Cannon.CannonExplode.wav',0),
'weapons/ut2004/flak_equip.wav':('UnrealI/flak.pdown.wav',0),
'weapons/ut2004/fire.wav':('UnrealShare/Eightball.Ignite.wav',0),
'weapons/ut2004/altfire.wav':('UnrealShare/Eightball.Ignite.wav',0),
'weapons/ut2004/load.wav':('UnrealShare/Eightball.Loading.wav',0),
'weapons/ut2004/plunger.wav':('UnrealShare/Eightball.BarrelMove.wav',0),
'weapons/ut2004/flight.wav':('Botpack/RocketLauncher.RocketFly1.wav',0),
'weapons/ut2004/equip.wav':('UnrealShare/Eightball.Selecting.wav',0),
'common/bodysplat.wav':('UnrealShare/Gibs.Gib1.wav',0),
'ricochet/blast.wav':('UnrealShare/General.Expl04.wav',0),
'weapons/mortarhit.wav':('UnrealShare/General.Explo1.wav',0),
'weapons/energy_sing_explosion2.wav':('UnrealShare/General.SpecialExpl.wav',0),
}
for i in range(1,5):FILES[f'common/bodydrop{i}.wav']=(f'UnrealShare/Gibs.{"Thump" if i%2 else "Gib1"}.wav',0)
for i in range(1,8):FILES[f'debris/flesh{i}.wav']=(f'UnrealShare/Gibs.Gib{(i-1)%5+1}.wav',0)
for i in range(1,3):FILES[f'debris/bustflesh{i}.wav']=(f'UnrealShare/Gibs.gibP{3 if i==1 else 4}.wav',0)
for i,name in enumerate(['Expl03','Expl04','Explg02'],3):FILES[f'weapons/explode{i}.wav']=(f'UnrealShare/General.{name}.wav',0)

REVERTED=['ricochet/blast.wav', 'weapons/mortarhit.wav', 'weapons/energy_sing_explosion2.wav', 'weapons/explode3.wav', 'weapons/explode4.wav', 'weapons/explode5.wav', 'weapons/ut2004/fire.wav', 'weapons/ut2004/altfire.wav']
for name in REVERTED:FILES.pop(name,None)

# User-selected general explosion recordings.
FILES.update({'weapons/explode3.wav': ('Botpack/General.Expl04.wav', 0), 'weapons/explode4.wav': ('Botpack/General.Explg02.wav', 0), 'weapons/explode5.wav': ('Botpack/flak.expl2.wav', 0), 'weapons/mortarhit.wav': ('Botpack/General.Expl04.wav', 0), 'weapons/energy_sing_explosion2.wav': ('Botpack/General.Explg02.wav', 0), 'ricochet/blast.wav': ('Botpack/flak.expl2.wav', 0)})

# User-selected impacts for body fragments.
FILES.update({'debris/flesh1.wav': ('UnrealShare/Gibs.gibP4.wav', 0), 'debris/flesh2.wav': ('UnrealShare/Gibs.gibP5.wav', 0), 'debris/flesh3.wav': ('UnrealShare/Gibs.gibP6.wav', 0), 'debris/flesh4.wav': ('UnrealShare/Gibs.gibP4.wav', 0), 'debris/flesh5.wav': ('UnrealShare/Gibs.gibP5.wav', 0), 'debris/flesh6.wav': ('UnrealShare/Gibs.gibP6.wav', 0), 'debris/flesh7.wav': ('UnrealShare/Gibs.gibP4.wav', 0), 'debris/bustflesh1.wav': ('UnrealShare/Gibs.gibP4.wav', 0), 'debris/bustflesh2.wav': ('UnrealShare/Gibs.gibP5.wav', 0), 'common/bodydrop1.wav': ('UnrealShare/Gibs.gibP4.wav', 0), 'common/bodydrop2.wav': ('UnrealShare/Gibs.gibP5.wav', 0), 'common/bodydrop3.wav': ('UnrealShare/Gibs.gibP6.wav', 0), 'common/bodydrop4.wav': ('UnrealShare/Gibs.gibP4.wav', 0)})

def install():
 report={};backup=MOD/'backups/before-ut99-sounds'
 for name,(src,pitch) in FILES.items():
  source=SOURCE/src;target=MOD/'sound'/name;save=backup/name
  if target.exists() and not save.exists():save.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(target,save)
  with wave.open(str(source),'rb') as w:rate=w.getframerate();channels=w.getnchannels();width=w.getsampwidth();raw=w.readframes(w.getnframes())
  if width==1:values=[(v-128)*256 for v in raw]
  elif width==2:values=struct.unpack('<'+'h'*(len(raw)//2),raw)
  else:raise ValueError(source)
  mono=[sum(values[i:i+channels])/channels for i in range(0,len(values),channels)]
  step=rate/22050*2**(pitch/12);out=[]
  for n in range(math.ceil(len(mono)/step)):
   pos=min(n*step,len(mono)-1);i=int(pos);a=pos-i;out.append(max(-32768,min(32767,round(mono[i]*(1-a)+mono[min(i+1,len(mono)-1)]*a))))
  target.parent.mkdir(parents=True,exist_ok=True)
  with wave.open(str(target),'wb') as w:w.setparams((1,2,22050,0,'NONE','not compressed'));w.writeframes(struct.pack('<'+'h'*len(out),*out))
  report['sound/'+name]={'source':src,'semitones':pitch,'seconds':len(out)/22050,'sha256':hashlib.sha256(target.read_bytes()).hexdigest()}
 manifest=MOD/'file-manifest.json';data=json.loads(manifest.read_text());data.update({k:v['sha256'] for k,v in report.items()});manifest.write_text(json.dumps(data,indent=2)+'\n')
 (MOD/'ut99-sound-assets.json').write_text(json.dumps({'installation':'C:/UnrealTournament','packages':['System/Botpack.u','System/UnrealShare.u','System/UnrealI.u'],'format':'mono PCM16 22050 Hz','linkgun_equivalent':'UT classic Pulse Gun, confirmed from exported PulseGun.uc defaults','rocket_secondary':'Ignite: charged mode still launches rockets, not EightAltFire grenades','installed':report},indent=2)+'\n')
 print(f'Installed {len(report)} UT classic sound replacements.')
if __name__=='__main__':install()
