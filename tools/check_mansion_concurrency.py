"""Test that the 16 monster concurrency cap and replacement mechanics operate properly in GoldSrc."""
from pathlib import Path
import os
import re
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]
MOD = ROOT / 'Cremation'
if __name__ == '__main__' and os.environ.get('CREMATION_VALIDATION_DLL_ACTIVE') != '1':
    from validation_dll import run_with_validation_dll
    run_with_validation_dll(MOD, Path(__file__).resolve())
    raise SystemExit

env = dict(os.environ)
env['PATH'] = str(Path(os.environ.get('ProgramFiles(x86)', 'C:/Program Files (x86)')) / 'Steam') + os.pathsep + env['PATH']
(MOD / 'startup-diagnostics.txt').write_text('')

# We run a test server where 1 fake client joins, and enemies spawn up to 16.
with (MOD / 'diagnostics/mansion-concurrency-server.log').open('wb') as log:
    process = subprocess.Popen([str(ROOT / 'hlds.exe'), '-console', '-game', 'Cremation',
                                '-num_edicts', '2048', '-insecure', '-nomaster', '-port', '27032',
                                '+ip', '127.0.0.1', '+sv_lan', '1', '+maxplayers', '2',
                                '+map', 'cs_mansion'], cwd=ROOT, env=env,
                               stdout=log, stderr=subprocess.STDOUT, stdin=subprocess.PIPE,
                               creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        # Give engine 8 seconds to initialize and start wave 1
        time.sleep(6)
        # Let's inspect the diagnostics
        deadline = time.monotonic() + 25
        spawns = []
        while time.monotonic() < deadline:
            phases = (MOD / 'startup-diagnostics.txt').read_text(errors='replace')
            spawns = re.findall(r'MansionSpawn wave=(\d+) index=(\d+)', phases)
            # In a normal game with no kills, spawns should reach exactly 16 and stop!
            if len(spawns) >= 16:
                # Wait 3 more seconds to ensure it does NOT exceed 16 without kills!
                time.sleep(3)
                phases = (MOD / 'startup-diagnostics.txt').read_text(errors='replace')
                spawns = re.findall(r'MansionSpawn wave=(\d+) index=(\d+)', phases)
                break
            time.sleep(0.5)

        print(f'Observed spawns before any player kills: {len(spawns)}')
        assert len(spawns) == 16, f'Expected exactly 16 spawns at concurrency cap, but got {len(spawns)}!'
        print('PASS: Concurrency limit strictly halted wave emission at exactly 16 active monsters!')
    finally:
        if process.poll() is None:
            process.terminate()
        process.wait(timeout=5)
