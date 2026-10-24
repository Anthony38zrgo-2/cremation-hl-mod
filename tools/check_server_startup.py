"""Bounded real GoldSrc startup check; no game UI or input automation."""
from pathlib import Path
import subprocess,time,json,argparse,re
parser=argparse.ArgumentParser();parser.add_argument("--seconds",type=int,default=8,choices=range(8,31));seconds=parser.parse_args().seconds

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
        process.wait(timeout=seconds)
        raise RuntimeError(f'Server exited unexpectedly: {process.returncode}')
    except subprocess.TimeoutExpired:
        phases=diagnostics.read_text() if diagnostics.exists() else ''
        assert 'Cremation DLL initialized' in phases and 'ServerActivate rapidcore' in phases,phases
        movement=re.findall(r'HordeMovement teleports=(\d+) near_spawn_256=(\d+)',phases)
        assert movement and all(int(jumps)==0 and int(near)==0 for jumps,near in movement),phases
        observed=[int(n) for n in re.findall(r'HordeProgress zombies=40 moved_over_32=(\d+)',phases)]
        assert observed and max(observed)>0,phases
        print(phases.strip())
        print(f'PASS: real GoldSrc engine loaded the custom DLL, spawned Rapidcore and remained running for {seconds} seconds.')
        (ROOT/'Cremation/server-startup-result.json').write_text(json.dumps(dict(map='rapidcore',custom_dll=True,server_activate=True,survived_seconds=seconds,zombie_teleports=0,spawn_proximity_violations=0,movers_observed=max(observed)),indent=2)+'\n')
    finally:
        if process.poll() is None:process.terminate()
        process.wait(timeout=5)
