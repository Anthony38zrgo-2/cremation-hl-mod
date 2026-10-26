"""Bounded dedicated GoldSrc acceptance; never operates on the user's session."""
from pathlib import Path
import os, subprocess

ROOT=Path(__file__).resolve().parents[2]
log=ROOT/'Cremation/validation-david-engine.txt'
env=dict(os.environ,CREMATION_DAVID_VALIDATE='1')
with log.open('wb') as output:
    p=subprocess.Popen([str(ROOT/'hlds.exe'),'-console','-game','Cremation','-insecure',
        '-nomaster','-port','27039','+ip','127.0.0.1','+sv_lan','1','+maxplayers','1',
        '+map','rapidcore'],cwd=ROOT,env=env,stdout=output,stderr=subprocess.STDOUT,
        creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        try:
            p.wait(timeout=12)
        except subprocess.TimeoutExpired:
            pass
        text=(ROOT/'Cremation/startup-diagnostics.txt').read_text(errors='replace')
        (ROOT/'Cremation/validation-david-diagnostics.txt').write_text(text)
        lines=[line for line in text.splitlines() if 'DavidTest' in line or 'DavidProbe' in line or 'David ERROR' in line]
        print('\n'.join(lines))
        assert p.poll() is None,f'HLDS exited unexpectedly: {p.returncode}; see {log}'
        assert 'DavidTest COMPLETE' in text,'engine acceptance did not complete'
        assert 'DavidTest FAIL' not in text and 'David ERROR' not in text
        print('PASS: dedicated GoldSrc David damage, anatomy, native movement, crowd gating, gibs and serialization.')
    finally:
        if p.poll() is None:p.terminate()
        p.wait(timeout=5)
