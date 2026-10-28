"""Isolated real GoldSrc comparison of Randy and the original fast zombie."""
from pathlib import Path
import json, os, subprocess, time

ROOT=Path(__file__).resolve().parents[2];MOD=ROOT/'Cremation'
if __name__=='__main__' and os.environ.get('CREMATION_VALIDATION_DLL_ACTIVE')!='1':
    from validation_dll import run_with_validation_dll
    run_with_validation_dll(MOD,Path(__file__).resolve());raise SystemExit

env=dict(os.environ,CREMATION_DAVID_VALIDATE='1',CREMATION_RANDY_VALIDATE='1')
(MOD/'startup-diagnostics.txt').write_text('')
with (MOD/'diagnostics/randy-server.log').open('wb') as output:
    process=subprocess.Popen([str(ROOT/'hlds.exe'),'-console','-game','Cremation','-insecure','-nomaster',
        '-port','27039','+ip','127.0.0.1','+sv_lan','1','+maxplayers','1','+map','rapidcore'],
        cwd=ROOT,env=env,stdout=output,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        deadline=time.monotonic()+25
        while time.monotonic()<deadline:
            if process.poll() is not None:raise RuntimeError(f'GoldSrc exited: {process.returncode}')
            trace=(MOD/'startup-diagnostics.txt').read_text(errors='replace')
            if 'RandyTestComplete' in trace:break
            time.sleep(.25)
        else:raise AssertionError('Randy runtime check timed out')
        (MOD/'diagnostics/randy-trace.log').write_text(trace)
        checks=[line for line in trace.splitlines() if line.startswith(('RandyTest ','RandyProbe '))]
        print('\n'.join(checks))
        assert 'RandyTest FAIL' not in trace and sum('RandyTest PASS' in line for line in checks)>=6
        (MOD/'diagnostics/randy-runtime.json').write_text(json.dumps(dict(passed=True,checks=checks),indent=2)+'\n')
    finally:
        if process.poll() is None:process.terminate()
        process.wait(timeout=5)
