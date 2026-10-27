"""Save in one actual client process, verify restoration in a fresh process."""
from pathlib import Path
import os,subprocess,time
ROOT=Path(__file__).resolve().parents[2];MOD=ROOT/'Cremation'
if __name__=='__main__' and os.environ.get('CREMATION_VALIDATION_DLL_ACTIVE')!='1':
    from validation_dll import run_with_validation_dll
    run_with_validation_dll(MOD,Path(__file__).resolve());raise SystemExit
for mode in (1,2):
 env=dict(os.environ,CREMATION_SAVE_PROBE=str(mode),CREMATION_HUD_TRACE='1');env['PATH']=r'C:\Program Files (x86)\Steam'+os.pathsep+env['PATH']
 (MOD/'startup-diagnostics.txt').write_text('');(MOD/'diagnostics/hud-client-trace.log').write_text('')
 command=[str(ROOT/'hl.exe'),'-game','Cremation','-num_edicts','2048','-window','-w','800','-h','600','-console','-condebug']
 command+=['+exec','cremation.cfg'] if mode==1 else ['+load','cremation_tactical_probe']
 with (MOD/f'diagnostics/tactical-save-{mode}.log').open('wb') as log:
  p=subprocess.Popen(command,cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
  try:
   deadline=time.monotonic()+30
   while time.monotonic()<deadline:
    if p.poll() is not None:raise RuntimeError(f'Save probe process exited {p.returncode}')
    trace=(MOD/'startup-diagnostics.txt').read_text(errors='replace')
    if ('SaveProbeCreated' if mode==1 else 'SaveProbeComplete') in trace:time.sleep(.5);break
    time.sleep(.3)
   else:raise AssertionError('Save probe timed out: '+trace[-1500:])
   if mode==1:assert (MOD/'SAVE/cremation_tactical_probe.sav').exists()
   else:
    checks=[line for line in trace.splitlines() if line.startswith('SaveProbe ')]
    render=(MOD/'diagnostics/hud-client-trace.log').read_text();assert 'state=1 bar=0 power=9 kit=1 alive=1 sprites=1' in render,render
    print('PASS: restored bomb and DMC timers reach the client HUD.')
    print('\n'.join(checks));assert len(checks)==5 and all(' PASS ' in c for c in checks),trace[-2500:]
  finally:
   if p.poll() is None:p.terminate()
   p.wait(timeout=5)
