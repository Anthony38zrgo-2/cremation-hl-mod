"""Bounded real GoldSrc startup check; no game UI or input automation."""
from pathlib import Path
import subprocess,time,json

ROOT=Path(__file__).resolve().parents[2]
log=ROOT/'Cremation/server-startup-check.txt'
diagnostics=ROOT/'Cremation/startup-diagnostics.txt'
with log.open('wb') as output:
    process=subprocess.Popen([str(ROOT/'hlds.exe'),'-console','-game','Cremation',
        '-insecure','-nomaster','-port','27029','+ip','127.0.0.1',
        '+sv_lan','1','+maxplayers','1','+map','rapidcore'],cwd=ROOT,
        stdin=subprocess.PIPE,stdout=output,stderr=subprocess.STDOUT,
        creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        process.wait(timeout=8)
        raise RuntimeError(f'Server exited unexpectedly: {process.returncode}')
    except subprocess.TimeoutExpired:
        phases=diagnostics.read_text() if diagnostics.exists() else ''
        assert 'Cremation DLL initialized' in phases and 'ServerActivate rapidcore' in phases,phases
        print(phases.strip())
        print('PASS: real GoldSrc engine loaded the custom DLL, spawned Rapidcore and remained running for 8 seconds.')
        (ROOT/'Cremation/server-startup-result.json').write_text(json.dumps(dict(map='rapidcore',custom_dll=True,server_activate=True,survived_seconds=8),indent=2)+'\n')
    finally:
        if process.poll() is None:process.terminate()
        process.wait(timeout=5)
