"""Real GoldSrc compatibility probe for the independent CZ zombie NPC."""
from pathlib import Path
import os,subprocess,time,json
from PIL import Image
ROOT=Path(__file__).resolve().parents[2];MOD=ROOT/'Cremation'
existing=set(ROOT.glob('*.bmp'))|set(MOD.glob('*.bmp'))
env=dict(os.environ,CREMATION_CZERO_VALIDATE='1');env['PATH']=r'C:\Program Files (x86)\Steam'+os.pathsep+env['PATH']
(MOD/'startup-diagnostics.txt').write_text('')
with (MOD/'diagnostics/czero-client.log').open('wb') as log:
 p=subprocess.Popen([str(ROOT/'hl.exe'),'-game','Cremation','-num_edicts','2048','-window','-w','1024','-h','768','-console','+exec','cremation.cfg'],cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 try:
  deadline=time.monotonic()+35
  while time.monotonic()<deadline:
   if p.poll() is not None:raise RuntimeError(f'Client exited {p.returncode}')
   trace=(MOD/'startup-diagnostics.txt').read_text(errors='replace')
   if 'CZTestComplete' in trace:break
   time.sleep(.25)
  else:raise AssertionError('CZ probe timed out: '+trace[-2000:])
  checks=[line for line in trace.splitlines() if line.startswith('CZTest ')]
  print('\n'.join(checks));(MOD/'diagnostics/czero-trace.log').write_text(trace)
  assert len(checks)==23 and all(' PASS ' in line for line in checks),trace[-6000:]
  (MOD/'diagnostics/czero-runtime.json').write_text(json.dumps(dict(checks=checks,passed=True,interactive_acceptance=False),indent=2)+'\n')
 finally:
  if p.poll() is None:p.terminate()
  p.wait(timeout=5)
files=sorted((set(ROOT.glob('*.bmp'))|set(MOD.glob('*.bmp')))-existing,key=lambda p:p.stat().st_mtime)
if files:
 out=MOD/'diagnostics/czero-zombie-preview.png';Image.open(files[0]).save(out);print(out)
 if len(files)>1:
  out=MOD/'diagnostics/czero-smoke-preview.png';Image.open(files[-1]).save(out);print(out)
