"""Real client HUD receive/render assertions and six screenshots."""
from pathlib import Path
import os,subprocess,time,json
from PIL import Image,ImageDraw
ROOT=Path(__file__).resolve().parents[2];MOD=ROOT/'Cremation'
if __name__=='__main__' and os.environ.get('CREMATION_VALIDATION_DLL_ACTIVE')!='1':
    from validation_dll import run_with_validation_dll
    run_with_validation_dll(MOD,Path(__file__).resolve());raise SystemExit
for row in json.loads((MOD/'assets-source/tactical-hud/installed.json').read_text()):assert Path(row['source']).read_bytes()==(MOD/row['installed']).read_bytes()
existing=set(ROOT.glob('*.bmp'))|set(MOD.glob('*.bmp'))
env=dict(os.environ,CREMATION_HUD_VALIDATE='1');env['PATH']=r'C:\Program Files (x86)\Steam'+os.pathsep+env['PATH']
(MOD/'startup-diagnostics.txt').write_text('');(MOD/'diagnostics/hud-client-trace.log').write_text('')
with (MOD/'diagnostics/hud-probe.log').open('wb') as log:
 p=subprocess.Popen([str(ROOT/'hl.exe'),'-game','Cremation','-num_edicts','2048','-window','-w','1280','-h','800','-console','+exec','cremation.cfg'],cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 try:
  deadline=time.monotonic()+35
  while time.monotonic()<deadline:
   if p.poll() is not None:raise RuntimeError(f'HUD client exited {p.returncode}')
   trace=(MOD/'startup-diagnostics.txt').read_text(errors='replace')
   if 'HUDProbeComplete' in trace:time.sleep(.4);break
   time.sleep(.2)
  else:raise AssertionError('HUD probe timed out: '+trace[-1500:])
  render=(MOD/'diagnostics/hud-client-trace.log').read_text()
  for expected in ('state=1 bar=0 power=15 kit=1 alive=1 sprites=1','state=3 bar=1 power=15 kit=1 alive=1 sprites=1','state=2 bar=0 power=15 kit=1 alive=1 sprites=1','state=4 bar=0 power=15 kit=1 alive=1 sprites=1','state=4 bar=0 power=0 kit=1 alive=1 sprites=1','state=5 bar=0 power=0 kit=0 alive=0 sprites=1'):
   assert expected in render,(expected,render)
  print('PASS: planted, defusing, cancelled, defused, powerups expired and explosion render with original sprites.')
 finally:
  if p.poll() is None:p.terminate()
  p.wait(timeout=5)
files=sorted((set(ROOT.glob('*.bmp'))|set(MOD.glob('*.bmp')))-existing,key=lambda p:p.stat().st_mtime)
assert len(files)==6,files
sheet=Image.new('RGB',(1920,800));draw=ImageDraw.Draw(sheet)
for i,(path,label) in enumerate(zip(files,('plantada','desactivando','cancelada','desactivada','powerups-expirados','explosion'))):
 pic=Image.open(path);pic.save(MOD/f'diagnostics/hud-{label}.png');x=i%3*640;y=i//3*400;sheet.paste(pic.resize((640,400)),(x,y));draw.text((x+12,y+12),label,fill='white')
sheet.save(MOD/'diagnostics/tactical-hud.png')
