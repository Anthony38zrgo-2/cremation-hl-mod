"""Real GoldSrc compatibility probe for the independent CZ zombie NPC."""
from pathlib import Path
import os,subprocess,time,json
from PIL import Image,ImageDraw
ROOT=Path(__file__).resolve().parents[2];MOD=ROOT/'Cremation'
existing=set(ROOT.glob('*.bmp'))|set(MOD.glob('*.bmp'))
env=dict(os.environ,CREMATION_CZERO_ANIMATIONS='1');env['PATH']=r'C:\Program Files (x86)\Steam'+os.pathsep+env['PATH']
(MOD/'startup-diagnostics.txt').write_text('')
with (MOD/'diagnostics/czero-client.log').open('wb') as log:
 p=subprocess.Popen([str(ROOT/'hl.exe'),'-game','Cremation','-num_edicts','2048','-window','-w','1024','-h','768','-console','+exec','cremation.cfg'],cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 try:
  deadline=time.monotonic()+35
  while time.monotonic()<deadline:
   if p.poll() is not None:raise RuntimeError(f'Client exited {p.returncode}')
   trace=(MOD/'startup-diagnostics.txt').read_text(errors='replace')
   if 'CZAnimationComplete' in trace:break
   time.sleep(.25)
  else:raise AssertionError('CZ probe timed out: '+trace[-2000:])
  print('PASS: captured ten real-engine animation poses.')
 finally:
  if p.poll() is None:p.terminate()
  p.wait(timeout=5)
files=sorted((set(ROOT.glob('*.bmp'))|set(MOD.glob('*.bmp')))-existing,key=lambda p:p.stat().st_mtime)
assert len(files)==10,files
labels=['idle','walk_0','walk_64','walk_128','walk_192','run_64','run_128','attack_64','attack_160','throw_128']
for path,label in zip(files,labels):
 out=MOD/f'diagnostics/cz-animation-{label}.png';Image.open(path).save(out);print(out)
sheet=Image.new('RGB',(1200,600),'#141a22');draw=ImageDraw.Draw(sheet)
for i,label in enumerate(['idle','walk_0','walk_64','walk_128','run_64','run_128','attack_64','attack_160']):
 pic=Image.open(MOD/f'diagnostics/cz-animation-{label}.png').crop((410,290,610,490)).resize((300,300))
 x=(i%4)*300;y=(i//4)*300;sheet.paste(pic,(x,y));draw.text((x+8,y+8),label,fill='white')
sheet.save(MOD/'diagnostics/cz-animation-contact.png')
