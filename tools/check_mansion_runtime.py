"""Bounded GoldSrc test: precache, 240 native spawns/deaths, wave completion."""
from pathlib import Path
import collections
import json
import os
import re
import subprocess
import time
import argparse

ROOT = Path(__file__).resolve().parents[2]
MOD = ROOT/'Cremation'
parser = argparse.ArgumentParser()
parser.add_argument('--routes', action='store_true')
parser.add_argument('--inventory', action='store_true')
parser.add_argument('--czero-route', action='store_true')
args = parser.parse_args()
if args.czero_route:args.routes=True
env = dict(os.environ, CREMATION_MANSION_VALIDATE='3' if args.inventory else ('2' if args.routes else '1'))
if args.czero_route:env['CREMATION_MANSION_VALIDATE']='4'
env['PATH'] = str(Path(os.environ.get('ProgramFiles(x86)', 'C:/Program Files (x86)'))/'Steam') + os.pathsep + env['PATH']
(MOD/'startup-diagnostics.txt').write_text('')
with (MOD/('diagnostics/mansion-routes-server.log' if args.routes else 'diagnostics/mansion-server.log')).open('wb') as log:
    process = subprocess.Popen([str(ROOT/'hlds.exe'), '-console', '-game', 'Cremation',
                                '-num_edicts', '2048', '-insecure', '-nomaster', '-port', '27029',
                                '+ip', '127.0.0.1', '+sv_lan', '1', '+maxplayers', '1',
                                '+map', 'cs_mansion'], cwd=ROOT, env=env,
                               stdout=log, stderr=subprocess.STDOUT, stdin=subprocess.PIPE,
                               creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        deadline = time.monotonic()+(300 if args.routes else 100)
        while time.monotonic() < deadline:
            if process.poll() is not None:
                raise RuntimeError(f'GoldSrc exited: {process.returncode}')
            phases = (MOD/'startup-diagnostics.txt').read_text(errors='replace')
            complete='MansionRouteComplete CZ route reached main door' if args.czero_route else ('InventoryProbeComplete' if args.inventory else ('MansionRouteComplete 20 routes from four fronts reached main door' if args.routes else 'MansionComplete four waves cleared'))
            if complete in phases:
                break
            time.sleep(.5)
        else:
            raise AssertionError('Mansion wave test timed out; inspect startup-diagnostics.txt')
        (MOD/('diagnostics/mansion-routes-trace.log' if args.routes else 'diagnostics/mansion-waves-trace.log')).write_text(phases)
        log.flush()
        engine_output=(MOD/('diagnostics/mansion-routes-server.log' if args.routes else 'diagnostics/mansion-server.log')).read_text(errors='replace')
        assert 'not precached' not in engine_output and 'FATAL ERROR' not in engine_output, engine_output
        if args.inventory:
            items=re.findall(r'StartupInventory (weapon_\S+) attached=(\d)',phases)
            assert len(items)==8 and all(attached=='1' for name,attached in items),phases
            match=re.search(r'InventoryProbeSpawn origin=([\d.-]+),([\d.-]+),([\d.-]+)',phases)
            assert match and float(match[1])<0 and float(match[3])>190,phases
            assert 'InventoryProbeComplete owned=8 ground=0' in phases,phases
            (MOD/'diagnostics/mansion-inventory-runtime.json').write_text(json.dumps(dict(weapons=items,tt_spawn=list(match.groups()),direct_inventory=True),indent=2)+'\n')
            print('PASS: real GoldSrc fake client spawned in TT and received all eight weapons directly into native inventory.')
            raise SystemExit(0)
        if args.routes:
            arrivals = re.findall(r'MansionRouteArrival (\S+) type=(\S+) at=([\d.-]+),([\d.-]+),([\d.-]+)', phases)
            count=1 if args.czero_route else 20
            assert len(arrivals)==count and len({name for name,kind,x,y,z in arrivals})==count, arrivals
            if args.czero_route:assert arrivals[0][1]=='monster_czero_zombie',arrivals
            # Engine logs round coordinates to one decimal; a valid distance
            # below 20 can therefore appear as exactly 20 in the trace.
            assert all(((float(x)-64)**2+(float(y)+256)**2)**.5<=20.1 and abs(float(z)-56)<1 for name,kind,x,y,z in arrivals), arrivals
            (MOD/'diagnostics/mansion-routes-runtime.json').write_text(json.dumps(dict(arrivals=arrivals, completed=True),indent=2)+'\n')
            print('PASS: CZ zombie physically followed its random spawn route to the main door.' if args.czero_route else 'PASS: 20 actors from all four symmetric fronts physically reached the main door in GoldSrc.')
            raise SystemExit(0)
        spawns = re.findall(r'MansionSpawn wave=(\d+) index=(\d+) ct=(\d+) type=(\S+)', phases)
        assert len(spawns) == 244, len(spawns)
        expected = [(40,0,0,0,0),(20,20,0,0,0),(20,20,20,20,0),(10,10,10,10,40)]
        kinds = ['monster_zombie','monster_quake_zombie','monster_fastzombie','monster_poisionzombie','monster_human_grunt']
        for wave in range(1,5):
            rows = [(int(index),int(ct),kind) for w,index,ct,kind in spawns if int(w)==wave]
            special=[row for row in rows if row[2]=='monster_czero_zombie']
            assert len(special)==1 and 0<=special[0][1]<40 and special[0][0] in (40,80),special
            rows=[row for row in rows if row[2]!='monster_czero_zombie']
            counts = collections.Counter(kind for index,ct,kind in rows)
            assert tuple(counts[k] for k in kinds)==expected[wave-1], counts
            assert [ct for index,ct,kind in rows] == [i%40 for i in range(len(rows))]
            for side in range(4):
                side_counts=collections.Counter(kind for index,ct,kind in rows if ct%4==side)
                assert sum(side_counts.values())==len(rows)//4
                assert all(expected[wave-1][k]//4<=side_counts[kind]<=(expected[wave-1][k]+3)//4 for k,kind in enumerate(kinds))
        assert 'MansionError' not in phases
        result = dict(map='cs_mansion', native_spawns=244, czero_npcs=4,native_damage_deaths=True,
                      compositions=expected, round_robin_ct=True, completed=True,
                      interactive_visual_acceptance=False)
        (MOD/'diagnostics/mansion-runtime.json').write_text(json.dumps(result,indent=2)+'\n')
        print('PASS: 244 native spawns/deaths; original hordes balanced and exactly one random-spawn CZ NPC per wave.')
    finally:
        if process.poll() is None:
            process.terminate()
        process.wait(timeout=5)
