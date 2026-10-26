"""Four actual-client captures of the authentic floating DMC models."""
from pathlib import Path
import os,subprocess,time
from PIL import Image,ImageDraw
ROOT=Path(__file__).resolve().parents[2];MOD=ROOT/'Cremation'
existing=set(ROOT.glob('*.bmp'))|set(MOD.glob('*.bmp'))
env=dict(os.environ,CREMATION_DMC_PREVIEW='1');env['PATH']=r'C:\Program Files (x86)\Steam'+os.pathsep+env['PATH']
(MOD/'startup-diagnostics.txt').write_text('')
with (MOD/'diagnostics/dmc-preview.log').open('wb') as log:
 p=subprocess.Popen([str(ROOT/'hl.exe'),'-game','Cremation','-num_edicts','2048','-window','-w','1024','-h','768','-console','+exec','cremation.cfg'],cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 try:
  deadline=time.monotonic()+25
  while time.monotonic()<deadline:
   if p.poll() is not None:raise RuntimeError(f'Preview exited {p.returncode}')
   if 'DMCPreviewComplete' in (MOD/'startup-diagnostics.txt').read_text(errors='replace'):break
   time.sleep(.25)
  else:raise AssertionError('DMC preview timed out')
 finally:
  if p.poll() is None:p.terminate()
  p.wait(timeout=5)
files=sorted((set(ROOT.glob('*.bmp'))|set(MOD.glob('*.bmp')))-existing,key=lambda p:p.stat().st_mtime)
assert len(files)==4,files
sheet=Image.new('RGB',(1200,900));draw=ImageDraw.Draw(sheet)
for i,(path,label) in enumerate(zip(files,('Quad Damage','Invulnerabilidad','Invisibilidad','Traje protector'))):
 pic=Image.open(path);pic.save(MOD/f'diagnostics/dmc-{i}.png');pic=pic.resize((600,450));x=i%2*600;y=i//2*450;sheet.paste(pic,(x,y));draw.text((x+12,y+12),label,fill='white')
sheet.save(MOD/'diagnostics/dmc-powerups.png');print(MOD/'diagnostics/dmc-powerups.png')
