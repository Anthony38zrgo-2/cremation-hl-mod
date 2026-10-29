"""Bounded GoldSrc test: precache, 240 native spawns/deaths, wave completion."""
from pathlib import Path
import collections
import json
import math
import os
import re
import subprocess
import time
import argparse

ROOT = Path(__file__).resolve().parents[2]
MOD = ROOT/'Cremation'
if __name__=='__main__' and os.environ.get('CREMATION_VALIDATION_DLL_ACTIVE')!='1':
    from validation_dll import run_with_validation_dll
    run_with_validation_dll(MOD,Path(__file__).resolve());raise SystemExit
parser = argparse.ArgumentParser()
parser.add_argument('--routes', action='store_true')
parser.add_argument('--inventory', action='store_true')
parser.add_argument('--czero-route', action='store_true')
parser.add_argument('--door-break', action='store_true')
parser.add_argument('--door-touch', action='store_true')
args = parser.parse_args()
if args.czero_route:args.routes=True
if args.door_break:args.routes=True
if args.door_touch:assert not args.door_break
env = dict(os.environ, CREMATION_MANSION_VALIDATE='3' if args.inventory else ('2' if args.routes else '1'))
if args.czero_route:env['CREMATION_MANSION_VALIDATE']='4'
if args.door_break:env['CREMATION_MANSION_VALIDATE']='6'
if args.door_touch:env['CREMATION_MANSION_VALIDATE']='7'
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
        deadline = time.monotonic()+(150 if args.door_break else (30 if args.door_touch else (300 if args.routes else 100)))
        while time.monotonic() < deadline:
            if process.poll() is not None:
                raise RuntimeError(f'GoldSrc exited: {process.returncode}')
            phases = (MOD/'startup-diagnostics.txt').read_text(errors='replace')
            complete='MansionDoorTouchComplete both native rotating doors opened through their player touch callback' if args.door_touch else ('MansionDoorBreakComplete both native doors reached zero HP, swung open and remained non-solid' if args.door_break else ('MansionRouteComplete CZ used side stairs and reached upper hostage zone' if args.czero_route else ('InventoryProbeComplete' if args.inventory else ('MansionRouteComplete 20 routes from four fronts reached their assigned door' if args.routes else 'MansionComplete four waves cleared'))))
            if args.door_break:complete='MansionDoorStableComplete 20 seconds of post-break horde navigation'
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
            expected={'weapon_pipewrench','weapon_ricochet','weapon_357'}
            available=expected|{'weapon_nailgun','weapon_bounce_shotgun','weapon_shockrifle','weapon_rpg','weapon_handgrenade'}
            assert len(items)==3 and {name for name,attached in items}==expected and all(attached=='1' for name,attached in items),phases
            registry=re.findall(r'WeaponRegistry raw=\S+ name=(\S+) id=(\d+) group=(\d+) position=(\d+) ammo=(-?\d+)',phases)
            assert {name for name,*rest in registry}==available and len(registry)==8,registry
            assert len({row[1] for row in registry})==8 and len({(row[2],row[3]) for row in registry})==8,registry
            assert 'WeaponRegistryConflict' not in phases,phases
            selections=re.findall(r'WeaponRegistrySelection name=(\S+) id=(\d+) owned=(\d+) model=(\S+)',phases)
            assert len(selections)==3 and all(owned=='1' for name,id,owned,model in selections),selections
            by_name={name:model for name,id,owned,model in selections}
            assert by_name['weapon_357']=='models/v_357.mdl',selections
            assert len(re.findall(r'^MansionGrenade mansion_grenade_[0-3]$',phases,re.M))==4,phases
            supplies=re.findall(r'MansionRevolverAmmo index=(\d+) floor=(\d+) origin=([^\n]+)',phases)
            assert len(supplies)==8 and {int(row[0]) for row in supplies}==set(range(8)),supplies
            ammo_probe=re.search(r'MansionRevolverAmmoProbe boxes=(\d+) upper=(\d+) lower=(\d+) before=(\d+) after=(\d+)',phases)
            assert ammo_probe and tuple(map(int,ammo_probe.groups()[:3]))==(8,4,4),phases
            assert int(ammo_probe[5])>int(ammo_probe[4]),ammo_probe.groups()
            match=re.search(r'InventoryProbeSpawn origin=([\d.-]+),([\d.-]+),([\d.-]+)',phases)
            assert match and float(match[1])<0 and float(match[3])>190,phases
            assert 'InventoryProbeComplete owned=3 ground=0' in phases,phases
            (MOD/'diagnostics/mansion-inventory-runtime.json').write_text(json.dumps(dict(weapons=items,registry=registry,selections=selections,revolver_ammo=supplies,native_ammo_pickup=ammo_probe.groups(),tt_spawn=list(match.groups()),direct_inventory=True),indent=2)+'\n')
            print('PASS: real GoldSrc fake client spawned in TT and received only revolver, Ricochet and chainsaw; unique HUD IDs/positions and revolver selection and eight house ammo boxes with native pickup passed.')
            raise SystemExit(0)
        if args.routes:
            arrivals = re.findall(r'MansionRouteArrival (\S+) type=(\S+) at=([\d.-]+),([\d.-]+),([\d.-]+) target=(\S*) phase=(\d+)', phases)
            if args.door_break:
                door_arrivals=re.findall(r'MansionDoorTestArrival (\S+) door=(\d+) at=([\d.-]+),([\d.-]+),([\d.-]+)',phases)
                assert {int(row[1]) for row in door_arrivals}=={1,2},(door_arrivals,phases)
                assert 'MansionDoorBreakComplete both native doors reached zero HP, swung open and remained non-solid' in phases,phases
                budget=re.search(r'MansionNavBudget started=(\d+) completed=(\d+) failed=(\d+) paths=(\d+) peak_paths=(\d+) peak_edges=(\d+) peak_traces=(\d+) peak_ms=([\d.]+) peak_move_ms=([\d.]+)',phases)
                assert budget,phases
                metrics=dict(zip(('started','completed','failed','paths','peak_paths','peak_edges','peak_traces','peak_ms','peak_move_ms'),map(float,budget.groups())))
                assert metrics['started']>0 and metrics['completed']>0,metrics
                assert metrics['peak_paths']<=80 and metrics['peak_edges']<=8192 and metrics['peak_traces']<=256,metrics
                assert metrics['peak_ms']<20 and metrics['peak_move_ms']<25,metrics
                upper=re.search(r'upper_completed=(\d+) upper_steps=(\d+)',phases)
                assert upper and int(upper[1])>0 and int(upper[2])>0,phases
                metrics.update(upper_completed=int(upper[1]),upper_steps=int(upper[2]))
                assert len(re.findall(r'MansionSpawn wave=3 ',phases))==80,phases
                assert 'MansionDoorStableComplete 20 seconds of post-break horde navigation' in phases,phases
                (MOD/'diagnostics/mansion-door-break-runtime.json').write_text(json.dumps(dict(arrivals=door_arrivals,actors=80,post_break_seconds=20,navigation=metrics,completed=True),indent=2)+'\n')
                print('PASS: 80 actors broke both doors and navigated for 20 seconds afterward; bounded navigation metrics:',metrics)
                raise SystemExit(0)
            count=1 if args.czero_route else 20
            assert len(arrivals)==count and len({name for name,*rest in arrivals})==count, arrivals
            if args.czero_route:
                name,kind,x,y,z,target,phase=arrivals[0]
                assert kind=='monster_czero_zombie' and target in ('','none') and phase=='4',arrivals
                assert 'MansionRouteComplete CZ used side stairs and reached upper hostage zone' in phases,phases
                assert math.dist((float(x),float(y),float(z)),(-624,-176,192.1))<24.1,arrivals
            else:
                for name,kind,x,y,z,target,phase in arrivals:
                    if target=='mansion_wait_door_1':assert math.dist((float(x),float(y)),(64,-256))<160,arrivals
                    else:
                        assert target=='mansion_wait_door_2',arrivals
                        assert math.dist((float(x),float(y)),(-328,-792))<160,arrivals
            (MOD/'diagnostics/mansion-routes-runtime.json').write_text(json.dumps(dict(arrivals=arrivals, completed=True),indent=2)+'\n')
            print('PASS: CZ zombie physically used the side stairs and reached the upper hostage zone in GoldSrc.' if args.czero_route else 'PASS: 20 actors from all four fronts physically reached their assigned doors in GoldSrc.')
            raise SystemExit(0)
        if args.door_touch:
            issued=re.findall(r'MansionDoorTouchIssued door=(\d+) targetname=(\d+)',phases)
            opened=re.findall(r'MansionDoorTouchOpened door=(\d+) angle=([\d.-]+),([\d.-]+),([\d.-]+)',phases)
            assert {int(row[0]) for row in issued}=={1,2} and all(row[1]=='0' for row in issued),(issued,phases)
            assert {int(row[0]) for row in opened}=={1,2},(opened,phases)
            assert 'MansionDoorTouchComplete both native rotating doors opened through their player touch callback' in phases,phases
            (MOD/'diagnostics/mansion-door-touch-runtime.json').write_text(json.dumps(dict(issued=issued,opened=opened,completed=True),indent=2)+'\n')
            print('PASS: fake GoldSrc player touch opened both original rotating doors with targetname unset.')
            raise SystemExit(0)
        spawns = re.findall(r'MansionSpawn wave=(\d+) index=(\d+) ct=(\d+) type=(\S+)', phases)
        assert len(spawns) == 244, len(spawns)
        randy_models=re.findall(r'MansionSpawn wave=\d+ index=\d+ ct=\d+ type=monster_randy .*?model=(\S+)', phases)
        assert len(randy_models)==30 and all(model=='models/cremation/hotd2/randy/randy.mdl' for model in randy_models),randy_models
        kinds = ['monster_zombie','monster_max','monster_randy','monster_poisionzombie','monster_human_grunt']
        compositions = []
        for wave in range(1,5):
            rows = [(int(index),int(ct),kind) for w,index,ct,kind in spawns if int(w)==wave]
            special_kind='judgment' if wave==4 else 'monster_czero_zombie'
            special=[row for row in rows if row[2]==special_kind]
            assert len(special)==1 and 0<=special[0][1]<40 and special[0][0] in (40,80),special
            rows=[row for row in rows if row[2]!=special_kind]
            counts = collections.Counter(kind for index,ct,kind in rows)
            expected = []
            for index in range(40 if wave<=2 else 80):
                sector,round_number=index%4,index//4
                kind = (0 if wave==1 else (round_number+sector)%2 if wave==2 else
                        (round_number+sector)%4 if wave==3 else
                        4 if round_number%2 else (round_number//2+sector)%4)
                expected.append('monster_johnny' if index<(10 if wave==1 else 15 if wave==2 else 0) else kinds[kind])
            assert [kind for index,ct,kind in rows]==expected, (wave,counts,collections.Counter(expected))
            compositions.append(dict(counts))
            if wave < 4:
                assert [ct for index,ct,kind in rows] == [i%40 for i in range(len(rows))]
            else:
                assert all(0 <= ct < 40 for index,ct,kind in rows), rows
                role_counts=re.search(r'MansionCultistRoles perimeter=(\d+) door=(\d+) backdoor=(\d+)',phases)
                assert role_counts and tuple(map(int,role_counts.groups()))==(20,18,2),phases
                cultist_roles=re.findall(r'MansionCultistSpawn index=(\d+) role=(perimeter|door|backdoor) ct=(\d+)',phases)
                assert len(cultist_roles)==40 and len({int(index) for index,role,ct in cultist_roles})==40,cultist_roles
                assert collections.Counter(role for index,role,ct in cultist_roles)=={'perimeter':20,'door':18,'backdoor':2},cultist_roles
        assert 'MansionError' not in phases and 'MansionPrecacheCreateFailed' not in phases and 'MansionCreateFailed' not in phases
        assert 'JudgmentSpawn wave=4' in phases and 'shared_hp=1500' in phases
        assert phases.count('JudgmentEncounter horde_alive=0 begin_boss_phase') == 1
        result = dict(map='cs_mansion', native_spawns=243, czero_npcs=3, judgment_pairs=1, judgment_actors=2,
                      judgment_shared_hp=1500, kuarl_fall_hp=90, native_damage_deaths=True,
                      compositions=compositions, round_robin_ct_waves_1_3=True, wave4_cultist_spawns_random=True,
                      cultist_roles=dict(perimeter=20,door=18,backdoor=2), completed=True,
                      interactive_visual_acceptance=False)
        (MOD/'diagnostics/mansion-runtime.json').write_text(json.dumps(result,indent=2)+'\n')
        print('PASS: 243 horde/CZero spawns plus Judgment pair; compositions and all four waves completed in GoldSrc.')
    finally:
        if process.poll() is None:
            process.terminate()
        process.wait(timeout=5)
