"""Bounded real GoldSrc physics, inventory and instant-death validation."""
from pathlib import Path
import json
import os
import subprocess
import time
import argparse

ROOT=Path(__file__).resolve().parents[2]
MOD=ROOT/'Cremation'
if __name__=='__main__' and os.environ.get('CREMATION_VALIDATION_DLL_ACTIVE')!='1':
    from validation_dll import run_with_validation_dll
    run_with_validation_dll(MOD,Path(__file__).resolve());raise SystemExit
parser=argparse.ArgumentParser();parser.add_argument('--client',action='store_true',default=True);args=parser.parse_args()
env=dict(os.environ,CREMATION_REACTOR_VALIDATE='2' if args.client else '1')
env['PATH']=r'C:\Program Files (x86)\Steam'+os.pathsep+env['PATH']
(MOD/'startup-diagnostics.txt').write_text('')
with (MOD/'diagnostics/reactor-server.log').open('wb') as output:
    command=([str(ROOT/'hl.exe'),'-game','Cremation','-num_edicts','2048','-window','-w','800','-h','600','-condebug','-console','+developer','1','+exec','cremation.cfg'] if args.client else [str(ROOT/'hlds.exe'),'-console','-game','Cremation','-num_edicts','2048','-insecure','-nomaster','-port','27029','+ip','127.0.0.1','+sv_lan','1','+maxplayers','1','+map','cs_mansion'])
    p=subprocess.Popen(command,cwd=ROOT,env=env,stdout=output,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        deadline=time.monotonic()+30
        while time.monotonic()<deadline:
            if p.poll() is not None:raise RuntimeError(f'GoldSrc exited {p.returncode}')
            trace=(MOD/'startup-diagnostics.txt').read_text(errors='replace')
            if 'ReactorTestComplete' in trace:break
            time.sleep(.25)
        else:raise AssertionError('Reactor engine checks timed out')
        (MOD/'diagnostics/reactor-test-trace.log').write_text(trace)
        checks=[line for line in trace.splitlines() if line.startswith('ReactorTest ')]
        print('\n'.join(checks))
        print('\n'.join(line for line in trace.splitlines() if line.startswith('ReactorTestDetail')))
        assert len(checks)==23 and all(' PASS ' in line for line in checks),trace[-6000:]
        (MOD/'diagnostics/reactor-runtime.json').write_text(json.dumps(dict(checks=checks,passed=True,interactive_acceptance=False),indent=2)+'\n')
    finally:
        if p.poll() is None:p.terminate()
        p.wait(timeout=5)
