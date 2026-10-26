"""Exercise the existing four-wave real-engine validation with David installed."""
from pathlib import Path
import os,subprocess,time
ROOT=Path(__file__).resolve().parents[2]
MOD=ROOT/'Cremation'
env=dict(os.environ,CREMATION_MANSION_VALIDATE='1')
log=MOD/'validation-david-mansion-engine.txt'
with log.open('wb') as output:
    p=subprocess.Popen([str(ROOT/'hlds.exe'),'-console','-game','Cremation','-insecure','-nomaster',
        # This existing test kills 244 enemies in rapid succession, faster than
        # the 25-second general-gib lifetime. Raise only this child server's
        # edict budget; ordinary game configuration/lifetimes stay unchanged.
        '-num_edicts','4096','-port','27040','+ip','127.0.0.1','+sv_lan','1','+maxplayers','1','+map','cs_mansion'],
        cwd=ROOT,env=env,stdout=output,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        deadline=time.monotonic()+45
        while time.monotonic()<deadline and p.poll() is None:
            text=(MOD/'startup-diagnostics.txt').read_text(errors='replace')
            if 'MansionComplete four waves cleared' in text:break
            time.sleep(.5)
        text=(MOD/'startup-diagnostics.txt').read_text(errors='replace')
        (MOD/'validation-david-mansion-diagnostics.txt').write_text(text)
        normal=sum(l.startswith('David spawn ') for l in text.splitlines())
        assert p.poll() is None,f'engine exited {p.returncode}'
        assert 'MansionComplete four waves cleared' in text
        assert normal>=60 and 'David ERROR' not in text
        print(f'PASS: cs_mansion completed all four existing mixed waves; {normal} normal-zombie spawns used David.')
    finally:
        if p.poll() is None:p.terminate()
        p.wait(timeout=5)
errors=log.read_text(errors='replace')
assert all(s not in errors for s in ('Host_Error','FATAL ERROR','not precached'))
