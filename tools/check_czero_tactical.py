"""Actual GoldSrc test: artifacts, chain, grenades, doors, ladder and C4."""
from pathlib import Path
import os,subprocess,time,json
ROOT=Path(__file__).resolve().parents[2];MOD=ROOT/'Cremation'
for row in json.loads((MOD/'assets-source/dmc-powerups/installed.json').read_text()):
    assert Path(row['source']).read_bytes()==(MOD/row['installed']).read_bytes()
env=dict(os.environ,CREMATION_TACTICAL_VALIDATE='1');env['PATH']=r'C:\Program Files (x86)\Steam'+os.pathsep+env['PATH']
(MOD/'startup-diagnostics.txt').write_text('')
with (MOD/'diagnostics/tactical-client.log').open('wb') as log:
 p=subprocess.Popen([str(ROOT/'hl.exe'),'-game','Cremation','-num_edicts','2048','-window','-w','1024','-h','768','-console','-condebug','+developer','1','+exec','cremation.cfg'],cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 try:
  deadline=time.monotonic()+150
  while time.monotonic()<deadline:
   if p.poll() is not None:raise RuntimeError(f'Client exited {p.returncode}')
   trace=(MOD/'startup-diagnostics.txt').read_text(errors='replace')
   if 'TacticalTestComplete' in trace:break
   time.sleep(.4)
  else:raise AssertionError('Tactical probe timed out: '+trace[-3000:])
  checks=[line for line in trace.splitlines() if line.startswith('TacticalTest ')]
  print('\n'.join(checks));print('\n'.join(line for line in trace.splitlines() if line.startswith(('TacticalRouteTimeout','CZObjective','CZBomb','CZFlash'))))
  (MOD/'diagnostics/tactical-trace.log').write_text(trace)
  assert len(checks)>=25 and all(' PASS ' in c for c in checks),trace[-5000:]
  (MOD/'diagnostics/tactical-runtime.json').write_text(json.dumps(dict(passed=True,checks=checks),indent=2)+'\n')
 finally:
  if p.poll() is None:p.terminate()
  p.wait(timeout=5)
